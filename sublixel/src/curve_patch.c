#include "sublixel/curve_patch.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

static int finite2(cp_vec2 point) {
    return isfinite(point.x) && isfinite(point.y);
}

static int valid_patch(const cp_curve_patch *patch) {
    if (!patch || !finite2(patch->origin) ||
        !finite2(patch->tangent) || !finite2(patch->normal) ||
        !isfinite(patch->a2) || !isfinite(patch->a3) ||
        !isfinite(patch->half_width) || !isfinite(patch->valid_u) ||
        patch->half_width <= 0.0 || patch->valid_u <= 0.0) return 0;
    double tangent_norm = hypot(patch->tangent.x, patch->tangent.y);
    double normal_norm = hypot(patch->normal.x, patch->normal.y);
    double dot = patch->tangent.x * patch->normal.x +
                 patch->tangent.y * patch->normal.y;
    double determinant = patch->tangent.x * patch->normal.y -
                         patch->tangent.y * patch->normal.x;
    return fabs(tangent_norm - 1.0) <= 1e-12 &&
           fabs(normal_norm - 1.0) <= 1e-12 &&
           fabs(dot) <= 1e-12 && fabs(determinant - 1.0) <= 1e-12;
}

cp_status cp_patch_from_jet2(cp_curve_patch *out, cp_vec2 origin,
                             cp_vec2 first_derivative, cp_vec2 second_derivative,
                             double half_width, double valid_u) {
    if (!out || !finite2(origin) || !finite2(first_derivative) ||
        !finite2(second_derivative) || !isfinite(half_width) ||
        !isfinite(valid_u) || half_width <= 0.0 || valid_u <= 0.0)
        return CP_INVALID;
    double speed = hypot(first_derivative.x, first_derivative.y);
    if (speed == 0.0) return CP_SINGULAR;
    if (!isfinite(speed)) return CP_OVERFLOW;
    cp_curve_patch patch = {0};
    patch.origin = origin;
    patch.tangent = (cp_vec2){first_derivative.x / speed, first_derivative.y / speed};
    patch.normal = (cp_vec2){-patch.tangent.y, patch.tangent.x};
    /* First divide acceleration by speed to avoid squaring a large speed. */
    double projected = patch.normal.x * (second_derivative.x / speed) +
                       patch.normal.y * (second_derivative.y / speed);
    patch.a2 = 0.5 * (projected / speed);
    patch.a3 = 0.0;
    patch.half_width = half_width;
    patch.valid_u = valid_u;
    if (!valid_patch(&patch)) return CP_OVERFLOW;
    *out = patch;
    return CP_OK;
}

cp_status cp_patch_from_segment(cp_curve_patch *out, cp_vec2 start,
                                cp_vec2 end, double half_width) {
    if (!out || !finite2(start) || !finite2(end) ||
        !isfinite(half_width) || half_width <= 0.0) return CP_INVALID;
    cp_vec2 delta = {end.x - start.x, end.y - start.y};
    if (!finite2(delta)) return CP_OVERFLOW;
    double length = hypot(delta.x, delta.y);
    if (length == 0.0) return CP_SINGULAR;
    if (!isfinite(length)) return CP_OVERFLOW;
    cp_vec2 middle = {0.5 * start.x + 0.5 * end.x,
                      0.5 * start.y + 0.5 * end.y};
    return cp_patch_from_jet2(out, middle, delta,
                              (cp_vec2){0.0, 0.0}, half_width, 0.5 * length);
}

static cp_vec2 lerp(cp_vec2 start, cp_vec2 end, double t) {
    cp_vec2 result = {start.x * (1.0 - t) + end.x * t,
                      start.y * (1.0 - t) + end.y * t};
    return result;
}

cp_status cp_patch_from_cubic(cp_curve_patch *out,
                              const cp_vec2 controls[4], double t,
                              double half_width, double valid_u) {
    if (!out || !controls || !isfinite(t) || t < 0.0 || t > 1.0 ||
        !isfinite(half_width) || half_width <= 0.0 ||
        !isfinite(valid_u) || valid_u <= 0.0) return CP_INVALID;
    for (int i = 0; i < 4; ++i)
        if (!finite2(controls[i])) return CP_INVALID;

    cp_vec2 first[3], second[2];
    for (int i = 0; i < 3; ++i) first[i] = lerp(controls[i], controls[i + 1], t);
    for (int i = 0; i < 2; ++i) second[i] = lerp(first[i], first[i + 1], t);
    cp_vec2 position = lerp(second[0], second[1], t);
    cp_vec2 velocity = {3.0 * (second[1].x - second[0].x),
                        3.0 * (second[1].y - second[0].y)};
    cp_vec2 acceleration = {
        6.0 * ((first[2].x - first[1].x) - (first[1].x - first[0].x)),
        6.0 * ((first[2].y - first[1].y) - (first[1].y - first[0].y))
    };
    if (!finite2(position) || !finite2(velocity) || !finite2(acceleration))
        return CP_OVERFLOW;
    return cp_patch_from_jet2(out, position, velocity,
                              acceleration, half_width, valid_u);
}

