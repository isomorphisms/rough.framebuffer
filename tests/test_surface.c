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
