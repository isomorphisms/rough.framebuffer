#include "font_5x7.h"

#include <inttypes.h>
#include <stdio.h>

static int dump_glyph(char name) {
    const sf57_glyph *glyph = sf57_find_glyph(name);
    uint64_t masks[SF57_CELL_COUNT];
    unsigned char counts[SF57_CELL_COUNT];
    if (!glyph || !sf57_sample_masks(glyph, 0, masks, counts)) return 0;

    printf("glyph %c: 40x56 midpoint samples; | marks 5x7 cell boundaries\n", name);
    for (unsigned fine_y = 0; fine_y < SF57_FINE_HEIGHT; ++fine_y) {
        unsigned row = fine_y / SF57_SAMPLES_PER_AXIS;
        unsigned sample_y = fine_y % SF57_SAMPLES_PER_AXIS;
        if (sample_y == 0) puts("+--------+--------+--------+--------+--------+");
        putchar('|');
        for (unsigned column = 0; column < SF57_COLUMNS; ++column) {
            uint64_t mask = masks[(size_t)row * SF57_COLUMNS + column];
            for (unsigned sample_x = 0; sample_x < SF57_SAMPLES_PER_AXIS; ++sample_x)
                putchar((mask & (UINT64_C(1) <<
                    (SF57_SAMPLES_PER_AXIS * sample_y + sample_x))) ? '#' : '.');
            putchar('|');
        }
        putchar('\n');
    }
    puts("+--------+--------+--------+--------+--------+");
    puts("counts and masks, row-major cells:");
    for (unsigned row = 0; row < SF57_ROWS; ++row) {
        for (unsigned column = 0; column < SF57_COLUMNS; ++column) {
            size_t cell = (size_t)row * SF57_COLUMNS + column;
            printf("%2u:%016" PRIx64 "%c", counts[cell], masks[cell],
                   column + 1 == SF57_COLUMNS ? '\n' : ' ');
        }
    }
    putchar('\n');
    return 1;
}

int main(void) {
    const char names[] = {'O', 'C', 'S', '8'};
    for (unsigned index = 0; index < sizeof names; ++index)
        if (!dump_glyph(names[index])) return 1;
    return 0;
}
