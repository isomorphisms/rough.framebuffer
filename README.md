# rough.framebuffer

A direct-framebuffer adaptation of [Rough.js](https://github.com/rough-stuff/rough).

This is intended to be a **Field Mouse translation of the drawing algorithms**, not
an embedded copy of Rough.js and not a Canvas/SVG wrapper. Field Mouse emits
geometry commands (`M`, `C`, `L`) plus pen colors (`P`); a small C renderer
writes them into a caller-owned 32-bit pixel buffer. The CLI writes PPM only as
a portable inspection format, not as an intermediate rendering dependency.

The first slice implements seeded, bowed double-stroke **lines, polylines,
polygons and rectangles**. It does not yet implement ellipses, SVG paths,
filled shapes, hachures, textured brushes, or stroke-width variation.

## Owned toolchains only

The **authoritative build orchestrator is [Ike](https://github.com/dilapidated-shed/ike)** with its checked `Ikefile` and `ike-build-v1` receipt. Consumer C code is compiled exclusively by pinned source-built **[ICK](https://github.com/dilapidated-shed/ick)**. ICKY remains the developing parser; Ithon/ILua and other owned tools must be used only at stages they actually support.

GNU Make is explicitly blocked by the top-level and Sublixel `Makefile` stubs. There is no `CC=cc`, GCC, Clang, NDK or GNU Make product fallback. The checked entrypoint is `sh scripts/run-ike.sh test`, which demands a pinned compiled Ike, caller-supplied expected executable digest (independent verification pending) and fresh receipt path. Refer to [owned-toolchain qualification](sublixel/qualification/README.md).

**Explicit stage-zero exceptions:** Building the GCC-derived ICK compiler still requires the system host C/C++ bootstrap toolchain and GNU Make; a declared system linker supplies Linux CRT/libc/libm after ICK generated the project and Ike objects. None of these stages compiles any consumer C source. This is not yet a self-hosting complete-toolchain claim.

## Build and run

A pinned, qualified Ike and ICK must both be supplied (the workflow builds them from source). Example from a qualified host:

```sh
ICK_CC=/absolute/ick/stage/bin/x86_64-linux-gnu-gcc \
ICK_SOURCE_DIR=/absolute/pinned/ick/checkout \
HOST_LINKER=/usr/bin/x86_64-linux-gnu-gcc \
IKE_BIN=/absolute/verified/ike \
IKE_SOURCE_DIR=/absolute/pinned/ike/checkout \
IKE_EXPECTED_SHA256="$PINNED_IKE_SHA256" \
IKE_RECEIPT=/absolute/unused/receipt.tsv \
sh scripts/run-ike.sh test
fieldmouse src/rough.fm fixtures/scene.json scene.ops
./build/rough-fb scene.ops scene.ppm 240 200
```

`fieldmouse` means the native Idriç-based interpreter from
[dilapidated-shed/fieldmouse](https://github.com/dilapidated-shed/fieldmouse),
not Node and not a JavaScript transpiler. Ike invokes the checked ICK C build and native tests; it does not run Field Mouse or substitute Node for it.

Ike's `test` target executes ICK-compiled C tests for Sublixel and the framebuffer. The legacy Field Mouse integration and Python numerical oracle remain separate, unqualified under Ithon; neither runs automatically and neither is counted toward this owned-toolchain qualification.

Scene format uses `seed` plus `shapes`; each shape has a `type` and coordinates,
optional `roughness`, `bowing`, `maxRandomnessOffset`, `preserveVertices`,
`disableMultiStroke`, and `color` (an RGB triplet). Supported types: `line`,
`rectangle`, `polyline`, `polygon`.

## Native Sublixel visual comparison (candidate)

The ICK-built `sublixel/examples/preview.c` consumes both local curvature APIs and accumulates real 8×8 subpixel occupancy into two side-by-side 96×72 panels. It draws a genuinely slanted cubic Bézier stroke rather than a horizontal synthetic line or copied bitmap.

From a host with the pinned source-built ICK and Ike environment configured above:

```sh
sh scripts/run-ike.sh preview
```

The resulting `sublixel/build/curve-3x3-vs-5x5.ppm` is a P6 image of size 192×72. The example runs genuine slant, partial-coverage, duplicate-union, deterministic frame and PPM format tests, then records the image SHA-256. See [the example's scope and limitations](sublixel/examples/README.md). It is a **native CPU preview only**, not an Android APK or a claimed repair of 3D triangle aliasing on C67.

## Pipeline

```
JSON scene -> Field Mouse geometry -> M/C/L/P operation stream
                                        |
                                        v
                             native software pixel buffer
                                        |
                                        v
                              PPM (optional fixture)
```

The renderer's public `rough_render_stream` API accepts an allocated `uint32_t`
pixel surface and explicit width, height, and stride. Its native caller can own
an Android surface or a mapped framebuffer; the library itself does not open
`/dev/fb0` and does not claim Android packaging or touch integration.

## Provenance

Geometry for strokes is adapted from `src/renderer.ts` and seeded randomness
from `src/math.ts` in [Rough.js](https://github.com/rough-stuff/rough)
commit `56a2762171b1294d643501e8d14f120db6b27bd7` (Preet Shihn, MIT).
The upstream notice is retained in `LICENSE-ROUGHJS`.

## Reference code

[`reference-code/processingjs/`](reference-code/processingjs/) preserves the
original source from archived [Processing.js v1.6.6](https://github.com/processing-js/processing-js/tree/v1.6.6),
including its license and authors. It is read-only comparison material for drawing,
transforms, shapes, parsing, and touch/mouse input, **not a build or runtime
requirement** of the Field Mouse/C framebuffer pipeline.
