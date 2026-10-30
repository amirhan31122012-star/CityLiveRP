"""Exercise map recovery and failure handling without a local Unreal install."""
import copy
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import types
import unittest
import weakref
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'Scripts'))
CURRENT = '/Game/CityLiveMaps/ModernCity/Run_1_2_3/CityLive_ModernCity'
BASELINE = '/Game/CityLiveMaps/ModernCity/Run_1_2_3/CityLive_BeforeV11'
DESTINATION = '/Game/CityLiveMaps/ModernCity/Run_4_5_6/CityLive_ModernCity'


class MeshActor:
    def __init__(self, name, shadows=True, distance=0):
        self.name, self.shadows, self.distance = name, shadows, distance
        # No mutators: modifying V1 shadows/culling would fail this test.
        mesh = types.SimpleNamespace(get_name=lambda: name)
        self.static_mesh_component = types.SimpleNamespace(get_editor_property=lambda key: mesh)
    def get_actor_label(self): return self.name


class Light:
    def __init__(self):
        self.intensity, self.shadows, self.realtime = 2.5, True, True
        self.light_component = types.SimpleNamespace(recapture_sky=lambda: None)


class PlayableCar:
    def get_actor_label(self): return 'CitySedan'


class WorldReference:
    """A Python world wrapper must be gone when Unreal switches maps."""
    pass


class RecoveryTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.project = Path(self.temp.name)
        (self.project / 'Saved').mkdir()
        (self.project / 'Config').mkdir()
        self.marker = self.project / 'Saved/ModernCityMap.txt'
        self.state = self.project / 'Saved/CityV11.json'
        self.ini = self.project / 'Config/DefaultEngine.ini'
        self.marker.write_text(CURRENT)
        self.state.write_text(json.dumps({'version': '1.1.0', 'map': CURRENT}))
        self.ini.write_text('GameDefaultMap=' + CURRENT + '\nEditorStartupMap=' + CURRENT + '\n')
        self.maps = {CURRENT: [MeshActor('Building', False, 18000)],
                     BASELINE: [MeshActor('Building', True, 0), MeshActor('Parking_Block_Cars'),
                                MeshActor('Road'), Light(), PlayableCar()]}
        self.loaded = None
        self.can_save = True
        self.cyclic_world_reference = False
        self.world_reference = None
        def duplicate(source, target):
            self.maps[target] = copy.deepcopy(self.maps[source])
            wrapper = WorldReference()
            if self.cyclic_world_reference:
                wrapper.cycle = wrapper
            self.world_reference = weakref.ref(wrapper)
            return wrapper
        def load(path):
            # Mirrors the FPyReferenceCollector root in the user's fatal log.
            if self.world_reference() is not None:
                raise RuntimeError('World Memory Leaks: Python still references duplicated world')
            self.loaded = path
            return True
        def remove(actor): self.maps[self.loaded].remove(actor); return True
        levels = types.SimpleNamespace(load_level=load, save_current_level=lambda: self.can_save)
        actors = types.SimpleNamespace(get_all_level_actors=lambda: self.maps[self.loaded], destroy_actor=remove)
        self.api = types.SimpleNamespace(
            Paths=types.SimpleNamespace(project_dir=lambda: str(self.project),
                                        convert_relative_path_to_full=lambda path: path),
            EditorAssetLibrary=types.SimpleNamespace(does_asset_exist=lambda path: path in self.maps,
                duplicate_asset=duplicate, save_loaded_asset=lambda *args, **kwargs: True),
            LevelEditorSubsystem='levels', EditorActorSubsystem='actors',
            get_editor_subsystem=lambda kind: levels if kind == 'levels' else actors,
            StaticMeshActor=MeshActor, SkyLight=Light, log=lambda message: None)
        self.sky_calls = []
        sky = types.SimpleNamespace(install_mobile_sky=lambda root, actors: self.sky_calls.append(root))
        spec = importlib.util.spec_from_file_location('restore_test', ROOT / 'Scripts/RestoreCityV1.py')
        self.module = importlib.util.module_from_spec(spec)
        with patch.dict(sys.modules, {'unreal': self.api, 'CityMobileSky': sky}):
            spec.loader.exec_module(self.module)

    def run_restore(self):
        with patch.dict('os.environ', {'CITYLIVE_UPDATE_TOKEN': '4_5_6'}):
            self.module.main()

    def test_recovers_original_lighting_and_keeps_both_previous_maps(self):
        self.run_restore()
        self.assertEqual(self.marker.read_text(), DESTINATION)
        result = self.maps[DESTINATION]
        building = next(actor for actor in result if isinstance(actor, MeshActor) and actor.name == 'Building')
        self.assertEqual((building.shadows, building.distance), (True, 0))
        light = next(actor for actor in result if isinstance(actor, Light))
        self.assertEqual((light.intensity, light.shadows, light.realtime), (2.5, True, True))
        self.assertTrue(any(isinstance(actor, PlayableCar) for actor in result))
        self.assertFalse(any(isinstance(actor, MeshActor) and actor.name == 'Parking_Block_Cars' for actor in result))
        self.assertEqual(len(self.maps[BASELINE]), 5)
        self.assertFalse(self.maps[CURRENT][0].shadows)
        self.assertEqual(self.maps[CURRENT][0].distance, 18000)
        state = json.loads(self.state.read_text())
        self.assertEqual(state['baseline_map'], BASELINE)
        self.assertEqual(state['graphics_baseline'], 'v1')
        self.assertEqual(self.sky_calls, [DESTINATION.rsplit('/', 1)[0]])
        self.assertIn(DESTINATION, self.ini.read_text())
        self.assertTrue((self.project / 'Saved/CityUpdateV11/4_5_6/complete.ok').is_file())
        # A repeat must use the same original map, not stack edits on the latest.
        self.assertEqual(self.module.select_baseline(DESTINATION, state, self.maps.__contains__), BASELINE)

    def test_missing_backup_does_not_reuse_optimized_map(self):
        del self.maps[BASELINE]
        with self.assertRaisesRegex(RuntimeError, 'backup missing'):
            self.run_restore()
        self.assertEqual(self.marker.read_text(), CURRENT)
        self.assertNotIn(DESTINATION, self.maps)

    def test_cyclic_python_world_reference_is_collected_before_map_load(self):
        self.cyclic_world_reference = True
        self.run_restore()
        self.assertIsNone(self.world_reference())
        self.assertEqual(self.loaded, DESTINATION)

    def test_failed_save_does_not_activate_new_map_or_claim_success(self):
        self.can_save = False
        ini_before, state_before = self.ini.read_text(), self.state.read_text()
        with self.assertRaisesRegex(RuntimeError, 'Cannot save restored city'):
            self.run_restore()
        self.assertEqual(self.marker.read_text(), CURRENT)
        self.assertEqual(self.ini.read_text(), ini_before)
        self.assertEqual(self.state.read_text(), state_before)
        self.assertFalse((self.project / 'Saved/CityUpdateV11/4_5_6/complete.ok').exists())

    def test_fresh_import_can_use_current_unmodified_city(self):
        self.assertEqual(self.module.select_baseline(CURRENT, None, {CURRENT}.__contains__), CURRENT)
        with self.assertRaisesRegex(RuntimeError, 'Original V1 map missing'):
            self.module.select_baseline(CURRENT, {'map': CURRENT, 'graphics_baseline': 'v1',
                'baseline_map': BASELINE}, {CURRENT}.__contains__)


if __name__ == '__main__': unittest.main()
