// An elliptical power law potential model  \Phi(r,theta) = re ( G(r,\theta)/re )^alpha  G(r,\theta) = sqrt( x^2 + q^2 y^2) 
// see Leier & Metcalf 2015
OBJECT(eplp) = LENS;

PARAMS(eplp) = {
    { "x" },
    { "y" },
    { "re" },
    { "alpha" },
    { "q" },
    { "pa", true }
};

struct eplp
{
    float2 x;
    mat22 m;
    mat22 w;
    
    float q2;
    float e;
    float re;
    float alpha;
};

static float2 eplp(constant struct eplp* eplp, float2 x)
{
    float2 dy,y;
    float r;
 
    dy = mv22(eplp->m, x - eplp->x);
    
    r = length(dy);

    float eta = sqrt(dy.x*dy.x + eplp->q2*dy.y*dy.y);
    
    float a_iso = eplp->re*powr(eta/eplp->re , eplp->alpha) / eta;

    y.x = a_iso * dy.x ;
    y.y = a_iso * eplp->q2 * dy.y;
    
    return mv22(eplp->w, y);
}

static void set_eplp(global struct eplp* eplp, float x1, float x2, float re, float alpha, float q, float pa)
{
    float c = cos(pa*DEG2RAD);
    float s = sin(pa*DEG2RAD);
    
    // lens position
    eplp->x = (float2)(x1, x2);
    
    // rotation matrix
    eplp->m = (mat22)(c, s, -s, c);
    
    // inverse rotation matrix
    eplp->w = (mat22)(c, -s, s, c);
    
    eplp->q2 = q*q;
    eplp->e = 1 - q*q;
    eplp->re = re;
    eplp->alpha = alpha;
    
}
