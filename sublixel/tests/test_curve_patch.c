#include "sublixel/curve_patch.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static cp_vec2 vec(double x, double y) { return (cp_vec2){x, y}; }
static void near(double a, double b, double tolerance) { assert(fabs(a - b) <= tolerance); }
static void equal_grids(double a[3][3], double b[3][3]) {
    for (int y = 0; y < 3; ++y)
        for (int x = 0; x < 3; ++x) near(a[y][x], b[y][x], 0.0);
}
static void test_straight_line_and_fractional_coverage(void) {
    cp_curve_patch line;
    assert(cp_patch_from_segment(&line, vec(-4.0, 0.0), vec(4.0, 0.0), 0.5) == CP_OK);
    near(line.a2, 0.0, 0.0);
    near(line.valid_u, 4.0, 0.0);
    double grid[3][3];
    assert(cp_patch_cover_3x3(&line, 0, 0, grid) == CP_OK);
    for (int x = 0; x < 3; ++x) {
        near(grid[0][x], 0.0, 0.0);
        near(grid[1][x], 1.0, 0.0);
        near(grid[2][x], 0.0, 0.0);
    }
    double p = -1.0;
    assert(cp_patch_coverage_pixel(&line, 0, 0, &p) == CP_OK);
    near(p, grid[1][1], 0.0);
    assert(cp_patch_from_segment(&line, vec(-4, 0.25), vec(4, 0.25), 0.25) == CP_OK);
    assert(cp_patch_cover_3x3(&line, 0, 0, grid) == CP_OK);
    for (int x = 0; x < 3; ++x) {
        near(grid[0][x], 0.0, 0.0);
        near(grid[1][x], 0.5, 0.0);
        near(grid[2][x], 0.0, 0.0);
    }
    /* A finite segment has flat caps at its exact tangent-domain ends. */
    assert(cp_patch_from_segment(&line, vec(-0.25, 0), vec(0.25, 0), 0.5) == CP_OK);
    assert(cp_patch_cover_3x3(&line, 0, 0, grid) == CP_OK);
    near(grid[1][1], 0.5, 0.0);
    near(grid[1][0], 0.0, 0.0);
    near(grid[1][2], 0.0, 0.0);
}

static void test_jet_curvature_and_bezier(void) {
    cp_curve_patch patch;
    /* x=t, y=t*t/4 has curvature +1/2 at the origin. */
    assert(cp_patch_from_jet2(&patch, vec(0, 0), vec(1, 0), vec(0, 0.5),
                              0.2, 2.0) == CP_OK);
    near(patch.tangent.x, 1.0, 0.0);
    near(patch.normal.y, 1.0, 0.0);
    near(patch.a2, 0.25, 0.0);
    near(patch.a3, 0.0, 0.0);
    cp_vec2 on_curve;
    assert(cp_patch_point(&patch, 1.0, &on_curve) == CP_OK);
    near(on_curve.x, 1.0, 0.0);
    near(on_curve.y, 0.25, 0.0);
    double residual = 10.0;
    assert(cp_patch_signed_distance(&patch, on_curve, &residual) == CP_OK);
    near(residual, 0.0, 0.0);
    assert(cp_patch_signed_distance(&patch, vec(1, 0.35), &residual) == CP_OK);
    near(residual, 0.10, 1e-15);
    assert(cp_patch_point(&patch, 3.0, &on_curve) == CP_OUTSIDE);
    assert(cp_patch_signed_distance(&patch, vec(3, 0.0), &residual) == CP_OUTSIDE);

    /* A radius-two circle, at its top moving toward +x, bends downward. */
    assert(cp_patch_from_jet2(&patch, vec(0, 2), vec(2, 0), vec(0, -2),
                              0.2, 1.5) == CP_OK);
    near(patch.a2, -0.25, 0.0);
    assert(cp_patch_point(&patch, 1, &on_curve) == CP_OK);
    near(on_curve.y, 1.75, 0.0);

    /* Cubic symmetric arch: at t=1/2, first derivative (2,0), D2=(0,-6). */
    const cp_vec2 controls[4] = {
        {-1, 0}, {-1.0 / 3.0, 1}, {1.0 / 3.0, 1}, {1, 0}
    };
    assert(cp_patch_from_cubic(&patch, controls, 0.5, 0.3, 2.0) == CP_OK);
    near(patch.origin.x, 0.0, 1e-15);
    near(patch.origin.y, 0.75, 0.0);
    near(patch.a2, -0.75, 1e-15);
}

