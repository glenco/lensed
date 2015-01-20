// uniform shear

OBJECT(shear) = LENS;

PARAMS(shear) = {
    { "g1" },
    { "g2" }
};

struct shear
{
    float g1;
    float g2;
};

static float2 shear(constant struct shear* shear, float2 x)
{
    return (float2)( shear->g2*x.y - shear->g1*x.x
                   , shear->g1*x.y + shear->g2*x.x);    
}

static void set_shear(global struct shear* shear, float g1, float g2)
{
    shear->g1 = g1;
    shear->g2 = g2;
}
