// object types
enum
{
    LENS       = 'L',
    SOURCE     = 'S',
    FOREGROUND = 'F'
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
    POS_ANGLE
};

// structure that holds parameter definition
struct __attribute__ ((aligned (4))) param
{
    char  name[16];
    int   type;
    float bounds[2];
};
