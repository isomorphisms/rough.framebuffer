/* 5x7 design coordinates. Lines stay lines at every scale and sample phase.
 * Curves are authored Beziers. No bitmap interpolation enters this table.
 * Individual open strokes are unioned before coverage is resolved. */
#define L(x,y,u,v) LINE(P(x,y),P(u,v))
#define Q(x,y,u,v,a,b) QUAD(P(x,y),P(u,v),P(a,b))
#define B(x,y,u,v,a,b,c,d) CUBIC(P(x,y),P(u,v),P(a,b),P(c,d))
#define ELLIPSE(l,t,r,b) \
    B(((l)+(r))/2,t, (r)-.224*((r)-(l)),t, r,(t)+.224*((b)-(t)), r,((t)+(b))/2), \
    B(r,((t)+(b))/2, r,(b)-.224*((b)-(t)), (r)-.224*((r)-(l)),b, ((l)+(r))/2,b), \
    B(((l)+(r))/2,b, (l)+.224*((r)-(l)),b, l,(b)-.224*((b)-(t)), l,((t)+(b))/2), \
    B(l,((t)+(b))/2, l,(t)+.224*((b)-(t)), (l)+.224*((r)-(l)),t, ((l)+(r))/2,t)
#define SEGMENTS(n,...) static const sf57_segment n[] = {__VA_ARGS__}
static const sf57_subpath strokes1[] = {{0,1,0}};
static const sf57_subpath strokes2[] = {{0,1,0},{1,1,0}};
static const sf57_subpath strokes3[] = {{0,1,0},{1,1,0},{2,1,0}};
static const sf57_subpath strokes4[] = {{0,1,0},{1,1,0},{2,1,0},{3,1,0}};
static const sf57_subpath strokes5[] = {{0,1,0},{1,1,0},{2,1,0},{3,1,0},{4,1,0}};
static const sf57_subpath loop4[] = {{0,4,1}};
static const sf57_subpath loop_tail[] = {{0,4,1},{4,1,0}};
static const sf57_subpath d_paths[] = {{0,3,1}};
static const sf57_subpath p_paths[] = {{0,1,0},{1,2,1}};
static const sf57_subpath r_paths[] = {{0,1,0},{1,2,1},{3,1,0}};

SEGMENTS(slash, L(.6,6.35,4.4,.65));
SEGMENTS(backslash, L(.6,.65,4.4,6.35));
SEGMENTS(plus, L(.7,3.5,4.3,3.5),L(2.5,1.5,2.5,5.5));
SEGMENTS(minus, L(.7,3.5,4.3,3.5));
SEGMENTS(times, L(.9,1.9,4.1,5.1),L(.9,5.1,4.1,1.9));
SEGMENTS(dot, L(2.5,6.0,2.5,6.05));
SEGMENTS(colon, L(2.5,2.4,2.5,2.45),L(2.5,5.3,2.5,5.35));
SEGMENTS(comma, Q(2.7,5.8,2.8,6.4,2.0,6.6));
SEGMENTS(equal, L(.7,2.6,4.3,2.6),L(.7,4.4,4.3,4.4));
SEGMENTS(question, B(.9,1.7,.9,.2,4.1,.2,4.1,1.9),
    B(4.1,1.9,4.1,3.1,2.5,3.0,2.5,4.4),L(2.5,6.0,2.5,6.05));
SEGMENTS(zero, ELLIPSE(.9,.65,4.1,6.35));
SEGMENTS(one, L(1.3,2.0,2.7,.7),L(2.7,.7,2.7,6.3),L(1.1,6.3,4.2,6.3));
SEGMENTS(two, B(.8,1.9,.8,.1,4.2,.1,4.2,2.0),
    B(4.2,2.0,4.2,3.2,1.8,4.5,.8,6.3),L(.8,6.3,4.2,6.3));
SEGMENTS(three, B(.9,.9,5.2,-.3,4.9,3.5,2.4,3.5),
    B(2.4,3.5,5.1,3.5,5.0,7.2,.8,6.1));
SEGMENTS(four, L(3.7,6.3,3.7,.7),L(3.7,.7,.7,4.5),L(.7,4.5,4.4,4.5));
SEGMENTS(five, L(4.1,.7,1.0,.7),L(1.0,.7,.9,3.4),
    B(.9,3.4,4.9,1.8,5.1,7.7,.8,6.0));
