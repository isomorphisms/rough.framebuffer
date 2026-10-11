#include "sublixel_demo.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t pixel(rough_framebuffer *frame, unsigned x, unsigned y) {
    return frame->pixels[(size_t)y * frame->stride_pixels + x];
}

/* An integer-rational coverage oracle independent of Sublixel's
 * floating-point projection and evaluation. */
static unsigned reference_coverage(int x, int y, int cubic) {
    unsigned count = 0u;
    for (int sy = 0; sy < 8; ++sy)
        for (int sx = 0; sx < 8; ++sx) {
            long nx = 16L * x + 2L * sx - 7L;
            long ny = 16L * y + 2L * sy - 7L;
            long U = 4L * nx + 3L * ny;
            long V = -3L * nx + 4L * ny;
            long residual = 102400L * V - 320L * U * U;
            if (cubic) residual -= U * U * U;
            if (U >= -240L && U <= 240L &&
                residual >= -1536000L && residual <= 1536000L)
                ++count;
        }
    return count;
}

/* Exact midpoint coverage fractions have denominator 64, so integer
 * rounding matches the renderer's opaque coverage conversion. */
static uint32_t expected_color(uint32_t ink, unsigned occupied) {
    uint32_t result = UINT32_C(0xff000000);
    const unsigned shifts[] = {16u, 8u, 0u};
    assert(occupied <= 64);
    for (unsigned i = 0; i < 3; ++i) {
        unsigned component = (ink >> shifts[i]) & 255u;
        unsigned rounded = (255u * (64u - occupied) +
                            component * occupied + 32u) / 64u;
        result |= (uint32_t)rounded << shifts[i];
    }
    return result;
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

    /* 3-4-5 tangent ensures a visible diagonal, not a flat horizontal
     * row; the cubic 5x5 extends beyond the smaller support.
     * These counts are independent of the floating-point curve library.
     *
     * Sample pixel (x,y) at integer lattice (nx,ny)/16.
     * u=(4*nx+3*ny)/80=U/80; v=(-3*nx+4*ny)/80=V/80.
     * For half-width 3/16 the exact occupied predicate is
     * |102400*V-320*U^2-(cubic?U^3:0)| <= 1536000,
     * plus |U|<=240. All products fit in a signed 32-bit long. */
    static const unsigned expected3[3][3] = {
        {5, 0, 0},
        {18, 29, 1},
        {0, 3, 19}
    };
    static const unsigned expected5[5][5] = {
        {0, 0, 0, 0, 0},
        {27, 11, 0, 0, 0},
        {0, 13, 29, 1, 0},
        {0, 0, 4, 17, 0},
        {0, 0, 0, 14, 0}
    };
    /* Explicit regression against horizontal-tangent 3x3: both upper
     * left and lower right cells are covered, with different sample counts. */
    assert(expected3[0][0] > 0 && expected3[2][2] > 0);
    assert(expected3[0][0] != expected3[2][2]);
    assert(expected3[1][0] > expected3[1][2]);
    for (unsigned y = 0; y < 5; ++y)
        for (unsigned x = 0; x < 5; ++x) {
            unsigned q = 0;
            if (x >= 1 && x <= 3 && y >= 1 && y <= 3)
                q = expected3[y - 1][x - 1];
            unsigned c = expected5[y][x];
            if (x >= 1 && x <= 3 && y >= 1 && y <= 3) {
                assert(q == reference_coverage((int)x - 2, (int)y - 2, 0));
            }
            assert(c == reference_coverage((int)x - 2, (int)y - 2, 1));
            /* Check pixel colors, not merely scalar oracle counts. */
            assert(pixel(&frame, layout.left_x + x * side + side / 2u,
                         top + y * side + side / 2u) ==
                   expected_color(UINT32_C(0xff097ca2), q));
            assert(pixel(&frame, layout.right_x + x * side + side / 2u,
                         top + y * side + side / 2u) ==
                   expected_color(UINT32_C(0xffa63873), c));
        }
    /* Outside the 3x3 support but occupied by the cubic 5x5. */
    assert(expected5[4][3] == 14);
    assert(pixel(&frame, layout.left_x + 3u * side + side / 2u,
                 top + 4u * side + side / 2u) == UINT32_C(0xffffffff));
    assert(pixel(&frame, layout.right_x + 3u * side + side / 2u,
                 top + 4u * side + side / 2u) != UINT32_C(0xffffffff));
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
