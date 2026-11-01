#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build-host/media140/flac-fixture
ffmpeg -hide_banner -loglevel error -f lavfi -i 'sine=frequency=440:sample_rate=44100:duration=4' -ac 2 -c:a flac -sample_fmt s32 -metadata title='FLAC title' -metadata artist='FLAC artist' -y build-host/media140/flac-fixture/track.flac
ffmpeg -hide_banner -loglevel error -f lavfi -i 'sine=frequency=440:sample_rate=48000:duration=4' -ac 2 -c:a aac -metadata title='Test title' -metadata artist='Test artist' -metadata album='Test album' -y build-host/media140/aac.m4a
ffmpeg -hide_banner -loglevel error -f lavfi -i 'sine=frequency=1000:sample_rate=32000:duration=4' -ac 2 -c:a alac -metadata artist='Test artist' -metadata album='Test album' -y build-host/media140/alac.m4a
ffmpeg -hide_banner -loglevel error -f lavfi -i 'sine=frequency=440:duration=4' -ac 2 -y build-host/media140/test.mp3
python3 - <<'PY'
from PIL import Image
from pathlib import Path
import shutil
Image.new('RGB',(32,32),(80,140,200)).save('build-host/media140/cover.png')
shutil.rmtree('build-host/media140/config',ignore_errors=True)
PY
make -f Makefile.host -j4 build-host/features140 build-host/app140
python3 tests/radio_fixture.py build-host/media140/test.mp3 > build-host/radio-fixture.log 2>&1 &
fixture_pid=$!
trap 'kill "$fixture_pid" 2>/dev/null || true' EXIT
python3 - <<'PY'
import socket,time
for _ in range(100):
 try:
  socket.create_connection(('127.0.0.1',18766),.1).close();break
 except OSError:time.sleep(.01)
else:raise SystemExit('Radio fixture failed to start')
PY
./build-host/features140 "$PWD/build-host/media140"
./build-host/app140 "$PWD/build-host/media140"
