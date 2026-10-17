#!/bin/sh
# Real 2D coverage preview. Called by the source-pinned Ike preview target.
# The Ike graph's test dependency prepares and verifies all native ICK objects.
set -eu
root=$(CDPATH= cd "$(dirname "$0")/../.." && pwd)
: "${ICK_CC:?source-built ICK is required}"
: "${ICK_SOURCE_DIR:?pinned ICK source checkout is required}"
: "${HOST_LINKER:?link-only driver is required}"
test "$ICK_CC" = "$ICK_SOURCE_DIR/stage/bin/x86_64-linux-gnu-gcc" || {
    echo 'Subpixel preview requires exact pinned ICK installation' >&2
    exit 2
}
test -x "$ICK_CC" && test -x "$HOST_LINKER"

out="$root/subpixel/build/ick"
test -s "$out/curve_patch.o"
test -s "$out/curve_patch_5x5.o"
mkdir -p "$out"
flags="-O2 -std=c11 -Wall -Wextra -Werror -pedantic -fPIC -ffp-contract=off -fno-fast-math"

# Never pass project .c source to the object-only system linker.
"$ICK_CC" $flags -I"$root/subpixel/include" -I"$root/src" \
    -c "$root/subpixel/examples/preview.c" -o "$out/subpixel-preview.o"
readelf -h "$out/subpixel-preview.o" | grep -q 'Class:.*ELF64'
readelf -h "$out/subpixel-preview.o" | grep -q 'Machine:.*Advanced Micro Devices X86-64'
"$HOST_LINKER" "$out/curve_patch.o" "$out/curve_patch_5x5.o" \
    "$out/coverage_plane.o" "$out/subpixel_framebuffer.o" \
    "$out/subpixel-preview.o" -lm -o "$out/subpixel-preview"
"$out/subpixel-preview" --self-test
"$out/subpixel-preview" "$root/subpixel/build/curve-3x3-vs-5x5.ppm"
test -s "$root/subpixel/build/curve-3x3-vs-5x5.ppm"

printf 'SUBPIXEL_PREVIEW_SOURCE_COMPILER=ICK\n'
sha256sum "$ICK_CC" "$out/subpixel-preview.o" "$out/subpixel-preview" \
    "$root/subpixel/build/curve-3x3-vs-5x5.ppm"
echo 'Subpixel ICK-built 2D curvature preview: PASS (no Android/triangle claim)'
