#pragma once

/* configuration */
struct config
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
    double evitol;
    double efr;
    int maxmodes;
    int updint;
    int seed;
    int fb;
    int resume;
    int outfile;
    int maxiter;
};

void read_config(int argc, char* argv[], struct config*);
void print_config(const struct config*);
