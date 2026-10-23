# Continuous instrument typography

`subpixel/font.h` exposes `sf57_render_coverage`: a Unicode scalar, integer
scale, and fractional pixel phase produce a 5×7-scaled 8-bit coverage mask.
The existing 8×8 whole-path producer owns every coverage decision. Strokes are
unioned as sample masks before the single coverage resolve. Color blending
belongs to the window adapter. This is grayscale coverage, not RGB stripe
rendering dependent on a particular display's subpixel arrangement.

The existing O/C/S/8 masters and their bitmap controls remain intact. The
instrument alphabet adds digits, uppercase, lowercase, slash, backslash,
plus, minus, decimal point, colon, comma, equals, question mark, ×, −, °, ²,
³, μ and µ. All masters are directly authored straight/Bezier geometry;
bitmap rows never determine control points. Geometry-only masters deliberately
have no bitmap control, and `sf57_bitmap_cell` returns −1 for those masters.

The API refuses unknown scalars, scale outside 1–20, nonfinite/out-of-range
phase, insufficient output capacity and allocation/rasterization failure.
Refusal preserves output. A space returns transparent coverage. Consumers
cache by scalar/scale/phase; color is not part of the mask key. Do not silently
restore a bitmap renderer after a refusal. Current Android consumers use
integer positions and phase (0,0); Unicode byte decoding is a separate text
boundary, not performed by this scalar API.

## Evidence and limits

`test_font_render.c` checks the complete alphabet, deterministic repeated
rendering, symbol distinctions, transparent spaces, atomic refusal and an
independent Euclidean slash oracle at five scales and four fractional phases.
The retained alphabet specimen comes from the real whole-path producer.
The pre-existing O/C/S/8 preview still supplies bitmap/legacy/3×3/5×5/auto
comparisons. Existing ai-ci `font_verify.c` independently checks those masters,
76 render cases and 463 assertions. It does not independently qualify all the
new alphabet's letter shapes. New contour and visual acceptance remains work.

Initial host tests used the source-built ICK installation from source
`fbe86e23d55cfec2000c08e61deea2a407fd7175` on Linux x86_64. This is separate
from the repository's c61e448 ICK/Ike required gate. No Android, physical-device,
APK-size or device-performance acceptance follows from the host results.

## Consumer inventory at start of SUN SUBPIXEL-FONT-ANDROID-01

| Application | Recovered source | Required remaining acceptance |
| --- | --- | --- |
| Accelerometer | utilities main and existing slash branch | shared font build, APK/update, A1/C67, responsive updates |
| Proximity | standalone repository main | ICK source producer, shared font build, APK/update, A1/C67 |
| Spirit Level | utilities `spirit-level/minimal-retained-slice` | reconcile existing implementation, shared font, build/delivery |
| Gyroscope | utilities has only `.gitkeep` scaffolding | actual instrument implementation and shared font build/delivery |
| Magnetometer | utilities has only `.gitkeep` scaffolding | actual instrument implementation and shared font build/delivery |
| Ambient Light | repository has no refs | actual source recovery/implementation and shared font build/delivery |
| Other consumers | Unicode Picker has a separate picker renderer | inspect semantics before extending this instrument font's scope |

This inventory is not a completion claim. Every named consumer must have a
concrete source, built artifact and separate acceptance result before closing
the cross-application SUN.
