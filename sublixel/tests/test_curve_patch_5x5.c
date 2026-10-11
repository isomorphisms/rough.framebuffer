#include "sublixel/curve_patch.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static cp_vec2 vec(double x, double y) { return (cp_vec2){x, y}; }
static void near(double a, double b, double tolerance) {
    assert(fabs(a - b) <= tolerance);
}
static void equal_grids5(double a[5][5], double b[5][5]) {
    for (int y = 0; y < 5; ++y)
        for (int x = 0; x < 5; ++x)
            near(a[y][x], b[y][x], 0.0);
}

static void test_cubic_jet_and_legacy_quadratic(void) {
    cp_curve_patch cubic, quadratic;
    assert(cp_patch_from_jet3(&cubic, vec(0, 0), vec(1, 0),
                              vec(0, 0.5), vec(0, 0.375),
                              3.0 / 16.0, 3) == CP_OK);
    assert(cp_patch_from_jet2(&quadratic, vec(0, 0), vec(1, 0),
                              vec(0, 0.5), 3.0 / 16.0, 3) == CP_OK);
    near(cubic.a2, 0.25, 0);
    near(cubic.a3, 1.0 / 16.0, 0);
    near(quadratic.a3, 0, 0);

    cp_vec2 point = vec(91, 92);
    assert(cp_patch_point_cubic(&cubic, 2, &point) == CP_OK);
    near(point.x, 2, 0);
    near(point.y, 1.5, 0);
    assert(cp_patch_point(&cubic, 2, &point) == CP_OK);
    near(point.y, 1, 0); /* Existing quadratic API does not use a3. */

    double residual = 91;
    assert(cp_patch_signed_distance_cubic(&cubic, vec(2, 1.5),
                                          &residual) == CP_OK);
    near(residual, 0, 0);
    assert(cp_patch_signed_distance_cubic(&cubic, vec(2, 1.6),
                                          &residual) == CP_OK);
    near(residual, 0.1, 1e-14);
    assert(cp_patch_point_cubic(&cubic, 4, &point) == CP_OUTSIDE);
    assert(cp_patch_signed_distance_cubic(&cubic, vec(4, 0),
                                          &residual) == CP_OUTSIDE);
    near(residual, 0.1, 1e-14);

    double before[3][3], after[3][3];
    assert(cp_patch_cover_3x3(&quadratic, 0, 0, before) == CP_OK);
    assert(cp_patch_cover_3x3(&cubic, 0, 0, after) == CP_OK);
    for (int y = 0; y < 3; ++y)
        for (int x = 0; x < 3; ++x)
            near(before[y][x], after[y][x], 0);
}

static void test_integer_lattice_cubic_oracle(void) {
    cp_curve_patch patch;
    assert(cp_patch_from_jet3(&patch, vec(0, 0), vec(1, 0),
                              vec(0, 0.5), vec(0, 0.375),
                              3.0 / 16.0, 3) == CP_OK);
    double grid[5][5];
    assert(cp_patch_cover_5x5(&patch, 0, 0, grid) == CP_OK);

    /* Independent integer-rational coverage oracle.
     * On the 8x8 midpoint lattice, u=nu/16, v=nv/16.
     * For v=u^2/4+u^3/16, 65536*(v-graph) =
     * 4096*nv -64*nu^2 -nu^3.
     * half_width=3/16 => bound 12288; valid_u=3 => |nu|<=48.
     * This path does not call any Sublixel geometry or mask helper. */
    const unsigned expected[5][5] = {
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {13, 24, 24, 18, 0},
        {11, 0, 0, 6, 12},
        {0, 0, 0, 0, 11}
    };
    for (int y = 0; y < 5; ++y)
        for (int x = 0; x < 5; ++x) {
            unsigned occupied = 0;
            for (int sy = 0; sy < 8; ++sy)
                for (int sx = 0; sx < 8; ++sx) {
                    long nu = 16L * (x - 2) + 2L * sx - 7;
                    long nv = 16L * (y - 2) + 2L * sy - 7;
                    long residual = 4096L * nv - 64L * nu * nu -
                                    nu * nu * nu;
                    if (nu >= -48 && nu <= 48 &&
                        residual >= -12288 && residual <= 12288)
                        ++occupied;
                }
            assert(occupied == expected[y][x]);
            near(grid[y][x], occupied / 64.0, 0);
            double individual = -1;
            assert(cp_patch_coverage_pixel_cubic(&patch, x - 2, y - 2,
                                                 &individual) == CP_OK);
            near(grid[y][x], individual, 0);
        }
    assert(grid[2][0] > grid[2][4]); /* Actually asymmetric, not 5x5 quadratic. */

    /* Pixel-center translation does not change footprint fractions. */
    patch.origin = vec(7, -3);
    double translated[5][5];
    assert(cp_patch_cover_5x5(&patch, 7, -3, translated) == CP_OK);
    equal_grids5(grid, translated);
}

