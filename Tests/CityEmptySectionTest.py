"""Regression: an empty optional GLTF must not abort the whole city export.
Run with Python; needs Blender on PATH. Originals stay inside a temporary folder.
"""
import json
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[1] / 'Scripts/PrepareCityBlender.py'
PARTS = ('Road', 'Sidewalk', 'Parking', 'Green Area', 'Building', 'Towers')


def fixture(root, empty):
    for part in PARTS:
        folder = root / (part + '_unpacked_joined_GLTF')
        folder.mkdir(parents=True)
        scene = {'asset': {'version': '2.0'}, 'scene': 0, 'scenes': [{'nodes': []}]}
        if part not in empty:
            binary = struct.pack('<9f3H', 0, 0, 0, 2, 0, 0, 0, 0, 2, 0, 1, 2)
            (folder / 'part.bin').write_bytes(binary)
            scene.update({
                'scenes': [{'nodes': [0]}], 'nodes': [{'mesh': 0}],
                'meshes': [{'primitives': [{'attributes': {'POSITION': 0}, 'indices': 1}]}],
                'buffers': [{'uri': 'part.bin', 'byteLength': len(binary)}],
                'bufferViews': [{'buffer': 0, 'byteLength': 36},
                                {'buffer': 0, 'byteOffset': 36, 'byteLength': 6}],
                'accessors': [{'bufferView': 0, 'componentType': 5126, 'count': 3,
                               'type': 'VEC3', 'min': [0, 0, 0], 'max': [2, 0, 2]},
                              {'bufferView': 1, 'componentType': 5123, 'count': 3, 'type': 'SCALAR'}]})
        (folder / 'part.gltf').write_text(json.dumps(scene))


@unittest.skipUnless(shutil.which('blender'), 'Blender not installed')
class EmptySectionTest(unittest.TestCase):
    def run_case(self, empty, success):
        with tempfile.TemporaryDirectory() as temp:
            source, output = Path(temp) / 'source', Path(temp) / 'output'
            fixture(source, empty)
            before = {p.relative_to(source): p.read_bytes() for p in source.rglob('*') if p.is_file()}
            result = subprocess.run(['blender', '--background', '--python-exit-code', '1',
                                     '--python', str(SCRIPT), '--', str(source), str(output)],
                                    capture_output=True, text=True, timeout=90)
            self.assertEqual(result.returncode == 0, success, result.stdout + result.stderr)
            self.assertEqual(before, {p.relative_to(source): p.read_bytes()
                                     for p in source.rglob('*') if p.is_file()})
            if success:
                report = json.loads((output / 'manifest.json').read_text())
                self.assertEqual({p['name'] for p in report['skipped_parts']}, empty)
                self.assertEqual(len(report['parts']), 6 - len(empty))
                for part in report['parts']:
                    self.assertTrue(Path(part['fbx']).is_file())
                self.assertIn('CITYLIVE_CITY_PREPARED', result.stdout)
            else:
                self.assertFalse((output / 'manifest.json').exists())
                self.assertIn('required city part', result.stdout + result.stderr)

    def test_empty_optional_sections_keep_remaining_parts(self):
        self.run_case({'Green Area', 'Towers'}, True)

    def test_empty_road_is_still_an_error(self):
        self.run_case({'Road'}, False)


if __name__ == '__main__':
    unittest.main()
