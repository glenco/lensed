type = SOURCE;

params
{
    { "x" },
    { "y" },
    { "rs" },
    { "mag" },
    { "q" },
    { "pa", true },
};

data
{
    float2 x;   // source position
    mat22 t;    // coordinate transformation matrix
    float rs;   // scale length
    float norm; // normalisation
};

static float brightness(constant data* this, float2 x)
{
    // exponential profile for centered and rotated coordinate system
    return this->norm*exp(-length(mv22(this->t, x - this->x))/this->rs);
}

static void set(global data* this, float x, float y, float rs, float mag, float q, float pa)
{
    float c = cos(pa*DEG2RAD);
    float s = sin(pa*DEG2RAD);
    
    // source position
    this->x = (float2)(x, y);
    
    // transformation matrix: rotate and scale
    this->t = (mat22)(q*c, q*s, -s, c);
    
    // scale length
    this->rs = rs;
    
    // normalisation to total luminosity
    this->norm = exp(-0.4f*mag*LOG_10)*0.5f/PI/rs/rs/q;
}
