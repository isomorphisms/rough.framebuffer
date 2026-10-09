#include "framebuffer.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static int numbers(const char *text, double *values, int count) {
    char *end;
    for (int i = 0; i < count; ++i) {
        while (isspace((unsigned char)*text)) ++text;
        if (!*text) return 0;
        errno = 0;
        values[i] = strtod(text, &end);
        if (text == end || errno == ERANGE || !isfinite(values[i])) return 0;
        text = end;
    }
    while (isspace((unsigned char)*text)) ++text;
    return *text == '\0';
}

static void dot(rough_framebuffer *fb, int x, int y, uint32_t color) {
    if (x < 0 || y < 0 || (unsigned)x >= fb->width || (unsigned)y >= fb->height) return;
    fb->pixels[(size_t)y * fb->stride_pixels + (unsigned)x] = color;
}

/* Clip before integer conversion and rasterization. Extremely remote strokes
 * can thus be skipped without huge loops or integer conversion overflow. */
static int clip(rough_framebuffer *fb, double *x0, double *y0, double *x1, double *y1) {
    double dx = *x1 - *x0, dy = *y1 - *y0;
    double p[4] = {-dx, dx, -dy, dy};
    double q[4] = {*x0, fb->width - 1.0 - *x0, *y0, fb->height - 1.0 - *y0};
    double start = 0, stop = 1;
    for (int k = 0; k < 4; ++k) {
        if (p[k] == 0) {
            if (q[k] < 0) return 0;
        } else {
            double r = q[k] / p[k];
            if (p[k] < 0) {
                if (r > stop) return 0;
                if (r > start) start = r;
            } else {
                if (r < start) return 0;
                if (r < stop) stop = r;
            }
        }
    }
    double sx = *x0, sy = *y0;
    *x0 = sx + start * dx;
    *y0 = sy + start * dy;
    *x1 = sx + stop * dx;
    *y1 = sy + stop * dy;
    return isfinite(*x0) && isfinite(*y0) && isfinite(*x1) && isfinite(*y1);
}

static void segment(rough_framebuffer *fb, double x0, double y0, double x1, double y1, uint32_t color) {
    if (!clip(fb, &x0, &y0, &x1, &y1)) return;
    int x = (int)lround(x0), y = (int)lround(y0);
    int end_x = (int)lround(x1), end_y = (int)lround(y1);
    int dx = abs(end_x - x), dy = abs(end_y - y);
    int sx = x < end_x ? 1 : -1, sy = y < end_y ? 1 : -1;
    int err = dx - dy;
    for (;;) {
        dot(fb, x, y, color);
        if (x == end_x && y == end_y) break;
        int twice = 2 * err;
        if (twice > -dy) { err -= dy; x += sx; }
        if (twice < dx) { err += dx; y += sy; }
    }
}

static double cubic(double p0, double p1, double p2, double p3, double t) {
    double s = 1.0 - t;
    return s*s*s*p0 + 3*s*s*t*p1 + 3*s*t*t*p2 + t*t*t*p3;
}

static void bezier(rough_framebuffer *fb, double x0, double y0, const double *p, uint32_t color) {
    double length = hypot(p[0]-x0, p[1]-y0) + hypot(p[2]-p[0], p[3]-p[1]) + hypot(p[4]-p[2], p[5]-p[3]);
    if (!isfinite(length)) return;
    int steps = (int)fmin(512.0, fmax(16.0, length / 2.0));
    for (int i = 1; i <= steps; ++i) {
        double t = (double)i / steps;
        double x = cubic(x0, p[0], p[2], p[4], t);
        double y = cubic(y0, p[1], p[3], p[5], t);
        segment(fb, x0, y0, x, y, color);
        x0 = x; y0 = y;
    }
}

int rough_render_stream(FILE *input, rough_framebuffer *fb) {
    if (!input || !fb || !fb->pixels || !fb->width || !fb->height || fb->stride_pixels < fb->width ||
        fb->width > 1000000 || fb->height > 1000000) return -1;
    uint32_t color = 0xff000000;
    double x = 0, y = 0;
    int pen = 0;
    char line[512];
    while (fgets(line, sizeof line, input)) {
        size_t n = strlen(line);
        if (!n) return -2;
        if (line[n-1] != '\n' && !feof(input)) return -2; /* no truncated command */
        if (line[0] == '#' || line[0] == '\n') continue;
        double p[6] = {0};
        switch (line[0]) {
        case 'P':
            if (!numbers(line+1, p, 3)) return -2;
            for (int i=0; i<3; ++i) {
                if (p[i] < 0 || p[i] > 255 || p[i] != floor(p[i])) return -2;
            }
            color = 0xff000000u | ((uint32_t)p[0]<<16) | ((uint32_t)p[1]<<8) | (uint32_t)p[2];
            break;
        case 'M':
            if (!numbers(line+1, p, 2)) return -2;
            x = p[0]; y = p[1]; pen = 1;
            break;
        case 'L':
            if (!pen || !numbers(line+1, p, 2)) return -2;
            segment(fb, x, y, p[0], p[1], color);
            x = p[0]; y = p[1];
            break;
        case 'C':
            if (!pen || !numbers(line+1, p, 6)) return -2;
            bezier(fb, x, y, p, color);
            x = p[4]; y = p[5];
            break;
        default: return -2;
        }
    }
    return ferror(input) ? -3 : 0;
}
