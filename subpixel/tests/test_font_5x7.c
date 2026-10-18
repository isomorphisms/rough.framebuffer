#include "../examples/font_5x7.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "check failed at %s:%d: %s\n", __FILE__, __LINE__, #condition); \
    return 1; \
} } while (0)

static const char *expected_rows[][SF57_ROWS] = {
    {".###.", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."},
    {".###.", "#...#", "#....", "#....", "#....", "#...#", ".###."},
    {".###.", "#...#", "#....", ".###.", "....#", "#...#", ".###."},
    {".###.", "#...#", "#...#", ".###.", "#...#", "#...#", ".###."}
};

static unsigned popcount64(uint64_t value) {
    unsigned count = 0;
    while (value != 0) { value &= value - UINT64_C(1); ++count; }
    return count;
}

static int test_glyph(char name, unsigned ordinal) {
    const sf57_glyph *glyph = sf57_find_glyph(name);
    CHECK(glyph != NULL);
    CHECK(sf57_validate_glyph(glyph));
    CHECK(glyph->advance == 6.0 && glyph->bearing_x == 0.0 &&
          glyph->baseline == 6.5);
    for (unsigned row = 0; row < SF57_ROWS; ++row)
        for (unsigned column = 0; column < SF57_COLUMNS; ++column)
            CHECK(sf57_bitmap_cell(glyph, column, row) ==
                  (expected_rows[ordinal][row][column] == '#'));

    uint64_t forward[SF57_CELL_COUNT], reverse[SF57_CELL_COUNT];
    unsigned char forward_counts[SF57_CELL_COUNT], reverse_counts[SF57_CELL_COUNT];
    CHECK(sf57_sample_masks(glyph, 0, forward, forward_counts));
    CHECK(sf57_sample_masks(glyph, 1, reverse, reverse_counts));
    CHECK(memcmp(forward, reverse, sizeof forward) == 0);
    CHECK(memcmp(forward_counts, reverse_counts, sizeof forward_counts) == 0);
    unsigned fractional = 0, occupied = 0;
    for (size_t cell = 0; cell < SF57_CELL_COUNT; ++cell) {
        CHECK(forward_counts[cell] == popcount64(forward[cell]));
        if (forward_counts[cell] != 0) ++occupied;
        if (forward_counts[cell] > 0 && forward_counts[cell] < 64) ++fractional;
    }
    CHECK(occupied > 5);
    CHECK(fractional > 3);
    return 0;
}

static int test_invalid_is_atomic(void) {
    const sf57_glyph *source = sf57_find_glyph('C');
    CHECK(source != NULL);
    sf57_glyph invalid = *source;
    sf57_subpath path = source->subpaths[0];
    path.closed = 1; /* C must not acquire a false closing segment. */
    invalid.subpaths = &path;
    CHECK(!sf57_validate_glyph(&invalid));

    uint64_t masks[SF57_CELL_COUNT];
    unsigned char counts[SF57_CELL_COUNT];
    memset(masks, 0xa5, sizeof masks);
    memset(counts, 0x5a, sizeof counts);
    uint64_t before_masks[SF57_CELL_COUNT];
    unsigned char before_counts[SF57_CELL_COUNT];
    memcpy(before_masks, masks, sizeof masks);
    memcpy(before_counts, counts, sizeof counts);
    CHECK(!sf57_sample_masks(&invalid, 0, masks, counts));
    CHECK(memcmp(masks, before_masks, sizeof masks) == 0);
    CHECK(memcmp(counts, before_counts, sizeof counts) == 0);

    sf57_segment bad_segment = source->segments[0];
    bad_segment.point[1].x = NAN;
    invalid = *source;
    invalid.segments = &bad_segment;
    invalid.segment_count = 1;
    path = (sf57_subpath){0, 1, 0};
    invalid.subpaths = &path;
    CHECK(!sf57_validate_glyph(&invalid));
    CHECK(!sf57_sample_masks(source, 2, masks, counts));
    return 0;
}

int main(void) {
    const char names[] = {'O', 'C', 'S', '8'};
    for (unsigned index = 0; index < sizeof names; ++index)
        CHECK(test_glyph(names[index], index) == 0);
    CHECK(sf57_find_glyph('0') == NULL);
    CHECK(sf57_bitmap_cell(sf57_find_glyph('O'), SF57_COLUMNS, 0) == -1);
    CHECK(test_invalid_is_atomic() == 0);
    puts("Subpixel 5x7 exact masks + continuous masters + 40x56 sampling: PASS");
    return 0;
}
