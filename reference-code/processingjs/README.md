# Processing.js — reference code only

This directory is a **verbatim copy of the 28 upstream `src/` files** in
[Processing.js v1.6.6](https://github.com/processing-js/processing-js/tree/v1.6.6),
the project's final release. Processing.js was archived in December 2018.

Upstream repository: https://github.com/processing-js/processing-js

Pinned upstream commit: `caa7c9e16d8e784b0f3325a06fb12b6ee013829c`

- `src/`: all 28 original, unmodified JavaScript source files.
- `LICENSE` and `AUTHORS`: copied from upstream `bundle/`.
- `MANIFEST.tsv`: upstream Git blob SHA-1 values and byte counts for each file.
- Excluded: generated `processing.js` and `processing.min.js` bundles,
  package/build scaffolding, third-party `lib/`, and upstream tests.

Useful places to read for native adaptations:

- `src/Processing.js` — drawing and sketch lifecycle, Canvas and WebGL.
- `src/P5Functions/touchmouse.js` — mouse/touch dispatch and state.
- `src/Objects/PShape.js` / `src/Objects/PShapeSVG.js` — shape/path handling.
- `src/Objects/PMatrix2D.js` / `src/Objects/PMatrix3D.js` — transforms.
- `src/P5Functions/Math.js` — mathematical helpers.
- `src/Parser/Parser.js` — Processing language transformation.

This is **not** an embedded browser engine, Field Mouse program, or integration
with the framebuffer. Processing.js remains a historical algorithm reference.
New adaptations belong in the active Field Mouse and C code and should be
tested against the actual framebuffer output. Nothing in this directory is
required by `make` or by the packaged renderer.

The original Processing.js licensing and contributor credits are preserved in
`LICENSE` and `AUTHORS`.
