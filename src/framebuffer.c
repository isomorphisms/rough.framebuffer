#include "framebuffer.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STROKE_WIDTH 1000000.0
#define CURVE_TOLERANCE 0.25
#define CURVE_MAX_DEPTH 18

typedef struct {
    double x;
    double y;
} point;

/* Parse all numeric fields while retaining the stream's strict no-junk rule.
 * A negative result means malformed input or too many fields. */
static int parse_numbers(const char *text, double *values, int capacity) {
    int count = 0;
    char *end;
    for (;;) {
        while (isspace((unsigned char)*text)) ++text;
        if (!*text) return count;
        if (count == capacity) return -1;
        errno = 0;
        values[count] = strtod(text, &end);
        if (text == end || errno == ERANGE || !isfinite(values[count])) return -1;
        ++count;
        text = end;
    }
}

static double clamp_unit(double value) {
    if (value <= 0.0) return 0.0;
    if (value >= 1.0) return 1.0;
    return value;
}

static unsigned byte_from_unit(double value) {
    return (unsigned)floor(clamp_unit(value) * 255.0 + 0.5);
}

/* Straight-alpha source-over composition into the documented AARRGGBB surface. */
static void blend_pixel(rough_framebuffer *fb, int x, int y, uint32_t color, double alpha) {
    if (x < 0 || y < 0 || (unsigned)x >= fb->width || (unsigned)y >= fb->height) return;
    double source_alpha = clamp_unit(alpha);
    if (source_alpha == 0.0) return;

    uint32_t destination = fb->pixels[(size_t)y * fb->stride_pixels + (unsigned)x];
    double destination_alpha = ((destination >> 24) & 255u) / 255.0;
    double output_alpha = source_alpha + destination_alpha * (1.0 - source_alpha);

    double source_red = ((color >> 16) & 255u) / 255.0;
    double source_green = ((color >> 8) & 255u) / 255.0;
    double source_blue = (color & 255u) / 255.0;
    double destination_red = ((destination >> 16) & 255u) / 255.0;
    double destination_green = ((destination >> 8) & 255u) / 255.0;
    double destination_blue = (destination & 255u) / 255.0;

    double destination_gain = destination_alpha * (1.0 - source_alpha);
    double output_red = 0.0;
    double output_green = 0.0;
    double output_blue = 0.0;
    if (output_alpha > 0.0) {
        output_red = (source_red * source_alpha + destination_red * destination_gain) / output_alpha;
        output_green = (source_green * source_alpha + destination_green * destination_gain) / output_alpha;
        output_blue = (source_blue * source_alpha + destination_blue * destination_gain) / output_alpha;
    }

    fb->pixels[(size_t)y * fb->stride_pixels + (unsigned)x] =
        (uint32_t)(byte_from_unit(output_alpha) << 24) |
        (uint32_t)(byte_from_unit(output_red) << 16) |
        (uint32_t)(byte_from_unit(output_green) << 8) |
        (uint32_t)byte_from_unit(output_blue);
}

static int valid_width(double width) {
    return width > 0.0 && width <= MAX_STROKE_WIDTH && isfinite(width);
}

/* Clip the centerline against the target enlarged by the largest radius.
 * Widths are interpolated to the clipped endpoints, preserving a taper. */
static int clip_segment(rough_framebuffer *fb,
                        double *x0, double *y0, double *x1, double *y1,
                        double *width0, double *width1) {
    double margin = fmax(*width0, *width1) * 0.5 + 0.5;
    double left = -margin;
    double right = (double)fb->width - 1.0 + margin;
    double top = -margin;
    double bottom = (double)fb->height - 1.0 + margin;
    double dx = *x1 - *x0;
    double dy = *y1 - *y0;
    if (!isfinite(dx) || !isfinite(dy)) return 0;

    double p[4] = {-dx, dx, -dy, dy};
    double q[4] = {*x0 - left, right - *x0, *y0 - top, bottom - *y0};
    double start = 0.0;
    double stop = 1.0;
    for (int k = 0; k < 4; ++k) {
        if (p[k] == 0.0) {
            if (q[k] < 0.0) return 0;
        } else {
            double ratio = q[k] / p[k];
            if (p[k] < 0.0) {
                if (ratio > stop) return 0;
                if (ratio > start) start = ratio;
            } else {
                if (ratio < start) return 0;
                if (ratio < stop) stop = ratio;
            }
        }
    }

    double original_x = *x0;
    double original_y = *y0;
    double original_width = *width0;
    double width_delta = *width1 - *width0;
    *x0 = original_x + start * dx;
    *y0 = original_y + start * dy;
    *x1 = original_x + stop * dx;
    *y1 = original_y + stop * dy;
    *width0 = original_width + start * width_delta;
    *width1 = original_width + stop * width_delta;
    return isfinite(*x0) && isfinite(*y0) && isfinite(*x1) && isfinite(*y1) &&
           valid_width(*width0) && valid_width(*width1);
}

