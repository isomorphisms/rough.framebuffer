/* Real whole-path consumer. The reference font sampler is not a renderer here.
 * Every fine-grid and grayscale panel reads the SAME final coverage masks. */
#include "font_5x7.h"
#include "subpixel/path.h"
#include "subpixel_framebuffer.h"

#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { MAX_WIDTH = 20, MAX_HEIGHT = 28, PAD = 3, MAX_STRIDE = MAX_WIDTH + PAD,
       MAX_WORDS = MAX_STRIDE * MAX_HEIGHT, MAX_PACKED = MAX_WIDTH * MAX_HEIGHT,
       MODES = 5, GLYPHS = 4, MAX_PANEL_WIDTH = 5 * (20 * MAX_WIDTH + 16),
       MAX_PANEL_HEIGHT = 4 * (10 * MAX_HEIGHT + 12) };
static uint64_t masks[MAX_WORDS], scratch[MAX_PACKED], reference[MAX_PACKED];
static uint32_t resolved[MAX_WORDS], panel[MAX_PANEL_WIDTH * MAX_PANEL_HEIGHT];
static const char names[GLYPHS] = {'O', 'C', 'S', '8'};
static const char *mode_names[MODES] = {
    "pixel-block", "legacy-stroke", "quadratic-3x3", "cubic-5x5", "auto"
};

static unsigned count_bits(uint64_t bits) {
    unsigned count = 0;
    while (bits) { bits &= bits - UINT64_C(1); ++count; }
    return count;
}

static int output_path(char path[1024], const char *directory, const char *name) {
    int length = snprintf(path, 1024, "%s/%s", directory, name);
    return length > 0 && length < 1024;
}

static uint64_t mask_hash(const cp_coverage_plane *plane) {
    uint64_t hash = UINT64_C(14695981039346656037);
    for (unsigned y = 0; y < plane->height; ++y)
        for (unsigned x = 0; x < plane->width; ++x) {
            uint64_t bits = plane->occupied[y * plane->stride_words + x];
            for (unsigned byte = 0; byte < 8; ++byte) {
                hash ^= (bits >> (8 * byte)) & UINT64_C(255);
                hash *= UINT64_C(1099511628211);
            }
        }
    return hash;
}

static void clear_storage(unsigned width, unsigned height, int inverse) {
    for (unsigned y = 0; y < height; ++y)
        for (unsigned x = 0; x < width + PAD; ++x) {
            masks[y * (width + PAD) + x] = x < width ? 0 : UINT64_C(0xa5a5a5a5a5a5a5a5);
            resolved[y * (width + PAD) + x] = inverse ? 0xff000000U : 0xffffffffU;
        }
}

/* Intentional hard-edged control. Integer scales preserve exact 0/64 cells.
 * Fractional origins keep the rectangle edges at their true sample phase. */
static void bitmap_masks(const sf57_glyph *glyph, double scale, double origin,
                         cp_coverage_plane *plane) {
    for (unsigned y = 0; y < plane->height; ++y)
        for (unsigned x = 0; x < plane->width; ++x)
            for (unsigned sy = 0; sy < 8; ++sy)
                for (unsigned sx = 0; sx < 8; ++sx) {
                    double material_x = (x + (sx + .5) / 8.0 - origin) / scale;
                    double material_y = (y + (sy + .5) / 8.0 - origin) / scale;
                    if (material_x < 0 || material_x >= SF57_COLUMNS ||
                        material_y < 0 || material_y >= SF57_ROWS)
                        continue;
                    if (glyph->bitmap_rows[(unsigned)material_y][(unsigned)material_x] == '#')
                        plane->occupied[y * plane->stride_words + x] |=
                            UINT64_C(1) << (8 * sy + sx);
                }
}

