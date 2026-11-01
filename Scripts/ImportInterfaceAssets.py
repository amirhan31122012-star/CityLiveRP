"""Import the login artwork; map geometry itself is native vector data, not a capture."""
import unreal,json,hashlib
from pathlib import Path
project=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
art=project/'Assets/Branding/CityLiveLoginCity.png'
task=unreal.AssetImportTask();task.filename=str(art);task.destination_path='/Game/CityLiveUI';task.destination_name='T_LoginCity'
task.automated=True;task.replace_existing=True;task.save=True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
texture=unreal.load_asset('/Game/CityLiveUI/T_LoginCity')
if not isinstance(texture,unreal.Texture2D):raise RuntimeError('Cannot import login artwork')
texture.set_editor_property('never_stream',True)
texture.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
texture.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
texture.set_editor_property('srgb',True)
if not unreal.EditorAssetLibrary.save_loaded_asset(texture,only_if_is_dirty=False):raise RuntimeError('Cannot save login artwork')
marker=project/'Saved/CityInterface.json'
marker.write_text(json.dumps(dict(revision=1,art_sha256=hashlib.sha256(art.read_bytes()).hexdigest())),encoding='utf-8')
unreal.log('CITYLIVE_ORANGE_INTERFACE_READY')
