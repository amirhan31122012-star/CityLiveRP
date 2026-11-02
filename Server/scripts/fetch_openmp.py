"""Download the pinned official open.mp server; keep existing deployment files."""
import argparse,hashlib,shutil,tarfile,tempfile,urllib.request
from pathlib import Path
URL='https://github.com/openmultiplayer/open.mp/releases/download/v1.6.7.3218/open.mp-linux-x86.tar.gz'
SHA256='0f7bfe70bb68ea780fe24a910b69412046e53d10feb65a9d4a9de8ff1f7f1329'
p=argparse.ArgumentParser();p.add_argument('destination',type=Path);a=p.parse_args();a.destination.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory() as temporary:
 root=Path(temporary);archive=root/'server.tar.gz'
 with urllib.request.urlopen(URL,timeout=30) as response,archive.open('wb') as file:
  total=0
  while chunk:=response.read(1024*1024):
   total+=len(chunk)
   if total>100*1024*1024:raise SystemExit('Unexpected archive size')
   file.write(chunk)
 if hashlib.sha256(archive.read_bytes()).hexdigest()!=SHA256:raise SystemExit('Official server checksum mismatch')
 with tarfile.open(archive) as tar:tar.extractall(root/'unpacked',filter='data')
 source=root/'unpacked/Server'
 for file in source.rglob('*'):
  if not file.is_file():continue
  relative=file.relative_to(source)
  if relative.parts[0]=='qawno':continue
  target=a.destination/relative
  if target.exists():continue
  target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(file,target)
print('Official open.mp downloaded and checksum verified. Existing files preserved.')
