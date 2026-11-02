"""Local smoke test only; run in an isolated network namespace with the test DB."""
import argparse,shutil,socket,struct,subprocess,tempfile,time,signal
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('source',type=Path);p.add_argument('--signal',action='store_true');p.add_argument('--supervised',action='store_true');a=p.parse_args()
with tempfile.TemporaryDirectory(dir='/run/citytest') as temporary:
 root=Path(temporary)/'Server';shutil.copytree(a.source,root)
 with (root/'smoke.log').open('w') as output:
  command=['/usr/bin/qemu-i386','./omp-server']
  if a.supervised:command=['python3','/src/scripts/supervise-vps.py']+command
  child=subprocess.Popen(command,cwd=root,stdin=subprocess.PIPE,stdout=output,stderr=subprocess.STDOUT)
  try:
   for _ in range(100):
    log=(root/'smoke.log').read_text(errors='replace')
    if 'MySQL connected.' in log and 'Legacy Network started' in log:break
    if child.poll() is not None:raise RuntimeError('Server exited: '+log[-1500:])
    time.sleep(.2)
   else:raise RuntimeError('Server startup timed out: '+log[-1500:])
   request=b'SAMP'+socket.inet_aton('127.0.0.1')+struct.pack('<H',17811)+b'i'
   with socket.socket(socket.AF_INET,socket.SOCK_DGRAM) as s:
    s.settimeout(3);s.sendto(request,('127.0.0.1',17811));data,_=s.recvfrom(4096)
   assert data[:11]==request and len(data)>20
   offset=16;fields=[]
   for _ in range(3):
    length=struct.unpack_from('<I',data,offset)[0];offset+=4;fields.append(data[offset:offset+length].decode('cp1251',errors='replace'));offset+=length
   assert 'CityLive' in fields[0] and 'CityLive' in fields[1],fields
   if a.signal:child.send_signal(signal.SIGTERM if a.supervised else signal.SIGINT)
   else:child.stdin.write(b'exit\n');child.stdin.flush()
   try:child.wait(timeout=10)
   except subprocess.TimeoutExpired:child.send_signal(signal.SIGINT);child.wait(timeout=5)
   assert child.returncode==0,(child.returncode,(root/'smoke.log').read_text(errors='replace')[-8000:])
   print('PASS: open.mp plugin load, C++ gamemode, MySQL connection, SA-MP query and graceful shutdown')
  finally:
   if child.poll() is None:child.kill();child.wait()
