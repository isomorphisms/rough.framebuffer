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
"$ICK_CC" $flags -I"$repo_root/subpixel/include" -c \
    "$repo_root/subpixel/src/font.c" -o "$out/font.o"
"$ICK_CC" $flags -I"$repo_root/subpixel/include" -c \
    "$repo_root/subpixel/tests/test_font_render.c" -o "$out/test_font_render.o"
"$ICK_CC" $flags -I"$repo_root/src" -c \
    "$repo_root/src/framebuffer.c" -o "$out/framebuffer.o"
"$ICK_CC" $flags -I"$repo_root/src" -c \
    "$repo_root/src/main.c" -o "$out/rough_main.o"
"$ICK_CC" $flags -I"$repo_root/src" -c \
    "$repo_root/tests/test_raster_ick.c" -o "$out/test_raster_ick.o"
"$ICK_CC" $flags -I"$repo_root/src" -c \
    "$repo_root/tests/test_api.c" -o "$out/test_api.o"
# Qualify the preserved historical headless art source against current ICK.
"$ICK_CC" $flags -I"$repo_root/src" -c \
    "$repo_root/src/surface.c" -o "$out/surface.o"
"$ICK_CC" $flags -I"$repo_root/src" -c \
    "$repo_root/src/surface_main.c" -o "$out/surface_main.o"
"$ICK_CC" $flags -I"$repo_root/src" -c \
    "$repo_root/tests/test_surface.c" -o "$out/test_surface.o"
"$ICK_CC" $flags -I"$repo_root/src" -c \
    "$repo_root/src/present.c" -o "$out/present.o"
"$ICK_CC" $flags -I"$repo_root/src" -c \
    "$repo_root/src/touch.c" -o "$out/touch.o"
"$ICK_CC" $flags -I"$repo_root/src" -c \
    "$repo_root/tests/test_present.c" -o "$out/test_present.o"
"$ICK_CC" $flags -I"$repo_root/src" -c \
    "$repo_root/tests/test_touch.c" -o "$out/test_touch.o"
"$ICK_CC" $flags -I"$repo_root/src" -c \
    "$repo_root/tests/test_rotation_grab.c" -o "$out/test_rotation_grab.o"
"$ICK_CC" $flags -I"$repo_root/src" -c \
    "$repo_root/src/surface_aa.c" -o "$out/surface_aa.o"
"$ICK_CC" $flags -I"$repo_root/src" -c \
    "$repo_root/tests/test_surface_aa.c" -o "$out/test_surface_aa.o"
"$ICK_CC" $flags -I"$repo_root/src" -I"$repo_root/subpixel/include" \
    -c "$repo_root/src/subpixel_demo.c" -o "$out/subpixel_demo.o"
"$ICK_CC" $flags -I"$repo_root/src" -c \
    "$repo_root/tests/test_subpixel_demo.c" -o "$out/test_subpixel_demo.o"
"$ICK_CC" $flags -I"$repo_root/src" -c \
    "$repo_root/tests/test_smooth.c" -o "$out/test_smooth.o"

for object in "$out/curve_patch.o" "$out/test_curve_patch.o" \
              "$out/curve_patch_5x5.o" "$out/test_curve_patch_5x5.o" \
              "$out/coverage_plane.o" "$out/subpixel_framebuffer.o" "$out/test_coverage_plane.o" \
              "$out/path.o" "$out/test_path.o" \
              "$out/font_5x7.o" "$out/test_font_5x7.o" "$out/font_5x7_dump.o" \
              "$out/framebuffer.o" "$out/rough_main.o" "$out/test_raster_ick.o" "$out/test_api.o" \
              "$out/surface.o" "$out/surface_main.o" "$out/test_surface.o" \
              "$out/present.o" "$out/touch.o" "$out/test_present.o" \
              "$out/test_touch.o" "$out/test_rotation_grab.o" \
              "$out/surface_aa.o" "$out/test_surface_aa.o" \
              "$out/subpixel_demo.o" "$out/test_subpixel_demo.o" \
              "$out/test_smooth.o"; do
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
"$HOST_LINKER" "$out/font.o" "$out/font_5x7.o" "$out/curve_patch.o" \
    "$out/curve_patch_5x5.o" "$out/coverage_plane.o" "$out/path.o" \
    "$out/test_font_render.o" -lm -o "$out/test_font_render"
