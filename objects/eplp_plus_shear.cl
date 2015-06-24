// An elliptical power law potential model plus shear
// \Phi(r,theta) = re ( G(r,\theta)/re )^alpha  G(r,\theta) = sqrt( x^2 + q^2 y^2) 
// see Leier & Metcalf 2015
type = LENS;

params
{
    { "x",      POSITION_X  },
    { "y",      POSITION_Y  },
    { "re",     RADIUS      },
    { "alpha",  PARAMETER   },
    { "q",      AXIS_RATIO  },
    { "pa",     POS_ANGLE   },
    { "g1",     PARAMETER   },
    { "g2",     PARAMETER   }
};

data
{
    float2 x;
    mat22 m;
    mat22 w;
    
    float q2;
    float e;
    float re;
    float alpha;

    mat22 g;
};

static float2 deflection(constant data* this, float2 x)
{
    float2 y;
    
    float2 dx = x - this->x;
    float2 dy = mv22(this->m,dx);
    
    float eta = sqrt(dy.x*dy.x + this->q2*dy.y*dy.y);
    
    float a_iso = this->re*powr(eta/this->re , this->alpha) / eta;
    
    y.x = a_iso * dy.x ;
    y.y = a_iso * this->q2 * dy.y;
    
    y = mv22(this->w,y);
    
    return y + mv22(this->g, dx);
}

static void set(global data* this, float x, float y, float re, float alpha, float q, float pa, float g1, float g2)
{
    float c = cos(pa*DEG2RAD);
    float s = sin(pa*DEG2RAD);
    
    // lens position
    this->x = (float2)(x, y);
    
    // rotation matrix
    this->m = (mat22)(c, s, -s, c);
    
    // inverse rotation matrix
    this->w = (mat22)(c, -s, s, c);
    
    this->q2 = q*q;
    this->e = 1 - q*q;
    this->re = re;
    this->alpha = alpha;
    
    this->g = (mat22)(g1,g2,g2,-g1);
}
