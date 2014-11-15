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

/* read input from config */
extern void read_input(const struct config*, struct data*);

int main(int argc, char* argv[])
{
    /* program data */
    struct lensed lensed;
    
    /* dimension of parameter space */
    int ndim;
    
    /* parameter wrapping */
    int* wrap;
    
    /* read config file */
    read_config(argc, argv, &lensed.config);
    
    /* main engine on */
    info("lensed %d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    
    /* print config file */
    print_config(&lensed.config);
    
    /* create input from config */
    read_input(&lensed.config, &lensed.data);
    
    /* TODO make lenses and sources dynamic */
    lensed.nlenses = 1;
    lensed.nsources = 1;
    
    /* check that number of lenses and sources make sense */
    if(lensed.nlenses < 0)
        error("invalid number (%d) of lenses", lensed.nlenses);
    if(lensed.nsources < 0)
        error("invalid number (%d) of sources", lensed.nsources);
    
    /* print number of sources and number of lenses */
    /* not yet meaningful
    info("number of lenses: %d", lensed.nlenses);
    info("number of sources: %d", lensed.nsources);
    */
    
    /* create parameter space */
    lensed.npspace = lensed.nlenses + lensed.nsources;
    lensed.pspace = malloc(lensed.npspace*sizeof(struct parametrizable));
    
    /* map lens and source arrays on top of parameter space */
    lensed.lenses = (struct lens*)lensed.pspace;
    lensed.sources = (struct source*)(lensed.pspace + lensed.nlenses);
    
    /* create lensed and sources */
    for(int i = 0; i < lensed.nlenses; ++i)
        make_sie(&lensed.lenses[i]);
    for(int i = 0; i < lensed.nsources; ++i)
        make_sersic(&lensed.sources[i]);
    
    /* count dimension of parameter space */
    ndim = 0;
    for(int i = 0; i < lensed.npspace; ++i)
        ndim += lensed.pspace[i].ndim;
    
    info("dimension of parameter space: %d", ndim);
    
    /* collect wrap information */
    wrap = malloc(ndim*sizeof(int));
    for(int i = 0, p = 0; i < lensed.npspace; p += lensed.pspace[i].ndim, ++i)
        lensed.pspace[i].wrap(&wrap[p]);
    
    /* allocate scratch space for calculations */
    lensed.z = malloc(400*sizeof(double));
    
    /* gather MultiNest options */
    int npar = ndim;
    int nclspar = ndim;
    double ztol = -1E90;
    char root[100] = {0};
    strncpy(root, lensed.config.root, 99);
    int initmpi = 1;
    double logzero = -DBL_MAX;
    
    info("starting MultiNest...");
    
    /* run MultiNest */
    run(lensed.config.ins, lensed.config.mmodal, lensed.config.ceff,
        lensed.config.nlive, lensed.config.evitol, lensed.config.efr,
        ndim, npar, nclspar, lensed.config.maxmodes, lensed.config.updint, ztol,
        root, lensed.config.seed, wrap, lensed.config.fb, lensed.config.resume,
        lensed.config.outfile, initmpi, logzero, lensed.config.maxiter,
        loglike, dumper, &lensed);
    
    /* free config */
    free(lensed.config.image);
    free(lensed.config.mask);
    free(lensed.config.root);
    
    /* free input */
    free(lensed.data.model);
    free(lensed.data.error);
    free(lensed.data.image);
    free(lensed.data.variance);
    free(lensed.data.xmin);
    free(lensed.data.xmax);
    
    /* free parameter space */
    for(int i = 0; i < lensed.npspace; ++i)
        free(lensed.pspace[i].ptr);
    free(lensed.pspace);
    
    /* free auxiliary data */
    free(lensed.z);
    free(wrap);
    
    return EXIT_SUCCESS;
}
