# Bounded whole-path rendering

`cp_path_rasterize` accepts finite line, quadratic and cubic segments. An affine
transform maps material coordinates to screen pixels; width is explicitly in
screen pixels. Segment starts/ends are explicit. Coverage is the union of round
stroke tubes, including round caps and round joins at shared endpoints. Closed
subpaths explicitly return to their start. Disjoint subpaths acquire no invented
connection. Color, opacity, taper, fill rules and 3D depth stay outside this API.

The automatic selector examines the screen-space third-order jet, tolerance and
sample budget. Forced quadratic and cubic strategies remain available. Every
selection records its reason. The actual historical P/A/W/M/L/C implementation
is preserved as a separately selectable native-consumer baseline: its analytic
fringe and per-segment alpha semantics are intentionally visible in comparisons.
It does not pretend to produce binary occupancy masks. Automatic mask rendering
selects between the two union-preserving neighborhood algorithms.

## Work and failure

Caller-owned destination may have padding and a negative global origin. Two
separate packed scratch planes hold candidate and reference masks. No allocation
occurs inside the path API. Aliasing any of these planes is refused. Inputs and
the report must not alias those buffers. Failure, cancellation, budget exhaustion
and nonconvergence leave destination and report unchanged; scratch is disposable.
Successful coverage is ORed into destination. Resolve once per shape with
`rough_blend_subpixel_coverage`, then clear masks for a different shape/color.

Local subdivision checks tangent monotonicity, nine normal residual samples,
control polygon length and normal-width distortion. Zero-length pieces become
round point strokes; singular jets split until regular or are refused at the
depth cap. This diagnostic is not a certified continuous Taylor remainder.
Patch footprints are tiled through the existing 3×3/5×5 occupancy kernels.
No fixed-rate showcase loop remains.

An independently structured adaptive polyline reference uses the distance of
both inner Bézier controls from the endpoint segment as its stopping criterion.
The convex hull lies within that distance of the chord; endpoint continuity
also bounds the opposite Hausdorff direction. Its stroke-distance error is thus
bounded by the reported reference flatness, apart from floating-point rounding.
Samples within that narrow boundary band may change classification. This is a
geometric distance bound, not an exact continuous-area or floating-point proof.

After every complete candidate render, all 64 sample bits per pixel are compared
to that reference. The report gives worst XOR-bit count, attempts, cumulative
patch count, reference chords and charged work. A failing diagnostic halves the
partition scale, up to eight attempts. Exceeding sample, patch or recursion caps
returns an explicit failure. The reference shares no patch jet implementation;
independent AICI acceptance is still a separate gate.

The memory field is an allowance: two packed planes plus 4096 bytes per allowed
recursion level and fixed overhead. It excludes caller inputs/destination and
arbitrary callback work. Pinned ICK `-fstack-usage` output is retained; measured
partition stack is below this allowance. It is not a measured process RSS peak.

## Native consumer and Conway seam

`path-render baseline|quadratic|cubic|auto INPUT OUTPUT.ppm METRICS.tsv [MASKS.tsv]`
consumes `SUBPIXEL-PATH 1`, `canvas width height AARRGGBB`, ordered
`shape id AARRGGBB opacity screen_width`, explicit `line`/`quad`/`cubic` controls,
and `end`. Pixel centers are integer coordinates. The caller chooses the
geometry-to-pixel transform before serialization. Output dimensions are capped
at 512 per side, with 8192 total segments and 2048 shapes. Each shape is limited
to 100,000 patches and 200 million charged tests; the scene is limited to one
billion. The historical baseline keeps its own bounded subdivision implementation
and reports uninstrumented work honestly. Native input rejects unsupported syntax.

Conway owns parser, groups, orbit enumeration and vector geometry. Its versioned
exporter supplies transformed controls and a documented circular approximation
bound. The native library acquires no Node or group-engine dependency. Individual
shapes retain their draw order; subpaths of one shape share one mask union.

## Toolchain identity

Consumer C uses ICK c61e448251744a2f40ad743ebef1a027bdcd2f9d and Ike
a92fe68bf8aad8dfcfe9f7a7abd84e07290f7fe0. System drivers link objects only.
The source-built hosted installation remains the default. A second installation
of the same ICK source can be qualified from artifact 12174071008 of
https://github.com/isomorphisms/billiard/actions/runs/39225847109.
The archive and driver/cc1 hashes are hard-pinned; an arbitrary installed GCC,
wrapper or compiler-stage override cannot satisfy this route. The package target
spelling is x86_64-pc-linux-gnu. This is compiler-artifact reuse, not a stock
compiler fallback. Existing POSIX qualification recipes remain the narrow Ike v1
compatibility boundary; no new shell implementation was introduced.

Native host checks, hosted checks, registered FP dispatch, independent AICI,
APK packaging and physical devices remain distinct evidence categories.
