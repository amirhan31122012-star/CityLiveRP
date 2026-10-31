"""Validate the Skylight adapter against the documented Python property names."""
import importlib.util
from pathlib import Path
import sys
import types
import unittest
from unittest.mock import patch


class Component:
    allowed={'intensity','cast_shadows','light_source_angle',
             'lower_hemisphere_is_black','lower_hemisphere_color'}
    def __init__(self):self.values={}
    def set_editor_property(self,name,value):
        if name not in self.allowed:raise AttributeError(name)
        self.values[name]=value


class LightingTest(unittest.TestCase):
    def test_supported_skylight_names_and_nonblack_fill(self):
        spec=importlib.util.spec_from_file_location('lighting_adapter',
            Path(__file__).resolve().parents[1]/'Scripts/PoliceDistrictLighting.py')
        module=importlib.util.module_from_spec(spec)
        with patch.dict(sys.modules,{'unreal':types.SimpleNamespace(LinearColor=lambda *a:a)}):
            spec.loader.exec_module(module)
        sky,sun=Component(),Component()
        module.configure_sky(sky);module.configure_sun(sun)
        self.assertTrue(sky.values['lower_hemisphere_is_black'])
        self.assertTrue(all(v>0 for v in sky.values['lower_hemisphere_color'][:3]))
        self.assertEqual(sky.values['intensity'],2.2)
        self.assertTrue(sun.values['cast_shadows'])


if __name__=='__main__':unittest.main()
