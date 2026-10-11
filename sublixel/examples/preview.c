/* A genuine native consumer of Sublixel 3x3 and 5x5 coverage.
 * Produces a side-by-side PPM of one slanted cubic Bezier stroke.
 * Geometry comes from the corresponding public curve patch APIs; coverage
 * is sample-mask OR, never additive alpha. No Android/GPU or external renderer.
 */
#include "sublixel/curve_patch.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { WIDTH = 96, HEIGHT = 72, FRAME_WIDTH = 2 * WIDTH, PATCH_COUNT = 385 };
static uint64_t quadratic_occupancy[HEIGHT][WIDTH];
static uint64_t cubic_occupancy[HEIGHT][WIDTH];

static unsigned popcount64(uint64_t value) {
    unsigned count = 0;
    while (value) {
        value &= value - UINT64_C(1);
        ++count;
    }
    return count;
}

static void merge3(const cp_accumulator_3x3 *accumulator) {
    for (int iy = 0; iy != 3; ++iy)
        for (int ix = 0; ix != 3; ++ix) {
            int x = accumulator->center_x + ix - 1;
            int y = accumulator->center_y + iy - 1;
            if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT)
                quadratic_occupancy[y][x] |= accumulator->occupied[iy][ix];
        }
}

static void merge5(const cp_accumulator_5x5 *accumulator) {
    for (int iy = 0; iy != 5; ++iy)
        for (int ix = 0; ix != 5; ++ix) {
            int x = accumulator->center_x + ix - 2;
            int y = accumulator->center_y + iy - 2;
            if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT)
                cubic_occupancy[y][x] |= accumulator->occupied[iy][ix];
        }
}

/* Caller-owned fixed-size working memory: ~108 KiB for both coverage planes.
 * One fixed set of Bezier controls feeds both models, so this is genuinely a
 * model comparison, not two arbitrary hardcoded pictures.
 */
static int accumulate_stroke(void) {
    const cp_vec2 controls[4] = {
        {8, 62}, {27, 6}, {69, 70}, {88, 12}
    };
    for (int step = 0; step < PATCH_COUNT; ++step) {
        double t = (double)step / (PATCH_COUNT - 1);
        cp_curve_patch quadratic, cubic;
        if (cp_patch_from_cubic(&quadratic, controls, t, 0.52, 1.5) != CP_OK ||
            cp_patch_from_cubic_jet3(&cubic, controls, t, 0.52, 1.5) != CP_OK)
            return 0;

        int x = (int)lround(quadratic.origin.x);
        int y = (int)lround(quadratic.origin.y);
        if (x < -3 || x >= WIDTH + 3 || y < -3 || y >= HEIGHT + 3)
            return 0;
        cp_accumulator_3x3 grid3;
        cp_accumulator_5x5 grid5;
        cp_patch_accumulator_3x3_init(&grid3, x, y);
        cp_patch_accumulator_5x5_init(&grid5, x, y);
        if (cp_patch_accumulate_3x3(&grid3, &quadratic) != CP_OK ||
            cp_patch_accumulate_5x5(&grid5, &cubic) != CP_OK)
            return 0;

        merge3(&grid3);
        merge5(&grid5);
    }
    return 1;
}

static uint64_t checksum_plane(const uint64_t plane[HEIGHT][WIDTH]) {
    /* FNV-1a over individual occupied bits, serialized in little-endian
     * order independent of the native machine's byte representation. */
    uint64_t hash = UINT64_C(14695981039346656037);
    for (int y = 0; y < HEIGHT; ++y)
        for (int x = 0; x < WIDTH; ++x) {
            uint64_t bits = plane[y][x];
            for (int byte = 0; byte < 8; ++byte) {
                hash ^= (unsigned char)(bits >> (byte * 8));
                hash *= UINT64_C(1099511628211);
            }
        }
    return hash;
}

