#pragma once

struct lensed
{
    // worker queue
    cl_command_queue queue;
    
    // input data
    size_t width;
    size_t height;
    size_t size;
    cl_uint2* indices;
    
    // loglike arrays
    size_t nd;
    cl_mem model;
    cl_mem error;
    cl_mem mean;
    cl_mem variance;
    cl_mem chi_sq;
    cl_mem log_norm;
    
    // global log-likelihood normalisation from gain
    double log_norm_glob;
    
    // worker arrays
    size_t nq;
    size_t np;
    cl_mem xx;
    cl_mem aa;
    cl_mem yy;
    cl_mem ff;
    cl_mem ww;
    cl_mem ee;
    
    // objects for lenses and sources
    size_t nobjects;
    cl_mem* objects;
    
    // worker kernels
    size_t nkernels;
    cl_kernel* kernels;
    
    // reduce kernels
    size_t nreduce;
    cl_kernel* reduce;
    
    // parameter space
    size_t ndim;
    cl_mem pspace;
    cl_kernel* setters;
    
    // dumper settings
    char* fits;
};
