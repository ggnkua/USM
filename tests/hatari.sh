#!/bin/sh
# Launch a USM-built cart in Hatari.
#
# Usage: tests/hatari.sh <rom-name> [extra hatari args...]
#
# <rom-name> can be:
#   - a bare name (e.g. SWITV310)            -> build/SWITV310.ROM
#   - a file in build/ (e.g. SWITV310.ROM)   -> build/SWITV310.ROM
#   - a path containing '/'                  -> used as-is
#
# Any trailing arguments are forwarded to Hatari (e.g. --tos /path/to/tos.img,
# --debug-output cart.log, --bios-intercept true).

set -e

cd "$(dirname "$0")/.."

if [ "$#" -lt 1 ]; then
    echo "Usage: $0 <rom-name> [extra hatari args...]" >&2
    echo "Available carts:" >&2
    ls -1 build/*.ROM 2>/dev/null | sed 's|^|  |' >&2 || echo "  (none — run tests/build-roms.sh first)" >&2
    exit 1
fi

rom_arg=$1
shift

case "$rom_arg" in
    */*)            rom=$rom_arg ;;
    *.ROM|*.rom)    rom="build/$rom_arg" ;;
    *)              rom="build/${rom_arg}.ROM" ;;
esac

if [ ! -f "$rom" ]; then
    echo "Cart not found: $rom" >&2
    echo "(Did you run tests/build-roms.sh first?)" >&2
    exit 1
fi

if command -v hatari >/dev/null 2>&1; then
    hatari_bin=$(command -v hatari)
elif [ -x /Applications/hatari.app/Contents/MacOS/hatari ]; then
    hatari_bin=/Applications/hatari.app/Contents/MacOS/hatari
elif [ -x /Applications/Hatari.app/Contents/MacOS/hatari ]; then
    hatari_bin=/Applications/Hatari.app/Contents/MacOS/hatari
else
    echo "Hatari not found on PATH or in /Applications" >&2
    exit 1
fi

echo "==> Booting $rom in $hatari_bin"
exec "$hatari_bin" \
    --cartridge "$rom" \
#    --fast-boot true \
#    --natfeats true \
    "$@"
