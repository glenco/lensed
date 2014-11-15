#pragma once

/* context for sampling */
struct context
{
    /* input data */
    struct data data;
    
    /* integration settings */
    int maxevals;
    double abstol;
    
    /* array of lenses */
    int nlenses;
    struct lens* lenses;
    
    /* array of sources */
    int nsources;
    struct source* sources;
    
    /* parameter space */
    int npspace;
    struct parametrizable* pspace;
    
    /* scratch space for calculations */
    double* z;
};

/* callbacks */
void loglike(double cube[], int* ndim, int* npar, double* lnew, void* context);
void dumper(int* nsamples, int* nlive, int* npar, double** physlive,
            double** posterior, double** constraints, double* maxloglike,
            double* logz, /* double* inslogz, */ double* logzerr, void* context);
