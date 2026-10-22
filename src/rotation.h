#ifndef ROUGH_ROTATION_H
#define ROUGH_ROTATION_H

#include <math.h>

/* Unit quaternion, scalar first. Active right-handed model-to-camera rotation.
 * Gesture deltas multiply on the left, so axes follow the screen after any
 * previous rotations. No Euler angles are stored or recovered. */
typedef struct { double w, x, y, z; } rough_rotation;

static inline int rough_rotation_normalize(rough_rotation q,
                                            rough_rotation *result) {
    double length = hypot(hypot(q.w, q.x), hypot(q.y, q.z));
    if (!result || !isfinite(length) || length < 1e-12) return -1;
    *result = (rough_rotation){q.w/length,q.x/length,q.y/length,q.z/length};
    return 0;
}

static inline rough_rotation rough_rotation_product(rough_rotation a,
                                                     rough_rotation b) {
    return (rough_rotation){
        a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z,
        a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,
        a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,
        a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w};
}

static inline int rough_rotation_drag(rough_rotation anchor,
                                      double horizontal, double vertical,
                                      rough_rotation *result) {
    double angle = hypot(horizontal, vertical);
    if (!isfinite(angle)) return -1;
    if (angle == 0.0) return rough_rotation_normalize(anchor, result);
    double scale = sin(0.5 * angle) / angle;
    rough_rotation delta = {cos(0.5 * angle), vertical*scale,
                             -horizontal*scale, 0.0};
    return rough_rotation_normalize(rough_rotation_product(delta, anchor), result);
}

/* Derived once per frame, for the existing triangle walk and analytic normals. */
static inline int rough_rotation_matrix(rough_rotation q, double m[9]) {
    if (!m || rough_rotation_normalize(q, &q)) return -1;
    m[0]=1-2*(q.y*q.y+q.z*q.z); m[1]=2*(q.x*q.y-q.w*q.z); m[2]=2*(q.x*q.z+q.w*q.y);
    m[3]=2*(q.x*q.y+q.w*q.z); m[4]=1-2*(q.x*q.x+q.z*q.z); m[5]=2*(q.y*q.z-q.w*q.x);
    m[6]=2*(q.x*q.z-q.w*q.y); m[7]=2*(q.y*q.z+q.w*q.x); m[8]=1-2*(q.x*q.x+q.y*q.y);
    return 0;
}

#endif
