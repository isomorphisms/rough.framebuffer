# rough.framebuffer

A direct-framebuffer adaptation of [Rough.js](https://github.com/rough-stuff/rough).

This is intended to be a **Field Mouse translation of the drawing algorithms**, not
an embedded copy of Rough.js and not a Canvas/SVG wrapper. Field Mouse emits
geometry commands; a small C renderer writes them into a caller-owned 32-bit pixel
buffer. The CLI writes PPM only as a portable inspection format, not as an
intermediate rendering dependency.

The geometry slice implements seeded, bowed double-stroke **lines, polylines,
polygons and rectangles**. The native renderer now treats those operations as
continuous strokes rather than integer pixel walks: it provides coverage
antialiasing, straight-alpha source-over composition, round caps, linearly tapered
widths, and adaptive cubic subdivision. It does not yet implement ellipses, SVG
paths, filled shapes, hachures, textured brushes, joins as an independently
selectable style, triangles, or a depth buffer.

## Owned toolchains only

The **authoritative build orchestrator is [Ike](https://github.com/dilapidated-shed/ike)** with its checked `Ikefile` and `ike-build-v1` receipt. Consumer C code is compiled exclusively by pinned source-built **[ICK](https://github.com/dilapidated-shed/ick)**. ICKY remains the developing parser; Ithon/ILua and other owned tools must be used only at stages they actually support.

GNU Make is explicitly blocked by the top-level and Subpixel `Makefile` stubs. There is no `CC=cc`, GCC, Clang, NDK or GNU Make product fallback. The checked entrypoint is `sh scripts/run-ike.sh test`, which demands a pinned compiled Ike, caller-supplied expected executable digest (independent verification pending) and fresh receipt path. Refer to [owned-toolchain qualification](subpixel/qualification/README.md).

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

Ike's `test` target executes ICK-compiled C tests for Subpixel and the framebuffer. The legacy Field Mouse integration and Python numerical oracle remain separate, unqualified under Ithon; neither runs automatically and neither is counted toward this owned-toolchain qualification.

For a direct renderer example that does not require Field Mouse:

```sh
./build/rough-fb fixtures/coverage.ops coverage.ppm 200 160
```

Scene format uses `seed` plus `shapes`; each shape has a `type` and coordinates,
optional `roughness`, `bowing`, `maxRandomnessOffset`, `preserveVertices`,
`disableMultiStroke`, and `color` (an RGB triplet). Supported types: `line`,
`rectangle`, `polyline`, `polygon`.

## Native Subpixel visual comparison (candidate)

The ICK-built `subpixel/examples/preview.c` compares the historical continuous-stroke renderer, quadratic 3×3, cubic 5×5, and automatic whole-path selection in four 96×72 panels. The whole-path API adaptively partitions the source and checks the final masks against a fine polyline reference before committing coverage.

From a host with the pinned source-built ICK and Ike environment configured above:

```sh
sh scripts/run-ike.sh preview
```

The retained `subpixel/build/curve-3x3-vs-5x5.ppm` filename now holds a 384×72 P6 comparison. The target also builds the versioned native path consumer and produces 32 font specimen panels with actual sample masks and count/coverage metrics. See [scope and limitations](subpixel/examples/README.md) and [the whole-path contract](subpixel/PATH.md). These are host CPU results; Android and 3D depth acceptance remain separate.
## Operation stream

One command occupies one line. Coordinates and widths are floating-point values;
colors remain integer RGB values. Defaults are black, opacity `1`, and width `1`.

```text
P red green blue
A opacity
W width
M x y
L x y [endWidth]
C control1X control1Y control2X control2Y endX endY [endWidth]
```

`A` accepts opacity from `0` through `1`. `W` requires a positive width. When the
optional final width is present on `L` or `C`, width varies linearly across that
operation and the final value becomes the current width for the next operation.
Old `P/M/L/C` streams remain valid.

Pixel centers use integer coordinates. A width-one horizontal stroke at `y = 2`
therefore fully covers row 2, preserving the original stream-to-pixel convention;
non-axis-aligned edges receive fractional coverage.

## Pipeline

```text
JSON scene -> Field Mouse geometry -> operation stream
                                        |
                                        v
                             native software pixel buffer
                                        |
                                        v
                              PPM (optional fixture)
```

The renderer's public `rough_render_stream` API accepts an allocated, straight-alpha
`0xAARRGGBB` pixel surface and explicit width, height, and stride. Its native caller
can own an Android surface or a mapped framebuffer; the library itself does not
open `/dev/fb0` and does not claim Android packaging or touch integration. The
renderer allocates no internal image-sized storage.

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

## Subpixel terminology

Current product code, headers, paths, executables, diagnostics and workflows use **subpixel**.
The original GitHub issues, PR titles, immutable commits and previously generated receipts
retain their historical spelling; they are not executable interfaces. This migration
is tracked in [SUN #120](https://github.com/isomorphisms/flexible-pipes/issues/120).
The `test`/`preview` Ike recipes run the naming gate, which rejects tracked stale
paths/text and attempts to compile two intentionally obsolete interface probes using
pinned ICK. The native coverage and composition source mathematics are unchanged.
