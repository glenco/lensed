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
    
    // MultiNest tolerance
    double tol;
    
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
    
    // parameter kernel
    cl_kernel set_params;
    cl_mem params;
    
    // render kernel
    cl_mem value_mem;
    cl_mem error_mem;
    cl_kernel render;
    size_t render_lws[1];
    size_t render_gws[1];
    
    // convolve kernel
    cl_mem convolve_mem;
    cl_kernel convolve;
    size_t convolve_lws[2];
    size_t convolve_gws[2];
    
    // loglike kernel
    cl_mem loglike_mem;
    cl_kernel loglike;
    size_t loglike_lws[1];
    size_t loglike_gws[1];
    
    // profiling info
    struct {
        profile* map_params;
        profile* unmap_params;
        profile* set_params;
        profile* render;
        profile* convolve;
        profile* loglike;
        profile* map_loglike_mem;
        profile* unmap_loglike_mem;
    }* profile;
};
