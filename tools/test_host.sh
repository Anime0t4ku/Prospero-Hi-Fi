#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build-host
cxx=${CXX:-c++};cc=${CC:-cc}
read -r -a extra <<< "${HOST_EXTRA_CFLAGS:-}"
read -r -a libs <<< "${HOST_EXTRA_LIBS:--lSDL2 -lSDL2_image -lSDL2_ttf -lpng -ljpeg}"
flags=(-std=c++17 -O1 -g -pthread -Iinclude -Ithird_party -Ithird_party/libsmb2/include "${extra[@]}")
"$cc" -O2 -Iinclude -Ithird_party -Ithird_party/libsmb2/include "${extra[@]}" -c source/decoder.c -o build-host/decoder.o
"$cc" -O2 -Iinclude "${extra[@]}" -c source/cover_codec.c -o build-host/cover_codec.o
"$cxx" "${flags[@]}" -c source/core.cpp -o build-host/core.o
build_run(){ local name=$1;shift; "$cxx" "${flags[@]}" "tests/${name}_test.cpp" "$@" -o "build-host/$name"; "build-host/$name"; }
build_run core build-host/core.o build-host/decoder.o -lm -ldl
build_run natural_sort build-host/core.o
build_run badge_layout build-host/core.o "${libs[@]}"
build_run disc source/disc_metadata.cpp build-host/core.o
build_run storage_devices source/storage_devices.cpp source/optical_detection.cpp build-host/core.o
build_run layout
build_run player_navigation build-host/core.o
build_run settings_navigation build-host/core.o
build_run input_timing
build_run keyboard source/keyboard.cpp build-host/core.o "${libs[@]}"
build_run platform_network -D__PROSPERO__ source/platform.cpp "${libs[@]}"
build_run socket_write
build_run smb_lifecycle ${HOST_SMB_LIB:--lsmb2}
build_run disc_player source/player.cpp build-host/decoder.o build-host/core.o -Wl,--wrap=SDL_ClearQueuedAudio "${libs[@]}" -lm -ldl
build_run cover_codec build-host/cover_codec.o build-host/core.o "${libs[@]}"
build_run remote source/remote.cpp source/discovery.cpp source/websocket.cpp source/storage_devices.cpp source/optical_detection.cpp build-host/core.o
build_run client source/client.cpp source/remote.cpp source/discovery.cpp source/websocket.cpp source/storage_devices.cpp source/optical_detection.cpp build-host/core.o

build_run visualizer build-host/core.o build-host/decoder.o "${libs[@]}" -lm -ldl

# Exercise the actual native GPU backend against a deterministic AGC API mock.
"$cc" -std=c11 -D_POSIX_C_SOURCE=200809L -O1 -g "${extra[@]}" -Itests tests/agc_renderer_test.c "${libs[@]}" -lm -o build-host/agc_renderer
./build-host/agc_renderer
