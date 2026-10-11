# Mandatory owned build orchestration and compilation

The authoritative work orchestrator is **Ike**, repository
`dilapidated-shed/ike`, source commit `a92fe68bf8aad8dfcfe9f7a7abd84e07290f7fe0`.
The checked `Ikefile` invokes `sublixel/qualification/test-ick.sh`.
`scripts/run-ike.sh` requires absolute paths to the pinned source checkout
and actual executable, a caller-supplied expected binary SHA-256, and an unused
receipt path. It binds the current Ikefile SHA-256 to an `ike-build-v1`
receipt and verifies the selected target, identity, runner mode and PASS
before claiming success. Neither a PATH-shadowed GNU Make nor any unverified
Ike-named executable can substitute. A missing owned builder must fail.

All five consumer C sources compile exclusively through **source-built ICK**,
`dilapidated-shed/ick@c61e448251744a2f40ad743ebef1a027bdcd2f9d`,
with GCC reference submodule
`6294f1d9e7536e5ffcde09d1528c918d63abfef5`.
The ICK C11 compile script binds exact ICK source, actual installed cc1
binary, target triple, output ELF ABI and object/executable checksums.

- `sublixel/src/curve_patch.c` and `sublixel/tests/test_curve_patch.c`
- `src/framebuffer.c`, `src/main.c`, and `tests/test_raster_ick.c`

Both native test programs run from the actual ICK-produced objects; the
framebuffer CLI is assembled from ICK objects. No direct consumer GCC/Clang
or NDK compilation jobs remain. The two former reference runs are
historical only. Root and Sublixel `Makefile` are explicit GNU Make
refusal stubs. Ike v1 owns the dependency graph without pretending to
parse GNU Make syntax.

## Required host bindings

```sh
ICK_CC=/absolute/ick/stage/bin/x86_64-linux-gnu-gcc \
ICK_SOURCE_DIR=/absolute/pinned/ick/checkout \
HOST_LINKER=/usr/bin/x86_64-linux-gnu-gcc \
IKE_BIN=/absolute/verified/ike \
IKE_SOURCE_DIR=/absolute/pinned/ike/checkout \
IKE_EXPECTED_SHA256=<trusted-64-hex-digest> \
IKE_RECEIPT=/absolute/unused/receipt.tsv \
sh scripts/run-ike.sh test
```

The CI workflow checks out both compiler and orchestrator at immutable
commits, builds ICK from its complete owned source, compiles Ike's `ike.c`
with ICK, then uses the resulting pinned Ike to execute the ICK source
compiler workload and native tests. Missing-Ike and direct GNU Make
negative controls must fail.

**Stage-zero / system-link exceptions:** ICK is GCC-derived and not yet
self-hosting. System GCC/G++ and GNU Make bootstrap the compiler from
pinned GCC reference plus ICK-owned overlays. A system compiler driver is
allowed solely for final CRT/libc/libm *linking of already-produced ICK
objects*, including the ICK-built Ike executable; it never receives a
consumer `.c` file. These exceptions are explicit and audited, not an
all-owned bootstrapping claim. POSIX shell is an explicitly bounded recipe
transport, not a competing build orchestrator. The runner refuses an
unqualified `IKE_RECIPE_RUNNER`.

**Open:** ICKY is not yet the general C parser; Ithon/ILua remain
language-specific; historical Python fixture scripts are not default
acceptance; Android compiler/sysroot/APK/device runtime, registered
Flexible Pipes dispatch and independent ai-ci acceptance remain separate.
Do not equate creation of an issue, an Ikefile, a hosted job or a PR with
those missing gates.
