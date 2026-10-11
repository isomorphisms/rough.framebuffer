#!/bin/sh
# Negative controls: stock compilers and cached reference builds cannot stand in for ICK.
set -eu
root=$(CDPATH= cd "$(dirname "$0")/.." && pwd)
log=$(mktemp)
trap 'rm -f "$log"' EXIT HUP INT TERM

for cc in gcc clang; do
    if env -u ICK_CC -u ICK_SOURCE_DIR -u HOST_LINKER \
        make -C "$root" all CC="$cc" >"$log" 2>&1; then
        echo "FAIL: product target silently used $cc without ICK" >&2
        exit 1
    fi
    grep -q 'ICK_CC' "$log" || {
        echo "FAIL: product target failed for a reason other than missing ICK_CC" >&2
        cat "$log" >&2
        exit 1
    }

    if env -u ICK_CC -u ICK_SOURCE_DIR -u HOST_LINKER \
        make -C "$root" test-sublixel CC="$cc" >"$log" 2>&1; then
        echo "FAIL: Sublixel silently used $cc without ICK" >&2
        exit 1
    fi
    grep -q 'ICK_CC' "$log" || {
        echo "FAIL: Sublixel failure did not identify ICK_CC" >&2
        cat "$log" >&2
        exit 1
    }
done

# When a real ICK checkout is present, reject a stock or success-only wrapper
# even if CC is also set. Neither substitute may satisfy a product build.
if [ -n "${ICK_SOURCE_DIR-}" ] && [ -n "${HOST_LINKER-}" ]; then
    for fake in /usr/bin/gcc /bin/true; do
        test -x "$fake" || continue
        if ICK_CC="$fake" make -C "$root" all CC=clang >"$log" 2>&1; then
            echo "FAIL: product accepted counterfeit ICK_CC=$fake" >&2
            exit 1
        fi
        if ICK_CC="$fake" make -C "$root" test-sublixel CC=gcc >"$log" 2>&1; then
            echo "FAIL: Sublixel accepted counterfeit ICK_CC=$fake" >&2
            exit 1
        fi
    done
fi

printf '%s\n' 'ICK-first policy negative controls: PASS (no substitute accepted)'
