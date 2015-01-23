// singular isothermal sphere

OBJECT(sis) = LENS;

PARAMS(sis) = {
    { "x" },
    { "y" },
    { "r" },
};

struct sis
{
    float2 x;
    float d;
};

static float2 sis(constant struct sis* sis, float2 x)
{
    float2 y;
    float r;
    
    y = x - sis->x;

    r = 1./sqrt(y.x*y.x + y.y*y.y);

    y = sis->d*(float2)(y.x*r, y.y*r);
    
    return y;
}

static void set_sis(global struct sis* sis, float x1, float x2, float r)
{
    // lens position
    sis->x = (float2)(x1, x2);
    // Einstein radius
    sis->d = r;
}
