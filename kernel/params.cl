// structure that holds parameter definition
struct  __attribute__ ((aligned(4))) param
{
    char name[32];
    int  wrap;
};

// macro to simplify parameter definition for object
#define PARAMS(obj) constant struct param params_##obj[]

// macro to get number of parameters for object
#define NPARAMS(obj) (sizeof(params_##obj)/sizeof(struct param))

// macro to access specific parameter definition for object
#define PARAM(obj, n) params_##obj[n]

// copy constant param to global array
static void parcpy(global struct param* dst, constant struct param* src)
{
    for(size_t i = 0; i < sizeof(dst->name); ++i)
        dst->name[i] = src->name[i];
    dst->wrap = src->wrap;
}
