// harmonic lens \nabla^2 \psi = 0

type = LENS;

params
{
    { "x",  POSITION_X },
    { "y",  POSITION_Y },
    { "r",  RADIUS     },
    { "g1", PARAMETER, UNBOUNDED, -0.0f },
    { "g2", PARAMETER, UNBOUNDED, -0.0f },
    { "q2", PARAMETER, UNBOUNDED, -0.0f }, { "a2", ANGLE, UNBOUNDED, -0.0f },
    { "q3", PARAMETER, UNBOUNDED, -0.0f }, { "a3", ANGLE, UNBOUNDED, -0.0f },
    { "q4", PARAMETER, UNBOUNDED, -0.0f }, { "a4", ANGLE, UNBOUNDED, -0.0f },
    { "q5", PARAMETER, UNBOUNDED, -0.0f }, { "a5", ANGLE, UNBOUNDED, -0.0f },
    { "q6", PARAMETER, UNBOUNDED, -0.0f }, { "a6", ANGLE, UNBOUNDED, -0.0f },
    { "q7", PARAMETER, UNBOUNDED, -0.0f }, { "a7", ANGLE, UNBOUNDED, -0.0f },
    { "q8", PARAMETER, UNBOUNDED, -0.0f }, { "a8", ANGLE, UNBOUNDED, -0.0f },
    { "q9", PARAMETER, UNBOUNDED, -0.0f }, { "a9", ANGLE, UNBOUNDED, -0.0f },
    { "p2", PARAMETER, UNBOUNDED, -0.0f }, { "b2", ANGLE, UNBOUNDED, -0.0f },
    { "p3", PARAMETER, UNBOUNDED, -0.0f }, { "b3", ANGLE, UNBOUNDED, -0.0f },
    { "p4", PARAMETER, UNBOUNDED, -0.0f }, { "b4", ANGLE, UNBOUNDED, -0.0f },
    { "p5", PARAMETER, UNBOUNDED, -0.0f }, { "b5", ANGLE, UNBOUNDED, -0.0f },
    { "p6", PARAMETER, UNBOUNDED, -0.0f }, { "b6", ANGLE, UNBOUNDED, -0.0f },
    { "p7", PARAMETER, UNBOUNDED, -0.0f }, { "b7", ANGLE, UNBOUNDED, -0.0f },
    { "p8", PARAMETER, UNBOUNDED, -0.0f }, { "b8", ANGLE, UNBOUNDED, -0.0f },
    { "p9", PARAMETER, UNBOUNDED, -0.0f }, { "b9", ANGLE, UNBOUNDED, -0.0f }
};

data
{
    float2 x;     // position
    float  r;     // Einstein radius
    float2 Q[10]; // exterior multipoles
    float2 P[10]; // interior multipoles
};

static float2 deflection(local data* this, float2 x)
{
    float r, s;
    
    mat22 M;
    float R;
    float2 T;
    
    // delection angle
    float2 a = 0;
    
    // translate to central coordinates
    x = x - this->x;
    
    // distance from expansion point
    r = length(x);
    
    // iteration matrix, (cos, -sin, sin, cos)
    M = (mat22)(x.x, -x.y, x.y, x.x)/r;
    
    // inverse scale length
    s = this->r/r;
    
    // iteration start
    R = (this->r*this->r)/r;
    T = x/r;
    
    // exterior multipoles
    for(int n = 0; n < 10; ++n, R = R*s, T = mv22(M, T))
        a += 2*R*(float2)(T.x*this->Q[n].x + T.y*this->Q[n].y,
                          T.y*this->Q[n].x - T.x*this->Q[n].y);
    
    // scale length
    s = r/this->r;
    
    // iteration restart
    R = r;
    T = x/r;
    
    // interior multipoles
    for(int n = 2; n < 10; ++n, R = R*s, T = mv22(M, T))
        a += 2*R*(float2)(T.x*this->P[n].x + T.y*this->P[n].y,
                          T.x*this->P[n].y - T.y*this->P[n].x);
    
    // return the deflection angle
    return a;
}

static void set(local data* this,
                float x1, float x2, float r,
                float g1, float g2,
                float q2, float a2,
                float q3, float a3,
                float q4, float a4,
                float q5, float a5,
                float q6, float a6,
                float q7, float a7,
                float q8, float a8,
                float q9, float a9,
                float p2, float b2,
                float p3, float b3,
                float p4, float b4,
                float p5, float b5,
                float p6, float b6,
                float p7, float b7,
                float p8, float b8,
                float p9, float b9)
{
    // lens position
    this->x = (float2)(x1, x2);
    
    // Einstein radius
    this->r = r;
    
    // exterior multipoles
    this->Q[0] = (float2)(0.5f, 0);
    this->Q[1] = 0;
    this->Q[2] = q2*(float2)(cos(2*a2*DEG2RAD), sin(2*a2*DEG2RAD));
    this->Q[3] = q3*(float2)(cos(3*a3*DEG2RAD), sin(3*a3*DEG2RAD));
    this->Q[4] = q4*(float2)(cos(4*a4*DEG2RAD), sin(4*a4*DEG2RAD));
    this->Q[5] = q5*(float2)(cos(5*a5*DEG2RAD), sin(5*a5*DEG2RAD));
    this->Q[6] = q6*(float2)(cos(6*a6*DEG2RAD), sin(6*a6*DEG2RAD));
    this->Q[7] = q7*(float2)(cos(7*a7*DEG2RAD), sin(7*a7*DEG2RAD));
    this->Q[8] = q8*(float2)(cos(8*a8*DEG2RAD), sin(8*a8*DEG2RAD));
    this->Q[9] = q9*(float2)(cos(9*a9*DEG2RAD), sin(9*a9*DEG2RAD));
    
    // interior multipoles
    this->P[0] = 0;
    this->P[1] = 0;
    this->P[2] = 0.5f*(float2)(g1, g2);
    this->P[3] = p3*(float2)(cos(3*b3*DEG2RAD), -sin(3*b3*DEG2RAD));
    this->P[4] = p4*(float2)(cos(4*b4*DEG2RAD), -sin(4*b4*DEG2RAD));
    this->P[5] = p5*(float2)(cos(5*b5*DEG2RAD), -sin(5*b5*DEG2RAD));
    this->P[6] = p6*(float2)(cos(6*b6*DEG2RAD), -sin(6*b6*DEG2RAD));
    this->P[7] = p7*(float2)(cos(7*b7*DEG2RAD), -sin(7*b7*DEG2RAD));
    this->P[8] = p8*(float2)(cos(8*b8*DEG2RAD), -sin(8*b8*DEG2RAD));
    this->P[9] = p9*(float2)(cos(9*b9*DEG2RAD), -sin(9*b9*DEG2RAD));
}
