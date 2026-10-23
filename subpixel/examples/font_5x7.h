#ifndef SUBPIXEL_FONT_5X7_H
#define SUBPIXEL_FONT_5X7_H

#include <stddef.h>
#include <stdint.h>

enum {
    SF57_COLUMNS = 5,
    SF57_ROWS = 7,
    SF57_SAMPLES_PER_AXIS = 8,
    SF57_CELL_COUNT = SF57_COLUMNS * SF57_ROWS,
    SF57_FINE_WIDTH = SF57_COLUMNS * SF57_SAMPLES_PER_AXIS,
    SF57_FINE_HEIGHT = SF57_ROWS * SF57_SAMPLES_PER_AXIS
};

typedef struct { double x, y; } sf57_point;

typedef enum {
    SF57_LINE = 1,
    SF57_QUADRATIC = 2,
    SF57_CUBIC = 3
} sf57_segment_kind;

typedef struct {
    sf57_segment_kind kind;
    sf57_point point[4];
} sf57_segment;

typedef struct {
    size_t first_segment;
    size_t segment_count;
    int closed;
} sf57_subpath;

typedef struct {
    uint32_t name;
    const char *bitmap_rows[SF57_ROWS];
    const sf57_segment *segments;
    size_t segment_count;
    const sf57_subpath *subpaths;
    size_t subpath_count;
    double stroke_width;
    double advance;
    double bearing_x;
    double baseline;
} sf57_glyph;

/* The bitmap is an exact 35-cell reference. The curve master is independent:
 * ordered line/quadratic/cubic centerline geometry in [0,5] x [0,7], with
 * explicit stroke width and topology. It is not reconstructed from the bits. */
const sf57_glyph *sf57_find_glyph(uint32_t name);
int sf57_validate_glyph(const sf57_glyph *glyph);
int sf57_bitmap_cell(const sf57_glyph *glyph, unsigned column, unsigned row);

/* Didactic reference sampler for the masters. Each output word is one final
 * 5x7 cell. Bit 8*sample_y+sample_x corresponds to the midpoint
 * (column+(sample_x+0.5)/8, row+(sample_y+0.5)/8). The output changes only on
 * success. reverse_parameterization must be 0 or 1. This sampler is not the
 * qualified whole-path producer owned by Flexible Pipes #145. */
int sf57_sample_masks(const sf57_glyph *glyph, int reverse_parameterization,
                      uint64_t masks[SF57_CELL_COUNT],
                      unsigned char counts[SF57_CELL_COUNT]);

#endif
