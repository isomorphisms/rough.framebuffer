#!/bin/sh
# Host-only reference check for the C67 diagnostic naming surface.
# The C67 NDK exception does not imply ICK source acceptance.
set -eu
root=$(CDPATH= cd "$(dirname "$0")/.." && pwd)
: "${CC:=cc}"
legacy=sub
legacy="${legacy}lixel"
if git -C "$root" ls-files | grep -i "$legacy"; then
    echo 'Old tracked diagnostic path remains' >&2
    exit 1
fi
if git -C "$root" grep -I -n -i "$legacy" -- .; then
    echo 'Old diagnostic naming remains in current tracked text' >&2
    exit 1
fi
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
printf '#include "%s/curve_patch.h"\nint main(void) { return 0; }\n' "$legacy" > "$tmp/include.c"
if "$CC" -std=c11 -Wall -Wextra -Werror -pedantic \
    -I"$root/subpixel/include" -c "$tmp/include.c" -o "$tmp/include.o" \
    > "$tmp/include.log" 2>&1; then
    echo 'Obsolete diagnostic header unexpectedly compiled' >&2
    exit 1
fi
grep -Fq "$legacy/curve_patch.h" "$tmp/include.log"
printf '#include "subpixel_demo.h"\nint main(void) { return rough_%s_demo_draw(0, 0); }\n' "$legacy" > "$tmp/symbol.c"
if "$CC" -std=c11 -Wall -Wextra -Werror -pedantic \
    -I"$root/src" -c "$tmp/symbol.c" -o "$tmp/symbol.o" \
    > "$tmp/symbol.log" 2>&1; then
    echo 'Obsolete diagnostic API unexpectedly compiled' >&2
    exit 1
fi
grep -Fq "rough_${legacy}_demo_draw" "$tmp/symbol.log"
echo 'C67 current naming and two host-only legacy compile refusals: PASS'
