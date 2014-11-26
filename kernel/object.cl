// object types
enum
{
    LENS   = 'L',
    SOURCE = 'S'
};

// structure that holds parameter definition
struct  __attribute__ ((aligned(4))) param
{
    char name[32];
    int  wrap;
};

// macro to specify type of object
#define OBJECT(x) constant int object_##x

// macro to simplify parameter definition for object
#define PARAMS(x) constant struct param parlst_##x[]

// macro to get number of parameters for object
#define NPARAMS(x) (sizeof(parlst_##x)/sizeof(struct param))

// macro to access specific parameter definition for object
#define PARAM(x, n) parlst_##x[n]
