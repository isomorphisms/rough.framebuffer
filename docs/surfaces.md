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

## Gesture-direction regression: 2026-10-22

Two direction corrections were needed during C67 testing. The repeated mistake
was checking an internal coordinate convention instead of the movement of the
visible surface under the user's finger. Changing frame hashes proves movement,
but cannot establish that its direction is correct.

The v0.4 vertical defect survived a supposedly independent quaternion-sandwich
test because that test omitted the renderer's inversion of the vertical axis.
Camera y increases upward; Android touch y and framebuffer row numbers increase
downward. Actual projection uses screen y = center y − focal × camera y ÷ camera z.
The test incorrectly compared positive camera y with positive touch y, so code
and oracle agreed on the same wrong assumption. After the earlier horizontal
direction defect, both axes should have been checked through screen projection.

The v0.5 correction reverses the vertical quaternion delta and makes the oracle
apply the actual screen-y sign. Horizontal rotation and pinch zoom are retained;
orientation remains a normalized quaternion. The corrected oracle passed with
the fix and rejected genuinely compiled prior-sign source. The user then
confirmed the corrected v0.5 behavior on the physical C67. This is user-reported
gesture acceptance, not a measured frame-rate or device-wide performance claim.

Future interaction changes must preserve these checks:

- Follow actual near and far surface landmarks through orientation, perspective
  division and screen projection. The visible near surface must follow the finger.
- Exercise up, down, left and right after existing rotations, not only at the
  identity pose. Keep screen axes distinct from model and camera axes.
- Verify that one-finger rotation leaves camera distance unchanged and pinch
  changes distance without rotating.
- Require a direction oracle to reject the known reversed-sign behavior; a
  changed image or a quaternion algebra identity alone is insufficient.

The regression tests live in [test_rotation_grab.c](../tests/test_rotation_grab.c)
and [test_touch.c](../tests/test_touch.c). This contract belongs at the geometry,
projection and interaction boundary in this repository. NDK packaging cannot
establish gesture direction.
