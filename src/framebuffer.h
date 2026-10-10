#ifndef ROUGH_FRAMEBUFFER_H
#define ROUGH_FRAMEBUFFER_H

#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint32_t *pixels;       /* straight-alpha 0xAARRGGBB, caller-owned */
    unsigned width;
    unsigned height;
    unsigned stride_pixels; /* at least width */
    float *depth;           /* optional caller-owned depth; smaller is nearer */
    unsigned depth_stride;  /* float elements, at least width when depth != NULL */
} rough_framebuffer;

typedef struct {
    double x;
    double y;
    double depth;
} rough_vertex;

/* Fill a screen-space triangle with a half-open top-left rule. Depth is
 * interpolated linearly at integer-coordinate pixel centers. When target->depth
 * is non-NULL, a fragment passes only when its depth is strictly smaller.
 * Return 0 on success (including degenerate/off-screen triangles), -1 on an
 * invalid target or parameter. Color contributes its RGB bytes; opacity is
 * supplied separately.
 */
int rough_fill_triangle(rough_framebuffer *target,
                        rough_vertex vertex0,
                        rough_vertex vertex1,
                        rough_vertex vertex2,
                        uint32_t color,
                        double opacity);

/* Read P/A/W/M/L/C/T operations and rasterize into the provided buffers.
 * L and C may carry an optional final width for linearly tapered strokes.
 * T carries three x/y/depth vertices and uses the current color and opacity.
 * Return 0 on success, nonzero on malformed input or an invalid surface.
 * Does not allocate, own, or clear color or depth memory.
 */
int rough_render_stream(FILE *operations, rough_framebuffer *target);

#endif
