#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <string.h>
#include <multinest.h>

#include "lensed.h"
#include "config.h"
#include "nested.h"
#include "log.h"
#include "version.h"

/* lens constructors */
extern void make_sie(struct lens*);

/* source constructors */
extern void make_sersic(struct source*);

/* read config from arguments */
extern void read_config(int argc, char* argv[], struct config*);

/* read input from config */
extern void read_input(const struct config*, struct data*);

int main(int argc, char* argv[])
{
    /* program config */
    struct config config;
    
    /* dimension of parameter space */
    int ndim;
    
    /* parameter wrapping */
    int* wrap;
    
    /* context for nested sampling */
    struct context context;
    
    /* read config file */
    read_config(argc, argv, &config);
    
    /* main engine on */
    info("lensed %d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    
    /* create input from config */
    read_input(&config, &context.data);
    
    /* TODO make lenses and sources dynamic */
    context.nlenses = 1;
    context.nsources = 1;
    
    /* check that number of lenses and sources make sense */
    if(context.nlenses < 0)
        error("invalid number (%d) of lenses", context.nlenses);
    if(context.nsources < 0)
        error("invalid number (%d) of sources", context.nsources);
    
    /* print number of sources and number of lenses */
    /* not yet meaningful
    info("number of lenses: %d", context.nlenses);
    info("number of sources: %d", context.nsources);
    */
    
    /* create parameter space */
    context.npspace = context.nlenses + context.nsources;
    context.pspace = malloc(context.npspace*sizeof(struct parametrizable));
    
    /* map lens and source arrays on top of parameter space */
    context.lenses = (struct lens*)context.pspace;
    context.sources = (struct source*)(context.pspace + context.nlenses);
    
    /* TODO dynamic types of lenses and sources */
    for(int i = 0; i < context.nlenses; ++i)
        make_sie(&context.lenses[i]);
    for(int i = 0; i < context.nsources; ++i)
        make_sersic(&context.sources[i]);
    
    /* count dimension of parameter space */
    ndim = 0;
    for(int i = 0; i < context.npspace; ++i)
        ndim += context.pspace[i].ndim;
    
    info("dimension of parameter space: %d", ndim);
    
    /* collect wrap information */
    wrap = malloc(ndim*sizeof(int));
    for(int i = 0, p = 0; i < context.npspace; p += context.pspace[i].ndim, ++i)
        context.pspace[i].wrap(&wrap[p]);
    
    /* set integration config */
    context.maxevals = config.maxevals;
    context.abstol = config.abstol;
    
    /* allocate scratch space for calculations */
    context.z = malloc(400*sizeof(double));
    
    /* gather MultiNest options */
    double tol = 0.5;
    double efr = 1.0;
    int npar = ndim;
    int nclspar = ndim;
    int maxmodes = 100;
    double ztol = -1E90;
    char root[100] = {0};
    strncpy(root, config.root, 99);
    int seed = -1;
    int fb = 1;
    int resume = 1;
    int outfile = 1;
    int initmpi = 1;
    double logzero = -DBL_MAX;
    int maxiter = 0;
    
    info("starting MultiNest...");
    
    /* run MultiNest */
    run(config.ins, config.mmodal, config.ceff, config.nlive, tol, efr, ndim,
        npar, nclspar, maxmodes, config.updint, ztol, root, seed, wrap, fb,
        resume, outfile, initmpi, logzero, maxiter, loglike, dumper, &context);
    
    /* free config */
    free(config.image);
    free(config.mask);
    free(config.root);
    
    /* free context */
    free(context.data.model);
    free(context.data.error);
    free(context.data.image);
    free(context.data.variance);
    free(context.data.xmin);
    free(context.data.xmax);
    for(int i = 0; i < context.npspace; ++i)
        free(context.pspace[i].ptr);
    free(context.pspace);
    free(context.z);
    
    /* free auxiliary data */
    free(wrap);
    
    return EXIT_SUCCESS;
}
