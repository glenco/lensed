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
    float2 x;   // source position
    mat22 t;    // coordinate transformation matrix
    float s2;   // variance
    float norm; // normalisation
};

static float gauss(constant struct gauss* data, float2 x)
{
    // Gaussian profile for centered and rotated coordinate system
    float2 y = mv22(data->t, x - data->x);
    return data->norm*exp(-0.5f*dot(y, y)/data->s2);
}

static void set_gauss(global struct gauss* data, float x1, float x2, float sigma, float mag, float q, float pa)
{
    float c = cos(pa*DEG2RAD);
    float s = sin(pa*DEG2RAD);
    
    // source position
    data->x = (float2)(x1, x2);
    
    // transformation matrix: rotate and scale
    data->t = (mat22)(q*c, q*s, -s, c);
    
    data->s2 = sigma*sigma;
    data->norm = exp(-0.4f*mag*LOG_10)*0.5f/PI/data->s2/q;
}
