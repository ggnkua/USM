#!/bin/sh
# Build a .ROM cart from each binary in tests/binaries/.
# Output goes to build/<name>.ROM (one cart per input, default mode).
# Run from the repo root.

set -e

cd "$(dirname "$0")/.."

if [ ! -x ./usm ]; then
    echo "==> Building usm"
    ./build_mac_linux.sh
fi

mkdir -p build

status=0
for src in tests/binaries/*.PRG tests/binaries/*.TOS; do
    [ -e "$src" ] || continue
    name=$(basename "$src")
    rom="build/${name%.*}.ROM"
    printf '==> %-32s -> %s\n' "$src" "$rom"
    # No -f flag: CA_INIT high byte = 0, so TOS does NOT auto-run the
    # cart at boot. The program shows up on the cart's c: drive and the
    # user launches it from the desktop (TOS Pexec's us into user mode;
    # the default-mode stub handles either context).
    if ! ./usm "$rom" "$src"; then
        echo "    FAILED: $src" >&2
        status=1
    fi
done

exit "$status"
