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

/* Conservative byte spans include row padding. The validated last active
 * word bounds each span, so different origins and strides cannot trick
 * this operation into reading a word it has just overwritten. Compare
 * integer representations: relational comparison of unrelated C pointers
 * is not defined. A nonrepresentable span fails closed. */
static cp_status disjoint_mask_storage(const cp_coverage_plane *destination,
    const cp_coverage_plane *source) {
    size_t destination_words = ((size_t)destination->height - 1) *
        destination->stride_words + destination->width;
    size_t source_words = ((size_t)source->height - 1) *
        source->stride_words + source->width;
    if (destination_words > UINTPTR_MAX / sizeof(uint64_t) ||
        source_words > UINTPTR_MAX / sizeof(uint64_t))
        return CP_OVERFLOW;

    uintptr_t destination_start = (uintptr_t)destination->occupied;
    uintptr_t source_start = (uintptr_t)source->occupied;
    uintptr_t destination_bytes = (uintptr_t)destination_words * sizeof(uint64_t);
    uintptr_t source_bytes = (uintptr_t)source_words * sizeof(uint64_t);
    if (destination_start > UINTPTR_MAX - destination_bytes ||
        source_start > UINTPTR_MAX - source_bytes)
        return CP_OVERFLOW;

    uintptr_t destination_end = destination_start + destination_bytes;
    uintptr_t source_end = source_start + source_bytes;
    if (destination_start < source_end && source_start < destination_end)
        return CP_INVALID;
    return CP_OK;
}

cp_status cp_coverage_plane_combine(cp_coverage_plane *destination,
    const cp_coverage_plane *source, cp_mask_operation operation) {
    cp_status status = cp_coverage_plane_validate(destination);
    if (status != CP_OK) return status;
    status = cp_coverage_plane_validate(source);
    if (status != CP_OK) return status;
    if (operation != CP_MASK_UNION &&
        operation != CP_MASK_INTERSECTION &&
        operation != CP_MASK_DIFFERENCE &&
        operation != CP_MASK_SYMMETRIC_DIFFERENCE)
        return CP_INVALID;
    status = disjoint_mask_storage(destination, source);
    if (status != CP_OK) return status;

    /* No failing operations occur after the preflight: bad inputs cannot
     * leave a partially changed destination. Pixel bounds are computed in
     * int64_t to support a negative origin and the full valid int range. */
    for (unsigned row = 0; row < destination->height; ++row) {
        int64_t source_row = (int64_t)destination->origin_y + row -
            source->origin_y;
        int row_inside = source_row >= 0 &&
            source_row < (int64_t)source->height;
        for (unsigned column = 0; column < destination->width; ++column) {
            int64_t source_column = (int64_t)destination->origin_x +
                column - source->origin_x;
            uint64_t from = 0;
            if (row_inside && source_column >= 0 &&
                source_column < (int64_t)source->width)
                from = source->occupied[(size_t)source_row *
                    source->stride_words + (size_t)source_column];
            uint64_t *into = &destination->occupied[(size_t)row *
                destination->stride_words + column];
            switch (operation) {
            case CP_MASK_UNION: *into |= from; break;
            case CP_MASK_INTERSECTION: *into &= from; break;
            case CP_MASK_DIFFERENCE: *into &= ~from; break;
            case CP_MASK_SYMMETRIC_DIFFERENCE: *into ^= from; break;
            }
        }
    }
    return CP_OK;
}