static void test_cubic_bezier_derivatives(void) {
    /* P(t)=(t,t^3); its cubic local graph is exact at t=0. */
    const cp_vec2 controls[4] = {
        {0, 0}, {1.0 / 3.0, 0}, {2.0 / 3.0, 0}, {1, 1}
    };
    const cp_vec2 reversed[4] = {
        {1, 1}, {2.0 / 3.0, 0}, {1.0 / 3.0, 0}, {0, 0}
    };
    cp_curve_patch forward, backward;
    assert(cp_patch_from_cubic_jet3(&forward, controls, 0.0,
                                    0.2, 1.0) == CP_OK);
    assert(cp_patch_from_cubic_jet3(&backward, reversed, 1.0,
                                    0.2, 1.0) == CP_OK);
    near(forward.origin.x, 0, 0);
    near(forward.origin.y, 0, 0);
    near(forward.a2, 0, 0);
    near(forward.a3, 1, 1e-15);
    near(backward.a2, 0, 0);
    near(backward.a3, 1, 1e-15);

    cp_vec2 p1, p2;
    assert(cp_patch_point_cubic(&forward, 0.25, &p1) == CP_OK);
    assert(cp_patch_point_cubic(&backward, -0.25, &p2) == CP_OK);
    near(p1.x, 0.25, 1e-15);
    near(p1.y, 1.0 / 64.0, 1e-15);
    near(p1.x, p2.x, 1e-15);
    near(p1.y, p2.y, 1e-15);

    double grid[5][5], backward_grid[5][5];
    assert(cp_patch_cover_5x5(&forward, 0, 0, grid) == CP_OK);
    assert(cp_patch_cover_5x5(&backward, 0, 0, backward_grid) == CP_OK);
    equal_grids5(grid, backward_grid);
}

static void test_nonlinear_reparameterization_and_reversal(void) {
    cp_curve_patch original, faster, reversed;
    const cp_vec2 pos = {0.15, 0.12};
    /* t(s)=3s+(5/2)s^2+(7/6)s^3:
     * D1'=3D1, D2'=9D2+5D1, D3'=27D3+45D2+7D1.
     * Reversed t(s)=-2s+(5/2)s^2+(7/6)s^3:
     * D1'=-2D1, D2'=4D2+5D1, D3'=-8D3-30D2+7D1. */
    assert(cp_patch_from_jet3(&original, pos, vec(2, 1),
                              vec(3, -1), vec(2, -4),
                              0.37, 2.2) == CP_OK);
    assert(cp_patch_from_jet3(&faster, pos, vec(6, 3),
                              vec(37, -4), vec(203, -146),
                              0.37, 2.2) == CP_OK);
    assert(cp_patch_from_jet3(&reversed, pos, vec(-4, -2),
                              vec(22, 1), vec(-92, 69),
                              0.37, 2.2) == CP_OK);
    near(original.a2, faster.a2, 1e-14);
    near(original.a2, -reversed.a2, 1e-14);
    near(original.a3, faster.a3, 1e-14);
    near(original.a3, reversed.a3, 1e-14);
    double reference[5][5], check[5][5];
    assert(cp_patch_cover_5x5(&original, 0, 0, reference) == CP_OK);
    assert(cp_patch_cover_5x5(&faster, 0, 0, check) == CP_OK);
    equal_grids5(reference, check);
    assert(cp_patch_cover_5x5(&reversed, 0, 0, check) == CP_OK);
    equal_grids5(reference, check);
}

static void test_vertical_horizontal_and_union(void) {
    cp_curve_patch whole, left, right, vertical;
    assert(cp_patch_from_segment(&whole, vec(-3, 0.25),
                                 vec(3, 0.25), 0.25) == CP_OK);
    double grid[5][5];
    assert(cp_patch_cover_5x5(&whole, 0, 0, grid) == CP_OK);
    for (int x = 0; x < 5; ++x) {
        near(grid[1][x], 0, 0);
        near(grid[2][x], 0.5, 0);
        near(grid[3][x], 0, 0);
    }

    assert(cp_patch_from_segment(&vertical, vec(0, -4),
                                 vec(0, 4), 0.5) == CP_OK);
    assert(cp_patch_cover_5x5(&vertical, 0, 0, grid) == CP_OK);
    for (int y = 0; y < 5; ++y)
        for (int x = 0; x < 5; ++x)
            near(grid[y][x], x == 2 ? 1.0 : 0.0, 0);

    assert(cp_patch_from_segment(&left, vec(-3, 0.25),
                                 vec(0, 0.25), 0.25) == CP_OK);
    assert(cp_patch_from_segment(&right, vec(0, 0.25),
                                 vec(3, 0.25), 0.25) == CP_OK);
    cp_accumulator_5x5 single, split, flipped;
    cp_patch_accumulator_5x5_init(&single, 0, 0);
    assert(cp_patch_accumulate_5x5(&single, &whole) == CP_OK);
    cp_accumulator_5x5 once = single;
    assert(cp_patch_accumulate_5x5(&single, &whole) == CP_OK);
    assert(memcmp(&single, &once, sizeof(single)) == 0);

    cp_patch_accumulator_5x5_init(&split, 0, 0);
    assert(cp_patch_accumulate_5x5(&split, &left) == CP_OK);
    assert(cp_patch_accumulate_5x5(&split, &right) == CP_OK);
    cp_patch_accumulator_5x5_init(&flipped, 0, 0);
    assert(cp_patch_accumulate_5x5(&flipped, &right) == CP_OK);
    assert(cp_patch_accumulate_5x5(&flipped, &left) == CP_OK);
    assert(memcmp(&single, &split, sizeof(single)) == 0);
    assert(memcmp(&split, &flipped, sizeof(split)) == 0);
    double accumulation[5][5], direct[5][5];
    cp_patch_accumulator_5x5_coverage(&split, accumulation);
    assert(cp_patch_cover_5x5(&whole, 0, 0, direct) == CP_OK);
    equal_grids5(accumulation, direct);
}

