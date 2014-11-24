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

#include "lensed.h"
#include "nested.h"
#include "constants.h"
#include "log.h"

/* TODO dynamic prior selection */
void priors(double cube[])
{
#if 1
    /* lens */
    cube[0] = 45. + 10*cube[0],
    cube[1] = 45. + 10*cube[1],
    cube[2] = 12. + 10*cube[2],
    cube[3] = cube[3],
    cube[4] = PI*cube[4],
    /* source */
    cube[5] = 40. + 20*cube[5];
    cube[6] = 40. + 20*cube[6];
    cube[7] = 5*cube[7];
    cube[8] = 5*cube[8];
    cube[9] = 0.5 + 7.5*cube[9];
    cube[10] = cube[10];
    cube[11] = PI*cube[11];
#else
    /* lens */
    cube[0] = 50.5;
    cube[1] = 50.5;
    cube[2] = 17.4381;
    cube[3] = 0.75;
    cube[4] = 2.35619;
    /* source */
    cube[5] = 51.7637;
    cube[6] = 49.933;
    cube[7] = 2.04882;
    cube[8] = 3.074;
    cube[9] = 7.159;
    cube[10] = 0.923181;
    cube[11] = 2.43091;
#endif
}

void loglike(double cube[], int* ndim, int* npar, double* lnew, void* lensed_)
{
    struct lensed* lensed = lensed_;
    
    // transform from unit cube to physical
    priors(cube);
    
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
