#!/bin/sh
# Compile the actual framebuffer program with pinned, source-built ICK.
# HOST_LINKER receives object files only; it never compiles product C.
set -eu
root=$(CDPATH= cd "$(dirname "$0")/.." && pwd)

: "${ICK_CC:?BLOCKED: ICK_CC must name the pinned source-built ICK compiler}"
: "${ICK_SOURCE_DIR:?BLOCKED: ICK_SOURCE_DIR must name the pinned ICK checkout}"
: "${HOST_LINKER:?BLOCKED: HOST_LINKER must name the object-only system linker driver}"

for value in "$ICK_CC" "$ICK_SOURCE_DIR" "$HOST_LINKER"; do
    case "$value" in
        /*) ;;
        *) echo "BLOCKED: compiler source, driver and linker paths must be absolute" >&2; exit 2 ;;
    esac
done
test -x "$ICK_CC" || { echo 'BLOCKED: ICK compiler missing' >&2; exit 2; }
test -x "$HOST_LINKER" || { echo 'BLOCKED: object linker missing' >&2; exit 2; }
test "$(git -C "$ICK_SOURCE_DIR" rev-parse HEAD)" = "c61e448251744a2f40ad743ebef1a027bdcd2f9d" || {
    echo 'BLOCKED: wrong ICK source revision' >&2; exit 3;
}
test "$(git -C "$ICK_SOURCE_DIR/gcc" rev-parse HEAD)" = "6294f1d9e7536e5ffcde09d1528c918d63abfef5" || {
    echo 'BLOCKED: wrong GCC reference submodule' >&2; exit 3;
}
grep -Fxq 'gcc_commit=6294f1d9e7536e5ffcde09d1528c918d63abfef5' "$ICK_SOURCE_DIR/ick/SOURCE.lock" || {
    echo 'BLOCKED: mismatched ICK source lock' >&2; exit 3;
}
test "$("$ICK_CC" -dumpmachine)" = "x86_64-linux-gnu" || {
    echo 'BLOCKED: ICK driver target is not x86_64-linux-gnu' >&2; exit 3;
}
stage_dir=$(dirname "$(dirname "$ICK_CC")")
cc1=$("$ICK_CC" -print-prog-name=cc1)
case "$cc1" in
    "$stage_dir"/*) test -x "$cc1" || exit 3 ;;
    *) echo "BLOCKED: cc1 is not within the pinned ICK installation: $cc1" >&2; exit 3 ;;
esac

mkdir -p "$root/build/ick"
flags="-O2 -std=c11 -Wall -Wextra -Werror -pedantic -ffp-contract=off -fno-fast-math"
for name in main framebuffer; do
    "$ICK_CC" $flags -I"$root/src" -c "$root/src/$name.c" -o "$root/build/ick/$name.o"
    readelf -h "$root/build/ick/$name.o" | grep -q 'Class:.*ELF64'
    readelf -h "$root/build/ick/$name.o" | grep -q 'Machine:.*Advanced Micro Devices X86-64'
done
"$HOST_LINKER" "$root/build/ick/main.o" "$root/build/ick/framebuffer.o" -lm -o "$root/build/rough-fb"
printf 'PRODUCT_COMPILER=ICK\nICK_SOURCE=%s\nICK_DRIVER=%s\nICK_CC1=%s\nOBJECT_ONLY_LINKER=%s\n' \
    'c61e448251744a2f40ad743ebef1a027bdcd2f9d' "$ICK_CC" "$cc1" "$HOST_LINKER"
sha256sum "$ICK_CC" "$cc1" "$root/build/ick/main.o" "$root/build/ick/framebuffer.o" "$root/build/rough-fb"
printf '%s\n' 'Framebuffer ICK x86_64 objects and executable: BUILT (runtime tests are separate)'
