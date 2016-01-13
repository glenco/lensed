// object types
enum
{
    LENS       = 'L',
    SOURCE     = 'S',
    FOREGROUND = 'F',
    PLANE      = 'P'
};

// parameter types
enum
{
    PARAMETER = 0,
    POSITION_X,
    POSITION_Y,
    RADIUS,
    MAGNITUDE,
    AXIS_RATIO,
    POS_ANGLE,
    SCALE
};

// parameter bounds
#define UNBOUNDED {0, 0}
#define POS_BOUND {0, +FLT_MAX}
#define NEG_BOUND {-FLT_MAX, 0}

// structure that holds parameter definition
struct __attribute__ ((aligned (4))) param
{
    char  name[16];
    int   type;
    float bounds[2];
    float defval;
};
