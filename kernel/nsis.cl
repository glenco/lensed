// non-singular isothermal sphere

OBJECT(nsis) = LENS;

PARAMS(nsis) = {
    { "x" },
    { "y" },
    { "r" },
    { "rc" },
};

struct nsis
{
    float2 x;
    float d;
    float rc;
};

static float2 nsis(constant struct nsis* nsis, float2 x)
{
    float2 y;
    float r;
    
    y = x - nsis->x;

    r = 1./(nsis->rc+sqrt(y.x*y.x + y.y*y.y));

    y = nsis->d*(float2)(y.x*r, y.y*r);
    
    return y;
}

static void set_nsis(global struct nsis* nsis, float x1, float x2, float r, float rc)
{
    // lens position
    nsis->x = (float2)(x1, x2);
    // Einstein radius
    nsis->d = r;
    // core radius
    nsis->rc = rc;
}
