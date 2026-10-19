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

## Geometry review

The continuous masters use round-ended strokes: the occupied shape is the
union of disks with radius half the declared stroke width along each centerline.
The reference sampler approximates that union with 96 chords per cubic. It
does not fill the interiors of the closed loops; those interiors are counters.

- `O` consists of four joined cubic quarters, symmetric about x = 2.5 and
  y = 3.5. Its centerline extrema are x = 0.9, 4.1 and y = 0.75, 6.25;
  the stroke width is 0.72.
- `C` is an open three-cubic path. Its endpoints are (4.1, 1.25) and
  (4.1, 5.75); the 0.72-wide endpoint disks remain separated. Closing that
  opening changes the glyph, rather than repairing its geometry.
- `S` is one open three-cubic path. The middle cubic changes signed turning
  from negative to positive; its y derivative remains positive, so this is an
  inflection rather than a stationary-point artifact. The stroke width is 0.72.
- `8` uses separate upper and lower closed four-cubic centerlines with stroke
  width 0.66. Their strokes overlap near the waist while their empty counters
  remain distinct. A two-subpath declaration alone does not establish that
  sampled result.

The tests flood-fill empty samples in the complete 40×56 binary lattice using
four-neighbour connectivity. At the unit origin and scale they find one bounded
empty component for `O`, none for `C` or `S`, and two for `8`. This checks the
actual emitted sample bits, separately from vector closure flags. It makes no
claim about the topology of a thresholded 5×7 grayscale image, transformed
glyphs, or the later production path renderer. Zero width and a finite width
whose squared radius overflows are rejected without changing output arrays.

## Whole-path font preview

`font_5x7_preview.c` consumes the native `cp_path_rasterize` API rather than
calling the didactic font sampler. Its executable takes one existing output
directory. All continuous modes reuse the E1 controls, use round stroke union,
and resolve each complete glyph once through `rough_blend_subpixel_coverage`.
The transform translates material cell edges to the path API's pixel-center
coordinates by subtracting 0.5 from each screen origin. Width scales with the
glyph; advance, bearing and baseline stay separate from occupancy.

Each P6 PPM has rows `O`, `C`, `S`, `8`. Columns are:

| Column | Geometry / producer |
| --- | --- |
| 1 | Exact hard-edged bitmap rectangles |
| 2 | Existing `rough_render_stream` cubic stroke baseline |
| 3 | Forced quadratic 3×3 whole-path producer |
| 4 | Forced cubic 5×5 whole-path producer |
| 5 | Whole-path auto selector, with its actual choice in the metrics |

Each mask-producing tile shows individual binary samples with gray macrocell
boundaries, a magnified grayscale resolve, and the actual native-size resolve.
The grayscale and binary views read the identical mask words. The legacy
baseline exposes pixels rather than sample masks, so its fine-grid area stays
blank and its metrics say `MASKS_UNAVAILABLE`. A refused whole-path strategy
gets a red tile and an explicit status; it never borrows a successful image.
Distinct backed strategies can converge to identical sample decisions while
using different patch counts and work.

The 32 panels cover 1×, 1.5×, 2× and 4× raster scaling, origins −1/8, 0, +1/8,
and a deliberately clipped −2.5 origin, in both polarities. Bitmap rectangles
remain hard edged under fractional translation: only samples within the exact
translated rectangles are occupied. At integer scale and origin zero every
bitmap cell remains exactly 0 or 64 samples.

`font-masks.tsv` saves all output masks and popcounts, including every
individual bit of the unit-scale 40×56 glyphs. `font-metrics.tsv` records the
strategy, actual selected algorithm and reason, work, declared caps, bounded
owned memory, occupancy, checksum, and worst/mean bit disagreement against the
path producer's own adaptive-polyline diagnostic reference. This diagnostic
is **not independent acceptance**. The independent M1 oracle, exact source and
compiler pins, output SHA-256 values, and physical-device evidence remain
separate requirements. FNV-1a identifies the canonical little-endian mask
serialization in the metrics; SHA-256 of the actual output files belongs in
the build receipt.

Every successful mask tile checks its resolved pixel values against the actual
popcounts, rejects empty/nonfractional continuous output, and checks that
coverage row padding remains untouched. The tool owns fixed arrays, with no
image-sized heap allocation. Its renderer options cap work at 100,000 patches,
200,000,000 sample tests and depth 24 per glyph, with tolerance 0.01 pixels
and maximum two diagnostic XOR bits per pixel.
