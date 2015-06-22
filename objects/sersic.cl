type = SOURCE;

params
{
    { "x",      POSITION_X  },
    { "y",      POSITION_Y  },
    { "r",      RADIUS      },
    { "mag",    MAGNITUDE   },
    { "n",      PARAMETER   },
    { "q",      AXIS_RATIO  },
    { "pa",     POS_ANGLE   }
};

data
{
    float2 x;   // source position
    mat22 t;    // coordinate transformation matrix
    float log0; // profile constants
    float log1;
    float m;
};

static float brightness(constant data* this, float2 x)
{
    float2 y = mv22(this->t, x - this->x);
    return exp(this->log0 - exp(this->log1 + this->m*log(dot(y, y))));
}

static void set(global data* this, float x, float y, float r, float mag, float n, float q, float a)
{
    float b = 1.9992f*n - 0.3271f; // approximation valid for 0.5 < n < 8
    
    float c = cos(a*DEG2RAD);
    float s = sin(a*DEG2RAD);
    
    // source position
    this->x = (float2)(x, y);
    
    // transformation matrix: rotate and scale
    this->t = (mat22)(q*c, q*s, -s, c);
    
    this->log0 = -0.4f*mag*LOG_10 + 2*n*log(b) - LOG_PI - 2*log(r) - log(tgamma(2*n+1));
    this->log1 = log(b) - (0.5f*log(q) + log(r))/n;
    this->m = 0.5f/n;
}
