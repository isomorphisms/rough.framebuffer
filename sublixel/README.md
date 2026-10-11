# Sublixel: local 3×3 and 5×5 curvature neighborhoods

An independent C11 curve-neighborhood library underneath the
`rough.framebuffer` workspace. It computes local curve geometry and
fractional grayscale pixel coverage with no framebuffer, Android, Field Mouse,
GPU, heap allocations or color-compositing dependency.

This **stacked candidate** extends the quadratic 3×3 work in PR #6. It does
not yet make the C67 renderer use Sublixel or fix jagged triangles.

## Geometry

At regular curve point P, choose unit tangent T and left normal
N=(-T.y,T.x). In that fixed local orthonormal frame, set
u=dot(Q-P,T), v=dot(Q-P,N). The graph used to approximate a curve is

    3×3: v(u) = a2*u*u
    5×5: v(u) = a2*u*u + a3*u*u*u
    -valid_u <= u <= valid_u

For parametric derivatives D1, D2, D3 at the base point, define
s=|D1|, A=T·D2, B=N·D2, C=N·D3. The coefficients are

    a2 = B / (2*s*s) = signed_curvature / 2
    a3 = (C/s^3 - 3*A*B/s^4) / 6

They represent a local graph expansion, *not* a whole cubic Bézier segment.
The caller chooses `valid_u` in pixel units to bound approximation error.
`half_width` is also in pixel units. The curve band uses the residual
`v - (a2*u²+a3*u³)`; this is **not exact Euclidean signed distance** and its
constant-residual band is not exactly a fixed-distance stroke at high slope.

`cp_patch_from_jet3` accepts a regular parametric 3-jet.
`cp_patch_from_cubic_jet3` evaluates a cubic Bézier and all three
derivatives at a chosen parameter value. The existing
`cp_patch_from_jet2` / `cp_patch_from_cubic` constructors remain quadratic.
Nonzero speed is required. Reparameterization (including nonlinear and
reverse orientation) preserves geometric coverage: reversing orientation
negates `a2` but leaves `a3` unchanged.

The existing quadratic calls `cp_patch_point`,
`cp_patch_signed_distance` and `cp_patch_coverage_pixel` still ignore
`a3`. The explicit `_cubic` versions evaluate both coefficients.

## Pixel neighborhoods

Pixel centers are at integer coordinates. Pixel (x,y) has a unit-square
footprint sampled at 8×8 midpoint locations. One 64-bit mask stores the
occupied subsamples in row-major order.

| Model | Pixel offsets | Coverage array center | Subsamples/patch |
| --- | --- | --- | --- |
| 3×3 quadratic | -1 to +1 | `[1][1]` | 9×64 = 576 |
| 5×5 cubic | -2 to +2 | `[2][2]` | 25×64 = 1,600 |

`cp_patch_cover_3x3` and `cp_patch_cover_5x5` return fractional coverage
in row-major `coverage[y][x]`, with values from 0 to 1 inclusive. Individual
pixels can be queried with `cp_patch_coverage_pixel` (quadratic) or
`cp_patch_coverage_pixel_cubic` (cubic). These are sampled footprint
integrals, not exact analytic areas, convolution kernels or pre-stamped pixel
patterns.

For one stroke composed of multiple local patches, initialize
`cp_accumulator_3x3` or `cp_accumulator_5x5`, repeatedly accumulate each
patch, and retrieve fractional coverage. The occupancy operation is **OR of
subsample masks**, never addition of alpha. Repeating a patch, changing patch
order, and splitting a collinear straight segment cannot increase its
coverage. Use a separate accumulator per shape/color; blending distinct
shapes is out of scope. Input validation, arithmetic overflow and mutation
of outputs/accumulators on failure are handled transactionally.

## Build and verification: pinned Ike orchestrating pinned ICK

The 3×3 and 5×5 libraries are compiled and tested with **source-built ICK**, orchestrated by pinned **Ike**, through the same `Ikefile`. The root and Sublixel GNU Makefiles deliberately refuse product builds; the old reference-only GCC/Clang jobs are historical diagnostics, not a selectable build.

From the repository root, with checked `ICK_CC`, `ICK_SOURCE_DIR`, `HOST_LINKER`, `IKE_BIN`, `IKE_SOURCE_DIR`, `IKE_EXPECTED_SHA256`, and a fresh `IKE_RECEIPT`:

    sh scripts/run-ike.sh test

The execution compiles both curve implementations, both independent test suites, and framebuffer renderer/CLI/smoke using ICK; runs all three native test executables; checks x86_64 ELF object identities; and emits `ike-build-v1` and artifact hashes. See [the exact qualification contract](qualification/README.md). Ike itself is built by ICK in the hosted workflow.

Tests cover the 3×3 quadratic and 5×5 cubic coefficient/coverage oracles, reparameterization, reversal, accumulation, malformed inputs and transactional error behavior. They do not prove C67/A1 Android source compilation, production rendering integration or device performance.

## Not claimed

Exact closest-point distance; higher-order global curve fitting; automatic
selection of valid tangent intervals or stroke joins/caps; implicit/spline
adapters; analytic coverage integration; arbitrary-color composition; direct
Android/GPU/Field Mouse integration; antialiased 3D triangle edges; physical
C67 validation. Existing renderer PRs remain separate.

This is a **draft, unmerged, stacked extension** to
[3×3 Sublixel PR #6](https://github.com/isomorphisms/rough.framebuffer/pull/6),
tracked by [Flexible Pipes work envelope #77](https://github.com/isomorphisms/flexible-pipes/issues/77).
That work envelope is not a registered dispatch or ai-ci approval.
