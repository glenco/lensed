#pragma once

// all input settings
struct input
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
};

void read_input(int argc, char* argv[], struct input*);
void print_input(const struct input*);
