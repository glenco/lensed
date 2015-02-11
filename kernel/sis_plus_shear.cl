// singular isothermal sphere plus shear

OBJECT(sis_plus_shear) = LENS;

PARAMS(sis_plus_shear) = {
    { "x" },
    { "y" },
    { "r" },
    { "g1" },
    { "g2" }
};

struct sis_plus_shear
{
    float2 x; // lens position
    mat22 g;  // shear matrix
    float r;  // Einstein radius
};

static float2 sis_plus_shear(constant struct sis_plus_shear* data, float2 x)
{
    // move to central coordinates
    x -= data->x;
    
    // SIS deflection plus external shear
    return data->r*normalize(x) + mv22(data->g, x);
}

static void set_sis_plus_shear(global struct sis_plus_shear* data, float x1, float x2, float r, float g1, float g2)
{
    // lens position
    data->x = (float2)(x1, x2);
    
    // Einstein radius
    data->r = r;
    
    // shear matrix
    data->g = (mat22)(g1, g2, g2, -g1);
}
