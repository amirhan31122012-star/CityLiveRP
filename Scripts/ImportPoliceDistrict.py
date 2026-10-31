"""Install the approved authored city as the active level, preserving old assets."""
import copy
import hashlib
import json
import math
import os
from pathlib import Path
import re
import sys
import unreal

sys.path.insert(0,str(Path(__file__).parent))
from PoliceDistrictPolicy import validate_package, package_file, imported_kind
from CityPbrMaterials import build_materials, assign_materials
from CitySpawnSearch import find_open_spawn
from CityMobileSky import install_mobile_sky
from PoliceDistrictLighting import configure_sun, configure_sky, REVISION


def atomic(path,text):
    temp=path.with_name(path.name+'.tmp');temp.write_text(text,encoding='utf-8');temp.replace(path)


def main():
    project=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
    token=os.environ['CITYLIVE_DISTRICT_TOKEN']
    if not re.fullmatch(r'[0-9_]+',token): raise RuntimeError('Invalid district import token')
    logs=project/'Saved/PoliceDistrictImport'/token;logs.mkdir(parents=True,exist_ok=True)
    package=project/'Assets/Maps/PoliceDistrictGame'
    manifest=json.loads((package/'manifest.json').read_text(encoding='utf-8'))
    validate_package(package,manifest)
    # Retain the existing marker format used by Android packaging and startup.
    root='/Game/CityLiveMaps/ModernCity/Run_'+token
    map_path=root+'/CityLive_ModernCity'
    library=unreal.EditorAssetLibrary
    if library.does_asset_exist(map_path): raise RuntimeError('Destination exists; run the importer again')
    marker=project/'Saved/ModernCityMap.txt'
    mobile_marker=project/'Saved/CityV11.json'
    previous={'map':marker.read_text(encoding='utf-8').strip() if marker.is_file() else None,
              'mobile_state':json.loads(mobile_marker.read_text(encoding='utf-8')) if mobile_marker.is_file() else None}
    ini=project/'Config/DefaultEngine.ini';config=ini.read_text(encoding='utf-8')
    for key in ('GameDefaultMap','EditorStartupMap'):
        pattern=r'^'+key+r'=[^\r\n]*'
        if len(re.findall(pattern,config,flags=re.MULTILINE))!=1: raise RuntimeError('Missing startup key '+key)
        config=re.sub(pattern,key+'='+map_path+'.CityLive_ModernCity',config,flags=re.MULTILINE)
    records=copy.deepcopy(manifest['materials'])
    for record in records:
        for role in ('base','normal','packed','emissive'):
            if record.get(role):record[role]['file']=str(package_file(package,record[role]['file']))
    task=unreal.AssetImportTask()
    task.filename=str(package_file(package,manifest['fbx']))
    task.destination_path=root+'/PoliceDistrict'
    task.automated=True;task.save=True;task.replace_existing=False;task.factory=unreal.FbxFactory()
    options=unreal.FbxImportUI()
    options.automated_import_should_detect_type=False
    options.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
    options.import_as_skeletal=False;options.import_materials=False;options.import_textures=False;options.import_animations=False
    data=options.static_mesh_import_data
    for key,value in (('combine_meshes',False),('transform_vertex_to_absolute',True),
                      ('convert_scene',True),('convert_scene_unit',True),('auto_generate_collision',False)):
        data.set_editor_property(key,value)
    task.options=options
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    meshes=[]
    for path in library.list_assets(task.destination_path,recursive=True):
        asset=unreal.load_asset(path)
        if isinstance(asset,unreal.StaticMesh):meshes.append(asset)
    if len(meshes)!=manifest['mesh_count']:raise RuntimeError('Imported mesh count mismatch: '+str(len(meshes)))
    # These are purpose-baked 1K/2K textures, not the old unbounded Fab city.
    materials=build_materials(records,task.destination_path,texture_budget=False)
    for mesh in meshes:
        imported_kind(mesh.get_name())  # reject unexpected assets before activation
        assign_materials(mesh,materials)
        body=mesh.get_editor_property('body_setup')
        if not body:raise RuntimeError('Missing collision body: '+mesh.get_name())
        body.set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        if not library.save_loaded_asset(mesh,only_if_is_dirty=False):raise RuntimeError('Cannot save mesh '+mesh.get_name())
    levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not levels.new_level(map_path):raise RuntimeError('Cannot create district level')
    roads=[];landmarks=[];all_actors=[]
    for mesh in meshes:
        kind=imported_kind(mesh.get_name())
        actor=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,0))
        actor.set_actor_label(mesh.get_name())
        component=actor.static_mesh_component;component.set_static_mesh(mesh)
        component.set_editor_property('mobility',unreal.ComponentMobility.STATIC)
        component.set_collision_profile_name('NoCollision' if kind=='D' else 'BlockAll')
        all_actors.append(actor)
        if kind=='R':roads.append(actor)
        if kind=='P':landmarks.append(actor)
    if not roads or not landmarks:raise RuntimeError('Road or police landmark missing')

    def bounds(items):
        low=[float('inf')]*3;high=[-float('inf')]*3
        for actor in items:
            origin,extent=actor.get_actor_bounds(False)
            for index,axis in enumerate(('x','y','z')):
                low[index]=min(low[index],getattr(origin,axis)-getattr(extent,axis))
                high[index]=max(high[index],getattr(origin,axis)+getattr(extent,axis))
        return low,high
    low,high=bounds(roads)
    if not 30000<max(high[0]-low[0],high[1]-low[1])<50000:
        raise RuntimeError('District scale is incorrect; expected about 410 metres')
    police_low,police_high=bounds(landmarks)
    target=unreal.Vector(*[(a+b)/2 for a,b in zip(police_low,police_high)])
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()

    def floor_at(x,y):
        hit=unreal.SystemLibrary.line_trace_single(world,unreal.Vector(x,y,5000),unreal.Vector(x,y,-500),
             unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,True,[],unreal.DrawDebugTrace.NONE,True)
        if hit is None:return None
        values=hit.to_tuple();point,normal,actor=values[5],values[7],values[9]
        if actor not in roads or normal.z<.98:return None
        return point.z

    def clear(x,y,z,dx,dy):
        for height,radius in ((55,35),(145,45)):
            hit=unreal.SystemLibrary.sphere_trace_single(world,unreal.Vector(x,y,z+height),
                unreal.Vector(x+dx,y+dy,z+height),radius,unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
                True,[],unreal.DrawDebugTrace.NONE,True)
            if hit is not None:return False
        return True
    x,y,z=find_open_spawn((target.x,target.y),(18000,13000),floor_at,clear)
    spawn=unreal.Vector(x,y,z+110)
    yaw=math.degrees(math.atan2(target.y-y,target.x-x))
    actors.spawn_actor_from_class(unreal.PlayerStart,spawn,unreal.Rotator(0,yaw,0))
    unreal.log('CITYLIVE_POLICE_SPAWN '+str(spawn))
    mode=unreal.load_class(None,'/Game/ThirdPerson/Blueprints/BP_ThirdPersonGameMode.BP_ThirdPersonGameMode_C')
    if not mode:raise RuntimeError('Gameplay mode missing')
    world.get_world_settings().set_editor_property('default_game_mode',mode)
    sun=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,6000),unreal.Rotator(-38,-35,0))
    sun.light_component.set_editor_property('mobility',unreal.ComponentMobility.MOVABLE)
    configure_sun(sun.light_component)
    sky=actors.spawn_actor_from_class(unreal.SkyLight,unreal.Vector(0,0,5000))
    sky.light_component.set_editor_property('mobility',unreal.ComponentMobility.MOVABLE)
    sky.light_component.set_editor_property('real_time_capture',False)
    configure_sky(sky.light_component)
    install_mobile_sky(root,actors)
    sky.light_component.recapture_sky()
    if not levels.save_current_level():raise RuntimeError('Cannot save district map')
    summary={'version':'1.1.2','graphics_baseline':'v1','mobile_sky':True,'map':map_path,
             'lighting_revision':REVISION,
             'baseline_map':map_path,'previous_map':previous['map'],'source':'PoliceDistrict',
             'package_hash':hashlib.sha256((package/'manifest.json').read_bytes()).hexdigest(),
             'source_hash':manifest['source_blend_sha256'],'triangles':manifest['triangles']}
    atomic(logs/'previous.json',json.dumps(previous,indent=2))
    atomic(project/'Saved/PoliceDistrictPrevious.json',json.dumps(previous,indent=2))
    atomic(mobile_marker,json.dumps(summary,indent=2))
    atomic(ini,config)
    atomic(marker,map_path)
    atomic(logs/'complete.ok',map_path)
    unreal.log('CITYLIVE_POLICE_DISTRICT_ACTIVE '+map_path)


if __name__=='__main__':main()