static void test_errors_are_transactional(void) {
    cp_curve_patch patch = {0};
    patch.origin.x = 91;
    assert(cp_patch_from_jet3(&patch, vec(0, 0), vec(0, 0),
                              vec(0, 1), vec(0, 1),
                              0.5, 3) == CP_SINGULAR);
    near(patch.origin.x, 91, 0);
    assert(cp_patch_from_jet3(&patch, vec(0, 0), vec(1, 0),
                              vec(0, 1), vec(NAN, 0),
                              0.5, 3) == CP_INVALID);
    near(patch.origin.x, 91, 0);
    assert(cp_patch_from_jet3(&patch, vec(0, 0), vec(1, 0),
                              vec(0, 1), vec(0, 1e308),
                              0.5, 3) == CP_OK);
    assert(cp_patch_from_jet3(&patch, vec(0, 0), vec(1, 0),
                              vec(0, 1), vec(0, 1e308),
                              0.5, 0) == CP_INVALID);
    const cp_vec2 cusp[4] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}};
    assert(cp_patch_from_cubic_jet3(&patch, cusp, 0.5,
                                    0.5, 2) == CP_SINGULAR);
    assert(cp_patch_from_cubic_jet3(&patch, cusp, 2.0,
                                    0.5, 2) == CP_INVALID);

    assert(cp_patch_from_jet3(&patch, vec(0, 0), vec(1, 0),
                              vec(0, 0), vec(0, 0),
                              0.25, 3) == CP_OK);
    double grid[5][5];
    for (int y = 0; y < 5; ++y)
        for (int x = 0; x < 5; ++x) grid[y][x] = 91;
    patch.normal = vec(1, 0);
    assert(cp_patch_cover_5x5(&patch, 0, 0, grid) == CP_INVALID);
    near(grid[2][2], 91, 0);
    patch.normal = vec(0, 1);

    cp_accumulator_5x5 acc;
    cp_patch_accumulator_5x5_init(&acc, 0, 0);
    assert(cp_patch_accumulate_5x5(&acc, &patch) == CP_OK);
    cp_accumulator_5x5 previous = acc;
    patch.a3 = 1e308; /* Valid finite field, overflowing evaluation. */
    assert(cp_patch_accumulate_5x5(&acc, &patch) == CP_OVERFLOW);
    assert(memcmp(&acc, &previous, sizeof(acc)) == 0);
    assert(cp_patch_cover_5x5(&patch, 0, 0, grid) == CP_OVERFLOW);
    near(grid[2][2], 91, 0);
    double result = 91;
    assert(cp_patch_coverage_pixel_cubic(&patch, 2, 1,
                                         &result) == CP_OVERFLOW);
    near(result, 91, 0);
    cp_vec2 point = vec(91, 92);
    assert(cp_patch_point_cubic(&patch, 2, &point) == CP_OVERFLOW);
    near(point.x, 91, 0);
    assert(cp_patch_cover_5x5(NULL, 0, 0, grid) == CP_INVALID);
    assert(cp_patch_cover_5x5(&patch, 0, 0, NULL) == CP_INVALID);
    patch.a3 = NAN;
    assert(cp_patch_cover_5x5(&patch, 0, 0, grid) == CP_INVALID);
    assert(memcmp(&acc, &previous, sizeof(acc)) == 0);
}

int main(void) {
    test_cubic_jet_and_legacy_quadratic();
    test_integer_lattice_cubic_oracle();
    test_cubic_bezier_derivatives();
    test_nonlinear_reparameterization_and_reversal();
    test_vertical_horizontal_and_union();
    test_errors_are_transactional();
    puts("sublixel 5x5 cubic neighborhoods: PASS");
    return 0;
}
