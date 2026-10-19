#ifndef ROUGH_FRAMEBUFFER_H
#define ROUGH_FRAMEBUFFER_H

#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint32_t *pixels;       /* straight-alpha 0xAARRGGBB, caller-owned */
    unsigned width;
    unsigned height;
    unsigned stride_pixels; /* at least width */
} rough_framebuffer;

/* Read P/A/W/M/L/C operations and rasterize into the provided buffer.
 * L and C may carry an optional final width for linearly tapered strokes.
 * Return 0 on success, nonzero on malformed input or an invalid surface.
 * Does not allocate, own, or clear the pixel memory.
 */
int rough_render_stream(FILE *operations, rough_framebuffer *target);

#endif