static void test_curved_pixel_coverage_against_independent_counts(void) {
    cp_curve_patch patch;
    assert(cp_patch_from_jet2(&patch, vec(0, 0), vec(1, 0), vec(0, 0.5),
                              0.18, 2.0) == CP_OK);
    double grid[3][3];
    assert(cp_patch_cover_3x3(&patch, 0, 0, grid) == CP_OK);
    /* Independent fixed-point-style tally of |y - x*x/4| <= 0.18
     * on each 8x8 midpoint lattice, not based on library distance code. */
    const unsigned expected[3][3] = {{0, 0, 0}, {20, 22, 20}, {3, 0, 3}};
    for (int y = 0; y < 3; ++y)
        for (int x = 0; x < 3; ++x)
            near(grid[y][x], (double)expected[y][x] / 64.0, 0.0);
}

static void test_reparameterization_and_orientation(void) {
    cp_curve_patch original, fast, reversed, original_bezier, reversed_bezier;
    const cp_vec2 pos = {0.15, 0.12};
    const cp_vec2 d1 = {2, 1};
    const cp_vec2 d2 = {3, -1};
    assert(cp_patch_from_jet2(&original, pos, d1, d2, 0.37, 2.2) == CP_OK);
    /* t = 3*s + (5/2)*s*s: D1 becomes 3D1, D2 becomes 9D2+5D1. */
    assert(cp_patch_from_jet2(&fast, pos, vec(6, 3), vec(37, -4),
                              0.37, 2.2) == CP_OK);
    /* t = -2*s + (5/2)*s*s: D1=-2D1, D2=4D2+5D1. */
    assert(cp_patch_from_jet2(&reversed, pos, vec(-4, -2), vec(22, 1),
                              0.37, 2.2) == CP_OK);
    near(original.a2, fast.a2, 1e-15);
    near(original.a2, -reversed.a2, 1e-15);
    double reference[3][3], check[3][3];
    assert(cp_patch_cover_3x3(&original, 0, 0, reference) == CP_OK);
    assert(cp_patch_cover_3x3(&fast, 0, 0, check) == CP_OK);
    equal_grids(reference, check);
    assert(cp_patch_cover_3x3(&reversed, 0, 0, check) == CP_OK);
    equal_grids(reference, check);

    const cp_vec2 controls[4] = {
        {-1, 0}, {-1.0 / 3.0, 1}, {1.0 / 3.0, 1}, {1, 0}
    };
    const cp_vec2 reversed_controls[4] = {
        {1, 0}, {1.0 / 3.0, 1}, {-1.0 / 3.0, 1}, {-1, 0}
    };
    assert(cp_patch_from_cubic(&original_bezier, controls, 0.4, 0.3, 1.6) == CP_OK);
    assert(cp_patch_from_cubic(&reversed_bezier, reversed_controls, 0.6,
                               0.3, 1.6) == CP_OK);
    assert(cp_patch_cover_3x3(&original_bezier, 0, 0, reference) == CP_OK);
    assert(cp_patch_cover_3x3(&reversed_bezier, 0, 0, check) == CP_OK);
    equal_grids(reference, check);
}

static void test_coverage_union_and_order(void) {
    cp_curve_patch whole, left, right;
    assert(cp_patch_from_segment(&whole, vec(-2, 0), vec(2, 0), 0.27) == CP_OK);
    assert(cp_patch_from_segment(&left, vec(-2, 0), vec(0, 0), 0.27) == CP_OK);
    assert(cp_patch_from_segment(&right, vec(0, 0), vec(2, 0), 0.27) == CP_OK);
    cp_accumulator_3x3 acc, split, reversed;
    cp_patch_accumulator_3x3_init(&acc, 0, 0);
    assert(cp_patch_accumulate_3x3(&acc, &whole) == CP_OK);
    const cp_accumulator_3x3 exact_copy = acc;
    /* Adding the identical patch twice cannot double its coverage. */
    assert(cp_patch_accumulate_3x3(&acc, &whole) == CP_OK);
    assert(memcmp(&acc, &exact_copy, sizeof(acc)) == 0);
    cp_patch_accumulator_3x3_init(&split, 0, 0);
    assert(cp_patch_accumulate_3x3(&split, &left) == CP_OK);
    assert(cp_patch_accumulate_3x3(&split, &right) == CP_OK);
    cp_patch_accumulator_3x3_init(&reversed, 0, 0);
    assert(cp_patch_accumulate_3x3(&reversed, &right) == CP_OK);
    assert(cp_patch_accumulate_3x3(&reversed, &left) == CP_OK);
    assert(memcmp(&split, &reversed, sizeof(split)) == 0);
    assert(memcmp(&split, &exact_copy, sizeof(split)) == 0);

    double grid[3][3], direct[3][3];
    cp_patch_accumulator_3x3_coverage(&split, grid);
    assert(cp_patch_cover_3x3(&whole, 0, 0, direct) == CP_OK);
    equal_grids(grid, direct);
    for (int y = 0; y < 3; ++y)
        for (int x = 0; x < 3; ++x) {
            double individual;
            assert(cp_patch_coverage_pixel(&whole, x-1, y-1, &individual) == CP_OK);
            near(individual, grid[y][x], 0.0);
        }
}

