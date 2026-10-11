/*
 * DEX-free Android NativeActivity adapter.
 *
 * NDK's android_native_app_glue owns lifecycle/input looper dispatch; this
 * application owns geometry, gesture state and pixels. No EGL, WebView, Java,
 * Gradle or alternate rasterizer. A native RGBA_8888 ANativeWindow buffer is
 * updated only when the mathematical view becomes dirty.
 */
#include <android/input.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android_native_app_glue.h>

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "framebuffer.h"
#include "present.h"
#include "surface.h"
#include "touch.h"

#define ROUGH_TAG "RoughFrameArt"
#define ROUGH_MAX_WINDOW 16384
#define ROUGH_MAX_RENDER_PIXELS 180000u

typedef struct {
    struct android_app *app;
    rough_touch_state touch;
    uint32_t *pixels;
    float *depth;
    unsigned render_width, render_height;
    int focused;
    int dirty;
    unsigned long input_sequence;
    unsigned long render_sequence;
} rough_android_state;

static void log_error(const char *message) {
    __android_log_print(ANDROID_LOG_ERROR, ROUGH_TAG, "%s", message);
}

static void release_framebuffer(rough_android_state *state) {
    free(state->pixels);
    free(state->depth);
    state->pixels = NULL;
    state->depth = NULL;
    state->render_width = 0;
    state->render_height = 0;
}

static int choose_resolution(unsigned width, unsigned height,
                             unsigned *render_width, unsigned *render_height) {
    if (!width || !height || width > ROUGH_MAX_WINDOW ||
        height > ROUGH_MAX_WINDOW) return -1;
    unsigned divisor = 1;
    for (;;) {
        unsigned candidate_width = (width + divisor - 1u) / divisor;
        unsigned candidate_height = (height + divisor - 1u) / divisor;
        size_t count = (size_t)candidate_width * candidate_height;
        if (candidate_width <= 512u && candidate_height <= 768u &&
            count <= ROUGH_MAX_RENDER_PIXELS) {
            *render_width = candidate_width;
            *render_height = candidate_height;
            return 0;
        }
        ++divisor;
        if (divisor > ROUGH_MAX_WINDOW) return -1;
    }
}

static int prepare_framebuffer(rough_android_state *state,
                               unsigned width, unsigned height) {
    if (state->pixels && state->depth &&
        width == state->render_width && height == state->render_height) return 0;

    size_t count = (size_t)width * height;
    if (!count || count > ROUGH_MAX_RENDER_PIXELS ||
        count > SIZE_MAX / sizeof(uint32_t) ||
        count > SIZE_MAX / sizeof(float)) return -1;
    uint32_t *pixels = malloc(count * sizeof *pixels);
    float *depth = malloc(count * sizeof *depth);
    if (!pixels || !depth) {
        free(pixels);
        free(depth);
        return -1;
    }

    release_framebuffer(state);
    state->pixels = pixels;
    state->depth = depth;
    state->render_width = width;
    state->render_height = height;
    return 0;
}

static void redraw(rough_android_state *state) {
    struct android_app *app = state->app;
    if (!app || !app->window) return;

    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(app->window, &buffer, NULL) < 0) {
        log_error("Could not lock Android window; waiting for a lifecycle event");
        return;
    }
    if (buffer.width <= 0 || buffer.height <= 0 ||
        buffer.width > ROUGH_MAX_WINDOW || buffer.height > ROUGH_MAX_WINDOW ||
        buffer.stride < buffer.width || !buffer.bits ||
        buffer.format != WINDOW_FORMAT_RGBA_8888) {
        log_error("Unsupported native RGBA window layout");
        ANativeWindow_unlockAndPost(app->window);
        return;
    }

    unsigned width = (unsigned)buffer.width;
    unsigned height = (unsigned)buffer.height;
    if (state->touch.width != width || state->touch.height != height) {
        if (rough_touch_resize(&state->touch, width, height)) {
            log_error("Invalid window dimensions for touch");
            ANativeWindow_unlockAndPost(app->window);
            return;
        }
    }
    unsigned rw, rh;
    if (choose_resolution(width, height, &rw, &rh) ||
        prepare_framebuffer(state, rw, rh)) {
        log_error("Unable to allocate bounded framebuffer");
        ANativeWindow_unlockAndPost(app->window);
        return;
    }

    size_t count = (size_t)rw * rh;
    for (size_t i = 0; i < count; ++i) {
        state->pixels[i] = 0xffffffffu;
        state->depth[i] = INFINITY;
    }
    rough_framebuffer target = {state->pixels, rw, rh, rw,
                                state->depth, rw};
    if (rough_draw_surface(&target, state->touch.shape,
                           state->touch.yaw_degrees,
                           state->touch.camera_distance)) {
        log_error("Native surface generation/rasterization failed");
        ANativeWindow_unlockAndPost(app->window);
        return;
    }

    uint64_t hash = UINT64_C(14695981039346656037);
    unsigned painted = 0u;
    for (size_t i = 0; i < count; ++i) {
        uint32_t pixel = state->pixels[i];
        hash ^= (uint64_t)pixel;
        hash *= UINT64_C(1099511628211);
        if (pixel != 0xffffffffu) ++painted;
    }

    /* Explicit straight-alpha ARGB -> Android RGBA8888 conversion and
     * premultiplied-alpha bilinear upsampling. The same native triangle
     * framebuffer supplies every pixel; padding in the Android stride stays
     * untouched, and no second graphics renderer is introduced. */
    if (rough_present_rgba8888(&target, (uint8_t *)buffer.bits,
                               width, height, (unsigned)buffer.stride)) {
        log_error("Could not present RGBA8888 framebuffer");
        ANativeWindow_unlockAndPost(app->window);
        return;
    }
    if (ANativeWindow_unlockAndPost(app->window) < 0) {
        log_error("Could not post Android window frame");
        return;
    }
    ++state->render_sequence;
    __android_log_print(ANDROID_LOG_INFO, ROUGH_TAG,
                        "FRAME input_seq=%lu render_seq=%lu shape=%s yaw=%.4f "
                        "distance=%.4f surface=%ux%u rendered=%ux%u "
                        "painted=%u rgba_hash=%016llx",
                        state->input_sequence, state->render_sequence,
                        state->touch.shape == ROUGH_SURFACE_TORUS ? "torus" : "enneper",
                        state->touch.yaw_degrees,
                        state->touch.camera_distance,
                        width, height, rw, rh, painted,
                        (unsigned long long)hash);
}