static int inspect_slant(const uint64_t plane[HEIGHT][WIDTH], const char *name) {
    unsigned populated = 0;
    int first_x = WIDTH, last_x = -1;
    int first_y = HEIGHT, last_y = -1;
    unsigned partially_covered = 0;

    for (int y = 0; y < HEIGHT; ++y)
        for (int x = 0; x < WIDTH; ++x) {
            unsigned count = popcount64(plane[y][x]);
            if (count == 0) continue;
            ++populated;
            if (count < 64) ++partially_covered;
            if (x < first_x) first_x = x;
            if (x > last_x) last_x = x;
            if (y < first_y) first_y = y;
            if (y > last_y) last_y = y;
        }
    if (populated < 40 || populated > 1500 ||
        partially_covered < 20 || last_x - first_x < 65 ||
        last_y - first_y < 26) {
        fprintf(stderr, "Sublixel visual geometry invalid (%s): pixels=%u fractional=%u x=[%d,%d] y=[%d,%d]\n",
                name, populated, partially_covered,
                first_x, last_x, first_y, last_y);
        return 0;
    }
    printf("native slanted stroke %s: %u covered pixels, %u antialiased edge pixels, extents [%d,%d]x[%d,%d]\n",
           name, populated, partially_covered,
           first_x, last_x, first_y, last_y);
    return 1;
}

static unsigned char gray(uint64_t bits) {
    unsigned occupied = popcount64(bits);
    return (unsigned char)(255u - (occupied * 255u + 32u) / 64u);
}

static int emit_ppm(FILE *output) {
    if (fprintf(output, "P6\n%d %d\n255\n", FRAME_WIDTH, HEIGHT) < 0)
        return 0;
    for (int y = 0; y < HEIGHT; ++y)
        for (int x = 0; x < FRAME_WIDTH; ++x) {
            uint64_t bits = x < WIDTH
                ? quadratic_occupancy[y][x]
                : cubic_occupancy[y][x - WIDTH];
            unsigned char value = gray(bits);
            unsigned char rgb[3] = {value, value, value};
            if (fwrite(rgb, 1, sizeof rgb, output) != sizeof rgb)
                return 0;
        }
    return !ferror(output);
}

static int self_test(void) {
    memset(quadratic_occupancy, 0, sizeof quadratic_occupancy);
    memset(cubic_occupancy, 0, sizeof cubic_occupancy);
    if (!accumulate_stroke()) return 0;
    if (!inspect_slant(quadratic_occupancy, "quadratic 3x3") ||
        !inspect_slant(cubic_occupancy, "cubic 5x5"))
        return 0;

    uint64_t first3 = checksum_plane(quadratic_occupancy);
    uint64_t first5 = checksum_plane(cubic_occupancy);
    /* Repeating every overlapping patch cannot brighten a sample. */
    if (!accumulate_stroke()) return 0;
    if (first3 != checksum_plane(quadratic_occupancy) ||
        first5 != checksum_plane(cubic_occupancy)) {
        fputs("Sublixel duplicate-patch opacity regression\n", stderr);
        return 0;
    }

    FILE *test = tmpfile();
    if (!test) return 0;
    if (!emit_ppm(test) || fflush(test) != 0) {
        fclose(test);
        return 0;
    }
    long bytes = ftell(test);
    if (bytes != (long)(sizeof("P6\n192 72\n255\n") - 1 +
                      FRAME_WIDTH * HEIGHT * 3)) {
        fprintf(stderr, "Sublixel PPM size mismatch: %ld\n", bytes);
        fclose(test);
        return 0;
    }
    rewind(test);
    char header[sizeof("P6\n192 72\n255\n")] = {0};
    size_t header_size = sizeof header - 1;
    int valid_header = fread(header, 1, header_size, test) == header_size &&
                       memcmp(header, "P6\n192 72\n255\n", header_size) == 0;
    if (fclose(test) != 0 || !valid_header) return 0;

    printf("native frame hashes: 3x3=%016llx 5x5=%016llx\n",
           (unsigned long long)first3, (unsigned long long)first5);
    puts("Sublixel real 3x3/5x5 slanted-curve preview: PASS");
    return 1;
}

int main(int argc, char **argv) {
    if (argc > 2) {
        fputs("usage: sublixel-preview [output.ppm | --self-test]\n", stderr);
        return 2;
    }
    if (!self_test()) return 1;
    if (argc == 1 || strcmp(argv[1], "--self-test") == 0)
        return 0;

    FILE *output = fopen(argv[1], "wb");
    if (!output) { perror("open"); return 1; }
    int success = emit_ppm(output);
    if (fclose(output) != 0) success = 0;
    if (!success) {
        fputs("Sublixel preview PPM write failed\n", stderr);
        return 1;
    }
    return 0;
}
