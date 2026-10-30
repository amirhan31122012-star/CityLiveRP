"""Material graph adapter regression; run with Python, not a substitute for UE."""
import importlib.util
from pathlib import Path
import sys
import types
import unittest
from unittest.mock import patch


class Node:
    def __init__(self, kind): self.kind, self.properties = kind, {}
    def set_editor_property(self, key, value): self.properties[key] = value
    def get_class(self): return types.SimpleNamespace(get_name=lambda: self.kind)


class SkyTest(unittest.TestCase):
    def load_sky(self, unary_pin):
        material = Node('material')
        nodes, links = [], []
        state = {'clears': 0, 'creates': 0, 'saves': 0}
        def names(node):
            return ['A', 'B', 'Alpha'] if node.kind == 'Lerp' else [unary_pin]
        def clear(mat):
            nodes.clear(); links.clear(); state['clears'] += 1
        def create(mat, kind):
            node = Node(kind); nodes.append(node); return node
        def connect(source, output, target, pin):
            if pin not in names(target): return False
            links.append((source, target, pin)); return True
        def save(*args, **kwargs): state['saves'] += 1; return True
        def asset(*args): state['creates'] += 1; return material
        api = types.SimpleNamespace(
            load_asset=lambda path: material, Material=object, MaterialFactoryNew=lambda: object(),
            AssetToolsHelpers=types.SimpleNamespace(get_asset_tools=lambda: types.SimpleNamespace(create_asset=asset)),
            MaterialEditingLibrary=types.SimpleNamespace(
                delete_all_material_expressions=clear, create_material_expression=create,
                get_material_expression_input_names=names, connect_material_expressions=connect,
                connect_material_property=lambda *args: True, recompile_material=lambda mat: None),
            EditorAssetLibrary=types.SimpleNamespace(save_loaded_asset=save),
            MaterialShadingModel=types.SimpleNamespace(MSM_UNLIT='unlit'),
            MaterialProperty=types.SimpleNamespace(MP_EMISSIVE_COLOR='emissive'),
            LinearColor=lambda *args: args,
            MaterialExpressionConstant3Vector='Color', MaterialExpressionVertexNormalWS='Normal',
            MaterialExpressionComponentMask='Mask', MaterialExpressionAbs='Abs',
            MaterialExpressionLinearInterpolate='Lerp')
        spec = importlib.util.spec_from_file_location('sky_test_module',
            Path(__file__).resolve().parents[1]/'Scripts/CityMobileSky.py')
        module = importlib.util.module_from_spec(spec)
        with patch.dict(sys.modules, {'unreal': api}): spec.loader.exec_module(module)
        return module, material, nodes, links, state

    def test_unnamed_unary_pins_connect(self):
        module, material, nodes, links, state = self.load_sky('')
        self.assertIs(module.build_mobile_sky_material('/Game/Test'), material)
        self.assertEqual([link[2] for link in links], ['', '', 'A', 'B', 'Alpha'])
        self.assertTrue(material.properties['is_sky'])
        self.assertEqual(len(nodes), 6)
        self.assertEqual(state['saves'], 1)

    def test_named_unary_pins_and_repeat_rebuild(self):
        module, material, nodes, links, state = self.load_sky('Input')
        module.build_mobile_sky_material('/Game/Test')
        module.build_mobile_sky_material('/Game/Test')
        self.assertEqual([link[2] for link in links], ['Input', 'Input', 'A', 'B', 'Alpha'])
        self.assertEqual(len(nodes), 6)  # no accumulation on retry
        self.assertEqual(state['clears'], 2)
        self.assertEqual(state['saves'], 2)
        self.assertEqual(state['creates'], 0)  # repair the existing generated asset


if __name__ == '__main__': unittest.main()
