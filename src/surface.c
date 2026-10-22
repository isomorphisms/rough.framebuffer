#include "surface.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>

#define SURFACE_NEAR 0.5
#define SURFACE_MAX_DIMENSION 8192u

typedef struct {
    double x, y, z;
} point3;

typedef struct {
    double x, y, depth;
} projected_vertex;

static point3 difference(point3 a, point3 b) {
    point3 result = {a.x - b.x, a.y - b.y, a.z - b.z};
    return result;
}

static point3 cross(point3 a, point3 b) {
    point3 result = {a.y * b.z - a.z * b.y,
                     a.z * b.x - a.x * b.z,
                     a.x * b.y - a.y * b.x};
    return result;
}

static double clamp_unit(double value) {
    if (value < 0.0) return 0.0;
    if (value > 1.0) return 1.0;
    return value;
}

/* Torus has two periodic parameters, and Enneper's minimal surface is sampled
 * on a square patch. Neither shape requires a triangle/mesh library. */
static point3 sample_surface(rough_surface_shape shape, double u, double v) {
    if (shape == ROUGH_SURFACE_TORUS) {
        double radius = 1.18 + 0.43 * cos(v);
        point3 p = {radius * cos(u), radius * sin(u), 0.43 * sin(v)};
        return p;
    }

    double u2 = u * u;
    double v2 = v * v;
    point3 p = {0.85 * (u - u * u2 / 3.0 + u * v2),
                0.85 * (v - v * v2 / 3.0 + u2 * v),
                0.85 * (u2 - v2)};
    return p;
}

/* Right-handed yaw around y, followed by fixed pitch around x. The camera
 * looks along positive z. The fixed pitch makes angle zero visibly three-
 * dimensional; a caller-controlled yaw supplies frame-to-frame rotation. */
static point3 to_camera(point3 p, double sine, double cosine, double distance) {
    const double pitch = 0.42;
    double sx = sin(pitch);
    double cx = cos(pitch);
    double x = cosine * p.x + sine * p.z;
    double z = -sine * p.x + cosine * p.z;
    point3 result = {x, cx * p.y - sx * z,
                     sx * p.y + cx * z + distance};
    return result;
}

static point3 interpolate(point3 a, point3 b, double t) {
    point3 result = {a.x + t * (b.x - a.x),
                     a.y + t * (b.y - a.y),
                     a.z + t * (b.z - a.z)};
    return result;
}

/* Sutherland-Hodgman clipping against z >= NEAR. Always interpolate from the
 * outside endpoint toward the inside endpoint, independent of edge traversal
 * direction, so shared clipped edges produce identical vertex coordinates. */
static unsigned clip_near(const point3 input[3], point3 output[4]) {
    unsigned count = 0;
    point3 previous = input[2];
    int previous_inside = previous.z >= SURFACE_NEAR;
    for (unsigned i = 0; i < 3; ++i) {
        point3 current = input[i];
        int current_inside = current.z >= SURFACE_NEAR;
        if (current_inside != previous_inside) {
            point3 outside = current_inside ? previous : current;
            point3 inside = current_inside ? current : previous;
            double t = (SURFACE_NEAR - outside.z) / (inside.z - outside.z);
            point3 intersection = interpolate(outside, inside, t);
            intersection.z = SURFACE_NEAR;
            output[count++] = intersection;
        }
        if (current_inside) output[count++] = current;
        previous = current;
        previous_inside = current_inside;
    }
    return count;
}

/* A perspective depth must be affine in screen coordinates. The value
 * 1 - NEAR/z is in [0,1) for visible points, equals zero at the near plane,
 * and grows with distance, matching rough_fill_triangle's smaller-is-nearer
 * depth comparison and its screen-space barycentric interpolation. */
static projected_vertex project(point3 p, unsigned width, unsigned height) {
    double extent = (double)(width < height ? width : height);
    double focal = 0.88 * extent;
    projected_vertex vertex = {
        ((double)width - 1.0) * 0.5 + focal * p.x / p.z,
        ((double)height - 1.0) * 0.5 - focal * p.y / p.z,
        1.0 - SURFACE_NEAR / p.z
    };
    return vertex;
}

static unsigned color_byte(double base, double illumination) {
    double value = floor(base * illumination + 0.5);
    if (value <= 0.0) return 0;
    if (value >= 255.0) return 255;
    return (unsigned)value;
}

