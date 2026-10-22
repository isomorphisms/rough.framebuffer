#include "touch.h"
#include "framebuffer.h"

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIDTH 160u
#define HEIGHT 160u

static uint64_t render_frame(const rough_touch_state *state, unsigned *painted) {
    const size_t count = (size_t)WIDTH * HEIGHT;
    uint32_t *pixels = malloc(count * sizeof *pixels);
    float *depth = malloc(count * sizeof *depth);
    assert(pixels && depth);
    for (size_t i = 0; i < count; ++i) {
        pixels[i] = 0xffffffffu;
        depth[i] = INFINITY;
    }
    rough_framebuffer fb = {pixels, WIDTH, HEIGHT, WIDTH, depth, WIDTH};
    assert(rough_draw_surface(&fb, state->shape, state->yaw_degrees,
                              state->camera_distance) == 0);
    *painted = 0u;
    uint64_t checksum = UINT64_C(14695981039346656037);
    for (size_t i = 0; i < count; ++i) {
        if (pixels[i] != 0xffffffffu) {
            ++*painted;
            assert(isfinite(depth[i]) && depth[i] >= 0.0f && depth[i] < 1.0f);
        } else assert(isinf(depth[i]));
        checksum ^= (uint64_t)pixels[i];
        checksum *= UINT64_C(1099511628211);
    }
    assert(*painted > 100u);
    free(depth);
    free(pixels);
    return checksum;
}

