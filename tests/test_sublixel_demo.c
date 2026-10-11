#include "sublixel_demo.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t pixel(rough_framebuffer *frame, unsigned x, unsigned y) {
    return frame->pixels[(size_t)y * frame->stride_pixels + x];
}

int main(void) {
    enum { W = 288, H = 220, STRIDE = 292 };
    uint32_t *pixels = malloc((size_t)STRIDE * H * sizeof(uint32_t));
    assert(pixels);
    for (unsigned i = 0; i < (unsigned)STRIDE * H; ++i)
        pixels[i] = UINT32_C(0xffffffff);
    rough_framebuffer frame = {pixels, W, H, STRIDE, NULL, 0};
    rough_sublixel_demo_layout layout = {0};
    assert(rough_sublixel_demo_draw(&frame, &layout) == 0);
    assert(layout.cell >= 12u);
    assert(layout.right_x > layout.left_x + 5u * layout.cell);
    unsigned side = layout.cell;
    unsigned top = layout.top_y;

    /* A rational-coverage oracle elsewhere requires the cubic corner
     * [4][4] to be 11/64 while the quadratic 3x3 has no [4][4].
     * Sample INSIDE cells rather than the border/label. */
    uint32_t outside_quadratic =
        pixel(&frame, layout.left_x + 4u * side + side / 2u,
              top + 4u * side + side / 2u);
    uint32_t outside_cubic =
        pixel(&frame, layout.right_x + 4u * side + side / 2u,
              top + 4u * side + side / 2u);
    assert(outside_quadratic == UINT32_C(0xffffffff));
    assert(outside_cubic != UINT32_C(0xffffffff));
    assert(outside_cubic != UINT32_C(0xffcccccc));
    /* Both panels contain genuine fractional alpha samples. */
    uint32_t center3 = pixel(&frame, layout.left_x + 2u * side + side / 2u,
                             top + 2u * side + side / 2u);
    uint32_t center5 = pixel(&frame, layout.right_x + 2u * side + side / 2u,
                             top + 2u * side + side / 2u);
    assert(center3 != UINT32_C(0xffffffff));
    assert(center5 != UINT32_C(0xffffffff));
    assert(center5 != center3);
    for (unsigned y = 0; y < H; ++y)
        for (unsigned x = W; x < STRIDE; ++x)
            assert(pixel(&frame, x, y) == UINT32_C(0xffffffff));

    uint32_t *copy = malloc((size_t)STRIDE * H * sizeof(uint32_t));
    assert(copy);
    memcpy(copy, pixels, (size_t)STRIDE * H * sizeof(uint32_t));
    assert(rough_sublixel_demo_draw(&frame, NULL) == 0);
    assert(memcmp(copy, pixels, (size_t)STRIDE * H * sizeof(uint32_t)) == 0);

    rough_framebuffer invalid = frame;
    invalid.width = 20;
    assert(rough_sublixel_demo_draw(&invalid, NULL) == -1);
    assert(memcmp(copy, pixels, (size_t)STRIDE * H * sizeof(uint32_t)) == 0);
    free(copy);
    free(pixels);
    puts("C67 Sublixel 3x3/5x5 visual panels: PASS");
    return 0;
}
