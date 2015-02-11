// point mass lens

OBJECT(point_mass) = LENS;

PARAMS(point_mass) = {
    { "x" },
    { "y" },
    { "r" }
};

struct point_mass
{
    float2 x; // lens position
    float r2; // Einstein radius squared
};

static float2 point_mass(constant struct point_mass* data, float2 x)
{
    // point mass deflection
    return data->r2*normalize(x - data->x);
}

static void set_point_mass(global struct point_mass* data, float x1, float x2, float r)
{
    // lens position
    data->x = (float2)(x1, x2);
    
    // Einstein radius squared
    data->r2 = r*r;
}
