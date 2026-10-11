#include "framebuffer.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int render(const char *operations, rough_framebuffer *framebuffer) {
    FILE *input = tmpfile();
    assert(input != NULL);
    assert(fputs(operations, input) >= 0);
    assert(fflush(input) == 0);
    rewind(input);
    int result = rough_render_stream(input, framebuffer);
    assert(fclose(input) == 0);
    return result;
}

static void test_stream_and_pixel_values(void) {
    uint32_t pixels[20 * 10];
    for (size_t i = 0; i < sizeof(pixels) / sizeof(pixels[0]); ++i)
        pixels[i] = 0xffffffffu;

    rough_framebuffer frame = {pixels, 20, 10, 20};
    assert(render("P 240 20 50\n"
                  "M 2 2\nL 18 2\n"
                  "M 2 4\nC 5 4 14 4 18 4\n"
                  "P 0 0 0\n"
                  "M -5000 -5000\nL -4000 -4000\n", &frame) == 0);

    assert(pixels[2 * 20 + 2] == 0xfff01432u);
    assert(pixels[2 * 20 + 10] == 0xfff01432u);
    assert(pixels[4 * 20 + 10] == 0xfff01432u);
    assert(pixels[8 * 20 + 10] == 0xffffffffu);

    /* The caller owns pixel storage, which remains valid after rendering. */
    pixels[0] = 0x12345678u;
    assert(pixels[0] == 0x12345678u);
}

static void test_refusals_and_clipping(void) {
    uint32_t pixels[20 * 10];
    for (size_t i = 0; i < sizeof(pixels) / sizeof(pixels[0]); ++i)
        pixels[i] = 0xffffffffu;

    rough_framebuffer frame = {pixels, 20, 10, 20};
    assert(render("M 0 0 junk\n", &frame) != 0);
    assert(render("L 8 2\n", &frame) != 0);
    assert(render("M 1 2 3\n", &frame) != 0);
    assert(render("M -100000000 -100000000\n"
                  "L 100000000 100000000\n", &frame) == 0);
    assert(pixels[0] == 0xff000000u);

    rough_framebuffer invalid_stride = {pixels, 20, 10, 19};
    assert(render("M 0 0\nL 2 2\n", &invalid_stride) != 0);
    rough_framebuffer invalid_pixels = {NULL, 20, 10, 20};
    assert(render("M 0 0\nL 2 2\n", &invalid_pixels) != 0);
    assert(rough_render_stream(NULL, &frame) != 0);
}

int main(void) {
    test_stream_and_pixel_values();
    test_refusals_and_clipping();
    puts("ICK-compiled framebuffer C smoke: PASS");
    return 0;
}
