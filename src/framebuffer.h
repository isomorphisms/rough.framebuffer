#ifndef ROUGH_FRAMEBUFFER_H
#define ROUGH_FRAMEBUFFER_H

#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint32_t *pixels;       /* 0xAARRGGBB, on a caller-owned buffer */
    unsigned width;
    unsigned height;
    unsigned stride_pixels; /* at least width */
} rough_framebuffer;

/* Read P/M/L/C operations, rasterize into the provided buffer.
 * Return 0 on success, nonzero on malformed input or invalid surface.
 * Does not allocate, own or clear the pixel memory.
 */
int rough_render_stream(FILE *operations, rough_framebuffer *target);

#endif
