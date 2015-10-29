// elliptical power law profile lens (Tessore & Metcalf 2015)

type = LENS;

params
{
    { "x",  POSITION_X              },
    { "y",  POSITION_Y              },
    { "r",  RADIUS                  },
    { "t",  PARAMETER, { 0.f, 2.f } },
    { "q",  AXIS_RATIO              },
    { "pa", POS_ANGLE               }
};

data
{
    float2 x; // lens position
    mat22 m;  // rotation matrix for position angle
    mat22 w;  // inverse rotation matrix
    float r;  // scale radius
    float t;  // slope
    float f;  // second flattening of ellipse
    float n;  // normalisation
};

static float2 deflection(local data* this, float2 x)
{
    float r, phi;
    float2 a, A;
    float c, s, c2, s2;
    mat22 R;
    
    const float T = 2 - this->t;
    const float f = this->f;
    
    // translate to central coordinates
    // rotate by position angle and make elliptical
    x = mv22(this->m, x - this->x);
    
    // scaled elliptical radius and polar angle
    r = length(x)/this->r;
    phi = atan2(x.y, x.x);
    
    // sines and cosines
    s = sincos(phi, &c);
    s2 = sincos(2*phi, &c2);
    
    // rotation matrix
    R = (mat22)(c2, -s2, s2, c2);
    
    // angular part of deflection, up to tenth order
    a  = A = (float2)(c, s);
    a += A = -f*(2* 1 - T)/(2* 1 + T)*mv22(R, A);
    a += A = -f*(2* 2 - T)/(2* 2 + T)*mv22(R, A);
    a += A = -f*(2* 3 - T)/(2* 3 + T)*mv22(R, A);
    a += A = -f*(2* 4 - T)/(2* 4 + T)*mv22(R, A);
    a += A = -f*(2* 5 - T)/(2* 5 + T)*mv22(R, A);
    a += A = -f*(2* 6 - T)/(2* 6 + T)*mv22(R, A);
    a += A = -f*(2* 7 - T)/(2* 7 + T)*mv22(R, A);
    a += A = -f*(2* 8 - T)/(2* 8 + T)*mv22(R, A);
    a += A = -f*(2* 9 - T)/(2* 9 + T)*mv22(R, A);
    a += A = -f*(2*10 - T)/(2*10 + T)*mv22(R, A);
    
    // radial part of deflection
    a *= this->n*powr(r, 1-this->t);
    
    // reverse coordinate rotation
    return mv22(this->w, a);
}

static void set(local data* this,
                float x1, float x2, float r, float t, float q, float pa)
{
    float c;
    float s = sincos(pa*DEG2RAD, &c);
    
    // lens position
    this->x = (float2)(x1, x2);
    
    // scale radius
    this->r = r;
    
    // rotation matrix with elliptical factor and scaling
    this->m = (1/r)*(mat22)(q*c, q*s, -s, c);
    
    // inverse rotation matrix
    this->w = (mat22)(c, -s, s, c);
    
    // slope of power law profile
    this->t = t;
    
    // second flattening of ellipse with axis ratio q
    this->f = (1 - q)/(1 + q);
    
    // normalisation of deflection
    this->n = 2*r*sqrt(q)/(1 + q);
}
