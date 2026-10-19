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
# The source pin alone does not authenticate an arbitrary binary that a
# caller names ICK_CC. This qualification installs from the pinned checkout
# into its stage/bin; refuse stock gcc, wrappers, or unrelated installations.
required_driver="$ICK_SOURCE_DIR/stage/bin/x86_64-linux-gnu-gcc"
target=x86_64-linux-gnu
if [ -n "${ICK_ARTIFACT_STAGE-}" ]; then
    # Alternate installation of the SAME c61e448 ICK source, exported by
    # billiard run 39225847109 (artifact 12174071008). Authenticate the archive
    # and both executable stages; never accept an arbitrary installed compiler.
    : "${ICK_STAGE_ARCHIVE:?qualified ICK artifact ZIP required}"
    test "$(sha256sum "$ICK_STAGE_ARCHIVE" | cut -d ' ' -f1)" = 8ee043088fffadba3704fd567c206c4852b0de69f0b8bb76a131bfb8fa4004a4
    required_driver="$ICK_ARTIFACT_STAGE/bin/gcc"
    target=x86_64-pc-linux-gnu
    test "$(sha256sum "$required_driver" | cut -d ' ' -f1)" = 45805e3295f2709737fcc87985a8a2341aa8663ff8e4e398d2a9d78dc1283e10
fi
if [ "$ICK_CC" != "$required_driver" ]; then
    echo "BLOCKED: compiler is not the pinned ICK installation: $ICK_CC" >&2
    exit 3
fi
test "$("$ICK_CC" -dumpmachine)" = "$target" || {
    echo "BLOCKED: ICK target is not x86_64-linux-gnu" >&2; exit 3;
}
test -z "${GCC_EXEC_PREFIX-}${COMPILER_PATH-}" || { echo 'BLOCKED: compiler stage override' >&2; exit 3; }
cc1=$(readlink -f "$("$ICK_CC" -print-prog-name=cc1)")
if [ -n "${ICK_ARTIFACT_STAGE-}" ]; then
    test "$cc1" = "$ICK_ARTIFACT_STAGE/libexec/gcc/x86_64-pc-linux-gnu/17.0.0/cc1"
    test "$(sha256sum "$cc1" | cut -d ' ' -f1)" = c09665543aa807967d22bcaf0959aedd970f9d21be309d42170e4bae5a555292
