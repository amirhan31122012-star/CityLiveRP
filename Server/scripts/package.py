"""Package SA-MP artifacts only after auditing their imported ELF ABI versions."""
import argparse
import hashlib
import json
import os
import re
import struct
import subprocess
import zipfile
from pathlib import Path

VERSION = '0.1.1'
GLIBC_BASELINE = '2.27'
SYSTEM_LIBRARIES = frozenset((
    'libc.so.6', 'libm.so.6', 'ld-linux.so.2', 'libpthread.so.0', 'libdl.so.2', 'librt.so.1',
))


def validate_elf_header(plugin):
    if (len(plugin) < 52 or plugin[:7] != b'\x7fELF\x01\x01\x01'
            or struct.unpack_from('<H', plugin, 16)[0] != 3
            or struct.unpack_from('<H', plugin, 18)[0] != 3):
        raise ValueError('Plugin must be a Linux ELF32 little-endian i386 shared object')


def parse_required_versions(output):
    """Read .gnu.version_r only: exported versions do not constrain the host loader."""
    in_needs = False
    versions = []
    for line in output.splitlines():
        if re.match(r'^Version needs section\b', line):
            in_needs = True
        elif re.match(r'^Version (?:symbols|definition) section\b', line):
            in_needs = False
        elif in_needs:
            match = re.search(r'\bName:\s*(\S+)', line)
            if match:
                versions.append(match.group(1))
    if not versions:
        raise ValueError('ELF version-import audit found no version requirements')
    return sorted(set(versions))


def validate_versions(versions):
    glibc = []
    for version in versions:
        if version.startswith(('GLIBCXX_', 'CXXABI_')):
            raise ValueError('External C++ runtime ABI is not allowed: ' + version)
        if version.startswith('GLIBC_'):
            number = version[len('GLIBC_'):]
            if not re.fullmatch(r'[0-9]+\.[0-9]+(?:\.[0-9]+)?', number):
                raise ValueError('Unsupported glibc ABI requirement: ' + version)
            parts = tuple(int(value) for value in number.split('.'))
            glibc.append((parts + (0,) * (3 - len(parts)), number))
    if not glibc:
        raise ValueError('ELF version-import audit found no GLIBC requirements')
    maximum, label = max(glibc)
    if maximum > (2, 27, 0):
        raise ValueError('Plugin requires GLIBC_' + label + '; supported baseline is GLIBC_' + GLIBC_BASELINE)
    return label


def parse_dependencies(output):
    libraries = sorted(set(re.findall(r'\(NEEDED\).*?Shared library:\s*\[([^\]]+)\]', output)))
    if not libraries or 'libc.so.6' not in libraries:
        raise ValueError('ELF dependency audit found no libc dependency')
    unexpected = set(libraries) - SYSTEM_LIBRARIES
    if unexpected:
        raise ValueError('Unexpected external libraries: ' + ', '.join(sorted(unexpected)))
    return libraries


def readelf(path, option):
    try:
        return subprocess.run(
            ['readelf', option, '--wide', str(path)], stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            universal_newlines=True, check=True, env=dict(os.environ, LC_ALL='C'),
        ).stdout
    except FileNotFoundError:
        raise ValueError('readelf is required for the binary compatibility audit (install binutils)')
    except subprocess.CalledProcessError as error:
        raise ValueError('readelf could not audit the plugin: ' + error.stderr.strip())


def audit_plugin(path):
    validate_elf_header(path.read_bytes())
    versions = parse_required_versions(readelf(path, '--version-info'))
    return {
        'required_glibc_max': validate_versions(versions),
        'imported_symbol_versions': versions,
        'dynamic_dependencies': parse_dependencies(readelf(path, '--dynamic')),
    }


