#ifndef ROUGH_SURFACE_AA_H
#define ROUGH_SURFACE_AA_H

#include "surface.h"

/* Resolve a 2x2 pixel grid into one image pixel. The higher-resolution
 * framebuffer contains independent color AND depth samples: each subpixel is
 * depth tested before coverage is resolved. A single center-sample depth
 * behind a partially covered silhouette would produce wrong occlusion.
 *
 * Colors are stored as straight-alpha 0xAARRGGBB and averaged in
 * premultiplied-alpha space. The caller owns all memory and strides.
 * Color and depth buffers are required in both framebuffers. Returns -1 for
 * incompatible or invalid layouts without silently changing sample count.
 */
int rough_resolve_2x2(const rough_framebuffer *samples,
                      rough_framebuffer *destination);

/* The scratch image must be exactly twice the destination dimensions,
 * with independent depth for each of the four subpixels per destination
 * pixel. The scratch is cleared to opaque white and +infinity depth on every
 * call. The same native rough_draw_surface() renders at the high resolution,
 * then rough_resolve_2x2() resolves into the caller's existing framebuffer.
 *
 * This is real 2x2 coverage at the software framebuffer's resolution;
 * a final scale-up to the Android window cannot create new geometry.
 * The first C67/A1 slice retains face-flat colors; smooth normals are a
 * separate unresolved part of rough.framebuffer issue #5.
 */
int rough_draw_surface_aa2(rough_framebuffer *destination,
                           rough_framebuffer *scratch,
                           rough_surface_shape shape,
                           double angle_degrees,
                           double camera_distance);

#endif
