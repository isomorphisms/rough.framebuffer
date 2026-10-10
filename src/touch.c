#include "touch.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

#define TOUCH_MAX_DIMENSION 16384u
#define TOUCH_DRAG_THRESHOLD 8.0
#define TOUCH_MIN_CAMERA_DISTANCE 2.4
#define TOUCH_MAX_CAMERA_DISTANCE 12.0

static double bounded(double value, double minimum, double maximum) {
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

static double normalized_angle(double degrees) {
    double angle = fmod(degrees, 360.0);
    if (angle < 0.0) angle += 360.0;
    return angle;
}

static int dimensions_ok(unsigned width, unsigned height) {
    return width > 0 && width <= TOUCH_MAX_DIMENSION &&
           height > 0 && height <= TOUCH_MAX_DIMENSION;
}

int rough_touch_init(rough_touch_state *state, unsigned width, unsigned height) {
    if (!state || !dimensions_ok(width, height)) return -1;
    memset(state, 0, sizeof *state);
    state->width = width;
    state->height = height;
    state->shape = ROUGH_SURFACE_TORUS;
    state->camera_distance = 5.2;
    state->active_pointer_id = -1;
    return 0;
}

int rough_touch_resize(rough_touch_state *state, unsigned width, unsigned height) {
    if (!state || !dimensions_ok(width, height)) return -1;
    state->width = width;
    state->height = height;
    state->active = 0;                 /* previous coordinate frame is invalid */
    state->active_pointer_id = -1;
    state->moved = 0;
    return 0;
}

int rough_touch_apply(rough_touch_state *state, rough_touch_event event,
                      int pointer_id, double x, double y) {
    if (!state || !dimensions_ok(state->width, state->height) ||
        pointer_id < 0 || !isfinite(x) || !isfinite(y) ||
        (event != ROUGH_TOUCH_DOWN && event != ROUGH_TOUCH_MOVE &&
         event != ROUGH_TOUCH_UP && event != ROUGH_TOUCH_CANCEL)) return -1;

    if (event == ROUGH_TOUCH_DOWN) {
        if (state->active) return 0; /* another finger does not seize control */
        state->active = 1;
        state->active_pointer_id = pointer_id;
        state->moved = 0;
        state->down_x = x;
        state->down_y = y;
        state->anchor_yaw = state->yaw_degrees;
        state->anchor_distance = state->camera_distance;
        return 0;
    }
    if (!state->active || state->active_pointer_id != pointer_id) return 0;

    if (event == ROUGH_TOUCH_CANCEL) {
        int changed = (state->yaw_degrees != state->anchor_yaw ||
                       state->camera_distance != state->anchor_distance);
        state->yaw_degrees = state->anchor_yaw;
        state->camera_distance = state->anchor_distance;
        state->active = 0;
        state->active_pointer_id = -1;
        state->moved = 0;
        return changed;
    }

    double dx = x - state->down_x;
    double dy = y - state->down_y;
    if (!isfinite(dx) || !isfinite(dy)) return -1;
    double threshold2 = TOUCH_DRAG_THRESHOLD * TOUCH_DRAG_THRESHOLD;
    /* Squaring extremely large but finite coordinates may overflow to
     * infinity; this still correctly exceeds the drag threshold. */
    if (dx * dx + dy * dy >= threshold2) state->moved = 1;
    int changed = 0;
    if (state->moved) {
        double angle = normalized_angle(state->anchor_yaw +
                        dx * (180.0 / (double)state->width));
        double distance = bounded(state->anchor_distance +
                          dy * (3.0 / (double)state->height),
                          TOUCH_MIN_CAMERA_DISTANCE, TOUCH_MAX_CAMERA_DISTANCE);
        if (!isfinite(angle) || !isfinite(distance)) return -1;
        changed = angle != state->yaw_degrees ||
                  distance != state->camera_distance;
        state->yaw_degrees = angle;
        state->camera_distance = distance;
    }

    if (event == ROUGH_TOUCH_UP) {
        if (!state->moved) {
            state->shape = state->shape == ROUGH_SURFACE_TORUS ?
                           ROUGH_SURFACE_ENNEPER : ROUGH_SURFACE_TORUS;
            changed = 1;
        }
        state->active = 0;
        state->active_pointer_id = -1;
        state->moved = 0;
    }
    return changed;
}
