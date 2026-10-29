"""Install the CC BY male skin and its textures from the repository."""
import os
from pathlib import Path
import unreal

project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
source = project / 'Assets/Characters/Beige'
destination = '/Game/CityLiveCharacters/Beige'
mesh_path = destination + '/SK_CityBeige_v2'
tools = unreal.AssetToolsHelpers.get_asset_tools()

def import_file(filename, name, options=None):
    task = unreal.AssetImportTask()
    task.filename = str(source / filename)
    task.destination_path = destination
    task.destination_name = name
    task.automated = True
    task.save = True
    task.replace_existing = False
    if options:
        task.factory = unreal.FbxFactory()
        task.options = options
    tools.import_asset_tasks([task])
    result = unreal.load_asset(destination + '/' + name)
    if not result:
        raise RuntimeError('Asset import failed: ' + filename)
    return result

mesh = unreal.load_asset(mesh_path)
if not mesh:
    options = unreal.FbxImportUI()
    options.automated_import_should_detect_type = False
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
    options.import_as_skeletal = True
    options.import_mesh = True
    options.import_animations = False
    options.import_materials = False
    options.import_textures = False
    options.create_physics_asset = False
    options.skeletal_mesh_import_data.set_editor_property('convert_scene', True)
    options.skeletal_mesh_import_data.set_editor_property('convert_scene_unit', True)
    options.skeletal_mesh_import_data.set_editor_property('normal_import_method', unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
    mesh = import_file('SK_CityBeige.fbx', 'SK_CityBeige_v2', options)
if not isinstance(mesh, unreal.SkeletalMesh):
    raise RuntimeError('Character asset is not a skeletal mesh.')
skeleton = mesh.get_editor_property('skeleton')
if not skeleton or not unreal.EditorAssetLibrary.save_loaded_asset(skeleton, only_if_is_dirty=False):
    raise RuntimeError('Character Skeleton is missing or cannot be saved.')
probe = unreal.PoseableMeshComponent()
probe.set_skinned_asset_and_update(mesh)
for bone in ('pelvis', 'head', 'hand_l', 'hand_r', 'foot_l', 'foot_r'):
    if probe.get_bone_index(bone) < 0:
        raise RuntimeError('Required character bone missing: ' + bone)

material_path = destination + '/M_CityBeige'
material = unreal.load_asset(material_path)
if not material:
    base = unreal.load_asset(destination + '/T_Beige_Base') or import_file('Image_1.png', 'T_Beige_Base')
    normal = unreal.load_asset(destination + '/T_Beige_Normal') or import_file('Image_3.png', 'T_Beige_Normal')
    packed = unreal.load_asset(destination + '/T_Beige_ORM') or import_file('Image_0.png', 'T_Beige_ORM')
    normal.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_NORMALMAP)
    normal.set_editor_property('srgb', False)
    normal.set_editor_property('flip_green_channel', True)
    packed.set_editor_property('srgb', False)
    base.set_editor_property('max_texture_size', 2048)
    normal.set_editor_property('max_texture_size', 2048)
    packed.set_editor_property('max_texture_size', 1024)
    for texture in (base, normal, packed):
        unreal.EditorAssetLibrary.save_loaded_asset(texture)
    material = tools.create_asset('M_CityBeige', destination, unreal.Material, unreal.MaterialFactoryNew())
    edit = unreal.MaterialEditingLibrary
    for texture, sampler, channel, prop in (
        (base, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, 'RGB', unreal.MaterialProperty.MP_BASE_COLOR),
        (normal, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, 'RGB', unreal.MaterialProperty.MP_NORMAL),
        (packed, unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR, 'G', unreal.MaterialProperty.MP_ROUGHNESS),
    ):
        node = edit.create_material_expression(material, unreal.MaterialExpressionTextureSample)
        node.set_editor_property('texture', texture)
        node.set_editor_property('sampler_type', sampler)
        if not edit.connect_material_property(node, channel, prop):
            raise RuntimeError('Could not connect character material texture: ' + str(prop))
    edit.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material):
        raise RuntimeError('Character material could not be saved.')
slots = list(mesh.get_editor_property('materials'))
if not slots:
    raise RuntimeError('Character mesh has no material slots.')
for slot in slots:
    slot.set_editor_property('material_interface', material)
mesh.set_editor_property('materials', slots)
if not unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False):
    raise RuntimeError('Character mesh could not be saved.')
token = os.environ.get('CITYLIVE_SKIN_TOKEN', '')
if token and all(c.isdigit() or c == '_' for c in token):
    folder = project / 'Saved/CharacterImport'
    folder.mkdir(parents=True, exist_ok=True)
    (folder / ('Import_' + token + '.ok')).write_text(mesh_path, encoding='utf-8')
    (folder / 'Installed_v2.ok').write_text(mesh_path, encoding='utf-8')
unreal.log('CityLive: male skin installed; run BuildAndOpen.cmd.')
