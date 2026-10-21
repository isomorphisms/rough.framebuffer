#include "subpixel_framebuffer.h"

#include <limits.h>
#include <math.h>
#include <stddef.h>

static unsigned occupied_samples(uint64_t mask) {
    unsigned count = 0;
    while (mask) { mask &= mask - UINT64_C(1); ++count; }
    return count;
}

static unsigned rounded_byte(long double byte_value) {
    return (unsigned)floorl(byte_value + 0.5L);
}

int rough_blend_subpixel_coverage(rough_framebuffer *target,
    const cp_coverage_plane *coverage, uint32_t color, double opacity) {
    if (!target || !target->pixels || !target->width || !target->height ||
        target->width > INT_MAX || target->height > INT_MAX ||
        target->stride_pixels < target->width ||
        !isfinite(opacity) || opacity < 0.0 || opacity > 1.0 ||
        cp_coverage_plane_validate(coverage) != CP_OK) return -1;
    size_t last_column = (size_t)target->width - 1;
    size_t last_row = (size_t)target->height - 1;
    if (last_row > (SIZE_MAX - last_column) / target->stride_pixels)
        return -1;
    if (last_row * target->stride_pixels + last_column >=
        SIZE_MAX / sizeof(uint32_t)) return -1;
    /* Keep byte alpha and the 64-sample denominator explicit. Normalizing
     * through /255 and multiplying back can move an exact half-byte below
     * its rounding boundary (e.g. alpha 64, 33 samples, opacity 1/2). */
    long double source_base_units = ((color >> 24) & 255u) * (long double)opacity;
    if (source_base_units == 0.0L) return 0;
    for (unsigned row = 0; row < coverage->height; ++row) {
        int64_t pixel_y = (int64_t)coverage->origin_y + row;
        if (pixel_y < 0 || pixel_y >= target->height) continue;
        for (unsigned column = 0; column < coverage->width; ++column) {
            int64_t pixel_x = (int64_t)coverage->origin_x + column;
            if (pixel_x < 0 || pixel_x >= target->width) continue;
            uint64_t mask = coverage->occupied[(size_t)row *
                coverage->stride_words + column];
            long double source_units = source_base_units * occupied_samples(mask);
            if (source_units == 0.0L) continue;
            size_t index = (size_t)pixel_y * target->stride_pixels + (size_t)pixel_x;
            uint32_t destination = target->pixels[index];
            long double source_weight = 255.0L * source_units;
            long double destination_weight = ((destination >> 24) & 255u) *
                (255.0L * 64.0L - source_units);
            long double total_weight = source_weight + destination_weight;
            uint32_t output = (uint32_t)(rounded_byte(total_weight / (255.0L * 64.0L)) << 24);
            for (unsigned shift = 0; shift < 24; shift += 8) {
                unsigned source = (color >> shift) & 255u;
                unsigned background = (destination >> shift) & 255u;
                long double component = (source * source_weight +
                    background * destination_weight) / total_weight;
                output |= (uint32_t)(rounded_byte(component) << shift);
            }
            target->pixels[index] = output;
        }
    }
    return 0;
}
