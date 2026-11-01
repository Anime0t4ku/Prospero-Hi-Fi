#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
: "${PS5_PAYLOAD_SDK:?Set PS5_PAYLOAD_SDK}"
root="$PWD"
mkdir -p build-ps5/ffmpeg
cd build-ps5/ffmpeg
"$root/third_party/ffmpeg/configure" --prefix="$PWD/install" --target-os=freebsd --arch=x86_64 --enable-cross-compile --cc="$PS5_PAYLOAD_SDK/bin/prospero-clang" --ar="$PS5_PAYLOAD_SDK/bin/llvm-ar" --ranlib="$PS5_PAYLOAD_SDK/bin/llvm-ranlib" --disable-autodetect --disable-everything --disable-programs --disable-doc --disable-network --disable-avdevice --disable-avfilter --disable-swscale --disable-pthreads --disable-x86asm --disable-asm --disable-shared --enable-static --enable-pic --enable-small --enable-avformat --enable-avcodec --enable-avutil --enable-swresample --enable-demuxer=mov,mp3,flac,ogg,wav,aac --enable-parser=aac,mpegaudio,flac,vorbis --enable-decoder=aac,alac,mp3float,flac,vorbis,pcm_s16le,pcm_s24le,pcm_s32le,pcm_f32le --extra-cflags='-O2 -ffunction-sections -fdata-sections'
make -j4
make install
