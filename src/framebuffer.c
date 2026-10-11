#include "framebuffer.h"

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SURFACE_DIMENSION 1000000u
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

static int valid_layout(unsigned width, unsigned height, unsigned stride, size_t element_size) {
    if (!width || !height || width > MAX_SURFACE_DIMENSION || height > MAX_SURFACE_DIMENSION ||
        stride < width) return 0;
    size_t row = (size_t)height - 1u;
    size_t column = (size_t)width - 1u;
    if (row > (SIZE_MAX - column) / (size_t)stride) return 0;
    size_t last_index = row * (size_t)stride + column;
    return last_index <= SIZE_MAX / element_size;
}

static int valid_target(const rough_framebuffer *fb) {
    if (!fb || !fb->pixels ||
        !valid_layout(fb->width, fb->height, fb->stride_pixels, sizeof *fb->pixels)) return 0;
    if (fb->depth &&
        !valid_layout(fb->width, fb->height, fb->depth_stride, sizeof *fb->depth)) return 0;
    return 1;
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

static double edge_value(rough_vertex start, rough_vertex end, double x, double y) {
    return (end.x - start.x) * (y - start.y) -
           (end.y - start.y) * (x - start.x);
}

/* With y increasing downward and positive signed area, these are the top and
 * left members of a half-open edge pair. Reversing an edge reverses ownership. */
static int top_left_edge(rough_vertex start, rough_vertex end) {
    double dx = end.x - start.x;
    double dy = end.y - start.y;
    return dy < 0.0 || (dy == 0.0 && dx > 0.0);
}

static int inside_edge(double value, int inclusive) {
    return value > 0.0 || (value == 0.0 && inclusive);
}

static int valid_vertex(rough_vertex vertex) {
    return isfinite(vertex.x) && isfinite(vertex.y) && isfinite(vertex.depth) &&
           fabs(vertex.depth) <= FLT_MAX;
}

int rough_fill_triangle(rough_framebuffer *fb,
                        rough_vertex vertex0,
                        rough_vertex vertex1,
                        rough_vertex vertex2,
                        uint32_t color,
                        double opacity) {
    if (!valid_target(fb) || !valid_vertex(vertex0) || !valid_vertex(vertex1) ||
        !valid_vertex(vertex2) || !isfinite(opacity) || opacity < 0.0 || opacity > 1.0) return -1;
    if (opacity == 0.0) return 0;

    double area = edge_value(vertex0, vertex1, vertex2.x, vertex2.y);
    if (!isfinite(area)) return -1;
    if (area == 0.0) return 0;
    if (area < 0.0) {
        rough_vertex temporary = vertex1;
        vertex1 = vertex2;
        vertex2 = temporary;
        area = -area;
    }

    double minimum_x = fmin(vertex0.x, fmin(vertex1.x, vertex2.x));
    double maximum_x = fmax(vertex0.x, fmax(vertex1.x, vertex2.x));
    double minimum_y = fmin(vertex0.y, fmin(vertex1.y, vertex2.y));
    double maximum_y = fmax(vertex0.y, fmax(vertex1.y, vertex2.y));
    if (maximum_x < 0.0 || maximum_y < 0.0 ||
        minimum_x > (double)fb->width - 1.0 || minimum_y > (double)fb->height - 1.0) return 0;

    int first_x = (int)ceil(fmax(0.0, minimum_x));
    int last_x = (int)floor(fmin((double)fb->width - 1.0, maximum_x));
    int first_y = (int)ceil(fmax(0.0, minimum_y));
    int last_y = (int)floor(fmin((double)fb->height - 1.0, maximum_y));
    if (first_x > last_x || first_y > last_y) return 0;

    int edge0_inclusive = top_left_edge(vertex1, vertex2);
    int edge1_inclusive = top_left_edge(vertex2, vertex0);
    int edge2_inclusive = top_left_edge(vertex0, vertex1);

    for (int y = first_y; y <= last_y; ++y) {
        for (int x = first_x; x <= last_x; ++x) {
            double edge0 = edge_value(vertex1, vertex2, (double)x, (double)y);
            double edge1 = edge_value(vertex2, vertex0, (double)x, (double)y);
            double edge2 = edge_value(vertex0, vertex1, (double)x, (double)y);
            if (!inside_edge(edge0, edge0_inclusive) ||
                !inside_edge(edge1, edge1_inclusive) ||
                !inside_edge(edge2, edge2_inclusive)) continue;

            double weight0 = edge0 / area;
            double weight1 = edge1 / area;
            double weight2 = edge2 / area;
            double fragment_depth = weight0 * vertex0.depth +
                                    weight1 * vertex1.depth +
                                    weight2 * vertex2.depth;
            if (!isfinite(fragment_depth) || fabs(fragment_depth) > FLT_MAX) return -1;

            if (fb->depth) {
                size_t depth_index = (size_t)y * fb->depth_stride + (unsigned)x;
                if (!(fragment_depth < (double)fb->depth[depth_index])) continue;
                fb->depth[depth_index] = (float)fragment_depth;
            }
            blend_pixel(fb, x, y, color, opacity);
        }
    }
    return 0;
}

/* Per-fragment Phong shading shares the original rasterizer's *actual*
 * barycentric edge tests, half-open top-left rule and closer-is-smaller
 * depth contract. Unlike the old P/T flat-color operation, normals and
 * material colors are perspective-correct and interpolated at every covered
 * sample. The AA2 wrapper runs this at each of four independently z-tested
 * subpixel centers; resolving foreground and background happens afterward.
 */
static int valid_smooth_vertex(rough_smooth_vertex v) {
    if (!valid_vertex(v.position) ||
        !isfinite(v.reciprocal_z) || !(v.reciprocal_z > 0.0) ||
        !isfinite(v.normal_x) || !isfinite(v.normal_y) ||
        !isfinite(v.normal_z) ||
        !isfinite(v.base_red) || !isfinite(v.base_green) ||
        !isfinite(v.base_blue)) return 0;
    return v.base_red >= 0.0 && v.base_red <= 255.0 &&
           v.base_green >= 0.0 && v.base_green <= 255.0 &&
           v.base_blue >= 0.0 && v.base_blue <= 255.0 &&
           v.normal_x * v.normal_x +
           v.normal_y * v.normal_y +
           v.normal_z * v.normal_z > 1e-12;
}

static unsigned smooth_byte(double value) {
    if (value <= 0.0) return 0u;
    if (value >= 255.0) return 255u;
    return (unsigned)floor(value + 0.5);
}

int rough_fill_smooth_triangle(rough_framebuffer *fb,
                               rough_smooth_vertex v0,
                               rough_smooth_vertex v1,
                               rough_smooth_vertex v2) {
    if (!valid_target(fb) || !fb->depth ||
        !valid_smooth_vertex(v0) ||
        !valid_smooth_vertex(v1) ||
        !valid_smooth_vertex(v2)) return -1;

    rough_vertex p0 = v0.position, p1 = v1.position, p2 = v2.position;
    double area = edge_value(p0, p1, p2.x, p2.y);
    if (!isfinite(area)) return -1;
    if (area == 0.0) return 0;
    if (area < 0.0) {
        rough_smooth_vertex temp = v1;
        v1 = v2;
        v2 = temp;
        p1 = v1.position;
        p2 = v2.position;
        area = -area;
    }

    double minimum_x = fmin(p0.x, fmin(p1.x, p2.x));
    double maximum_x = fmax(p0.x, fmax(p1.x, p2.x));
    double minimum_y = fmin(p0.y, fmin(p1.y, p2.y));
    double maximum_y = fmax(p0.y, fmax(p1.y, p2.y));
    if (maximum_x < 0.0 || maximum_y < 0.0 ||
        minimum_x > (double)fb->width - 1.0 ||
        minimum_y > (double)fb->height - 1.0) return 0;

    int first_x = (int)ceil(fmax(0.0, minimum_x));
    int last_x = (int)floor(fmin((double)fb->width - 1.0, maximum_x));
    int first_y = (int)ceil(fmax(0.0, minimum_y));
    int last_y = (int)floor(fmin((double)fb->height - 1.0, maximum_y));
    if (first_x > last_x || first_y > last_y) return 0;

    int edge0_inclusive = top_left_edge(p1, p2);
    int edge1_inclusive = top_left_edge(p2, p0);
    int edge2_inclusive = top_left_edge(p0, p1);

    /* The direction is a fixed camera-space light, normalized once.
     * Keeping this identical across triangles prevents lighting seams. */
    const double light_x = -0.42;
    const double light_y = -0.55;
    const double light_z = -0.72;
    const double light_length = 0.998649087151;
    for (int y = first_y; y <= last_y; ++y) {
        for (int x = first_x; x <= last_x; ++x) {
            double e0 = edge_value(p1, p2, (double)x, (double)y);
            double e1 = edge_value(p2, p0, (double)x, (double)y);
            double e2 = edge_value(p0, p1, (double)x, (double)y);
            if (!inside_edge(e0, edge0_inclusive) ||
                !inside_edge(e1, edge1_inclusive) ||
                !inside_edge(e2, edge2_inclusive)) continue;
            double w0 = e0 / area, w1 = e1 / area, w2 = e2 / area;
            double depth = w0 * p0.depth + w1 * p1.depth + w2 * p2.depth;
            if (!isfinite(depth) || fabs(depth) > FLT_MAX) return -1;
            size_t d_index = (size_t)y * fb->depth_stride + (unsigned)x;
            if (!(depth < (double)fb->depth[d_index])) continue;

            double k0 = w0 * v0.reciprocal_z;
            double k1 = w1 * v1.reciprocal_z;
            double k2 = w2 * v2.reciprocal_z;
            double total = k0 + k1 + k2;
            if (!isfinite(total) || !(total > 0.0)) return -1;

            /* Dividing each interpolated normal by total cancels during
             * normalization; this saves three fragment divisions. */
            double nx = k0 * v0.normal_x + k1 * v1.normal_x +
                        k2 * v2.normal_x;
            double ny = k0 * v0.normal_y + k1 * v1.normal_y +
                        k2 * v2.normal_y;
            double nz = k0 * v0.normal_z + k1 * v1.normal_z +
                        k2 * v2.normal_z;
            double normal_length = sqrt(nx * nx + ny * ny + nz * nz);
            if (!(normal_length > 1e-14) || !isfinite(normal_length))
                return -1;
            double lambert = fabs(nx * light_x + ny * light_y +
                                  nz * light_z) /
                             (normal_length * light_length);
            double illumination = 0.20 + 0.80 * clamp_unit(lambert);
            double red = (k0 * v0.base_red + k1 * v1.base_red +
                          k2 * v2.base_red) / total;
            double green = (k0 * v0.base_green + k1 * v1.base_green +
                            k2 * v2.base_green) / total;
            double blue = (k0 * v0.base_blue + k1 * v1.base_blue +
                           k2 * v2.base_blue) / total;
            if (!isfinite(red) || !isfinite(green) || !isfinite(blue))
                return -1;

            uint32_t color = UINT32_C(0xff000000) |
                             (uint32_t)(smooth_byte(red * illumination) << 16) |
                             (uint32_t)(smooth_byte(green * illumination) << 8) |
                             (uint32_t)smooth_byte(blue * illumination);
            fb->depth[d_index] = (float)depth;
            fb->pixels[(size_t)y * fb->stride_pixels + (unsigned)x] = color;
        }
    }
    return 0;
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
    if (!input || !valid_target(fb)) return -1;

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

        double fields[9] = {0.0};
        int count = parse_numbers(line + 1, fields, 9);
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
        case 'T': {
            if (count != 9) return -2;
            rough_vertex vertex0 = {fields[0], fields[1], fields[2]};
            rough_vertex vertex1 = {fields[3], fields[4], fields[5]};
            rough_vertex vertex2 = {fields[6], fields[7], fields[8]};
            if (rough_fill_triangle(fb, vertex0, vertex1, vertex2, color, opacity)) return -2;
            break;
        }
        default:
            return -2;
        }
    }
    return ferror(input) ? -3 : 0;
}
