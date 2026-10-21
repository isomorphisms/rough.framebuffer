#ifndef SUBPIXEL_PATH_H
#define SUBPIXEL_PATH_H
#include "subpixel/coverage_plane.h"

/* All controls are material coordinates; the affine transform maps them to
 * screen pixels. Width is in SCREEN pixels, including under nonuniform scale.
 * Each segment has its own start/end. Their union gives round caps/joins;
 * closed subpaths explicitly repeat their starting point. No implicit joins
 * between disjoint segments, fill rule, taper, or depth is inferred. */
typedef struct { unsigned degree; cp_vec2 controls[4]; } cp_path_segment;
typedef struct { double xx, xy, yx, yy, tx, ty; } cp_path_transform;
typedef enum { CP_PATH_AUTO, CP_PATH_QUADRATIC, CP_PATH_CUBIC } cp_path_strategy;
typedef enum {
    CP_PATH_OK = 0, CP_PATH_INVALID = -1, CP_PATH_BUDGET = -2,
    CP_PATH_NONCONVERGENCE = -3, CP_PATH_CANCELLED = -4
} cp_path_status;
typedef struct {
    cp_path_transform transform;
    double width;
    double tolerance;         /* local diagnostic, [1e-5,1/8] pixels */
    unsigned max_pixel_error; /* maximum XOR bits against fine reference, 0..64 */
    unsigned max_depth;       /* subdivision depth, 1..24 */
    size_t max_patches, max_sample_tests;
    cp_path_strategy strategy;
    int (*cancelled)(void *);
    void *cancel_context;
} cp_path_options;
typedef struct {
    uint64_t *candidate, *reference; /* two disjoint packed width*height buffers */
    size_t capacity_words;          /* capacity of EACH buffer */
} cp_path_workspace;
typedef struct {
    cp_path_strategy selected;
    const char *reason;
    size_t patches, reference_segments, sample_tests;
    /* Conservative working bound: two scratch buffers plus 4096 bytes per
     * allowed recursive level and 4096 fixed bytes. Pinned ICK stack-usage
     * qualification must remain below this allowance. Excludes caller input
     * and destination, or caller callback stack/allocation. This is an upper
     * allowance, not a measured peak. */
    size_t peak_bytes;
    unsigned attempts, worst_pixel_error;
    double reference_flatness;
} cp_path_report;

/* No allocation. On EVERY failure destination and report remain unchanged;
 * scratch is disposable. On success union this shape into destination once.
 * Destination may be padded; scratch buffers are packed. Aliasing any of the
 * three buffers is rejected. Geometry/width <=1e6/1024 pixels, <=2048 segments.
 * The measured whole-image diagnostic is against an adaptive polyline tube,
 * not a proof of continuous area accuracy or exact nearest-curve distance.
 * Alpha is applied afterward through rough_blend_subpixel_coverage, once per
 * shape. The historical stroke baseline retains its own stream semantics. */
cp_path_status cp_path_rasterize(cp_coverage_plane *destination,
    const cp_path_segment *segments, size_t segment_count,
    const cp_path_options *options, cp_path_workspace *workspace,
    cp_path_report *report);
#endif
