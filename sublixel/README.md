# Sublixel: native 3×3 local curvature patches

An independent C11 curve-neighborhood library under the rough.framebuffer workspace. This slice calculates local geometry and grayscale coverage without invoking the framebuffer, Field Mouse, Android or graphics hardware. It does not yet improve C67 triangle edges.

## Geometry

At a regular parametric curve point P, let T be the unit tangent and N=(-T.y,T.x) its left normal. With first/second derivatives D1,D2, the signed curvature is cross(D1,D2)/|D1|^3. The quadratic 3×3 model has coefficient a2=curvature/2 and local graph:

    world(u) = P + T*u + N*(a2*u*u)
    |u| <= valid_u
    residual(Q) = dot(Q-P,N) - a2*dot(Q-P,T)^2

The residual is a *local normal-frame approximation*, not an exact Euclidean signed distance or nearest-point solver. Both valid_u and half_width are measured in pixel units. a3 is reserved for future 5×5 asymmetric models and ignored by 3×3. The constructor from a 2-jet requires a nonzero tangent; zero-speed cusps, invalid fields, malformed frames, and arithmetic overflow fail closed. The local quadratic should not be treated as a whole-curve approximation beyond the caller-declared tangent interval.

## Pixel footprints and 3×3

Pixel centers lie at integer coordinates as in rough.framebuffer. A 3×3 block uses offsets (-1..+1,-1..+1), returned as coverage[y][x], with the middle element [1][1] corresponding to the requested center. Each pixel's unit-square footprint has 8×8 midpoint samples. Coverage is the fraction of those 64 samples satisfying:

    abs(u) <= valid_u  AND  abs(v - a2*u*u) <= half_width

cp_patch_coverage_pixel computes a single grayscale fraction; cp_patch_cover_3x3 returns nine fractions. To combine multiple local patches of one shape, initialize cp_accumulator_3x3, call cp_patch_accumulate_3x3 repeatedly, and resolve with cp_patch_accumulator_3x3_coverage. Accumulation ORs 64-bit sample masks, so repeating a patch does not brighten a stroke. The order of contributions does not matter. Use separate accumulators for different colors; compositing them is outside this library. A straight segment uses flat tangent-domain endpoints; splitting it into collinear segments preserves sample coverage.

These are sampled footprint integrals, not exact analytic area, opaque 3×3 stamps, or a generic convolution kernel. Regular parameter changes including reversal preserve the geometric footprint; reverse orientation flips the sign of a2. Cubic Bézier inputs are evaluated exactly for P,D1,D2 at a chosen parameter, then approximated quadratically over valid_u.

## Build and verify

Header: include/sublixel/curve_patch.h. Implementation: src/curve_patch.c. No dynamic allocations; C11 plus libm.

The default build is a checked Ike `Ikefile`, not GNU Make. A trusted Ike and source-built ICK are required; see [qualification](qualification/README.md).
Run the repository-root `sh scripts/run-ike.sh test` with the documented `IKE_*` and `ICK_*` environment bindings. Absence or mismatch blocks the build.

Tests cover exact 0, 1/2 and 1 coverage, independent 8×8 counts for a known parabola, quadratic and circle curvature, Bézier derivative evaluation, regular nonlinear reparameterization/reversal, grid ordering, duplicate/split patch union, invalid inputs, overflow and failure without partial output mutation.

## Deliberately absent

5×5 cubic patches, exact nearest-point distance, analytic area integration, implicit/spline adapters, direct framebuffer/color blending, per-triangle sample depth/coverage, shader code, Android integration and physical-device evidence. The C67 jagged-triangle rendering defect remains a separate task.
