#ifndef SUBLIXEL_COVERAGE_PLANE_H
#define SUBLIXEL_COVERAGE_PLANE_H

#include "sublixel/curve_patch.h"
#include <stddef.h>

/* Caller-owned global occupancy. Each word uses exactly the same 8x8 sample
 * coordinates as the local neighborhood APIs. Pixels outside the plane are
 * clipped; no geometry is rounded before its occupancy has been evaluated.
 * capacity_words includes row padding. Buffers are never allocated or cleared
 * implicitly. Separate shapes/colors require separate occupancy planes. */
typedef struct {
    uint64_t *occupied;
    size_t capacity_words;
    size_t stride_words;
    unsigned width, height;
    int origin_x, origin_y;
} cp_coverage_plane;

cp_status cp_coverage_plane_validate(const cp_coverage_plane *plane);

/* Evaluate one genuine local neighborhood, then union its masks into the
 * plane. Invalid layout or failed patch evaluation leaves the plane unchanged.
 * A neighborhood may be wholly outside the plane and still succeed.
 * These functions do not claim to cover an entire curve from one local patch;
 * curve constructors/sampling plans remain independent geometry producers. */
cp_status cp_coverage_plane_accumulate_3x3(cp_coverage_plane *plane,
    const cp_curve_patch *patch, int center_x, int center_y);
cp_status cp_coverage_plane_accumulate_5x5(cp_coverage_plane *plane,
    const cp_curve_patch *patch, int center_x, int center_y);

#endif
