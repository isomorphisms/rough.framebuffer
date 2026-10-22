# rough.framebuffer

A direct-framebuffer adaptation of [Rough.js](https://github.com/rough-stuff/rough).

This is intended to be a **Field Mouse translation of the drawing algorithms**, not
an embedded copy of Rough.js and not a Canvas/SVG wrapper. Field Mouse emits
geometry commands; a small C renderer writes them into caller-owned color and
optional depth buffers. The CLI writes PPM only as a portable inspection format,
not as an intermediate rendering dependency.

The geometry slice implements seeded, bowed double-stroke **lines, polylines,
polygons and rectangles**. The native renderer treats those operations as
continuous strokes rather than integer pixel walks: it provides coverage
antialiasing, straight-alpha source-over composition, round caps, linearly tapered
widths, and adaptive cubic subdivision. It also fills flat-color screen-space
triangles with barycentric depth interpolation and a half-open shared-edge rule.
The native surface layer also supplies a camera, near-plane clipping and
depth-aware 2×2 triangle sampling. The smooth-lighting layer supplies
perspective-correct attributes. Ellipses, SVG paths, hachures, textured brushes
and independently selectable joins remain outside the implemented stroke slice.

[Mathematical and rendering ownership](docs/rendering-boundary.md) defines the
boundary with Fulton, applications, Android packaging and WIGGLE-VIDEO-01.

## Owned toolchains only

The **authoritative build orchestrator is [Ike](https://github.com/dilapidated-shed/ike)** with its checked `Ikefile` and `ike-build-v1` receipt. Consumer C code is compiled exclusively by pinned source-built **[ICK](https://github.com/dilapidated-shed/ick)**. ICKY remains the developing parser; Ithon/ILua and other owned tools must be used only at stages they actually support.

GNU Make is explicitly blocked by the top-level and Subpixel `Makefile` stubs. No stock host compiler or GNU Make product fallback is supported. The checked entrypoint is `sh scripts/run-ike.sh test`, which demands pinned compiled Ike, an independently anchored executable digest and a fresh receipt path. The Android NDK gap lane is declared separately. Refer to [owned-toolchain qualification](subpixel/qualification/README.md).

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

Ike's `test` target executes ICK-compiled C tests for Subpixel and the framebuffer,
including triangle edge ownership, winding, depth testing and interpolation. The
continuous-stroke native API checks and both offscreen dash-phase source mutants
are preserved under the same checked entrypoint. The complete raster fixture
set runs separately through the accepted mandatory Ithon host frontend. Field Mouse integration remains a
separate, unqualified end-to-end obligation.

Direct renderer examples that do not require Field Mouse:

```sh
./build/rough-fb fixtures/coverage.ops coverage.ppm 200 160
./build/rough-fb fixtures/triangles.ops triangles.ppm 210 170
```

Scene format uses `seed` plus `shapes`; each shape has a `type` and coordinates,
optional `roughness`, `bowing`, `maxRandomnessOffset`, `preserveVertices`,
`disableMultiStroke`, and `color` (an RGB triplet). Supported Field Mouse shape
types remain `line`, `rectangle`, `polyline`, and `polygon`; triangle operations
currently enter at the native stream or C API layer.

## Native Subpixel visual comparison (candidate)

The ICK-built `subpixel/examples/preview.c` compares the historical continuous-stroke renderer, quadratic 3×3, cubic 5×5, and automatic whole-path selection in four 96×72 panels. The whole-path API adaptively partitions the source and checks the final masks against a fine polyline reference before committing coverage.

From a host with the pinned source-built ICK and Ike environment configured above:

```sh
sh scripts/run-ike.sh preview
```

The retained `subpixel/build/curve-3x3-vs-5x5.ppm` filename now holds a 384×72 P6 comparison. The target also builds the versioned native path consumer and produces 32 font specimen panels with actual sample masks and count/coverage metrics. See [scope and limitations](subpixel/examples/README.md) and [the whole-path contract](subpixel/PATH.md). These are host CPU results; Android and 3D depth acceptance remain separate.
## Operation stream

One command occupies one line. Coordinates, depths, and widths are floating-point
values; colors remain integer RGB values. Defaults are black, opacity `1`, and
width `1`.

```text
P red green blue
A opacity
W width
M x y
L x y [endWidth]
C control1X control1Y control2X control2Y endX endY [endWidth]
T x0 y0 depth0 x1 y1 depth1 x2 y2 depth2
```

`A` accepts opacity from `0` through `1`. `W` requires a positive width. When the
optional final width is present on `L` or `C`, width varies linearly across that
operation and the final value becomes the current width for the next operation.
`T` fills one triangle using the current color and opacity. It does not alter the
current pen position or stroke width. Old `P/M/L/C` streams remain valid.

Pixel centers use integer coordinates. A width-one horizontal stroke at `y = 2`
therefore fully covers row 2, preserving the original stream-to-pixel convention;
non-axis-aligned stroke edges receive fractional coverage. Triangle interiors use
pixel-center sampling and a top-left half-open rule, so two consistently specified
faces sharing an edge neither crack nor double-own a sample.

## Color and depth surfaces

The public `rough_framebuffer` contains an allocated, straight-alpha `0xAARRGGBB`
color surface with explicit width, height, and stride. Its optional `float` depth
surface is also caller-owned and has its own stride. Smaller depth values are
nearer; callers should clear the active depth region to `INFINITY` before a frame.
If the depth pointer is `NULL`, triangles use submission order without depth
testing. `rough_fill_triangle` exposes the same primitive directly without a text
stream.

Depth varies linearly in screen space across a triangle. A passing fragment writes
depth and then blends its color. Partly translucent triangles therefore still
occlude later geometry; order-independent transparency is outside this slice.
Strokes do not currently read or write depth.

The native caller can own an Android surface or a mapped framebuffer; the library
itself does not open `/dev/fb0` and does not claim Android packaging or touch
integration. The renderer allocates no internal image-sized storage.

## ICK-only compilation for the dotted framebuffer candidate

The user requires **ICK exclusively** for the Billiard consumer. On this branch
the product Makefile no longer accepts default stock C compilation: it must
receive an identified, source-built ICK compiler and the matching exact source
checkout. The pinned GitHub workflow uses the owned `ick-host` action and
rejects both missing and stock C compilers. ICK's executable name ends in
`gcc` only because its compiler implementation is GCC-derived; verify the
commit and owning `cc1`, not its filename.

Any old GCC/Clang-created dot-stroke images remain historical and do not
establish ICK-only acceptance. This branch supplies the required exact-head
ICK output to `isomorphisms/billiard`.
## Translucent dotted strokes (stacked on coverage-strokes)

The `renderer/continuous-dots` candidate extends the reviewed `P/A/W/M/L/C`
geometry stream with `D ON OFF` in framebuffer pixel units:

```text
P 48 130 165
A 0.20
W 1.1
D 1 6
M 10 20
L 85 20
L 85 105
D 0 0
```

`D` repeats ON units painted, then OFF units unpainted. Each nonzero interval
must be at least 0.25 and at most 1,000,000. `D 0 0` switches back to a solid
stroke; any other zero/negative interval is rejected. Pattern phase is based on
**the entire mathematical segment length**, including portions outside the
viewport. Successive `L` segments and flattened `C` chords carry phase
continuously across joints; `M` and `D` reset phase to zero.

The painter's stroke may be rough, but mathematical collision geometry belongs
to the consumer. Alpha and width remain independent of the dash pattern.
The coverage and depth work from main is retained together with these patterned
strokes. Native API fixtures check phase after a wholly clipped cubic and its
following line, alpha blending, and unchanged depth. Two production mutants
discard the clipped curve's phase or reset incoming segment phase; both must
be rejected by that same native oracle. Every application and native-test C
compile uses the pinned ICK frontend. The inherited raster harness is now
checked Ithon, including its dot, clipping, depth and malformed-input fixtures.
The unchanged Field Mouse Python harness remains separate migration debt.
Android-device acceptance remains separate.

## Pipeline

```text
JSON scene -> Field Mouse geometry -> operation stream
                                        |
                                        v
                       native color buffer + optional depth
                                        |
                                        v
                              PPM (optional fixture)
```

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

## Headless mathematical surface source

The preserved torus/Enneper P/T generator and its real framebuffer-pixel
tests run under the same pinned ICK/Ike test route. This is a source stage,
not an alternative rasterizer or an Android build. See
[the surface integration contract](docs/surfaces.md). Fulton in
`walnut-burgundy` remains upstream for algebraic curves, intersections
and mathematical composition.

## Native display preparation (source qualification)

The original Android art draft's gesture reducer, direct geometry-to-framebuffer
path, and straight-alpha RGBA8888 software presenter are preserved and
independently exercised as native C via pinned ICK/Ike. The old GNU Make
and ad-hoc GCC/Clang build routes have **not** been reintroduced.

The historical NativeActivity source and manifest are retained as unqualified
Android consumers. Actual A1/C67 packaging remains with the maintained
android-NDK, Kitchen and Cat Food boundaries; current source admission does not
claim a new APK, real device run or signer acceptance. See
[Android source scope](android/README.md).

## Depth-aware 2×2 antialiasing source

The retained native `rough_draw_surface_aa2` independently tests color and
depth at four samples per software pixel before resolving in premultiplied
alpha, with caller-owned scratch. Its ICK/Ike native tests reject the original
single-center-sample approximation. Android frame timing, physical screen
quality and smooth normals remain separate qualifications.

## Visual 3×3 versus 5×5 Subpixel diagnostic

The preserved native diagnostic draws the **same oblique curve** in quadratic
3×3 and cubic 5×5 occupancy neighborhoods, without substituting this fixture
for genuine 3D triangle antialiasing. Its exact integer-rational 8×8 coverage,
colors, padding, clipping and deterministic rerender assertions run under ICK
and the maintained Ike test route. The test-only Android overlay is source
only until packaging and A1/C67 validation are repeated. No duplicate
Subpixel library or GNU Make build is imported from the old Android branch.

## Smooth analytic surface lighting

The retained smooth-shading candidate interpolates analytic surface
normals and base colors perspective-correctly at each covered sample after
camera near-plane clipping, while preserving the flat P/T stream,
sample coverage and z-depth conventions. Torus seams remain periodic.
The independent numerical test checks central-difference normals,
continuous seams, projective interpolation, varying fragment lighting,
actual surface depth equality and a constant-normal source mutant.

This is a host source implementation under ICK/Ike. Its source host
qualification does not prove a new C67 APK, measured graphics performance,
or physically smooth image. Fulton remains upstream for mathematical
intersection and composition semantics.
