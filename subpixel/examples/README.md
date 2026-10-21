# Actual native specimens

Ike's `preview` target builds all native C through pinned ICK, then runs:

- `subpixel-preview`: a 384×72 comparison, left to right historical stroke,
  quadratic 3×3, cubic 5×5, automatic. Its oblique line, inflection, crossing,
  reversed duplicate and viewport-edge clipping use real geometry. The legacy
  retained filename is `curve-3x3-vs-5x5.ppm`.
- `font-5x7-preview`: O/C/S/8 at scales 1, 1.5, 2 and 4, fractional phases
  −1/8, zero, +1/8, a clipped origin, and both polarities. Each panel compares
  bitmap control, actual historical stroke, quadratic, cubic and auto. Every
  binary microcell, fractional grayscale cell and count comes from the same
  masks. Unit-scale glyphs contain 40×56 sample bits, not 35 binary pixels.
- `path-render`: the versioned geometry consumer used by Conway. It accepts
  whole-path.txt and external geometry with explicit limits and per-shape union.

Font outputs include masks/counts TSV and measured work, strategy and error
metrics. Converged quadratic/cubic outputs may agree; differing pictures are
not required when both algorithms satisfy the diagnostic. Metrics preserve
their different actual work. Empty and fabricated output are not acceptance.

See [the path contract](../PATH.md) and [glyph geometry](font-5x7.md).
The preview is native host execution, with no Android or depth-correct 3D claim.
Independent source/mask/topology acceptance remains in AICI.
