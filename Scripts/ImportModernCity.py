"""Import the prepared local city into a new, non-overwritten preview map."""
import json
import os
from pathlib import Path
import re
import sys
import unreal

sys.path.insert(0, str(Path(__file__).parent))
from CityPbrMaterials import build_materials, assign_materials
from CitySpawnSearch import find_open_spawn

project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
prepared = Path(os.environ['CITYLIVE_CITY_PREPARED'])
report = json.loads((prepared / 'manifest.json').read_text(encoding='utf-8'))
if report.get('version', 0) < 2:
    raise RuntimeError('City must be prepared again to preserve original PBR materials')
token = prepared.name
if not re.fullmatch(r'[0-9_]+', token):
    raise RuntimeError('Invalid import run token')
# Unique destination preserves edited maps and enables safe retry after a failed run.
root = '/Game/CityLiveMaps/ModernCity/Run_' + token
map_path = root + '/CityLive_ModernCity'
assets = unreal.AssetToolsHelpers.get_asset_tools()
meshes = []
for skipped in report.get('skipped_parts', []):
    unreal.log_warning('City part skipped: ' + skipped['name'] + ' — ' + skipped['reason'])
for part in report['parts']:
    task = unreal.AssetImportTask()
    task.filename = part['fbx']
    task.destination_path = root + '/' + part['name']
    task.automated = True
    task.save = True
    task.replace_existing = False
    task.factory = unreal.FbxFactory()
    options = unreal.FbxImportUI()
    options.automated_import_should_detect_type = False
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    options.import_as_skeletal = False
    options.import_materials = False
    options.import_textures = False
    options.import_animations = False
    static = options.static_mesh_import_data
    static.set_editor_property('combine_meshes', False)
    static.set_editor_property('transform_vertex_to_absolute', True)
    static.set_editor_property('convert_scene', True)
    static.set_editor_property('convert_scene_unit', True)
    static.set_editor_property('auto_generate_collision', False)
    task.options = options
    assets.import_asset_tasks([task])
    found = []
    for path in unreal.EditorAssetLibrary.list_assets(task.destination_path, recursive=True):
        obj = unreal.load_asset(path)
        if isinstance(obj, unreal.StaticMesh):
            found.append(obj)
        elif isinstance(obj, unreal.Texture2D):
            obj.set_editor_property('max_texture_size', 2048)
            unreal.EditorAssetLibrary.save_loaded_asset(obj, only_if_is_dirty=False)
    if not found:
        raise RuntimeError('No imported meshes for ' + part['name'])
    materials = build_materials(part['materials'], task.destination_path)
    for mesh in found:
        assign_materials(mesh, materials)
    meshes.extend((part['name'], mesh) for mesh in found)

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not levels.new_level(map_path):
    raise RuntimeError('Cannot create city map')
road_actor = None
for group, mesh in meshes:
    # Complex static collision avoids convex hulls sealing courtyards and roads.
    body = mesh.get_editor_property('body_setup')
    if not body:
        raise RuntimeError('Missing BodySetup for ' + mesh.get_path_name())
    body.set_editor_property('collision_trace_flag', unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    if not unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False):
        raise RuntimeError('Cannot save mesh ' + mesh.get_path_name())
    actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, 0))
    actor.set_actor_label(group + '_' + mesh.get_name())
    component = actor.static_mesh_component
    component.set_static_mesh(mesh)
    component.set_editor_property('mobility', unreal.ComponentMobility.STATIC)
    component.set_collision_profile_name('BlockAll')
    if group == 'Road' and 'traffic' not in mesh.get_name().lower():
        road_actor = actor
if not road_actor:
    raise RuntimeError('Road geometry not found; cannot place spawn')
# Find a floor point on the imported road with a line trace. Do not guess from
# the merged mesh centre: the centre can be an empty courtyard.
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
origin, extent = road_actor.get_actor_bounds(False)
def floor_at(x, y):
    result = unreal.SystemLibrary.line_trace_single(
        world, unreal.Vector(x, y, origin.z+extent.z+2000),
        unreal.Vector(x, y, origin.z-extent.z-2000), unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
        True, [], unreal.DrawDebugTrace.NONE, True)
    if result is None:
        return None
    values = result.to_tuple()
    point, normal = values[5], values[7]
    if values[9] != road_actor or normal.z < .98 or abs(point.z-origin.z) > 100:
        return None
    return point.z


def corridor_clear(x, y, z, dx, dy):
    for height, radius in ((55, 35), (145, 45)):
        result = unreal.SystemLibrary.sphere_trace_single(
            world, unreal.Vector(x, y, z+height), unreal.Vector(x+dx, y+dy, z+height),
            radius, unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True, [],
            unreal.DrawDebugTrace.NONE, True)
        if result is not None:
            return False
    return True

x, y, z = find_open_spawn((origin.x, origin.y), (extent.x, extent.y), floor_at, corridor_clear)
spawn = unreal.Vector(x, y, z+110)
unreal.log('CITYLIVE_OPEN_SPAWN ' + str(spawn))
actors.spawn_actor_from_class(unreal.PlayerStart, spawn)
mode = unreal.load_class(None, '/Game/ThirdPerson/Blueprints/BP_ThirdPersonGameMode.BP_ThirdPersonGameMode_C')
if not mode:
    raise RuntimeError('Game mode missing')
world.get_world_settings().set_editor_property('default_game_mode', mode)
sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 6000), unreal.Rotator(-45, -25, 0))
sun.light_component.set_editor_property('mobility', unreal.ComponentMobility.MOVABLE)
sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 5000))
sky.light_component.set_editor_property('mobility', unreal.ComponentMobility.MOVABLE)
sky.light_component.set_editor_property('real_time_capture', True)
actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
if not levels.save_current_level():
    raise RuntimeError('Cannot save city map')
# Activate only after import, collision, spawn and map save have succeeded.
ini = project / 'Config/DefaultEngine.ini'
text = ini.read_text(encoding='utf-8')
for key in ('GameDefaultMap', 'EditorStartupMap'):
    text = re.sub(r'^'+key+r'=.*$', key+'='+map_path+'.CityLive_ModernCity', text, flags=re.MULTILINE)
ini.write_text(text, encoding='utf-8')
(project / 'Saved/ModernCityMap.txt').write_text(map_path, encoding='utf-8')
(prepared / 'import.ok').write_text(map_path, encoding='utf-8')
unreal.log('CITYLIVE_MODERN_CITY_IMPORTED ' + map_path)
