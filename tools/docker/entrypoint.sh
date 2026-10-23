#!/usr/bin/env bash
set -euo pipefail
cd /src
export GIT_CONFIG_COUNT=1 GIT_CONFIG_KEY_0=safe.directory GIT_CONFIG_VALUE_0=/src
python3 tools/docker/bootstrap.py
# Leave caches and exported files owned by the invoking user on Linux too.
# Extracted SDK/runtime are read-only inputs to the unprivileged build.
mkdir -p /src/work/home
chown "${CR3_HOST_UID}:${CR3_HOST_GID}" /src/work /src/work/home
export HOME=/src/work/home XDG_CONFIG_HOME=/src/work/home/.config
export POCKETBOOK_SDK=/src/work/sdk/SDK-B288-6.10
export CR3_BUILD_DIR=/src/work/build-docker-touch-hd3
export CR3_QEMU=/usr/bin/qemu-arm
setpriv --reuid="$CR3_HOST_UID" --regid="$CR3_HOST_GID" --clear-groups \
    bash tools/build-touch-hd3.sh
# Export matching reports/debug symbols alongside the install ZIP, using the
# same user identity as packaging. These are not installed on the reader.
setpriv --reuid="$CR3_HOST_UID" --regid="$CR3_HOST_GID" --clear-groups \
    python3 tools/docker/export_results.py
