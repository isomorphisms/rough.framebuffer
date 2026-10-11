#include "sublixel/curve_patch.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

/* Keep the 3x3 quadratic API intact.  Its existing frame validator also
 * checks the reserved a3 field, so validate 5x5 inputs through that path. */
static int valid_patch5(const cp_curve_patch *patch) {
    cp_vec2 point;
    return cp_patch_point(patch, 0.0, &point) == CP_OK;
}

static int finite2_5(cp_vec2 point) {
    return isfinite(point.x) && isfinite(point.y);
}

/* For a regular parametric 3-jet, u'=s, u''=A, v''=B, v'''=C
 * in the fixed tangent/normal frame at the base point.
 *
 * d^3 v/du^3 = C/s^3 - 3*A*B/s^4.
 * Use ordered divisions instead of forming s^3 or s^4 directly. */
cp_status cp_patch_from_jet3(cp_curve_patch *out, cp_vec2 origin,
                             cp_vec2 first_derivative,
                             cp_vec2 second_derivative,
                             cp_vec2 third_derivative,
                             double half_width, double valid_u) {
    if (!out || !finite2_5(third_derivative)) return CP_INVALID;

    cp_curve_patch patch;
    cp_status status = cp_patch_from_jet2(
        &patch, origin, first_derivative, second_derivative,
        half_width, valid_u);
    if (status != CP_OK) return status;

    double speed = hypot(first_derivative.x, first_derivative.y);
    double A = patch.tangent.x * second_derivative.x +
               patch.tangent.y * second_derivative.y;
    double B = patch.normal.x * second_derivative.x +
               patch.normal.y * second_derivative.y;
    double C = patch.normal.x * third_derivative.x +
               patch.normal.y * third_derivative.y;
    double along = A / speed;
    double bend = B / speed;
    double jerk = C / speed;
    double coefficient = (((jerk - 3.0 * along * bend) / speed) / speed) / 6.0;
    if (!isfinite(coefficient)) return CP_OVERFLOW;
    patch.a3 = coefficient;
    *out = patch;
    return CP_OK;
}

static cp_vec2 lerp5(cp_vec2 a, cp_vec2 b, double t) {
    return (cp_vec2){a.x * (1.0 - t) + b.x * t,
                     a.y * (1.0 - t) + b.y * t};
}

/* Cubic Bezier position and its first three parametric derivatives.
 * The ordinary cp_patch_from_cubic() remains explicitly quadratic. */
cp_status cp_patch_from_cubic_jet3(cp_curve_patch *out,
                                  const cp_vec2 controls[4], double t,
                                  double half_width, double valid_u) {
    if (!out || !controls || !isfinite(t) || t < 0.0 || t > 1.0 ||
        !isfinite(half_width) || half_width <= 0.0 ||
        !isfinite(valid_u) || valid_u <= 0.0) return CP_INVALID;
    for (int i = 0; i < 4; ++i)
        if (!finite2_5(controls[i])) return CP_INVALID;

    cp_vec2 first[3], second[2];
    for (int i = 0; i < 3; ++i)
        first[i] = lerp5(controls[i], controls[i + 1], t);
    for (int i = 0; i < 2; ++i)
        second[i] = lerp5(first[i], first[i + 1], t);

    cp_vec2 position = lerp5(second[0], second[1], t);
    cp_vec2 velocity = {
        3.0 * (second[1].x - second[0].x),
        3.0 * (second[1].y - second[0].y)
    };
    cp_vec2 acceleration = {
        6.0 * (first[2].x - 2.0 * first[1].x + first[0].x),
        6.0 * (first[2].y - 2.0 * first[1].y + first[0].y)
    };
    cp_vec2 third = {
        6.0 * ((controls[3].x - controls[2].x) -
               2.0 * (controls[2].x - controls[1].x) +
               (controls[1].x - controls[0].x)),
        6.0 * ((controls[3].y - controls[2].y) -
               2.0 * (controls[2].y - controls[1].y) +
               (controls[1].y - controls[0].y))
    };
    if (!finite2_5(position) || !finite2_5(velocity) ||
        !finite2_5(acceleration) || !finite2_5(third)) return CP_OVERFLOW;
    return cp_patch_from_jet3(out, position, velocity, acceleration, third,
                              half_width, valid_u);
}

static cp_status graph5(const cp_curve_patch *patch, double u, double *out) {
    double quadratic = (patch->a2 * u) * u;
    double cubic = ((patch->a3 * u) * u) * u;
    double value = quadratic + cubic;
    if (!isfinite(quadratic) || !isfinite(cubic) || !isfinite(value))
        return CP_OVERFLOW;
    *out = value;
    return CP_OK;
}

cp_status cp_patch_point_cubic(const cp_curve_patch *patch, double u,
                               cp_vec2 *out) {
    if (!valid_patch5(patch) || !out || !isfinite(u)) return CP_INVALID;
    if (fabs(u) > patch->valid_u) return CP_OUTSIDE;
    double v = 0.0;
    cp_status status = graph5(patch, u, &v);
    if (status != CP_OK) return status;
    cp_vec2 point = {
        patch->origin.x + u * patch->tangent.x + v * patch->normal.x,
        patch->origin.y + u * patch->tangent.y + v * patch->normal.y
    };
    if (!finite2_5(point)) return CP_OVERFLOW;
    *out = point;
    return CP_OK;
}

/* The returned residual is v - (a2*u^2+a3*u^3) in a fixed normal frame.
 * It is NOT the closest-point Euclidean signed distance. */
