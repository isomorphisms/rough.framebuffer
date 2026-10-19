#!/bin/sh
# Real 2D coverage preview. Called by the source-pinned Ike preview target.
# The Ike graph's test dependency prepares and verifies all native ICK objects.
set -eu
root=$(CDPATH= cd "$(dirname "$0")/../.." && pwd)
: "${ICK_CC:?source-built ICK is required}"
: "${ICK_SOURCE_DIR:?pinned ICK source checkout is required}"
: "${HOST_LINKER:?link-only driver is required}"
required_driver="$ICK_SOURCE_DIR/stage/bin/x86_64-linux-gnu-gcc"
if [ -n "${ICK_ARTIFACT_STAGE-}" ]; then
    required_driver="$ICK_ARTIFACT_STAGE/bin/gcc"
    test "$(sha256sum "$required_driver" | cut -d ' ' -f1)" = 45805e3295f2709737fcc87985a8a2341aa8663ff8e4e398d2a9d78dc1283e10
fi
test "$ICK_CC" = "$required_driver" || {
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
    "$out/path.o" "$out/framebuffer.o" "$out/subpixel-preview.o" -lm -o "$out/subpixel-preview"
"$ICK_CC" $flags -I"$root/subpixel/include" -I"$root/src" \
    -c "$root/subpixel/examples/path_render.c" -o "$out/path_render.o"
"$HOST_LINKER" "$out/curve_patch.o" "$out/curve_patch_5x5.o" \
    "$out/coverage_plane.o" "$out/subpixel_framebuffer.o" "$out/path.o" \
    "$out/framebuffer.o" "$out/path_render.o" -lm -o "$out/path-render"
"$ICK_CC" $flags -I"$root/subpixel/include" -I"$root/subpixel/examples" -I"$root/src" \
    -c "$root/subpixel/examples/font_5x7_preview.c" -o "$out/font_5x7_preview.o"
"$HOST_LINKER" "$out/curve_patch.o" "$out/curve_patch_5x5.o" \
    "$out/coverage_plane.o" "$out/subpixel_framebuffer.o" "$out/path.o" \
    "$out/framebuffer.o" "$out/font_5x7.o" "$out/font_5x7_preview.o" \
    -lm -o "$out/font-5x7-preview"
"$out/font-5x7-preview" "$root/subpixel/build"
for strategy in baseline quadratic cubic auto; do
    "$out/path-render" "$strategy" "$root/subpixel/examples/whole-path.txt" \
        "$root/subpixel/build/path-$strategy.ppm" "$root/subpixel/build/path-$strategy.tsv"
done
"$out/subpixel-preview" --self-test
"$out/subpixel-preview" "$root/subpixel/build/curve-3x3-vs-5x5.ppm"
test -s "$root/subpixel/build/curve-3x3-vs-5x5.ppm"

printf 'SUBPIXEL_PREVIEW_SOURCE_COMPILER=ICK\n'
sha256sum "$ICK_CC" "$out/subpixel-preview.o" "$out/subpixel-preview" \
    "$root/subpixel/build/curve-3x3-vs-5x5.ppm"
echo 'Subpixel ICK-built 2D curvature preview: PASS (no Android/triangle claim)'
