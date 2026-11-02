"""Preview server.cfg changes; --apply writes a backup. Keep hosting network/admin settings."""
import argparse,re,shutil
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('config',type=Path);p.add_argument('--apply',action='store_true');a=p.parse_args()
raw=a.config.read_bytes();text=raw.decode('cp1251');lines=text.splitlines();out=[];plugins=[];found=False
for line in lines:
 match=re.match(r'^\s*([^#\s]+)(?:\s+(.*))?$',line)
 if not match:out.append(line);continue
 key,value=match[1].lower(),match[2] or ''
 if re.fullmatch(r'gamemode\d+',key):
  if not found:out.append('gamemode0 citylive 1');found=True
 elif key=='plugins':plugins.extend(re.split(r'\s+//|\s+#',value,maxsplit=1)[0].split())
 else:out.append(line)
if not found:out.append('gamemode0 citylive 1')
plugins=[x for x in plugins if x not in ('citylive','citylive.so')];plugins.append('citylive.so');out.append('plugins '+' '.join(dict.fromkeys(plugins)))
# Do not print preserved passwords or other private hosting configuration.
print('gamemode0 citylive 1');print('plugins '+' '.join(dict.fromkeys(plugins)))
print('Other settings, including port, bind, maxplayers and rcon_password, are preserved.')
if a.apply:
 backup=a.config.with_name(a.config.name+'.before-citylive')
 if backup.exists():raise SystemExit('Backup exists; review it before applying again: '+str(backup))
 shutil.copy2(a.config,backup);a.config.write_bytes(('\r\n'.join(out)+'\r\n').encode('cp1251'));print('Updated; backup: '+str(backup))
else:print('Preview only; add --apply to update and create a backup.')
