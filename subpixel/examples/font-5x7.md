# Continuous 5×7 glyph masters

`font_5x7.c` keeps two interpretations separate:

- the exact 35-cell `#`/`.` bitmap control;
- continuous ordered Bézier centerlines in the same `[0,5] × [0,7]` design
  coordinates, with explicit stroke width, subpaths and open/closed topology.

The masters are `O` (one closed path), `C` (one open path), `S` (one open
inflected path), and `8` (two separately closed paths). Advance, bearing and
baseline remain metrics rather than occupied cells. The curve data is authored
directly; the bitmap does not determine the control points.

`font_5x7_dump.c` emits every individual midpoint decision as a **40×56 binary
lattice**, divided by heavy text boundaries into 35 cells of 8×8 samples. It
then prints each cell's 64-bit mask and popcount. Bit `8×sample_y+sample_x`
corresponds to
`(column+(sample_x+0.5)/8, row+(sample_y+0.5)/8)`.

The fixed-subdivision sampler is reviewable reference math for inspecting the
masters. It does not claim to replace or qualify the whole-path producer owned
by Flexible Pipes #145. E2 must feed reviewed geometry through the real
Subpixel producer and display its identical masks in fine and resolved views.
