#!/usr/bin/env bash
# Builds the Reckless HS20 (VST3 + AU + Standalone), runs the tests and
# optionally installs the plug-ins into ~/Library/Audio/Plug-Ins.
#
#   ./build.sh            build + test
#   ./build.sh install    build + test + install
set -euo pipefail
cd "$(dirname "$0")"

git submodule update --init --recursive

GENERATOR=()
command -v ninja >/dev/null && GENERATOR=(-G Ninja)

# Some Command Line Tools installs ship an incomplete libc++ header folder;
# point the compiler at the SDK's copy when <algorithm> can't be found.
if ! echo '#include <algorithm>' | c++ -x c++ -fsyntax-only - 2>/dev/null; then
  SDK="$(xcrun --show-sdk-path)"
  export CXXFLAGS="-nostdinc++ -isystem $SDK/usr/include/c++/v1"
  export OBJCXXFLAGS="$CXXFLAGS"
fi

cmake -S . -B build "${GENERATOR[@]}" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target RecklessHS20_VST3 RecklessHS20_AU RecklessHS20_Standalone HS20Tests

./build/HS20Tests_artefacts/Release/HS20Tests

if [[ "${1:-}" == "install" ]]; then
  A="build/RecklessHS20_artefacts/Release"
  mkdir -p ~/Library/Audio/Plug-Ins/Components ~/Library/Audio/Plug-Ins/VST3
  rm -rf ~/Library/Audio/Plug-Ins/Components/"Reckless HS20.component" ~/Library/Audio/Plug-Ins/VST3/"Reckless HS20.vst3"
  cp -R "$A/AU/Reckless HS20.component" ~/Library/Audio/Plug-Ins/Components/
  cp -R "$A/VST3/Reckless HS20.vst3" ~/Library/Audio/Plug-Ins/VST3/
  killall -9 AudioComponentRegistrar 2>/dev/null || true
  echo "Installed. Rescan plug-ins in your DAW."
fi