SEGMENTS(six, B(3.9,.8,1.3,-.1,.6,3.5,.9,4.8),
    B(.9,4.8,1.2,7.4,4.5,6.8,4.2,4.6),B(4.2,4.6,4.1,2.7,1.8,2.7,.9,4.0));
SEGMENTS(seven, L(.7,.7,4.3,.7),L(4.3,.7,1.5,6.3));
SEGMENTS(nine, B(1.1,6.2,3.7,7.1,4.4,3.5,4.1,2.2),
    B(4.1,2.2,3.8,-.4,.5,.2,.8,2.4),B(.8,2.4,.9,4.3,3.2,4.3,4.1,3.0));
SEGMENTS(a_upper, L(.7,6.3,2.5,.7),L(2.5,.7,4.3,6.3),L(1.35,4.3,3.65,4.3));
SEGMENTS(b_upper, L(.9,6.3,.9,.7),B(.9,.7,5.3,.3,5.3,3.5,.9,3.5),
    B(.9,3.5,5.5,3.0,5.5,6.8,.9,6.3));
SEGMENTS(d_upper, L(.9,6.3,.9,.7),B(.9,.7,5.2,.3,5.2,6.7,.9,6.3),L(.9,6.3,.9,6.3));
SEGMENTS(e_upper, L(4.2,.7,.9,.7),L(.9,.7,.9,6.3),L(.9,6.3,4.2,6.3),L(.9,3.4,3.7,3.4));
SEGMENTS(f_upper, L(.9,6.3,.9,.7),L(.9,.7,4.2,.7),L(.9,3.4,3.7,3.4));
SEGMENTS(g_upper, B(4.0,1.1,.0,-1.5,-.4,7.9,4.1,6.0),L(4.1,6.0,4.1,3.7),L(4.1,3.7,2.7,3.7));
SEGMENTS(h_upper, L(.9,.7,.9,6.3),L(4.1,.7,4.1,6.3),L(.9,3.5,4.1,3.5));
SEGMENTS(i_upper, L(.9,.7,4.1,.7),L(2.5,.7,2.5,6.3),L(.9,6.3,4.1,6.3));
SEGMENTS(j_upper, L(1.5,.7,4.1,.7),L(3.4,.7,3.4,4.8),B(3.4,4.8,3.4,6.9,.7,6.9,.7,5.0));
SEGMENTS(k_upper, L(.9,.7,.9,6.3),L(4.1,.7,.9,3.8),L(2.2,2.55,4.2,6.3));
SEGMENTS(l_upper, L(.9,.7,.9,6.3),L(.9,6.3,4.2,6.3));
SEGMENTS(m_upper, L(.7,6.3,.7,.7),L(.7,.7,2.5,3.8),L(2.5,3.8,4.3,.7),L(4.3,.7,4.3,6.3));
SEGMENTS(n_upper, L(.9,6.3,.9,.7),L(.9,.7,4.1,6.3),L(4.1,6.3,4.1,.7));
SEGMENTS(p_upper, L(.9,6.3,.9,.7),B(.9,.7,5.3,.2,5.3,3.8,.9,3.5),L(.9,3.5,.9,.7));
SEGMENTS(q_upper, ELLIPSE(.9,.65,4.1,6.35),L(2.9,4.9,4.4,6.5));
SEGMENTS(r_upper, L(.9,6.3,.9,.7),B(.9,.7,5.3,.2,5.3,3.8,.9,3.5),L(.9,3.5,.9,.7),L(2.5,3.5,4.2,6.3));
SEGMENTS(t_upper, L(.7,.7,4.3,.7),L(2.5,.7,2.5,6.3));
SEGMENTS(u_upper, L(.9,.7,.9,4.8),B(.9,4.8,.9,6.9,4.1,6.9,4.1,4.8),L(4.1,4.8,4.1,.7));
SEGMENTS(v_upper, L(.7,.7,2.5,6.3),L(2.5,6.3,4.3,.7));
SEGMENTS(w_upper, L(.6,.7,1.3,6.3),L(1.3,6.3,2.5,3.7),L(2.5,3.7,3.7,6.3),L(3.7,6.3,4.4,.7));
SEGMENTS(x_upper, L(.8,.7,4.2,6.3),L(.8,6.3,4.2,.7));
SEGMENTS(y_upper, L(.8,.7,2.5,3.5),L(4.2,.7,2.5,3.5),L(2.5,3.5,2.5,6.3));
SEGMENTS(z_upper, L(.8,.7,4.2,.7),L(4.2,.7,.8,6.3),L(.8,6.3,4.2,6.3));