static cp_status evaluate_local(const cp_curve_patch *patch, cp_vec2 point,
                                double *signed_distance, int *in_domain) {
    cp_vec2 offset = {point.x - patch->origin.x, point.y - patch->origin.y};
    if (!finite2(offset)) return CP_OVERFLOW;
    double u = offset.x * patch->tangent.x + offset.y * patch->tangent.y;
    if (!isfinite(u)) return CP_OVERFLOW;
    *in_domain = fabs(u) <= patch->valid_u;
    if (!*in_domain) return CP_OK;
    double v = offset.x * patch->normal.x + offset.y * patch->normal.y;
    double graph = (patch->a2 * u) * u;
    double residual = v - graph;
    if (!isfinite(v) || !isfinite(graph) || !isfinite(residual)) return CP_OVERFLOW;
    *signed_distance = residual;
    return CP_OK;
}

cp_status cp_patch_point(const cp_curve_patch *patch, double u, cp_vec2 *out) {
    if (!valid_patch(patch) || !out || !isfinite(u)) return CP_INVALID;
    if (fabs(u) > patch->valid_u) return CP_OUTSIDE;
    double v = (patch->a2 * u) * u;
    cp_vec2 point = {patch->origin.x + u * patch->tangent.x + v * patch->normal.x,
                     patch->origin.y + u * patch->tangent.y + v * patch->normal.y};
    if (!isfinite(v) || !finite2(point)) return CP_OVERFLOW;
    *out = point;
    return CP_OK;
}

cp_status cp_patch_signed_distance(const cp_curve_patch *patch,
                                   cp_vec2 point, double *out) {
    if (!valid_patch(patch) || !finite2(point) || !out) return CP_INVALID;
    double signed_distance = 0.0;
    int in_domain = 0;
    cp_status status = evaluate_local(patch, point, &signed_distance, &in_domain);
    if (status != CP_OK) return status;
    if (!in_domain) return CP_OUTSIDE;
    *out = signed_distance;
    return CP_OK;
}

static cp_status pixel_mask(const cp_curve_patch *patch,
                            double pixel_x, double pixel_y, uint64_t *out) {
    uint64_t occupancy = UINT64_C(0);
    for (unsigned sample_y = 0; sample_y < CP_SUBPIXEL_SIDE; ++sample_y) {
        for (unsigned sample_x = 0; sample_x < CP_SUBPIXEL_SIDE; ++sample_x) {
            cp_vec2 point = {
                pixel_x + ((double)sample_x + 0.5) / CP_SUBPIXEL_SIDE - 0.5,
                pixel_y + ((double)sample_y + 0.5) / CP_SUBPIXEL_SIDE - 0.5
            };
            double signed_distance = 0.0;
            int in_domain = 0;
            cp_status status = evaluate_local(patch, point,
                                              &signed_distance, &in_domain);
            if (status != CP_OK) return status;
            if (in_domain && fabs(signed_distance) <= patch->half_width)
                occupancy |= UINT64_C(1) << (sample_y * CP_SUBPIXEL_SIDE + sample_x);
        }
    }
    *out = occupancy;
    return CP_OK;
}

static unsigned count_bits(uint64_t mask) {
    unsigned count = 0;
    while (mask != 0) {
        mask &= mask - UINT64_C(1);
        ++count;
    }
    return count;
}

cp_status cp_patch_coverage_pixel(const cp_curve_patch *patch,
                                  int pixel_x, int pixel_y, double *out) {
    if (!valid_patch(patch) || !out) return CP_INVALID;
    uint64_t mask = 0;
    cp_status status = pixel_mask(patch, (double)pixel_x, (double)pixel_y, &mask);
    if (status != CP_OK) return status;
    *out = (double)count_bits(mask) / 64.0;
    return CP_OK;
}

void cp_patch_accumulator_3x3_init(cp_accumulator_3x3 *acc,
                                   int center_x, int center_y) {
    if (!acc) return;
    memset(acc, 0, sizeof(*acc));
    acc->center_x = center_x;
    acc->center_y = center_y;
}

cp_status cp_patch_accumulate_3x3(cp_accumulator_3x3 *acc,
                                  const cp_curve_patch *patch) {
    if (!acc || !valid_patch(patch)) return CP_INVALID;
    uint64_t new_masks[3][3] = {{0}};
    for (int y = 0; y < CP_GRID3_SIDE; ++y) {
        for (int x = 0; x < CP_GRID3_SIDE; ++x) {
            cp_status status = pixel_mask(patch,
                (double)acc->center_x + (double)(x - 1),
                (double)acc->center_y + (double)(y - 1), &new_masks[y][x]);
            if (status != CP_OK) return status;
        }
    }
    for (int y = 0; y < CP_GRID3_SIDE; ++y)
        for (int x = 0; x < CP_GRID3_SIDE; ++x)
            acc->occupied[y][x] |= new_masks[y][x];
    return CP_OK;
}

void cp_patch_accumulator_3x3_coverage(const cp_accumulator_3x3 *acc,
                                      double out[3][3]) {
    if (!acc || !out) return;
    for (int y = 0; y < CP_GRID3_SIDE; ++y)
        for (int x = 0; x < CP_GRID3_SIDE; ++x)
            out[y][x] = (double)count_bits(acc->occupied[y][x]) / 64.0;
}

cp_status cp_patch_cover_3x3(const cp_curve_patch *patch,
                             int center_x, int center_y, double out[3][3]) {
    if (!out || !valid_patch(patch)) return CP_INVALID;
    cp_accumulator_3x3 accumulator;
    cp_patch_accumulator_3x3_init(&accumulator, center_x, center_y);
    cp_status status = cp_patch_accumulate_3x3(&accumulator, patch);
    if (status != CP_OK) return status;
    cp_patch_accumulator_3x3_coverage(&accumulator, out);
    return CP_OK;
}
