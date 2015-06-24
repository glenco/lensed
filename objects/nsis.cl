// non-singular isothermal sphere
// follows Schneider, Kochanek, Wambsganss (2006)

type = LENS;

params
{
    { "x",  POSITION_X  },
    { "y",  POSITION_Y  },
    { "r",  RADIUS      },
    { "rc", RADIUS      }
};

data
{
    float2 x; // lens position
    float r;  // Einstein radius
    float rc; // core radius
};

static float2 deflection(constant data* this, float2 x)
{
    // move to central coordinates
    x -= this->x;
    
    // NSIS deflection
    return this->r/(this->rc + length(x))*x;
}

static void set(global data* this, float x, float y, float r, float rc)
{
    // lens position
    this->x = (float2)(x, y);
    
    // Einstein radius
    this->r = r;
    
    // core radius
    this->rc = rc;
}
