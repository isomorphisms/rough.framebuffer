#include "surface_aa.h"

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void test_explicit_edge_coverage_and_depth(void) {
    uint32_t high_pixels[4u * 6u];
    float high_depth[4u * 6u];
    uint32_t low_pixels[2u * 3u];
    float low_depth[2u * 3u];
    for (unsigned i = 0; i < 24u; ++i) {
        high_pixels[i] = UINT32_C(0xffabcdef);
        high_depth[i] = -42.0f; /* poison outside logical width */
    }
    for (unsigned y = 0; y < 4u; ++y)
        for (unsigned x = 0; x < 4u; ++x) {
            high_pixels[y * 6u + x] = UINT32_C(0xffffffff);
            high_depth[y * 6u + x] = INFINITY;
        }
    for (unsigned i = 0; i < 6u; ++i) {
        low_pixels[i] = UINT32_C(0x12345678);
        low_depth[i] = -77.0f;
    }

    /* One covered red subpixel and three white subpixels form a fractional
     * red edge at the software pixel boundary. The depth comes from that
     * foreground red sample, not the (uncovered) center of the coarse pixel. */
    high_pixels[0] = UINT32_C(0xffff0000);
    high_depth[0] = 0.25f;
    rough_framebuffer high = {high_pixels, 4u, 4u, 6u, high_depth, 6u};
    rough_framebuffer low = {low_pixels, 2u, 2u, 3u, low_depth, 3u};
    assert(rough_resolve_2x2(&high, &low) == 0);
    assert(low_pixels[0] == UINT32_C(0xffffbfbf));
    assert(low_depth[0] == 0.25f);
    assert(low_pixels[1] == UINT32_C(0xffffffff));
    assert(isinf(low_depth[1]));
    assert(low_pixels[2] == UINT32_C(0x12345678));
    assert(low_depth[2] == -77.0f);
    assert(low_pixels[5] == UINT32_C(0x12345678));
    assert(low_depth[5] == -77.0f);

    /* Same coarse pixel, two *independently* z-tested foreground samples:
     * the nearer green sample controls the closest depth witness. */
    high_pixels[2] = UINT32_C(0xffff0000);
    high_depth[2] = 0.8f;
    high_pixels[3] = UINT32_C(0xff00ff00);
    high_depth[3] = 0.2f;
    assert(rough_resolve_2x2(&high, &low) == 0);
    assert(low_depth[1] == 0.2f);
    assert(low_pixels[1] == UINT32_C(0xffbfdf80));

    /* Even when only one of four subpixels has opacity, the RGBA color
     * must be unpremultiplied after averaging, never tinted by RGB from
     * the entirely transparent samples. */
    for (unsigned y = 0; y < 4u; ++y)
        for (unsigned x = 0; x < 4u; ++x) {
            high_pixels[y * 6u + x] = UINT32_C(0x00ff0000);
            high_depth[y * 6u + x] = INFINITY;
        }
    high_pixels[0] = UINT32_C(0xff0000ff);
    high_depth[0] = 0.1f;
    assert(rough_resolve_2x2(&high, &low) == 0);
    assert(low_pixels[0] == UINT32_C(0x400000ff));
    assert(low_depth[0] == 0.1f);

    /* Fail closed on malformed source and never read padding as a sample. */
    high_depth[0] = NAN;
    low_pixels[0] = UINT32_C(0xdeadbeef);
    assert(rough_resolve_2x2(&high, &low) == -1);
    assert(low_pixels[0] == UINT32_C(0xdeadbeef));
    high_depth[0] = INFINITY;
    assert(rough_resolve_2x2(NULL, &low) == -1);
    assert(rough_resolve_2x2(&high, NULL) == -1);
    rough_framebuffer invalid = high;
    invalid.width = 3u;
    assert(rough_resolve_2x2(&invalid, &low) == -1);
    invalid = high;
    invalid.stride_pixels = 2u;
    assert(rough_resolve_2x2(&invalid, &low) == -1);
}