"$HOST_LINKER" "$out/framebuffer.o" "$out/test_raster_ick.o" \
    -lm -o "$out/test_raster_ick"
"$HOST_LINKER" "$out/framebuffer.o" "$out/test_api.o" \
    -lm -o "$out/test_api"
"$HOST_LINKER" "$out/framebuffer.o" "$out/rough_main.o" \
    -lm -o "$repo_root/build/rough-fb"
"$HOST_LINKER" "$out/surface.o" "$out/surface_main.o" \
    "$out/framebuffer.o" -lm \
    -o "$repo_root/build/rough-surface"
"$HOST_LINKER" "$out/framebuffer.o" "$out/surface.o" "$out/test_surface.o" -lm \
    -o "$out/test_surface"
"$HOST_LINKER" "$out/present.o" "$out/surface.o" "$out/framebuffer.o" \
    "$out/test_present.o" -lm -o "$out/test_present"
"$HOST_LINKER" "$out/touch.o" "$out/surface.o" "$out/framebuffer.o" \
    "$out/test_touch.o" -lm -o "$out/test_touch"
"$HOST_LINKER" "$out/touch.o" "$out/surface.o" "$out/framebuffer.o" \
    "$out/test_rotation_grab.o" -lm -o "$out/test_rotation_grab"
"$HOST_LINKER" "$out/surface_aa.o" "$out/surface.o" "$out/framebuffer.o" \
    "$out/test_surface_aa.o" -lm -o "$out/test_surface_aa"
"$HOST_LINKER" "$out/subpixel_demo.o" "$out/test_subpixel_demo.o" \
    "$out/curve_patch.o" "$out/curve_patch_5x5.o" -lm \
    -o "$out/test_subpixel_demo"
"$HOST_LINKER" "$out/framebuffer.o" "$out/surface.o" \
    "$out/test_smooth.o" -lm -o "$out/test_smooth"

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
"$out/test_font_render" "$out/sensor-font.pgm" > "$out/sensor-font.tsv"
cat "$out/sensor-font.tsv"
"$out/font_5x7_dump" > "$out/font-5x7-subpixels.txt"
test -s "$out/font-5x7-subpixels.txt"
sha256sum "$out/font-5x7-subpixels.txt"
"$out/test_raster_ick"
"$out/test_api"
"$out/test_surface"
"$out/test_touch"
"$out/test_present"
"$out/test_rotation_grab"
"$out/test_surface_aa"
"$out/test_subpixel_demo"
"$out/test_smooth"
# A constant face-normal shader can shade a frame but violates
# per-fragment interpolation. Generate a single real source mutation with
# the existing recipe's AWK helper and compile/run through ICK.
awk '
/double nx = k0 \* v0.normal_x/ {
    print "            double nx = v0.normal_x;"
    print "            double ny = v0.normal_y;"
    print "            double nz = v0.normal_z;"
    skip = 1
    changed++
    next
}
skip && /double normal_length =/ { skip = 0 }
skip { next }
{ print }
END { if (changed != 1) exit 6 }
' "$repo_root/src/framebuffer.c" > "$out/smooth-flat-normal-mutant.c"
grep -Fq 'double nx = v0.normal_x;' "$out/smooth-flat-normal-mutant.c"
"$ICK_CC" $flags -I"$repo_root/src" -c "$out/smooth-flat-normal-mutant.c" \
    -o "$out/smooth-flat-normal-mutant.o"
"$HOST_LINKER" "$out/smooth-flat-normal-mutant.o" "$out/surface.o" \
    "$out/test_smooth.o" -lm -o "$out/reject-flat-normal"
ulimit -c 0
if "$out/reject-flat-normal" > "$out/reject-flat-normal.log" 2>&1; then
    echo 'Constant face-normal shader incorrectly accepted' >&2; exit 1
fi
grep -Fq 'test_normal_is_evaluated_per_fragment' "$out/reject-flat-normal.log"
echo 'ICK analytic normals, projective Phong lighting and flat-normal mutant: PASS'
# Kill old center-sample silhouette: the real AA oracle must notice the
# foreground/intermediate coverage missing when 2x2 resolve is bypassed.
sed 's/return rough_resolve_2x2(scratch, destination);/return rough_draw_surface(destination, shape, angle_degrees, camera_distance);/' \
    "$repo_root/src/surface_aa.c" > "$out/aa-single-sample-mutant.c"
