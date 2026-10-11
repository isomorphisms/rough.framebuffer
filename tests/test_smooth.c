#include "framebuffer.h"
#include "surface.h"

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { double x, y, z; } position3;

static position3 evaluate_independent(rough_surface_shape shape,
                                       double u, double v) {
    position3 p;
    if (shape == ROUGH_SURFACE_TORUS) {
        double r = 1.18 + 0.43 * cos(v);
        p.x = r * cos(u);
        p.y = r * sin(u);
        p.z = 0.43 * sin(v);
    } else {
        p.x = 0.85 * (u - u*u*u/3.0 + u*v*v);
        p.y = 0.85 * (v - v*v*v/3.0 + u*u*v);
        p.z = 0.85 * (u*u - v*v);
    }
    return p;
}

static void normal_oracles(rough_surface_shape shape,
                            double u, double v) {
    double normal[3] = {9.0, 9.0, 9.0};
    assert(rough_surface_unit_normal(shape, u, v, normal) == 0);
    assert(fabs(normal[0]*normal[0] +
                normal[1]*normal[1] +
                normal[2]*normal[2] - 1.0) < 1e-12);

    /* Independent central differences rather than restating the formula.
     * The normalized du x dv orientation must agree with the real surface
     * derivatives and remain independent of triangle tessellation. */
    double h = 0.00001;
    position3 au = evaluate_independent(shape, u + h, v);
    position3 bu = evaluate_independent(shape, u - h, v);
    position3 av = evaluate_independent(shape, u, v + h);
    position3 bv = evaluate_independent(shape, u, v - h);
    double du[3] = {(au.x-bu.x)/(2*h), (au.y-bu.y)/(2*h),
                    (au.z-bu.z)/(2*h)};
    double dv[3] = {(av.x-bv.x)/(2*h), (av.y-bv.y)/(2*h),
                    (av.z-bv.z)/(2*h)};
    double cross[3] = {du[1]*dv[2]-du[2]*dv[1],
                       du[2]*dv[0]-du[0]*dv[2],
                       du[0]*dv[1]-du[1]*dv[0]};
    double length = sqrt(cross[0]*cross[0] +
                         cross[1]*cross[1] +
                         cross[2]*cross[2]);
    assert(length > 0.001);
    for (unsigned i=0; i<3u; ++i)
        assert(fabs(normal[i] - cross[i]/length) < 1e-6);
}

static void test_analytic_normals(void) {
    normal_oracles(ROUGH_SURFACE_TORUS, 0.0, 0.0);
    normal_oracles(ROUGH_SURFACE_TORUS, 1.18, 0.68);
    normal_oracles(ROUGH_SURFACE_TORUS, 3.0, 2.45);
    normal_oracles(ROUGH_SURFACE_ENNEPER, 0.0, 0.0);
    normal_oracles(ROUGH_SURFACE_ENNEPER, 1.0, 0.0);
    normal_oracles(ROUGH_SURFACE_ENNEPER, -0.7, 1.0);
    normal_oracles(ROUGH_SURFACE_ENNEPER, 0.77, -1.2);
    const double pi = 3.14159265358979323846264338327950288;
    double start[3], end[3];
    for (unsigned i=0; i<3u; ++i) {
        double u = 0.58 * (double)i;
        assert(rough_surface_unit_normal(ROUGH_SURFACE_TORUS,
                                         u, 0.0, start) == 0);
        assert(rough_surface_unit_normal(ROUGH_SURFACE_TORUS,
                                         u, 2*pi, end) == 0);
        for (unsigned k=0; k<3u; ++k)
            assert(fabs(start[k]-end[k]) < 1e-12);
        assert(rough_surface_unit_normal(ROUGH_SURFACE_TORUS,
                                         0.0, u, start) == 0);
        assert(rough_surface_unit_normal(ROUGH_SURFACE_TORUS,
                                         2*pi, u, end) == 0);
        for (unsigned k=0; k<3u; ++k)
            assert(fabs(start[k]-end[k]) < 1e-12);
    }

    assert(rough_surface_unit_normal(ROUGH_SURFACE_TORUS,
                                     0.0, 0.0, NULL) == -1);
    assert(rough_surface_unit_normal((rough_surface_shape)88,
                                     0.0, 0.0, start) == -1);
    start[0] = 44.0;
    assert(rough_surface_unit_normal(ROUGH_SURFACE_TORUS,
                                     NAN, 0.0, start) == -1);
    assert(start[0] == 44.0);
}

