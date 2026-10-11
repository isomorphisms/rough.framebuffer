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

The C framebuffer renderer and Sublixel library are compiled **only by the pinned
source-built [ICK](https://github.com/dilapidated-shed/ick) C compiler**.
`make`, `make test`, and `make -C sublixel test` now reject missing ICK instead
of selecting host `cc`, GCC, Clang, or an Android NDK fallback. Specify
`ICK_CC`, `ICK_SOURCE_DIR`, and a declared link-only `HOST_LINKER` as documented
in [Sublixel's exact toolchain contract](sublixel/qualification/README.md).

The host GCC/G++ bootstrap of the not-yet-self-hosting ICK compiler and the
system runtime/link driver are explicitly identified **stage-zero / link-only**
dependencies; neither is allowed to compile this repository's C source.
The prior GCC/Clang differential CI lanes have been removed. The native
tests include an ICK-compiled framebuffer smoke test. Legacy Python test
scripts are not part of the current acceptance run; Field Mouse integration
requires an explicitly named Ithon and remains unqualified for that interpreter.

For other languages, select only an actually supported owned toolchain
(ICKY, IKE, Ithon, ILua, etc.). An unsupported capability blocks the build
rather than silently substituting another implementation.

## Build and run

The verified ICK environment is required; plain `make` with no compiler
binding correctly fails closed.

```sh
ICK_CC=/absolute/ick/stage/bin/x86_64-linux-gnu-gcc \
ICK_SOURCE_DIR=/absolute/pinned/ick/checkout \
HOST_LINKER=/usr/bin/x86_64-linux-gnu-gcc \
make test
fieldmouse src/rough.fm fixtures/scene.json scene.ops
./build/rough-fb scene.ops scene.ppm 240 200
```

`fieldmouse` means the native Idriç-based interpreter from
[dilapidated-shed/fieldmouse](https://github.com/dilapidated-shed/fieldmouse),
not Node and not a JavaScript transpiler. `make` compiles the C renderer and native tests exclusively through ICK; it does not run Field Mouse or substitute Node for it.

`make test` executes ICK-compiled C tests for Sublixel and the framebuffer. `FIELD_MOUSE=/path/to/fieldmouse
make test-fieldmouse` exercises the actual Field Mouse executor, including
repeated seeded output. The older Python numeric oracle is historical reference material until it is
qualified under Ithon; it does not replace Field Mouse execution.

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

## Reference code

[`reference-code/processingjs/`](reference-code/processingjs/) preserves the
original source from archived [Processing.js v1.6.6](https://github.com/processing-js/processing-js/tree/v1.6.6),
including its license and authors. It is read-only comparison material for drawing,
transforms, shapes, parsing, and touch/mouse input, **not a build or runtime
requirement** of the Field Mouse/C framebuffer pipeline.
