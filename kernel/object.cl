// object types
enum
{
    LENS       = 'L',
    SOURCE     = 'S',
    FOREGROUND = 'F'
};

// structure that holds parameter definition
struct  __attribute__ ((aligned(4))) param
{
    char name[32];
    int  wrap;
};
