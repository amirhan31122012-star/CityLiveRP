"""Patch the current local city for Android V1.1; keep a pre-update map backup."""
import json
import os
from pathlib import Path
import re
import sys
import unreal

sys.path.insert(0, str(Path(__file__).parent))
from CityMobilePolicy import is_decorative_vehicle, draw_distance
from CityMobileSky import install_mobile_sky

project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
map_path = (project / 'Saved/ModernCityMap.txt').read_text(encoding='utf-8').strip()
if not re.fullmatch(r'/Game/CityLiveMaps/ModernCity/Run_[0-9_]+/CityLive_ModernCity', map_path):
    raise RuntimeError('Invalid map marker')
root = map_path.rsplit('/', 1)[0]
backup = root + '/CityLive_BeforeV11'
if not unreal.EditorAssetLibrary.does_asset_exist(map_path):
    raise RuntimeError('City map missing')
if not unreal.EditorAssetLibrary.does_asset_exist(backup):
    copy = unreal.EditorAssetLibrary.duplicate_asset(map_path, backup)
    if not copy or not unreal.EditorAssetLibrary.save_loaded_asset(copy, only_if_is_dirty=False):
        raise RuntimeError('Cannot back up city map')
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not levels.load_level(map_path):
    raise RuntimeError('Cannot load city map')
removed = []
static_count = 0
for actor in list(actors.get_all_level_actors()):
    if isinstance(actor, unreal.StaticMeshActor):
        component = actor.static_mesh_component
        mesh = component.get_editor_property('static_mesh')
        name = actor.get_actor_label() + '_' + (mesh.get_name() if mesh else '')
        if is_decorative_vehicle(name):
            removed.append(name)
            if not actors.destroy_actor(actor):
                raise RuntimeError('Cannot remove decorative vehicle ' + name)
            continue
        component.set_editor_property('cast_shadow', False)
        component.set_cull_distance(draw_distance(name))
        static_count += 1
    elif isinstance(actor, unreal.DirectionalLight):
        actor.light_component.set_editor_property('cast_shadows', False)
    elif isinstance(actor, unreal.SkyLight):
        component = actor.light_component
        component.set_editor_property('real_time_capture', False)
        component.set_editor_property('cast_shadows', False)
        component.set_editor_property('intensity', 1.0)
install_mobile_sky(root, actors)
for actor in actors.get_all_level_actors():
    if isinstance(actor, unreal.SkyLight):
        actor.light_component.recapture_sky()
if not levels.save_current_level():
    raise RuntimeError('Cannot save updated city')
# Restore startup after pulling renderer settings from the repository.
ini = project / 'Config/DefaultEngine.ini'
text = ini.read_text(encoding='utf-8')
for key in ('GameDefaultMap', 'EditorStartupMap'):
    text = re.sub(r'^'+key+r'=.*$', key+'='+map_path+'.CityLive_ModernCity', text, flags=re.MULTILINE)
ini.write_text(text, encoding='utf-8')
summary = {'version': '1.1.0', 'map': map_path, 'backup': backup,
           'removed_vehicles': removed, 'static_actors': static_count, 'mobile_sky': True}
(project / 'Saved/CityV11.json').write_text(json.dumps(summary, indent=2), encoding='utf-8')
folder = project / 'Saved/CityUpdateV11' / os.environ['CITYLIVE_UPDATE_TOKEN']
folder.mkdir(parents=True, exist_ok=True)
(folder / 'complete.ok').write_text(map_path, encoding='utf-8')
unreal.log('CITYLIVE_V11_READY removed vehicles=' + str(len(removed)) + ' sky=mobile')
