#include "subpixel/font.h"
#include "../examples/font_5x7.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define CHECK(c) do { if (!(c)) { fprintf(stderr,"font rendering failed: %s:%d: %s\n",__FILE__,__LINE__,#c); return 1; } } while (0)
static uint8_t first[14000], again[14000];

/* Independent exact finite-segment tube; never calls the path producer. */
static unsigned slash_oracle(unsigned x, unsigned y, unsigned scale,
    double phase_x, double phase_y) {
    double ax = .6 * scale + phase_x, ay = 6.35 * scale + phase_y;
    double dx = 3.8 * scale, dy = -5.7 * scale;
    double radius = .36 * scale;
    unsigned count = 0;
    for (unsigned sy = 0; sy < 8; ++sy)
        for (unsigned sx = 0; sx < 8; ++sx) {
            double px = x + (sx + .5)/8 - ax, py = y + (sy + .5)/8 - ay;
            double t = (px*dx + py*dy)/(dx*dx + dy*dy);
            if (t < 0) t = 0;
            if (t > 1) t = 1;
            double ex = px - t*dx, ey = py - t*dy;
            count += ex*ex + ey*ey <= radius*radius;
        }
    return (count*255 + 32)/64;
}

static int slash_tests(void) {
    const unsigned scales[] = {1,3,5,8,14};
    const double phases[] = {-.375,0,.125,.5};
    puts("glyph\tscale\tphase_x\tphase_y\tworst_coverage_error\tfractional_pixels");
    for (unsigned si = 0; si < sizeof scales/sizeof scales[0]; ++si)
        for (unsigned pi = 0; pi < sizeof phases/sizeof phases[0]; ++pi) {
            unsigned scale = scales[si], width = 5*scale, height = 7*scale;
            double px = phases[pi], py = phases[3-pi];
            CHECK(sf57_render_coverage('/',scale,px,py,first,sizeof first));
            unsigned worst = 0, fractional = 0;
            for (unsigned y = 0; y < height; ++y)
                for (unsigned x = 0; x < width; ++x) {
                    unsigned actual = first[y*width+x];
                    unsigned expected = slash_oracle(x,y,scale,px,py);
                    unsigned error = actual > expected ? actual-expected : expected-actual;
                    if (error > worst) worst = error;
                    fractional += actual > 0 && actual < 255;
                }
            CHECK(worst <= 8);
            CHECK(fractional > 0);
            printf("/\t%u\t%.3f\t%.3f\t%u\t%u\n",scale,px,py,worst,fractional);
        }
    return 0;
}

static int alphabet_tests(void) {
    const char *alphabet = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz/\\+-.?:,=";
    const uint32_t units[] = {0x2212,0x00d7,0x00b0,0x00b2,0x00b3,0x03bc,0x00b5};
    for (size_t i = 0; i < strlen(alphabet) + sizeof units/sizeof units[0]; ++i) {
        uint32_t code = i < strlen(alphabet) ? (unsigned char)alphabet[i] : units[i-strlen(alphabet)];
        const sf57_glyph *glyph = sf57_find_glyph(code);
        CHECK(sf57_validate_glyph(glyph));
        CHECK(glyph->advance == 6 && glyph->baseline == 6.5);
        CHECK(sf57_render_coverage(code,3,.125,-.375,first,sizeof first));
        CHECK(sf57_render_coverage(code,3,.125,-.375,again,sizeof again));
        CHECK(memcmp(first,again,315) == 0);
        unsigned ink = 0, fractional = 0;
        for (unsigned j = 0; j < 315; ++j) {
            ink += first[j] > 0;
            fractional += first[j] > 0 && first[j] < 255;
        }
        CHECK(ink > 0 && fractional > 0);
    }
    /* Symbols must retain their meanings, including diagonal orientation. */
    CHECK(sf57_render_coverage('/',5,0,0,first,sizeof first));
    CHECK(sf57_render_coverage('\\',5,0,0,again,sizeof again));
    CHECK(memcmp(first,again,875) != 0);
    CHECK(sf57_render_coverage('-',5,0,0,first,sizeof first));
    CHECK(sf57_render_coverage(0x2212,5,0,0,again,sizeof again));
    CHECK(memcmp(first,again,875) == 0);
    CHECK(sf57_render_coverage('X',5,0,0,first,sizeof first));
    CHECK(sf57_render_coverage(0x00d7,5,0,0,again,sizeof again));
    CHECK(memcmp(first,again,875) != 0);
    return 0;
}

static int atomic_tests(void) {
    memset(first,0xa5,sizeof first);
    CHECK(!sf57_render_coverage('/',0,0,0,first,sizeof first));
    CHECK(!sf57_render_coverage('/',21,0,0,first,sizeof first));
    CHECK(!sf57_render_coverage('/',3,NAN,0,first,sizeof first));
    CHECK(!sf57_render_coverage('/',3,0,.501,first,sizeof first));
    CHECK(!sf57_render_coverage('/',3,0,0,first,314));
    CHECK(!sf57_render_coverage(0x10ffff,3,0,0,first,sizeof first));
    for (size_t i = 0; i < sizeof first; ++i) CHECK(first[i] == 0xa5);
    CHECK(sf57_render_coverage(' ',3,0,0,first,sizeof first));
    for (unsigned i = 0; i < 315; ++i) CHECK(first[i] == 0);
    CHECK(first[315] == 0xa5);
    return 0;
}

/* Actual producer specimens, emitted without another drawing implementation. */
static int specimen(const char *path) {
    enum { SCALE = 3, WIDTH = 480, HEIGHT = 168 };
    uint8_t image[WIDTH*HEIGHT];
    const char *rows[] = {"0123456789 /\\ + - .", "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
        "abcdefghijklmnopqrstuvwxyz", "m/s  m/s2  rad/s  lux  cm"};
    memset(image,255,sizeof image);
    for (unsigned row = 0; row < 4; ++row)
        for (unsigned column = 0; rows[row][column]; ++column) {
            CHECK(sf57_render_coverage((unsigned char)rows[row][column],SCALE,0,0,first,sizeof first));
            for (unsigned y = 0; y < 7*SCALE; ++y)
                for (unsigned x = 0; x < 5*SCALE; ++x)
                    image[(row*42+y+8)*WIDTH+column*18+x+5] = 255-first[y*15+x];
        }
    FILE *out = fopen(path,"wb");
    CHECK(out != NULL);
    CHECK(fprintf(out,"P5\n%d %d\n255\n",WIDTH,HEIGHT) > 0);
    CHECK(fwrite(image,1,sizeof image,out) == sizeof image);
    CHECK(fclose(out) == 0);
    return 0;
}

int main(int argc, char **argv) {
    clock_t start = clock();
    CHECK(slash_tests() == 0);
    CHECK(alphabet_tests() == 0);
    CHECK(atomic_tests() == 0);
    if (argc == 2) CHECK(specimen(argv[1]) == 0);
    printf("host_cpu_seconds\t%.6f\n",(double)(clock()-start)/CLOCKS_PER_SEC);
    puts("PASS continuous sensor alphabet, exact slash, fractional phases, deterministic masks and atomic refusals");
    return 0;
}
