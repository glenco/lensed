#pragma once

struct lensed
{
    // input data
    size_t width;
    size_t height;
    size_t size;
    cl_float* image;
    cl_float* weight;
    
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
    
    // global and local work size
    size_t lws[2];
    size_t gws[2];
    
    // parameter kernel
    cl_kernel set_params;
    cl_mem params;
    
    // kernel
    cl_kernel render;
    cl_mem value_mem;
    cl_mem error_mem;
    cl_kernel convolve;
    cl_mem convolve_mem;
    cl_kernel loglike;
    cl_mem loglike_mem;
};
