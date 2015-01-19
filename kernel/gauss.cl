OBJECT(gauss) = SOURCE;

PARAMS(gauss) = {
    { "x" },
    { "y" },
    { "sigma" },
    { "mag" },
    { "q" },
    { "pa", true },
};

struct gauss
{
    float2 x;
    float4 t;
    float s2;
    float norm;
};

static float gauss(constant struct gauss* src, float2 x)
{
    float4 t = src->t;
    float2 y = x - src->x;
    
    y = (float2)(dot(t.lo, y), dot(t.hi, y));

    float d2 = dot(y, y);
	
    return src->norm*exp(-0.5f*d2/src->s2);
}

static void set_gauss(global struct gauss* src, float x1, float x2, float sigma, float mag, float q, float pa)
{
    float c = cos(pa*DEG2RAD);
    float s = sin(pa*DEG2RAD);
    
    // source position
    src->x = (float2)(x1, x2);
    
    // transformation matrix: rotate and scale
    src->t = (float4)(q*c, q*s, -s, c);
    
    src->s2 = sigma*sigma;
    src->norm = exp(-0.4f*mag*LOG_10)*0.5f/PI/src->s2/q;
}
