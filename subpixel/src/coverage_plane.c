#include "subpixel/coverage_plane.h"

#include <limits.h>

cp_status cp_coverage_plane_validate(const cp_coverage_plane *plane) {
    if (!plane || !plane->occupied || !plane->width || !plane->height ||
        plane->stride_words < plane->width ||
        plane->width > INT_MAX || plane->height > INT_MAX)
        return CP_INVALID;
    if ((int64_t)plane->origin_x + plane->width - 1 > INT_MAX ||
        (int64_t)plane->origin_y + plane->height - 1 > INT_MAX)
        return CP_OVERFLOW;
    size_t last_column = (size_t)plane->width - 1;
    size_t last_row = (size_t)plane->height - 1;
    if (last_row > (SIZE_MAX - last_column) / plane->stride_words)
        return CP_OVERFLOW;
    size_t last_index = last_row * plane->stride_words + last_column;
    if (last_index >= SIZE_MAX / sizeof(uint64_t)) return CP_OVERFLOW;
    if (last_index >= plane->capacity_words) return CP_INVALID;
    return CP_OK;
}

static void merge_mask(cp_coverage_plane *plane, int64_t pixel_x,
                       int64_t pixel_y, uint64_t mask) {
    int64_t column = pixel_x - plane->origin_x;
    int64_t row = pixel_y - plane->origin_y;
    if (column < 0 || row < 0 || column >= plane->width || row >= plane->height)
        return;
    plane->occupied[(size_t)row * plane->stride_words + (size_t)column] |= mask;
}

cp_status cp_coverage_plane_accumulate_3x3(cp_coverage_plane *plane,
    const cp_curve_patch *patch, int center_x, int center_y) {
    cp_status status = cp_coverage_plane_validate(plane);
    if (status != CP_OK) return status;
    cp_accumulator_3x3 neighborhood;
    cp_patch_accumulator_3x3_init(&neighborhood, center_x, center_y);
    status = cp_patch_accumulate_3x3(&neighborhood, patch);
    if (status != CP_OK) return status;
    for (int row = 0; row < CP_GRID3_SIDE; ++row)
        for (int column = 0; column < CP_GRID3_SIDE; ++column)
            merge_mask(plane, (int64_t)center_x + column - 1,
                       (int64_t)center_y + row - 1,
                       neighborhood.occupied[row][column]);
    return CP_OK;
}

cp_status cp_coverage_plane_accumulate_5x5(cp_coverage_plane *plane,
    const cp_curve_patch *patch, int center_x, int center_y) {
    cp_status status = cp_coverage_plane_validate(plane);
    if (status != CP_OK) return status;
    cp_accumulator_5x5 neighborhood;
    cp_patch_accumulator_5x5_init(&neighborhood, center_x, center_y);
    status = cp_patch_accumulate_5x5(&neighborhood, patch);
    if (status != CP_OK) return status;
    for (int row = 0; row < CP_GRID5_SIDE; ++row)
        for (int column = 0; column < CP_GRID5_SIDE; ++column)
            merge_mask(plane, (int64_t)center_x + column - 2,
                       (int64_t)center_y + row - 2,
                       neighborhood.occupied[row][column]);
    return CP_OK;
}
