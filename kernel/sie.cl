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
    float2 x; // lens position
    mat22 m;  // rotation matrix for position angle
    mat22 w;  // inverse rotation matrix
    
    // auxiliary
    float q2;
    float e;
    float d;
};

static float2 sie(constant struct sie* data, float2 x)
{
    float2 y;
    float r;
    
    // move to central coordinates
    x -= data->x;
    
    // rotate coordinates by position angle
    y = mv22(data->m, x);
    
    // SIE deflection
    r = data->e/sqrt(data->q2*y.x*y.x + y.y*y.y);
    y = data->d*(float2)(atan(y.x*r), atanh(y.y*r));
    
    // reverse coordinate rotation
    return mv22(data->w, y);
}

static void set_sie(global struct sie* sie, float x1, float x2, float r, float q, float pa)
{
    float c = cos(pa*DEG2RAD);
    float s = sin(pa*DEG2RAD);
    
    // lens position
    sie->x = (float2)(x1, x2);
    
    // rotation matrix
    sie->m = (mat22)(c, s, -s, c);
    
    // inverse rotation matrix
    sie->w = (mat22)(c, -s, s, c);
    
    // auxiliary quantities
    sie->q2 = q*q;
    sie->e = sqrt(1 - q*q);
    sie->d = r*sqrt(q)/sqrt(1 - q*q);
}
