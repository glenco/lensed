// A point mass lens

OBJECT(point_mass) = LENS;

PARAMS(point_mass) = {
    { "x" },
    { "y" },
    { "re" }  // Einstein radius in pixels 
};

struct point_mass
{
    float2 x;
    float re2;
};

static float2 point_mass(constant struct point_mass* point_mass, float2 x)
{
    float2 y;
    float r;
    
    y = x - point_mass->x;

    r = 1./(y.x*y.x + y.y*y.y);

    y = point_mass->re2*r*(float2)(y.x, y.y);
    
    return y;
}

static void set_point_mass(global struct point_mass* point_mass, float x1, float x2, float re)
{
    // lens position
    point_mass->x = (float2)(x1, x2);
    // Einstein radius
    point_mass->re2 = re*re;
}
