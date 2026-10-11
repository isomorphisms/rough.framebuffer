#include "surface_aa.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>

/* Pixel-space AA for CPU surfaces without changing rough_fill_triangle().
 * Color and depth are independently tested at four physical subpixels before
 * an opaque white composited image is reduced to display resolution.
 * Deliberately bounded: 4 * 360,000 = 1,440,000 subpixels, or 11.5 MiB for
 * color + depth. The current C67 NativeActivity uses at most 180,000 base
 * pixels, so the scratch is at most 5.76 MiB plus the base framebuffer.
 */
#define AA_MAX_BASE_PIXELS 360000u
#define AA_MAX_SOURCE_DIM 8192u

static int valid_surface(const rough_framebuffer *surface) {
    if (!surface || !surface->pixels || !surface->depth ||
        !surface->width || !surface->height ||
        surface->width > AA_MAX_SOURCE_DIM ||
        surface->height > AA_MAX_SOURCE_DIM ||
        surface->stride_pixels < surface->width ||
        surface->depth_stride < surface->width) return 0;

    size_t row = (size_t)surface->height - 1u;
    size_t last = (size_t)surface->width - 1u;
    if (row > (SIZE_MAX - last) / (size_t)surface->stride_pixels ||
        row > (SIZE_MAX - last) / (size_t)surface->depth_stride)
        return 0;
    if (row * (size_t)surface->stride_pixels + last >
        SIZE_MAX / sizeof(uint32_t) - 1u) return 0;
    if (row * (size_t)surface->depth_stride + last >
        SIZE_MAX / sizeof(float) - 1u) return 0;
    return 1;
}

static int valid_pair(const rough_framebuffer *samples,
                      const rough_framebuffer *destination) {
    if (!valid_surface(samples) || !valid_surface(destination)) return 0;
    if ((size_t)destination->width * (size_t)destination->height >
        AA_MAX_BASE_PIXELS) return 0;
    if (destination->width > AA_MAX_SOURCE_DIM / 2u ||
        destination->height > AA_MAX_SOURCE_DIM / 2u ||
        samples->width != destination->width * 2u ||
        samples->height != destination->height * 2u) return 0;
    return 1;
}

static unsigned component(uint32_t color, unsigned shift) {
    return (color >> shift) & 255u;
}

static unsigned round_div(unsigned sum, unsigned total) {
    if (!total) return 0u;
    unsigned result = (sum + total / 2u) / total;
    return result > 255u ? 255u : result;
}

int rough_resolve_2x2(const rough_framebuffer *samples,
                      rough_framebuffer *destination) {
    if (!valid_pair(samples, destination)) return -1;

    /* Preflight all depth samples before writing any destination pixels.
     * +INFINITY represents the untouched background; NaNs, negative depth
     * and -INFINITY must never silently pass as an occluded surface. */
    for (unsigned y = 0; y < samples->height; ++y) {
        for (unsigned x = 0; x < samples->width; ++x) {
            float d = samples->depth[(size_t)y * samples->depth_stride + x];
            if (isnan(d) || d < 0.0f) return -1;
        }
    }

    const unsigned shifts[3] = {16u, 8u, 0u};
    for (unsigned y = 0; y < destination->height; ++y) {
        size_t high_top = (size_t)(y * 2u) * samples->stride_pixels;
        size_t high_bottom = high_top + samples->stride_pixels;
        size_t depth_top = (size_t)(y * 2u) * samples->depth_stride;
        size_t depth_bottom = depth_top + samples->depth_stride;
        for (unsigned x = 0; x < destination->width; ++x) {
            size_t x2 = (size_t)x * 2u;
            uint32_t colors[4] = {
                samples->pixels[high_top + x2],
                samples->pixels[high_top + x2 + 1u],
                samples->pixels[high_bottom + x2],
                samples->pixels[high_bottom + x2 + 1u]
            };
            float depth[4] = {
                samples->depth[depth_top + x2],
                samples->depth[depth_top + x2 + 1u],
                samples->depth[depth_bottom + x2],
                samples->depth[depth_bottom + x2 + 1u]
            };

            unsigned alpha_sum = 0u;
            for (unsigned i = 0; i < 4u; ++i)
                alpha_sum += component(colors[i], 24u);

            unsigned alpha = round_div(alpha_sum, 4u);
            uint32_t argb = (uint32_t)alpha << 24u;
            for (unsigned channel = 0; channel < 3u; ++channel) {
                unsigned numerator = 0u;
                for (unsigned i = 0; i < 4u; ++i)
                    numerator += component(colors[i], shifts[channel]) *
                                 component(colors[i], 24u);
                argb |= (uint32_t)round_div(numerator, alpha_sum)
                        << shifts[channel];
            }

            float nearest = INFINITY;
            for (unsigned i = 0; i < 4u; ++i)
                if (depth[i] < nearest) nearest = depth[i];
            destination->pixels[(size_t)y * destination->stride_pixels + x] =
                argb;
            destination->depth[(size_t)y * destination->depth_stride + x] =
                nearest;
        }
    }
    return 0;
}

int rough_draw_surface_aa2(rough_framebuffer *destination,
                           rough_framebuffer *scratch,
                           rough_surface_shape shape,
                           double angle_degrees,
                           double camera_distance) {
    if (!valid_pair(scratch, destination) ||
        !isfinite(angle_degrees) || !isfinite(camera_distance) ||
        (shape != ROUGH_SURFACE_TORUS &&
         shape != ROUGH_SURFACE_ENNEPER)) return -1;

    /* Scratch is caller-owned and reusable across frames. Each subpixel
     * retains an independent depth test until the entire frame is resolved.
     * No blend of a foreground edge with a distant rear face is attempted
     * before those two rays have had independent z-buffer results. */
    for (unsigned y = 0; y < scratch->height; ++y) {
        for (unsigned x = 0; x < scratch->width; ++x) {
            scratch->pixels[(size_t)y * scratch->stride_pixels + x] =
                UINT32_C(0xffffffff);
            scratch->depth[(size_t)y * scratch->depth_stride + x] = INFINITY;
        }
    }

    if (rough_draw_surface_smooth(scratch, shape, angle_degrees, camera_distance))
        return -1;
    return rough_resolve_2x2(scratch, destination);
}
