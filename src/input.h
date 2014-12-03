#pragma once

// options
typedef struct
{
    // lensed
    int gpu;
    int output;
    char* root;
    int batch_header;
    
    // data
    char* image;
    char* weight;
    char* mask;
    double gain;
    double offset;
    
    // MultiNest
    int nlive;
    int ins;
    int mmodal;
    int ceff;
    double tol;
    double eff;
    int maxmodes;
    int updint;
    int seed;
    int resume;
    int maxiter;
} options;

// opaque type for priors
typedef struct prior prior;

// parameters for objects
typedef struct
{
    // name of parameter
    char name[32];
    
    // identifier of parameter
    const char* id;
    
    // label, used for output
    const char* label;
    
    // prior for parameter
    prior* pri;
    
    // flag for wrap-around parameters
    int wrap;
} param;

// object types
enum
{
    OBJ_LENS = 'L',
    OBJ_SOURCE = 'S'
};

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
