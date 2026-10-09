#include "framebuffer.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static int dimension(const char *text, unsigned *out) {
    char *end;
    errno = 0;
    unsigned long value = strtoul(text, &end, 10);
    if (errno || !*text || *end || value < 1 || value > 8192) return 0;
    *out = (unsigned)value;
    return 1;
}

int main(int argc, char **argv) {
    unsigned width, height;
    if (argc != 5 || !dimension(argv[3], &width) || !dimension(argv[4], &height)) {
        fprintf(stderr, "usage: rough-fb geometry.ops output.ppm width height\n");
        return 2;
    }
    size_t count = (size_t)width * height;
    if (count > SIZE_MAX / sizeof(uint32_t)) return 2;
    uint32_t *pixels = malloc(count * sizeof *pixels);
    if (!pixels) return 1;
    for (size_t i = 0; i < count; ++i) pixels[i] = 0xffffffffu;

    FILE *input = fopen(argv[1], "rb");
    if (!input) { perror("open operations"); free(pixels); return 1; }
    rough_framebuffer fb = {pixels, width, height, width};
    int status = rough_render_stream(input, &fb);
    fclose(input);
    if (status) { fprintf(stderr, "invalid geometry stream (%d)\n", status); free(pixels); return 1; }

    FILE *output = fopen(argv[2], "wb");
    if (!output) { perror("open output"); free(pixels); return 1; }
    int failed = fprintf(output, "P6\n%u %u\n255\n", width, height) < 0;
    for (size_t i = 0; i < count && !failed; ++i) {
        unsigned char rgb[3] = {(unsigned char)(pixels[i] >> 16),
                                (unsigned char)(pixels[i] >> 8), (unsigned char)pixels[i]};
        if (fwrite(rgb, 1, 3, output) != 3) failed = 1;
    }
    if (fclose(output)) failed = 1;
    free(pixels);
    if (failed) { fprintf(stderr, "could not write PPM\n"); return 1; }
    return 0;
}
