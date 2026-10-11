#!/bin/sh
# Compile Sublixel's whole test workload as C11 with the pinned source-built ICK.
# The named system linker consumes only ICK-produced objects. It does not
# compile any Sublixel translation unit.
set -eu
repo_root=$(CDPATH= cd "$(dirname "$0")/../.." && pwd)
: "${ICK_CC:?set ICK_CC to the absolute pinned ICK compiler driver}"
: "${ICK_SOURCE_DIR:?set ICK_SOURCE_DIR to the pinned ICK repository checkout}"
: "${HOST_LINKER:?set HOST_LINKER explicitly; it links objects only}"

case "$ICK_CC" in
  /*) ;;
  *) echo "ICK_CC must be an absolute path" >&2; exit 2 ;;
esac
case "$ICK_SOURCE_DIR" in
  /*) ;;
  *) echo "ICK_SOURCE_DIR must be an absolute path" >&2; exit 2 ;;
esac
case "$HOST_LINKER" in
  /*) ;;
  *) echo "HOST_LINKER must be an absolute path" >&2; exit 2 ;;
esac
test -x "$ICK_CC"
test -x "$HOST_LINKER"
test "$(git -C "$ICK_SOURCE_DIR" rev-parse HEAD)" = "c61e448251744a2f40ad743ebef1a027bdcd2f9d"
test "$(git -C "$ICK_SOURCE_DIR/gcc" rev-parse HEAD)" = "6294f1d9e7536e5ffcde09d1528c918d63abfef5"
test -s "$ICK_SOURCE_DIR/ick/SOURCE.lock"
test "$(sed -n 's/^gcc_commit=//p' "$ICK_SOURCE_DIR/ick/SOURCE.lock")" = "6294f1d9e7536e5ffcde09d1528c918d63abfef5"
test "$("$ICK_CC" -dumpmachine)" = "x86_64-linux-gnu"
stage_dir=$(dirname "$(dirname "$ICK_CC")")
cc1=$("$ICK_CC" -print-prog-name=cc1)
case "$cc1" in
  "$stage_dir"/*) test -x "$cc1" ;;
  *) echo "ICK cc1 escaped its pinned compiler installation: $cc1" >&2; exit 3 ;;
esac

output_dir="$repo_root/sublixel/build/ick"
mkdir -p "$output_dir"
flags="-O2 -std=c11 -Wall -Wextra -Werror -pedantic -ffp-contract=off -fno-fast-math"
# Deliberate ordinary C dialect: ICKY's replacement parser is not yet adopted.
# Separate -c invocations must not silently invoke host GCC or Clang as a C compiler.
"$ICK_CC" $flags -I"$repo_root/sublixel/include" -c   "$repo_root/sublixel/src/curve_patch.c" -o "$output_dir/curve_patch.o"
"$ICK_CC" $flags -I"$repo_root/sublixel/include" -c   "$repo_root/sublixel/tests/test_curve_patch.c" -o "$output_dir/test_curve_patch.o"
readelf -h "$output_dir/curve_patch.o" | grep -q 'Class:.*ELF64'
readelf -h "$output_dir/curve_patch.o" | grep -q 'Machine:.*Advanced Micro Devices X86-64'
readelf -h "$output_dir/test_curve_patch.o" | grep -q 'Class:.*ELF64'
readelf -h "$output_dir/test_curve_patch.o" | grep -q 'Machine:.*Advanced Micro Devices X86-64'

"$HOST_LINKER" "$output_dir/curve_patch.o" "$output_dir/test_curve_patch.o"   -lm -o "$output_dir/test_curve_patch"
printf 'ICK source: %s\n' "c61e448251744a2f40ad743ebef1a027bdcd2f9d"
printf 'ICK driver: %s\n' "$ICK_CC"
printf 'ICK cc1: %s\n' "$cc1"
printf 'Host object linker (does not compile C): %s\n' "$HOST_LINKER"
sha256sum "$ICK_CC" "$cc1" "$output_dir/curve_patch.o"   "$output_dir/test_curve_patch.o" "$output_dir/test_curve_patch"
"$output_dir/test_curve_patch"
printf '%s\n' 'Sublixel ICK x86_64 C11 compile/object/link/run: PASS'
