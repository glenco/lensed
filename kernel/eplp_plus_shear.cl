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
    mat22 m;
    mat22 w;
    
    float q2;
    float e;
    float re;
    float alpha;

    mat22 g;
};

static float2 eplp_plus_shear(constant struct eplp_plus_shear* eplp_plus_shear, float2 x)
{
    float2 y;

    float2 dx = x - eplp_plus_shear->x;
    float2 dy = mv22(eplp_plus_shear->m,dx);
    
    float eta = sqrt(dy.x*dy.x + eplp_plus_shear->q2*dy.y*dy.y);
    
    float a_iso = eplp_plus_shear->re*powr(eta/eplp_plus_shear->re , eplp_plus_shear->alpha) / eta;

    y.x = a_iso * dy.x ;
    y.y = a_iso * eplp_plus_shear->q2 * dy.y;

    y = mv22(eplp_plus_shear->w,y);
    
    return y + mv22(eplp_plus_shear->g, dx);
}

static void set_eplp_plus_shear(global struct eplp_plus_shear* eplp_plus_shear, float x1, float x2, float re, float alpha, float q, float pa, float g1, float g2)
{
    float c = cos(pa*DEG2RAD);
    float s = sin(pa*DEG2RAD);
    
    // lens position
    eplp_plus_shear->x = (float2)(x1, x2);
    
    // rotation matrix
    eplp_plus_shear->m = (mat22)(c, s, -s, c);
    
    // inverse rotation matrix
    eplp_plus_shear->w = (mat22)(c, -s, s, c);
    
    eplp_plus_shear->q2 = q*q;
    eplp_plus_shear->e = 1 - q*q;
    eplp_plus_shear->re = re;
    eplp_plus_shear->alpha = alpha;

    eplp_plus_shear->g = (mat22)(g1,g2,g2,-g1);
}
