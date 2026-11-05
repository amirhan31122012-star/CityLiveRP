"""Regression checks for the host ABI failure seen on RideHost."""
import importlib.util
import struct
import unittest
from pathlib import Path

spec = importlib.util.spec_from_file_location('city_package', Path(__file__).resolve().parents[1] / 'scripts/package.py')
package = importlib.util.module_from_spec(spec)
spec.loader.exec_module(package)


def version_report(*versions):
    return (
        "Version symbols section '.gnu.version' contains 3 entries:\n"
        "  000:   0 (*local*)       2 (GLIBC_2.34)\n"
        "Version needs section '.gnu.version_r' contains 1 entry:\n"
        "  000000: Version: 1  File: libc.so.6  Cnt: 2\n"
        + ''.join('  0x0010: Name: {}  Flags: none  Version: {}\n'.format(value, index + 2)
                  for index, value in enumerate(versions))
    )


class ElfCompatibilityTest(unittest.TestCase):
    def test_compatible_old_runtime_and_numeric_order(self):
        versions = package.parse_required_versions(version_report('GLIBC_2.4', 'GLIBC_2.17', 'GLIBC_2.27', 'GLIBC_2.9'))
        self.assertEqual(package.validate_versions(versions), '2.27')
        self.assertEqual(package.validate_versions(['GLIBC_2.2.5']), '2.2.5')

    def test_original_host_failure_is_rejected(self):
        for unsupported in ('GLIBC_2.33', 'GLIBC_2.34', 'GLIBC_2.27.1'):
            with self.subTest(version=unsupported), self.assertRaisesRegex(ValueError, 'supported baseline'):
                package.validate_versions(package.parse_required_versions(version_report('GLIBC_2.0', unsupported)))

    def test_exported_versions_do_not_count_as_imports(self):
        report = version_report('GLIBC_2.17') + "Version definition section '.gnu.version_d' contains 1 entry:\n  Name: GLIBC_9.99\n"
        self.assertEqual(package.validate_versions(package.parse_required_versions(report)), '2.17')

    def test_missing_or_malformed_versions_fail_closed(self):
        for report in ('', 'No version information found in this file.', "Version symbols section '.gnu.version':\n  Name: GLIBC_2.27\n", "Version needs section '.gnu.version_r':\n  Name:\n"):
            with self.subTest(report=report), self.assertRaises(ValueError):
                package.parse_required_versions(report)
        for version in ('GLIBC_PRIVATE', 'GLIBC_ABI_DT_RELR', 'GLIBC_2.bad', 'GLIBC_2', 'GLIBC_2.27.0.extra'):
            with self.subTest(version=version), self.assertRaises(ValueError):
                package.validate_versions([version])
        with self.assertRaises(ValueError):
            package.validate_versions(['OTHER_ABI_1.0'])

    def test_external_cpp_runtime_fails(self):
        for version in ('GLIBCXX_3.4.26', 'CXXABI_1.3.11'):
            with self.subTest(version=version), self.assertRaisesRegex(ValueError, 'External C\\+\\+ runtime'):
                package.validate_versions(['GLIBC_2.27', version])

    def test_old_glibc_separate_system_libraries_are_allowed(self):
        dependencies = ('libc.so.6', 'libm.so.6', 'libpthread.so.0', 'libdl.so.2', 'librt.so.1', 'ld-linux.so.2')
        output = ''.join(' 0x00000001 (NEEDED) Shared library: [{}]\n'.format(name) for name in dependencies)
        self.assertEqual(package.parse_dependencies(output), sorted(dependencies))
        for bad in ('libstdc++.so.6', 'libcrypto.so.3', 'libssl.so.3'):
            with self.subTest(library=bad), self.assertRaisesRegex(ValueError, 'Unexpected external'):
                package.parse_dependencies(output + ' 0x00000001 (NEEDED) Shared library: [{}]\n'.format(bad))
        with self.assertRaises(ValueError):
            package.parse_dependencies('There is no dynamic section in this file.')

    def test_only_i386_shared_object_header_is_accepted(self):
        elf = bytearray(52)
        elf[:7] = b'\x7fELF\x01\x01\x01'
        struct.pack_into('<HH', elf, 16, 3, 3)
        package.validate_elf_header(elf)
        for invalid in (b'', bytes(elf[:20]), bytes(elf[:4]) + b'\x02' + bytes(elf[5:])):
            with self.subTest(header=invalid), self.assertRaises(ValueError):
                package.validate_elf_header(invalid)
        struct.pack_into('<H', elf, 16, 2)
        with self.assertRaises(ValueError):
            package.validate_elf_header(elf)


if __name__ == '__main__':
    unittest.main()