else
case "$cc1" in
  "$ICK_SOURCE_DIR"/stage/libexec/gcc/x86_64-linux-gnu/*/cc1)
      test -x "$cc1" ;;
  *) echo "BLOCKED: cc1 is outside pinned ICK stage: $cc1" >&2; exit 3 ;;
esac
fi

sh "$repo_root/qualification/test-subpixel-name.sh"

out="$repo_root/subpixel/build/ick"
mkdir -p "$out" "$repo_root/build"
flags="-O2 -std=c11 -Wall -Wextra -Werror -pedantic -ffp-contract=off -fno-fast-math -fPIC"
"$ICK_CC" $flags -I"$repo_root/subpixel/include" -c \
    "$repo_root/subpixel/src/curve_patch.c" -o "$out/curve_patch.o"
"$ICK_CC" $flags -I"$repo_root/subpixel/include" -c \
    "$repo_root/subpixel/tests/test_curve_patch.c" -o "$out/test_curve_patch.o"
"$ICK_CC" $flags -I"$repo_root/subpixel/include" -c \
    "$repo_root/subpixel/src/curve_patch_5x5.c" -o "$out/curve_patch_5x5.o"
"$ICK_CC" $flags -I"$repo_root/subpixel/include" -c \
    "$repo_root/subpixel/tests/test_curve_patch_5x5.c" -o "$out/test_curve_patch_5x5.o"
"$ICK_CC" $flags -I"$repo_root/subpixel/include" -c \
    "$repo_root/subpixel/src/coverage_plane.c" -o "$out/coverage_plane.o"
"$ICK_CC" $flags -fstack-usage -I"$repo_root/subpixel/include" -c \
    "$repo_root/subpixel/src/path.c" -o "$out/path.o"
"$ICK_CC" $flags -I"$repo_root/subpixel/include" -c \
    "$repo_root/subpixel/tests/test_path.c" -o "$out/test_path.o"
"$ICK_CC" $flags -I"$repo_root/subpixel/include" -I"$repo_root/src" -c \
    "$repo_root/src/subpixel_framebuffer.c" -o "$out/subpixel_framebuffer.o"
"$ICK_CC" $flags -I"$repo_root/subpixel/include" -I"$repo_root/src" -c \
    "$repo_root/subpixel/tests/test_coverage_plane.c" -o "$out/test_coverage_plane.o"
"$ICK_CC" $flags -I"$repo_root/subpixel/examples" -c \
    "$repo_root/subpixel/examples/font_5x7.c" -o "$out/font_5x7.o"
"$ICK_CC" $flags -I"$repo_root/subpixel/examples" -c \
    "$repo_root/subpixel/tests/test_font_5x7.c" -o "$out/test_font_5x7.o"
"$ICK_CC" $flags -I"$repo_root/subpixel/examples" -c \
    "$repo_root/subpixel/examples/font_5x7_dump.c" -o "$out/font_5x7_dump.o"
"$ICK_CC" $flags -I"$repo_root/src" -c \
    "$repo_root/src/framebuffer.c" -o "$out/framebuffer.o"
"$ICK_CC" $flags -I"$repo_root/src" -c \
    "$repo_root/src/main.c" -o "$out/rough_main.o"
"$ICK_CC" $flags -I"$repo_root/src" -c \
    "$repo_root/tests/test_raster_ick.c" -o "$out/test_raster_ick.o"
"$ICK_CC" $flags -I"$repo_root/src" -c \
    "$repo_root/tests/test_api.c" -o "$out/test_api.o"

for object in "$out/curve_patch.o" "$out/test_curve_patch.o" \
              "$out/curve_patch_5x5.o" "$out/test_curve_patch_5x5.o" \
              "$out/coverage_plane.o" "$out/subpixel_framebuffer.o" "$out/test_coverage_plane.o" \
              "$out/path.o" "$out/test_path.o" \
              "$out/font_5x7.o" "$out/test_font_5x7.o" "$out/font_5x7_dump.o" \
              "$out/framebuffer.o" "$out/rough_main.o" "$out/test_raster_ick.o" "$out/test_api.o"; do
    test -s "$object"
    readelf -h "$object" | grep -q 'Class:.*ELF64'
    readelf -h "$object" | grep -q 'Machine:.*Advanced Micro Devices X86-64'
done

# crt/libc/libm through an explicit system link driver; no project C compilation.
"$HOST_LINKER" "$out/curve_patch.o" "$out/curve_patch_5x5.o" \
    "$out/coverage_plane.o" "$out/path.o" "$out/test_path.o" -lm -o "$out/test_path"
"$HOST_LINKER" "$out/curve_patch.o" "$out/test_curve_patch.o" \
    -lm -o "$out/test_curve_patch"
"$HOST_LINKER" "$out/curve_patch.o" "$out/curve_patch_5x5.o" \
    "$out/test_curve_patch_5x5.o" -lm -o "$out/test_curve_patch_5x5"
"$HOST_LINKER" "$out/curve_patch.o" "$out/curve_patch_5x5.o" \
    "$out/coverage_plane.o" "$out/subpixel_framebuffer.o" \
    "$out/test_coverage_plane.o" -lm -o "$out/test_coverage_plane"
"$HOST_LINKER" "$out/font_5x7.o" "$out/test_font_5x7.o" \
    -lm -o "$out/test_font_5x7"
"$HOST_LINKER" "$out/font_5x7.o" "$out/font_5x7_dump.o" \
    -lm -o "$out/font_5x7_dump"
"$HOST_LINKER" "$out/framebuffer.o" "$out/test_raster_ick.o" \
    -lm -o "$out/test_raster_ick"
"$HOST_LINKER" "$out/framebuffer.o" "$out/test_api.o" \
    -lm -o "$out/test_api"
"$HOST_LINKER" "$out/framebuffer.o" "$out/rough_main.o" \
    -lm -o "$repo_root/build/rough-fb"

echo "ICK source: $ick_pin"
echo "ICK compiler: $ICK_CC"
echo "ICK cc1: $cc1"
echo "Declared system CRT/link driver, never source compiler: $HOST_LINKER"
sha256sum "$ICK_CC" "$cc1" "$out/curve_patch.o" \
    "$out/test_curve_patch.o" "$out/framebuffer.o" "$out/rough_main.o" \
    "$out/test_raster_ick.o" "$out/curve_patch_5x5.o" \
    "$out/test_curve_patch_5x5.o" "$out/test_curve_patch" \
    "$out/test_curve_patch_5x5" "$out/test_raster_ick" \
    "$repo_root/build/rough-fb"
sha256sum "$out/coverage_plane.o" "$out/subpixel_framebuffer.o" \
    "$out/test_coverage_plane.o" "$out/test_coverage_plane" \
    "$out/font_5x7.o" "$out/test_font_5x7.o" "$out/test_font_5x7" \
    "$out/font_5x7_dump.o" "$out/font_5x7_dump"
"$out/test_curve_patch"
"$out/test_path"
sha256sum "$out/path.o" "$out/test_path.o" "$out/test_path" "$out/path.su"
"$out/test_curve_patch_5x5"
"$out/test_coverage_plane"
"$out/test_font_5x7"
"$out/font_5x7_dump" > "$out/font-5x7-subpixels.txt"
test -s "$out/font-5x7-subpixels.txt"
sha256sum "$out/font-5x7-subpixels.txt"
"$out/test_raster_ick"
"$out/test_api"

# Preserve both real source regressions from the integrated continuous-stroke
# parent within the existing Ike-v1 POSIX recipe boundary. All mutated C is
# still compiled by the authenticated ICK; the system driver links objects.
sed 's/if (dash_on == 0.0 \&\& dash_off == 0.0 \&\&/if (/' \
    "$repo_root/src/framebuffer.c" > "$out/framebuffer-cull-mutant.c"
sed 's/double start_phase = \*phase;/double start_phase = 0.0;/' \
    "$repo_root/src/framebuffer.c" > "$out/framebuffer-phase-mutant.c"
ulimit -c 0
for defect in cull phase; do
    if cmp -s "$repo_root/src/framebuffer.c" "$out/framebuffer-$defect-mutant.c"; then
        echo "Missing source mutation: $defect" >&2; exit 1
    fi
    "$ICK_CC" $flags -I"$repo_root/src" -c "$out/framebuffer-$defect-mutant.c" -o "$out/framebuffer-$defect-mutant.o"
    "$HOST_LINKER" "$out/framebuffer-$defect-mutant.o" "$out/test_api.o" -lm -o "$out/reject-$defect"
    if "$out/reject-$defect" > "$out/reject-$defect.log" 2>&1; then
        echo "Accepted defective dash-phase source: $defect" >&2; exit 1
    fi
    grep -Fq 'patterned_stream' "$out/reject-$defect.log"
done
sha256sum "$out/test_api.o" "$out/test_api" "$out/framebuffer-cull-mutant.c" "$out/framebuffer-phase-mutant.c"
echo "Native triangle API and both executed dash-phase source mutants: PASS"
echo "ICK-compiled Subpixel 3x3 + 5x5 and framebuffer native C11 tests: PASS"
