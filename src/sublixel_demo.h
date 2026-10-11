#ifndef ROUGH_SUBLIXEL_DEMO_H
#define ROUGH_SUBLIXEL_DEMO_H

#include "framebuffer.h"

/* Test-only visual probe of independent Sublixel 3x3 vs 5x5 neighborhood
 * coverage. This DOES NOT antialias the surface's triangle silhouettes.
 * The caller owns pixels, and may keep existing surface/depth data.
 */
typedef struct {
    unsigned left_x, right_x, top_y, cell;
} rough_sublixel_demo_layout;

/* Paint two magnified coverage grids over the existing RGBA framebuffer.
 * Left: quadratic 3x3 on a 5x5 canvas; right: cubic 5x5. No heap,
 * no graphics dependencies, no touches/implicit allocations. Returns -1
 * on invalid/diminutive surfaces before any pixel mutation.
 */
int rough_sublixel_demo_draw(rough_framebuffer *frame,
                             rough_sublixel_demo_layout *out_layout);

#endif