/* Pixel centers lie on integer coordinates, preserving the original fixture's
 * mapping. Coverage uses a one-pixel linear antialiasing fringe. */
static void stroke_segment(rough_framebuffer *fb,
                           double x0, double y0, double x1, double y1,
                           double width0, double width1,
                           uint32_t color, double opacity) {
    if (!clip_segment(fb, &x0, &y0, &x1, &y1, &width0, &width1)) return;

    double maximum_radius = fmax(width0, width1) * 0.5;
    double minimum_x = fmin(x0, x1) - maximum_radius - 0.5;
    double maximum_x = fmax(x0, x1) + maximum_radius + 0.5;
    double minimum_y = fmin(y0, y1) - maximum_radius - 0.5;
    double maximum_y = fmax(y0, y1) + maximum_radius + 0.5;
    if (maximum_x < 0.0 || maximum_y < 0.0 ||
        minimum_x > (double)fb->width - 1.0 || minimum_y > (double)fb->height - 1.0) return;

    int first_x = (int)fmax(0.0, floor(minimum_x));
    int last_x = (int)fmin((double)fb->width - 1.0, ceil(maximum_x));
    int first_y = (int)fmax(0.0, floor(minimum_y));
    int last_y = (int)fmin((double)fb->height - 1.0, ceil(maximum_y));
    double dx = x1 - x0;
    double dy = y1 - y0;
    double length_square = dx * dx + dy * dy;

    for (int y = first_y; y <= last_y; ++y) {
        for (int x = first_x; x <= last_x; ++x) {
            double parameter = 0.0;
            if (length_square > 0.0 && isfinite(length_square)) {
                parameter = (((double)x - x0) * dx + ((double)y - y0) * dy) / length_square;
                parameter = clamp_unit(parameter);
            }
            double nearest_x = x0 + parameter * dx;
            double nearest_y = y0 + parameter * dy;
            double radius = (width0 + parameter * (width1 - width0)) * 0.5;
            double distance = hypot((double)x - nearest_x, (double)y - nearest_y);
            double coverage = clamp_unit(radius + 0.5 - distance);
            blend_pixel(fb, x, y, color, opacity * coverage);
        }
    }
}

static point midpoint(point a, point b) {
    point result = {a.x * 0.5 + b.x * 0.5, a.y * 0.5 + b.y * 0.5};
    return result;
}

static double distance_from_chord(point p, point start, point end) {
    double dx = end.x - start.x;
    double dy = end.y - start.y;
    double length = hypot(dx, dy);
    if (!isfinite(length)) return INFINITY;
    if (length == 0.0) return hypot(p.x - start.x, p.y - start.y);
    double unit_x = dx / length;
    double unit_y = dy / length;
    return fabs((p.x - start.x) * unit_y - (p.y - start.y) * unit_x);
}

/* A cubic is contained in the convex hull of its controls, so this cull cannot
 * discard a visible curve. */
static int cubic_outside(rough_framebuffer *fb, point p0, point p1, point p2, point p3,
                         double width0, double width1) {
    double margin = fmax(width0, width1) * 0.5 + 0.5;
    if (p0.x < -margin && p1.x < -margin && p2.x < -margin && p3.x < -margin) return 1;
    if (p0.y < -margin && p1.y < -margin && p2.y < -margin && p3.y < -margin) return 1;
    double right = (double)fb->width - 1.0 + margin;
    double bottom = (double)fb->height - 1.0 + margin;
    if (p0.x > right && p1.x > right && p2.x > right && p3.x > right) return 1;
    if (p0.y > bottom && p1.y > bottom && p2.y > bottom && p3.y > bottom) return 1;
    return 0;
}

