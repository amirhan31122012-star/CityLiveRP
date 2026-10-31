"""Create the lightweight boot map and runtime artwork after the editor build."""
import os
import re
from pathlib import Path
import unreal

project=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
folder=Path(os.environ['CITYLIVE_LOADING_DIR'])
art=project/'Assets/Branding/CityLiveLoadingCity.png'
task=unreal.AssetImportTask()
task.filename=str(art);task.destination_path='/Game/CityLiveUI';task.destination_name='T_LoadingCity'
task.automated=True;task.replace_existing=True;task.save=True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
texture=unreal.load_asset('/Game/CityLiveUI/T_LoadingCity.T_LoadingCity')
if not isinstance(texture,unreal.Texture2D):raise RuntimeError('Cannot import loading artwork')
texture.set_editor_property('never_stream',True)
texture.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
texture.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
if not unreal.EditorAssetLibrary.save_loaded_asset(texture,only_if_is_dirty=False):raise RuntimeError('Cannot save loading artwork')
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
boot='/Game/CityLiveMaps/CityLive_Boot'
if unreal.EditorAssetLibrary.does_asset_exist(boot):
    if not levels.load_level(boot):raise RuntimeError('Cannot load boot map')
elif not levels.new_level(boot):raise RuntimeError('Cannot create boot map')
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
mode=unreal.load_class(None,'/Script/CityLiveRP.CityBootGameMode')
if not mode:raise RuntimeError('Loading classes missing; build the project first')
world.get_world_settings().set_editor_property('default_game_mode',mode)
if not levels.save_current_level():raise RuntimeError('Cannot save boot map')
city=(project/'Saved/ModernCityMap.txt').read_text(encoding='utf-8').strip()
if not re.fullmatch(r'/Game/CityLiveMaps/ModernCity/Run_[0-9_]+/CityLive_ModernCity',city):raise RuntimeError('Invalid city map marker')
game_ini=project/'Config/DefaultGame.ini'
text=game_ini.read_text(encoding='utf-8')
if len(re.findall(r'^CityMapPath=[^\r\n]*',text,flags=re.MULTILINE))!=1:raise RuntimeError('Update DefaultGame.ini from the repository')
text=re.sub(r'^CityMapPath=[^\r\n]*','CityMapPath='+city,text,flags=re.MULTILINE)
game_ini.write_text(text,encoding='utf-8')
engine_ini=project/'Config/DefaultEngine.ini';text=engine_ini.read_text(encoding='utf-8')
text=re.sub(r'^GameDefaultMap=[^\r\n]*','GameDefaultMap='+boot+'.CityLive_Boot',text,flags=re.MULTILINE)
if 'GameInstanceClass=/Script/CityLiveRP.CityLoadingGameInstance' not in text:
    raise RuntimeError('Update DefaultEngine.ini to include CityLoadingGameInstance')
engine_ini.write_text(text,encoding='utf-8')
folder.mkdir(parents=True,exist_ok=True)
(folder/'loading.ok').write_text(boot,encoding='utf-8')
unreal.log('CITYLIVE_BOOT_READY')
