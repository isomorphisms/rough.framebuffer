# Mathematical meaning and finite rendering

[Fulton](https://github.com/walnut-burgundy/fulton) owns the upstream
mathematical interpretation of curves, algebraic intersections, multiplicities
and composition. Its current repository is a reading, source and synthesis
collection. It does not supply an executable renderer or a formal graphics API.

rough.framebuffer owns the finite numerical realization: bounded curve sampling,
64-bit coverage masks, color/depth surfaces, raster operations, numerical
composition and presentation primitives. Applications own mathematical input
selection, interaction and platform behavior. The existing torus/Enneper
producer is retained as an executable source and renderer example; it is not
evidence of a general Fulton implementation.

## Crossing the boundary

An application turns its chosen mathematical object into finite data accepted
by the existing interfaces: local oriented curve jets, piecewise paths,
projected triangles, or the P/A/W/M/L/C/T/D operation stream. The application
must choose coordinates, orientation, domain, scale, precision and approximation
tolerance. These are numerical representations, not the mathematical objects
themselves. No new interchange runtime or parallel renderer is required.

Local curve patches describe a bounded graph strip in tangent/normal
coordinates. Whole paths partition that representation and commit caller-owned
coverage only after their checks pass. Their 8×8 midpoint masks approximate
the selected region; they do not prove exact geometric intersection or recover
algebraic intersection multiplicity.

`cp_coverage_plane_combine` performs Boolean operations on the same individual
sample locations. Union, intersection, difference and symmetric difference are
exact for those stored bits. Independent origins/extents/strides are respected;
samples outside a source domain are empty. Invalid layouts and overlapping
storage fail before any destination word changes. The caller still owns and
must actually allocate the declared capacity.

For one logical shape, combine its sample masks before resolving coverage once.
Separately painted objects retain ordered straight-alpha source-over
composition. Triangle samples retain independent depth tests; translucent
triangles remain order-dependent. None of these pixel operations replaces
Fulton's intersection theory.

## Execution owners

The maintained host product and regression path is pinned ICK through pinned
Ike. Compiler bootstrap and Linux object-only linking are separately declared
exceptions. Android's full NativeActivity closure uses the specific NDK gap
lane recorded in `ci/build-toolchain.tsv`; it is not a stock-host-C product
fallback. Native window lifecycle, events and presentation stay in the Android
adapter. APK construction/signing stays in the maintained android-NDK packager,
with independent AI-CI checks and Cat Food target planning.

Time-indexed mathematical motion and video assembly belong to
[WIGGLE-VIDEO-01](https://github.com/isomorphisms/flexible-pipes/issues/250).
Rendering a frame neither selects a movie's motion plan nor encodes a movie.

## Evidence

[The independent stack verifier](https://github.com/isomorphisms/ai-ci/pull/319)
checks actual geometric, pixel and bit results with address/undefined-behavior
instrumentation and executed source mutations. The independent headless
[six-bad-case verifier](https://github.com/isomorphisms/ai-ci/pull/248) remains
separate. Preserve the wrong-drag, nonperiodic-color, wrong-depth,
single-sample and constant-normal controls.

Host execution, cross-compilation, a signed package, emulator execution and a
physical handset observation are distinct results. Historical C67 observations
remain historical. Current physical A1/C67 image quality, gesture/lifecycle
behavior and timings require receipts tied to the actual installed APK.