static void stroke_cubic(rough_framebuffer *fb,
                         point p0, point p1, point p2, point p3,
                         double width0, double width1,
                         uint32_t color, double opacity, int depth) {
    if (cubic_outside(fb, p0, p1, p2, p3, width0, width1)) return;
    double chord_length = hypot(p3.x - p0.x, p3.y - p0.y);
    double control_length = hypot(p1.x - p0.x, p1.y - p0.y) +
                            hypot(p2.x - p1.x, p2.y - p1.y) +
                            hypot(p3.x - p2.x, p3.y - p2.y);
    double excess_length = control_length - chord_length;
    double flatness = fmax(distance_from_chord(p1, p0, p3),
                           distance_from_chord(p2, p0, p3));
    int accept_chord = isfinite(excess_length) && flatness <= CURVE_TOLERANCE &&
                       excess_length <= CURVE_TOLERANCE;
    if (depth >= CURVE_MAX_DEPTH || accept_chord) {
        stroke_segment(fb, p0.x, p0.y, p3.x, p3.y, width0, width1, color, opacity);
        return;
    }

    point p01 = midpoint(p0, p1);
    point p12 = midpoint(p1, p2);
    point p23 = midpoint(p2, p3);
    point p012 = midpoint(p01, p12);
    point p123 = midpoint(p12, p23);
    point center = midpoint(p012, p123);
    double middle_width = (width0 + width1) * 0.5;
    stroke_cubic(fb, p0, p01, p012, center, width0, middle_width,
                 color, opacity, depth + 1);
    stroke_cubic(fb, center, p123, p23, p3, middle_width, width1,
                 color, opacity, depth + 1);
}

int rough_render_stream(FILE *input, rough_framebuffer *fb) {
    if (!input || !fb || !fb->pixels || !fb->width || !fb->height || fb->stride_pixels < fb->width ||
        fb->width > 1000000 || fb->height > 1000000) return -1;

    uint32_t color = 0xff000000u;
    double opacity = 1.0;
    double width = 1.0;
    double x = 0.0;
    double y = 0.0;
    int pen = 0;
    char line[512];

    while (fgets(line, sizeof line, input)) {
        size_t length = strlen(line);
        if (!length) return -2;
        if (line[length - 1] != '\n' && !feof(input)) return -2;
        if (line[0] == '#' || line[0] == '\n') continue;

        double fields[7] = {0.0};
        int count = parse_numbers(line + 1, fields, 7);
        if (count < 0) return -2;
        switch (line[0]) {
        case 'P':
            if (count != 3) return -2;
            for (int i = 0; i < 3; ++i) {
                if (fields[i] < 0.0 || fields[i] > 255.0 || fields[i] != floor(fields[i])) return -2;
            }
            color = 0xff000000u | ((uint32_t)fields[0] << 16) |
                    ((uint32_t)fields[1] << 8) | (uint32_t)fields[2];
            break;
        case 'A':
            if (count != 1 || fields[0] < 0.0 || fields[0] > 1.0) return -2;
            opacity = fields[0];
            break;
        case 'W':
            if (count != 1 || !valid_width(fields[0])) return -2;
            width = fields[0];
            break;
        case 'M':
            if (count != 2) return -2;
            x = fields[0];
            y = fields[1];
            pen = 1;
            break;
        case 'L': {
            if (!pen || (count != 2 && count != 3)) return -2;
            double end_width = count == 3 ? fields[2] : width;
            if (!valid_width(end_width)) return -2;
            stroke_segment(fb, x, y, fields[0], fields[1], width, end_width, color, opacity);
            x = fields[0];
            y = fields[1];
            width = end_width;
            break;
        }
        case 'C': {
            if (!pen || (count != 6 && count != 7)) return -2;
            double end_width = count == 7 ? fields[6] : width;
            if (!valid_width(end_width)) return -2;
            point p0 = {x, y};
            point p1 = {fields[0], fields[1]};
            point p2 = {fields[2], fields[3]};
            point p3 = {fields[4], fields[5]};
            stroke_cubic(fb, p0, p1, p2, p3, width, end_width, color, opacity, 0);
            x = fields[4];
            y = fields[5];
            width = end_width;
            break;
        }
        default:
            return -2;
        }
    }
    return ferror(input) ? -3 : 0;
}
