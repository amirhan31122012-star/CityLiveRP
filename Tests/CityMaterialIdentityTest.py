"""Regression for duplicated/renamed glTF material names such as Fence."""
import json
from pathlib import Path
import sys
import tempfile
import unittest
from urllib.parse import unquote

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'Scripts'))
from CityMaterialIdentity import stage_scene, bind_materials


class MaterialIdentityTest(unittest.TestCase):
    def test_same_names_have_different_ids_and_source_is_unchanged(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / 'source folder'
            output = root / 'Saved'
            source.mkdir(); output.mkdir()
            (source / 'textures').mkdir()
            (source / 'textures/fence paint.png').write_bytes(b'texture fixture')
            (source / 'scene.bin').write_bytes(b'bin fixture')
            scene = {'asset': {'version': '2.0'},
                     'materials': [{'name': 'Fence'}, {'name': 'Fence'},
                                   {'name': 'Fence|' + 'a'*300}],
                     'buffers': [{'uri': 'scene.bin'}],
                     'images': [{'uri': 'textures/fence%20paint.png'}]}
            file = source / 'scene.gltf'
            file.write_text(json.dumps(scene))
            original = file.read_bytes()
            staged = stage_scene(file, output, 'Building')
            result = json.loads(staged.read_text())
            self.assertEqual([m['name'] for m in result['materials']],
                             ['CL_Building_0000', 'CL_Building_0001', 'CL_Building_0002'])
            self.assertEqual(file.read_bytes(), original)
            image_path = (staged.parent / unquote(result['images'][0]['uri'])).resolve()
            self.assertEqual(image_path, source / 'textures/fence paint.png')
            self.assertEqual(image_path.read_bytes(), b'texture fixture')

    def test_blender_variants_share_one_source_id(self):
        class Material:
            def __init__(self, name): self.name = name
        class Object:
            name = 'fence'
            class Data: pass
            def __init__(self, materials):
                self.data = self.Data()
                self.data.materials = materials
        a = Material('CL_Building_0000')
        b = Material('CL_Building_0000.001')
        objects = [Object([b, a])]
        records = [{'slot': 'CL_Building_0000', 'original_name': 'Fence'}]
        self.assertEqual(bind_materials(objects, records), records)
        self.assertIs(objects[0].data.materials[0], objects[0].data.materials[1])


if __name__ == '__main__':
    unittest.main()
