#!/bin/sh
# Real 2D coverage preview. Called by the source-pinned Ike preview target.
# The Ike graph's test dependency prepares and verifies all native ICK objects.
set -eu
root=$(CDPATH= cd "$(dirname "$0")/../.." && pwd)
: "${ICK_CC:?source-built ICK is required}"
: "${ICK_SOURCE_DIR:?pinned ICK source checkout is required}"
: "${HOST_LINKER:?link-only driver is required}"
test "$ICK_CC" = "$ICK_SOURCE_DIR/stage/bin/x86_64-linux-gnu-gcc" || {
    echo 'Sublixel preview requires exact pinned ICK installation' >&2
    exit 2
}
test -x "$ICK_CC" && test -x "$HOST_LINKER"

out="$root/sublixel/build/ick"
test -s "$out/curve_patch.o"
test -s "$out/curve_patch_5x5.o"
mkdir -p "$out"
flags="-O2 -std=c11 -Wall -Wextra -Werror -pedantic -fPIC -ffp-contract=off -fno-fast-math"

# Never pass project .c source to the object-only system linker.
"$ICK_CC" $flags -I"$root/sublixel/include" \
    -c "$root/sublixel/examples/preview.c" -o "$out/sublixel-preview.o"
readelf -h "$out/sublixel-preview.o" | grep -q 'Class:.*ELF64'
readelf -h "$out/sublixel-preview.o" | grep -q 'Machine:.*Advanced Micro Devices X86-64'
"$HOST_LINKER" "$out/curve_patch.o" "$out/curve_patch_5x5.o" \
    "$out/sublixel-preview.o" -lm -o "$out/sublixel-preview"
"$out/sublixel-preview" --self-test
"$out/sublixel-preview" "$root/sublixel/build/curve-3x3-vs-5x5.ppm"
test -s "$root/sublixel/build/curve-3x3-vs-5x5.ppm"

printf 'SUBLIXEL_PREVIEW_SOURCE_COMPILER=ICK\n'
sha256sum "$ICK_CC" "$out/sublixel-preview.o" "$out/sublixel-preview" \
    "$root/sublixel/build/curve-3x3-vs-5x5.ppm"
echo 'Sublixel ICK-built 2D curvature preview: PASS (no Android/triangle claim)'
