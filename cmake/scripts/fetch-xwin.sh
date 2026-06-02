#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."

XWIN_SDK_VERSION="${XWIN_SDK_VERSION:-10.0.26100}"

# Grab the MSVC CRT + Windows SDK for x64.
xwin --accept-license --arch x86_64 --cache-dir .xwin/cache --sdk-version "$XWIN_SDK_VERSION" \
    splat --include-debug-libs --output .xwin/splat

# Symlink additional spellings of headers / libs.
um=.xwin/splat/sdk/include/um
[ -e "$um/WS2tcpip.h" ] && ln -sf WS2tcpip.h "$um/Ws2tcpip.h"
[ -e "$um/shlobj.h" ]   && ln -sf shlobj.h   "$um/Shlobj.h"

umlib=.xwin/splat/sdk/lib/um/x86_64
[ -e "$umlib/winmm.lib" ] && ln -sf winmm.lib "$umlib/Winmm.lib"
