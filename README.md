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
It does not yet implement ellipses, SVG paths, hachures, textured brushes, joins as
an independently selectable style, antialiased triangle edges, perspective-correct
attributes, a general camera API, or arbitrary mesh clipping. The separate native
surface generator provides fixed-camera projection and near clipping for two
specific mathematical meshes.

## Build and run

```sh
make
fieldmouse src/rough.fm fixtures/scene.json scene.ops
./build/rough-fb scene.ops scene.ppm 240 200
```

`fieldmouse` means the native Idriç-based interpreter from
[dilapidated-shed/fieldmouse](https://github.com/dilapidated-shed/fieldmouse),
not Node and not a JavaScript transpiler. `make` builds the C renderer and
an independent native mathematical surface stream generator; it does not
secretly substitute Node if Field Mouse is unavailable.

`make test` verifies clipping, strict parsing, stroke coverage, alpha composition,
tapered widths, adaptive cubic rasterization, triangle edge ownership, winding,
depth testing, and interpolated depth, plus source-to-renderer mathematical
surface frames and near-plane clipping. `FIELD_MOUSE=/path/to/fieldmouse make
test-fieldmouse` exercises the actual Field Mouse executor, including repeated
seeded output. The Python numeric oracle in `test-fieldmouse` checks the initial
geometry against pinned Rough.js math; it does not replace Field Mouse execution.

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

## Rotating mathematical surfaces

The native scene generator emits shaded triangles into the same P/T operation
stream accepted by the existing depth-tested renderer. It supports a torus and
Enneper's minimal surface with controllable rotation, fixed-camera projection,
near-plane clipping, and two-sided diffuse/ambient face shading:

```sh
./build/rough-surface enneper 25 320 240 > build/enneper.ops
./build/rough-fb build/enneper.ops build/enneper.ppm 320 240
./build/rough-surface torus 60 320 240 > build/torus.ops
./build/rough-fb build/torus.ops build/torus.ppm 320 240
```

This produces headless inspectable PPM frames, **not** an Android APK,
interactive animation, or a new Field Mouse frontend. See
[the projection, shading, input, and acceptance contract](docs/surfaces.md).

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
