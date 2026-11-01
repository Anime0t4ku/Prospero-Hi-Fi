#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
bash tools/test_features140.sh
make -f Makefile.host -j4 build-host/player-files-test build-host/smb-startup-test
./build-host/player-files-test "$PWD/build-host/media140"
./build-host/smb-startup-test "$PWD/build-host/media140/flac-fixture/track.flac"
# The complete UI, protocol, artwork and renderer regression suite:
export HOST_EXTRA_LIBS="${HOST_EXTRA_LIBS:--lSDL2 -lSDL2_image -lSDL2_ttf -lpng -ljpeg -lcurl} ${FFMPEG_LIBS:--lavformat -lavcodec -lswresample -lavutil}"
bash tools/test_host.sh
