#ifndef ROUGH_SURFACE_H
#define ROUGH_SURFACE_H

#include <stdio.h>
#include "framebuffer.h"

/* Mathematical sampling is a source of the existing framebuffer P/T operation
 * stream, not a second renderer. A positive camera z points into the scene.
 * The output uses screen-space x/y and depth = 1 - NEAR/z, so smaller depth
 * represents nearer geometry under rough_fill_triangle's depth contract.
 */
typedef enum {
    ROUGH_SURFACE_TORUS = 1,
    ROUGH_SURFACE_ENNEPER = 2
} rough_surface_shape;

/* Emit a complete, reproducible P/T stream for one angle. The caller owns out.
 * Near-plane clipping precedes perspective division; the same vertex is
 * shared across each pair of mesh triangles. No framebuffer is allocated.
 * Returns 0 on success and -1 for invalid inputs or output errors.
 */
int rough_emit_surface(FILE *out, rough_surface_shape shape, double degrees,
                       unsigned width, unsigned height, double camera_distance);

/* Render the same geometry and colors directly into caller-owned pixels/depth,
 * without serializing triangle operations to a temporary file. No implicit
 * framebuffer clear or ownership transfer occurs. Explicit normal/alpha
 * semantics match the P/T stream exactly. */
int rough_draw_surface(rough_framebuffer *target, rough_surface_shape shape,
                       double degrees, double camera_distance);

/* Alternative direct rasterized surface appearance: true per-fragment
 * diffuse lighting from perspective-correctly interpolated ANALYTIC normals.
 * The face geometry, top-left coverage and z-buffer have the same meaning
 * as rough_draw_surface; the legacy P/T stream remains flat-shaded.
 * Requires a caller-owned depth buffer and does not clear either buffer.
 */
int rough_draw_surface_smooth(rough_framebuffer *target,
                              rough_surface_shape shape,
                              double degrees, double camera_distance);

#endif
