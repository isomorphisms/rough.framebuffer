# NativeActivity source retained from the original Android art draft

## Gestures and orientation

One-finger horizontal and vertical swipes compose screen-axis rotations into
a normalized scalar-first quaternion. The gesture state stores this quaternion
and its cancellation anchor, with no Euler-angle state. Geometry and analytic
normals share a matrix derived from that orientation once per frame. The old
angle-based headless stream remains a compatibility interface.

Pinch alone changes camera distance. Spreading zooms in; bringing fingers
together zooms out. A pinch never switches shapes. Taps still toggle shapes.

## Reproducible current-head test packaging

`build-ndk.grease` replaces the retired Android Makefile. It uses only the
explicit NDK exception already recorded in `ci/build-toolchain.tsv`, compiles
the complete native closure including the retained Subpixel diagnostic,
and checks 16 KiB ELF load alignment for ARM32, ARM64 and x86-64.
The platform-neutral host source and mutation tests remain under ICK/Ike.
The packager receives stripped native libraries. Unstripped debugging copies
remain separate, with before/after byte counts and an exported-entrypoint
check after stripping. Both build and packaging scripts locate their own
source directory instead of relying on the caller's current directory.

`package-test.grease` uses the maintained android-NDK packager, preserves
`org.isomorphisms.roughframebuffer` and the central public-test certificate,
and uses versionCode 4 for the quaternion-rotation update. The digest-bound historical artifact
is used solely for update identity comparison, not source qualification.
Every finished single-ABI package goes through Flexible Pipes'
`android-apk-preflight` and its accepted independent AI-CI producer gate.
This preflight is package acceptance, not a registered end-to-end FP Android
producer dispatch or a physical-device receipt.

The current-head Android workflow exercises actual Android 14 launch,
drag, shape switch, zoom and restart against the existing independent
pixel oracle, including an unchanged-image rejection control. Historical
workflow results remain historical; a new workflow must actually pass.

Set `ROUGH_COMPARISON_DIR` to an existing output directory while running
the ICK/Ike host test target to retain deterministic torus/Enneper PPMs.
Each image shows the preserved flat single-sample render on the left and
current AA2/smooth shading on the right, at the same size, yaw and distance.
This compares production raster functions; it does not measure C67 speed.

This directory preserves the existing C NativeActivity consumer and its
manifest, with the stable historical package identity. The renderer remains
the common rough.framebuffer library; the Android layer owns window,
gesture event delivery, lifecycle, stride and presentation only.

## Current qualification state

- The platform-neutral `touch.c`, `present.c`, direct `surface.c` and
  actual pixel- / depth-effect tests are compiled by the maintained
  source-built ICK and pinned Ike **host** route.
- The Android `native/rough_android.c` shell requires NDK-specific Android
  headers, NativeActivity native app glue, ABI sysroot and canonical APK
  packaging. The owned ICK A1/C67 full Android code path is still a
  documented compiler/ABI gap, not an invitation to silently use host GCC.
- Historically accepted source/test APKs and emulator screenshot receipts
  remain linked in the original Android PR; they are not fresh test results
  for the reconciled current branch.
- Do not run the obsolete `android/Makefile` (intentionally not carried
  into this branch). The maintained android-NDK/Kitchen/FP/Cat Food path
  must replace that packaging route with explicit compiler exception and
  independent AI-CI ABI/signing/device evidence.
- No current A1 or C67 physical acceptance, publication, or release-signer
  claim. The front-hemisphere drag mutant is checked at the native source
  level, not treated as an emulator-only pixel-change test.

Upstream mathematical curves/intersections remain with
[walnut-burgundy/fulton](https://github.com/walnut-burgundy/fulton);
this Android artifact merely paints and interacts with computed frames.

## C67 2×2 depth-aware software sampling source

The preserved C67/x86-64 code path uses four independently depth-tested
color samples per logical framebuffer pixel and premultiplied-alpha resolve.
ARM32 remains the original single-sample route. This is a source/API
qualification, not a measured frame rate, physical pixel AA guarantee or
release artifact; the actual shell still requires separately qualified NDK
packaging. The ICK host tests check fractional edges, overlapping depths,
transparent fringe, invalid/padded input and a deliberately bad single-sample
implementation. Smooth normal shading remains a separate downstream draft.

## Curvature visual diagnostic

The renamed Subpixel native overlay displays an actual tilted quadratic
versus cubic sampled mask; the historical spelling is not a supported API.
Its C source, bitmap output and integer-rational sample-count oracle are
integrated with the existing owned ICK host code. This overlay is **not**
the 3D surface silhouette antialiaser; C67 package/device output remains
unverified on the current merged branch.

## Analytic-normal shading source

The C67/x86-64 2×2 sampling path now uses real perspective-correct
analytic-normal shading; the A1 path and P/T stream retain historical
flat shading. This is *source-only* until current Android ABI/signer/APK
and A1/C67 physical qualification is repeated; no old emulator screenshot
constitutes current-head acceptance.
