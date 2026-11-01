"""Import full-screen account scenes and our district spawn-card renders."""
import unreal,json,hashlib
from pathlib import Path
project=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
assets={'T_LoginCity':'Assets/Branding/CityLiveLoginCity.png','T_AccountCity':'Assets/Branding/CityLiveAccountCity.png'}
assets.update({'T_Spawn'+name:'Assets/UI/Spawn/'+name+'.png' for name in ('Organization','Start','Last','Parking','Home')})
hashes={}
for name,relative in assets.items():
    art=project/relative
    task=unreal.AssetImportTask();task.filename=str(art);task.destination_path='/Game/CityLiveUI';task.destination_name=name
    task.automated=True;task.replace_existing=True;task.save=True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task]);texture=unreal.load_asset('/Game/CityLiveUI/'+name)
    if not isinstance(texture,unreal.Texture2D):raise RuntimeError('Cannot import '+name)
    texture.set_editor_property('never_stream',True)
    texture.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI);texture.set_editor_property('srgb',True)
    if not unreal.EditorAssetLibrary.save_loaded_asset(texture,only_if_is_dirty=False):raise RuntimeError('Cannot save '+name)
    hashes[relative]=hashlib.sha256(art.read_bytes()).hexdigest()
(project/'Saved/CityInterface.json').write_text(json.dumps(dict(revision=2,assets=hashes)),encoding='utf-8')
unreal.log('CITYLIVE_FULLSCREEN_INTERFACE_READY')
