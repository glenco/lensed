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

// definition of objects
typedef struct
{
    const char* name;
    const char* type;
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