static cp_status evaluate_local5(const cp_curve_patch *patch, cp_vec2 point,
                                 double *residual, int *in_domain) {
    cp_vec2 offset = {
        point.x - patch->origin.x, point.y - patch->origin.y
    };
    if (!finite2_5(offset)) return CP_OVERFLOW;
    double u = offset.x * patch->tangent.x +
               offset.y * patch->tangent.y;
    if (!isfinite(u)) return CP_OVERFLOW;
    *in_domain = fabs(u) <= patch->valid_u;
    if (!*in_domain) return CP_OK;
    double v = offset.x * patch->normal.x +
               offset.y * patch->normal.y;
    double graph = 0.0;
    cp_status status = graph5(patch, u, &graph);
    if (status != CP_OK || !isfinite(v)) return CP_OVERFLOW;
    double value = v - graph;
    if (!isfinite(value)) return CP_OVERFLOW;
    *residual = value;
    return CP_OK;
}

cp_status cp_patch_signed_distance_cubic(const cp_curve_patch *patch,
                                         cp_vec2 point, double *out) {
    if (!valid_patch5(patch) || !finite2_5(point) || !out)
        return CP_INVALID;
    double residual = 0.0;
    int in_domain = 0;
    cp_status status = evaluate_local5(patch, point, &residual, &in_domain);
    if (status != CP_OK) return status;
    if (!in_domain) return CP_OUTSIDE;
    *out = residual;
    return CP_OK;
}

/* One 8x8 midpoint footprint; bits are (sample_y*8+sample_x). */
static cp_status pixel_mask5(const cp_curve_patch *patch,
                             double pixel_x, double pixel_y, uint64_t *out) {
    uint64_t occupancy = UINT64_C(0);
    for (unsigned sy = 0; sy < CP_SUBPIXEL_SIDE; ++sy) {
        for (unsigned sx = 0; sx < CP_SUBPIXEL_SIDE; ++sx) {
            cp_vec2 point = {
                pixel_x + ((double)sx + 0.5) / CP_SUBPIXEL_SIDE - 0.5,
                pixel_y + ((double)sy + 0.5) / CP_SUBPIXEL_SIDE - 0.5
            };
            double residual = 0.0;
            int in_domain = 0;
            cp_status status = evaluate_local5(patch, point,
                                               &residual, &in_domain);
            if (status != CP_OK) return status;
            if (in_domain && fabs(residual) <= patch->half_width)
                occupancy |= UINT64_C(1) << (sy * CP_SUBPIXEL_SIDE + sx);
        }
    }
    *out = occupancy;
    return CP_OK;
}

static unsigned count_bits5(uint64_t mask) {
    unsigned count = 0;
    while (mask != 0) {
        mask &= mask - UINT64_C(1);
        ++count;
    }
    return count;
}

cp_status cp_patch_coverage_pixel_cubic(const cp_curve_patch *patch,
                                        int pixel_x, int pixel_y,
                                        double *out) {
    if (!valid_patch5(patch) || !out) return CP_INVALID;
    uint64_t mask = 0;
    cp_status status = pixel_mask5(patch, (double)pixel_x,
                                  (double)pixel_y, &mask);
    if (status != CP_OK) return status;
    *out = (double)count_bits5(mask) / 64.0;
    return CP_OK;
}

void cp_patch_accumulator_5x5_init(cp_accumulator_5x5 *acc,
                                   int center_x, int center_y) {
    if (!acc) return;
    memset(acc, 0, sizeof(*acc));
    acc->center_x = center_x;
    acc->center_y = center_y;
}

cp_status cp_patch_accumulate_5x5(cp_accumulator_5x5 *acc,
                                  const cp_curve_patch *patch) {
    if (!acc || !valid_patch5(patch)) return CP_INVALID;
    uint64_t incoming[CP_GRID5_SIDE][CP_GRID5_SIDE] = {{0}};
    for (int y = 0; y < CP_GRID5_SIDE; ++y)
        for (int x = 0; x < CP_GRID5_SIDE; ++x) {
            cp_status status = pixel_mask5(patch,
                (double)acc->center_x + (double)(x - 2),
                (double)acc->center_y + (double)(y - 2),
                &incoming[y][x]);
            if (status != CP_OK) return status;
        }
    /* Commit only once every pixel succeeds. Bitwise OR is idempotent,
     * commutative and associative for patches of the SAME shape. */
    for (int y = 0; y < CP_GRID5_SIDE; ++y)
        for (int x = 0; x < CP_GRID5_SIDE; ++x)
            acc->occupied[y][x] |= incoming[y][x];
    return CP_OK;
}

void cp_patch_accumulator_5x5_coverage(const cp_accumulator_5x5 *acc,
                                      double out[5][5]) {
    if (!acc || !out) return;
    for (int y = 0; y < CP_GRID5_SIDE; ++y)
        for (int x = 0; x < CP_GRID5_SIDE; ++x)
            out[y][x] = (double)count_bits5(acc->occupied[y][x]) / 64.0;
}

cp_status cp_patch_cover_5x5(const cp_curve_patch *patch,
                             int center_x, int center_y, double out[5][5]) {
    if (!out || !valid_patch5(patch)) return CP_INVALID;
    cp_accumulator_5x5 acc;
    cp_patch_accumulator_5x5_init(&acc, center_x, center_y);
    cp_status status = cp_patch_accumulate_5x5(&acc, patch);
    if (status != CP_OK) return status;
    cp_patch_accumulator_5x5_coverage(&acc, out);
    return CP_OK;
}