static cp_path_status continuous_masks(const sf57_glyph *glyph, double scale,
    double origin, cp_path_strategy strategy, cp_coverage_plane *plane,
    cp_path_report *report) {
    cp_path_segment segments[8];
    if (!sf57_validate_glyph(glyph) || glyph->segment_count > 8)
        return CP_PATH_INVALID;
    for (size_t segment = 0; segment < glyph->segment_count; ++segment) {
        segments[segment].degree = (unsigned)glyph->segments[segment].kind;
        for (unsigned point = 0; point < 4; ++point) {
            segments[segment].controls[point].x = glyph->segments[segment].point[point].x;
            segments[segment].controls[point].y = glyph->segments[segment].point[point].y;
        }
    }
    cp_path_options options = {
        .transform = {scale, 0, 0, scale, origin - .5, origin - .5},
        .width = glyph->stroke_width * scale, .tolerance = .01,
        .max_pixel_error = 2, .max_depth = 24, .max_patches = 100000,
        .max_sample_tests = 200000000, .strategy = strategy,
        .cancelled = NULL, .cancel_context = NULL
    };
    cp_path_workspace workspace = {scratch, reference, MAX_PACKED};
    return cp_path_rasterize(plane, segments, glyph->segment_count, &options,
                             &workspace, report);
}

static int legacy_pixels(const sf57_glyph *glyph, double scale, double origin,
                          rough_framebuffer *target, int inverse) {
    FILE *operations = tmpfile();
    if (!operations) return 0;
    fprintf(operations, "P %d %d %d\nW %.17g\n", inverse ? 255 : 0,
            inverse ? 255 : 0, inverse ? 255 : 0, glyph->stroke_width * scale);
    for (size_t path = 0; path < glyph->subpath_count; ++path) {
        const sf57_subpath *subpath = &glyph->subpaths[path];
        const sf57_segment *first = &glyph->segments[subpath->first_segment];
        fprintf(operations, "M %.17g %.17g\n",
                first->point[0].x * scale + origin - .5,
                first->point[0].y * scale + origin - .5);
        for (size_t index = subpath->first_segment;
             index < subpath->first_segment + subpath->segment_count; ++index) {
            const sf57_segment *segment = &glyph->segments[index];
            if (segment->kind != SF57_CUBIC) { fclose(operations); return 0; }
            fprintf(operations, "C %.17g %.17g %.17g %.17g %.17g %.17g\n",
                segment->point[1].x * scale + origin - .5,
                segment->point[1].y * scale + origin - .5,
                segment->point[2].x * scale + origin - .5,
                segment->point[2].y * scale + origin - .5,
                segment->point[3].x * scale + origin - .5,
                segment->point[3].y * scale + origin - .5);
        }
    }
    if (fflush(operations) != 0) { fclose(operations); return 0; }
    rewind(operations);
    int result = rough_render_stream(operations, target);
    return fclose(operations) == 0 && result == 0;
}

static void rectangle(unsigned panel_width, unsigned x, unsigned y,
                       unsigned width, unsigned height, uint32_t color) {
    for (unsigned dy = 0; dy < height; ++dy)
        for (unsigned dx = 0; dx < width; ++dx)
            panel[(y + dy) * panel_width + x + dx] = color;
}

static int paint_tile(unsigned panel_width, unsigned glyph_index, unsigned mode,
    int inverse, int status, const cp_coverage_plane *plane) {
    unsigned width = plane->width, height = plane->height;
    unsigned tile_width = 20 * width + 16, tile_height = 10 * height + 12;
    unsigned left = mode * tile_width + 4, top = glyph_index * tile_height + 4;
    uint32_t ink = inverse ? 0xffffffffU : 0xff000000U;
    uint32_t paper = inverse ? 0xff000000U : 0xffffffffU;
    if (status != 0) {
        /* A refusal is visibly red; it never borrows another strategy's image. */
        rectangle(panel_width, left, top, tile_width - 8, tile_height - 8, 0xffa02020U);
        return 1;
    }
    for (unsigned y = 0; y < height; ++y)
        for (unsigned x = 0; x < width; ++x) {
            uint32_t gray = resolved[y * (width + PAD) + x];
            rectangle(panel_width, left + 10 * width + 4 + 8 * x,
                      top + 8 * y, 8, 8, gray);
            rectangle(panel_width, left + 18 * width + 8 + x,
                      top + y, 1, 1, gray); /* native 1x display size */
            if (mode == 1) continue; /* legacy does not expose sample masks */
            uint64_t bits = masks[y * (width + PAD) + x];
            rectangle(panel_width, left + 10 * x, top + 10 * y,
                      10, 10, 0xff808080U);
            for (unsigned sy = 0; sy < 8; ++sy)
                for (unsigned sx = 0; sx < 8; ++sx)
                    rectangle(panel_width, left + 10 * x + sx + 1,
                        top + 10 * y + sy + 1, 1, 1,
                        (bits & (UINT64_C(1) << (8 * sy + sx))) ? ink : paper);
        }
    return 1;
}

