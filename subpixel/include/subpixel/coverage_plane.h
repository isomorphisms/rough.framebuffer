#ifndef SUBPIXEL_COVERAGE_PLANE_H
#define SUBPIXEL_COVERAGE_PLANE_H

#include "subpixel/curve_patch.h"
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

/* Boolean algebra on the actual 64 midpoint samples, never on rounded
 * pixel coverage fractions. The destination is the output domain; absent
 * source pixels count as the empty mask. Both planes may have independent
 * origins, extents and padded strides. Only active destination words change;
 * source pixels and all row padding remain unchanged.
 *
 * Preflight checks BOTH layouts, the operation and overlapping backing
 * storage before writing anything. Even self-aliasing is refused, matching
 * whole-path scratch isolation. No allocation and no depth/alpha operations.
 * Combine masks for one logical shape before one source-over resolve. */
typedef enum {
    CP_MASK_UNION,
    CP_MASK_INTERSECTION,
    CP_MASK_DIFFERENCE,
    CP_MASK_SYMMETRIC_DIFFERENCE
} cp_mask_operation;

cp_status cp_coverage_plane_combine(cp_coverage_plane *destination,
    const cp_coverage_plane *source, cp_mask_operation operation);

#endif
