// non-singular isothermal ellipsoid
// follows Schneider, Kochanek, Wambsganss (2006)

OBJECT(nsie) = LENS;

PARAMS(nsie) = {
    { "x" },
    { "y" },
    { "r" },
    { "rc" },
    { "q" },
    { "pa", true }
};

struct nsie
{
    float2 x; // lens position
    mat22 m;  // rotation matrix for position angle
    mat22 w;  // inverse rotation matrix
    float rc; // core radius
    
    // auxiliary
    float q2;
    float e;
    float d;
};

static float2 nsie(constant struct nsie* data, float2 x)
{
    float2 y;
    float r;
    
    // move to central coordinates
    x -= data->x;
    
    // rotate coordinates by position angle
    y = mv22(data->m, x);
    
    // NSIE deflection
    r = sqrt(data->q2*y.x*y.x + y.y*y.y);
    y = data->d*(float2)(atan(y.x*data->e/(data->rc + r)), atanh(y.y*data->e/(data->rc*data->q2 + r)));
    
    // reverse coordinate rotation
    return mv22(data->w, y);
}

static void set_nsie(global struct nsie* data, float x1, float x2, float r, float rc, float q, float pa)
{
    float c = cos(pa*DEG2RAD);
    float s = sin(pa*DEG2RAD);
    
    // lens position
    data->x = (float2)(x1, x2);
    
    // rotation matrix
    data->m = (mat22)(c, s, -s, c);
    
    // inverse rotation matrix
    data->w = (mat22)(c, -s, s, c);
    
    // core radius
    data->rc = rc;
    
    // auxiliary quantities
    data->q2 = q*q;
    data->e = sqrt(1 - q*q);
    data->d = r*sqrt(q)/sqrt(1 - q*q);
}
