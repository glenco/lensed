OBJECT(sersic) = SOURCE;

PARAMS(sersic) = {
    { "x" },
    { "y" },
    { "r" },
    { "mag" },
    { "n" },
    { "f" },
    { "pa", true },
};

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

static float sersic(constant struct sersic* src, float2 y)
{
    float u = (y.x - src->x)*src->cos - (y.y - src->y)*src->sin;
    float v = (y.y - src->y)*src->cos + (y.x - src->x)*src->sin;
    
    float r2 = u*u + v*v/src->q/src->q;
    
    return exp(src->log0 - exp(src->log1 + src->m*log(r2)));
}

static void set_sersic(global struct sersic* src, float x, float y, float r, float mag, float n, float q, float pa)
{
    float b = 1.9992f*n - 0.3271f; // approximation valid for 0.5 < n < 8
    
    src->x = x;
    src->y = y;
    src->q = q;
    
    src->cos = cos(pa);
    src->sin = sin(pa);
    src->log0 = -0.4f*mag*LOG_10 + 2*n*log(b) - LOG_PI - log(q) - 2*log(r) - log(tgamma(2*n+1));
    src->log1 = log(b) - log(r)/n;
    src->m = 0.5f/n;
}
