#pragma once

struct lensed
{
    // parameter space
    size_t npars;
    prior** pris;
    
    // worker queue
    cl_command_queue queue;
    
    // loglike arrays
    size_t size;
    size_t nd;
    cl_mem indices;
    cl_mem mean;
    cl_mem variance;
    cl_mem loglike;
    
    // global log-likelihood normalisation from gain
    double lognorm;
    
    // quadrature rule
    cl_mem qq;
    cl_mem ww;
    cl_mem ee;
    
    // main kernel
    cl_kernel kernel;
    
    // parameter kernel
    cl_kernel set_params;
    cl_mem params;
};
