/* Actual historical baseline / 3x3 / 5x5 / auto; no fixed patch loop. */
#include "subpixel/path.h"
#include "subpixel_framebuffer.h"
#include <stdio.h>
#include <string.h>
enum { WIDTH=96, HEIGHT=72, WORDS=WIDTH*HEIGHT, PANELS=4 };
static uint32_t pixels[PANELS][WORDS];
static uint64_t occupied[WORDS],candidate[WORDS],reference[WORDS];
static const cp_path_segment curves[]={
    {1,{{-8,66},{32,12},{0,0},{0,0}}},
    {3,{{8,62},{27,6},{69,70},{88,12}}},
    {3,{{88,12},{69,70},{27,6},{8,62}}},
    {1,{{28,58},{87,43},{0,0},{0,0}}}
};
static uint64_t hash(const void *data,size_t size) {
    const unsigned char *p=data;uint64_t h=UINT64_C(14695981039346656037);
    while(size--){h^=*p++;h*=UINT64_C(1099511628211);}return h;
}
static int render(void) {
    for(unsigned i=0;i<PANELS;i++)for(size_t j=0;j<WORDS;j++)pixels[i][j]=0xffffffffu;
    rough_framebuffer fb={pixels[0],WIDTH,HEIGHT,WIDTH,NULL,0};
    FILE *legacy=tmpfile();if(!legacy)return 0;
    int ok=fputs("P 23 57 92\nA .7\nW 1.04\nM -8 66\nL 32 12\nM 8 62\nC 27 6 69 70 88 12\nM 88 12\nC 69 70 27 6 8 62\nM 28 58\nL 87 43\n",legacy)>=0;
    if(fflush(legacy))ok=0;
    rewind(legacy);if(ok)ok=rough_render_stream(legacy,&fb)==0;
    if(fclose(legacy))ok=0;
    if(!ok)return 0;
    cp_path_strategy strategies[3]={CP_PATH_QUADRATIC,CP_PATH_CUBIC,CP_PATH_AUTO};
    for(unsigned panel=1;panel<4;panel++) {
        memset(occupied,0,sizeof occupied);
        cp_coverage_plane plane={occupied,WORDS,WIDTH,WIDTH,HEIGHT,0,0};
        cp_path_workspace scratch={candidate,reference,WORDS};
        cp_path_options options={.transform={1,0,0,1,0,0},.width=1.04,.tolerance=.01,
            .max_pixel_error=2,.max_depth=24,.max_patches=100000,.max_sample_tests=200000000,.strategy=strategies[panel-1]};
        cp_path_report report;
        if(cp_path_rasterize(&plane,curves,sizeof curves/sizeof curves[0],&options,&scratch,&report)!=CP_PATH_OK)return 0;
        uint64_t before=hash(occupied,sizeof occupied);
        if(cp_path_rasterize(&plane,curves,sizeof curves/sizeof curves[0],&options,&scratch,&report)!=CP_PATH_OK||
            before!=hash(occupied,sizeof occupied))return 0;
        unsigned populated=0,fractional=0;
        for(size_t j=0;j<WORDS;j++){populated+=occupied[j]!=0;fractional+=occupied[j]!=0&&occupied[j]!=UINT64_MAX;}
        if(populated<150||fractional<100)return 0;
        fb.pixels=pixels[panel];
        if(rough_blend_subpixel_coverage(&fb,&plane,0xff17395cu,.7))return 0;
        printf("panel=%u strategy=%d reason=%s patches=%zu tests=%zu worst_xor_bits=%u memory_allowance=%zu\n",
            panel,report.selected,report.reason,report.patches,report.sample_tests,report.worst_pixel_error,
            report.peak_bytes+sizeof occupied+sizeof pixels);
    }
    return memcmp(pixels[0],pixels[1],sizeof pixels[0])!=0;
}
int main(int argc,char **argv) {
    if(argc!=2)return 2;
    if(!render())return 1;
    if(!strcmp(argv[1],"--self-test")){puts("Adaptive whole-path preview: PASS");return 0;}
    FILE *out=fopen(argv[1],"wb");if(!out)return 1;
    int ok=fprintf(out,"P6\n%d %d\n255\n",WIDTH*PANELS,HEIGHT)>0;
    for(unsigned y=0;y<HEIGHT&&ok;y++)for(unsigned panel=0;panel<PANELS&&ok;panel++)for(unsigned x=0;x<WIDTH&&ok;x++) {
        uint32_t p=pixels[panel][(size_t)y*WIDTH+x];
        unsigned char rgb[3]={(unsigned char)(p>>16),(unsigned char)(p>>8),(unsigned char)p};
        ok=fwrite(rgb,1,3,out)==3;
    }
    if(fclose(out))ok=0;
    return ok?0:1;
}
