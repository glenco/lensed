OBJECT(sie) = LENS;

PARAMS(sie) = {
    { "x" },
    { "y" },
    { "r" },
    { "f" },
    { "pa", true }
};

struct sie
{
    float x;
    float y;
    float c;
    float s;
    
    float f2;
    float e;
    float d;
};

static float2 sie(constant struct sie* sie, float2 x)
{
    float u, v, r;
    
    u = (x.x - sie->x)*sie->c - (x.y - sie->y)*sie->s;
    v = (x.y - sie->y)*sie->c + (x.x - sie->x)*sie->s;
    
    r = sie->e/sqrt(sie->f2*u*u + v*v);
    u = sie->d*atan(u*r);
    v = sie->d*atanh(v*r);
    
    return (float2)( u*sie->c + v*sie->s, v*sie->c - u*sie->s );
}

static void set_sie(global struct sie* sie, float x, float y, float r, float f, float pa)
{
    sie->x = x;
    sie->y = y;
    sie->c = cos(PI_HALF - pa);
    sie->s = sin(PI_HALF - pa);
    sie->f2 = f*f;
    sie->e = sqrt(1.0f - sie->f2);
    sie->d = r*sqrt(f)/sie->e;
}
