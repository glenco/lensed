#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>

#include "opencl.h"
#include "input.h"
#include "prior.h"
#include "data.h"
#include "lensed.h"
#include "nested.h"
#include "log.h"

void loglike(double cube[], int* ndim, int* npar, double* lnew, void* lensed_)
{
    struct lensed* lensed = lensed_;
    
    // transform from unit cube to physical
    for(size_t i = 0; i < lensed->npars; ++i)
    {
        double phys;
        do
            phys = apply_prior(lensed->pars[i]->pri, cube[i]);
        while(lensed->pars[i]->bounded && (phys < lensed->pars[i]->lower || phys > lensed->pars[i]->upper));
        cube[i] = phys;
    }
    
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
    
    // sum chi^2 value
    double chi2 = 0.0;
    for(size_t i = 0; i < lensed->size; ++i)
        chi2 += loglike[i];
    
    // unmap result
    clEnqueueUnmapMemObject(lensed->queue, lensed->loglike_mem, loglike, 0, NULL, NULL);
    
    // set log-likelihood
    *lnew = -0.5*chi2;
}

void dumper(int* nsamples, int* nlive, int* npar, double** physlive,
            double** posterior, double** constraints, double* maxloglike,
            double* logz, double* inslogz, double* logzerr, void* lensed_)
{
    // result array indices
    enum { MEAN, SIGMA, ML, MAP };
    
    struct lensed* lensed = lensed_;
    
    cl_int err;
    cl_mem image_mem;
    cl_float* value_map;
    cl_float* error_map;
    cl_float* image_map;
    cl_float* residuals;
    cl_float* relerr;
    
    cl_float* output[4] = {0};
    
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
    
    // output results if asked to
    if(lensed->fits)
    {
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
        
        // where values are depends on convolution
        image_mem = lensed->convolve ? lensed->convolve_mem : lensed->value_mem;
        
        // map output from device
        image_map = clEnqueueMapBuffer(lensed->queue, image_mem, CL_FALSE, CL_MAP_READ, 0, lensed->size*sizeof(cl_float), 0, NULL, NULL, NULL);
        value_map = clEnqueueMapBuffer(lensed->queue, lensed->value_mem, CL_FALSE, CL_MAP_READ, 0, lensed->size*sizeof(cl_float), 0, NULL, NULL, NULL);
        error_map = clEnqueueMapBuffer(lensed->queue, lensed->error_mem, CL_TRUE, CL_MAP_READ, 0, lensed->size*sizeof(cl_float), 0, NULL, NULL, NULL);
        if(!value_map || !error_map || !image_map)
            error("failed to map output buffer");
        
        // calculate residuals
        residuals = malloc(lensed->size*sizeof(cl_float));
        if(!residuals)
            errori(NULL);
        for(size_t i = 0; i < lensed->size; ++i)
            residuals[i] = lensed->image[i] - image_map[i];
        
        // calculate relative error
        relerr = malloc(lensed->size*sizeof(cl_float));
        if(!relerr)
            errori(NULL);
        for(size_t i = 0; i < lensed->size; ++i)
            relerr[i] = error_map[i]/value_map[i];
        
        // output layers
        output[0] = image_map;
        output[1] = residuals;
        output[2] = value_map;
        output[3] = relerr;
        
        // write output to FITS
        write_output(lensed->fits, lensed->width, lensed->height, 4, output);
        
        // unmap buffers
        clEnqueueUnmapMemObject(lensed->queue, image_mem, image_map, 0, NULL, NULL);
        clEnqueueUnmapMemObject(lensed->queue, lensed->value_mem, value_map, 0, NULL, NULL);
        clEnqueueUnmapMemObject(lensed->queue, lensed->error_mem, error_map, 0, NULL, NULL);
        
        // free arrays
        free(residuals);
        free(relerr);
    }
}