static int emit_panel(const char *directory, double scale, int offset_index,
                       int inverse, unsigned width, unsigned height) {
    char name[100], path[1024];
    snprintf(name, sizeof name, "font-%.2gx-origin-%s-%s.ppm", scale,
        offset_index == 0 ? "minus-eighth" : offset_index == 1 ? "zero" :
        offset_index == 2 ? "plus-eighth" : "clipped-minus-2.5",
        inverse ? "white-on-black" : "black-on-white");
    if (!output_path(path, directory, name)) return 0;
    FILE *file = fopen(path, "wb");
    if (!file) return 0;
    int okay = fprintf(file, "P6\n%u %u\n255\n", width, height) > 0;
    for (unsigned y = 0; okay && y < height; ++y)
        for (unsigned x = 0; okay && x < width; ++x) {
            uint32_t pixel = panel[y * width + x];
            unsigned char rgb[] = {(unsigned char)(pixel >> 16),
                                   (unsigned char)(pixel >> 8), (unsigned char)pixel};
            okay = fwrite(rgb, 1, 3, file) == 3;
        }
    return fclose(file) == 0 && okay;
}

static int run_case(FILE *metrics, FILE *sample_masks, double scale,
                     int offset_index, int inverse) {
    double origin = offset_index == 3 ? -2.5 : (offset_index - 1) / 8.0;
    unsigned width = (unsigned)ceil(SF57_COLUMNS * scale);
    unsigned height = (unsigned)ceil(SF57_ROWS * scale);
    unsigned panel_width = MODES * (20 * width + 16);
    unsigned panel_height = GLYPHS * (10 * height + 12);
    for (size_t pixel = 0; pixel < (size_t)panel_width * panel_height; ++pixel)
        panel[pixel] = inverse ? 0xff202020U : 0xffddddddU;
    for (unsigned glyph_index = 0; glyph_index < GLYPHS; ++glyph_index) {
        const sf57_glyph *glyph = sf57_find_glyph(names[glyph_index]);
        if (!glyph) return 0;
        for (unsigned mode = 0; mode < MODES; ++mode) {
            clear_storage(width, height, inverse);
            cp_coverage_plane plane = {masks, MAX_WORDS, width + PAD,
                                       width, height, 0, 0};
            rough_framebuffer target = {resolved, width, height, width + PAD};
            cp_path_report report = {0};
            int status = 0;
            if (mode == 0) bitmap_masks(glyph, scale, origin, &plane);
            else if (mode == 1) status = legacy_pixels(glyph, scale, origin,
                                                      &target, inverse) ? 0 : -1;
            else status = continuous_masks(glyph, scale, origin,
                mode == 2 ? CP_PATH_QUADRATIC : mode == 3 ? CP_PATH_CUBIC : CP_PATH_AUTO,
                &plane, &report);
            if (status == 0 && mode != 1 && rough_blend_subpixel_coverage(&target,
                    &plane, inverse ? 0xffffffffU : 0xff000000U, 1.0) != 0)
                return 0;
            unsigned fractional = 0, occupied = 0;
            size_t total_difference = 0;
            for (unsigned y = 0; y < height; ++y) {
                for (unsigned padding = width; padding < width + PAD; ++padding)
                    if (masks[y * (width + PAD) + padding] != UINT64_C(0xa5a5a5a5a5a5a5a5))
                        return 0;
                for (unsigned x = 0; x < width; ++x) {
                    uint64_t bits = masks[y * (width + PAD) + x];
                    unsigned count = count_bits(bits);
                    if (status == 0 && mode != 1) {
                        unsigned expected_gray = (unsigned)floor(
                            255.0 * (inverse ? count : 64U - count) / 64.0 + .5);
                        uint32_t expected_pixel = 0xff000000U |
                            (expected_gray << 16) | (expected_gray << 8) | expected_gray;
                        if (resolved[y * (width + PAD) + x] != expected_pixel)
                            return 0;
                        if (mode == 0 && origin == 0.0 && scale == floor(scale) &&
                            count != 0 && count != 64)
                            return 0;
                    }
                    occupied += count;
                    fractional += count > 0 && count < 64;
                    if (mode >= 2 && status == 0)
                        total_difference += count_bits(bits ^ reference[y * width + x]);
                    if (status == 0 && mode != 1 &&
                        fprintf(sample_masks, "%c\t%s\t%.3f\t%.3f\t%d\t%u\t%u\t%016" PRIx64 "\t%u\n",
                            glyph->name, mode_names[mode], scale, origin, inverse,
                            x, y, bits, count) < 0)
                        return 0;
                }
            }
            if (status == 0 && mode >= 2 && (occupied == 0 || fractional == 0))
                return 0;
            if (fprintf(metrics,
                "%c\tcontinuous-masters-v1\t%s\t%.3f\t%.3f\t%d\t%d\t%s\t%d\t%s\t%zu\t%zu\t%zu\t%zu\t%zu\t100000\t200000000\t24\t%u\t%.6f\t%u\t%u\t%016" PRIx64 "\n",
                glyph->name, mode_names[mode], scale, origin, inverse, status,
                mode == 1 ? "MASKS_UNAVAILABLE" : status ? "REFUSED" : "MASKS",
                mode >= 2 && status == 0 ? (int)report.selected : -1,
                report.reason ? report.reason : mode == 0 ? "exact-rectangles" :
                    mode == 1 ? "legacy-stream" : "transactional-refusal",
                report.patches, report.reference_segments, report.sample_tests,
                report.peak_bytes, sizeof masks + sizeof scratch + sizeof reference +
                    sizeof resolved + sizeof panel, report.worst_pixel_error,
                (double)total_difference / (width * height), occupied,
                fractional, mode == 1 || status ? UINT64_C(0) : mask_hash(&plane)) < 0)
                return 0;
            if (!paint_tile(panel_width, glyph_index, mode, inverse, status, &plane))
                return 0;
        }
    }
    return 1;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fputs("usage: font-5x7-preview EXISTING-OUTPUT-DIRECTORY\n", stderr);
        return 2;
    }
    char path[1024];
    if (!output_path(path, argv[1], "font-metrics.tsv")) return 2;
    FILE *metrics = fopen(path, "w");
    if (!metrics) return 1;
    if (!output_path(path, argv[1], "font-masks.tsv")) { fclose(metrics); return 2; }
    FILE *sample_masks = fopen(path, "w");
    if (!sample_masks) { fclose(metrics); return 1; }
    fputs("glyph\tmaster\talgorithm\tscale\torigin_xy\tinverse\tstatus\tevidence\tselected_strategy\treason\tpatches\treference_segments\tsample_tests\tpath_peak_bytes\tpreview_owned_bytes\tmax_patches\tmax_sample_tests\tmax_depth\tworst_xor_bits\tmean_xor_bits\toccupied_samples\tfractional_cells\tfnv1a_masks\n", metrics);
    fputs("glyph\talgorithm\tscale\torigin_xy\tinverse\tx\ty\tmask\tpopcount\n", sample_masks);
    int okay = 1;
    const double scales[] = {1.0, 1.5, 2.0, 4.0};
    for (unsigned scale_index = 0; okay && scale_index < 4; ++scale_index)
        for (int offset = 0; okay && offset < 4; ++offset)
            for (int inverse = 0; okay && inverse < 2; ++inverse) {
                double scale = scales[scale_index];
                /* run_case paints the shared panel, then main saves it. */
                okay = run_case(metrics, sample_masks, scale, offset, inverse);
                if (okay) okay = emit_panel(argv[1], scale, offset, inverse,
                    MODES * (20 * (unsigned)ceil(SF57_COLUMNS * scale) + 16),
                    GLYPHS * (10 * (unsigned)ceil(SF57_ROWS * scale) + 12));
            }
    if (fclose(metrics) != 0) okay = 0;
    if (fclose(sample_masks) != 0) okay = 0;
    if (!okay) { fputs("font preview failed\n", stderr); return 1; }
    puts("Real Subpixel font masks, padded coverage, 32 PPM panels and metrics: PASS");
    return 0;
}
