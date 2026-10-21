#include "subpixel/path.h"
#include <math.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    cp_coverage_plane plane;
    const cp_path_options *options;
    cp_path_report report;
    double step, tolerance;
    int reference;
} path_context;
static int finite_point(cp_vec2 p) { return isfinite(p.x) && isfinite(p.y); }
static cp_vec2 middle(cp_vec2 a, cp_vec2 b) {
    return (cp_vec2){(a.x+b.x)*.5,(a.y+b.y)*.5};
}
static double separation(cp_vec2 a, cp_vec2 b) { return hypot(a.x-b.x,a.y-b.y); }
static double dot(cp_vec2 a, cp_vec2 b) { return a.x*b.x+a.y*b.y; }
static cp_vec2 difference(cp_vec2 a, cp_vec2 b) {return (cp_vec2){a.x-b.x,a.y-b.y};}
static cp_vec2 position(const cp_vec2 p[4], double t) {
    cp_vec2 a[3], b[2];
    for(unsigned i=0;i<3;i++) a[i]=(cp_vec2){p[i].x*(1-t)+p[i+1].x*t,p[i].y*(1-t)+p[i+1].y*t};
    for(unsigned i=0;i<2;i++) b[i]=(cp_vec2){a[i].x*(1-t)+a[i+1].x*t,a[i].y*(1-t)+a[i+1].y*t};
    return (cp_vec2){b[0].x*(1-t)+b[1].x*t,b[0].y*(1-t)+b[1].y*t};
}
static void split(const cp_vec2 p[4], cp_vec2 left[4], cp_vec2 right[4]) {
    cp_vec2 a=middle(p[0],p[1]),b=middle(p[1],p[2]),c=middle(p[2],p[3]);
    cp_vec2 d=middle(a,b),e=middle(b,c),f=middle(d,e);
    left[0]=p[0];left[1]=a;left[2]=d;left[3]=f;
    right[0]=f;right[1]=e;right[2]=c;right[3]=p[3];
}
static double segment_distance(cp_vec2 q, cp_vec2 a, cp_vec2 b) {
    cp_vec2 d=difference(b,a),v=difference(q,a);
    double length2=dot(d,d), t=length2?dot(v,d)/length2:0;
    t=fmax(0,fmin(1,t));
    return hypot(v.x-t*d.x,v.y-t*d.y);
}
static cp_path_status charge(path_context *c,size_t samples) {
    if(c->options->cancelled && c->options->cancelled(c->options->cancel_context)) return CP_PATH_CANCELLED;
    if(samples>c->options->max_sample_tests-c->report.sample_tests) return CP_PATH_BUDGET;
    c->report.sample_tests+=samples;
    return CP_PATH_OK;
}
/* Bounds are clipped in double before integer conversion. */
static int bounds(path_context *c,double minx,double miny,double maxx,double maxy,int *x0,int *y0,int *x1,int *y1) {
    double left=c->plane.origin_x,top=c->plane.origin_y;
    double right=left+c->plane.width-1,bottom=top+c->plane.height-1;
    if(maxx<left-.5 || maxy<top-.5 || minx>right+.5 || miny>bottom+.5)return 0;
    *x0=(int)fmax(left,floor(minx));*y0=(int)fmax(top,floor(miny));
    *x1=(int)fmin(right,ceil(maxx));*y1=(int)fmin(bottom,ceil(maxy));
    return 1;
}
static cp_path_status capsule(path_context *c,cp_vec2 a,cp_vec2 b,double radius) {
    int x0,y0,x1,y1;
    if(!bounds(c,fmin(a.x,b.x)-radius-.5,fmin(a.y,b.y)-radius-.5,
        fmax(a.x,b.x)+radius+.5,fmax(a.y,b.y)+radius+.5,&x0,&y0,&x1,&y1))return CP_PATH_OK;
    for(int64_t y=y0;y<=y1;y++)for(int64_t x=x0;x<=x1;x++) {
        cp_path_status status=charge(c,64);if(status)return status;
        uint64_t mask=0;
        for(unsigned sy=0;sy<8;sy++)for(unsigned sx=0;sx<8;sx++) {
            cp_vec2 sample={(double)x+((double)sx+.5)/8-.5,(double)y+((double)sy+.5)/8-.5};
            if(segment_distance(sample,a,b)<=radius)mask|=UINT64_C(1)<<(8*sy+sx);
        }
        size_t index=(size_t)(y-c->plane.origin_y)*c->plane.stride_words+(size_t)(x-c->plane.origin_x);
        c->plane.occupied[index]|=mask;
    }
    return CP_PATH_OK;
}
static cp_path_status accumulate(path_context *c,const cp_curve_patch *p) {
    int x0,y0,x1,y1;
    double u=p->valid_u,v=fabs(p->a2)*u*u+fabs(p->a3)*u*u*u+p->half_width;
    double dx=fabs(p->tangent.x)*u+fabs(p->normal.x)*v;
    double dy=fabs(p->tangent.y)*u+fabs(p->normal.y)*v;
    if(!bounds(c,p->origin.x-dx-.5,p->origin.y-dy-.5,p->origin.x+dx+.5,p->origin.y+dy+.5,&x0,&y0,&x1,&y1))return CP_PATH_OK;
    int side=c->report.selected==CP_PATH_CUBIC?5:3;
    for(int64_t y=y0;y<=y1;y+=side)for(int64_t x=x0;x<=x1;x+=side) {
        cp_path_status status=charge(c,(size_t)side*(size_t)side*64);if(status)return status;
        /* Keep centers inside int even at the extreme viewport boundary. */
        if(x+side/2>INT32_MAX || y+side/2>INT32_MAX)return CP_PATH_INVALID;
        cp_status s=side==5?cp_coverage_plane_accumulate_5x5(&c->plane,p,(int)x+2,(int)y+2):
            cp_coverage_plane_accumulate_3x3(&c->plane,p,(int)x+1,(int)y+1);
        if(s!=CP_OK)return CP_PATH_INVALID;
    }
    return CP_PATH_OK;
}
static int outside(const path_context *c,const cp_vec2 p[4]) {
    double r=c->options->width*.5+1;
    double left=c->plane.origin_x-r,top=c->plane.origin_y-r;
    double right=(double)c->plane.origin_x+c->plane.width-1+r;
    double bottom=(double)c->plane.origin_y+c->plane.height-1+r;
    unsigned l=0,t=0,rr=0,b=0;
    for(unsigned i=0;i<4;i++){l+=p[i].x<left;t+=p[i].y<top;rr+=p[i].x>right;b+=p[i].y>bottom;}
    return l==4||t==4||rr==4||b==4;
}
static cp_path_status partition(path_context *c,const cp_vec2 p[4],unsigned depth) {
    cp_path_status status=charge(c,1);if(status)return status;
    if(outside(c,p))return CP_PATH_OK;
    double length=separation(p[0],p[1])+separation(p[1],p[2])+separation(p[2],p[3]);
    double flatness=fmax(segment_distance(p[1],p[0],p[3]),segment_distance(p[2],p[0],p[3]));
    double radius=c->options->width*.5;
    if(c->reference && flatness<=c->report.reference_flatness) {
        if(c->report.reference_segments>=c->options->max_patches)return CP_PATH_BUDGET;
        c->report.reference_segments++;
        return capsule(c,p[0],p[3],radius);
    }
    if(!c->reference && length<=1e-12) {
        if(c->report.patches>=c->options->max_patches)return CP_PATH_BUDGET;
        c->report.patches++;
        return capsule(c,p[0],p[0],radius);
    }
    if(!c->reference && length<=c->step) {
        cp_curve_patch patch;
        cp_status built=c->report.selected==CP_PATH_CUBIC?
            cp_patch_from_cubic_jet3(&patch,p,.5,radius,1):cp_patch_from_cubic(&patch,p,.5,radius,1);
        if(built==CP_OK) {
            double first=dot(difference(p[0],patch.origin),patch.tangent);
            double last=dot(difference(p[3],patch.origin),patch.tangent);
            int valid=first<0 && last>0;
            for(unsigned i=0;i<3;i++) if(dot(difference(p[i+1],p[i]),patch.tangent)<=0)valid=0;
            patch.valid_u=fmax(-first,last);
            double error=0;
            for(unsigned i=0;i<=8;i++) {
                cp_vec2 offset=difference(position(p,(double)i/8),patch.origin);
                double u=dot(offset,patch.tangent),v=dot(offset,patch.normal);
                double graph=patch.a2*u*u+patch.a3*u*u*u;
                error=fmax(error,fabs(v-graph));
            }
            double slope=2*fabs(patch.a2)*patch.valid_u+3*fabs(patch.a3)*patch.valid_u*patch.valid_u;
            if(valid && error<=c->tolerance && radius*slope*slope<=c->tolerance) {
                if(c->report.patches>=c->options->max_patches)return CP_PATH_BUDGET;
                c->report.patches++;
                status=accumulate(c,&patch);if(status)return status;
                status=capsule(c,p[0],p[0],radius);if(status)return status;
                return capsule(c,p[3],p[3],radius);
            }
        }
    }
    if(depth>=c->options->max_depth)return CP_PATH_NONCONVERGENCE;
    cp_vec2 left[4],right[4];split(p,left,right);
    status=partition(c,left,depth+1);if(status)return status;
    return partition(c,right,depth+1);
}
static int overlaps(const void *a,size_t an,const void *b,size_t bn) {
    uintptr_t aa=(uintptr_t)a,bb=(uintptr_t)b;
    if(an>UINTPTR_MAX-aa || bn>UINTPTR_MAX-bb)return 1;
    return aa<bb+bn && bb<aa+an;
}
static int controls(const cp_path_segment *segment,const cp_path_transform *m,cp_vec2 p[4]) {
    if(segment->degree<1 || segment->degree>3)return 0;
    cp_vec2 q[4];
    for(unsigned i=0;i<=segment->degree;i++) {
        cp_vec2 a=segment->controls[i];if(!finite_point(a))return 0;
        q[i]=(cp_vec2){m->xx*a.x+m->xy*a.y+m->tx,m->yx*a.x+m->yy*a.y+m->ty};
        if(!finite_point(q[i])||fabs(q[i].x)>1e6||fabs(q[i].y)>1e6)return 0;
    }
    p[0]=q[0];p[3]=q[segment->degree];
    if(segment->degree==3){p[1]=q[1];p[2]=q[2];}
    else if(segment->degree==2) {
        p[1]=(cp_vec2){(q[0].x+2*q[1].x)/3,(q[0].y+2*q[1].y)/3};
        p[2]=(cp_vec2){(q[2].x+2*q[1].x)/3,(q[2].y+2*q[1].y)/3};
    }else {
        p[1]=(cp_vec2){(2*q[0].x+q[1].x)/3,(2*q[0].y+q[1].y)/3};
        p[2]=(cp_vec2){(q[0].x+2*q[1].x)/3,(q[0].y+2*q[1].y)/3};
    }
    return 1;
}
static unsigned bits(uint64_t word){unsigned n=0;while(word){word&=word-1;n++;}return n;}
cp_path_status cp_path_rasterize(cp_coverage_plane *destination,
    const cp_path_segment *segments,size_t count,const cp_path_options *options,
    cp_path_workspace *workspace,cp_path_report *report) {
    if(cp_coverage_plane_validate(destination)!=CP_OK || !segments || !count || count>2048 ||
       !options || !workspace || !report || !workspace->candidate || !workspace->reference ||
       !isfinite(options->width)||options->width<=0||options->width>1024 ||
       !isfinite(options->tolerance)||options->tolerance<1e-5||options->tolerance>.125 ||
       options->max_depth<1||options->max_depth>24||!options->max_patches||!options->max_sample_tests||
       options->max_pixel_error>64||options->strategy<CP_PATH_AUTO||options->strategy>CP_PATH_CUBIC)return CP_PATH_INVALID;
    const cp_path_transform *m=&options->transform;
    if(!isfinite(m->xx)||!isfinite(m->xy)||!isfinite(m->yx)||!isfinite(m->yy)||!isfinite(m->tx)||!isfinite(m->ty))return CP_PATH_INVALID;
    size_t width=destination->width,height=destination->height;
    if(height>SIZE_MAX/width)return CP_PATH_INVALID;
    size_t words=width*height;
    if(words>SIZE_MAX/sizeof(uint64_t)||words>workspace->capacity_words ||
        destination->capacity_words>SIZE_MAX/sizeof(uint64_t))return CP_PATH_INVALID;
    size_t bytes=words*sizeof(uint64_t),destbytes=destination->capacity_words*sizeof(uint64_t);
    if(overlaps(workspace->candidate,bytes,workspace->reference,bytes)||
       overlaps(workspace->candidate,bytes,destination->occupied,destbytes)||
       overlaps(workspace->reference,bytes,destination->occupied,destbytes))return CP_PATH_INVALID;
    cp_path_strategy selected=options->strategy;
    const char *reason="manual strategy";
    double max_cubic=0;
    /* Validate ALL source before touching scratch. AUTO compares the cubic
     * residual with requested tolerance at screen scale; a small budget favors
     * the cheaper footprint. Forced algorithms never silently change. */
    for(size_t i=0;i<count;i++) {
        cp_vec2 p[4];if(!controls(&segments[i],m,p))return CP_PATH_INVALID;
        cp_curve_patch patch;
        if(cp_patch_from_cubic_jet3(&patch,p,.5,options->width*.5,1)==CP_OK)max_cubic=fmax(max_cubic,fabs(patch.a3));
    }
    if(selected==CP_PATH_AUTO) {
        selected=max_cubic>options->tolerance*.1 && options->max_sample_tests>=1600?CP_PATH_CUBIC:CP_PATH_QUADRATIC;
        reason=selected==CP_PATH_CUBIC?"screen cubic residual exceeds quality threshold":"quadratic adequate or smaller footprint budget";
    }
    path_context c={0};
    c.options=options;c.report.selected=selected;c.report.reason=reason;
    size_t stack_allowance=4096*((size_t)options->max_depth+2);
    if(bytes>(SIZE_MAX-stack_allowance)/2)return CP_PATH_INVALID;
    c.report.peak_bytes=2*bytes+stack_allowance;
    c.report.reference_flatness=fmin(options->tolerance/32,1.0/8192);
    c.plane=(cp_coverage_plane){workspace->reference,words,width,(unsigned)width,(unsigned)height,destination->origin_x,destination->origin_y};
    memset(workspace->reference,0,bytes);c.reference=1;
    for(size_t i=0;i<count;i++) {
        cp_vec2 p[4];if(!controls(&segments[i],m,p))return CP_PATH_INVALID;
        cp_path_status status=partition(&c,p,0);if(status)return status;
    }
    c.reference=0;c.plane.occupied=workspace->candidate;
    for(unsigned attempt=0;attempt<8;attempt++) {
        memset(workspace->candidate,0,bytes);
        c.step=1.0/(double)(1u<<attempt);c.tolerance=options->tolerance/(double)(1u<<attempt);
        c.report.attempts=attempt+1;
        for(size_t i=0;i<count;i++) {
            cp_vec2 p[4];if(!controls(&segments[i],m,p))return CP_PATH_INVALID;
            cp_path_status status=partition(&c,p,0);if(status)return status;
        }
        c.report.worst_pixel_error=0;
        for(size_t i=0;i<words;i++) {
            unsigned error=bits(workspace->candidate[i]^workspace->reference[i]);
            if(error>c.report.worst_pixel_error)c.report.worst_pixel_error=error;
        }
        if(c.report.worst_pixel_error<=options->max_pixel_error) {
            /* No callback or fallible operation after commit begins. */
            cp_path_status status=charge(&c,0);if(status)return status;
            for(size_t y=0;y<height;y++)for(size_t x=0;x<width;x++)
                destination->occupied[y*destination->stride_words+x]|=workspace->candidate[y*width+x];
            *report=c.report;return CP_PATH_OK;
        }
    }
    return CP_PATH_NONCONVERGENCE;
}
