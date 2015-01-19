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
    float2 x;
    float4 t;
    float rs;
    float norm;
};

static float devauc(constant struct devauc* src, float2 x)
{
    float4 t = src->t;
    float2 y = x - src->x;
    
    y = (float2)(dot(t.lo, y), dot(t.hi, y));

    float d = sqrt(dot(y, y));
	
    return src->norm*exp(-DEVAUC_B*pow(d/src->rs,0.25f));
}

static void set_devauc(global struct devauc* src, float x1, float x2, float r, float mag, float q, float pa)
{
    float c = cos(pa*DEG2RAD);
    float s = sin(pa*DEG2RAD);
    
    // source position
    src->x = (float2)(x1, x2);
    
    // transformation matrix: rotate and scale
    src->t = (float4)(q*c, q*s, -s, c);
    
    src->rs = r;
    src->norm = exp(-0.4f*mag*LOG_10)/PI/r/r/q*DEVAUC_C;
}
