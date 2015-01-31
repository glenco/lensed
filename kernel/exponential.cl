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
    float2 x;   // source position
    mat22 t;    // coordinate transformation matrix
    float rs;   // scale length
    float norm; // normalisation
};

static float exponential(constant struct exponential* data, float2 x)
{
    // exponential profile for centered and rotated coordinate system
    return data->norm*exp(-length(mv22(data->t, x - data->x))/data->rs);
}

static void set_exponential(global struct exponential* data, float x1, float x2, float rs, float mag, float q, float pa)
{
    float c = cos(pa*DEG2RAD);
    float s = sin(pa*DEG2RAD);
    
    // source position
    data->x = (float2)(x1, x2);
    
    // transformation matrix: rotate and scale
    data->t = (mat22)(q*c, q*s, -s, c);
    
    // scale length
    data->rs = rs;
    
    // normalisation to total luminosity
    data->norm = exp(-0.4f*mag*LOG_10)*0.5f/PI/rs/rs/q;
}
