#pragma once

// object types
enum
{
    OBJ_LENS       = 'L',
    OBJ_SOURCE     = 'S',
    OBJ_FOREGROUND = 'F'
};

// parameter types
enum
{
    PAR_PARAMETER = 0,
    PAR_POSITION_X,
    PAR_POSITION_Y,
    PAR_RADIUS,
    PAR_MAGNITUDE,
    PAR_AXIS_RATIO,
    PAR_POS_ANGLE
};

// option contains a path or a real
struct path_or_real {
    char* file;
    double value;
};

// options
typedef struct
{
    // lensed
    char* device;
    int output;
    char* root;
    int devices;
    int profile;
    int batch_header;
    int show_rules;
    char* rule;
    
    // data
    char* image;
    struct path_or_real* weight;
    struct path_or_real* xweight;
    char* mask;
    char* psf;
    double offset;
    struct path_or_real* gain;
    double bscale;
    
    // MultiNest
    int nlive;
    int ins;
    int mmodal;
    int ceff;
    double acc;
    double tol;
    double shf;
    int maxmodes;
    int feedback;
    int updint;
    int seed;
    int resume;
    int maxiter;
    
    // DS9 integration
    int ds9;
    char* ds9_name;
} options;

// opaque type for priors
typedef struct prior prior;

// parameters for objects
typedef struct
{
    // name of parameter
    const char* name;
    
    // identifier of parameter
    const char* id;
    
    // type of parameter
    int type;
    
    // hard parameter bounds
    double lower;
    double upper;
    int bounded;
    
    // prior for parameter
    prior* pri;
    
    // flag for wrap-around parameters
    int wrap;
    
    // flag for image plane priors
    int ipp;
    
    // flag for derived parameters
    int derived;
    
    // flag for default value
    int defval;
    
    // label, used for output
    const char* label;
} param;

// definition of objects
typedef struct
{
    // type of object
    int type;
    
    // size of object data
    size_t size;
    
    // unique identifier of object
    const char* id;
    
    // name of object, used in kernel
    const char* name;
    
    // parameters for object
    size_t npars;
    param* pars;
} object;

// all input settings
typedef struct
{
    // all configurable options
    options* opts;
    
    // keep track of required options
    int* reqs;
    
    // objects on the line of sight
    size_t nobjs;
    object* objs;
} input;

input* read_input(int argc, char* argv[]);
void print_input(const input* input);
void free_input(input* input);
