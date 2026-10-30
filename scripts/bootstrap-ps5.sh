#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p .deps/downloads .deps/unpack
sdk=.deps/downloads/ps5-payload-sdk-v0.45.zip
ports=.deps/downloads/pacbrew-v0.41.1.tar.gz
fetch(){ if [[ ! -f $1 ]];then curl -fL --retry 2 "$2" -o "$1";fi; }
fetch "$sdk" https://github.com/ps5-payload-dev/sdk/releases/download/v0.45/ps5-payload-sdk.zip
fetch "$ports" https://github.com/ps5-payload-dev/pacbrew-repo/releases/download/v0.41.1/ps5-payload-dev.tar.gz
printf '%s  %s\n' 5fac64093d0e3e69f2584ce7935f24f703f622d9681f1495c85b9f00d1291a2c "$sdk" e811bc2e4676c5bb7f80a8fe928d2985dce599996a5ca6b54da3e72699287887 "$ports" | sha256sum -c -
tar -xzf "$ports" -C .deps/unpack
mkdir -p .deps/ps5-payload-sdk
cp -a .deps/unpack/opt/ps5-payload-sdk/. .deps/ps5-payload-sdk/
unzip -qo "$sdk" -d .deps
printf 'Dependencies ready. Set PS5_PAYLOAD_SDK=%s/.deps/ps5-payload-sdk and run make.\n' "$PWD"