SEGMENTS(a_lower, ELLIPSE(.9,2.5,3.9,6.3),L(3.9,2.5,3.9,6.3));
SEGMENTS(b_lower, ELLIPSE(.9,2.5,4.1,6.3),L(.9,.7,.9,6.3));
SEGMENTS(c_lower, B(4.0,2.9,-.3,.0,-.3,8.8,4.0,5.9));
SEGMENTS(d_lower, ELLIPSE(.9,2.5,4.1,6.3),L(4.1,.7,4.1,6.3));
SEGMENTS(e_lower, L(.9,4.3,4.1,4.3),B(4.1,4.3,4.1,1.6,.7,1.7,.9,4.6),
    B(.9,4.6,1.0,6.2,2.6,6.9,4.0,5.9));
SEGMENTS(f_lower, B(3.8,.9,1.0,-.2,1.7,3.3,1.7,6.3),L(.7,2.8,3.5,2.8));
SEGMENTS(g_lower, ELLIPSE(.9,2.1,4.0,5.2),B(4.0,2.1,4.0,6.7,4.1,7.2,1.2,6.2));
SEGMENTS(h_lower, L(.9,.7,.9,6.3),B(.9,3.6,1.9,1.9,4.1,2.1,4.1,4.1),L(4.1,4.1,4.1,6.3));
SEGMENTS(i_lower, L(2.5,1.0,2.5,1.05),L(2.5,2.8,2.5,6.3),L(1.4,6.3,3.6,6.3));
SEGMENTS(j_lower, L(3.1,1.0,3.1,1.05),L(3.1,2.8,3.1,5.8),B(3.1,5.8,3.1,6.9,1.4,6.9,1.0,6.1));
SEGMENTS(k_lower, L(.9,.7,.9,6.3),L(3.9,2.6,.9,4.6),L(2.1,3.8,4.1,6.3));
SEGMENTS(l_lower, L(2.2,.7,2.2,5.6),Q(2.2,5.6,2.2,6.3,3.5,6.3));
SEGMENTS(m_lower, L(.7,2.6,.7,6.3),B(.7,3.4,.7,2.1,2.5,2.1,2.5,3.7),
    L(2.5,3.7,2.5,6.3),B(2.5,3.7,2.5,2.1,4.3,2.1,4.3,3.7),L(4.3,3.7,4.3,6.3));
