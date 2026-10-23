#ifndef SUBPIXEL_FONT_H
#define SUBPIXEL_FONT_H
#include <stddef.h>
#include <stdint.h>

/* Canonical 5x7 continuous masters and whole-path rasterizer. Return 1 on
 * success, 0 on refusal. Output is changed only on success. A space yields a
 * transparent mask. Unsupported Unicode scalars are refused, never substituted.
 * Caller supplies 35*scale*scale bytes; row stride is 5*scale. Phase is measured
 * in destination pixels and must be finite and between -0.5 and 0.5 inclusive.
 * Geometry, thickness and baseline are identical at every scale/phase.
 * Cache by (codepoint, scale, phase_x, phase_y), excluding foreground color. */
int sf57_render_coverage(uint32_t codepoint, unsigned scale,
    double phase_x, double phase_y, uint8_t *coverage, size_t capacity);
#endif
