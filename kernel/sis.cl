// singular isothermal sphere
// follows Schneider, Kochanek, Wambsganss (2006)

OBJECT(sis) = LENS;

PARAMS(sis) = {
    { "x" },
    { "y" },
    { "r" }
};

struct sis
{
    float2 x; // lens position
    float r;  // Einstein radius
};

static float2 sis(constant struct sis* data, float2 x)
{
    // SIS deflection
    return data->r*normalize(x - data->x);
}

static void set_sis(global struct sis* data, float x1, float x2, float r)
{
    // lens position
    data->x = (float2)(x1, x2);
    
    // Einstein radius
    data->r = r;
}
