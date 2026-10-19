#include "subpixel/path.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
enum { W=24,H=20,S=27,N=S*H };
static uint64_t destination[N],saved[N],candidate[W*H],reference[W*H];
static int cancelled(void *context){return *(int *)context;}
int main(void) {
    const cp_path_segment paths[]={
        {1,{{-5,17},{14,3},{0,0},{0,0}}},
        {3,{{1,17},{4,-2},{18,23},{23,1}}}
    };
    cp_coverage_plane plane={destination,N,S,W,H,0,0};
    cp_path_workspace work={candidate,reference,W*H};
    cp_path_options options={.transform={1,0,0,1,0,0},.width=.8,.tolerance=.01,
        .max_pixel_error=2,.max_depth=24,.max_patches=100000,.max_sample_tests=20000000};
    for(unsigned forced=CP_PATH_AUTO;forced<=CP_PATH_CUBIC;forced++) {
        memset(destination,0,sizeof destination);
        for(unsigned y=0;y<H;y++)for(unsigned x=W;x<S;x++)destination[y*S+x]=UINT64_C(0x5a5a);
        options.strategy=(cp_path_strategy)forced;cp_path_report report;
        assert(cp_path_rasterize(&plane,paths,2,&options,&work,&report)==CP_PATH_OK);
        unsigned populated=0;
        for(unsigned y=0;y<H;y++)for(unsigned x=0;x<S;x++) {
            if(x<W)populated+=destination[y*S+x]!=0;
            else assert(destination[y*S+x]==UINT64_C(0x5a5a));
        }
        assert(populated>30&&report.patches>0&&report.worst_pixel_error<=2);
        memcpy(saved,destination,sizeof saved);
        assert(cp_path_rasterize(&plane,paths,2,&options,&work,&report)==CP_PATH_OK);
        assert(memcmp(saved,destination,sizeof saved)==0);
        cp_path_report before=report;
        size_t budget=options.max_sample_tests;options.max_sample_tests=1;
        assert(cp_path_rasterize(&plane,paths,2,&options,&work,&report)==CP_PATH_BUDGET);
        assert(!memcmp(saved,destination,sizeof saved)&&!memcmp(&before,&report,sizeof report));
        options.max_sample_tests=budget;
        int stop=1;options.cancelled=cancelled;options.cancel_context=&stop;
        assert(cp_path_rasterize(&plane,paths,2,&options,&work,&report)==CP_PATH_CANCELLED);
        assert(!memcmp(saved,destination,sizeof saved));options.cancelled=NULL;
        work.candidate=destination;
        assert(cp_path_rasterize(&plane,paths,2,&options,&work,&report)==CP_PATH_INVALID);
        assert(!memcmp(saved,destination,sizeof saved));work.candidate=candidate;
    }
    puts("Adaptive path clipping/stride/union/budget/cancellation: PASS");
    return 0;
}
