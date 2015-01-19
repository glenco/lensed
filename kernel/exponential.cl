OBJECT(exponential) = SOURCE;

PARAMS(exponential) = {
    { "x" },
    { "y" },
    { "rs" },
    { "mag" },
    { "q" },
    { "pa", true },
};

struct exponential
{
    float2 x;
    float4 t;
    float rs;
    float norm;
};

static float exponential(constant struct exponential* src, float2 x)
{
    float4 t = src->t;
    float2 y = x - src->x;
    
    y = (float2)(dot(t.lo, y), dot(t.hi, y));

    float d = sqrt(dot(y, y));
	
    return src->norm*exp(-d/src->rs);
}

static void set_exponential(global struct exponential* src, float x1, float x2, float rs, float mag, float q, float pa)
{
    float c = cos(pa*DEG2RAD);
    float s = sin(pa*DEG2RAD);
    
    // source position
    src->x = (float2)(x1, x2);
    
    // transformation matrix: rotate and scale
    src->t = (float4)(q*c, q*s, -s, c);
    
    src->rs = rs;
    src->norm = exp(-0.4f*mag*LOG_10)*0.5f/PI/rs/rs/q;
}