static rough_smooth_vertex vertex(double x, double y, double depth,
                                   double inv_z,
                                   double nx, double ny, double nz,
                                   double r, double g, double b) {
    rough_smooth_vertex v = {
        {x,y,depth}, inv_z, nx,ny,nz,r,g,b
    };
    return v;
}

static void test_shader_perspective_and_depth(void) {
    uint32_t pixels[12u * 12u];
    float depth[12u * 12u];
    uint32_t reverse_pixels[12u * 12u];
    float reverse_depth[12u * 12u];
    for (unsigned i=0; i<144u; ++i) {
        pixels[i] = reverse_pixels[i] = UINT32_C(0xffffffff);
        depth[i] = reverse_depth[i] = INFINITY;
    }
    rough_framebuffer fb={pixels,12,12,12,depth,12};
    rough_framebuffer reverse={reverse_pixels,12,12,12,reverse_depth,12};
    /* Vertices all face the light, so this isolates projective material
     * interpolation: at pixel (3,3) screen barycentric (1/2,1/4,1/4).
     * Camera inverse depths (1,1/2,1) yield base R = 80 rather than the
     * wrong affine interpolation R = 95. */
    rough_smooth_vertex a=vertex(1,1,.5,1,-.42,-.55,-.72,60,80,100);
    rough_smooth_vertex b=vertex(9,1,.75,.5,-.42,-.55,-.72,200,80,100);
    rough_smooth_vertex c=vertex(1,9,.5,1,-.42,-.55,-.72,60,80,100);
    assert(rough_fill_smooth_triangle(&fb,a,b,c) == 0);
    assert(rough_fill_smooth_triangle(&reverse,a,c,b) == 0);
    assert(memcmp(pixels, reverse_pixels, sizeof pixels) == 0);
    assert(memcmp(depth, reverse_depth, sizeof depth) == 0);
    uint32_t middle=pixels[3u*12u+3u];
    assert(((middle>>16)&255u) >= 79u && ((middle>>16)&255u)<=81u);
    assert(((middle>>8)&255u) >= 79u && ((middle>>8)&255u)<=81u);
    assert((middle&255u) >= 99u && (middle&255u)<=101u);
    assert(fabsf(depth[3u*12u+3u]-.5625f) < 0.000001f);

    rough_smooth_vertex near_a=vertex(1,1,.25,1.5,-.42,-.55,-.72,20,230,20);
    rough_smooth_vertex near_b=vertex(9,1,.25,1.5,-.42,-.55,-.72,20,230,20);
    rough_smooth_vertex near_c=vertex(1,9,.25,1.5,-.42,-.55,-.72,20,230,20);
    assert(rough_fill_smooth_triangle(&fb,near_a,near_b,near_c) == 0);
    uint32_t near_color=pixels[3u*12u+3u];
    assert(((near_color>>8)&255u) > 220u);
    assert(fabsf(depth[3u*12u+3u]-.25f) < 0.000001f);
    assert(rough_fill_smooth_triangle(&fb,a,b,c) == 0);
    assert(pixels[3u*12u+3u] == near_color);
    assert(fabsf(depth[3u*12u+3u]-.25f) < 0.000001f);

    rough_smooth_vertex bad=a;
    bad.reciprocal_z=0.0;
    assert(rough_fill_smooth_triangle(&fb,bad,b,c) == -1);
    bad=a;bad.normal_x=NAN;
    assert(rough_fill_smooth_triangle(&fb,bad,b,c) == -1);
    bad=a;bad.base_blue=INFINITY;
    assert(rough_fill_smooth_triangle(&fb,bad,b,c) == -1);
    assert(rough_fill_smooth_triangle(NULL,a,b,c) == -1);
    rough_framebuffer missing=fb;
    missing.depth=NULL;
    assert(rough_fill_smooth_triangle(&missing,a,b,c) == -1);
}

