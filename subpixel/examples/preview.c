/* A genuine native consumer of Subpixel 3x3 and 5x5 coverage.
 * Produces a side-by-side PPM of one slanted cubic Bezier stroke.
 * Geometry comes from the corresponding public curve patch APIs; coverage
 * is sample-mask OR, never additive alpha. No Android/GPU or external renderer.
 */
#include "subpixel/curve_patch.h"
#include "subpixel/coverage_plane.h"
#include "subpixel_framebuffer.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { WIDTH = 96, HEIGHT = 72, FRAME_WIDTH = 2 * WIDTH, PATCH_COUNT = 385 };
static uint64_t quadratic_occupancy[WIDTH * HEIGHT];
static uint64_t cubic_occupancy[WIDTH * HEIGHT];
static uint32_t quadratic_pixels[WIDTH * HEIGHT];
static uint32_t cubic_pixels[WIDTH * HEIGHT];
static cp_coverage_plane quadratic_plane = {
    quadratic_occupancy, WIDTH * HEIGHT, WIDTH, WIDTH, HEIGHT, 0, 0
};
static cp_coverage_plane cubic_plane = {
    cubic_occupancy, WIDTH * HEIGHT, WIDTH, WIDTH, HEIGHT, 0, 0
};

static unsigned popcount64(uint64_t value) {
    unsigned count = 0;
    while (value) {
        value &= value - UINT64_C(1);
        ++count;
    }
    return count;
}

/* Caller-owned fixed-size working memory: ~162 KiB for coverage and actual color planes.
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
        if (cp_coverage_plane_accumulate_3x3(&quadratic_plane,
                &quadratic, x, y) != CP_OK ||
            cp_coverage_plane_accumulate_5x5(&cubic_plane,
                &cubic, x, y) != CP_OK)
            return 0;
    }
    return 1;
}

static uint64_t checksum_plane(const uint64_t *plane) {
    /* FNV-1a over individual occupied bits, serialized in little-endian
     * order independent of the native machine's byte representation. */
    uint64_t hash = UINT64_C(14695981039346656037);
    for (int y = 0; y < HEIGHT; ++y)
        for (int x = 0; x < WIDTH; ++x) {
            uint64_t bits = plane[(size_t)y * WIDTH + x];
            for (int byte = 0; byte < 8; ++byte) {
                hash ^= (unsigned char)(bits >> (byte * 8));
                hash *= UINT64_C(1099511628211);
            }
        }
    return hash;
}

static int inspect_slant(const uint64_t *plane, const char *name) {
    unsigned populated = 0;
    int first_x = WIDTH, last_x = -1;
    int first_y = HEIGHT, last_y = -1;
    unsigned partially_covered = 0;

    for (int y = 0; y < HEIGHT; ++y)
        for (int x = 0; x < WIDTH; ++x) {
            unsigned count = popcount64(plane[(size_t)y * WIDTH + x]);
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
        fprintf(stderr, "Subpixel visual geometry invalid (%s): pixels=%u fractional=%u x=[%d,%d] y=[%d,%d]\n",
                name, populated, partially_covered,
                first_x, last_x, first_y, last_y);
        return 0;
    }
    printf("native slanted stroke %s: %u covered pixels, %u antialiased edge pixels, extents [%d,%d]x[%d,%d]\n",
           name, populated, partially_covered,
           first_x, last_x, first_y, last_y);
    return 1;
}

static int compose_framebuffers(void) {
    for (size_t index = 0; index < WIDTH * HEIGHT; ++index) {
        quadratic_pixels[index] = UINT32_C(0xffffffff);
        cubic_pixels[index] = UINT32_C(0xffffffff);
    }
    rough_framebuffer quadratic_target = {
        .pixels = quadratic_pixels, .width = WIDTH, .height = HEIGHT,
        .stride_pixels = WIDTH
    };
    rough_framebuffer cubic_target = {
        .pixels = cubic_pixels, .width = WIDTH, .height = HEIGHT,
        .stride_pixels = WIDTH
    };
    return rough_blend_subpixel_coverage(&quadratic_target, &quadratic_plane,
               UINT32_C(0xff000000), 1.0) == 0 &&
           rough_blend_subpixel_coverage(&cubic_target, &cubic_plane,
               UINT32_C(0xff000000), 1.0) == 0;
}

static int emit_ppm(FILE *output) {
    if (fprintf(output, "P6\n%d %d\n255\n", FRAME_WIDTH, HEIGHT) < 0)
        return 0;
    for (int y = 0; y < HEIGHT; ++y)
        for (int x = 0; x < FRAME_WIDTH; ++x) {
            uint32_t pixel = x < WIDTH
                ? quadratic_pixels[(size_t)y * WIDTH + x]
                : cubic_pixels[(size_t)y * WIDTH + x - WIDTH];
            unsigned char rgb[3] = {
                (unsigned char)(pixel >> 16), (unsigned char)(pixel >> 8),
                (unsigned char)pixel
            };
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
        fputs("Subpixel duplicate-patch opacity regression\n", stderr);
        return 0;
    }

    if (!compose_framebuffers()) return 0;

    FILE *test = tmpfile();
    if (!test) return 0;
    if (!emit_ppm(test) || fflush(test) != 0) {
        fclose(test);
        return 0;
    }
    long bytes = ftell(test);
    if (bytes != (long)(sizeof("P6\n192 72\n255\n") - 1 +
                      FRAME_WIDTH * HEIGHT * 3)) {
        fprintf(stderr, "Subpixel PPM size mismatch: %ld\n", bytes);
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
    puts("Subpixel real 3x3/5x5 slanted-curve preview: PASS");
    return 1;
}

int main(int argc, char **argv) {
    if (argc > 2) {
        fputs("usage: subpixel-preview [output.ppm | --self-test]\n", stderr);
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
        fputs("Subpixel preview PPM write failed\n", stderr);
        return 1;
    }
    return 0;
}
