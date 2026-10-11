# Sublixel real 2D curvature preview

This is the first **native consumer** of the independently implemented Sublixel neighborhood APIs, not a separate rasterizer substituting the geometry model.

It draws the same cubic Bézier stroke with the quadratic **3×3** and cubic **5×5** local-neighborhood approximations. Every curve sample comes from the actual public `cp_patch_from_cubic` and `cp_patch_from_cubic_jet3` constructors. Each neighborhood computes its **own subpixel occupancy bits**, and overlapping local patches are merged by bitwise OR in caller-owned pixel buffers, before being converted to grayscale. This prevents duplicate local patches from artificially darkening the stroke.

The two 96×72 panels are written side by side into a **192×72 P6 PPM** at `sublixel/build/curve-3x3-vs-5x5.ppm`. The implementation has fixed memory and never allocates an image-sized heap buffer. The named points vary in both x and y: it is a visibly **slanted, curved** example, not a horizontal bar or a pre-stamped mask. This demonstrates the subpixel curve coverage contract, not a universal smoothness guarantee.

Run from the owning repository with verified environment bindings for both Ike (`IKE_BIN`, `IKE_SOURCE_DIR`, `IKE_EXPECTED_SHA256`, a fresh `IKE_RECEIPT`) and ICK (`ICK_CC`, `ICK_SOURCE_DIR`, `HOST_LINKER`):

```sh
sh scripts/run-ike.sh preview
```

The literal `Ikefile` first runs the native ICK build/test target, then builds this exact example with ICK and links only prebuilt ICK objects using the declared system CRT/libm linker. GNU Make, GCC or Clang cannot substitute for the ICK/Ike consumer build. `--self-test` exercises real slant extents, multiple partially covered pixels, pixel bytes/PPM header and dimensions, and idempotent per-subpixel union across duplicate curve patches. The preview emits hashes of the occupied subpixel planes and final PPM.

**Limits:** This is a CPU/host preview, **not** an APK, not a physical A1/C67 run, and it does not fix the separate rough.framebuffer **triangle silhouette and flat-shaded surface** defect in [#5](https://github.com/isomorphisms/rough.framebuffer/issues/5). It is not full integration into the existing P/M/L/C streaming renderer, nor a novel global distance-to-curve guarantee. It preserves and reuses the 3×3 and 5×5 mathematical libraries and their independent acceptance boundaries.

Tracked by [native consumer issue #12](https://github.com/isomorphisms/rough.framebuffer/issues/12) and the existing unmerged source stack [PR #6](https://github.com/isomorphisms/rough.framebuffer/pull/6) → [PR #8](https://github.com/isomorphisms/rough.framebuffer/pull/8) → [PR #10](https://github.com/isomorphisms/rough.framebuffer/pull/10). This candidate requires its own hosted ICK/Ike receipt and independent acceptance; an issue or preview output alone does not authorize merging.
