#include "sublixel_framebuffer.h"

#include <limits.h>
#include <math.h>
#include <stddef.h>

static unsigned occupied_samples(uint64_t mask) {
    unsigned count = 0;
    while (mask) { mask &= mask - UINT64_C(1); ++count; }
    return count;
}

static unsigned rounded_byte(double unit_value) {
    return (unsigned)floor(unit_value * 255.0 + 0.5);
}

int rough_blend_sublixel_coverage(rough_framebuffer *target,
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
    double source_base_alpha = ((color >> 24) & 255u) / 255.0 * opacity;
    if (source_base_alpha == 0.0) return 0;
    for (unsigned row = 0; row < coverage->height; ++row) {
        int64_t pixel_y = (int64_t)coverage->origin_y + row;
        if (pixel_y < 0 || pixel_y >= target->height) continue;
        for (unsigned column = 0; column < coverage->width; ++column) {
            int64_t pixel_x = (int64_t)coverage->origin_x + column;
            if (pixel_x < 0 || pixel_x >= target->width) continue;
            uint64_t mask = coverage->occupied[(size_t)row *
                coverage->stride_words + column];
            double source_alpha = source_base_alpha * occupied_samples(mask) / 64.0;
            if (source_alpha == 0.0) continue;
            size_t index = (size_t)pixel_y * target->stride_pixels + (size_t)pixel_x;
            uint32_t destination = target->pixels[index];
            double destination_gain = ((destination >> 24) & 255u) / 255.0 *
                (1.0 - source_alpha);
            double output_alpha = source_alpha + destination_gain;
            uint32_t output = (uint32_t)(rounded_byte(output_alpha) << 24);
            for (unsigned shift = 0; shift < 24; shift += 8) {
                double source = ((color >> shift) & 255u) / 255.0;
                double background = ((destination >> shift) & 255u) / 255.0;
                double component = (source * source_alpha +
                    background * destination_gain) / output_alpha;
                output |= (uint32_t)(rounded_byte(component) << shift);
            }
            target->pixels[index] = output;
        }
    }
    return 0;
}
