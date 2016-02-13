#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>

#include "opencl.h"
#include "input.h"
#include "prior.h"
#include "data.h"
#include "profile.h"
#include "lensed.h"
#include "nested.h"
#include "log.h"
#include "ds9.h"

void loglike(double cube[], int* ndim, int* npar, double* lnew, void* lensed_)
{
    struct lensed* lensed = lensed_;
    
    cl_event* map_params_ev        = NULL;
    cl_event* unmap_params_ev      = NULL;
    cl_event* set_params_ev        = NULL;
    cl_event* render_ev            = NULL;
    cl_event* convolve_ev          = NULL;
    cl_event* loglike_ev           = NULL;
    cl_event* map_loglike_mem_ev   = NULL;
    cl_event* unmap_loglike_mem_ev = NULL;
    
    if(lensed->profile)
    {
        map_params_ev        = profile_event();
        unmap_params_ev      = profile_event();
        set_params_ev        = profile_event();
        render_ev            = profile_event();
        convolve_ev          = profile_event();
        loglike_ev           = profile_event();
        map_loglike_mem_ev   = profile_event();
        unmap_loglike_mem_ev = profile_event();
    }
    
    // transform from unit cube to physical
    for(size_t i = 0; i < *npar; ++i)
    {
        // input parameter value; fixed to 0.5 for pseudo-priors
        double unit = i < *ndim ? cube[i] : 0.5;
        
        // physical parameter value
        double phys;
        
        // get parameter using map
        param* par = lensed->pars[lensed->pmap[i]];
        
        // draw parameter until it is within the bounds
        do
            phys = prior_apply(par->pri, unit);
        while(par->bounded && (phys < par->lower || phys > par->upper));
        
        // store physical parameter
        cube[i] = phys;
    }
    
    // error flag
    cl_int err = 0;
    
    // map parameter space on device
    cl_float* params = clEnqueueMapBuffer(lensed->queue, lensed->params, CL_TRUE, CL_MAP_WRITE, 0, lensed->npars*sizeof(cl_float), 0, NULL, map_params_ev, &err);
    
    // copy parameters to device using map
    for(size_t i = 0; i < lensed->npars; ++i)
        params[lensed->pmap[i]] = cube[i];
    
    // done with parameter space
    clEnqueueUnmapMemObject(lensed->queue, lensed->params, params, 0, NULL, unmap_params_ev);
    
    // set parameters
    err |= clEnqueueTask(lensed->queue, lensed->set_params, 0, NULL, set_params_ev);
    
    // check for errors
    if(err != CL_SUCCESS)
        error("failed to set parameters");
    
    // simulate objects
    err = clEnqueueNDRangeKernel(lensed->queue, lensed->render, 1, NULL, lensed->render_gws, lensed->render_lws, 0, NULL, render_ev);
    if(err != CL_SUCCESS)
        error("failed to run render kernel");
    
    // convolve with PSF if given
    if(lensed->convolve)
    {
        err = clEnqueueNDRangeKernel(lensed->queue, lensed->convolve, 2, NULL, lensed->convolve_gws, lensed->convolve_lws, 0, NULL, convolve_ev);
        if(err != CL_SUCCESS)
            error("failed to run convolve kernel");
    }
    
    // compare with observed image
    err = clEnqueueNDRangeKernel(lensed->queue, lensed->loglike, 1, NULL, lensed->loglike_gws, lensed->loglike_lws, 0, NULL, loglike_ev);
    if(err != CL_SUCCESS)
        error("failed to run loglike kernel");
    
    // map chi^2 values from device
    cl_float* loglike = clEnqueueMapBuffer(lensed->queue, lensed->loglike_mem, CL_TRUE, CL_MAP_READ, 0, lensed->size*sizeof(cl_float), 0, NULL, map_loglike_mem_ev, &err);
    if(err != CL_SUCCESS)
        error("failed to map loglike buffer");
    
    // sum chi^2 value
    double chi2 = 0.0;
    for(size_t i = 0; i < lensed->size; ++i)
        chi2 += loglike[i];
    
    // unmap result
    clEnqueueUnmapMemObject(lensed->queue, lensed->loglike_mem, loglike, 0, NULL, unmap_loglike_mem_ev);
    
    // set log-likelihood
    *lnew = -0.5*chi2;
    
    if(lensed->profile)
    {
        clFinish(lensed->queue);
        
        profile_read(lensed->profile->map_params, map_params_ev);
        profile_read(lensed->profile->unmap_params, unmap_params_ev);
        profile_read(lensed->profile->set_params, set_params_ev);
        profile_read(lensed->profile->render, render_ev);
        if(lensed->convolve)
            profile_read(lensed->profile->convolve, convolve_ev);
        profile_read(lensed->profile->loglike, loglike_ev);
        profile_read(lensed->profile->map_loglike_mem, map_loglike_mem_ev);
        profile_read(lensed->profile->unmap_loglike_mem, unmap_loglike_mem_ev);
    }
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
    
    cl_float* output[5] = {0};
    const char* names[5] = {0};
    
    // copy parameters to results
    for(size_t i = 0; i < lensed->npars; ++i)
    {
        // get parameter index from map
        size_t p = lensed->pmap[i];
        
        // store values
        lensed->mean[p]     = constraints[0][MEAN*lensed->npars+i];
        lensed->sigma[p]    = constraints[0][SIGMA*lensed->npars+i];
        lensed->ml[p]       = constraints[0][ML*lensed->npars+i];
        lensed->map[p]      = constraints[0][MAP*lensed->npars+i];
    }
    
    // copy summary statistics
    lensed->logev = *logz;
    lensed->logev_err = *logzerr;
    lensed->logev_ins = *inslogz;
    lensed->max_loglike = *maxloglike;
    
    // output results if asked to
    if(lensed->fits || lensed->ds9)
    {
        // map parameter space on device
        cl_float* params = clEnqueueMapBuffer(lensed->queue, lensed->params, CL_TRUE, CL_MAP_WRITE, 0, lensed->npars*sizeof(cl_float), 0, NULL, NULL, &err);
        
        // copy ML parameters to device using map
        for(size_t i = 0; i < lensed->npars; ++i)
            params[lensed->pmap[i]] = constraints[0][ML*lensed->npars+i];
        
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
        output[4] = lensed->weight;
        
        // output extension names
        names[0] = "IMG";
        names[1] = "RES";
        names[2] = "RAW";
        names[3] = "ERR";
        names[4] = "WHT";
        
        // write output to FITS if asked to
        if(lensed->fits)
            write_output(lensed->fits, lensed->width, lensed->height, 5, output, names);
        
        // send output to DS9 if asked to
        if(lensed->ds9)
        {
            // in-memory FITS
            void* fits;
            size_t len;
            
            // write FITS
            len = write_memory(&fits, lensed->width, lensed->height, 5, output, names);
            
            // show FITS
            ds9_mecube(lensed->ds9, fits, len);
            
            // free in-memory FITS
            free(fits);
        }
        
        // unmap buffers
        clEnqueueUnmapMemObject(lensed->queue, image_mem, image_map, 0, NULL, NULL);
        clEnqueueUnmapMemObject(lensed->queue, lensed->value_mem, value_map, 0, NULL, NULL);
        clEnqueueUnmapMemObject(lensed->queue, lensed->error_mem, error_map, 0, NULL, NULL);
        
        // free arrays
        free(residuals);
        free(relerr);
    }
    
    // status output
    if(LOG_LEVEL <= LOG_INFO)
    {
        // calculate progress
        double progress = fmax(0, fmin(1, 1.0*nsamples[0]/nlive[0]/(maxloglike[0] - logz[0] - log(lensed->tol))));
        
        int i = 0;
        int n = floor(20*progress) + 0.5;
        int p = fmin(99, floor(100*progress)) + 0.5;
        
        fprintf(stdout, "  [");
        for(; i < n; ++i)
            fputc('#', stdout);
        for(; i < 20; ++i)
            fputc(' ', stdout);
        fprintf(stdout, "] %2d%% | L: %+10.3e | S: %d\r", p, maxloglike[0], nsamples[0]);
        fflush(stdout);
    }
}
