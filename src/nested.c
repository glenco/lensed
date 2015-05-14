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
    
    // simulate objects
    err = clEnqueueNDRangeKernel(lensed->queue, lensed->render, 1, NULL, lensed->render_gws, lensed->render_lws, 0, NULL, NULL);
    if(err != CL_SUCCESS)
        error("failed to run render kernel");
    
    // convolve with PSF if given
    if(lensed->convolve)
    {
        err = clEnqueueNDRangeKernel(lensed->queue, lensed->convolve, 2, NULL, lensed->convolve_gws, lensed->convolve_lws, 0, NULL, NULL);
        if(err != CL_SUCCESS)
            error("failed to run convolve kernel");
    }
    
    // compare with observed image
    err = clEnqueueNDRangeKernel(lensed->queue, lensed->loglike, 1, NULL, lensed->loglike_gws, lensed->loglike_lws, 0, NULL, NULL);
    if(err != CL_SUCCESS)
        error("failed to run loglike kernel");
    
    // map chi^2 values from device
    cl_float* loglike = clEnqueueMapBuffer(lensed->queue, lensed->loglike_mem, CL_TRUE, CL_MAP_READ, 0, lensed->size*sizeof(cl_float), 0, NULL, NULL, &err);
    if(err != CL_SUCCESS)
        error("failed to map loglike buffer");
    
    // sum log-likelihood value
    *lnew = 0;
    for(size_t i = 0; i < lensed->size; ++i)
        *lnew += loglike[i];
    
    // unmap result
    clEnqueueUnmapMemObject(lensed->queue, lensed->loglike_mem, loglike, 0, NULL, NULL);
}

void dumper(int* nsamples, int* nlive, int* npar, double** physlive,
            double** posterior, double** constraints, double* maxloglike,
            double* logz, double* inslogz, double* logzerr, void* lensed_)
{
    // result array indices
    enum { MEAN, SIGMA, ML, MAP };
    
    struct lensed* lensed = lensed_;
    
    cl_int err;
    cl_float* params;
    cl_mem result_mem;
    cl_float2* result_map;
    cl_float* model;
    cl_float* modvar;
    cl_float* residuals;
    cl_float* relerr;
    
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
    params = clEnqueueMapBuffer(lensed->queue, lensed->params, CL_TRUE, CL_MAP_WRITE, 0, lensed->npars*sizeof(cl_float), 0, NULL, NULL, &err);
    
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
    
    // simulate objects
    err = clEnqueueNDRangeKernel(lensed->queue, lensed->render, 1, NULL, lensed->render_gws, lensed->render_lws, 0, NULL, NULL);
    if(err != CL_SUCCESS)
        error("failed to run render kernel");
    
    // convolve with PSF if given
    if(lensed->convolve)
    {
        err = clEnqueueNDRangeKernel(lensed->queue, lensed->convolve, 2, NULL, lensed->convolve_gws, lensed->convolve_lws, 0, NULL, NULL);
        if(err != CL_SUCCESS)
            error("failed to run convolve kernel");
    }
    
    // where results are depends on convolution
    result_mem = lensed->convolve ? lensed->convolve_mem : lensed->render_mem;
    
    // map results from device
    result_map = clEnqueueMapBuffer(lensed->queue, result_mem, CL_FALSE, CL_MAP_READ, 0, lensed->size*sizeof(cl_float2), 0, NULL, NULL, NULL);
    if(!result_map)
        error("failed to map result buffer");
    
    // get model and model variance from image
    model = malloc(lensed->size*sizeof(cl_float));
    modvar = malloc(lensed->size*sizeof(cl_float));
    if(!model || !modvar)
        errori(NULL);
    for(size_t i = 0; i < lensed->size; ++i)
    {
        model[i] = result_map[i].s[0];
        modvar[i] = result_map[i].s[1];
    }
    
    // calculate residuals
    residuals = malloc(lensed->size*sizeof(cl_float));
    if(!residuals)
        errori(NULL);
    for(size_t i = 0; i < lensed->size; ++i)
        residuals[i] = lensed->image[i] - model[i];
    
    // calculate relative integration error
    relerr = malloc(lensed->size*sizeof(cl_float));
    if(!relerr)
        errori(NULL);
    for(size_t i = 0; i < lensed->size; ++i)
        relerr[i] = sqrt(modvar[i])/fabs(model[i]);
    
    // calculate minimum chi^2 value
    lensed->min_chi2 = 0;
    for(size_t i = 0; i < lensed->size; ++i)
        lensed->min_chi2 += lensed->weight[i]*residuals[i]*residuals[i];
    
    // output results if asked to
    if(lensed->fits)
    {
        // output layers
        cl_float* output[3] = { model, residuals, relerr };
        
        // write output to FITS
        write_output(lensed->fits, lensed->width, lensed->height, 3, output);
    }
    
    // unmap buffer
    clEnqueueUnmapMemObject(lensed->queue, result_mem, result_map, 0, NULL, NULL);
    
    // free arrays
    free(model);
    free(modvar);
    free(residuals);
    free(relerr);
}
