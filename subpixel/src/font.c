#include "subpixel/font.h"
#include "subpixel/path.h"
#include "../examples/font_5x7.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

int sf57_render_coverage(uint32_t codepoint, unsigned scale,
    double phase_x, double phase_y, uint8_t *coverage, size_t capacity) {
    if (!coverage || scale < 1 || scale > 20 ||
        !isfinite(phase_x) || !isfinite(phase_y) ||
        fabs(phase_x) > .5 || fabs(phase_y) > .5) return 0;
    unsigned width = 5 * scale, height = 7 * scale;
    size_t pixels = (size_t)width * height;
    if (capacity < pixels) return 0;
    if (codepoint == ' ') { memset(coverage, 0, pixels); return 1; }
    const sf57_glyph *glyph = sf57_find_glyph(codepoint);
    if (!sf57_validate_glyph(glyph) || glyph->segment_count > 16) return 0;
    uint64_t *storage = calloc(3 * pixels, sizeof *storage);
    if (!storage) return 0;
    cp_path_segment segments[16];
    for (size_t index = 0; index < glyph->segment_count; ++index) {
        segments[index].degree = (unsigned)glyph->segments[index].kind;
        for (unsigned point = 0; point < 4; ++point) {
            segments[index].controls[point].x = glyph->segments[index].point[point].x;
            segments[index].controls[point].y = glyph->segments[index].point[point].y;
        }
    }
    cp_coverage_plane plane = {storage, pixels, width, width, height, 0, 0};
    cp_path_workspace workspace = {storage + pixels, storage + 2 * pixels, pixels};
    const cp_path_options options = {
        .transform = {(double)scale,0,0,(double)scale,phase_x-.5,phase_y-.5},
        .width = glyph->stroke_width * scale, .tolerance = .01,
        .max_pixel_error = 2, .max_depth = 24, .max_patches = 100000,
        .max_sample_tests = 200000000, .strategy = CP_PATH_AUTO,
        .cancelled = NULL, .cancel_context = NULL
    };
    cp_path_report report;
    cp_path_status status = cp_path_rasterize(&plane, segments,
        glyph->segment_count, &options, &workspace, &report);
    if (status == CP_PATH_OK) {
        for (size_t pixel = 0; pixel < pixels; ++pixel) {
            unsigned count = 0;
            uint64_t bits = storage[pixel];
            while (bits) { bits &= bits - UINT64_C(1); ++count; }
            coverage[pixel] = (uint8_t)((count * 255 + 32) / 64);
        }
    }
    free(storage);
    return status == CP_PATH_OK;
}
