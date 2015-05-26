// point mass lens

type = LENS;

params
{
    { "x" },
    { "y" },
    { "r" }
};

data
{
    float2 x; // lens position
    float r2; // Einstein radius squared
};

static float2 deflection(constant data* this, float2 x)
{
    // point mass deflection
    x -= this->x;
    return this->r2/dot(x, x)*x;
}

static void set(global data* this, float x, float y, float r)
{
    // lens position
    this->x = (float2)(x, y);
    
    // Einstein radius squared
    this->r2 = r*r;
}