SEGMENTS(n_lower, L(.9,2.6,.9,6.3),B(.9,3.6,1.9,1.9,4.1,2.1,4.1,4.1),L(4.1,4.1,4.1,6.3));
SEGMENTS(o_lower, ELLIPSE(.9,2.5,4.1,6.3));
SEGMENTS(p_lower, ELLIPSE(.9,2.1,4.1,5.2),L(.9,2.1,.9,6.5));
SEGMENTS(q_lower, ELLIPSE(.9,2.1,4.1,5.2),L(4.1,2.1,4.1,6.5));
SEGMENTS(r_lower, L(1.0,2.6,1.0,6.3),B(1.0,3.8,1.7,2.3,2.8,2.2,4.0,2.9));
SEGMENTS(s_lower, B(4.0,2.9,1.0,1.5,-.2,4.0,2.5,4.3),B(2.5,4.3,5.6,4.6,4.1,7.4,.9,5.9));
SEGMENTS(t_lower, L(2.0,1.2,2.0,5.2),Q(2.0,5.2,2.0,6.8,3.8,6.1),L(.7,2.9,3.8,2.9));
SEGMENTS(u_lower, L(.9,2.6,.9,4.9),B(.9,4.9,.9,7.0,4.1,6.4,4.1,4.9),L(4.1,2.6,4.1,6.3));
SEGMENTS(v_lower, L(.8,2.6,2.5,6.3),L(2.5,6.3,4.2,2.6));
SEGMENTS(w_lower, L(.6,2.6,1.3,6.3),L(1.3,6.3,2.5,4.3),L(2.5,4.3,3.7,6.3),L(3.7,6.3,4.4,2.6));
SEGMENTS(x_lower, L(.9,2.6,4.1,6.3),L(.9,6.3,4.1,2.6));
SEGMENTS(y_lower, L(.9,2.4,2.5,5.3),L(4.1,2.4,1.8,6.4));
SEGMENTS(z_lower, L(.9,2.7,4.1,2.7),L(4.1,2.7,.9,6.3),L(.9,6.3,4.1,6.3));
SEGMENTS(degree, ELLIPSE(1.6,.7,3.4,2.6));
SEGMENTS(squared, B(1.0,1.4,1.0,-.1,3.9,.1,3.9,1.4),Q(3.9,1.4,3.9,2.1,1.0,3.3),L(1.0,3.3,4.0,3.3));
SEGMENTS(cubed, B(1.0,.8,4.9,-.3,4.4,2.0,2.3,2.0),B(2.3,2.0,4.8,1.8,4.8,4.2,1.0,3.3));
SEGMENTS(mu, L(1.0,2.4,1.0,6.4),B(1.0,2.4,1.0,7.0,3.9,5.9,3.9,4.6),L(3.9,2.4,3.9,6.0));
#define MASTER(c,n,p,w) {c,{NULL,NULL,NULL,NULL,NULL,NULL,NULL},n,sizeof n/sizeof n[0],p,sizeof p/sizeof p[0],w,6.0,0.0,6.5}
#define G(c,n,k) MASTER(c,n,strokes##k,.72)
static const sf57_glyph sensor_glyphs[] = {
    G('/',slash,1),G('\\',backslash,1),G('+',plus,2),G('-',minus,1),G(0x2212,minus,1),
    G(0x00d7,times,2),G('.',dot,1),G(':',colon,2),G(',',comma,1),G('=',equal,2),G('?',question,3),
    MASTER('0',zero,loop4,.72),G('1',one,3),G('2',two,3),G('3',three,2),G('4',four,3),
    G('5',five,3),G('6',six,3),G('7',seven,2),G('9',nine,3),
    G('A',a_upper,3),G('B',b_upper,3),MASTER('D',d_upper,d_paths,.72),G('E',e_upper,4),
    G('F',f_upper,3),G('G',g_upper,3),G('H',h_upper,3),G('I',i_upper,3),G('J',j_upper,3),
    G('K',k_upper,3),G('L',l_upper,2),G('M',m_upper,4),G('N',n_upper,3),
    MASTER('P',p_upper,p_paths,.72),MASTER('Q',q_upper,loop_tail,.72),MASTER('R',r_upper,r_paths,.72),
    G('T',t_upper,2),G('U',u_upper,3),G('V',v_upper,2),G('W',w_upper,4),G('X',x_upper,2),G('Y',y_upper,3),G('Z',z_upper,3),
    MASTER('a',a_lower,loop_tail,.72),MASTER('b',b_lower,loop_tail,.72),G('c',c_lower,1),
    MASTER('d',d_lower,loop_tail,.72),G('e',e_lower,3),G('f',f_lower,2),MASTER('g',g_lower,loop_tail,.72),
    G('h',h_lower,3),G('i',i_lower,3),G('j',j_lower,3),G('k',k_lower,3),G('l',l_lower,2),
    G('m',m_lower,5),G('n',n_lower,3),MASTER('o',o_lower,loop4,.72),
    MASTER('p',p_lower,loop_tail,.72),MASTER('q',q_lower,loop_tail,.72),G('r',r_lower,2),G('s',s_lower,2),
    G('t',t_lower,3),G('u',u_lower,3),G('v',v_lower,2),G('w',w_lower,4),G('x',x_lower,2),G('y',y_lower,2),G('z',z_lower,3),
    MASTER(0x00b0,degree,loop4,.60),G(0x00b2,squared,3),G(0x00b3,cubed,2),G(0x03bc,mu,3),G(0x00b5,mu,3)
};
#undef G
#undef MASTER
#undef SEGMENTS
#undef ELLIPSE
#undef B
#undef Q
#undef L
