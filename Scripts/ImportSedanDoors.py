"""Import the privately shared four-door derivative, retaining local Fab materials."""
import os
from pathlib import Path
import unreal

fbx_path = Path(os.environ.get('CITYLIVE_DOORS_FBX', ''))
if not fbx_path.is_file() or fbx_path.suffix.lower() != '.fbx':
    raise RuntimeError('Drag the prepared four-door FBX onto ImportSedanDoors.cmd.')
destination = '/Game/CityLiveVehicles/Sedan'
asset_name = 'SK_CitySedan_4Doors'
asset_path = destination + '/' + asset_name
if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
    raise RuntimeError('The four-door mesh already exists; refusing to overwrite local edits: ' + asset_path)
original = unreal.load_asset('/Game/VehicleVarietyVol2/Skeletons/SK_Sedan_01a.SK_Sedan_01a')
if not isinstance(original, unreal.SkeletalMesh):
    raise RuntimeError('Original sedan must be installed locally first, including its materials.')

options = unreal.FbxImportUI()
options.automated_import_should_detect_type = False
options.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
options.import_as_skeletal = True
options.import_mesh = True
options.import_materials = False
options.import_textures = False
options.import_animations = False
options.create_physics_asset = False
options.skeletal_mesh_import_data.set_editor_property('import_morph_targets', False)
options.skeletal_mesh_import_data.set_editor_property('convert_scene', True)
options.skeletal_mesh_import_data.set_editor_property('convert_scene_unit', True)
options.skeletal_mesh_import_data.set_editor_property('normal_import_method', unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
task = unreal.AssetImportTask()
task.filename = str(fbx_path.resolve())
task.destination_path = destination
task.destination_name = asset_name
task.automated = True
task.replace_existing = False
task.save = True
task.factory = unreal.FbxFactory()
task.options = options
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
imported = unreal.load_asset(asset_path)
if not isinstance(imported, unreal.SkeletalMesh):
    raise RuntimeError('Skeletal mesh import failed: ' + repr(task.imported_object_paths))

source_slots = list(original.get_editor_property('materials'))
new_slots = list(imported.get_editor_property('materials'))
if len(source_slots) != len(new_slots):
    raise RuntimeError('Material slot count changed; review the imported mesh before use.')
by_name = {}
for slot in source_slots:
    interface = slot.get_editor_property('material_interface')
    by_name[str(slot.get_editor_property('material_slot_name'))] = interface
    by_name[str(slot.get_editor_property('imported_material_slot_name'))] = interface
    if interface:
        by_name[interface.get_name()] = interface
for index, slot in enumerate(new_slots):
    interface = by_name.get(str(slot.get_editor_property('imported_material_slot_name')))
    if not interface:
        interface = by_name.get(str(slot.get_editor_property('material_slot_name')))
    if not interface:
        interface = source_slots[index].get_editor_property('material_interface')
    slot.set_editor_property('material_interface', interface)
imported.set_editor_property('materials', new_slots)
if not unreal.EditorAssetLibrary.save_asset(asset_path):
    raise RuntimeError('Failed to save imported material assignments.')
saved = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())) / 'VehicleImport'
saved.mkdir(parents=True, exist_ok=True)
token = os.environ.get('CITYLIVE_IMPORT_TOKEN', '')
if token and all(c.isdigit() or c == '_' for c in token):
    (saved / ('Import_' + token + '.ok')).write_text(asset_path, encoding='utf-8')
unreal.log('CityLive: four-door sedan imported with original materials: ' + asset_path)
