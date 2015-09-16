// de Vaucouleurs b param
#define DEVAUC_B 7.6692494425008039044f
// (DEVAUC_B^8)/(8!)
#define DEVAUC_C 296.826303766893f

type = SOURCE;

params
{
    { "x",      POSITION_X  },
    { "y",      POSITION_Y  },
    { "r",      RADIUS      },
    { "mag",    MAGNITUDE   },
    { "q",      AXIS_RATIO  },
    { "pa",     POS_ANGLE   },
};

data
{
    float2 x;   // source position
    mat22 t;    // coordinate transformation matric
    float rs;   // scale length
    float norm; // normalisation
};

static float brightness(local data* this, float2 x)
{
    // de Vaucouleurs profile for centered and rotated coordinate system
    return this->norm*exp(-DEVAUC_B*sqrt(sqrt(length(mv22(this->t, x - this->x))/this->rs)));
}

static void set(local data* this, float x, float y, float r, float mag, float q, float pa)
{
    float c = cos(pa*DEG2RAD);
    float s = sin(pa*DEG2RAD);
    
    // source position
    this->x = (float2)(x, y);
    
    // transformation matrix: rotate and scale
    this->t = (mat22)(q*c, q*s, -s, c);
    
    // scale length
    this->rs = r;
    
    // normalisation to total luminosity
    this->norm = exp(-0.4f*mag*LOG_10)/PI/r/r/q*DEVAUC_C;
}
