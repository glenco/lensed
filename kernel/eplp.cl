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
    float4 m;
    float4 w;
    
    float q2;
    float e;
    float d;
    float alpha;
};

static float2 eplp(constant struct eplp* eplp, float2 x)
{
    float2 dy,y;
    float r;
 
    dy = (float2)(dot(eplp->m.lo, x - eplp->x), dot(eplp->m.hi, x - eplp->x));
    
    r = sqrt(dy.x*dy.x + dy.y*dy.y);

    float g = sqrt(dy.x*dy.x + eplp->q2*dy.y*dy.y)/r;
    float a_iso = eplp->d*pow(r*g/eplp->d , eplp->alpha);

    y.x = a_iso * g * dy.x * ( 1 + eplp->e*dy.y*dy.y/r/r/g/g ) / r;
    y.y = a_iso * g * dy.y * ( 1 - eplp->e*dy.x*dy.x/r/r/g/g ) / r;
    
    return (float2)(dot(eplp->w.lo, y), dot(eplp->w.hi, y));
}

static void set_eplp(global struct eplp* eplp, float x1, float x2, float re, float alpha, float q, float pa)
{
    float c = cos(pa*DEG2RAD);
    float s = sin(pa*DEG2RAD);
    
    // lens position
    eplp->x = (float2)(x1, x2);
    
    // rotation matrix
    eplp->m = (float4)(c, s, -s, c);
    
    // inverse rotation matrix
    eplp->w = (float4)(c, -s, s, c);
    
    eplp->q2 = q*q;
    eplp->e = 1 - q*q;
    eplp->d = re*sqrt(q)/sqrt(1 - q*q);
    eplp->alpha = alpha;
    
}
