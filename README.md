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

## Build and run

```sh
make
fieldmouse src/rough.fm fixtures/scene.json scene.ops
./build/rough-fb scene.ops scene.ppm 240 200
```

`fieldmouse` means the native Idriç-based interpreter from
[dilapidated-shed/fieldmouse](https://github.com/dilapidated-shed/fieldmouse),
not Node and not a JavaScript transpiler. `make` builds only the C renderer; it
does not secretly substitute Node if Field Mouse is unavailable.

`make test` verifies clipping, strict parsing, coverage antialiasing, alpha
composition, tapered widths, and adaptive cubic rasterization. `FIELD_MOUSE=/path/to/fieldmouse
make test-fieldmouse` exercises the actual Field Mouse executor, including
repeated seeded output. The Python numeric oracle in `test-fieldmouse` checks the
initial geometry against pinned Rough.js math; it does not replace Field Mouse
execution.

For a direct renderer example that does not require Field Mouse:

```sh
./build/rough-fb fixtures/coverage.ops coverage.ppm 200 160
```

Scene format uses `seed` plus `shapes`; each shape has a `type` and coordinates,
optional `roughness`, `bowing`, `maxRandomnessOffset`, `preserveVertices`,
`disableMultiStroke`, and `color` (an RGB triplet). Supported types: `line`,
`rectangle`, `polyline`, `polygon`.

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