/* All app glue callbacks execute on the app's own looper thread. Touch and
 * surface callbacks therefore serialize naturally, without sharing a window
 * pointer or scratch buffer across threads. */
static void app_command(struct android_app *app, int32_t command) {
    rough_android_state *state = (rough_android_state *)app->userData;
    switch (command) {
    case APP_CMD_INIT_WINDOW:
        if (app->window) {
            if (ANativeWindow_setBuffersGeometry(app->window, 0, 0,
                                                  WINDOW_FORMAT_RGBA_8888) < 0) {
                log_error("Cannot select RGBA_8888 native window format");
            } else {
                state->dirty = 1;
            }
        }
        break;
    case APP_CMD_WINDOW_RESIZED:
    case APP_CMD_CONTENT_RECT_CHANGED:
    case APP_CMD_CONFIG_CHANGED:
        state->dirty = 1;
        break;
    case APP_CMD_GAINED_FOCUS:
        state->focused = 1;
        state->dirty = 1;
        break;
    case APP_CMD_LOST_FOCUS:
        state->focused = 0;
        break;
    case APP_CMD_TERM_WINDOW:
        state->dirty = 0;
        state->touch.active = 0;
        state->touch.active_pointer_id = -1;
        break;
    default:
        break;
    }
}

static int32_t input_event(struct android_app *app, AInputEvent *event) {
    rough_android_state *state = (rough_android_state *)app->userData;
    if (AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION) return 0;
    if (!app->window) return 0;

    int action = AMotionEvent_getAction(event);
    int kind = action & AMOTION_EVENT_ACTION_MASK;
    size_t count = AMotionEvent_getPointerCount(event);
    if (count == 0u) return 0;
    size_t index = (size_t)((action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
                            AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
    rough_touch_event type;

    if (kind == AMOTION_EVENT_ACTION_MOVE) {
        index = count;
        for (size_t i = 0; i < count; ++i) {
            if (AMotionEvent_getPointerId(event, i) ==
                state->touch.active_pointer_id) {
                index = i;
                break;
            }
        }
        if (index == count) return 0;
        type = ROUGH_TOUCH_MOVE;
    } else if (kind == AMOTION_EVENT_ACTION_DOWN ||
               kind == AMOTION_EVENT_ACTION_POINTER_DOWN) {
        type = ROUGH_TOUCH_DOWN;
    } else if (kind == AMOTION_EVENT_ACTION_UP ||
               kind == AMOTION_EVENT_ACTION_POINTER_UP) {
        type = ROUGH_TOUCH_UP;
    } else if (kind == AMOTION_EVENT_ACTION_CANCEL) {
        if (!state->touch.active) return 0;
        type = ROUGH_TOUCH_CANCEL;
        index = 0;
    } else return 0;

    if (index >= count) return 0;
    int pointer = type == ROUGH_TOUCH_CANCEL ?
                  state->touch.active_pointer_id :
                  AMotionEvent_getPointerId(event, index);
    double x = (double)AMotionEvent_getX(event, index);
    double y = (double)AMotionEvent_getY(event, index);
    int result = rough_touch_apply(&state->touch, type, pointer, x, y);
    if (result < 0) {
        log_error("Invalid native touch event");
        return 0;
    }
    if (result > 0) {
        ++state->input_sequence;
        state->dirty = 1;
    }
    return 1;
}

void android_main(struct android_app *app) {
    rough_android_state state = {0};
    state.app = app;
    if (rough_touch_init(&state.touch, 1u, 1u)) return;
    app->userData = &state;
    app->onAppCmd = app_command;
    app->onInputEvent = input_event;
    __android_log_print(ANDROID_LOG_INFO, ROUGH_TAG,
                        "NativeActivity ready; drag=rotate/zoom, tap=switch shape");

    while (!app->destroyRequested) {
        struct android_poll_source *source = NULL;
        int events = 0;
        int timeout = (state.dirty && state.focused && app->window) ? 0 : -1;
        int ident = ALooper_pollOnce(timeout, NULL, &events, (void **)&source);
        if (ident >= 0 && source) source->process(app, source);
        if (app->destroyRequested) break;

        /* Drain queued lifecycle/input before expensive CPU rasterization. */
        while (ALooper_pollOnce(0, NULL, &events, (void **)&source) >= 0) {
            if (source) source->process(app, source);
            if (app->destroyRequested) break;
        }
        if (app->destroyRequested) break;
        if (state.dirty && state.focused && app->window) {
            state.dirty = 0;
            redraw(&state);
        }
    }
    release_framebuffer(&state);
}
