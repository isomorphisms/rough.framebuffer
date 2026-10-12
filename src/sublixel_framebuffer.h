#ifndef ROUGH_SUBLIXEL_FRAMEBUFFER_H
#define ROUGH_SUBLIXEL_FRAMEBUFFER_H

#include "framebuffer.h"
#include "sublixel/coverage_plane.h"

/* Resolve a single shape's unioned sample coverage and source-over blend into
 * the actual caller-owned framebuffer. Color is straight-alpha AARRGGBB;
 * effective source alpha = color alpha * opacity * occupied samples / 64.
 * Plane coordinates are framebuffer pixel coordinates. Padded rows and clipped
 * origins are supported. Neither buffer is allocated or cleared. Occupancy is
 * unchanged. Invalid inputs leave pixels unchanged. This is a 2D color layer,
 * not depth-aware strokes or subpixel triangle/Android qualification.
 * The occupancy and destination storage must not overlap. */
int rough_blend_sublixel_coverage(rough_framebuffer *target,
    const cp_coverage_plane *coverage, uint32_t color, double opacity);

#endif
