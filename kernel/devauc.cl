// De Vaucouleurs b param
#define DEVAUC_B 7.6692494425008039044f
// (DEVAUC_B^8)/(8!)
#define DEVAUC_C 296.826303766893f

OBJECT(devauc) = SOURCE;

PARAMS(devauc) = {
    { "x" },
    { "y" },
    { "r" },
    { "mag" },
    { "q" },
    { "pa", true },
};

struct devauc
{
    float2 x;   // source position
    mat22 t;    // coordinate transformation matric
    float rs;   // scale length
    float norm; // normalisation
};

static float devauc(constant struct devauc* data, float2 x)
{
    // DeVaucouleur's profile for centered and rotated coordinate system
    return data->norm*exp(-DEVAUC_B*sqrt(sqrt(length(mv22(data->t, x - data->x))/data->rs)));
}

static void set_devauc(global struct devauc* data, float x1, float x2, float r, float mag, float q, float pa)
{
    float c = cos(pa*DEG2RAD);
    float s = sin(pa*DEG2RAD);
    
    // source position
    data->x = (float2)(x1, x2);
    
    // transformation matrix: rotate and scale
    data->t = (mat22)(q*c, q*s, -s, c);
    
    // scale length
    data->rs = r;
    
    // normalisation to total luminosity
    data->norm = exp(-0.4f*mag*LOG_10)/PI/r/r/q*DEVAUC_C;
}
