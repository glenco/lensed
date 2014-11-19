#pragma once

/* configuration options */
struct options
{
    /* input */
    char* image;
    char* mask;
    double gain;
    double offset;
    
    /* MultiNest */
    char* root;
    int nlive;
    int ins;
    int mmodal;
    int ceff;
    double tol;
    double efr;
    int maxmodes;
    int updint;
    int seed;
    int fb;
    int resume;
    int outfile;
    int maxiter;
};

void read_options(int argc, char* argv[], struct options*);
void print_options(const struct options*);
