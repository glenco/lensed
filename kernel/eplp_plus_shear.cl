// An elliptical power law potential model plus shear
// \Phi(r,theta) = re ( G(r,\theta)/re )^alpha  G(r,\theta) = sqrt( x^2 + q^2 y^2) 
// see Leier & Metcalf 2015
OBJECT(eplp_plus_shear) = LENS;

PARAMS(eplp_plus_shear) = {
    { "x" },
    { "y" },
    { "re" },
    { "alpha" },
    { "q" },
    { "pa", true },
    { "g1" },
    { "g2" }

};

struct eplp_plus_shear
{
    float2 x;
    float4 m;
    float4 w;
    
    float q2;
    float e;
    float re;
    float alpha;

    float4 g;
};

static float2 eplp_plus_shear(constant struct eplp_plus_shear* eplp_plus_shear, float2 x)
{
    float2 y;
    float r;

    float2 dx = x - eplp_plus_shear->x;
    float2 dy = (float2)(dot(eplp_plus_shear->m.lo, dx), dot(eplp_plus_shear->m.hi, dx));
    
    r = length(dy);

    float g = sqrt(dy.x*dy.x + eplp_plus_shear->q2*dy.y*dy.y)/r;
    float a_iso = eplp_plus_shear->re*powr(r*g/eplp_plus_shear->re , eplp_plus_shear->alpha);

    y.x = a_iso * g * dy.x * ( 1 + eplp_plus_shear->e*pown(dy.y/r/g,2) ) / r;
    y.y = a_iso * g * dy.y * ( 1 - eplp_plus_shear->e*pown(dy.x/r/g,2) ) / r;

    y = (float2)(dot(eplp_plus_shear->w.lo, y), dot(eplp_plus_shear->w.hi, y));
    
    return y + (float2)(dot(eplp_plus_shear->g.lo, dx), dot(eplp_plus_shear->g.hi, dx)); 
}

static void set_eplp_plus_shear(global struct eplp_plus_shear* eplp_plus_shear, float x1, float x2, float re, float alpha, float q, float pa, float g1, float g2)
{
    float c = cos(pa*DEG2RAD);
    float s = sin(pa*DEG2RAD);
    
    // lens position
    eplp_plus_shear->x = (float2)(x1, x2);
    
    // rotation matrix
    eplp_plus_shear->m = (float4)(c, s, -s, c);
    
    // inverse rotation matrix
    eplp_plus_shear->w = (float4)(c, -s, s, c);
    
    eplp_plus_shear->q2 = q*q;
    eplp_plus_shear->e = 1 - q*q;
    eplp_plus_shear->re = re;
    eplp_plus_shear->alpha = alpha;

    eplp_plus_shear->g = (float4)(g1,g2,g2,-g1);
     
}