int main(void) {
    rough_touch_state touch;
    assert(rough_touch_init(NULL, WIDTH, HEIGHT) == -1);
    assert(rough_touch_init(&touch, 0, HEIGHT) == -1);
    assert(rough_touch_init(&touch, WIDTH, HEIGHT) == 0);
    assert(touch.shape == ROUGH_SURFACE_TORUS && touch.camera_distance == 5.2);
    unsigned painted;
    uint64_t initial = render_frame(&touch, &painted);

    /* Motion without a down is inert; an unknown pointer cannot steal drag. */
    assert(rough_touch_apply(&touch, ROUGH_TOUCH_MOVE, 0, 60, 40) == 0);
    assert(rough_touch_apply(&touch, ROUGH_TOUCH_DOWN, 7, 20, 40) == 0);
    assert(rough_touch_apply(&touch, ROUGH_TOUCH_MOVE, 8, 90, 40) == 0);
    assert(rough_touch_apply(&touch, ROUGH_TOUCH_UP, 8, 90, 40) == 0);
    assert(rough_touch_apply(&touch, ROUGH_TOUCH_MOVE, 7, 25, 40) == 0);
    assert(touch.yaw_degrees == 0.0); /* deadzone prevents accidental rotation */
    assert(rough_touch_apply(&touch, ROUGH_TOUCH_MOVE, 7, 60, 40) == 1);
    /* Camera +z points inward: right drag must grab the near half.
     * The old +45 assertion encoded the exact inverted-hand bug. */
    assert(fabs(touch.yaw_degrees - 315.0) < 1e-12);
    uint64_t rotated = render_frame(&touch, &painted);
    assert(rotated != initial && painted > 100u);

    /* Cancelling the only active pointer restores the exact original frame. */
    assert(rough_touch_apply(&touch, ROUGH_TOUCH_CANCEL, 7, 60, 40) == 1);
    assert(touch.active == 0 && touch.yaw_degrees == 0.0);
    assert(render_frame(&touch, &painted) == initial);

    /* A tap changes the mathematical shape, not just a UI log variable. */
    assert(rough_touch_apply(&touch, ROUGH_TOUCH_DOWN, 1, 50, 40) == 0);
    assert(rough_touch_apply(&touch, ROUGH_TOUCH_UP, 1, 52, 41) == 1);
    assert(touch.shape == ROUGH_SURFACE_ENNEPER);
    uint64_t enneper = render_frame(&touch, &painted);
    assert(enneper != initial);
    assert(rough_touch_apply(&touch, ROUGH_TOUCH_DOWN, 1, 50, 40) == 0);
    assert(rough_touch_apply(&touch, ROUGH_TOUCH_UP, 1, 50, 40) == 1);
    assert(touch.shape == ROUGH_SURFACE_TORUS);
    assert(render_frame(&touch, &painted) == initial);

    /* Vertical movement changes the camera; distance remains bounded. */
    assert(rough_touch_apply(&touch, ROUGH_TOUCH_DOWN, 1, 20, 20) == 0);
    assert(rough_touch_apply(&touch, ROUGH_TOUCH_MOVE, 1, 20, 80) == 1);
    assert(fabs(touch.camera_distance - 6.325) < 1e-12);
    assert(render_frame(&touch, &painted) != initial);
    assert(rough_touch_apply(&touch, ROUGH_TOUCH_MOVE, 1, 20, 20000) == 1);
    assert(touch.camera_distance == 12.0);
    assert(rough_touch_apply(&touch, ROUGH_TOUCH_UP, 1, 20, 20000) == 0);
    assert(touch.active == 0);

    /* A surface resize invalidates a pending gesture and preserves view. */
    assert(rough_touch_apply(&touch, ROUGH_TOUCH_DOWN, 1, 20, 40) == 0);
    assert(rough_touch_resize(&touch, 0, HEIGHT) == -1);
    assert(rough_touch_resize(&touch, 240, 320) == 0);
    assert(touch.active == 0 && touch.camera_distance == 12.0);

    assert(rough_touch_apply(&touch, ROUGH_TOUCH_DOWN, 0, NAN, 0) == -1);
    assert(rough_touch_apply(&touch, ROUGH_TOUCH_DOWN, -1, 0, 0) == -1);
    assert(rough_touch_apply(&touch, (rough_touch_event)99, 0, 0, 0) == -1);

    /* Exact span ratios, not pointer indices or motion direction: spreading
     * twice as far halves camera distance and changes actual painted pixels. */
    assert(rough_touch_init(&touch, WIDTH, HEIGHT) == 0);
    initial = render_frame(&touch, &painted);
    assert(rough_touch_apply(&touch, ROUGH_TOUCH_DOWN, 17, 40, 80) == 0);
    assert(rough_touch_apply(&touch, ROUGH_TOUCH_DOWN, 93, 80, 80) == 0);
    assert(touch.pinching && touch.active_pointer_id == 17);
    assert(rough_touch_apply(&touch, ROUGH_TOUCH_DOWN, 4, 10, 10) == 0);
    assert(rough_touch_apply(&touch, ROUGH_TOUCH_MOVE, 4, 150, 150) == 0);
    assert(rough_touch_apply(&touch, ROUGH_TOUCH_MOVE, 93, 120, 80) == 1);
    assert(fabs(touch.camera_distance - 2.6) < 1e-12);
    assert(touch.yaw_degrees == 0.0 && touch.shape == ROUGH_SURFACE_TORUS);
    assert(render_frame(&touch, &painted) != initial);
    /* Pinching together zooms out, with upper/lower and zero-span bounds. */
    assert(rough_touch_apply(&touch, ROUGH_TOUCH_MOVE, 93, 60, 80) == 1);
    assert(fabs(touch.camera_distance - 10.4) < 1e-12);
    assert(rough_touch_apply(&touch, ROUGH_TOUCH_MOVE, 93, 40, 80) == 1);
    assert(touch.camera_distance == 12.0);
    assert(rough_touch_apply(&touch, ROUGH_TOUCH_MOVE, 93, 1000, 80) == 1);
    assert(touch.camera_distance == 2.4);
    assert(rough_touch_apply(&touch, ROUGH_TOUCH_CANCEL, 17, 40, 80) == 1);
    assert(!touch.active && !touch.pinching && touch.camera_distance == 5.2);
    assert(render_frame(&touch, &painted) == initial);
    /* Either lift ends the pinch, without a tap or a remaining-finger jump. */
    for (unsigned primary_lifts = 0; primary_lifts < 2; ++primary_lifts) {
        assert(rough_touch_apply(&touch, ROUGH_TOUCH_DOWN, 17, 40, 80) == 0);
        assert(rough_touch_apply(&touch, ROUGH_TOUCH_DOWN, 93, 80, 80) == 0);
        assert(rough_touch_apply(&touch, ROUGH_TOUCH_UP,
                                primary_lifts ? 17 : 93, 40, 80) == 0);
        assert(rough_touch_apply(&touch, ROUGH_TOUCH_MOVE,
                                primary_lifts ? 93 : 17, 140, 10) == 0);
        assert(touch.shape == ROUGH_SURFACE_TORUS && touch.yaw_degrees == 0.0);
        assert(!touch.active && !touch.pinching);
    }
    /* Both fingers moving uses their separation, independent of IDs. */
    assert(rough_touch_apply(&touch, ROUGH_TOUCH_DOWN, 93, 40, 40) == 0);
    assert(rough_touch_apply(&touch, ROUGH_TOUCH_DOWN, 17, 40, 80) == 0);
    assert(rough_touch_apply(&touch, ROUGH_TOUCH_MOVE, 93, 40, 20) == 1);
    assert(rough_touch_apply(&touch, ROUGH_TOUCH_MOVE, 17, 40, 100) == 1);
    assert(fabs(touch.camera_distance - 2.6) < 1e-12);
    assert(rough_touch_resize(&touch, 240, 320) == 0 && !touch.pinching);
    puts("native gesture -> changed framebuffer pixels/depth: PASS");
    return 0;
}
