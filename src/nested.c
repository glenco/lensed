#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include "input.h"
#include "prior.h"
#include "data.h"
#include "lensed.h"
#include "nested.h"
#include "constants.h"
#include "log.h"

void loglike(double cube[], int* ndim, int* npar, double* lnew, void* lensed_)
{
    struct lensed* lensed = lensed_;
    
    // transform from unit cube to physical
    for(size_t i = 0; i < lensed->npars; ++i)
        apply_prior(lensed->pris[i], &cube[i]);
    
    // error flag
    cl_int err = 0;
    
    // map parameter space on device
    cl_float* params = clEnqueueMapBuffer(lensed->queue, lensed->params, CL_TRUE, CL_MAP_WRITE, 0, lensed->npars*sizeof(cl_float), 0, NULL, NULL, &err);
    
    // copy parameters to device
    for(size_t i = 0; i < lensed->npars; ++i)
        params[i] = cube[i];
    
    // done with parameter space
    clEnqueueUnmapMemObject(lensed->queue, lensed->params, params, 0, NULL, NULL);
    
    // set parameters
    err |= clEnqueueTask(lensed->queue, lensed->set_params, 0, NULL, NULL);
    
    // check for errors
    if(err != CL_SUCCESS)
        error("failed to set parameters");
    
    // run kernel
    err = clEnqueueNDRangeKernel(lensed->queue, lensed->kernel, 1, NULL, &lensed->nd, NULL, 0, NULL, NULL);
    
    // check for errors
    if(err != CL_SUCCESS)
        error("failed to run kernel");
    
    // map result from device
    cl_float* loglike = clEnqueueMapBuffer(lensed->queue, lensed->loglike, CL_TRUE, CL_MAP_READ, 0, lensed->nd*sizeof(cl_float), 0, NULL, NULL, &err);
    if(err != CL_SUCCESS)
        error("failed to map log-likelihood buffer");
    
    /* sum likelihood */
    double sum = 0.0;
    for(int i = 0; i < lensed->dat->size; ++i)
        sum += loglike[i] + lensed->lognorm;
    
    // unmap result
    clEnqueueUnmapMemObject(lensed->queue, lensed->loglike, loglike, 0, NULL, NULL);
    
    /* set likelihood */
    *lnew = sum;
}

void dumper(int* nsamples, int* nlive, int* npar, double** physlive,
            double** posterior, double** constraints, double* maxloglike,
            double* logz, double* inslogz, double* logzerr, void* lensed_)
{
    // result array indices
    enum { MEAN, SIGMA, ML, MAP };
    
    struct lensed* lensed = lensed_;
    
    cl_int err;
    cl_float4* output;
    
    // copy parameters to results
    for(size_t i = 0; i < lensed->npars; ++i)
        lensed->mean[i] = constraints[0][MEAN*lensed->npars+i];
    for(size_t i = 0; i < lensed->npars; ++i)
        lensed->sigma[i] = constraints[0][SIGMA*lensed->npars+i];
    for(size_t i = 0; i < lensed->npars; ++i)
        lensed->ml[i] = constraints[0][ML*lensed->npars+i];
    for(size_t i = 0; i < lensed->npars; ++i)
        lensed->map[i] = constraints[0][MAP*lensed->npars+i];
    
    // copy summary statistics
    lensed->logev = *logz;
    lensed->logev_err = *logzerr;
    lensed->logev_ins = *inslogz;
    lensed->max_loglike = *maxloglike;
    
    // map parameter space on device
    cl_float* params = clEnqueueMapBuffer(lensed->queue, lensed->params, CL_TRUE, CL_MAP_WRITE, 0, lensed->npars*sizeof(cl_float), 0, NULL, NULL, &err);
    
    // copy ML parameters to device
    for(size_t i = 0; i < lensed->npars; ++i)
        params[i] = constraints[0][ML*lensed->npars+i];
    
    // done with parameter space
    clEnqueueUnmapMemObject(lensed->queue, lensed->params, params, 0, NULL, NULL);
    
    // set parameters
    err |= clEnqueueTask(lensed->queue, lensed->set_params, 0, NULL, NULL);
    
    // check for errors
    if(err != CL_SUCCESS)
        error("failed to set parameters");
    
    // run kernel
    err = clEnqueueNDRangeKernel(lensed->queue, lensed->dumper, 1, NULL, &lensed->nd, NULL, 0, NULL, NULL);
    
    // check for errors
    if(err != CL_SUCCESS)
        error("failed to run dumper");
    
    // map output from device
    output = clEnqueueMapBuffer(lensed->queue, lensed->dumper_mem, CL_TRUE, CL_MAP_READ, 0, lensed->nd*sizeof(cl_float4), 0, NULL, NULL, &err);
    if(err != CL_SUCCESS)
        error("failed to map dumper buffer");
    
    // output results if asked to
    if(lensed->fits)
        write_output(lensed->fits, lensed->dat, 4, output);
    
    // unmap buffers
    clEnqueueUnmapMemObject(lensed->queue, lensed->dumper_mem, output, 0, NULL, NULL);
}
