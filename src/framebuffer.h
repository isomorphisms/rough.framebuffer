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

/* CPU per-fragment smooth shading on the same pixel-center/top-left and
 * smaller-is-nearer depth conventions as rough_fill_triangle. The vertex
 * position is already projected. reciprocal_z is 1 / positive camera z;
 * normal and unlit material RGB are camera/world attributes interpolated
 * perspective-correctly before a two-sided Lambert diffuse+ambient term.
 * Normal is re-normalized at each covered pixel (Phong, not Gouraud).
 * Caller supplies and owns color and depth surfaces. The operation is
 * deliberately fully opaque; the existing P/T/alpha protocol is unchanged.
 */
typedef struct {
    rough_vertex position;
    double reciprocal_z;
    double normal_x, normal_y, normal_z;
    double base_red, base_green, base_blue;
} rough_smooth_vertex;

int rough_fill_smooth_triangle(rough_framebuffer *target,
                               rough_smooth_vertex vertex0,
                               rough_smooth_vertex vertex1,
                               rough_smooth_vertex vertex2);


/* Read P/A/W/M/L/C/T operations and rasterize into the provided buffers.
 * L and C may carry an optional final width for linearly tapered strokes.
 * T carries three x/y/depth vertices and uses the current color and opacity.
 * Return 0 on success, nonzero on malformed input or an invalid surface.
 * Does not allocate, own, or clear color or depth memory.
 */
int rough_render_stream(FILE *operations, rough_framebuffer *target);

#endif
