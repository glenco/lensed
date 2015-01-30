// singular isothermal sphere plus shear

OBJECT(sis_plus_shear) = LENS;

PARAMS(sis_plus_shear) = {
    { "x" },
    { "y" },
    { "r" },
    { "g1" },
    { "g2" }
};

struct sis_plus_shear
{
    float2 x;
    float g1;
    float g2;
    float d;
};

static float2 sis_plus_shear(constant struct sis_plus_shear* sis_plus_shear, float2 x)
{
    float2 y,dx;
    float r;
    
    dx = x - sis_plus_shear->x;

    r = 1./sqrt(dx.x*dx.x + dx.y*dx.y);

    y = sis_plus_shear->d*(float2)(dx.x*r, dx.y*r);
    y = y + (float2)(sis_plus_shear->g1*dx.x + sis_plus_shear->g2*dx.y
      	            ,sis_plus_shear->g2*dx.x - sis_plus_shear->g1*dx.y)  ; 
    
    
    
    return y;
}

static void set_sis_plus_shear(global struct sis_plus_shear* sis_plus_shear, float x1, float x2
, float r, float g1, float g2)
{
    // lens position
    sis_plus_shear->x = (float2)(x1, x2);
    // Einstein radius
    sis_plus_shear->d = r;
    sis_plus_shear->g1 = g1;
    sis_plus_shear->g2 = g2;
}
