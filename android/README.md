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
