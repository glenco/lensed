OBJECT(sersic) = SOURCE;

struct sersic
{
    float x;
    float y;
    float q;
    float cos;
    float sin;
    float log0;
    float log1;
    float m;
};

PARAMS(sersic) = {
    { "x" },
    { "y" },
    { "r" },
    { "mag" },
    { "n" },
    { "f" },
    { "pa", true },
};

static float sersic(constant struct sersic* src, float2 y)
{
    float u = (y.x - src->x)*src->cos - (y.y - src->y)*src->sin;
    float v = (y.y - src->y)*src->cos + (y.x - src->x)*src->sin;
    
    float r2 = u*u + v*v/src->q/src->q;
    
    return exp(src->log0 - exp(src->log1 + src->m*log(r2)));
}

static void set_sersic(global struct sersic* src, constant float* P)
{
    enum { X, Y, R_EFF, MAG, N, Q, PA };
    
    float b = 1.9992f*P[N] - 0.3271f; // approximation valid for 0.5 < n < 8
    
    src->x = P[X];
    src->y = P[Y];
    src->q = P[Q];
    
    src->cos = cos(P[PA]);
    src->sin = sin(P[PA]);
    src->log0 = 0.4f*P[MAG]*LOG_10 + 2*P[N]*log(b) - LOG_PI - log(P[Q]) - 2*log(P[R_EFF]) - log(tgamma(2*P[N]+1));
    src->log1 = log(b) - log(P[R_EFF])/P[N];
    src->m = 0.5f/P[N];
}
