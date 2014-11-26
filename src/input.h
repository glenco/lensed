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
    char name[32];
    
    // label, used for output
    const char* label;
    
    // prior definition
    const char* prior;
    
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
