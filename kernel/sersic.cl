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
    float2 x;   // source position
    mat22 t;    // coordinate transformation matrix
    float log0; // profile constants
    float log1;
    float m;
};

static float sersic(constant struct sersic* data, float2 x)
{
    float2 y = mv22(data->t, x - data->x);
    return exp(data->log0 - exp(data->log1 + data->m*log(dot(y, y))));
}

static void set_sersic(global struct sersic* data, float x1, float x2, float r, float mag, float n, float q, float pa)
{
    float b = 1.9992f*n - 0.3271f; // approximation valid for 0.5 < n < 8
    
    float c = cos(pa*DEG2RAD);
    float s = sin(pa*DEG2RAD);
    
    // source position
    data->x = (float2)(x1, x2);
    
    // transformation matrix: rotate and scale
    data->t = (mat22)(q*c, q*s, -s, c);
    
    data->log0 = -0.4f*mag*LOG_10 + 2*n*log(b) - LOG_PI - 2*log(r) - log(tgamma(2*n+1));
    data->log1 = log(b) - (0.5f*log(q) + log(r))/n;
    data->m = 0.5f/n;
}
