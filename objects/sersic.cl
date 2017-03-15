type = SOURCE;

params
{
    { "x",   POSITION_X },
    { "y",   POSITION_Y },
    { "r",   RADIUS     },
    { "mag", MAGNITUDE  },
    { "n",   PARAMETER, POS_BOUND },
    { "q",   AXIS_RATIO },
    { "pa",  POS_ANGLE  }
};

data
{
    float2 x;   // source position
    mat22 t;    // coordinate transformation matrix
    float log0; // profile constants
    float log1;
    float m;
};

static float brightness(local data* this, float2 x)
{
    float2 y = mv22(this->t, x - this->x);
    return exp(this->log0 - exp(this->log1 + this->m*log(dot(y, y))));
}

static void set(local data* this, float x, float y, float r, float mag, float n, float q, float a)
{
    // for approximations see MacArthur, Courteau, Holtzman (2003)
    float b = n > 0.36f ? 2.0f*n - 1.0f/3 + 4.0f/(405*n) + 46.0f/(25515*(n*n)) + 131.0f/(1148175*(n*n*n)) - 2194697.0f/(30690717750*(n*n*n*n))
                        : 0.01945f - 0.8902f*n + 10.95f*(n*n) - 19.67f*(n*n*n) + 13.43f*(n*n*n*n);
    
    float c = cos(a*DEG2RAD);
    float s = sin(a*DEG2RAD);
    
    // source position
    this->x = (float2)(x, y);
    
    // transformation matrix: rotate and scale
    this->t = (mat22)(q*c, q*s, -s, c)/sqrt(q);
    
    this->log0 = -0.4f*mag*LOG_10 + 2*n*log(b) - LOG_PI - 2*log(r) - log(tgamma(2*n+1));
    this->log1 = log(b) - log(r)/n;
    this->m = 0.5f/n;
}
