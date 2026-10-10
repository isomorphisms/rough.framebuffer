#include "framebuffer.h"
#include "surface.h"

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    unsigned triangles;
    unsigned near_vertices;
} stream_stats;

static FILE *generate(rough_surface_shape shape, double angle, double distance) {
    FILE *stream = tmpfile();
    assert(stream);
    assert(rough_emit_surface(stream, shape, angle, 320, 240, distance) == 0);
    rewind(stream);
    return stream;
}

/* Independently parse the actual emitted protocol, not internal mesh structs. */
static stream_stats inspect(FILE *stream) {
    stream_stats stats = {0u, 0u};
    char line[512];
    int pen_ready = 0;
    rewind(stream);
    while (fgets(line, sizeof line, stream)) {
        if (line[0] == 'P') {
            unsigned r, g, b;
            char surplus;
            assert(sscanf(line, "P %u %u %u %c", &r, &g, &b, &surplus) == 3);
            assert(r <= 255u && g <= 255u && b <= 255u);
            assert(!pen_ready);
            pen_ready = 1;
        } else {
            double f[9];
            char surplus;
            assert(line[0] == 'T' && pen_ready);
            assert(sscanf(line, "T %lf %lf %lf %lf %lf %lf %lf %lf %lf %c",
                          &f[0], &f[1], &f[2], &f[3], &f[4], &f[5],
                          &f[6], &f[7], &f[8], &surplus) == 9);
            for (unsigned i = 0; i < 9u; ++i) assert(isfinite(f[i]));
            for (unsigned i = 2u; i < 9u; i += 3u) {
                assert(f[i] >= 0.0 && f[i] < 1.0);
                if (f[i] == 0.0) ++stats.near_vertices;
            }
            ++stats.triangles;
            pen_ready = 0;
        }
    }
    assert(!ferror(stream) && !pen_ready);
    assert(stats.triangles > 100u);
    rewind(stream);
    return stats;
}

static int equal_streams(FILE *left, FILE *right) {
    rewind(left);
    rewind(right);
    for (;;) {
        int a = fgetc(left);
        int b = fgetc(right);
        if (a != b) return 0;
        if (a == EOF) return 1;
    }
}

/* This oracle comes from the torus equations at u=v=0, not by reusing
 * the generator's transformation or projection functions. It catches
 * swapped camera axes, a wrong depth sign, or a mismatched focal length. */
static void reference_torus_vertex(FILE *stream) {
    char line[512];
    double values[9];
    char surplus;
    rewind(stream);
    assert(fgets(line, sizeof line, stream));
    assert(line[0] == 'P');
    assert(fgets(line, sizeof line, stream));
    assert(sscanf(line, "T %lf %lf %lf %lf %lf %lf %lf %lf %lf %c",
                  &values[0], &values[1], &values[2],
                  &values[3], &values[4], &values[5],
                  &values[6], &values[7], &values[8], &surplus) == 9);
    double expected_x = 159.5 + (0.88 * 240.0) * 1.61 / 5.2;
    double expected_y = 119.5;
    double expected_depth = 1.0 - 0.5 / 5.2;
    assert(fabs(values[0] - expected_x) < 1e-10);
    assert(fabs(values[1] - expected_y) < 1e-10);
    assert(fabs(values[2] - expected_depth) < 1e-12);
    rewind(stream);
}

/* Compare material colors on the two sides of each periodic torus seam.
 * The generator emits two P/T faces per parametric cell at camera distance
 * 5.2. A known-bad sin(1.3*u)/cos(1.8*v) palette violates this invariant,
 * although the 3D geometry closes normally and all raster tests still pass. */
static unsigned component_delta(unsigned left, unsigned right) {
    return left > right ? left - right : right - left;
}

static void assert_torus_seam_palette(FILE *stream) {
    unsigned rgb[56u][28u][3u];
    char line[512];
    rewind(stream);
    for (unsigned i = 0; i < 56u; ++i) {
        for (unsigned j = 0; j < 28u; ++j) {
            for (unsigned face = 0; face < 2u; ++face) {
                unsigned r, g, b;
                char trailing;
                assert(fgets(line, sizeof line, stream));
                assert(sscanf(line, "P %u %u %u %c",
                              &r, &g, &b, &trailing) == 3);
                assert(fgets(line, sizeof line, stream));
                assert(line[0] == 'T');
                if (face == 0u) {
                    rgb[i][j][0] = r;
                    rgb[i][j][1] = g;
                    rgb[i][j][2] = b;
                }
            }
        }
    }
    assert(fgetc(stream) == EOF);
    for (unsigned channel = 0; channel < 3u; ++channel) {
        for (unsigned j = 0; j < 28u; ++j) {
            unsigned delta = component_delta(rgb[0][j][channel],
                                             rgb[55][j][channel]);
            if (delta > 28u) {
                fprintf(stderr, "torus seam palette jump in u: %u\n", delta);
                abort();
            }
        }
        for (unsigned i = 0; i < 56u; ++i) {
            unsigned delta = component_delta(rgb[i][0][channel],
                                             rgb[i][27][channel]);
            if (delta > 28u) {
                fprintf(stderr, "torus seam palette jump in v: %u\n", delta);
                abort();
            }
        }
    }
    rewind(stream);
}

