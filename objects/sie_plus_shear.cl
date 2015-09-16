// singular isothermal ellipsoid plus shear
// follows Schneider, Kochanek, Wambsganss (2006)

type = LENS;

params
{
    { "x",  POSITION_X  },
    { "y",  POSITION_Y  },
    { "r",  RADIUS      },
    { "q",  AXIS_RATIO  },
    { "pa", POS_ANGLE   },
    { "g1", PARAMETER   },
    { "g2", PARAMETER   }
};

data
{
    float2 x; // lens position
    mat22 m;  // rotation matrix for position angle
    mat22 w;  // inverse rotation matrix
    mat22 g;  // shear matrix
    
    // auxiliary
    float q2;
    float e;
    float d;
};

static float2 deflection(local data* this, float2 x)
{
    float2 y;
    float r;
    
    // move to central coordinates
    x -= this->x;
    
    // rotate coordinates by position angle
    y = mv22(this->m, x);
    
    // SIE deflection
    r = this->e/sqrt(this->q2*y.x*y.x + y.y*y.y);
    y = this->d*(float2)(atan(y.x*r), atanh(y.y*r));
    
    // reverse coordinate rotation, apply shear
    return mv22(this->w, y) + mv22(this->g, x);
}

static void set(local data* this, float x, float y, float r, float q, float pa, float g1, float g2)
{
    float c = cos(pa*DEG2RAD);
    float s = sin(pa*DEG2RAD);
    
    // lens position
    this->x = (float2)(x, y);
    
    // rotation matrix
    this->m = (mat22)(c, s, -s, c);
    
    // inverse rotation matrix
    this->w = (mat22)(c, -s, s, c);
    
    // shear matrix
    this->g = (mat22)(g1, g2, g2, -g1);
    
    // auxiliary quantities
    this->q2 = q*q;
    this->e = sqrt(1 - q*q);
    this->d = r*sqrt(q)/sqrt(1 - q*q);
}
