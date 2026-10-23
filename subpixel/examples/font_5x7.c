#include "font_5x7.h"

#include <math.h>
#include <string.h>

#define P(x_value, y_value) { (x_value), (y_value) }
#define LINE(a, b) { SF57_LINE, { a, b, b, b } }
#define QUAD(a, b, c) { SF57_QUADRATIC, { a, b, c, c } }
#define CUBIC(a, b, c, d) { SF57_CUBIC, { a, b, c, d } }

/* Kappa gives the usual four-cubic approximation to an ellipse. These are
 * deliberately inspectable masters, not smoothed versions of the bitmap. */
static const sf57_segment glyph_o_segments[] = {
    CUBIC(P(2.50, 0.75), P(3.38, 0.75), P(4.10, 1.98), P(4.10, 3.50)),
    CUBIC(P(4.10, 3.50), P(4.10, 5.02), P(3.38, 6.25), P(2.50, 6.25)),
    CUBIC(P(2.50, 6.25), P(1.62, 6.25), P(0.90, 5.02), P(0.90, 3.50)),
    CUBIC(P(0.90, 3.50), P(0.90, 1.98), P(1.62, 0.75), P(2.50, 0.75))
};
static const sf57_subpath glyph_o_paths[] = {{0, 4, 1}};

static const sf57_segment glyph_c_segments[] = {
    CUBIC(P(4.10, 1.25), P(3.55, 0.82), P(2.95, 0.70), P(2.35, 0.78)),
    CUBIC(P(2.35, 0.78), P(0.45, 1.04), P(0.45, 5.96), P(2.35, 6.22)),
    CUBIC(P(2.35, 6.22), P(2.95, 6.30), P(3.55, 6.18), P(4.10, 5.75))
};
static const sf57_subpath glyph_c_paths[] = {{0, 3, 0}};

static const sf57_segment glyph_s_segments[] = {
    CUBIC(P(4.05, 1.15), P(3.25, 0.55), P(1.20, 0.62), P(0.95, 2.18)),
    CUBIC(P(0.95, 2.18), P(0.72, 3.60), P(4.25, 3.22), P(4.05, 4.82)),
    CUBIC(P(4.05, 4.82), P(3.82, 6.38), P(1.45, 6.42), P(0.82, 5.72))
};
static const sf57_subpath glyph_s_paths[] = {{0, 3, 0}};

static const sf57_segment glyph_8_segments[] = {
    CUBIC(P(2.50, 0.55), P(3.28, 0.55), P(3.92, 1.28), P(3.92, 2.18)),
    CUBIC(P(3.92, 2.18), P(3.92, 3.08), P(3.28, 3.62), P(2.50, 3.62)),
    CUBIC(P(2.50, 3.62), P(1.72, 3.62), P(1.08, 3.08), P(1.08, 2.18)),
    CUBIC(P(1.08, 2.18), P(1.08, 1.28), P(1.72, 0.55), P(2.50, 0.55)),
    CUBIC(P(2.50, 3.38), P(3.34, 3.38), P(4.02, 4.02), P(4.02, 5.05)),
    CUBIC(P(4.02, 5.05), P(4.02, 6.08), P(3.34, 6.48), P(2.50, 6.48)),
    CUBIC(P(2.50, 6.48), P(1.66, 6.48), P(0.98, 6.08), P(0.98, 5.05)),
    CUBIC(P(0.98, 5.05), P(0.98, 4.02), P(1.66, 3.38), P(2.50, 3.38))
};
static const sf57_subpath glyph_8_paths[] = {{0, 4, 1}, {4, 4, 1}};

