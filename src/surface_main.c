#include "surface.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int finite_number(const char *text, double *number) {
    char *end;
    errno = 0;
    double value = strtod(text, &end);
    if (errno || end == text || *end || !isfinite(value)) return 0;
    *number = value;
    return 1;
}

static int dimension(const char *text, unsigned *number) {
    char *end;
    errno = 0;
    unsigned long value = strtoul(text, &end, 10);
    if (errno || end == text || *end || value < 1ul || value > 8192ul) return 0;
    *number = (unsigned)value;
    return 1;
}

int main(int argc, char **argv) {
    double degrees;
    double distance = 5.2;
    unsigned width, height;
    rough_surface_shape shape;

    if (argc != 5 && argc != 6) goto usage;
    if (strcmp(argv[1], "torus") == 0) shape = ROUGH_SURFACE_TORUS;
    else if (strcmp(argv[1], "enneper") == 0) shape = ROUGH_SURFACE_ENNEPER;
    else goto usage;

    if (!finite_number(argv[2], &degrees) ||
        !dimension(argv[3], &width) ||
        !dimension(argv[4], &height) ||
        (argc == 6 && !finite_number(argv[5], &distance))) goto usage;

    if (rough_emit_surface(stdout, shape, degrees, width, height, distance)) {
        fprintf(stderr, "could not generate surface stream\n");
        return 1;
    }
    return 0;

usage:
    fprintf(stderr,
            "usage: rough-surface torus|enneper angle_degrees width height [camera_distance]\n"
            "  emits shaded P/T triangles on stdout; default camera_distance = 5.2\n"
            "  pipe into rough-fb with the same width and height\n");
    return 2;
}
