#!/bin/sh
# Consumer C and tests compile exclusively through the pinned owned ICK.
# System link driver accepts ICK-generated objects ONLY; never .c files.
set -eu
repo_root=$(CDPATH= cd "$(dirname "$0")/../.." && pwd)
: "${ICK_CC:?ICK_CC must name the pinned ICK driver}"
: "${ICK_SOURCE_DIR:?ICK_SOURCE_DIR must name the pinned ICK checkout}"
: "${HOST_LINKER:?HOST_LINKER must name the link-only system driver}"
case "$ICK_CC" in /*) ;; *) echo "ICK_CC must be absolute" >&2; exit 2 ;; esac
case "$ICK_SOURCE_DIR" in /*) ;; *) echo "ICK_SOURCE_DIR must be absolute" >&2; exit 2 ;; esac
case "$HOST_LINKER" in /*) ;; *) echo "HOST_LINKER must be absolute" >&2; exit 2 ;; esac
test -x "$ICK_CC"
test -x "$HOST_LINKER"
ick_pin=c61e448251744a2f40ad743ebef1a027bdcd2f9d
reference_pin=6294f1d9e7536e5ffcde09d1528c918d63abfef5
test "$(git -C "$ICK_SOURCE_DIR" rev-parse HEAD)" = "$ick_pin"
test "$(git -C "$ICK_SOURCE_DIR/gcc" rev-parse HEAD)" = "$reference_pin"
test "$(sed -n 's/^gcc_commit=//p' "$ICK_SOURCE_DIR/ick/SOURCE.lock")" = "$reference_pin"
test "$("$ICK_CC" -dumpmachine)" = x86_64-linux-gnu
stage_dir=$(dirname "$(dirname "$ICK_CC")")
cc1=$("$ICK_CC" -print-prog-name=cc1)
case "$cc1" in
  "$stage_dir"/*) test -x "$cc1" ;;
  *) echo "C frontend escaped the pinned ICK installation: $cc1" >&2; exit 3 ;;
esac

out="$repo_root/sublixel/build/ick"
mkdir -p "$out" "$repo_root/build"
flags="-O2 -std=c11 -Wall -Wextra -Werror -pedantic -ffp-contract=off -fno-fast-math -fPIC"
"$ICK_CC" $flags -I"$repo_root/sublixel/include" -c \
    "$repo_root/sublixel/src/curve_patch.c" -o "$out/curve_patch.o"
"$ICK_CC" $flags -I"$repo_root/sublixel/include" -c \
    "$repo_root/sublixel/tests/test_curve_patch.c" -o "$out/test_curve_patch.o"
"$ICK_CC" $flags -I"$repo_root/src" -c \
    "$repo_root/src/framebuffer.c" -o "$out/framebuffer.o"
"$ICK_CC" $flags -I"$repo_root/src" -c \
    "$repo_root/src/main.c" -o "$out/rough_main.o"
"$ICK_CC" $flags -I"$repo_root/src" -c \
    "$repo_root/tests/test_raster_ick.c" -o "$out/test_raster_ick.o"

for object in "$out/curve_patch.o" "$out/test_curve_patch.o" \
              "$out/framebuffer.o" "$out/rough_main.o" "$out/test_raster_ick.o"; do
    test -s "$object"
    readelf -h "$object" | grep -q 'Class:.*ELF64'
    readelf -h "$object" | grep -q 'Machine:.*Advanced Micro Devices X86-64'
done

# crt/libc/libm through an explicit system link driver; no project C compilation.
"$HOST_LINKER" "$out/curve_patch.o" "$out/test_curve_patch.o" \
    -lm -o "$out/test_curve_patch"
"$HOST_LINKER" "$out/framebuffer.o" "$out/test_raster_ick.o" \
    -lm -o "$out/test_raster_ick"
"$HOST_LINKER" "$out/framebuffer.o" "$out/rough_main.o" \
    -lm -o "$repo_root/build/rough-fb"

echo "ICK source: $ick_pin"
echo "ICK compiler: $ICK_CC"
echo "ICK cc1: $cc1"
echo "Declared system CRT/link driver, never source compiler: $HOST_LINKER"
sha256sum "$ICK_CC" "$cc1" "$out/curve_patch.o" \
    "$out/test_curve_patch.o" "$out/framebuffer.o" "$out/rough_main.o" \
    "$out/test_raster_ick.o" "$out/test_curve_patch" "$out/test_raster_ick" \
    "$repo_root/build/rough-fb"
"$out/test_curve_patch"
"$out/test_raster_ick"
echo "ICK-compiled Sublixel and framebuffer native C11 tests: PASS"
