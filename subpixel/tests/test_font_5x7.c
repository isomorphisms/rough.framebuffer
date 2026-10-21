#include "../examples/font_5x7.h"

#include <float.h>
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

static double cubic_endpoint_turn(const sf57_segment *segment, int at_end) {
    sf57_point tangent, bending;
    if (at_end) {
        tangent.x = segment->point[3].x - segment->point[2].x;
        tangent.y = segment->point[3].y - segment->point[2].y;
        bending.x = segment->point[3].x - 2.0 * segment->point[2].x +
                    segment->point[1].x;
        bending.y = segment->point[3].y - 2.0 * segment->point[2].y +
                    segment->point[1].y;
    } else {
        tangent.x = segment->point[1].x - segment->point[0].x;
        tangent.y = segment->point[1].y - segment->point[0].y;
        bending.x = segment->point[2].x - 2.0 * segment->point[1].x +
                    segment->point[0].x;
        bending.y = segment->point[2].y - 2.0 * segment->point[1].y +
                    segment->point[0].y;
    }
    return tangent.x * bending.y - tangent.y * bending.x;
}

/* Derive connectivity from the individual sample bits, rather than trusting
 * the vector's closed flag or a thresholded 5x7 count panel. Four-neighbour
 * background connectivity makes a counter an empty component which cannot
 * reach the exterior of the 40x56 lattice. This is sampled topology only. */
static int test_sample_topology(const uint64_t masks[SF57_CELL_COUNT],
                                unsigned expected_counters) {
    unsigned char occupied[SF57_FINE_HEIGHT][SF57_FINE_WIDTH] = {{0}};
    unsigned char seen[SF57_FINE_HEIGHT][SF57_FINE_WIDTH] = {{0}};
    unsigned queue[SF57_FINE_WIDTH * SF57_FINE_HEIGHT];
    for (unsigned fine_y = 0; fine_y < SF57_FINE_HEIGHT; ++fine_y)
        for (unsigned fine_x = 0; fine_x < SF57_FINE_WIDTH; ++fine_x) {
            unsigned cell = (fine_y / 8U) * SF57_COLUMNS + fine_x / 8U;
            unsigned bit = (fine_y % 8U) * 8U + fine_x % 8U;
            occupied[fine_y][fine_x] =
                (unsigned char)((masks[cell] >> bit) & UINT64_C(1));
        }
    unsigned counters = 0;
    for (unsigned fine_y = 0; fine_y < SF57_FINE_HEIGHT; ++fine_y)
        for (unsigned fine_x = 0; fine_x < SF57_FINE_WIDTH; ++fine_x) {
            if (occupied[fine_y][fine_x] || seen[fine_y][fine_x]) continue;
            unsigned read_index = 0, write_index = 0;
            int reaches_exterior = 0;
            queue[write_index++] = fine_y * SF57_FINE_WIDTH + fine_x;
            seen[fine_y][fine_x] = 1;
            while (read_index < write_index) {
                unsigned sample = queue[read_index++];
                unsigned x = sample % SF57_FINE_WIDTH;
                unsigned y = sample / SF57_FINE_WIDTH;
                if (x == 0 || y == 0 || x + 1 == SF57_FINE_WIDTH ||
                    y + 1 == SF57_FINE_HEIGHT)
                    reaches_exterior = 1;
                const int adjacent_x[] = {-1, 1, 0, 0};
                const int adjacent_y[] = {0, 0, -1, 1};
                for (unsigned direction = 0; direction < 4; ++direction) {
                    int next_x = (int)x + adjacent_x[direction];
                    int next_y = (int)y + adjacent_y[direction];
                    if (next_x < 0 || next_x >= SF57_FINE_WIDTH ||
                        next_y < 0 || next_y >= SF57_FINE_HEIGHT ||
                        occupied[next_y][next_x] || seen[next_y][next_x])
                        continue;
                    seen[next_y][next_x] = 1;
                    CHECK(write_index < sizeof queue / sizeof queue[0]);
                    queue[write_index++] = (unsigned)next_y * SF57_FINE_WIDTH +
                                           (unsigned)next_x;
                }
            }
            if (!reaches_exterior) ++counters;
        }
    CHECK(counters == expected_counters);
    return 0;
}

static int test_glyph(char name, unsigned ordinal) {
    const sf57_glyph *glyph = sf57_find_glyph(name);
    CHECK(glyph != NULL);
    CHECK(sf57_validate_glyph(glyph));
    CHECK(glyph->advance == 6.0 && glyph->bearing_x == 0.0 &&
          glyph->baseline == 6.5);
    CHECK(glyph->segments[0].kind == SF57_CUBIC);
    if (name == 'O')
        CHECK(glyph->subpath_count == 1 && glyph->subpaths[0].closed);
    if (name == 'C' || name == 'S')
        CHECK(glyph->subpath_count == 1 && !glyph->subpaths[0].closed);
    if (name == '8')
        CHECK(glyph->subpath_count == 2 && glyph->subpaths[0].closed &&
              glyph->subpaths[1].closed);
    if (name == 'S') {
        CHECK(glyph->segment_count == 3);
        CHECK(cubic_endpoint_turn(&glyph->segments[1], 0) < 0.0);
        CHECK(cubic_endpoint_turn(&glyph->segments[1], 1) > 0.0);
    }
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
    for (size_t cell = 0; cell < (size_t)SF57_CELL_COUNT; ++cell) {
        CHECK((unsigned)forward_counts[cell] == popcount64(forward[cell]));
        if (forward_counts[cell] != 0) ++occupied;
        if (forward_counts[cell] > 0 && forward_counts[cell] < 64) ++fractional;
    }
    CHECK(occupied > 5);
    CHECK(fractional > 3);
    CHECK(test_sample_topology(forward, name == 'O' ? 1U :
                                       name == '8' ? 2U : 0U) == 0);
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
    invalid = *source;
    invalid.stroke_width = 0.0;
    CHECK(!sf57_validate_glyph(&invalid));
    CHECK(!sf57_sample_masks(&invalid, 0, masks, counts));
    CHECK(memcmp(masks, before_masks, sizeof masks) == 0);
    CHECK(memcmp(counts, before_counts, sizeof counts) == 0);
    invalid.stroke_width = DBL_MAX;
    CHECK(!sf57_validate_glyph(&invalid));
    CHECK(!sf57_sample_masks(&invalid, 0, masks, counts));
    CHECK(memcmp(masks, before_masks, sizeof masks) == 0);
    CHECK(memcmp(counts, before_counts, sizeof counts) == 0);
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
