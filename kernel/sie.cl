// singular isothermal ellipsoid
// follows Schneider, Kochanek, Wambsganss (2006)

OBJECT(sie) = LENS;

PARAMS(sie) = {
    { "x" },
    { "y" },
    { "r" },
    { "q" },
    { "pa", true }
};

struct sie
{
    float2 x;
    float4 m;
    float4 w;
    
    float q2;
    float e;
    float d;
};

static float2 sie(constant struct sie* sie, float2 x)
{
    float2 y;
    float r;
    float4 m = sie->m;
    float4 w = sie->w;
    
    y = (float2)(dot(m.lo, x - sie->x), dot(m.hi, x - sie->x));
    
    r = sie->e/sqrt(sie->q2*y.x*y.x + y.y*y.y);
    
    y = sie->d*(float2)(atan(y.x*r), atanh(y.y*r));
    
    return (float2)(dot(w.lo, y), dot(w.hi, y));
}

static void set_sie(global struct sie* sie, float x1, float x2, float r, float q, float pa)
{
    float c = cos(pa*DEG2RAD);
    float s = sin(pa*DEG2RAD);
    
    // lens position
    sie->x = (float2)(x1, x2);
    
    // rotation matrix
    sie->m = (float4)(c, s, -s, c);
    
    // inverse rotation matrix
    sie->w = (float4)(c, -s, s, c);
    
    sie->q2 = q*q;
    sie->e = sqrt(1 - q*q);
    sie->d = r*sqrt(q)/sqrt(1 - q*q);
}
