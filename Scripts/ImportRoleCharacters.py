"""Install the original wardrobe meshes. Materials are explicit, never depend on FBX texture paths."""
import unreal,json,hashlib
from pathlib import Path
project=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
source=project/'Assets/Characters/Roles';manifest=json.loads((source/'manifest.json').read_text())
tools=unreal.AssetToolsHelpers.get_asset_tools();library=unreal.EditorAssetLibrary;edit=unreal.MaterialEditingLibrary
root='/Game/CityLiveCharacters/Roles';palette=manifest['roles'][0]['palette']
materials={}
base=unreal.load_asset('/Game/CityLiveCharacters/Beige/T_Beige_Base')
if not base:raise RuntimeError('Install the original character first')
for name in ['Skin']+list(palette):
    material=unreal.load_asset(root+'/M_Role_'+name)
    if not material:
        material=tools.create_asset('M_Role_'+name,root,unreal.Material,unreal.MaterialFactoryNew())
        edit.set_material_usage(material,unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)
        if name=='Skin':
            node=edit.create_material_expression(material,unreal.MaterialExpressionTextureSample)
            node.set_editor_property('texture',base);node.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
            edit.connect_material_property(node,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
        else:
            node=edit.create_material_expression(material,unreal.MaterialExpressionConstant3Vector)
            node.set_editor_property('constant',unreal.LinearColor(*palette[name]));edit.connect_material_property(node,'',unreal.MaterialProperty.MP_BASE_COLOR)
        rough=edit.create_material_expression(material,unreal.MaterialExpressionConstant);rough.set_editor_property('r',.95 if name in ('Hair','Ginger','GrayHair','Beard') else .35 if name in ('Steel','Gold') else .68)
        edit.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
        if name in ('Hair','Ginger','GrayHair','Beard'):
            spec=edit.create_material_expression(material,unreal.MaterialExpressionConstant);spec.set_editor_property('r',0)
            edit.connect_material_property(spec,'',unreal.MaterialProperty.MP_SPECULAR)
        if name in ('Steel','Gold'):
            metal=edit.create_material_expression(material,unreal.MaterialExpressionConstant);metal.set_editor_property('r',.7)
            edit.connect_material_property(metal,'',unreal.MaterialProperty.MP_METALLIC)
        edit.recompile_material(material)
    if not library.save_loaded_asset(material,only_if_is_dirty=False):raise RuntimeError('Cannot save material '+name)
    materials['Role_'+name]=material
for role in ('Civilian','Police','Medic','Courier'):
    name='T_Role_'+role
    task=unreal.AssetImportTask();task.filename=str(source/(name+'.png'));task.destination_path=root;task.destination_name=name
    task.automated=True;task.save=True;task.replace_existing=True
    tools.import_asset_tasks([task]);texture=unreal.load_asset(root+'/'+name)
    if not isinstance(texture,unreal.Texture2D):raise RuntimeError('Wardrobe texture missing: '+name)
    texture.set_editor_property('srgb',True)
    if not library.save_loaded_asset(texture,only_if_is_dirty=False):raise RuntimeError('Cannot save '+name)
    material=unreal.load_asset(root+'/M_Role_'+role)
    if not material:
        material=tools.create_asset('M_Role_'+role,root,unreal.Material,unreal.MaterialFactoryNew())
        edit.set_material_usage(material,unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)
        sample=edit.create_material_expression(material,unreal.MaterialExpressionTextureSample)
        sample.set_editor_property('texture',texture);sample.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
        edit.connect_material_property(sample,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
        rough=edit.create_material_expression(material,unreal.MaterialExpressionConstant);rough.set_editor_property('r',.68)
        edit.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
        edit.recompile_material(material)
    if not library.save_loaded_asset(material,only_if_is_dirty=False):raise RuntimeError('Cannot save role material '+role)
    materials['Role_'+role]=material

def install_texture(name,linear=False):
    task=unreal.AssetImportTask();task.filename=str(source/(name+'.png'));task.destination_path=root;task.destination_name=name
    task.automated=True;task.save=True;task.replace_existing=True;tools.import_asset_tasks([task])
    texture=unreal.load_asset(root+'/'+name)
    if not isinstance(texture,unreal.Texture2D):raise RuntimeError('Face texture missing: '+name)
    texture.set_editor_property('srgb',not linear)
    texture.set_editor_property('max_texture_size',1024)
    if linear:texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_MASKS)
    if not library.save_loaded_asset(texture,only_if_is_dirty=False):raise RuntimeError('Cannot save '+name)
    return texture

def textured_material(name,colour,roughness,specular):
    material=unreal.load_asset(root+'/M_'+name)
    if not material:material=tools.create_asset('M_'+name,root,unreal.Material,unreal.MaterialFactoryNew())
    edit.delete_all_material_expressions(material)
    edit.set_material_usage(material,unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)
    sample=edit.create_material_expression(material,unreal.MaterialExpressionTextureSample)
    sample.set_editor_property('texture',colour);sample.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    edit.connect_material_property(sample,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
    if isinstance(roughness,float):
        rough=edit.create_material_expression(material,unreal.MaterialExpressionConstant);rough.set_editor_property('r',roughness)
        channel=''
    else:
        rough=edit.create_material_expression(material,unreal.MaterialExpressionTextureSample)
        rough.set_editor_property('texture',roughness);rough.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
        channel='R'
    edit.connect_material_property(rough,channel,unreal.MaterialProperty.MP_ROUGHNESS)
    if specular is None:
        edit.connect_material_property(rough,'G',unreal.MaterialProperty.MP_SPECULAR)
    else:
        spec=edit.create_material_expression(material,unreal.MaterialExpressionConstant);spec.set_editor_property('r',specular)
        edit.connect_material_property(spec,'',unreal.MaterialProperty.MP_SPECULAR)
    edit.recompile_material(material)
    if not library.save_loaded_asset(material,only_if_is_dirty=False):raise RuntimeError('Cannot save '+name)
    materials[name]=material

for role in ('Civilian','Police','Medic','Courier'):
    textured_material('Role_Face_'+role,install_texture('T_Face_'+role),install_texture('T_FaceRough_'+role,linear=True),None)
textured_material('Role_Eyes',install_texture('T_Role_Eyes'),.26,.30)
skeleton=None
for role in ('Civilian','Police','Medic','Courier','CourierPack'):
    name='SK_City'+role
    task=unreal.AssetImportTask();task.filename=str(source/(name+'.fbx'));task.destination_path=root;task.destination_name=name
    task.automated=True;task.save=True;task.replace_existing=True;task.factory=unreal.FbxFactory()
    options=unreal.FbxImportUI();options.automated_import_should_detect_type=False
    options.mesh_type_to_import=unreal.FBXImportType.FBXIT_SKELETAL_MESH;options.import_as_skeletal=True
    options.import_mesh=True;options.import_animations=False;options.import_materials=False;options.import_textures=False;options.create_physics_asset=False
    if skeleton:options.skeleton=skeleton
    options.skeletal_mesh_import_data.set_editor_property('convert_scene',True)
    options.skeletal_mesh_import_data.set_editor_property('convert_scene_unit',True)
    options.skeletal_mesh_import_data.set_editor_property('normal_import_method',unreal.FBXNormalImportMethod.FBXNIM_COMPUTE_NORMALS)
    task.options=options;tools.import_asset_tasks([task]);mesh=unreal.load_asset(root+'/'+name)
    if not isinstance(mesh,unreal.SkeletalMesh):raise RuntimeError('Role import failed: '+name)
    if not skeleton:skeleton=mesh.get_editor_property('skeleton')
    if not skeleton:raise RuntimeError('Role skeleton missing')
    slots=list(mesh.get_editor_property('materials'))
    for slot in slots:
        key=str(slot.get_editor_property('imported_material_slot_name'))
        if key not in materials:key=str(slot.get_editor_property('material_slot_name'))
        if key not in materials:raise RuntimeError('Unknown wardrobe material '+key)
        slot.set_editor_property('material_interface',materials[key])
    mesh.set_editor_property('materials',slots)
    if not library.save_loaded_asset(mesh,only_if_is_dirty=False):raise RuntimeError('Cannot save '+name)
if not library.save_loaded_asset(skeleton,only_if_is_dirty=False):raise RuntimeError('Cannot save role skeleton')
marker=project/'Saved/RoleCharacters/Installed.json';marker.parent.mkdir(parents=True,exist_ok=True)
marker.write_text(json.dumps(dict(face_revision=manifest['face_revision'],package_hash=hashlib.sha256((source/'manifest.json').read_bytes()).hexdigest())),encoding='utf-8')
unreal.log('CITYLIVE_ROLE_CHARACTERS_READY')
