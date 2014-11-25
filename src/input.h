#pragma once

// options
typedef struct
{
    // data
    char* image;
    char* mask;
    double gain;
    double offset;
    
    // MultiNest
    char* root;
    int nlive;
    int ins;
    int mmodal;
    int ceff;
    double tol;
    double eff;
    int maxmodes;
    int updint;
    int seed;
    int fb;
    int resume;
    int outfile;
    int maxiter;
} options;

// parameters for objects
typedef struct
{
    // name of parameter, used as identifier
    const char* name;
    
    // label, used for output
    const char* label;
    
    // prior definition
    const char* prior;
} param;

// definition of objects
typedef struct
{
    // name of object, used as unique identifier
    const char* name;
    
    // type of object, used in kernel
    const char* type;
    
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
