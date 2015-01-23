// non-singular isothermal ellipsoid
// follows Schneider, Kochanek, Wambsganss (2006)

OBJECT(nsie) = LENS;

PARAMS(nsie) = {
    { "x" },
    { "y" },
    { "r" },
    { "rc" },
    { "q" },
    { "pa", true },
};

struct nsie
{
    float2 x;
    float4 m;
    float4 w;
    
    float q2;
    float e;
    float d;
    float rc;
};

static float2 nsie(constant struct nsie* nsie, float2 x)
{
    float2 y;
    float rx;
    float ry;
    float4 m = nsie->m;
    float4 w = nsie->w;
    
    y = (float2)(dot(m.lo, x - nsie->x), dot(m.hi, x - nsie->x));
    
    rx = nsie->e/(nsie->rc+sqrt(nsie->q2*y.x*y.x + y.y*y.y));
    ry = nsie->e/(nsie->rc*nsie->q2+sqrt(nsie->q2*y.x*y.x + y.y*y.y));
    
    y = nsie->d*(float2)(atan(y.x*rx), atanh(y.y*ry));
    
    return (float2)(dot(w.lo, y), dot(w.hi, y));
}

static void set_nsie(global struct nsie* nsie, float x1, float x2, float r, float rc, float q, float pa)
{
    float c = cos(pa*DEG2RAD);
    float s = sin(pa*DEG2RAD);
    
    // lens position
    nsie->x = (float2)(x1, x2);
    
    // rotation matrix
    nsie->m = (float4)(c, s, -s, c);
    
    // inverse rotation matrix
    nsie->w = (float4)(c, -s, s, c);
    
    nsie->q2 = q*q;
    nsie->e = sqrt(1 - q*q);
    nsie->d = r*sqrt(q)/sqrt(1 - q*q);

    nsie->rc = rc;
}
