"""Stop through the server console so the game and DB worker can finish cleanly."""
import signal,subprocess,sys,time

stopping=False

def stop(_signal,_frame):
 global stopping
 stopping=True

for event in (signal.SIGTERM,signal.SIGINT):signal.signal(event,stop)
if len(sys.argv)<2:raise SystemExit('Usage: supervise-vps.py ./omp-server [arguments]')
child=subprocess.Popen(sys.argv[1:],stdin=subprocess.PIPE,start_new_session=True)
deadline=None
try:
 while child.poll() is None:
  if stopping and deadline is None:
   print('[CityLive] Requesting a clean console shutdown.',flush=True)
   try:child.stdin.write(b'exit\n');child.stdin.flush();child.stdin.close()
   except (BrokenPipeError,OSError):pass
   deadline=time.monotonic()+35
  if deadline is not None and time.monotonic()>deadline:
   print('[CityLive] Shutdown deadline reached.',file=sys.stderr,flush=True)
   child.terminate()
   try:child.wait(timeout=3)
   except subprocess.TimeoutExpired:child.kill();child.wait()
   break
  time.sleep(.1)
finally:
 if child.poll() is None:child.kill();child.wait()
raise SystemExit(child.returncode if child.returncode>=0 else 128-child.returncode)