static void test_normal_is_evaluated_per_fragment(void) {
    uint32_t pixels[16u*16u];
    float depth[16u*16u];
    for (unsigned i=0; i<256u; ++i) {
        pixels[i] = UINT32_C(0xffffffff);
        depth[i] = INFINITY;
    }
    rough_framebuffer fb={pixels,16,16,16,depth,16};
    rough_smooth_vertex a=vertex(1,1,.5,1,0,0,-1,200,200,200);
    rough_smooth_vertex b=vertex(14,1,.5,1,0,1,0,200,200,200);
    rough_smooth_vertex c=vertex(1,14,.5,1,1,0,0,200,200,200);
    assert(rough_fill_smooth_triangle(&fb,a,b,c) == 0);
    unsigned center=(pixels[2u*16u+2u]>>16)&255u;
    unsigned toward_y=(pixels[2u*16u+10u]>>16)&255u;
    unsigned toward_x=(pixels[10u*16u+2u]>>16)&255u;
    assert(center>toward_y+8u && toward_y>toward_x+8u);
}

static void test_real_surface_color_only(rough_surface_shape shape,
                                          double yaw,double distance) {
    const unsigned width=144,height=144;
    const size_t count=(size_t)width*height;
    uint32_t *flat_color=malloc(count*sizeof(uint32_t));
    uint32_t *smooth_color=malloc(count*sizeof(uint32_t));
    float *flat_depth=malloc(count*sizeof(float));
    float *smooth_depth=malloc(count*sizeof(float));
    assert(flat_color && smooth_color && flat_depth && smooth_depth);
    for (size_t i=0; i<count; ++i) {
        flat_color[i]=smooth_color[i]=UINT32_C(0xffffffff);
        flat_depth[i]=smooth_depth[i]=INFINITY;
    }
    rough_framebuffer flat={flat_color,width,height,width,flat_depth,width};
    rough_framebuffer smooth={smooth_color,width,height,width,
                              smooth_depth,width};
    assert(rough_draw_surface(&flat,shape,yaw,distance) == 0);
    assert(rough_draw_surface_smooth(&smooth,shape,yaw,distance) == 0);
    unsigned different=0u,painted=0u;
    for (size_t i=0; i<count; ++i) {
        assert(memcmp(&flat_depth[i],&smooth_depth[i],
                      sizeof(float)) == 0);
        if (flat_color[i]!=UINT32_C(0xffffffff)) {
            ++painted;
            if (flat_color[i]!=smooth_color[i]) ++different;
        } else assert(smooth_color[i]==UINT32_C(0xffffffff));
    }
    assert(painted > 100u && different > 100u);
    assert(rough_draw_surface_smooth(NULL,shape,yaw,distance)==-1);
    rough_framebuffer no_depth=smooth;
    no_depth.depth=NULL;
    assert(rough_draw_surface_smooth(&no_depth,shape,yaw,distance)==-1);
    free(flat_color);free(smooth_color);free(flat_depth);free(smooth_depth);
}

int main(void) {
    test_analytic_normals();
    test_shader_perspective_and_depth();
    test_normal_is_evaluated_per_fragment();
    test_real_surface_color_only(ROUGH_SURFACE_TORUS,37.0,5.2);
    test_real_surface_color_only(ROUGH_SURFACE_ENNEPER,25.0,5.2);
    test_real_surface_color_only(ROUGH_SURFACE_TORUS,30.0,0.55);
    puts("SMOOTH_NORMALS_PASS analytic normals, perspective color, per-pixel lighting, unchanged z and clipping");
    return 0;
}
