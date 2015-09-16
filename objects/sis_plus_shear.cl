// singular isothermal sphere plus shear

type = LENS;

params
{
    { "x",  POSITION_X  },
    { "y",  POSITION_Y  },
    { "r",  RADIUS      },
    { "g1", PARAMETER   },
    { "g2", PARAMETER   }
};

data
{
    float2 x; // lens position
    mat22 g;  // shear matrix
    float r;  // Einstein radius
};

static float2 deflection(local data* this, float2 x)
{
    // move to central coordinates
    x -= this->x;
    
    // SIS deflection plus external shear
    return this->r*normalize(x) + mv22(this->g, x);
}

static void set(local data* this, float x, float y, float r, float g1, float g2)
{
    // lens position
    this->x = (float2)(x, y);
    
    // Einstein radius
    this->r = r;
    
    // shear matrix
    this->g = (mat22)(g1, g2, g2, -g1);
}
