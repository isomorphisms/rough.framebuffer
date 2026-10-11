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
grep -Fqx "schema${tab}ike-build-v1" "$IKE_RECEIPT"
grep -Fqx "final_result${tab}PASS" "$IKE_RECEIPT"
grep -Fqx "recipe_runner_mode${tab}posix-system" "$IKE_RECEIPT"
grep -Fqx "selected_target_hex${tab}$(hex_string "$target")" "$IKE_RECEIPT"
grep -Fqx "ikefile_identity_hex${tab}$(hex_string "$identity")" "$IKE_RECEIPT"
echo "Pinned Ike: $ike_pin"
echo "Ike ELF SHA256: $actual"
echo "Ikefile SHA256: $ikfilehash"
echo "Ike build receipt: $IKE_RECEIPT"
