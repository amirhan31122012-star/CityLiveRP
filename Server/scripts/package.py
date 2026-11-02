"""Package already-built SA-MP artifacts. Never package live passwords or server executables."""
import argparse,hashlib,json,shutil,struct,zipfile
from pathlib import Path
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--plugin',type=Path,required=True);p.add_argument('--bootstrap',type=Path,required=True);args=p.parse_args()
plugin=args.plugin.read_bytes();amx=args.bootstrap.read_bytes()
if plugin[:4]!=b'\x7fELF' or plugin[4:6]!=b'\x01\x01' or struct.unpack_from('<H',plugin,18)[0]!=3:raise SystemExit('Plugin must be Linux ELF32 i386')
if len(amx)<56 or struct.unpack_from('<H',amx,4)[0]!=0xf1e0:raise SystemExit('Invalid bootstrap AMX')
output=root/'dist';output.mkdir(exist_ok=True)
files={'plugins/citylive.so':plugin,'gamemodes/citylive.amx':amx,
 'scriptfiles/citylive/database.ini.example':(root/'config/database.ini.example').read_bytes(),
 'schema.sql':(root/'config/schema.sql').read_bytes(),'server.cfg.fragment':(root/'config/server.cfg.fragment').read_bytes(),
 'INSTALL_RU.md':(root/'README.md').read_bytes(),'patch_config.py':(root/'scripts/patch_config.py').read_bytes(),
 'THIRD_PARTY.md':(root/'ThirdParty/README.md').read_bytes(),
 'RELINKING.md':(root/'RELINKING.md').read_bytes(),'MIGRATION.md':(root/'MIGRATION.md').read_bytes(),'TESTING.md':(root/'TESTING.md').read_bytes()}
for name in ('OpenSSL-COPYRIGHT.txt','zlib-COPYRIGHT.txt','GCC-runtime-COPYRIGHT.txt','samp-cpp/LICENSE','quickmaffs/LICENSE','sampgdk/LICENSE.txt','mariadb-connector/COPYING.LIB','pawn-compiler/license.txt'):
 source=root/'ThirdParty'/name
 if not source.exists():raise SystemExit('Missing dependency license: '+name)
 files['licenses/'+name.replace('/','_')]=source.read_bytes()
# Supply the source needed to rebuild/relink the statically linked LGPL connector.
source_roots=['src','tests','config','scripts','ThirdParty']
for directory in source_roots:
 for source in (root/directory).rglob('*'):
  relative=source.relative_to(root)
  if not source.is_file() or any(part in ('__pycache__','runtime','runtime-openmp') for part in relative.parts):continue
  if source.suffix in ('.log','.pyc') or source.name in ('database.ini','cidl_parser.out','cidl_parsetab.py'):continue
  files['relink-source/'+relative.as_posix()]=source.read_bytes()
for name in ('CMakeLists.txt','Dockerfile','exports.map','.dockerignore','.gitignore','.clang-format','README.md','RELINKING.md','MIGRATION.md','TESTING.md'):
 files['relink-source/'+name]=(root/name).read_bytes()
for kind in ('RideHost','VPS'):
 contents=dict(files)
 if kind=='VPS':
  for source,name in [('config/citylive.service','citylive.service'),('scripts/start-vps.sh','start-vps.sh'),('scripts/supervise-vps.py','supervise-vps.py'),('scripts/fetch_openmp.py','fetch_openmp.py'),('scripts/configure_openmp.py','configure_openmp.py')]:contents[name]=(root/source).read_bytes()
 manifest={'version':'0.1.0','architecture':'linux-i386','minimum_glibc':'2.34','dependency_sources':json.loads((root/'ThirdParty/sources.json').read_text()),'sha256':{name:hashlib.sha256(data).hexdigest() for name,data in contents.items()}}
 contents['manifest.json']=json.dumps(manifest,indent=2).encode()
 path=output/('CityLiveRP-'+kind+'-0.1.0.zip')
 with zipfile.ZipFile(path,'w',compression=zipfile.ZIP_DEFLATED) as archive:
  for name,data in contents.items():
   entry=zipfile.ZipInfo(name);entry.create_system=3;entry.compress_type=zipfile.ZIP_DEFLATED
   entry.external_attr=(0o100755 if name.endswith('.sh') else 0o100644)<<16
   archive.writestr(entry,data)
 print(path)