static int emit_face(FILE *output, rough_framebuffer *target,
                     const point3 camera[3], double u, double v,
                     unsigned width, unsigned height) {
    point3 normal = cross(difference(camera[1], camera[0]),
                          difference(camera[2], camera[0]));
    double normal_length = sqrt(normal.x * normal.x +
                                normal.y * normal.y +
                                normal.z * normal.z);
    if (!(normal_length > 1e-12) || !isfinite(normal_length)) return 0;

    /* Double-sided diffuse shading: Enneper is an open patch, so neither side
     * should turn black merely because the parameterization is reversed. */
    const point3 light = {-0.42, -0.55, -0.72};
    const double light_length = 0.998649087151; /* approximate; normalized below */
    double dot = normal.x * light.x + normal.y * light.y + normal.z * light.z;
    double diffuse = clamp_unit(fabs(dot) / (normal_length * light_length));
    double illumination = 0.20 + 0.80 * diffuse;
    /* The torus identifies 0 with 2*pi in both parameters: integer
     * harmonics avoid a visible color tear at either periodic seam. */
    double warm = 0.5 + 0.5 * sin(u);
    double cool = 0.5 + 0.5 * cos(2.0 * v);
    unsigned red = color_byte(55.0 + 165.0 * warm, illumination);
    unsigned green = color_byte(65.0 + 155.0 * cool, illumination);
    unsigned blue = color_byte(75.0 + 165.0 * (1.0 - warm), illumination);

    point3 clipped[4];
    unsigned count = clip_near(camera, clipped);
    for (unsigned i = 1; i + 1 < count; ++i) {
        projected_vertex a = project(clipped[0], width, height);
        projected_vertex b = project(clipped[i], width, height);
        projected_vertex c = project(clipped[i + 1], width, height);
        double area = (b.x - a.x) * (c.y - a.y) -
                      (b.y - a.y) * (c.x - a.x);
        if (!isfinite(area)) return -1;
        if (fabs(area) <= 1e-10) continue;
        if (output) {
            if (fprintf(output,
                        "P %u %u %u\n"
                        "T %.17g %.17g %.17g %.17g %.17g %.17g %.17g %.17g %.17g\n",
                        red, green, blue,
                        a.x, a.y, a.depth,
                        b.x, b.y, b.depth,
                        c.x, c.y, c.depth) < 0) return -1;
        } else {
            rough_vertex v0 = {a.x, a.y, a.depth};
            rough_vertex v1 = {b.x, b.y, b.depth};
            rough_vertex v2 = {c.x, c.y, c.depth};
            uint32_t color = 0xff000000u | ((uint32_t)red << 16) |
                             ((uint32_t)green << 8) | (uint32_t)blue;
            if (rough_fill_triangle(target, v0, v1, v2, color, 1.0))
                return -1;
        }
    }
    return 0;
}

static int walk_surface(FILE *output, rough_framebuffer *target,
                        rough_surface_shape shape, double degrees,
                        unsigned width, unsigned height, double camera_distance) {
    if ((!output && (!target || !target->pixels || target->width != width ||
                      target->height != height)) ||
        (shape != ROUGH_SURFACE_TORUS && shape != ROUGH_SURFACE_ENNEPER) ||
        !isfinite(degrees) || !isfinite(camera_distance) ||
        !(camera_distance > 0.0) || camera_distance > 100.0 ||
        !width || !height ||
        width > SURFACE_MAX_DIMENSION || height > SURFACE_MAX_DIMENSION) return -1;

    const double pi = 3.14159265358979323846264338327950288;
    double yaw = fmod(degrees, 360.0) * (pi / 180.0);
    double sine = sin(yaw);
    double cosine = cos(yaw);
    unsigned steps_u = shape == ROUGH_SURFACE_TORUS ? 56u : 48u;
    unsigned steps_v = shape == ROUGH_SURFACE_TORUS ? 28u : 48u;

    for (unsigned i = 0; i < steps_u; ++i) {
        for (unsigned j = 0; j < steps_v; ++j) {
            double u0, u1, v0, v1, middle_u, middle_v;
            if (shape == ROUGH_SURFACE_TORUS) {
                u0 = (2.0 * pi * (double)i) / (double)steps_u;
                u1 = (2.0 * pi * (double)((i + 1u) % steps_u)) / (double)steps_u;
                v0 = (2.0 * pi * (double)j) / (double)steps_v;
                v1 = (2.0 * pi * (double)((j + 1u) % steps_v)) / (double)steps_v;
                middle_u = (2.0 * pi * ((double)i + 0.5)) / (double)steps_u;
                middle_v = (2.0 * pi * ((double)j + 0.5)) / (double)steps_v;
            } else {
                u0 = -1.24 + 2.48 * (double)i / (double)steps_u;
                u1 = -1.24 + 2.48 * (double)(i + 1u) / (double)steps_u;
                v0 = -1.24 + 2.48 * (double)j / (double)steps_v;
                v1 = -1.24 + 2.48 * (double)(j + 1u) / (double)steps_v;
                middle_u = 0.5 * (u0 + u1);
                middle_v = 0.5 * (v0 + v1);
            }

            point3 corners[4] = {
                to_camera(sample_surface(shape, u0, v0), sine, cosine, camera_distance),
                to_camera(sample_surface(shape, u1, v0), sine, cosine, camera_distance),
                to_camera(sample_surface(shape, u1, v1), sine, cosine, camera_distance),
                to_camera(sample_surface(shape, u0, v1), sine, cosine, camera_distance)
            };
            point3 first[3] = {corners[0], corners[1], corners[2]};
            point3 second[3] = {corners[0], corners[2], corners[3]};
            if (emit_face(output, target, first, middle_u, middle_v, width, height) ||
                emit_face(output, target, second, middle_u, middle_v, width, height)) return -1;
        }
    }
    return output && ferror(output) ? -1 : 0;
}

int rough_emit_surface(FILE *output, rough_surface_shape shape, double degrees,
                       unsigned width, unsigned height, double camera_distance) {
    if (!output) return -1;
    return walk_surface(output, NULL, shape, degrees, width, height, camera_distance);
}

int rough_draw_surface(rough_framebuffer *target, rough_surface_shape shape,
                       double degrees, double camera_distance) {
    if (!target || !target->pixels) return -1;
    return walk_surface(NULL, target, shape, degrees,
                        target->width, target->height, camera_distance);
}
