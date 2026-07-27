#!/usr/bin/env bash

exec env -i \
    PATH="$PATH" \
    HOME="$HOME" \
    WINEPREFIX="${WINEPREFIX:-$HOME/.wine}" \
    WINEDEBUG="${WINEDEBUG:--all}" \
    DirectXShaderCompiler="$DirectXShaderCompiler" \
    CompileShadersOutput="$CompileShadersOutput" \
    wine cmd /c CompileShaders.cmd "$@"
