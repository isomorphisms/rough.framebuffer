/* Versioned native geometry consumer. Group enumeration belongs to the
 * producer (e.g. Conway). This file contains no symmetry/group engine. */
#include "subpixel/path.h"
#include "subpixel_framebuffer.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum { MAX_SIDE=512, MAX_SEGMENTS=8192, MAX_SHAPES=2048 };
typedef struct { unsigned id; uint32_t rgba; double opacity,width; size_t first,count; } shape;
static cp_path_segment segments[MAX_SEGMENTS];
static shape shapes[MAX_SHAPES];
static unsigned width,height;
static uint32_t background;
static size_t segment_count,shape_count;
static int end_of_line(const char *line,int end) {
    while(isspace((unsigned char)line[end]))end++;
    return !line[end] || line[end]=='#';
}
static int load(FILE *input) {
    char line[1024],command[32];int stage=0,active=0;
    while(fgets(line,sizeof line,input)) {
        int n=0;unsigned version=0;
        if(!strchr(line,'\n')&&!feof(input))return 0;
        if(sscanf(line," %31s",command)!=1||command[0]=='#')continue;
        if(stage==0) {
            if(sscanf(line,"SUBPIXEL-PATH %u %n",&version,&n)!=1||version!=1||!end_of_line(line,n))return 0;
            stage=1;continue;
        }
        if(stage==1) {
            unsigned rgba;
            if(sscanf(line,"canvas %u %u %x %n",&width,&height,&rgba,&n)!=3||!end_of_line(line,n)||
                !width||!height||width>MAX_SIDE||height>MAX_SIDE)return 0;
            background=rgba;stage=2;continue;
        }
        if(!strcmp(command,"shape")) {
            if(active||shape_count==MAX_SHAPES)return 0;
            shape *s=&shapes[shape_count];unsigned rgba;
            if(sscanf(line,"shape %u %x %lf %lf %n",&s->id,&rgba,&s->opacity,&s->width,&n)!=4||
                !end_of_line(line,n)||!isfinite(s->opacity)||s->opacity<0||s->opacity>1||
                !isfinite(s->width)||s->width<=0||s->width>1024)return 0;
            s->rgba=rgba;s->first=segment_count;s->count=0;active=1;
        }else if(!strcmp(command,"end")) {
            if(!active||sscanf(line,"end %n",&n)!=0||!end_of_line(line,n)||!shapes[shape_count].count)return 0;
            shape_count++;active=0;
        }else {
            if(!active||segment_count==MAX_SEGMENTS)return 0;
            cp_path_segment *s=&segments[segment_count];
            double v[8]={0};int fields=-1;
            if(!strcmp(command,"line")) {
                s->degree=1;fields=sscanf(line,"line %lf %lf %lf %lf %n",v,v+1,v+2,v+3,&n);
            }else if(!strcmp(command,"quad")) {
                s->degree=2;fields=sscanf(line,"quad %lf %lf %lf %lf %lf %lf %n",v,v+1,v+2,v+3,v+4,v+5,&n);
            }else if(!strcmp(command,"cubic")) {
                s->degree=3;fields=sscanf(line,"cubic %lf %lf %lf %lf %lf %lf %lf %lf %n",v,v+1,v+2,v+3,v+4,v+5,v+6,v+7,&n);
            }else return 0;
            if(fields!=(int)(2*(s->degree+1))||!end_of_line(line,n))return 0;
            for(unsigned i=0;i<=s->degree;i++) {
                s->controls[i]=(cp_vec2){v[2*i],v[2*i+1]};
                if(!isfinite(v[2*i])||!isfinite(v[2*i+1])||fabs(v[2*i])>1e6||fabs(v[2*i+1])>1e6)return 0;
            }
            segment_count++;shapes[shape_count].count++;
        }
    }
    return !ferror(input)&&stage==2&&!active&&shape_count>0;
}
static int legacy(const shape *s,rough_framebuffer *frame) {
    FILE *stream=tmpfile();if(!stream)return 0;
    int ok=fprintf(stream,"P %u %u %u\nA %.17g\nW %.17g\n",(s->rgba>>16)&255,(s->rgba>>8)&255,s->rgba&255,
        s->opacity*((s->rgba>>24)&255)/255,s->width)>0;
    for(size_t i=0;i<s->count&&ok;i++) {
        const cp_path_segment *p=&segments[s->first+i];
        ok=fprintf(stream,"M %.17g %.17g\n",p->controls[0].x,p->controls[0].y)>0;
        if(p->degree==1)ok=ok&&fprintf(stream,"L %.17g %.17g\n",p->controls[1].x,p->controls[1].y)>0;
        else {
            cp_vec2 a=p->controls[1],b=p->controls[2],end=p->controls[3];
            if(p->degree==2) {
                end=p->controls[2];
                a=(cp_vec2){(p->controls[0].x+2*p->controls[1].x)/3,(p->controls[0].y+2*p->controls[1].y)/3};
                b=(cp_vec2){(end.x+2*p->controls[1].x)/3,(end.y+2*p->controls[1].y)/3};
            }
            ok=ok&&fprintf(stream,"C %.17g %.17g %.17g %.17g %.17g %.17g\n",a.x,a.y,b.x,b.y,end.x,end.y)>0;
        }
    }
    if(fflush(stream))ok=0;
    rewind(stream);if(ok)ok=rough_render_stream(stream,frame)==0;
    if(fclose(stream))ok=0;
    return ok;
}
static unsigned population(uint64_t bits){unsigned n=0;while(bits){bits&=bits-1;n++;}return n;}
int main(int argc,char **argv) {
    if(argc!=5&&argc!=6) {fputs("usage: path-render baseline|quadratic|cubic|auto INPUT OUTPUT.ppm METRICS.tsv [MASKS.tsv]\n",stderr);return 2;}
    int baseline=!strcmp(argv[1],"baseline");
    cp_path_strategy strategy=CP_PATH_AUTO;
    if(!strcmp(argv[1],"quadratic"))strategy=CP_PATH_QUADRATIC;
    else if(!strcmp(argv[1],"cubic"))strategy=CP_PATH_CUBIC;
    else if(!baseline&&strcmp(argv[1],"auto"))return 2;
    if(baseline&&argc==6){fputs("Historical baseline has no binary sample masks\n",stderr);return 2;}
    FILE *input=fopen(argv[2],"r");if(!input){perror("input");return 1;}
    int valid=load(input);if(fclose(input))valid=0;
    if(!valid){fputs("Invalid or unsupported SUBPIXEL-PATH 1 geometry\n",stderr);return 1;}
    size_t words=(size_t)width*height;
    uint32_t *pixels=malloc(words*sizeof *pixels);
    uint64_t *occupancy=calloc(words,sizeof *occupancy),*candidate=malloc(words*sizeof *candidate),*reference=malloc(words*sizeof *reference);
    if(!pixels||!occupancy||!candidate||!reference){free(pixels);free(occupancy);free(candidate);free(reference);return 1;}
    for(size_t i=0;i<words;i++)pixels[i]=background;
    rough_framebuffer frame={pixels,width,height,width};
    cp_coverage_plane plane={occupancy,words,width,width,height,0,0};
    cp_path_workspace scratch={candidate,reference,words};
    cp_path_report reports[MAX_SHAPES];double elapsed[MAX_SHAPES];
    /* All rendering succeeds in memory before user-facing outputs are opened. */
    int success=1;size_t work=0;
    FILE *maskfile=argc==6?tmpfile():NULL;
    if(argc==6&&!maskfile)success=0;
    if(maskfile)fputs("shape\tx\ty\tmask_hex\tpopcount\n",maskfile);
    for(size_t i=0;i<shape_count&&success;i++) {
        const shape *s=&shapes[i];clock_t started=clock();
        if(baseline){memset(&reports[i],0,sizeof reports[i]);success=legacy(s,&frame);}
        else {
            memset(occupancy,0,words*sizeof *occupancy);
            cp_path_options options={.transform={1,0,0,1,0,0},.width=s->width,.tolerance=.01,
                .max_pixel_error=2,.max_depth=24,.max_patches=100000,.max_sample_tests=200000000,.strategy=strategy};
            cp_path_status status=cp_path_rasterize(&plane,segments+s->first,s->count,&options,&scratch,&reports[i]);
            if(status!=CP_PATH_OK){fprintf(stderr,"Shape %u refused: path status %d\n",s->id,status);success=0;break;}
            work+=reports[i].sample_tests;
            if(work>1000000000){fputs("Scene work budget exceeded\n",stderr);success=0;break;}
            success=rough_blend_subpixel_coverage(&frame,&plane,s->rgba,s->opacity)==0;
            if(maskfile)for(unsigned y=0;y<height;y++)for(unsigned x=0;x<width;x++) {
                uint64_t mask=occupancy[(size_t)y*width+x];
                if(fprintf(maskfile,"%u\t%u\t%u\t%016llx\t%u\n",s->id,x,y,(unsigned long long)mask,population(mask))<0)success=0;
            }
        }
        elapsed[i]=(double)(clock()-started)/CLOCKS_PER_SEC;
    }
    FILE *output=NULL,*metrics=NULL,*masks=NULL;
    if(success){output=fopen(argv[3],"wb");metrics=fopen(argv[4],"w");success=output&&metrics;}
    if(success&&argc==6){masks=fopen(argv[5],"w");success=masks!=NULL;}
    if(success) {
        success=fprintf(output,"P6\n%u %u\n255\n",width,height)>0;
        for(size_t i=0;i<words&&success;i++) {
            unsigned char rgb[3]={(unsigned char)(pixels[i]>>16),(unsigned char)(pixels[i]>>8),(unsigned char)pixels[i]};
            success=fwrite(rgb,1,3,output)==3;
        }
        success=success&&fprintf(metrics,"shape\tstrategy\treason\tpatches_all_attempts\treference_segments\tsample_tests\tattempts\tworst_xor_bits\treference_flatness_px\tCPU_seconds\tworking_byte_allowance\n")>0;
        for(size_t i=0;i<shape_count&&success;i++) {
            cp_path_report *r=&reports[i];
            success=fprintf(metrics,"%u\t%s\t%s\t%zu\t%zu\t%zu\t%u\t%u\t%.17g\t%.6f\t%zu\n",shapes[i].id,
                baseline?"historical-stream":r->selected==CP_PATH_CUBIC?"cubic-5x5":"quadratic-3x3",
                baseline?"historical per-segment composition; work not instrumented":r->reason,r->patches,r->reference_segments,r->sample_tests,
                r->attempts,r->worst_pixel_error,r->reference_flatness,elapsed[i],
                (baseline?words*4:r->peak_bytes+words*12)+sizeof segments+sizeof shapes+sizeof reports+sizeof elapsed)>0;
        }
        if(masks) {
            rewind(maskfile);char copy[4096];size_t n;
            while((n=fread(copy,1,sizeof copy,maskfile))>0)if(fwrite(copy,1,n,masks)!=n)success=0;
            if(ferror(maskfile))success=0;
        }
    }
    if(output&&fclose(output))success=0;
    if(metrics&&fclose(metrics))success=0;
    if(masks&&fclose(masks))success=0;
    if(maskfile&&fclose(maskfile))success=0;
    free(pixels);free(occupancy);free(candidate);free(reference);
    return success?0:1;
}
