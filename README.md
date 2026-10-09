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

`make test` verifies a C raster fixture. `FIELD_MOUSE=/path/to/fieldmouse
make test-fieldmouse` exercises the actual Field Mouse executor, including
repeated seeded output. A separate Node oracle test may be used to compare
numeric operations to the pinned Rough.js implementation but does not count
as Field Mouse execution.

Scene format uses `seed` plus `shapes`; each shape has a `type` and coordinates,
optional `roughness`, `bowing`, `maxRandomnessOffset`, `preserveVertices`,
`disableMultiStroke`, and `color` (an RGB triplet). Supported types: `line`,
`rectangle`, `polyline`, `polygon`.

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