static const sf57_glyph glyphs[] = {
    {'O', {".###.", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."},
     glyph_o_segments, 4, glyph_o_paths, 1, 0.72, 6.0, 0.0, 6.5},
    {'C', {".###.", "#...#", "#....", "#....", "#....", "#...#", ".###."},
     glyph_c_segments, 3, glyph_c_paths, 1, 0.72, 6.0, 0.0, 6.5},
    {'S', {".###.", "#...#", "#....", ".###.", "....#", "#...#", ".###."},
     glyph_s_segments, 3, glyph_s_paths, 1, 0.72, 6.0, 0.0, 6.5},
    {'8', {".###.", "#...#", "#...#", ".###.", "#...#", "#...#", ".###."},
     glyph_8_segments, 8, glyph_8_paths, 2, 0.66, 6.0, 0.0, 6.5}
};

/* Directly authored instrument alphabet; never traced from bitmap cells. */
#include "font_5x7_masters.inc"

static int point_is_finite(sf57_point point) {
    return isfinite(point.x) && isfinite(point.y);
}

static int points_match(sf57_point left, sf57_point right) {
    return fabs(left.x - right.x) <= 1e-12 &&
           fabs(left.y - right.y) <= 1e-12;
}

static sf57_point segment_start(const sf57_segment *segment) {
    return segment->point[0];
}

static sf57_point segment_end(const sf57_segment *segment) {
    if (segment->kind == SF57_LINE) return segment->point[1];
    if (segment->kind == SF57_QUADRATIC) return segment->point[2];
    return segment->point[3];
}

const sf57_glyph *sf57_find_glyph(uint32_t name) {
    for (size_t index = 0; index < sizeof glyphs / sizeof glyphs[0]; ++index)
        if (glyphs[index].name == name) return &glyphs[index];
    for (size_t index = 0; index < sizeof sensor_glyphs / sizeof sensor_glyphs[0]; ++index)
        if (sensor_glyphs[index].name == name) return &sensor_glyphs[index];
    return NULL;
}

int sf57_validate_glyph(const sf57_glyph *glyph) {
    if (!glyph || !glyph->segments || !glyph->subpaths ||
        glyph->segment_count == 0 || glyph->subpath_count == 0 ||
        !isfinite(glyph->stroke_width) || glyph->stroke_width <= 0.0 ||
        !isfinite(0.25 * glyph->stroke_width * glyph->stroke_width) ||
        !isfinite(glyph->advance) || glyph->advance <= 0.0 ||
        !isfinite(glyph->bearing_x) || !isfinite(glyph->baseline))
        return 0;
    for (unsigned row = 0; row < SF57_ROWS; ++row) {
        if (!glyph->bitmap_rows[row]) continue; /* Geometry-only master. */
        if (strlen(glyph->bitmap_rows[row]) != (size_t)SF57_COLUMNS)
            return 0;
        for (unsigned column = 0; column < SF57_COLUMNS; ++column)
            if (glyph->bitmap_rows[row][column] != '.' &&
                glyph->bitmap_rows[row][column] != '#')
                return 0;
    }
    for (size_t index = 0; index < glyph->segment_count; ++index) {
        const sf57_segment *segment = &glyph->segments[index];
        unsigned point_count = segment->kind == SF57_LINE ? 2U :
                               segment->kind == SF57_QUADRATIC ? 3U :
                               segment->kind == SF57_CUBIC ? 4U : 0U;
        if (point_count == 0) return 0;
        for (unsigned point = 0; point < point_count; ++point)
            if (!point_is_finite(segment->point[point])) return 0;
    }
    size_t expected_first = 0;
    for (size_t path_index = 0; path_index < glyph->subpath_count; ++path_index) {
        const sf57_subpath *path = &glyph->subpaths[path_index];
        if ((path->closed != 0 && path->closed != 1) ||
            path->first_segment != expected_first || path->segment_count == 0 ||
            path->first_segment > glyph->segment_count ||
            path->segment_count > glyph->segment_count - path->first_segment)
            return 0;
        size_t end = path->first_segment + path->segment_count;
        for (size_t index = path->first_segment + 1; index < end; ++index)
            if (!points_match(segment_end(&glyph->segments[index - 1]),
                              segment_start(&glyph->segments[index])))
                return 0;
        if (path->closed &&
            !points_match(segment_end(&glyph->segments[end - 1]),
                          segment_start(&glyph->segments[path->first_segment])))
            return 0;
        expected_first = end;
    }
    return expected_first == glyph->segment_count;
}

int sf57_bitmap_cell(const sf57_glyph *glyph, unsigned column, unsigned row) {
    if (!sf57_validate_glyph(glyph) || column >= SF57_COLUMNS || row >= SF57_ROWS)
        return -1;
    return glyph->bitmap_rows[row] ? glyph->bitmap_rows[row][column] == '#' : -1;
}

static sf57_point evaluate(const sf57_segment *segment, double parameter) {
    double t = parameter;
    double u = 1.0 - t;
    sf57_point value;
    if (segment->kind == SF57_LINE) {
        value.x = u * segment->point[0].x + t * segment->point[1].x;
        value.y = u * segment->point[0].y + t * segment->point[1].y;
    } else if (segment->kind == SF57_QUADRATIC) {
        value.x = u*u*segment->point[0].x + 2.0*u*t*segment->point[1].x +
                  t*t*segment->point[2].x;
        value.y = u*u*segment->point[0].y + 2.0*u*t*segment->point[1].y +
                  t*t*segment->point[2].y;
    } else {
        value.x = u*u*u*segment->point[0].x + 3.0*u*u*t*segment->point[1].x +
                  3.0*u*t*t*segment->point[2].x + t*t*t*segment->point[3].x;
        value.y = u*u*u*segment->point[0].y + 3.0*u*u*t*segment->point[1].y +
                  3.0*u*t*t*segment->point[2].y + t*t*t*segment->point[3].y;
    }
    return value;
}

static double distance_squared_to_line(sf57_point point,
                                       sf57_point start, sf57_point end) {
    double dx = end.x - start.x;
    double dy = end.y - start.y;
    double length_squared = dx*dx + dy*dy;
    double t = length_squared > 0.0
        ? ((point.x - start.x)*dx + (point.y - start.y)*dy) / length_squared
        : 0.0;
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    double nearest_x = start.x + t*dx;
    double nearest_y = start.y + t*dy;
    double error_x = point.x - nearest_x;
    double error_y = point.y - nearest_y;
    return error_x*error_x + error_y*error_y;
}

static int sample_inside(const sf57_glyph *glyph, sf57_point sample,
                         int reverse_parameterization) {
    const unsigned subdivisions = 96;
    double radius_squared = 0.25 * glyph->stroke_width * glyph->stroke_width;
    for (size_t index = 0; index < glyph->segment_count; ++index) {
        const sf57_segment *segment = &glyph->segments[index];
        double first_parameter = reverse_parameterization ? 1.0 : 0.0;
        sf57_point previous = evaluate(segment, first_parameter);
        for (unsigned step = 1; step <= subdivisions; ++step) {
            double parameter = (double)step / (double)subdivisions;
            if (reverse_parameterization) parameter = 1.0 - parameter;
            sf57_point current = evaluate(segment, parameter);
            if (distance_squared_to_line(sample, previous, current) <=
                radius_squared)
                return 1;
            previous = current;
        }
    }
    return 0;
}

static unsigned popcount64(uint64_t value) {
    unsigned count = 0;
    while (value != 0) {
        value &= value - UINT64_C(1);
        ++count;
    }
    return count;
}

int sf57_sample_masks(const sf57_glyph *glyph, int reverse_parameterization,
                      uint64_t masks[SF57_CELL_COUNT],
                      unsigned char counts[SF57_CELL_COUNT]) {
    if (!sf57_validate_glyph(glyph) || !masks || !counts ||
        (reverse_parameterization != 0 && reverse_parameterization != 1))
        return 0;
    uint64_t next_masks[SF57_CELL_COUNT] = {0};
    unsigned char next_counts[SF57_CELL_COUNT] = {0};
    for (unsigned row = 0; row < SF57_ROWS; ++row)
        for (unsigned column = 0; column < SF57_COLUMNS; ++column) {
            size_t cell = (size_t)row * SF57_COLUMNS + column;
            for (unsigned sample_y = 0; sample_y < SF57_SAMPLES_PER_AXIS; ++sample_y)
                for (unsigned sample_x = 0; sample_x < SF57_SAMPLES_PER_AXIS; ++sample_x) {
                    sf57_point sample = {
                        column + (sample_x + 0.5) / SF57_SAMPLES_PER_AXIS,
                        row + (sample_y + 0.5) / SF57_SAMPLES_PER_AXIS
                    };
                    if (sample_inside(glyph, sample, reverse_parameterization))
                        next_masks[cell] |= UINT64_C(1) <<
                            (SF57_SAMPLES_PER_AXIS * sample_y + sample_x);
                }
            next_counts[cell] = (unsigned char)popcount64(next_masks[cell]);
        }
    memcpy(masks, next_masks, sizeof next_masks);
    memcpy(counts, next_counts, sizeof next_counts);
    return 1;
}

