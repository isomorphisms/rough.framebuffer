#ifndef ROUGH_TOUCH_H
#define ROUGH_TOUCH_H

#include "surface.h"

/* This file has no Android SDK dependency. The Android input adapter maps
 * motion action/pointer IDs to this reducer, and native host tests exercise
 * the same state transitions with real framebuffer output.
 *
 * A horizontal drag rotates the surface; vertical drag changes camera
 * distance (a bounded zoom). A tap toggles torus/Enneper. Gesture cancellation
 * restores the pre-gesture view. Only the first down pointer owns a gesture.
 */
typedef enum {
    ROUGH_TOUCH_DOWN = 1,
    ROUGH_TOUCH_MOVE = 2,
    ROUGH_TOUCH_UP = 3,
    ROUGH_TOUCH_CANCEL = 4
} rough_touch_event;

typedef struct {
    rough_surface_shape shape;
    double yaw_degrees;
    double camera_distance;
    int active;
    int active_pointer_id;
    int moved;
    unsigned width;
    unsigned height;
    double down_x, down_y;
    double anchor_yaw, anchor_distance;
} rough_touch_state;

/* Return 0 on valid state or -1 on bad dimensions/target. */
int rough_touch_init(rough_touch_state *state, unsigned width, unsigned height);
int rough_touch_resize(rough_touch_state *state, unsigned width, unsigned height);

/* Return 1 when pixels must be rendered again, 0 if no visible state change,
 * -1 for invalid input. An ignored secondary pointer returns 0. */
int rough_touch_apply(rough_touch_state *state, rough_touch_event event,
                      int pointer_id, double x, double y);

#endif
