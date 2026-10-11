# MIRO A1 NativeActivity framebuffer candidate

This application-specific adapter displays the **real existing C framebuffer**
on Android, not an EGL, Canvas, WebView or Processing.js replacement.

**Controls:** one-finger horizontal drag rotates the surface by 180 degrees
per full-screen width; vertical drag moves the camera between distances 2.4
and 12.0; a tap switches **torus ↔ Enneper**. The first finger owns the
gesture. Pointer cancellation restores the prior view, and window resizing
invalidates stale touch coordinates. A source/renderer pixel-and-depth parity
test ensures the direct path is the same mathematics as the original
`P/T` operation stream.

## C67 2×2 software coverage antialiasing

On the MIRO C67 `arm64-v8a` build, and in the x86-64 Android emulator, the
native rasterizer now evaluates **four independently depth-tested color samples
per bounded software framebuffer pixel**. The same triangle producer runs
at twice the width and height; `rough_resolve_2x2()` resolves those samples
in premultiplied-alpha space and uses their nearest depth, then the existing
Android RGBA8888 presenter maps the image onto the display.

This is **real subpixel edge coverage at the software framebuffer resolution**,
not a blur of an already aliased image. With the current 180,000-pixel base
budget, the high-resolution scratch has up to 720,000 pixels (color and
depth together about 5.76 MiB), plus about 1.44 MiB for the base buffers.
It is reused across touch redraws. The older Android ARM32 A1 build preserves
the one-sample renderer for responsiveness and memory.

It is not full-resolution multisampling of every 720×1600 display pixel.
Edges under extreme zoom may still expose the software resolution. More
importantly, this does **not** yet interpolate analytic normals or lighting
across triangles: colored polygon facets remain part of
[the open visual-quality issue #5](https://github.com/isomorphisms/rough.framebuffer/issues/5).
A physical C67 speed and quality comparison of this updated APK is still needed.

## Native lifecycle and pixels

The NDK native-app-glue callback receives Android NativeActivity window
and input events. The Android application uses `ANativeWindow_lock`,
`WINDOW_FORMAT_RGBA_8888`, the returned stride, and explicit conversion
from the renderer's straight-alpha `0xAARRGGBB` pixels to RGBA bytes.
Rendering occurs only when dirty, not on a CPU busy loop. The scratch
framebuffer is capped at 180,000 pixels, and scaled to the actual window
dimensions. On a reported 576×1152 MIRO screen this normally implies an
approximately 288×576 software image, not native-resolution GPU shading.
This is a deliberate first-step CPU performance compromise.

A successful redraw logs `RoughFrameArt FRAME` with an incrementing input
sequence, the actual surface and view parameters, painted-pixel count, and
hash of the offscreen color pixels. **A log is not a screenshot**; the
event-to-image claim must also be checked with real Android screencaps
taken before/after an ADB swipe, preferably followed by physical phone tests.

## Build

The application uses the generic
[android-NDK NativeActivity route](https://github.com/isomorphisms/android-NDK/blob/42f04d7654d54309ec3f8787bfd2bc40730399d7/ARCHITECTURE.md).
The reviewed Android target is `armeabi-v7a`, API 21, ARM A32
via `armv7a-linux-androideabi21-clang` from NDK
`29.0.14206865`. A Linux x86_64 host with that SDK/NDK can run:

```sh
make -C android NDK_ROOT="/path/to/ndk/29.0.14206865" ANDROID_API=21
```

The output is `build/android/armeabi-v7a/libroughart.so`. The exact
[hosted crosscompile workflow](../.github/workflows/android-native.yml)
probes tool availability, compiler target, ELF32/EM_ARM, exported lifecycle
symbols, linked Android libraries, and preserves a candidate artifact.
The host-only `make test` also checks the touch reducer and real rendered
pixel changes. Neither builds an APK by themselves.

At ICK revision `c61e448251744a2f40ad743ebef1a027bdcd2f9d`, the
Android ARM32 scalar BetterC work has not qualified the complete native
C/`NativeActivity`/libc integration needed for this application. This
specific reason for choosing NDK is recorded in
[`ci/build-toolchain.tsv`](../ci/build-toolchain.tsv), not hidden by
substituting host GCC.

## APK, signer and acceptance boundary

The manifest's proposed application package name is
`org.isomorphisms.roughframebuffer`, its `android.app.NativeActivity`
loads `libroughart.so`, and application DEX is absent. This name is
**proposed**, not an already approved installer/update identity.

Once a package identity and stable test signer are explicitly registered,
use the **maintained** generic
[direct NativeActivity packager](https://github.com/isomorphisms/android-NDK/blob/42f04d7654d54309ec3f8787bfd2bc40730399d7/apk/build-nativeactivity-apk.sh)
with the application manifest, the crosscompiled `.so`, `armeabi-v7a`,
and the declared SDK/package/version/signing environment. The packager
requires `ANDROID_KEYSTORE`, `ANDROID_EXPECTED_CERT_SHA256`, the
keystore credentials and approved update identity; it will not generate
a key, silently select another signer or fall back to Gradle.

An ELF32 ARM shared library is **not an installable APK**. A signed APK
is **not proof of a launched surface**. Launching it is **not proof
of the actual image changing after a drag**. A screenshot/emulator test
is **not a physical MIRO A1 test**.

This is a draft stacked follower on
[rough.framebuffer PR #3](https://github.com/isomorphisms/rough.framebuffer/pull/3),
from [Flexible Pipes job envelope #74](https://github.com/isomorphisms/flexible-pipes/issues/74).
No merge, release or device acceptance is implied.