static void test_real_math_surface(rough_surface_shape shape,
                                   double yaw, double distance) {
    const unsigned width = 120u, height = 120u;
    const size_t count = (size_t)width * height;
    uint32_t *pixels = malloc(count * sizeof *pixels);
    uint32_t *plain_pixels = malloc(count * sizeof *plain_pixels);
    float *depth = malloc(count * sizeof *depth);
    float *plain_depth = malloc(count * sizeof *plain_depth);
    const unsigned high_width = width * 2u, high_height = height * 2u;
    const size_t high_count = (size_t)high_width * high_height;
    uint32_t *high_pixels = malloc(high_count * sizeof *high_pixels);
    float *high_depth = malloc(high_count * sizeof *high_depth);
    assert(pixels && plain_pixels && depth && plain_depth &&
           high_pixels && high_depth);

    for (size_t i = 0; i < count; ++i) {
        pixels[i] = plain_pixels[i] = UINT32_C(0xffffffff);
        depth[i] = plain_depth[i] = INFINITY;
    }

    rough_framebuffer aa = {pixels, width, height, width, depth, width};
    rough_framebuffer normal = {plain_pixels, width, height, width,
                                plain_depth, width};
    rough_framebuffer high = {high_pixels, high_width, high_height, high_width,
                              high_depth, high_width};
    assert(rough_draw_surface_aa2(&aa, &high, shape, yaw, distance) == 0);
    assert(rough_draw_surface(&normal, shape, yaw, distance) == 0);
    unsigned changed = 0u;
    unsigned fractional = 0u;
    unsigned foreground = 0u;
    for (size_t i = 0; i < count; ++i) {
        if (pixels[i] != plain_pixels[i]) ++changed;
        if (pixels[i] != UINT32_C(0xffffffff)) ++foreground;
        /* The AA output must produce intermediate values between background
         * white and a colored triangle, not just the old center-sampled result. */
        if (plain_pixels[i] == UINT32_C(0xffffffff) &&
            pixels[i] != UINT32_C(0xffffffff)) ++fractional;
        if (pixels[i] != UINT32_C(0xffffffff)) {
            assert(isfinite(depth[i]));
            assert(depth[i] >= 0.0f && depth[i] < 1.0f);
        } else assert(isinf(depth[i]));
    }
    assert(foreground > 100u);
    assert(changed > 20u);
    assert(fractional > 4u);

    /* Caller scratch is recycled, but each AA call clears stale depth/color.
     * A repeated identical render must produce exactly identical bytes. */
    uint32_t *saved = malloc(count * sizeof *saved);
    float *saved_depth = malloc(count * sizeof *saved_depth);
    assert(saved && saved_depth);
    memcpy(saved, pixels, count * sizeof *saved);
    memcpy(saved_depth, depth, count * sizeof *saved_depth);
    for (size_t i = 0; i < high_count; ++i) {
        high_pixels[i] = UINT32_C(0x00000000);
        high_depth[i] = -3.0f;
    }
    assert(rough_draw_surface_aa2(&aa, &high, shape, yaw, distance) == 0);
    assert(memcmp(saved, pixels, count * sizeof *saved) == 0);
    assert(memcmp(saved_depth, depth, count * sizeof *saved_depth) == 0);

    assert(rough_draw_surface_aa2(NULL, &high, shape, yaw, distance) == -1);
    assert(rough_draw_surface_aa2(&aa, NULL, shape, yaw, distance) == -1);
    assert(rough_draw_surface_aa2(&aa, &high,
                                   (rough_surface_shape)999,
                                   yaw, distance) == -1);
    assert(rough_draw_surface_aa2(&aa, &high, shape, NAN, distance) == -1);

    free(saved);
    free(saved_depth);
    free(pixels);
    free(plain_pixels);
    free(depth);
    free(plain_depth);
    free(high_pixels);
    free(high_depth);
}

int main(void) {
    test_explicit_edge_coverage_and_depth();
    test_real_math_surface(ROUGH_SURFACE_TORUS, 37.0, 5.2);
    test_real_math_surface(ROUGH_SURFACE_ENNEPER, 25.0, 5.2);
    test_real_math_surface(ROUGH_SURFACE_ENNEPER, 70.0, 2.4);
    puts("AA2_DEPTH_RESOLVE_PASS 2x2 real coverage, 4 depths, transparency, near zoom");
    return 0;
}
