#!/bin/sh
set -eu
cd "$(dirname "$0")"
if [ ! -x ./samp03svr ] && [ ! -x ./omp-server ]; then
 echo 'Install the official open.mp server or your licensed SA-MP server first.' >&2
 exit 1
fi
if [ ! -f scriptfiles/citylive/database.ini ]; then
 echo 'Configure scriptfiles/citylive/database.ini and import schema.sql first.' >&2
 exit 1
fi
if [ -x ./omp-server ]; then exec python3 ./supervise-vps.py ./omp-server; fi
exec python3 ./supervise-vps.py ./samp03svr
