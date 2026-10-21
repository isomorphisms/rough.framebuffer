# Headless mathematical surface source

The original native torus/Enneper source from the older rotating-surface draft
is retained without importing its obsolete GNU Make or direct GCC/Clang
build. Its `rough_emit_surface` creates the established `P` and `T`
geometry stream, which `rough-fb` consumes with the same depth and color
semantics as other clients. It does not implement a second rasterizer.

## Existing ICK/Ike build and examples

From an environment with the repository's qualified pinned ICK and Ike:

```sh
sh scripts/run-ike.sh test
./build/rough-surface torus 40 160 160 > build/torus.ops
./build/rough-fb build/torus.ops build/torus.ppm 160 160
./build/rough-surface enneper 25 160 160 > build/enneper.ops
./build/rough-fb build/enneper.ops build/enneper.ppm 160 160
```

The checked test recipe compiles all native producer/consumer C with ICK,
links only object files with the declared system linker, runs the existing
geometry/projection/near-clip/depth/color tests, and rejects an intentionally
nonperiodic torus material palette. Both PPMs come from computed geometry,
not prepackaged images. No new independent Android/physical acceptance is
inferred from this host integration.

## Mathematical contract and ownership

The torus uses periodic parameters and radial cross-section
`R = 1.18 + 0.43 cos(v)`, with position
`(R cos(u), R sin(u), 0.43 sin(v))`. Enneper is sampled over a square patch
using its polynomial minimal-surface parametrization. Both are transformed
by camera yaw/pitch and clipped at camera `z = 0.5` before projection.
Perspective depth is `1 - 0.5/z`, increasing with distance and compatible
with the existing framebuffer test.

Current material lighting is intentionally flat, double-sided Lambert plus
ambient; analytic smooth normals are a distinct draft. Torus colors use
integer angular harmonics to avoid discontinuity at parameter seams.

Mathematical curve/intersection and composition semantics belong upstream
with [Fulton](https://github.com/walnut-burgundy/fulton); this C program is
a bounded source-stage fixture and reference visualizer. It does not replace
Fulton, Field Mouse or the application-specific geometry owner.

Historic two-surface frame/video artifacts and their independent test
evidence remain attached to
[the original draft](https://github.com/isomorphisms/rough.framebuffer/pull/3).
The shared movie/capture subsystem remains WIGGLE-VIDEO-01 rather than
copying a second FFmpeg pipeline into this repository.
