OBJECT(sersic) = SOURCE;

PARAMS(sersic) = {
    { "x" },
    { "y" },
    { "r" },
    { "mag" },
    { "n" },
    { "q" },
    { "pa", true },
};

struct sersic
{
    float2 x;
    float4 t;
    float log0;
    float log1;
    float m;
};

static float sersic(constant struct sersic* src, float2 x)
{
    float4 t = src->t;
    float2 y = x - src->x;
    
    y = (float2)(dot(t.lo, y), dot(t.hi, y));
    
    return exp(src->log0 - exp(src->log1 + src->m*log(dot(y, y))));
}

static void set_sersic(global struct sersic* src, float x1, float x2, float r, float mag, float n, float q, float pa)
{
    float b = 1.9992f*n - 0.3271f; // approximation valid for 0.5 < n < 8
    
    float c = cos(pa*DEG2RAD);
    float s = sin(pa*DEG2RAD);
    
    // source position
    src->x = (float2)(x1, x2);
    
    // transformation matrix: rotate and scale
    src->t = (float4)(q*c, q*s, -s, c);
    
    src->log0 = -0.4f*mag*LOG_10 + 2*n*log(b) - LOG_PI - 2*log(r) - log(tgamma(2*n+1));
    src->log1 = log(b) - (0.5f*log(q) + log(r))/n;
    src->m = 0.5f/n;
}
