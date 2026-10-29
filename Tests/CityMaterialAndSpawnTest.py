"""Pure source material and spawn selection regressions; no Unreal required."""
import json
import math
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'Scripts'))
from CityMaterialSource import material_sources
from CitySpawnSearch import find_open_spawn
from CityUvTransform import uv_rows


class CityMaterialTest(unittest.TestCase):
    def test_pbr_sources_preserve_color_uv_and_cutout(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / 'part.gltf'
            source.write_text(json.dumps({
                'images': [{'uri': 'textures/base%20color.png'}, {'uri': 'textures/packed.png'}],
                'textures': [{'source': 0}, {'source': 1}],
                'materials': [{'name': 'Paint.001', 'alphaMode': 'MASK', 'alphaCutoff': .4,
                               'doubleSided': True, 'pbrMetallicRoughness': {
                                   'baseColorFactor': [.1, .2, .3, .8],
                                   'baseColorTexture': {'index': 0, 'extensions': {
                                       'KHR_texture_transform': {'scale': [2, 3], 'offset': [.1, .2], 'rotation': math.pi / 2}}},
                                   'metallicRoughnessTexture': {'index': 1},
                                   'roughnessFactor': .7, 'metallicFactor': .2}}]}))
            material, = material_sources(source, root, 'Road')
            self.assertEqual(material['slot'], 'CL_Road_0000')
            self.assertEqual(Path(material['base']['file']), root / 'textures/base color.png')
            self.assertEqual(material['base']['scale'], [2, 3])
            self.assertEqual(material['base']['offset'], [.1, .2])
            self.assertEqual(material['base']['rotation'], math.pi / 2)
            rows = uv_rows(material['base'])
            u, v = 1, 2
            transformed = [row[0]*u + row[1]*v + offset
                           for row, offset in zip(rows, material['base']['offset'])]
            self.assertAlmostEqual(transformed[0], -5.9)
            self.assertAlmostEqual(transformed[1], 2.2)
            self.assertEqual(material['base_factor'], [.1, .2, .3, .8])
            self.assertEqual(material['alpha'], 'MASK')
            self.assertEqual(material['cutoff'], .4)
            self.assertTrue(material['two_sided'])
            self.assertEqual(material['roughness'], .7)
            self.assertEqual(material['metallic'], .2)


class CityUvTest(unittest.TestCase):
    def test_identity(self):
        self.assertEqual(uv_rows({}), ((1, 0), (0, 1)))

    def test_negative_rotation_and_mirrored_scale(self):
        rows = uv_rows({'rotation': -math.pi/2, 'scale': [-2, 3]})
        self.assertAlmostEqual(rows[0][0], 0)
        self.assertAlmostEqual(rows[0][1], 3)
        self.assertAlmostEqual(rows[1][0], 2)
        self.assertAlmostEqual(rows[1][1], 0)


class CitySpawnTest(unittest.TestCase):
    def test_open_intersection_uses_ground_height(self):
        floor = lambda x, y: 37 if abs(x) < 700 or abs(y) < 700 else None
        self.assertEqual(find_open_spawn((0, 0), (4000, 4000), floor,
                                        lambda *args: True), (0, 0, 37))

    def test_narrow_road_cannot_be_selected(self):
        floor = lambda x, y: 0 if abs(x) < 100 else None
        with self.assertRaises(RuntimeError):
            find_open_spawn((0, 0), (4000, 4000), floor, lambda *args: True)

    def test_fenced_candidate_is_rejected_for_open_area(self):
        # Open floor alone is not enough: low/high obstruction checks come from UE.
        def clear(x, y, z, dx, dy):
            # Simulate a walled strip extending 10m around the centre.
            return abs(x) > 1000 and abs(x + dx) > 1000
        x, y, z = find_open_spawn((0, 0), (4000, 4000), lambda x, y: 0, clear)
        self.assertGreater(abs(x), 1600)

    def test_fully_obstructed_map_fails_instead_of_trapping_player(self):
        with self.assertRaises(RuntimeError):
            find_open_spawn((0, 0), (4000, 4000), lambda x, y: 0, lambda *args: False)


if __name__ == '__main__':
    unittest.main()
