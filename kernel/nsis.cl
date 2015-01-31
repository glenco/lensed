// non-singular isothermal sphere
// follows Schneider, Kochanek, Wambsganss (2006)

OBJECT(nsis) = LENS;

PARAMS(nsis) = {
    { "x" },
    { "y" },
    { "r" },
    { "rc" }
};

struct nsis
{
    float2 x; // lens position
    float r;  // Einstein radius
    float rc; // core radius
};

static float2 nsis(constant struct nsis* data, float2 x)
{
    // move to central coordinates
    x -= data->x;
    
    // NSIS deflection
    return data->r/(data->rc + length(x))*x;
}

static void set_nsis(global struct nsis* nsis, float x1, float x2, float r, float rc)
{
    // lens position
    nsis->x = (float2)(x1, x2);
    
    // Einstein radius
    nsis->r = r;
    
    // core radius
    nsis->rc = rc;
}
