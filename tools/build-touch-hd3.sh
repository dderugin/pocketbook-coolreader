#!/usr/bin/env bash
# Build on Linux x86_64 with SDK-B288-6.10 (modern glibc/libpng16 firmware).
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cd "$root"
export POCKETBOOK_SDK="${POCKETBOOK_SDK:-$root/work/sdk/SDK-B288-6.10}"
if [[ -d "$root/work/host/usr/bin" ]]; then
    export PATH="$root/work/host/usr/bin:$PATH"
    export LD_LIBRARY_PATH="$root/work/host/usr/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
fi
build="$root/work/build-touch-hd3-fw6"
cmake -S "$root" -B "$build" \
    -DCMAKE_TOOLCHAIN_FILE="$root/tools/toolchain-touch-hd3.cmake" \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_SKIP_RPATH=ON \
    -DCMAKE_C_FLAGS_RELEASE='-O2 -g -DNDEBUG' \
    -DCMAKE_CXX_FLAGS_RELEASE='-O2 -g -DNDEBUG -fno-strict-aliasing -fno-delete-null-pointer-checks' \
    -DGUI=CRGUI_PB -DDEVICE_NAME=pb360 -DTARGET_TYPE=ARM \
    -DMAX_IMAGE_SCALE_MUL=2 -DPOCKETBOOK_PRO=ON \
    -DPOCKETBOOK_PRO_FW5=ON -DPOCKETBOOK_FW6=ON \
    -DBACKGROUND_CACHE_FILE_CREATION=ON -DENABLE_PB_DB_STATE=OFF \
    -DCR3_JPEG=ON -DCR3_PNG=OFF -DWITH_HARFBUZZ=OFF
cmake --build "$build" --parallel "${JOBS:-$(nproc)}"
"$POCKETBOOK_SDK/usr/bin/arm-obreey-linux-gnueabi-objcopy" --only-keep-debug "$build/cr3gui/cr3-pb.app" "$build/cr3gui/cr3-pb.debug"
"$POCKETBOOK_SDK/usr/bin/arm-obreey-linux-gnueabi-strip" "$build/cr3gui/cr3-pb.app"
python3 "$root/tools/package-touch-hd3.py" "$build"
