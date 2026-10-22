#ifndef ROUGH_PRESENT_H
#define ROUGH_PRESENT_H

#include "framebuffer.h"

#include <stdint.h>

/* Present a caller-owned straight-alpha 0xAARRGGBB software framebuffer to
 * the byte-ordered Android WINDOW_FORMAT_RGBA_8888 layout. Destination stride
 * is measured in four-byte RGBA pixels, not bytes; padding remains untouched.
 *
 * Pixel centers are aligned and bilinearly filtered in premultiplied-alpha
 * space, then returned as straight-alpha RGBA bytes. This avoids importing
 * transparent RGB colors into visible edges. No depth values are modified.
 *
 * The destination must actually have stride_pixels * height * four bytes
 * allocated; as with rough_framebuffer, the pointer alone cannot prove buffer
 * capacity. Input dimensions/strides are validated before any writes.
 * Return 0 on success, -1 for malformed arguments or temporary allocation
 * failure; no image-sized intermediary is allocated.
 */
int rough_present_rgba8888(const rough_framebuffer *source,
                           uint8_t *destination,
                           unsigned destination_width,
                           unsigned destination_height,
                           unsigned destination_stride_pixels);

#endif
