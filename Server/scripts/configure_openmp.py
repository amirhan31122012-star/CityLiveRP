"""Configure our C++ mode; keep existing private keys and unrelated settings."""
import argparse,json,shutil
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('config',type=Path);p.add_argument('--port',type=int,default=7777);a=p.parse_args()
if not 1<=a.port<=65535:raise SystemExit('Port must be 1..65535')
m=json.loads(a.config.read_text());backup=a.config.with_name(a.config.name+'.before-citylive')
if backup.exists():raise SystemExit('Backup exists: review it before configuring again')
shutil.copy2(a.config,backup)
m['name']='CityLive RP';m['max_players']=100;m['network']['port']=a.port;m['network']['allow_037_clients']=True
m['pawn']['main_scripts']=['citylive 1'];m['pawn']['side_scripts']=[]
plugins=[x for x in m['pawn'].get('legacy_plugins',[]) if x not in ('citylive','citylive.so')];plugins.append('citylive');m['pawn']['legacy_plugins']=plugins
m['rcon']['enable']=False;a.config.write_text(json.dumps(m,ensure_ascii=False,indent=2))
print('Configured CityLive RP, 100 slots, UDP port '+str(a.port)+'. RCON disabled; backup saved.')
