// singular isothermal sphere
// follows Schneider, Kochanek, Wambsganss (2006)

type = LENS;

params
{
    { "x",  POSITION_X  },
    { "y",  POSITION_Y  },
    { "r",  RADIUS      }
};

data
{
    float2 x; // lens position
    float r;  // Einstein radius
};

static float2 deflection(constant data* this, float2 x)
{
    // SIS deflection
    return this->r*normalize(x - this->x);
}

static void set(global data* this, float x, float y, float r)
{
    // lens position
    this->x = (float2)(x, y);
    
    // Einstein radius
    this->r = r;
}
