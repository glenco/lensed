#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <OpenCL/opencl.h>

#include "lensed.h"
#include "nested.h"
#include "constants.h"
#include "data.h"
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
    
    // zeros for buffer filling
    cl_float zero = 0;
    cl_float2 zero2 = {{ 0, 0 }};
    
    // zero the worker buffers
    err |= clEnqueueFillBuffer(lensed->queue, lensed->aa, &zero2, sizeof(cl_float2), 0, lensed->np*sizeof(cl_float2), 0, NULL, NULL);
    err |= clEnqueueFillBuffer(lensed->queue, lensed->ff, &zero, sizeof(cl_float), 0, lensed->np*sizeof(cl_float), 0, NULL, NULL);
    err |= clEnqueueFillBuffer(lensed->queue, lensed->model, &zero, sizeof(cl_float), 0, lensed->nd*sizeof(cl_float), 0, NULL, NULL);
    err |= clEnqueueFillBuffer(lensed->queue, lensed->error, &zero, sizeof(cl_float), 0, lensed->nd*sizeof(cl_float), 0, NULL, NULL);
    
    // map parameter space on device
    cl_float* pspace = clEnqueueMapBuffer(lensed->queue, lensed->pspace, CL_TRUE, CL_MAP_WRITE, 0, lensed->ndim*sizeof(cl_float), 0, NULL, NULL, &err);
    
    // copy parameters to device
    for(int i = 0; i < lensed->ndim; ++i)
        pspace[i] = cube[i];
    
    // done with parameter space
    clEnqueueUnmapMemObject(lensed->queue, lensed->pspace, pspace, 0, NULL, NULL);
    
    // set parameters
    for(size_t i = 0; i < lensed->nobjects; ++i)
        err |= clEnqueueTask(lensed->queue, lensed->setters[i], 0, NULL, NULL);
    
    // run worker kernels
    for(int i = 0; i < lensed->nkernels; ++i)
        err |= clEnqueueNDRangeKernel(lensed->queue, lensed->kernels[i], 1, NULL, &lensed->np, NULL, 0, NULL, NULL);
    
    // run reduce kernels
    for(int i = 0; i < lensed->nreduce; ++i)
        err |= clEnqueueNDRangeKernel(lensed->queue, lensed->reduce[i], 1, NULL, &lensed->nd, NULL, 0, NULL, NULL);
    
    // check for errors
    if(err != CL_SUCCESS)
        error("error running kernels");
    
    // map result from device
    cl_float* chi_sq = clEnqueueMapBuffer(lensed->queue, lensed->chi_sq, CL_TRUE, CL_MAP_READ, 0, lensed->nd*sizeof(cl_float), 0, NULL, NULL, &err);
    if(err != CL_SUCCESS)
        error("failed to map chi-square buffer");
    cl_float* log_norm = clEnqueueMapBuffer(lensed->queue, lensed->log_norm, CL_TRUE, CL_MAP_READ, 0, lensed->nd*sizeof(cl_float), 0, NULL, NULL, &err);
    if(err != CL_SUCCESS)
        error("failed to map log-normalisation buffer");
    
    /* sum likelihood */
    double sum = 0.0;
    for(int i = 0; i < lensed->size; ++i)
        sum += -0.5*chi_sq[i] + log_norm[i] + lensed->log_norm_glob;
    
    // unmap result
    clEnqueueUnmapMemObject(lensed->queue, lensed->chi_sq, chi_sq, 0, NULL, NULL);
    clEnqueueUnmapMemObject(lensed->queue, lensed->log_norm, log_norm, 0, NULL, NULL);
    
    /* set likelihood */
    *lnew = sum;
}

void dumper(int* nsamples, int* nlive, int* npar, double** physlive,
            double** posterior, double** constraints, double* maxloglike,
            double* logz, double* inslogz, double* logzerr, void* lensed_)
{
    struct lensed* lensed = lensed_;
    
    cl_int err;
    cl_float* data[3];
    
    // map model from device
    data[0] = clEnqueueMapBuffer(lensed->queue, lensed->model, CL_TRUE, CL_MAP_READ, 0, lensed->nd*sizeof(cl_float), 0, NULL, NULL, &err);
    if(err != CL_SUCCESS)
        error("failed to map model buffer");
    
    // map error from device
    data[1] = clEnqueueMapBuffer(lensed->queue, lensed->error, CL_TRUE, CL_MAP_READ, 0, lensed->nd*sizeof(cl_float), 0, NULL, NULL, &err);
    if(err != CL_SUCCESS)
        error("failed to map error buffer");
    
    // map chi-square from device
    data[2] = clEnqueueMapBuffer(lensed->queue, lensed->chi_sq, CL_TRUE, CL_MAP_READ, 0, lensed->nd*sizeof(cl_float), 0, NULL, NULL, &err);
    if(err != CL_SUCCESS)
        error("failed to map chi-square buffer");
    
    // write image to file
    write_data(lensed->fits, lensed->width, lensed->height, lensed->indices, 3, lensed->size, data);
    
    // unmap buffers
    clEnqueueUnmapMemObject(lensed->queue, lensed->model, data[0], 0, NULL, NULL);
    clEnqueueUnmapMemObject(lensed->queue, lensed->error, data[1], 0, NULL, NULL);
    clEnqueueUnmapMemObject(lensed->queue, lensed->chi_sq, data[2], 0, NULL, NULL);
}
