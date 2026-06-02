#!/usr/bin/env bash
#
# Regenerate the vcpkg overlay ports needed to cross-compile the SDK on Linux.

set -euo pipefail
cd "$(dirname "$0")/../.."

UPSTREAM=External/vcpkg/ports
OVERLAY=cmake/vcpkg-ports-cross
PATCHES=cmake/scripts/overlay-patches

PATCHED_PORTS=(fmt sentry-native)

for p in "${PATCHED_PORTS[@]}"; do
    src="$UPSTREAM/$p"
    dst="$OVERLAY/$p"
    patch="$PATCHES/$p.patch"

    if [ ! -d "$src" ]; then
        echo "error: $src not found -- run 'git submodule update --init External/vcpkg'" >&2
        exit 1
    fi
    if [ ! -f "$patch" ]; then
        echo "error: missing patch $patch" >&2
        exit 1
    fi

    echo ">> regenerating $dst from $src"
    rm -rf "$dst"
    cp -r "$src" "$dst"
    git apply --directory="$dst" "$patch"
done

echo
echo "Done!"
