#pragma once

struct lensed
{
    // input data
    data* dat;
    
    // global log-likelihood normalisation from gain
    double lognorm;
    
    // parameter space
    size_t npars;
    param** pars;
    prior** pris;
    
    // results
    const char* fits;
    double logev;
    double logev_err;
    double logev_ins;
    double max_loglike;
    double* mean;
    double* sigma;
    double* ml;
    double* map;
    
    // worker queue
    cl_command_queue queue;
    
    // quadrature rule
    cl_mem qq;
    cl_mem ww;
    cl_mem ee;
    
    // main kernel
    cl_kernel kernel;
    size_t nd;
    cl_mem loglike;
    
    // parameter kernel
    cl_kernel set_params;
    cl_mem params;
    
    // dumper kernel
    cl_kernel dumper;
    cl_mem dumper_mem;
};
