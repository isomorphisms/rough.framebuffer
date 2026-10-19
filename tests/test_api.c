#include "framebuffer.h"

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define WIDTH 6u
#define HEIGHT 6u
#define COLOR_STRIDE 8u
#define DEPTH_STRIDE 9u

static void clear(uint32_t *pixels, float *depth) {
    for (size_t i = 0; i < (size_t)COLOR_STRIDE * HEIGHT; ++i) pixels[i] = 0xffffffffu;
    for (size_t i = 0; i < (size_t)DEPTH_STRIDE * HEIGHT; ++i) depth[i] = INFINITY;
}

int main(void) {
    uint32_t pixels[COLOR_STRIDE * HEIGHT];
    float depth[DEPTH_STRIDE * HEIGHT];
    clear(pixels, depth);

    rough_framebuffer target = {pixels, WIDTH, HEIGHT, COLOR_STRIDE, depth, DEPTH_STRIDE};
    rough_vertex vertex0 = {1.0, 1.0, 0.4};
    rough_vertex vertex1 = {5.0, 1.0, 0.4};
    rough_vertex vertex2 = {1.0, 5.0, 0.4};
    size_t sample = 2u * COLOR_STRIDE + 2u;
    size_t sample_depth = 2u * DEPTH_STRIDE + 2u;

    assert(rough_fill_triangle(&target, vertex0, vertex1, vertex2, 0xff2040e0u, 1.0) == 0);
    assert(pixels[sample] == 0xff2040e0u);
    assert(fabsf(depth[sample_depth] - 0.4f) < 1e-6f);

    /* A later, farther triangle cannot replace the nearer sample. */
    vertex0.depth = vertex1.depth = vertex2.depth = 0.8;
    assert(rough_fill_triangle(&target, vertex0, vertex1, vertex2, 0xffe02020u, 1.0) == 0);
    assert(pixels[sample] == 0xff2040e0u);

    /* Winding does not change coverage; a nearer triangle does replace it. */
    vertex0.depth = vertex1.depth = vertex2.depth = 0.2;
    assert(rough_fill_triangle(&target, vertex0, vertex2, vertex1, 0xff20c040u, 1.0) == 0);
    assert(pixels[sample] == 0xff20c040u);
    assert(fabsf(depth[sample_depth] - 0.2f) < 1e-6f);

    /* Fully transparent geometry neither changes color nor occludes later work. */
    clear(pixels, depth);
    vertex0.depth = vertex1.depth = vertex2.depth = 0.1;
    assert(rough_fill_triangle(&target, vertex0, vertex1, vertex2, 0xff000000u, 0.0) == 0);
    assert(isinf(depth[sample_depth]));
    vertex0.depth = vertex1.depth = vertex2.depth = 0.9;
    assert(rough_fill_triangle(&target, vertex0, vertex1, vertex2, 0xff603020u, 1.0) == 0);
    assert(pixels[sample] == 0xff603020u);

    /* A target without depth remains useful as an explicit painter-order surface. */
    clear(pixels, depth);
    target.depth = NULL;
    target.depth_stride = 0;
    vertex0.depth = vertex1.depth = vertex2.depth = 0.1;
    assert(rough_fill_triangle(&target, vertex0, vertex1, vertex2, 0xff2020d0u, 1.0) == 0);
    vertex0.depth = vertex1.depth = vertex2.depth = 0.9;
    assert(rough_fill_triangle(&target, vertex0, vertex1, vertex2, 0xffd02020u, 1.0) == 0);
    assert(pixels[sample] == 0xffd02020u);

    target.depth = depth;
    target.depth_stride = WIDTH - 1u;
    assert(rough_fill_triangle(&target, vertex0, vertex1, vertex2, 0xff000000u, 1.0) == -1);

    puts("native triangle API: PASS");
    return 0;
}
