OBJECT(sie) = LENS;

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

PARAMS(sie) = {
    { "x" },
    { "y" },
    { "r" },
    { "f" },
    { "pa", true }
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

static void set_sie(global struct sie* sie, constant float* P)
{
    enum { X, Y, R_E, F, PA };
    
    sie->x = P[X];
    sie->y = P[Y];
    sie->c = cos(PI_HALF - P[PA]);
    sie->s = sin(PI_HALF - P[PA]);
    sie->f2 = P[F]*P[F];
    sie->e = sqrt(1.0f - sie->f2);
    sie->d = P[R_E]*sqrt(P[F])/sie->e;
}
