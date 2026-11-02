"""Import the scooter with explicit coloured PBR materials, including on reimport."""
import unreal,json,hashlib
from pathlib import Path
project=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
source=project/'Assets/Vehicles/Courier';manifest=json.loads((source/'manifest.json').read_text())
if hashlib.sha256((source/'SM_CityScooter.fbx').read_bytes()).hexdigest()!=manifest['mesh_sha256']:
    raise RuntimeError('Scooter mesh does not match its manifest')
root='/Game/CityLiveVehicles/Courier';tools=unreal.AssetToolsHelpers.get_asset_tools();edit=unreal.MaterialEditingLibrary;library=unreal.EditorAssetLibrary
# UE preserves the old mesh's slots during atomic reimport (including "light").
# This material layout gets its own asset; never replace the legacy mesh.
mesh_name='SM_CityScooter_v2';asset_path=root+'/'+mesh_name
marker=project/'Saved/CourierScooter.json'
marker.unlink(missing_ok=True)
materials={}
for key,values in manifest['materials'].items():
    name='M_Scooter_'+key;material=unreal.load_asset(root+'/'+name)
    if not material:material=tools.create_asset(name,root,unreal.Material,unreal.MaterialFactoryNew())
    edit.delete_all_material_expressions(material)
    colour=edit.create_material_expression(material,unreal.MaterialExpressionConstant3Vector)
    colour.set_editor_property('constant',unreal.LinearColor(*values['color']))
    edit.connect_material_property(colour,'',unreal.MaterialProperty.MP_BASE_COLOR)
    for value,prop in [('roughness',unreal.MaterialProperty.MP_ROUGHNESS),('metallic',unreal.MaterialProperty.MP_METALLIC)]:
        node=edit.create_material_expression(material,unreal.MaterialExpressionConstant);node.set_editor_property('r',values[value]);edit.connect_material_property(node,'',prop)
    if values.get('emission'):
        glow=edit.create_material_expression(material,unreal.MaterialExpressionConstant3Vector)
        glow.set_editor_property('constant',unreal.LinearColor(*(v*values['emission'] for v in values['color'][:3]),1))
        edit.connect_material_property(glow,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    edit.recompile_material(material)
    if not library.save_loaded_asset(material,only_if_is_dirty=False):raise RuntimeError('Cannot save '+name)
    materials['Scooter_'+key]=material
task=unreal.AssetImportTask();task.filename=str(source/'SM_CityScooter.fbx');task.destination_path=root;task.destination_name=mesh_name
task.automated=True;task.save=False;task.replace_existing=True;task.factory=unreal.FbxFactory()
options=unreal.FbxImportUI();options.automated_import_should_detect_type=False
options.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;options.import_as_skeletal=False
options.import_materials=False;options.import_textures=False
options.static_mesh_import_data.set_editor_property('combine_meshes',True)
options.static_mesh_import_data.set_editor_property('convert_scene_unit',True)
options.static_mesh_import_data.set_editor_property('auto_generate_collision',False)
task.options=options;tools.import_asset_tasks([task]);mesh=unreal.load_asset(asset_path)
if not isinstance(mesh,unreal.StaticMesh):raise RuntimeError('Cannot import courier scooter')
slots=list(mesh.get_editor_property('static_materials'))
bindings=[]
for slot in slots:
    key=str(slot.get_editor_property('imported_material_slot_name'))
    if key not in materials:key=str(slot.get_editor_property('material_slot_name'))
    if key not in materials:raise RuntimeError('Unknown scooter material slot '+key)
    bindings.append(key)
if len(bindings)!=len(materials) or set(bindings)!=set(materials):
    raise RuntimeError('Scooter material layout differs from the current FBX')
for i,key in enumerate(bindings):mesh.set_material(i,materials[key])
if not library.save_loaded_asset(mesh,only_if_is_dirty=False):raise RuntimeError('Cannot save scooter')
marker.write_text(json.dumps(dict(revision=manifest['revision'],import_revision=3,asset_path=asset_path,mesh_sha256=manifest['mesh_sha256'])),encoding='utf-8')
unreal.log('CITYLIVE_DETAILED_SCOOTER_READY')
