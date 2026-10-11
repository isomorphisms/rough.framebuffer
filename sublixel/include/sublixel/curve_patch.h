#ifndef SUBLIXEL_CURVE_PATCH_H
#define SUBLIXEL_CURVE_PATCH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* An independent, C11, pixel-coordinate curve-neighborhood library.
 * Pixel centers are at integer coordinates (as in rough.framebuffer).
 * The 3x3 patch covers offsets [-1,0,1] in row-major (y,x) order.
 * Each pixel is sampled at 8x8 midpoint locations, 64 samples total.
 */
#define CP_GRID3_SIDE 3
#define CP_GRID5_SIDE 5
#define CP_SUBPIXEL_SIDE 8

typedef struct { double x, y; } cp_vec2;

typedef enum {
    CP_OK = 0,
    CP_INVALID = -1,
    CP_SINGULAR = -2,
    CP_OVERFLOW = -3,
    CP_OUTSIDE = -4
} cp_status;

typedef struct {
    cp_vec2 origin;
    cp_vec2 tangent;       /* unit tangent */
    cp_vec2 normal;        /* left unit normal = (-tangent.y,tangent.x) */
    double a2;             /* signed curvature / 2, in inverse pixels */
    double a3;             /* signed cubic coefficient; 3x3 ignores it */
    double half_width;     /* positive, in pixels */
    double valid_u;        /* tangent domain [-valid_u,+valid_u], pixels */
} cp_curve_patch;

typedef struct {
    int center_x, center_y; /* integer pixel center of the 3x3 grid */
    uint64_t occupied[3][3]; /* one bit per midpoint subpixel sample */
} cp_accumulator_3x3;

/* 2-jet of a regular parametric plane curve. The derivatives can use any
 * regular parameter; signed curvature and resulting coverage are invariant
 * under reparameterization, including reversed orientation.
 * On a 3x3 patch, v(u) = a2*u*u and a3 is exactly zero.
 * These constructors change *out only on success.
 */
cp_status cp_patch_from_jet2(cp_curve_patch *out, cp_vec2 origin,
                             cp_vec2 first_derivative, cp_vec2 second_derivative,
                             double half_width, double valid_u);
/* Finite straight segment with flat tangent-domain endpoints. */
cp_status cp_patch_from_segment(cp_curve_patch *out, cp_vec2 start,
                                cp_vec2 end, double half_width);
/* Exact position and first two derivatives of a cubic Bezier at t in [0,1].
 * valid_u controls the local tangent interval, not the Bezier parameter range.
 */
cp_status cp_patch_from_cubic(cp_curve_patch *out,
                              const cp_vec2 controls[4], double t,
                              double half_width, double valid_u);

/* Quadratic local graph point. Values outside valid_u yield CP_OUTSIDE. */
cp_status cp_patch_point(const cp_curve_patch *patch, double u, cp_vec2 *out);
/* v - a2*u*u in the patch's local frame, not exact Euclidean distance. */
cp_status cp_patch_signed_distance(const cp_curve_patch *patch,
                                   cp_vec2 point, double *out);
/* Coverage of ONE pixel: fraction of its 64 midpoint samples inside the
 * finite patch |v-a2*u*u| <= half_width, |u| <= valid_u.
 */
cp_status cp_patch_coverage_pixel(const cp_curve_patch *patch,
                                  int pixel_x, int pixel_y, double *out);

/* Union of covered subpixel samples across multiple patches of ONE shape.
 * OR masks instead of adding grayscale coverages: repeated/split patches
 * cannot artificially darken a stroke. Accumulation is atomic on errors.
 * Use separate accumulators for different colors/compositing operations.
 */
void cp_patch_accumulator_3x3_init(cp_accumulator_3x3 *acc,
                                   int center_x, int center_y);
cp_status cp_patch_accumulate_3x3(cp_accumulator_3x3 *acc,
                                  const cp_curve_patch *patch);
void cp_patch_accumulator_3x3_coverage(const cp_accumulator_3x3 *acc,
                                      double out[3][3]);
/* Convenience for one patch. Does not modify out on failure. */
cp_status cp_patch_cover_3x3(const cp_curve_patch *patch,
                             int center_x, int center_y, double out[3][3]);


/* The 5x5 API uses cubic v(u)=a2*u^2+a3*u^3 and 8x8 midpoint
 * samples per pixel: 25 pixels, 1600 individual sample tests.
 * Existing 3x3 and single-pixel quadratic APIs retain their old semantics.
 */
typedef struct {
    int center_x, center_y;
    uint64_t occupied[CP_GRID5_SIDE][CP_GRID5_SIDE];
} cp_accumulator_5x5;

/* Third-order parametric jet, with derivatives all at the same parameter.
 * For s=|D1|, A=T.D2, B=N.D2, C=N.D3:
 * a2=B/(2*s*s), a3=(C/s^3 - 3*A*B/s^4)/6.
 * Under curve reversal a2 changes sign, a3 does not.
 * Output remains unchanged on failure.
 */
cp_status cp_patch_from_jet3(cp_curve_patch *out, cp_vec2 origin,
                             cp_vec2 first_derivative,
                             cp_vec2 second_derivative,
                             cp_vec2 third_derivative,
                             double half_width, double valid_u);
/* Position and first THREE derivatives of a cubic Bezier at t in [0,1].
 * The legacy cp_patch_from_cubic() intentionally computes only a 2-jet. */
cp_status cp_patch_from_cubic_jet3(cp_curve_patch *out,
                                  const cp_vec2 controls[4], double t,
                                  double half_width, double valid_u);

/* Cubic graph point and local normal-frame residual (NOT nearest distance).
 * Both queries reject u outside the caller-specified tangent domain. */
cp_status cp_patch_point_cubic(const cp_curve_patch *patch, double u,
                               cp_vec2 *out);
cp_status cp_patch_signed_distance_cubic(const cp_curve_patch *patch,
                                         cp_vec2 point, double *out);
cp_status cp_patch_coverage_pixel_cubic(const cp_curve_patch *patch,
                                        int pixel_x, int pixel_y,
                                        double *out);

/* Row-major coverage[y][x]; center pixel is [2][2].
 * Occupancy uses per-sample OR across patches of ONE shape.
 * Both accumulation and the one-shot cover are atomic on errors. */
void cp_patch_accumulator_5x5_init(cp_accumulator_5x5 *acc,
                                   int center_x, int center_y);
cp_status cp_patch_accumulate_5x5(cp_accumulator_5x5 *acc,
                                  const cp_curve_patch *patch);
void cp_patch_accumulator_5x5_coverage(const cp_accumulator_5x5 *acc,
                                      double out[5][5]);
cp_status cp_patch_cover_5x5(const cp_curve_patch *patch,
                             int center_x, int center_y, double out[5][5]);

#ifdef __cplusplus
}
#endif
#endif
