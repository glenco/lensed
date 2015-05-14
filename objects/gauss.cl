type = SOURCE;

params
{
    { "x" },
    { "y" },
    { "sigma" },
    { "mag" },
    { "q" },
    { "pa", true },
};

data
{
    float2 x;   // source position
    mat22 t;    // coordinate transformation matrix
    float s2;   // variance
    float norm; // normalisation
};

static float brightness(constant data* this, float2 x)
{
    // Gaussian profile for centered and rotated coordinate system
    float2 y = mv22(this->t, x - this->x);
    return this->norm*exp(-0.5f*dot(y, y)/this->s2);
}

static void set(global data* this, float x, float y, float sigma, float mag, float q, float pa)
{
    float c = cos(pa*DEG2RAD);
    float s = sin(pa*DEG2RAD);
    
    // source position
    this->x = (float2)(x, y);
    
    // transformation matrix: rotate and scale
    this->t = (mat22)(q*c, q*s, -s, c);
    
    this->s2 = sigma*sigma;
    this->norm = exp(-0.4f*mag*LOG_10)*0.5f/PI/this->s2/q;
}
