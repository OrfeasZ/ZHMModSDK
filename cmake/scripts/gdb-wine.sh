#!/usr/bin/env bash

set -euo pipefail

here=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
gdb=$(command -v gdb || true)

[ -n "$gdb" ] || { echo "${0##*/}: no gdb on PATH -- start CLion from 'nix develop'" >&2; exit 1; }

# CLion probes the binary before it debugs anything. These runs print and exit.
for arg in "$@"; do
	case $arg in
		--version|-v|--configuration|--help|-h)
			exec "$gdb" "$@"
			;;
	esac
done

exec "$gdb" -iex "source $here/gdb-wine-setup.py" "$@"
