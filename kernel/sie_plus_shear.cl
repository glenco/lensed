// singular isothermal ellipsoid plus shear
// follows Schneider, Kochanek, Wambsganss (2006)

OBJECT(sie_plus_shear) = LENS;

PARAMS(sie_plus_shear) = {
    { "x" },
    { "y" },
    { "r" },
    { "q" },
    { "pa", true },
    { "g1" },
    { "g2" }

};

struct sie_plus_shear
{
    float2 x;
    float4 m;
    float4 w;
    
    float q2;
    float e;
    float d;

    float g1;
    float g2;
};

static float2 sie_plus_shear(constant struct sie_plus_shear* sie_plus_shear, float2 x)
{
    float2 y;
    float r;
    float4 m = sie_plus_shear->m;
    float4 w = sie_plus_shear->w;
    
    y = (float2)(dot(m.lo, x - sie_plus_shear->x), dot(m.hi, x - sie_plus_shear->x));
    
    r = sie_plus_shear->e/sqrt(sie_plus_shear->q2*y.x*y.x + y.y*y.y);
    
    y = sie_plus_shear->d*(float2)(atan(y.x*r), atanh(y.y*r));

    y = (float2)(dot(w.lo, y), dot(w.hi, y));
    
    float2 dx = x - sie_plus_shear->x;
    
    y = y + (float2)(sie_plus_shear->g1*dx.x + sie_plus_shear->g2*dx.y
      	            ,sie_plus_shear->g2*dx.x - sie_plus_shear->g1*dx.y)  ; 
  
    
    return y;
}

static void set_sie_plus_shear(global struct sie_plus_shear* sie_plus_shear, float x1, float x2, float r, float q, float pa, float g1, float g2)
{
    float c = cos(pa*DEG2RAD);
    float s = sin(pa*DEG2RAD);
    
    // lens position
    sie_plus_shear->x = (float2)(x1, x2);
    
    // rotation matrix
    sie_plus_shear->m = (float4)(c, s, -s, c);
    
    // inverse rotation matrix
    sie_plus_shear->w = (float4)(c, -s, s, c);
    
    sie_plus_shear->q2 = q*q;
    sie_plus_shear->e = sqrt(1 - q*q);
    sie_plus_shear->d = r*sqrt(q)/sqrt(1 - q*q);

    sie_plus_shear->g1 = g1;
    sie_plus_shear->g2 = g2;
}