static uint64_t render_checksum(FILE *stream) {
    const unsigned width = 320u;
    const unsigned height = 240u;
    const size_t count = (size_t)width * height;
    uint32_t *pixels = malloc(count * sizeof *pixels);
    float *depth = malloc(count * sizeof *depth);
    assert(pixels && depth);
    for (size_t i = 0; i < count; ++i) {
        pixels[i] = 0xffffffffu;
        depth[i] = INFINITY;
    }
    rough_framebuffer target = {pixels, width, height, width, depth, width};
    rewind(stream);
    assert(rough_render_stream(stream, &target) == 0);

    unsigned painted = 0u;
    uint64_t checksum = UINT64_C(14695981039346656037);
    for (size_t i = 0; i < count; ++i) {
        if (pixels[i] != 0xffffffffu) {
            ++painted;
            assert(isfinite(depth[i]) && depth[i] >= 0.0f && depth[i] < 1.0f);
        } else {
            assert(isinf(depth[i]));
        }
        checksum ^= (uint64_t)pixels[i];
        checksum *= UINT64_C(1099511628211);
    }
    assert(painted > 1000u);
    free(pixels);
    free(depth);
    return checksum;
}

int main(void) {
    FILE *torus0 = generate(ROUGH_SURFACE_TORUS, 0.0, 5.2);
    FILE *torus0_repeat = generate(ROUGH_SURFACE_TORUS, 360.0, 5.2);
    FILE *torus37 = generate(ROUGH_SURFACE_TORUS, 37.0, 5.2);
    FILE *enneper = generate(ROUGH_SURFACE_ENNEPER, 25.0, 5.2);
    FILE *near = generate(ROUGH_SURFACE_TORUS, 30.0, 0.55);

    assert(inspect(torus0).triangles > 100u);
    assert(inspect(torus0_repeat).triangles > 100u);
    assert(inspect(torus37).triangles > 100u);
    assert(inspect(enneper).triangles > 100u);
    assert(inspect(near).near_vertices > 0u);
    reference_torus_vertex(torus0);
    assert_torus_seam_palette(torus0);
    assert(equal_streams(torus0, torus0_repeat));
    assert(!equal_streams(torus0, torus37));

    uint64_t first = render_checksum(torus0);
    uint64_t second = render_checksum(torus37);
    uint64_t third = render_checksum(enneper);
    assert(first != second);
    assert(first != third);

    assert(rough_emit_surface(NULL, ROUGH_SURFACE_TORUS, 0.0, 320, 240, 5.2) == -1);
    assert(rough_emit_surface(torus0, (rough_surface_shape)99, 0.0, 320, 240, 5.2) == -1);
    assert(rough_emit_surface(torus0, ROUGH_SURFACE_TORUS, NAN, 320, 240, 5.2) == -1);
    assert(rough_emit_surface(torus0, ROUGH_SURFACE_TORUS, INFINITY, 320, 240, 5.2) == -1);
    assert(rough_emit_surface(torus0, ROUGH_SURFACE_TORUS, 0.0, 0, 240, 5.2) == -1);
    assert(rough_emit_surface(torus0, ROUGH_SURFACE_TORUS, 0.0, 320, 0, 5.2) == -1);
    assert(rough_emit_surface(torus0, ROUGH_SURFACE_TORUS, 0.0, 8193, 240, 5.2) == -1);
    assert(rough_emit_surface(torus0, ROUGH_SURFACE_TORUS, 0.0, 320, 240, 0.0) == -1);
    assert(rough_emit_surface(torus0, ROUGH_SURFACE_TORUS, 0.0, 320, 240, -2.0) == -1);
    assert(rough_emit_surface(torus0, ROUGH_SURFACE_TORUS, 0.0, 320, 240, NAN) == -1);

    assert(fclose(torus0) == 0);
    assert(fclose(torus0_repeat) == 0);
    assert(fclose(torus37) == 0);
    assert(fclose(enneper) == 0);
    assert(fclose(near) == 0);
    puts("native surface stream, clipping, shading, and raster integration: PASS");
    return 0;
}
