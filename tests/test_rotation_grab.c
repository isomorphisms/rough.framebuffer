/* Independent quaternion sandwich oracle for actual torus landmarks.
 * Production uses a derived matrix; this test multiplies pure imaginary
 * quaternions explicitly before perspective division. */
#include "touch.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

typedef struct { double w,x,y,z; } oracle_quaternion;
static oracle_quaternion multiply(oracle_quaternion a, oracle_quaternion b) {
    return (oracle_quaternion){a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z,
        a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,
        a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,
        a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w};
}
static double landmark(rough_rotation rotation, double z, int vertical) {
    oracle_quaternion q={rotation.w,rotation.x,rotation.y,rotation.z};
    oracle_quaternion conjugate={q.w,-q.x,-q.y,-q.z};
    const double diagonal=1.18/sqrt(2.0);
    oracle_quaternion result=multiply(multiply(q,(oracle_quaternion){0,diagonal,diagonal,z}),conjugate);
    return (vertical ? result.y : result.x)/(5.2+result.z);
}
static int wrong(const char *reason) {
    fprintf(stderr,"FRONT_HEMISPHERE_WRONG_DIRECTION: %s\n",reason);
    return 1;
}
#define REQUIRE(test, reason) do { if (!(test)) return wrong(reason); } while (0)

int main(void) {
    const double pi=3.14159265358979323846;
    for (unsigned vertical=0;vertical<2;++vertical)
        for (int direction=-1;direction<=1;direction+=2)
            for (unsigned previous_roll=0;previous_roll<2;++previous_roll) {
                rough_touch_state state;
                REQUIRE(rough_touch_init(&state,160,320)==0,"init");
                double roll=previous_roll*pi/18;
                state.orientation=(rough_rotation){cos(roll/2),0,0,sin(roll/2)};
                rough_rotation initial=state.orientation;
                double near_start=landmark(initial,-.43,(int)vertical);
                double far_start=landmark(initial,.43,(int)vertical);
                double dx=vertical ? 0 : direction*20;
                double dy=vertical ? direction*40 : 0;
                REQUIRE(rough_touch_apply(&state,ROUGH_TOUCH_DOWN,19,60,80)==0,"down");
                REQUIRE(rough_touch_apply(&state,ROUGH_TOUCH_MOVE,19,60+dx,80+dy)==1,"drag inert");
                double near_move=landmark(state.orientation,-.43,(int)vertical)-near_start;
                double far_move=landmark(state.orientation,.43,(int)vertical)-far_start;
                REQUIRE(direction*near_move>0.001,"near surface opposes finger");
                REQUIRE(direction*far_move<-.001,"far surface follows finger");
                REQUIRE(state.camera_distance==5.2,"one-finger drag zoomed");
                double norm=state.orientation.w*state.orientation.w+state.orientation.x*state.orientation.x+
                    state.orientation.y*state.orientation.y+state.orientation.z*state.orientation.z;
                REQUIRE(fabs(norm-1)<1e-12,"rotation not unit length");
                REQUIRE(rough_touch_apply(&state,ROUGH_TOUCH_CANCEL,19,60+dx,80+dy)==1,"cancel inert");
                REQUIRE(memcmp(&state.orientation,&initial,sizeof initial)==0,"cancel lost orientation");
            }
    rough_rotation identity={1,0,0,0}, horizontal, vertical, first, second;
    REQUIRE(rough_rotation_drag(identity,pi/2,0,&horizontal)==0,"horizontal delta");
    REQUIRE(rough_rotation_drag(identity,0,pi/2,&vertical)==0,"vertical delta");
    REQUIRE(rough_rotation_drag(horizontal,0,pi/2,&first)==0,"composition");
    REQUIRE(rough_rotation_drag(vertical,pi/2,0,&second)==0,"reverse composition");
    REQUIRE(fabs(first.z-second.z)>.5,"orthogonal rotations incorrectly commute");
    /* Rendering's matrix must agree with this independent sandwich. */
    double matrix[9];
    REQUIRE(rough_rotation_matrix(first,matrix)==0,"matrix conversion");
    double diagonal=1.18/sqrt(2.0);
    double x=matrix[0]*diagonal+matrix[1]*diagonal-matrix[2]*.43;
    double y=matrix[3]*diagonal+matrix[4]*diagonal-matrix[5]*.43;
    double z=matrix[6]*diagonal+matrix[7]*diagonal-matrix[8]*.43;
    REQUIRE(fabs(x/(5.2+z)-landmark(first,-.43,0))<1e-12,"matrix horizontal mismatch");
    REQUIRE(fabs(y/(5.2+z)-landmark(first,-.43,1))<1e-12,"matrix vertical mismatch");
    rough_rotation opposite={-first.w,-first.x,-first.y,-first.z};
    double same[9];
    REQUIRE(rough_rotation_matrix(opposite,same)==0,"opposite quaternion");
    for(unsigned i=0;i<9;++i) REQUIRE(fabs(same[i]-matrix[i])<1e-12,"q/-q mismatch");
    REQUIRE(rough_rotation_matrix((rough_rotation){0,0,0,0},same)==-1,"zero quaternion admitted");
    REQUIRE(rough_rotation_matrix((rough_rotation){NAN,0,0,0},same)==-1,"NaN quaternion admitted");
    for(unsigned i=0;i<10000;++i)
        REQUIRE(rough_rotation_drag(first,.003,-.002,&first)==0,"repeated composition");
    REQUIRE(fabs(first.w*first.w+first.x*first.x+first.y*first.y+first.z*first.z-1)<1e-12,"unit drift");
    puts("FRONT_HEMISPHERE_GRAB_PASS: both screen axes; unit quaternion composition");
    return 0;
}
