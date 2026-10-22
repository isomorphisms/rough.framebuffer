#include "present.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define PRESENT_MAX_DIMENSION 16384u
#define PRESENT_ONE 256u
#define PRESENT_AREA (PRESENT_ONE * PRESENT_ONE)

typedef struct {
    unsigned first;
    unsigned second;
    unsigned fraction; /* 0..256 inclusive, with 256 meaning second pixel */
} axis_sample;

/* Align pixel centers when the software image is scaled to an Android
 * window. Integer arithmetic avoids platform-specific floating rounding at
 * pixel centers and amortizes horizontal division over each destination row.
 */
static axis_sample sample_axis(unsigned output_index,
                               unsigned source_length,
                               unsigned output_length) {
    axis_sample result = {0u, 0u, 0u};
    int64_t numerator = ((int64_t)output_index * 2 + 1) *
                        (int64_t)source_length - (int64_t)output_length;
    int64_t denominator = (int64_t)output_length * 2;

    if (numerator <= 0 || source_length == 1u) return result;
    if (numerator >= denominator * ((int64_t)source_length - 1)) {
        result.first = source_length - 1u;
        result.second = result.first;
        return result;
    }

    result.first = (unsigned)(numerator / denominator);
    result.second = result.first + 1u;
    int64_t remainder = numerator % denominator;
    result.fraction = (unsigned)((remainder * PRESENT_ONE +
                                  denominator / 2) / denominator);
    return result;
}

static unsigned channel(uint32_t argb, unsigned shift) {
    return (argb >> shift) & 255u;
}

static unsigned rounded_channel(uint64_t sum, uint64_t denominator) {
    if (denominator == 0) return 0;
    unsigned value = (unsigned)((sum + denominator / 2u) / denominator);
    return value > 255u ? 255u : value;
}

/* Byte bounds are verified against SIZE_MAX; allocation length is an external
 * ownership obligation, shared with the original rough_framebuffer contract.
 */
static int valid_layout(unsigned width, unsigned height,
                        unsigned stride_pixels) {
    if (width == 0 || height == 0 ||
        width > PRESENT_MAX_DIMENSION ||
        height > PRESENT_MAX_DIMENSION ||
        stride_pixels < width) return 0;

    size_t row = (size_t)height - 1u;
    size_t column = (size_t)width - 1u;
    if (row > (SIZE_MAX - column) / (size_t)stride_pixels) return 0;
    size_t last_pixel = row * (size_t)stride_pixels + column;
    return last_pixel <= (SIZE_MAX - 3u) / 4u;
}

int rough_present_rgba8888(const rough_framebuffer *source,
                           uint8_t *destination,
                           unsigned destination_width,
                           unsigned destination_height,
                           unsigned destination_stride_pixels) {
    if (!source || !source->pixels || !destination ||
        !valid_layout(source->width, source->height, source->stride_pixels) ||
        !valid_layout(destination_width, destination_height,
                      destination_stride_pixels)) return -1;

    /* destination_width is already bounded to 16384, so the temporary
     * axis array fits in size_t on both 32-bit ARM and 64-bit hosts. */
    axis_sample *columns = malloc((size_t)destination_width * sizeof *columns);
    if (!columns) return -1;
    for (unsigned x = 0; x < destination_width; ++x)
        columns[x] = sample_axis(x, source->width, destination_width);

    const unsigned shifts[3] = {16u, 8u, 0u};
    for (unsigned y = 0; y < destination_height; ++y) {
        axis_sample ys = sample_axis(y, source->height, destination_height);
        size_t row0 = (size_t)ys.first * source->stride_pixels;
        size_t row1 = (size_t)ys.second * source->stride_pixels;
        size_t destination_row = (size_t)y * destination_stride_pixels * 4u;

        unsigned w_top = PRESENT_ONE - ys.fraction;
        unsigned w_bottom = ys.fraction;
        for (unsigned x = 0; x < destination_width; ++x) {
            axis_sample xs = columns[x];
            unsigned w_left = PRESENT_ONE - xs.fraction;
            unsigned w_right = xs.fraction;
            uint64_t weights[4] = {
                (uint64_t)w_top * w_left,
                (uint64_t)w_top * w_right,
                (uint64_t)w_bottom * w_left,
                (uint64_t)w_bottom * w_right
            };
            uint32_t pixels[4] = {
                source->pixels[row0 + xs.first],
                source->pixels[row0 + xs.second],
                source->pixels[row1 + xs.first],
                source->pixels[row1 + xs.second]
            };
            unsigned alpha[4] = {
                channel(pixels[0], 24u), channel(pixels[1], 24u),
                channel(pixels[2], 24u), channel(pixels[3], 24u)
            };

            uint8_t *rgba = destination + destination_row + (size_t)x * 4u;
            if (alpha[0] == 255u && alpha[1] == 255u &&
                alpha[2] == 255u && alpha[3] == 255u) {
                /* Current mathematical art is opaque, so use a cheaper
                 * exact integer path on every ordinary framebuffer pixel. */
                for (unsigned component = 0; component < 3u; ++component) {
                    uint64_t sum = 0u;
                    for (unsigned index = 0; index < 4u; ++index)
                        sum += weights[index] *
                               channel(pixels[index], shifts[component]);
                    rgba[component] = (uint8_t)rounded_channel(sum, PRESENT_AREA);
                }
                rgba[3] = 255u;
            } else {
                /* Premultiply before interpolation and unpremultiply after:
                 * transparent red adjacent to opaque blue must stay blue,
                 * not develop a red fringe. */
                uint64_t alpha_sum = 0u;
                for (unsigned index = 0; index < 4u; ++index)
                    alpha_sum += weights[index] * alpha[index];

                rgba[3] = (uint8_t)rounded_channel(alpha_sum, PRESENT_AREA);
                for (unsigned component = 0; component < 3u; ++component) {
                    uint64_t premultiplied_sum = 0u;
                    for (unsigned index = 0; index < 4u; ++index)
                        premultiplied_sum += weights[index] * alpha[index] *
                            channel(pixels[index], shifts[component]);
                    rgba[component] =
                        (uint8_t)rounded_channel(premultiplied_sum, alpha_sum);
                }
            }
        }
    }

    free(columns);
    return 0;
}
