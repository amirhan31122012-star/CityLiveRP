"""Verify all material slots are validated and applied in a single mesh edit."""
import importlib.util
from pathlib import Path
import sys
import types
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'Scripts'))
fake_unreal = types.SimpleNamespace(log=lambda message: None,
    EditorAssetLibrary=types.SimpleNamespace(save_loaded_asset=lambda *a, **k: True))
with patch.dict(sys.modules, {'unreal': fake_unreal}):
    spec = importlib.util.spec_from_file_location('city_pbr_test', ROOT/'Scripts/CityPbrMaterials.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)


class Slot:
    def __init__(self, name):
        self.fields = {'imported_material_slot_name': name, 'material_slot_name': name,
                       'material_interface': 'old'}
    def copy(self):
        clone = Slot(self.fields['material_slot_name'])
        clone.fields = self.fields.copy()
        return clone
    def get_editor_property(self, key): return self.fields[key]
    def set_editor_property(self, key, value): self.fields[key] = value


class Mesh:
    def __init__(self, count):
        self.slots = [Slot('CL_Building_%04d' % index) for index in range(count)]
        self.edits = []
    def get_name(self): return 'Building'
    def get_editor_property(self, name):
        assert name == 'static_materials'
        return self.slots
    def set_editor_property(self, name, value):
        self.edits.append((name, value))
        self.slots = value
    def set_material(self, *args): raise AssertionError('Per-slot mesh rebuild used')


class BatchTest(unittest.TestCase):
    def test_56_slots_are_assigned_in_one_edit(self):
        mesh = Mesh(56)
        materials = {module.key(slot.fields['material_slot_name']): object() for slot in mesh.slots}
        module.assign_materials(mesh, materials)
        self.assertEqual(len(mesh.edits), 1)
        self.assertEqual(mesh.edits[0][0], 'static_materials')
        for slot in mesh.slots:
            self.assertIs(slot.fields['material_interface'], materials[module.key(slot.fields['material_slot_name'])])

    def test_missing_material_does_not_partially_modify_mesh(self):
        mesh = Mesh(2)
        with self.assertRaises(RuntimeError):
            module.assign_materials(mesh, {'clbuilding0000': object()})
        self.assertEqual(mesh.edits, [])
        self.assertTrue(all(slot.fields['material_interface'] == 'old' for slot in mesh.slots))


if __name__ == '__main__': unittest.main()
