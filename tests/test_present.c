#include "present.h"
#include "surface.h"

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void equal_rgba(const uint8_t *pixels, size_t pixel,
                       unsigned red, unsigned green, unsigned blue,
                       unsigned alpha) {
    size_t offset = pixel * 4u;
    assert(pixels[offset] == red);
    assert(pixels[offset + 1u] == green);
    assert(pixels[offset + 2u] == blue);
    assert(pixels[offset + 3u] == alpha);
}

static void test_bilinear_and_padding(void) {
    /* The poisoned third pixel in each source row must never be sampled;
     * two padded destination pixels on every row must remain unchanged. */
    uint32_t pixels[6] = {
        0xffff0000u, 0xff00ff00u, 0x01ffff00u,
        0xff0000ffu, 0xffffffffu, 0x01111111u
    };
    rough_framebuffer source = {pixels, 2u, 2u, 3u, NULL, 0u};
    uint8_t output[3u * 5u * 4u];
    memset(output, 0xa5, sizeof output);
    assert(rough_present_rgba8888(&source, output, 3u, 3u, 5u) == 0);

    equal_rgba(output, 0u, 255u, 0u, 0u, 255u);
    equal_rgba(output, 1u, 128u, 128u, 0u, 255u);
    equal_rgba(output, 2u, 0u, 255u, 0u, 255u);
    equal_rgba(output, 5u, 128u, 0u, 128u, 255u);
    equal_rgba(output, 6u, 128u, 128u, 128u, 255u);
    equal_rgba(output, 7u, 128u, 255u, 128u, 255u);
    equal_rgba(output, 10u, 0u, 0u, 255u, 255u);
    equal_rgba(output, 11u, 128u, 128u, 255u, 255u);
    equal_rgba(output, 12u, 255u, 255u, 255u, 255u);
    for (unsigned y = 0u; y < 3u; ++y) {
        for (unsigned x = 3u; x < 5u; ++x) {
            size_t at = ((size_t)y * 5u + x) * 4u;
            for (unsigned c = 0; c < 4u; ++c)
                assert(output[at + c] == 0xa5);
        }
    }

    /* Matching source/destination dimensions must preserve byte values. */
    uint8_t identity[2u * 2u * 4u] = {0};
    assert(rough_present_rgba8888(&source, identity, 2u, 2u, 2u) == 0);
    equal_rgba(identity, 0u, 255u, 0u, 0u, 255u);
    equal_rgba(identity, 1u, 0u, 255u, 0u, 255u);
    equal_rgba(identity, 2u, 0u, 0u, 255u, 255u);
    equal_rgba(identity, 3u, 255u, 255u, 255u, 255u);
}

static void test_straight_alpha(void) {
    uint32_t transparent_red_and_blue[2] = {0x00ff0000u, 0xff0000ffu};
    rough_framebuffer source = {transparent_red_and_blue, 2u, 1u, 2u, NULL, 0u};
    uint8_t output[3u * 4u] = {0};
    assert(rough_present_rgba8888(&source, output, 3u, 1u, 3u) == 0);
    equal_rgba(output, 0u, 0u, 0u, 0u, 0u);
    /* Half-transparent red must not make a purple fringe around blue. */
    equal_rgba(output, 1u, 0u, 0u, 255u, 128u);
    equal_rgba(output, 2u, 0u, 0u, 255u, 255u);

    uint32_t partially_transparent_red[] = {0x80ff0000u};
    source.pixels = partially_transparent_red;
    source.width = source.height = source.stride_pixels = 1u;
    uint8_t filled[5u * 4u] = {0};
    assert(rough_present_rgba8888(&source, filled, 5u, 1u, 5u) == 0);
    for (unsigned i = 0; i < 5u; ++i)
        equal_rgba(filled, i, 255u, 0u, 0u, 128u);
}

static void test_invalid_inputs(void) {
    uint32_t pixel = 0xffffffffu;
    rough_framebuffer valid = {&pixel, 1u, 1u, 1u, NULL, 0u};
    uint8_t sentinel[4] = {0xa5, 0xa5, 0xa5, 0xa5};
    assert(rough_present_rgba8888(NULL, sentinel, 1u, 1u, 1u) == -1);
    assert(rough_present_rgba8888(&valid, NULL, 1u, 1u, 1u) == -1);
    assert(rough_present_rgba8888(&valid, sentinel, 0u, 1u, 1u) == -1);
    assert(rough_present_rgba8888(&valid, sentinel, 1u, 0u, 1u) == -1);
    assert(rough_present_rgba8888(&valid, sentinel, 2u, 1u, 1u) == -1);
    assert(rough_present_rgba8888(&valid, sentinel, 16385u, 1u, 16385u) == -1);
    rough_framebuffer empty = {0};
    assert(rough_present_rgba8888(&empty, sentinel, 1u, 1u, 1u) == -1);
    rough_framebuffer padded = valid;
    padded.width = 2u;
    assert(rough_present_rgba8888(&padded, sentinel, 1u, 1u, 1u) == -1);
    for (unsigned i = 0; i < 4u; ++i) assert(sentinel[i] == 0xa5);
}

/* The app's resampling stage must preserve meaningful rotation and surface
 * changes as actual RGBA bytes, not merely changes to gesture variables.
 */
static uint64_t rendered_rgba_hash(rough_surface_shape shape, double yaw) {
    const unsigned width = 90u, height = 90u;
    const size_t size = (size_t)width * height;
    uint32_t *pixels = malloc(size * sizeof *pixels);
    float *depth = malloc(size * sizeof *depth);
    uint8_t *rgba = malloc(size * 4u * 4u);
    assert(pixels && depth && rgba);
    for (size_t i = 0; i < size; ++i) {
        pixels[i] = 0xffffffffu;
        depth[i] = INFINITY;
    }
    rough_framebuffer target = {pixels, width, height, width, depth, width};
    assert(rough_draw_surface(&target, shape, yaw, 5.2) == 0);
    assert(rough_present_rgba8888(&target, rgba,
                                  width * 2u, height * 2u,
                                  width * 2u) == 0);
    uint64_t checksum = UINT64_C(14695981039346656037);
    unsigned not_white = 0;
    for (size_t i = 0; i < size * 4u; ++i) {
        size_t offset = i * 4u;
        if (rgba[offset] != 255u || rgba[offset + 1u] != 255u ||
            rgba[offset + 2u] != 255u) ++not_white;
        for (unsigned c = 0; c < 4u; ++c) {
            checksum ^= (uint64_t)rgba[offset + c];
            checksum *= UINT64_C(1099511628211);
        }
    }
    assert(not_white > 300u);
    free(rgba);
    free(depth);
    free(pixels);
    return checksum;
}

int main(void) {
    test_bilinear_and_padding();
    test_straight_alpha();
    test_invalid_inputs();
    uint64_t torus0 = rendered_rgba_hash(ROUGH_SURFACE_TORUS, 0.0);
    uint64_t torus90 = rendered_rgba_hash(ROUGH_SURFACE_TORUS, 90.0);
    uint64_t enneper = rendered_rgba_hash(ROUGH_SURFACE_ENNEPER, 25.0);
    assert(torus0 != torus90);
    assert(torus0 != enneper);
    puts("Android RGBA8888 bilinear presentation, alpha, stride and visible changes: PASS");
    return 0;
}
