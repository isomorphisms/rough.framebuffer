#!/bin/sh
# Maintained current-interface naming guard, invoked after ICK identity is checked.
# Legacy historical issues, signed receipts and commits are not rewritten.
set -eu
root=$(CDPATH= cd "$(dirname "$0")/.." && pwd)
: "${ICK_CC:?source-pinned ICK required}"
legacy=sub
legacy="${legacy}lixel"

if git -C "$root" ls-files | grep -i "$legacy"; then
    echo 'BLOCKED: obsolete tracked path in current product' >&2
    exit 1
fi
if git -C "$root" grep -I -n -i "$legacy" -- .; then
    echo 'BLOCKED: obsolete spelling in current tracked source or docs' >&2
    exit 1
fi

temp=$(mktemp -d)
trap 'rm -rf "$temp"' EXIT HUP INT TERM
printf '#include "%s/curve_patch.h"\nint main(void) { return 0; }\n' "$legacy" > "$temp/old-include.c"
if "$ICK_CC" -O0 -std=c11 -Wall -Wextra -Werror -pedantic \
    -I"$root/subpixel/include" -c "$temp/old-include.c" \
    -o "$temp/old-include.o" > "$temp/old-include.log" 2>&1; then
    echo 'BLOCKED: obsolete include unexpectedly compiled' >&2
    exit 1
fi
grep -Fq "$legacy/curve_patch.h" "$temp/old-include.log"
printf '#include "subpixel_framebuffer.h"\nint main(void) { return rough_blend_%s_coverage(0, 0, 0, 0); }\n' "$legacy" > "$temp/old-symbol.c"
if "$ICK_CC" -O0 -std=c11 -Wall -Wextra -Werror -pedantic \
    -I"$root/subpixel/include" -I"$root/src" \
    -c "$temp/old-symbol.c" -o "$temp/old-symbol.o" \
    > "$temp/old-symbol.log" 2>&1; then
    echo 'BLOCKED: obsolete framebuffer symbol unexpectedly compiled' >&2
    exit 1
fi
grep -Fq "rough_blend_${legacy}_coverage" "$temp/old-symbol.log"
echo 'Subpixel naming: no stale tracked paths/text; stale include and symbol rejected by ICK'
