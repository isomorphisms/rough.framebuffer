# NativeActivity source retained from the original Android art draft

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
