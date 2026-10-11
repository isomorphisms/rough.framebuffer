/* Object-in-hand drag semantics for the current positive-z-into-screen
 * perspective and torus geometry. Do not replace this with a "hash changed"
 * test: the original implementation rendered changing pictures while moving
 * the BACK hemisphere in the drag direction.
 *
 * This oracle derives the movement of two *actual torus surface points* at
 * u = pi/2 and v = +/-pi/2 from the independently documented camera geometry:
 * x_world=0, y_world=1.18, z_world=+/-0.43. The negative-z point is nearer.
 */
#include "touch.h"
#include "framebuffer.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define SCREEN_W 160u
#define SCREEN_H 160u

static int wrong(const char *reason) {
    fprintf(stderr, "FRONT_HEMISPHERE_WRONG_DIRECTION: %s\n", reason);
    return 1;
}
#define REQUIRE(test, reason) do { if (!(test)) return wrong(reason); } while (0)

static double torus_landmark_x(double yaw_deg, double z_world,
                                unsigned width, unsigned height) {
    const double pi = 3.14159265358979323846264338327950288;
    double radians = yaw_deg * pi / 180.0;
    double z_camera = 5.2 + sin(0.42) * 1.18 +
                      cos(0.42) * cos(radians) * z_world;
    double focal = 0.88 * (double)(width < height ? width : height);
    return ((double)width - 1.0) / 2.0 +
           focal * sin(radians) * z_world / z_camera;
}

static uint64_t frame_hash(const rough_touch_state *state) {
    uint32_t pixels[SCREEN_W * SCREEN_H];
    float depth[SCREEN_W * SCREEN_H];
    for (unsigned i = 0; i < SCREEN_W * SCREEN_H; ++i) {
        pixels[i] = 0xffffffffu;
        depth[i] = INFINITY;
    }
    rough_framebuffer frame = {pixels, SCREEN_W, SCREEN_H, SCREEN_W,
                               depth, SCREEN_W};
    if (rough_draw_surface(&frame, state->shape, state->yaw_degrees,
                           state->camera_distance) != 0) return 0;
    uint64_t hash = UINT64_C(14695981039346656037);
    unsigned foreground = 0;
    for (unsigned i = 0; i < SCREEN_W * SCREEN_H; ++i) {
        hash ^= (uint64_t)pixels[i];
        hash *= UINT64_C(1099511628211);
        if (pixels[i] != 0xffffffffu) ++foreground;
    }
    return foreground > 100u ? hash : 0;
}

static int test_direction(unsigned width, unsigned height,
                          double initial_yaw, double dx, int id) {
    rough_touch_state state;
    REQUIRE(rough_touch_init(&state, width, height) == 0,
            "could not initialize aspect ratio");
    state.yaw_degrees = initial_yaw;

    double start_near = torus_landmark_x(initial_yaw, -0.43, width, height);
    double start_far = torus_landmark_x(initial_yaw, 0.43, width, height);
    REQUIRE(rough_touch_apply(&state, ROUGH_TOUCH_DOWN, id, 70.0, 70.0) == 0,
            "down rejected");
    REQUIRE(rough_touch_apply(&state, ROUGH_TOUCH_MOVE, id,
                               70.0 + dx, 70.0) == 1,
            "drag had no visible state change");
    double finish_near = torus_landmark_x(state.yaw_degrees, -0.43,
                                          width, height);
    double finish_far = torus_landmark_x(state.yaw_degrees, 0.43,
                                         width, height);
    if (dx > 0.0) {
        REQUIRE(finish_near > start_near + 0.1,
                "right drag moves NEAR torus surface left");
        REQUIRE(finish_far < start_far - 0.1,
                "right drag moves FAR torus surface right");
    } else {
        REQUIRE(finish_near < start_near - 0.1,
                "left drag moves NEAR torus surface right");
        REQUIRE(finish_far > start_far + 0.1,
                "left drag moves FAR torus surface left");
    }
    REQUIRE(frame_hash(&state) != 0, "rotation has no renderable surface");
    REQUIRE(rough_touch_apply(&state, ROUGH_TOUCH_UP, id,
                               70.0 + dx, 70.0) == 0,
            "unexpected touch-up side effect");
    return 0;
}

int main(void) {
    /* Actual screen sides must follow the finger at two aspect ratios
     * and at nonzero orientation, not just at the old symmetric pose. */
    if (test_direction(160, 160, 0.0, 40.0, 3)) return 1;
    if (test_direction(160, 160, 0.0, -40.0, 3)) return 1;
    if (test_direction(160, 320, 25.0, 35.0, 9)) return 1;
    if (test_direction(320, 160, 25.0, -70.0, 9)) return 1;

    /* A second pointer must not affect the first owner's orientation.
     * A cancellation must undo the current gesture. Vertical drag is zoom,
     * and must not silently reverse or advance the yaw. */
    rough_touch_state state;
    REQUIRE(rough_touch_init(&state, 160, 320) == 0, "bad init");
    REQUIRE(rough_touch_apply(&state, ROUGH_TOUCH_DOWN, 1, 30, 60) == 0,
            "bad primary down");
    REQUIRE(rough_touch_apply(&state, ROUGH_TOUCH_DOWN, 2, 50, 60) == 0,
            "secondary pointer stole gesture");
    REQUIRE(rough_touch_apply(&state, ROUGH_TOUCH_MOVE, 2, 120, 60) == 0,
            "secondary finger rotated object");
    REQUIRE(state.yaw_degrees == 0.0, "secondary pointer changed yaw");
    REQUIRE(rough_touch_apply(&state, ROUGH_TOUCH_MOVE, 1, 70, 60) == 1,
            "primary drag ignored");
    REQUIRE(state.yaw_degrees > 270.0 && state.yaw_degrees < 360.0,
            "yaw did not rotate by front-grab sign");
    REQUIRE(rough_touch_apply(&state, ROUGH_TOUCH_CANCEL, 1, 70, 60) == 1,
            "cancel did not restore original pose");
    REQUIRE(state.yaw_degrees == 0.0, "cancel retained rotation");

    REQUIRE(rough_touch_apply(&state, ROUGH_TOUCH_DOWN, 1, 30, 60) == 0,
            "zoom down failed");
    REQUIRE(rough_touch_apply(&state, ROUGH_TOUCH_MOVE, 1, 30, 90) == 1,
            "vertical drag did not zoom");
    REQUIRE(state.yaw_degrees == 0.0 && state.camera_distance > 5.2,
            "vertical drag incorrectly rotated");
    REQUIRE(rough_touch_apply(&state, ROUGH_TOUCH_UP, 1, 30, 90) == 0,
            "zoom up failed");
    puts("FRONT_HEMISPHERE_GRAB_PASS: near follows finger, back opposes");
    return 0;
}
