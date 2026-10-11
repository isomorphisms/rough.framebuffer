#!/bin/sh
# Negative controls for the maintained Ike -> ICK native build.
# This script is itself a checked Ike recipe, never a GNU Make product target.
set -eu
root=$(CDPATH= cd "$(dirname "$0")/.." && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
: "${IKE_BIN:?required pinned Ike}"
: "${IKE_SOURCE_DIR:?required pinned Ike checkout}"
: "${IKE_EXPECTED_SHA256:?required checked Ike digest}"
: "${ICK_CC:?required source-built ICK}"
: "${ICK_SOURCE_DIR:?required pinned ICK checkout}"
: "${HOST_LINKER:?required object-only linker}"

expect_block() {
    label=$1
    shift
    if "$@" > "$tmp/$label.log" 2>&1; then
        echo "FAIL: prohibited build was accepted: $label" >&2
        exit 1
    fi
}
require_text() {
    grep -Fq "$2" "$tmp/$1.log" || {
        echo "FAIL: $1 did not report expected refusal: $2" >&2
        cat "$tmp/$1.log" >&2
        exit 1
    }
}
require_failed_receipt() {
    file=$1
    test -s "$file" || { echo "FAIL: missing receipt $file" >&2; exit 1; }
    grep -Fqx "$(printf 'schema\tike-build-v1')" "$file"
    grep -Fqx "$(printf 'final_result\tFAIL')" "$file"
}

# Execute an ICK-specific lexical/semantic proof, not just --version.
# Ordinary stock GCC cannot parse ←, ×, ÷; the pinned ICK must compile
# the maintained exact ICK source fixture with its own cc1, then run it.
probe_src="$ICK_SOURCE_DIR/ick/source/gcc/testsuite/gcc.dg/ick-division-glyph.c"
test -s "$probe_src" || { echo "Missing pinned ICK glyph probe" >&2; exit 1; }
"$ICK_CC" -O2 -std=c11 -Wall -Wextra -Werror -pedantic -fPIC \
    -c "$probe_src" -o "$tmp/ick-dialect.o"
readelf -h "$tmp/ick-dialect.o" | grep -q 'Class:.*ELF64'
readelf -h "$tmp/ick-dialect.o" | grep -q 'Machine:.*Advanced Micro Devices X86-64'
"$HOST_LINKER" "$tmp/ick-dialect.o" -o "$tmp/ick-dialect"
"$tmp/ick-dialect"
sha256sum "$ICK_CC" "$tmp/ick-dialect.o" "$tmp/ick-dialect"
echo "Pinned ICK Unicode assignment/multiplication/division: PASS"

# Missing or substituted orchestrator fails without compiling any project C.
expect_block no-ike env -u IKE_BIN sh "$root/scripts/run-ike.sh" test
require_text no-ike IKE_BIN
expect_block wrong-ike env IKE_BIN=/usr/bin/make sh "$root/scripts/run-ike.sh" test
require_text wrong-ike 'Unverified Ike executable hash'
expect_block bad-ike-digest env IKE_EXPECTED_SHA256=0000000000000000000000000000000000000000000000000000000000000000 \
    sh "$root/scripts/run-ike.sh" test
require_text bad-ike-digest 'Unverified Ike executable hash'
expect_block alt-recipe env IKE_RECIPE_RUNNER=/bin/sh sh "$root/scripts/run-ike.sh" test
require_text alt-recipe 'Alternate Ike recipe runner requires independent qualification'
: > "$tmp/used-receipt.tsv"
expect_block reused-receipt env IKE_RECEIPT="$tmp/used-receipt.tsv" \
    sh "$root/scripts/run-ike.sh" test
require_text reused-receipt 'Refusing to overwrite or reuse an Ike receipt'

# A cached existing build must not turn an absent/fake ICK into success.
test -x "$root/build/rough-fb"
expect_block no-ick env -u ICK_CC IKE_RECEIPT="$tmp/no-ick.tsv" \
    sh "$root/scripts/run-ike.sh" test
require_text no-ick ICK_CC
require_failed_receipt "$tmp/no-ick.tsv"

for fake in /usr/bin/gcc /bin/true; do
    label=$(basename "$fake")
    file="$tmp/fake-$label.tsv"
    expect_block "fake-$label" env ICK_CC="$fake" IKE_RECEIPT="$file" \
        sh "$root/scripts/run-ike.sh" test
    require_text "fake-$label" 'BLOCKED: compiler is not the pinned ICK installation'
    require_failed_receipt "$file"
done

# Even if CC points at a host compiler, the product must still be compiled by
# authenticated ICK. This is a positive real rebuild, not a string-only check.
CC=/usr/bin/gcc IKE_RECEIPT="$tmp/cc-shadow.tsv" \
    sh "$root/scripts/run-ike.sh" test > "$tmp/cc-shadow.log" 2>&1
grep -Fq 'ICK-compiled Sublixel 3x3 + 5x5 and framebuffer native C11 tests: PASS' "$tmp/cc-shadow.log"
grep -Fqx "$(printf 'final_result\tPASS')" "$tmp/cc-shadow.tsv"

# System GNU Make may exist for isolated stage-zero bootstrap, but its
# maintained product entrypoint must fail visibly.
expect_block gnu-make /usr/bin/make -C "$root" all
require_text gnu-make 'GNU Make is forbidden'

echo "Ike and ICK substitution/receipt/fail-closed policy controls: PASS"
