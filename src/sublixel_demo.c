#include "sublixel_demo.h"
#include "sublixel/curve_patch.h"

#include <stddef.h>
#include <stdint.h>

/* Each tile shows exactly one software pixel's 64 midpoint samples,
 * magnified with a one-pixel border. This panel is a discrete diagnostic,
 * not a renderer-quality promise for triangles beneath it. */
static void paint_pixel(rough_framebuffer *frame, unsigned x, unsigned y,
                        uint32_t color) {
    frame->pixels[(size_t)y * frame->stride_pixels + x] = color;
}
static unsigned channel(unsigned shade, unsigned ink, double coverage) {
    double value = (double)shade + ((double)ink - shade) * coverage;
    return (unsigned)(value + 0.5);
}
static uint32_t blend_opaque(uint32_t ink, double coverage) {
    unsigned r = channel(255u, (ink >> 16) & 255u, coverage);
    unsigned g = channel(255u, (ink >> 8) & 255u, coverage);
    unsigned b = channel(255u, ink & 255u, coverage);
    return UINT32_C(0xff000000) | (r << 16) | (g << 8) | b;
}
static void cell(rough_framebuffer *frame, unsigned x, unsigned y,
                 unsigned side, uint32_t ink, double coverage) {
    uint32_t fill = blend_opaque(ink, coverage);
    for (unsigned yy = 0; yy < side; ++yy)
        for (unsigned xx = 0; xx < side; ++xx)
            paint_pixel(frame, x + xx, y + yy,
                        (xx == 0 || yy == 0) ? UINT32_C(0xffcccccc) : fill);
}
static void glyph(rough_framebuffer *frame, unsigned x, unsigned y,
                  char ch, uint32_t color) {
    /* Three explicit ASCII glyphs: "3x3" and "5x5". */
    static const unsigned char three[7] = {
        0x1e, 0x01, 0x01, 0x0e, 0x01, 0x01, 0x1e
    };
    static const unsigned char five[7] = {
        0x1f, 0x10, 0x10, 0x1e, 0x01, 0x01, 0x1e
    };
    static const unsigned char xletter[7] = {
        0, 0x11, 0x0a, 0x04, 0x0a, 0x11, 0
    };
    const unsigned char *bitmap =
        ch == '3' ? three : (ch == '5' ? five : xletter);
    for (unsigned row = 0; row < 7; ++row)
        for (unsigned col = 0; col < 5; ++col)
            if (bitmap[row] & (1u << (4u - col)))
                for (unsigned dy = 0; dy < 2; ++dy)
                    for (unsigned dx = 0; dx < 2; ++dx)
                        paint_pixel(frame, x + 2u * col + dx,
                                    y + 2u * row + dy, color);
}
static void title(rough_framebuffer *frame, unsigned x, unsigned y,
                  char digit, uint32_t color) {
    glyph(frame, x + 2u, y, digit, color);
    glyph(frame, x + 14u, y, 'x', color);
    glyph(frame, x + 26u, y, digit, color);
}

int rough_sublixel_demo_draw(rough_framebuffer *frame,
                             rough_sublixel_demo_layout *out_layout) {
    if (!frame || !frame->pixels || frame->width < 190u ||
        frame->height < 130u || frame->width > 8192u ||
        frame->height > 8192u || frame->stride_pixels < frame->width)
        return -1;
    size_t last_row = (size_t)frame->height - 1u;
    if (last_row > (SIZE_MAX / sizeof(uint32_t) -
                    (size_t)frame->width) / frame->stride_pixels) return -1;

    unsigned side = (frame->width - 30u) / 12u;
    if (side > 18u) side = 18u;
    if (side < 12u) side = 12u;
    unsigned padding = side;
    unsigned both = 10u * side + padding;
    if (both + 8u > frame->width || 5u * side + 28u > frame->height)
        return -1;
    unsigned left_x = (frame->width - both) / 2u;
    unsigned right_x = left_x + 5u * side + padding;
    unsigned top_y = 29u;

    cp_curve_patch cubic;
    cp_status status = cp_patch_from_jet3(&cubic,
        (cp_vec2){0.0, 0.0}, (cp_vec2){1.0, 0.0},
        (cp_vec2){0.0, 0.5}, (cp_vec2){0.0, 0.375},
        3.0 / 16.0, 3.0);
    if (status != CP_OK) return -1;
    double quadratic[3][3], cubic_grid[5][5];
    if (cp_patch_cover_3x3(&cubic, 0, 0, quadratic) != CP_OK ||
        cp_patch_cover_5x5(&cubic, 0, 0, cubic_grid) != CP_OK) return -1;

    /* Both panels are 5x5; unused 3x3 outer rows remain blank. */
    for (unsigned y = 0; y < 5u; ++y)
        for (unsigned x = 0; x < 5u; ++x) {
            double small = 0.0;
            if (x >= 1u && x <= 3u && y >= 1u && y <= 3u)
                small = quadratic[y - 1u][x - 1u];
            cell(frame, left_x + x * side, top_y + y * side,
                 side, UINT32_C(0xff097ca2), small);
            cell(frame, right_x + x * side, top_y + y * side,
                 side, UINT32_C(0xffa63873), cubic_grid[y][x]);
        }
    title(frame, left_x, 8u, '3', UINT32_C(0xff097ca2));
    title(frame, right_x, 8u, '5', UINT32_C(0xffa63873));
    if (out_layout)
        *out_layout = (rough_sublixel_demo_layout){
            left_x, right_x, top_y, side
        };
    return 0;
}