grep -Fq 'return rough_draw_surface(destination, shape, angle_degrees, camera_distance);' \
    "$out/aa-single-sample-mutant.c"
"$ICK_CC" $flags -I"$repo_root/src" -c "$out/aa-single-sample-mutant.c" \
    -o "$out/aa-single-sample-mutant.o"
"$HOST_LINKER" "$out/aa-single-sample-mutant.o" "$out/surface.o" \
    "$out/framebuffer.o" "$out/test_surface_aa.o" -lm \
    -o "$out/reject-1x-silhouette"
ulimit -c 0
if "$out/reject-1x-silhouette" > "$out/reject-1x-silhouette.log" 2>&1; then
    echo 'Single-sample triangle silhouette incorrectly accepted' >&2; exit 1
fi
grep -Fq 'changed > 20u' "$out/reject-1x-silhouette.log"
echo 'ICK depth-correct 2x2 subpixel silhouette and 1x mutant: PASS'
# Independently reject reversed near-side drag semantics; changed frame hashes
# alone falsely accepted the original defect. Compile the mutant with ICK too.
sed 's/state->anchor_yaw -/state->anchor_yaw +/' \
    "$repo_root/src/touch.c" > "$out/touch-backside-mutant.c"
grep -Fq 'state->anchor_yaw +' "$out/touch-backside-mutant.c"
"$ICK_CC" $flags -I"$repo_root/src" -c "$out/touch-backside-mutant.c" \
    -o "$out/touch-backside-mutant.o"
"$HOST_LINKER" "$out/touch-backside-mutant.o" "$out/surface.o" \
    "$out/framebuffer.o" "$out/test_rotation_grab.o" -lm \
    -o "$out/reject-backside-drag"
ulimit -c 0
if "$out/reject-backside-drag" > "$out/reject-backside-drag.log" 2>&1; then
    echo 'Inverted near-side gesture accepted' >&2; exit 1
fi
grep -Fq 'FRONT_HEMISPHERE_WRONG_DIRECTION:' "$out/reject-backside-drag.log"
echo 'ICK native touch, presentation, direct surface and backwards-drag mutant: PASS'
# Reproducible genuine source-stage frames, not a recorded movie or mock image.
"$repo_root/build/rough-surface" torus 40 160 160 > "$out/torus-40.ops"
"$repo_root/build/rough-surface" torus 40 160 160 > "$out/torus-40-repeat.ops"
"$repo_root/build/rough-surface" enneper 25 160 160 > "$out/enneper-25.ops"
cmp "$out/torus-40.ops" "$out/torus-40-repeat.ops"
test -s "$out/torus-40.ops" && test -s "$out/enneper-25.ops"
"$repo_root/build/rough-fb" "$out/torus-40.ops" "$out/torus-40.ppm" 160 160
"$repo_root/build/rough-fb" "$out/enneper-25.ops" "$out/enneper-25.ppm" 160 160
test -s "$out/torus-40.ppm" && test -s "$out/enneper-25.ppm"
sha256sum "$out/surface.o" "$out/surface_main.o" "$out/test_surface.o" \
    "$out/test_surface" "$repo_root/build/rough-surface" \
    "$out/torus-40.ppm" "$out/enneper-25.ppm"
# The original surface seam test must refuse a genuinely nonperiodic palette,
# using the same ICK frontend and its original independent test program.
sed -e 's/sin(u)/sin(1.3 * u)/' -e 's/cos(2.0 [*] v)/cos(1.8 * v)/' \
    "$repo_root/src/surface.c" > "$out/surface-palette-mutant.c"
grep -Fq 'sin(1.3 * u)' "$out/surface-palette-mutant.c"
grep -Fq 'cos(1.8 * v)' "$out/surface-palette-mutant.c"
"$ICK_CC" $flags -I"$repo_root/src" -c "$out/surface-palette-mutant.c" \
    -o "$out/surface-palette-mutant.o"
"$HOST_LINKER" "$out/framebuffer.o" "$out/surface-palette-mutant.o" \
    "$out/test_surface.o" -lm -o "$out/reject-surface-palette"
ulimit -c 0
if "$out/reject-surface-palette" > "$out/reject-surface-palette.log" 2>&1; then
    echo 'Defective torus color seam accepted' >&2; exit 1
fi
grep -Fq 'torus seam palette jump' "$out/reject-surface-palette.log"
echo 'ICK headless mathematical surface source/renderer and palette mutant: PASS'

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
