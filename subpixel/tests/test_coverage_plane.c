#include "subpixel/coverage_plane.h"
#include "subpixel_framebuffer.h"

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "coverage plane failure at line %d: %s\n", __LINE__, #condition); \
    return 1; } } while (0)

int main(void) {
    uint64_t words[20] = {0};
    cp_coverage_plane plane = {words, 20, 5, 4, 3, -1, -1};
    cp_curve_patch line;
    CHECK(cp_patch_from_segment(&line, (cp_vec2){-1, 0},
        (cp_vec2){2, 0}, 0.25) == CP_OK);
    CHECK(cp_coverage_plane_accumulate_3x3(&plane, &line, 0, 0) == CP_OK);
    /* Independent horizontal band: midpoint sample y lies in [-1/4,1/4]
     * exactly for sample rows 2..5. At x=0 every sample is in the domain. */
    CHECK(words[6] == UINT64_C(0x0000ffffffff0000));
    CHECK(words[4] == 0 && words[9] == 0 && words[14] == 0);
    uint64_t first[20];
    memcpy(first, words, sizeof first);
    CHECK(cp_coverage_plane_accumulate_3x3(&plane, &line, 0, 0) == CP_OK);
    CHECK(memcmp(first, words, sizeof first) == 0);
    cp_curve_patch invalid = line;
    invalid.a2 = NAN;
    CHECK(cp_coverage_plane_accumulate_5x5(&plane, &invalid, 0, 0) == CP_INVALID);
    CHECK(memcmp(first, words, sizeof first) == 0);
    cp_coverage_plane short_buffer = plane;
    short_buffer.capacity_words = 13;
    CHECK(cp_coverage_plane_accumulate_3x3(&short_buffer, &line, 0, 0) == CP_INVALID);
    CHECK(memcmp(first, words, sizeof first) == 0);
    cp_coverage_plane overflow = plane;
    overflow.origin_x = INT_MAX;
    CHECK(cp_coverage_plane_validate(&overflow) == CP_OVERFLOW);
    overflow = plane;
    overflow.stride_words = SIZE_MAX;
    CHECK(cp_coverage_plane_validate(&overflow) == CP_OVERFLOW);

    uint64_t one_word = UINT64_C(0x0000ffffffff0000);
    uint64_t overlap_words[9] = {0};
    cp_coverage_plane overlap = {overlap_words, 9, 3, 3, 3, -1, -1};
    cp_curve_patch lower, upper;
    CHECK(cp_patch_from_segment(&lower, (cp_vec2){-1, -0.1875},
        (cp_vec2){1, -0.1875}, 0.0625) == CP_OK);
    CHECK(cp_patch_from_segment(&upper, (cp_vec2){-1, 0.1875},
        (cp_vec2){1, 0.1875}, 0.0625) == CP_OK);
    CHECK(cp_coverage_plane_accumulate_3x3(&overlap, &lower, 0, 0) == CP_OK);
    CHECK(cp_coverage_plane_accumulate_5x5(&overlap, &upper, 0, 0) == CP_OK);
    CHECK(overlap_words[4] == UINT64_C(0x0000ff0000ff0000));
    cp_coverage_plane one = {&one_word, 1, 1, 1, 1, 1, 0};
    uint32_t pixels[4] = {UINT32_C(0xffabcdef), UINT32_C(0xffffffff),
                          UINT32_C(0x12345678), UINT32_C(0x87654321)};
    rough_framebuffer framebuffer = {
        .pixels = pixels, .width = 2, .height = 1, .stride_pixels = 4
    };
    CHECK(rough_blend_subpixel_coverage(&framebuffer, &one,
        UINT32_C(0xff000000), 1.0) == 0);
    CHECK(pixels[0] == UINT32_C(0xffabcdef));
    CHECK(pixels[1] == UINT32_C(0xff808080));
    CHECK(pixels[2] == UINT32_C(0x12345678) && pixels[3] == UINT32_C(0x87654321));
    pixels[1] = UINT32_C(0x800000ff);
    CHECK(rough_blend_subpixel_coverage(&framebuffer, &one,
        UINT32_C(0xffff0000), 1.0) == 0);
    CHECK(pixels[1] == UINT32_C(0xc0aa0055));
    /* Exactly 16.5 alpha bytes must round upward. The old normalized-double
     * calculation produced 16 after /255 and multiplication back by 255. */
    one_word = (UINT64_C(1) << 33) - 1;
    pixels[1] = UINT32_C(0x00f4e9d8);
    CHECK(rough_blend_subpixel_coverage(&framebuffer, &one,
        UINT32_C(0x40ff0000), 0.5) == 0);
    CHECK(pixels[1] == UINT32_C(0x11ff0000));
    uint32_t before[4];
    memcpy(before, pixels, sizeof before);
    CHECK(rough_blend_subpixel_coverage(&framebuffer, &one,
        UINT32_C(0xffffffff), NAN) == -1);
    CHECK(memcmp(before, pixels, sizeof before) == 0);
    CHECK(rough_blend_subpixel_coverage(&framebuffer, &one,
        UINT32_C(0x00ffffff), 1.0) == 0);
    CHECK(memcmp(before, pixels, sizeof before) == 0);
    one.origin_x = -1;
    CHECK(rough_blend_subpixel_coverage(&framebuffer, &one,
        UINT32_C(0xffffffff), 1.0) == 0);
    CHECK(memcmp(before, pixels, sizeof before) == 0);
    puts("Subpixel caller-owned occupancy and actual framebuffer composition: PASS");
    return 0;
}
