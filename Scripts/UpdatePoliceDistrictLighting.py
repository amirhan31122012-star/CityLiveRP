"""Update existing city lighting without reimporting geometry or materials."""
import gc
import json
import os
from pathlib import Path
import re
import sys
import unreal
sys.path.insert(0,str(Path(__file__).parent))
from PoliceDistrictLighting import configure_sun, configure_sky, REVISION

project=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
token=os.environ['CITYLIVE_POLISH_TOKEN']
if not re.fullmatch(r'[0-9_]+',token):raise RuntimeError('Invalid update token')
path=(project/'Saved/ModernCityMap.txt').read_text(encoding='utf-8').strip()
state_file=project/'Saved/CityV11.json'
state=json.loads(state_file.read_text(encoding='utf-8'))
if state.get('source')!='PoliceDistrict' or state.get('map')!=path or not re.fullmatch(r'/Game/CityLiveMaps/ModernCity/Run_[0-9_]+/CityLive_ModernCity',path):
    raise RuntimeError('Install the new district using ImportPoliceDistrict.cmd first')
ini=project/'Config/DefaultEngine.ini'
config=ini.read_text(encoding='utf-8')
for key in ('GameDefaultMap','EditorStartupMap'):
    pattern=r'^'+key+r'=[^\r\n]*'
    if len(re.findall(pattern,config,flags=re.MULTILINE))!=1:raise RuntimeError('Missing startup key '+key)
    config=re.sub(pattern,key+'='+path+'.CityLive_ModernCity',config,flags=re.MULTILINE)
backup=path.rsplit('/',1)[0]+'/CityLive_BeforePolish_'+token
library=unreal.EditorAssetLibrary
copy=library.duplicate_asset(path,backup)
if not copy or not library.save_loaded_asset(copy,only_if_is_dirty=False):raise RuntimeError('Cannot back up city')
del copy
gc.collect()
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not levels.load_level(path):raise RuntimeError('Cannot load active city')
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
suns=[a for a in actors if isinstance(a,unreal.DirectionalLight)]
skies=[a for a in actors if isinstance(a,unreal.SkyLight)]
if not suns or not skies:raise RuntimeError('City lights missing')
for a in suns:configure_sun(a.light_component)
for a in skies:
    configure_sky(a.light_component)
    a.light_component.recapture_sky()
if not levels.save_current_level():raise RuntimeError('Cannot save lighting')
ini.write_text(config,encoding='utf-8')
state['lighting_revision']=REVISION
state['lighting_backup']=backup
temporary=state_file.with_suffix('.tmp');temporary.write_text(json.dumps(state,indent=2),encoding='utf-8');temporary.replace(state_file)
folder=project/'Saved/PoliceDistrictPolish'/token
folder.mkdir(parents=True,exist_ok=True)
(folder/'complete.ok').write_text(path,encoding='utf-8')
unreal.log('CITYLIVE_LIGHTING_READY shadows=enabled ambient=2.2')