static void test_refusal_and_no_partial_mutations(void) {
    cp_curve_patch patch = {0};
    patch.origin.x = 91;
    assert(cp_patch_from_jet2(&patch, vec(0, 0), vec(0, 0), vec(0, 1),
                              0.5, 2) == CP_SINGULAR);
    near(patch.origin.x, 91, 0);
    assert(cp_patch_from_jet2(&patch, vec(NAN, 0), vec(1, 0), vec(0, 1),
                              0.5, 2) == CP_INVALID);
    assert(cp_patch_from_jet2(&patch, vec(0, 0), vec(1, 0), vec(0, 1),
                              0, 2) == CP_INVALID);
    assert(cp_patch_from_segment(&patch, vec(3, 3), vec(3, 3),
                                 0.5) == CP_SINGULAR);
    const cp_vec2 cusp[4] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}};
    assert(cp_patch_from_cubic(&patch, cusp, 0.5, 0.5, 2) == CP_SINGULAR);
    assert(cp_patch_from_cubic(&patch, cusp, 2, 0.5, 2) == CP_INVALID);
    assert(cp_patch_from_jet2(&patch, vec(0, 0), vec(1, 0), vec(0, 0),
                              0.5, 2) == CP_OK);
    double distance = 91;
    assert(cp_patch_signed_distance(&patch, vec(3, 0), &distance) == CP_OUTSIDE);
    near(distance, 91, 0);
    cp_vec2 point = vec(91, 92);
    assert(cp_patch_point(&patch, 3, &point) == CP_OUTSIDE);
    near(point.x, 91, 0);
    double grid[3][3];
    for (int y = 0; y < 3; ++y)
        for (int x = 0; x < 3; ++x) grid[y][x] = 91;
    patch.normal = vec(1, 0); /* forged nonorthogonal frame */
    assert(cp_patch_cover_3x3(&patch, 0, 0, grid) == CP_INVALID);
    near(grid[1][1], 91, 0);

    assert(cp_patch_from_jet2(&patch, vec(0, 0), vec(1, 0), vec(0, 0),
                              0.5, 3) == CP_OK);
    cp_accumulator_3x3 acc;
    cp_patch_accumulator_3x3_init(&acc, 0, 0);
    assert(cp_patch_accumulate_3x3(&acc, &patch) == CP_OK);
    const cp_accumulator_3x3 previous = acc;
    patch.a2 = 1e308; /* finite field, overflow during later pixel evaluation */
    assert(cp_patch_accumulate_3x3(&acc, &patch) == CP_OVERFLOW);
    assert(memcmp(&acc, &previous, sizeof(acc)) == 0);
    assert(cp_patch_cover_3x3(&patch, 0, 0, grid) == CP_OVERFLOW);
    near(grid[1][1], 91, 0);
    assert(cp_patch_coverage_pixel(&patch, 1, 1, &distance) == CP_OVERFLOW);
    near(distance, 91, 0);
    assert(cp_patch_cover_3x3(NULL, 0, 0, grid) == CP_INVALID);
    assert(cp_patch_cover_3x3(&patch, 0, 0, NULL) == CP_INVALID);
}

int main(void) {
    test_straight_line_and_fractional_coverage();
    test_jet_curvature_and_bezier();
    test_curved_pixel_coverage_against_independent_counts();
    test_reparameterization_and_orientation();
    test_coverage_union_and_order();
    test_refusal_and_no_partial_mutations();
    puts("sublixel 3x3 curve patch: PASS");
    return 0;
}
