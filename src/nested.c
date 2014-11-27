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
#include "lensed.h"
#include "nested.h"
#include "constants.h"
#include "log.h"

void loglike(double cube[], int* ndim, int* npar, double* lnew, void* lensed_)
{
    struct lensed* lensed = lensed_;
    
    // transform from unit cube to physical
    for(size_t i = 0; i < lensed->nparams; ++i)
        apply_prior(lensed->pris[i], &cube[i]);
    
    // error flag
    cl_int err = 0;
    
    // map parameter space on device
    cl_float* params = clEnqueueMapBuffer(lensed->queue, lensed->params, CL_TRUE, CL_MAP_WRITE, 0, lensed->nparams*sizeof(cl_float), 0, NULL, NULL, &err);
    
    // copy parameters to device
    for(int i = 0; i < lensed->nparams; ++i)
        params[i] = cube[i];
    
    // done with parameter space
    clEnqueueUnmapMemObject(lensed->queue, lensed->params, params, 0, NULL, NULL);
    
    // set parameters
    err |= clEnqueueTask(lensed->queue, lensed->set_params, 0, NULL, NULL);
    
    // run kernel
    err |= clEnqueueNDRangeKernel(lensed->queue, lensed->kernel, 1, NULL, &lensed->nd, NULL, 0, NULL, NULL);
    
    // check for errors
    if(err != CL_SUCCESS)
        error("error running kernels");
    
    // map result from device
    cl_float* loglike = clEnqueueMapBuffer(lensed->queue, lensed->loglike, CL_TRUE, CL_MAP_READ, 0, lensed->nd*sizeof(cl_float), 0, NULL, NULL, &err);
    if(err != CL_SUCCESS)
        error("failed to map log-likelihood buffer");
    
    /* sum likelihood */
    double sum = 0.0;
    for(int i = 0; i < lensed->size; ++i)
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
}
