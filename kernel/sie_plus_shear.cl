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
    float2 x; // lens position
    mat22 m;  // rotation matrix for position angle
    mat22 w;  // inverse rotation matrix
    mat22 g;  // shear matrix
    
    // auxiliary
    float q2;
    float e;
    float d;
};

static float2 sie_plus_shear(constant struct sie_plus_shear* data, float2 x)
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
    
    // reverse coordinate rotation, apply shear
    return mv(data->w, y) + mv22(data->g, x);
}

static void set_sie_plus_shear(global struct sie_plus_shear* data, float x1, float x2, float r, float q, float pa, float g1, float g2)
{
    float c = cos(pa*DEG2RAD);
    float s = sin(pa*DEG2RAD);
    
    // lens position
    data->x = (float2)(x1, x2);
    
    // rotation matrix
    data->m = (mat22)(c, s, -s, c);
    
    // inverse rotation matrix
    data->w = (mat22)(c, -s, s, c);
    
    // shear matrix
    data->g = (mat22)(g1, g2, g2, -g1);
    
    // auxiliary quantities
    data->q2 = q*q;
    data->e = sqrt(1 - q*q);
    data->d = r*sqrt(q)/sqrt(1 - q*q);
}
