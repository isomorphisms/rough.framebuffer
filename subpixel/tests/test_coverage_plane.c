#include "subpixel/coverage_plane.h"
#include "subpixel_framebuffer.h"

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "coverage plane failure at line %d: %s\n", __LINE__, #condition); \
    return 1; } } while (0)

/* Separate, literal expected words, including untouched padding. The two
 * source and destination extents differ by one pixel and start at negative
 * global coordinates. Each operation gets a fresh destination. */
static int test_mask_algebra(void) {
    const uint64_t pad_a = UINT64_C(0xfeedfacefeedface);
    const uint64_t pad_b = UINT64_C(0x1234567890abcdef);
    uint64_t original[8] = {1, 2, 4, pad_a, 8, 5, 3, pad_b};
    uint64_t destination_words[8];
    uint64_t source_words[6] = {
        3, 4, UINT64_C(0xaaaaaaaaaaaaaaaa),
        6, 1, UINT64_C(0x5555555555555555)
    };
    uint64_t source_original[6];
    memcpy(source_original, source_words, sizeof source_words);
    cp_coverage_plane destination = {
        destination_words, 8, 4, 3, 2, -2, -1
    };
    cp_coverage_plane source = {source_words, 6, 3, 2, 2, -1, -1};
    const cp_mask_operation operations[4] = {
        CP_MASK_UNION, CP_MASK_INTERSECTION, CP_MASK_DIFFERENCE,
        CP_MASK_SYMMETRIC_DIFFERENCE
    };
    const uint64_t expected[4][8] = {
        {1, 3, 4, pad_a, 8, 7, 3, pad_b},
        {0, 2, 4, pad_a, 0, 4, 1, pad_b},
        {1, 0, 0, pad_a, 8, 1, 2, pad_b},
        {1, 1, 0, pad_a, 8, 3, 2, pad_b}
    };
    for (unsigned operation = 0; operation < 4; ++operation) {
        memcpy(destination_words, original, sizeof original);
        CHECK(cp_coverage_plane_combine(&destination, &source,
            operations[operation]) == CP_OK);
        CHECK(memcmp(destination_words, expected[operation],
            sizeof destination_words) == 0);
        CHECK(memcmp(source_words, source_original,
            sizeof source_words) == 0);
        if (operations[operation] == CP_MASK_UNION) {
            CHECK(cp_coverage_plane_combine(&destination, &source,
                CP_MASK_UNION) == CP_OK);
            CHECK(memcmp(destination_words, expected[operation],
                sizeof destination_words) == 0);
        }
    }

    /* Equal grayscale sample counts are NOT enough to combine geometry:
     * the disjoint 4-bit masks 0x0f and 0xf0 have union 0xff. */
    memcpy(destination_words, original, sizeof original);
    destination_words[1] = UINT64_C(0x0f);
    source_words[0] = UINT64_C(0xf0);
    CHECK(cp_coverage_plane_combine(&destination, &source,
        CP_MASK_UNION) == CP_OK);
    CHECK(destination_words[1] == UINT64_C(0xff));
    source_words[0] = source_original[0];

    /* Empty geometric intersection does not reach beyond destination. */
    source.origin_x = 99;
    source.origin_y = 99;
    memcpy(destination_words, original, sizeof original);
    CHECK(cp_coverage_plane_combine(&destination, &source,
        CP_MASK_DIFFERENCE) == CP_OK);
    CHECK(memcmp(destination_words, original, sizeof original) == 0);
    CHECK(cp_coverage_plane_combine(&destination, &source,
        CP_MASK_INTERSECTION) == CP_OK);
    for (unsigned row = 0; row < 2; ++row)
        for (unsigned column = 0; column < 3; ++column)
            CHECK(destination_words[row * 4 + column] == 0);
    CHECK(destination_words[3] == pad_a &&
        destination_words[7] == pad_b);
    source.origin_x = -1;
    source.origin_y = -1;

    /* Every refusal is transactional, even when the source and destination
     * use shifted pointers into the same allocation. */
    memcpy(destination_words, original, sizeof original);
    CHECK(cp_coverage_plane_combine(&destination, &source,
        (cp_mask_operation)99) == CP_INVALID);
    CHECK(memcmp(destination_words, original, sizeof original) == 0);
    CHECK(cp_coverage_plane_combine(&destination, &destination,
        CP_MASK_UNION) == CP_INVALID);
    CHECK(memcmp(destination_words, original, sizeof original) == 0);
    cp_coverage_plane alias = source;
    alias.occupied = destination_words + 1;
    alias.capacity_words = 7;
    CHECK(cp_coverage_plane_combine(&destination, &alias,
        CP_MASK_SYMMETRIC_DIFFERENCE) == CP_INVALID);
    CHECK(memcmp(destination_words, original, sizeof original) == 0);

    cp_coverage_plane malformed = source;
    malformed.capacity_words = 4;
    CHECK(cp_coverage_plane_combine(&destination, &malformed,
        CP_MASK_UNION) == CP_INVALID);
    CHECK(memcmp(destination_words, original, sizeof original) == 0);
    malformed = source;
    malformed.stride_words = SIZE_MAX;
    CHECK(cp_coverage_plane_combine(&destination, &malformed,
        CP_MASK_UNION) == CP_OVERFLOW);
    CHECK(memcmp(destination_words, original, sizeof original) == 0);
    malformed = destination;
    malformed.stride_words = 2;
    CHECK(cp_coverage_plane_combine(&malformed, &source,
        CP_MASK_INTERSECTION) == CP_INVALID);
    CHECK(memcmp(destination_words, original, sizeof original) == 0);
    CHECK(cp_coverage_plane_combine(NULL, &source,
        CP_MASK_UNION) == CP_INVALID);
    CHECK(memcmp(source_words, source_original, sizeof source_words) == 0);
    puts("Subpixel 64-bit coverage mask algebra and refusal tests: PASS");
    return 0;
}

int main(void) {
    CHECK(test_mask_algebra() == 0);
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