def main():
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--plugin', type=Path, required=True)
    parser.add_argument('--bootstrap', type=Path, required=True)
    args = parser.parse_args()
    try:
        audit = audit_plugin(args.plugin)
    except (ValueError, OSError) as error:
        raise SystemExit(str(error))
    plugin = args.plugin.read_bytes()
    amx = args.bootstrap.read_bytes()
    if len(amx) < 56 or struct.unpack_from('<H', amx, 4)[0] != 0xf1e0:
        raise SystemExit('Invalid bootstrap AMX')
    output = root / 'dist'
    output.mkdir(exist_ok=True)
    files = {
        'plugins/citylive.so': plugin,
        'gamemodes/citylive.amx': amx,
        'scriptfiles/citylive/database.ini.example': (root / 'config/database.ini.example').read_bytes(),
        'schema.sql': (root / 'config/schema.sql').read_bytes(),
        'server.cfg.fragment': (root / 'config/server.cfg.fragment').read_bytes(),
        'INSTALL_RU.md': (root / 'README.md').read_bytes(),
        'patch_config.py': (root / 'scripts/patch_config.py').read_bytes(),
        'THIRD_PARTY.md': (root / 'ThirdParty/README.md').read_bytes(),
        'RELINKING.md': (root / 'RELINKING.md').read_bytes(),
        'MIGRATION.md': (root / 'MIGRATION.md').read_bytes(),
        'TESTING.md': (root / 'TESTING.md').read_bytes(),
    }
    for name in (
        'OpenSSL-COPYRIGHT.txt', 'OpenSSL-NOTICE.txt', 'zlib-COPYRIGHT.txt', 'GCC-runtime-COPYRIGHT.txt',
        'samp-cpp/LICENSE', 'quickmaffs/LICENSE', 'sampgdk/LICENSE.txt',
        'mariadb-connector/COPYING.LIB', 'pawn-compiler/license.txt',
    ):
        source = root / 'ThirdParty' / name
        if not source.exists():
            raise SystemExit('Missing dependency license: ' + name)
        files['licenses/' + name.replace('/', '_')] = source.read_bytes()
    # Supply the source needed to rebuild/relink the statically linked LGPL connector.
    for directory in ('src', 'tests', 'config', 'scripts', 'ThirdParty'):
        for source in (root / directory).rglob('*'):
            relative = source.relative_to(root)
            if not source.is_file() or any(part in ('__pycache__', 'runtime', 'runtime-openmp') for part in relative.parts):
                continue
            if source.suffix in ('.log', '.pyc') or source.name in ('database.ini', 'cidl_parser.out', 'cidl_parsetab.py'):
                continue
            files['relink-source/' + relative.as_posix()] = source.read_bytes()
    for name in (
        'CMakeLists.txt', 'Dockerfile', 'exports.map', '.dockerignore', '.gitignore', '.clang-format',
        'README.md', 'RELINKING.md', 'MIGRATION.md', 'TESTING.md',
    ):
        files['relink-source/' + name] = (root / name).read_bytes()
    for kind in ('RideHost', 'VPS'):
        contents = dict(files)
        if kind == 'VPS':
            for source, name in (
                ('config/citylive.service', 'citylive.service'),
                ('scripts/start-vps.sh', 'start-vps.sh'),
                ('scripts/supervise-vps.py', 'supervise-vps.py'),
                ('scripts/fetch_openmp.py', 'fetch_openmp.py'),
                ('scripts/configure_openmp.py', 'configure_openmp.py'),
            ):
                contents[name] = (root / source).read_bytes()
        manifest = {
            'version': VERSION, 'architecture': 'linux-i386', 'minimum_glibc': GLIBC_BASELINE,
            'dependency_sources': json.loads((root / 'ThirdParty/sources.json').read_text()),
            'sha256': {name: hashlib.sha256(data).hexdigest() for name, data in contents.items()},
        }
        manifest.update(audit)
        contents['manifest.json'] = json.dumps(manifest, indent=2).encode()
        path = output / ('CityLiveRP-' + kind + '-' + VERSION + '.zip')
        with zipfile.ZipFile(path, 'w', compression=zipfile.ZIP_DEFLATED) as archive:
            for name, data in contents.items():
                entry = zipfile.ZipInfo(name)
                entry.create_system = 3
                entry.compress_type = zipfile.ZIP_DEFLATED
                entry.external_attr = (0o100755 if name.endswith('.sh') else 0o100644) << 16
                archive.writestr(entry, data)
        print(path)


if __name__ == '__main__':
    main()
