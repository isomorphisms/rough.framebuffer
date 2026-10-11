#!/bin/sh
# Required Ike orchestration. Source/runner/hash/receipt are all explicit.
# No GNU Make, arbitrary PATH 'make', or system/Ike-name lookalike fallback.
set -eu
root=$(CDPATH= cd "$(dirname "$0")/.." && pwd)
target=all
if [ "$#" -gt 1 ]; then echo "usage: $0 [all|test|clean|policy-check|preview]" >&2; exit 2; fi
if [ "$#" -eq 1 ]; then target=$1; fi
case "$target" in all|test|clean|policy-check|preview) ;; *) echo "unsupported Ike target" >&2; exit 2 ;; esac

: "${IKE_BIN:?IKE_BIN must identify the verified owned Ike executable}"
: "${IKE_SOURCE_DIR:?IKE_SOURCE_DIR must name the pinned Ike source checkout}"
: "${IKE_EXPECTED_SHA256:?IKE_EXPECTED_SHA256 must be independently anchored}"
: "${IKE_RECEIPT:?IKE_RECEIPT must be an unused absolute receipt path}"

case "$IKE_BIN" in /*) ;; *) echo "IKE_BIN must be absolute" >&2; exit 2 ;; esac
case "$IKE_SOURCE_DIR" in /*) ;; *) echo "IKE_SOURCE_DIR must be absolute" >&2; exit 2 ;; esac
case "$IKE_RECEIPT" in /*) ;; *) echo "IKE_RECEIPT must be absolute" >&2; exit 2 ;; esac
test -x "$IKE_BIN"
if [ -n "${IKE_RECIPE_RUNNER-}" ]; then
    echo "Alternate Ike recipe runner requires independent qualification" >&2
    exit 3
fi
test ! -e "$IKE_RECEIPT" || {
    echo "Refusing to overwrite or reuse an Ike receipt" >&2
    exit 2
}
ike_pin=a92fe68bf8aad8dfcfe9f7a7abd84e07290f7fe0
test "$(git -C "$IKE_SOURCE_DIR" rev-parse HEAD)" = "$ike_pin"
actual=$(sha256sum "$IKE_BIN" | cut -d ' ' -f 1)
test "$actual" = "$IKE_EXPECTED_SHA256" || {
    echo "Unverified Ike executable hash; no system make substitution" >&2
    exit 3
}

ikfilehash=$(sha256sum "$root/Ikefile" | cut -d ' ' -f 1)
identity=sha256:$ikfilehash
(
    cd "$root"
    IKE_RECEIPT="$IKE_RECEIPT" \
    IKE_IKEFILE_IDENTITY="$identity" \
    "$IKE_BIN" "$target"
)

hex_string() { printf %s "$1" | od -An -tx1 | tr -d ' \n'; }
tab=$(printf '\t')
case "$target" in
    all|test) expected_recipe='sh sublixel/qualification/test-ick.sh' ;;
    clean) expected_recipe='rm -rf build sublixel/build' ;;
    policy-check) expected_recipe='sh qualification/test-compiler-policy.sh' ;;
    preview) expected_recipe='sh sublixel/examples/build-preview.sh' ;;
esac
hex_target=$(hex_string "$target")
hex_identity=$(hex_string "$identity")
hex_recipe=$(hex_string "$expected_recipe")
hex_runner=$(hex_string 'POSIX-system()')

# A final_result=PASS with zero executed rules is not a build. Validate
# the complete eight-line v1 receipt, including exactly one real recipe
# execution at exit 0. Reject extra rows, skipped/cached targets and
# receipts whose target or recipe was substituted.
match_line() {
    expected_line=$1
    if ! IFS= read -r actual_line || [ "$actual_line" != "$expected_line" ]; then
        echo "Ike recipe receipt mismatch: $IKE_RECEIPT" >&2
        exit 3
    fi
}
{
    match_line "schema${tab}ike-build-v1"
    match_line "selected_target_hex${tab}$hex_target"
    match_line "ikefile_identity_hex${tab}$hex_identity"
    match_line "recipe_runner_mode${tab}posix-system"
    match_line "recipe_runner_identity_hex${tab}$hex_runner"
    if [ "$target" = preview ]; then
        hex_test=$(hex_string test)
        hex_test_recipe=$(hex_string 'sh sublixel/qualification/test-ick.sh')
        match_line "rule${tab}0${tab}$hex_test"
        match_line "recipe${tab}1${tab}$hex_test${tab}$hex_test_recipe${tab}0"
        match_line "rule${tab}2${tab}$hex_target"
        match_line "recipe${tab}3${tab}$hex_target${tab}$hex_recipe${tab}0"
    else
        match_line "rule${tab}0${tab}$hex_target"
        match_line "recipe${tab}1${tab}$hex_target${tab}$hex_recipe${tab}0"
    fi
    match_line "final_result${tab}PASS"
    extra_line=
    if IFS= read -r extra_line || [ -n "$extra_line" ]; then
        echo "Ike recipe receipt mismatch: extra trailing data" >&2
        exit 3
    fi
} < "$IKE_RECEIPT"
echo "Pinned Ike: $ike_pin"
echo "Ike ELF SHA256: $actual"
echo "Ikefile SHA256: $ikfilehash"
echo "Ike build receipt: $IKE_RECEIPT"
