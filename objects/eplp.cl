// An elliptical power law potential model  \Phi(r,theta) = re ( G(r,\theta)/re )^alpha  G(r,\theta) = sqrt( x^2 + q^2 y^2) 
// see Leier & Metcalf 2015
type = LENS;

params
{
    { "x",      POSITION_X  },
    { "y",      POSITION_Y  },
    { "re",     RADIUS      },
    { "alpha",  PARAMETER   },
    { "q",      AXIS_RATIO  },
    { "pa",     POS_ANGLE   }
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
};

static float2 deflection(constant data* this, float2 x)
{
    float2 dy,y;
    float r;
 
    dy = mv22(this->m, x - this->x);
    
    r = length(dy);

    float eta = sqrt(dy.x*dy.x + this->q2*dy.y*dy.y);
    
    float a_iso = this->re*powr(eta/this->re , this->alpha) / eta;

    y.x = a_iso * dy.x ;
    y.y = a_iso * this->q2 * dy.y;
    
    return mv22(this->w, y);
}

static void set(global data* this, float x, float y, float re, float alpha, float q, float pa)
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
}
