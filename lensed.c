#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <string.h>
#include <math.h>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include "multinest.h"

#include "options.h"
#include "data.h"
#include "lensed.h"
#include "kernel.h"
#include "nested.h"
#include "quadrature.h"
#include "log.h"
#include "version.h"

void opencl_notify(const char* errinfo, const void* private_info,  size_t cb, void* user_data)
{
    verbose("%s", errinfo);
}

int main(int argc, char* argv[])
{
    /* program data */
    struct lensed lensed;
    
    /* parameter wrapping */
    int* wrap;
    
    // OpenCL error code
    cl_int err;
    
    
    /*****************
     * configuration *
     *****************/
    
    struct options options;
    
    /* read options */
    read_options(argc, argv, &options);
    
    /* main engine on */
    info("lensed %d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    
    /* print options */
    print_options(&options);
    
    
    /**************
     * input data *
     **************/
    
    struct data data;
    read_data(options.image, options.mask, &data);
    
    // data
    lensed.width = data.width;
    lensed.height = data.height;
    lensed.size = data.size;
    lensed.indices = data.indices;
    
    
    /**********************
     * lenses and sources *
     **********************/
    
    verbose("objects");
    
    // TODO make lenses and sources dynamic
    size_t nlenses = 1;
    size_t nsources = 1;
    
    // create arrays for lenses and sources
    const char** lenses = malloc(nlenses*sizeof(const char*));
    const char** sources = malloc(nsources*sizeof(const char*));
    
    // set lenses and sources
    lenses[0] = "sie";
    sources[0] = "sersic";
    
    verbose("  number of lenses: %zu", nlenses);
    verbose("  number of sources: %zu", nsources);
    
    // total number of objects
    lensed.nobjects = nlenses + nsources;
    
    verbose("  number of objects: %zu", lensed.nobjects);
    
    // create a list with all object names
    const char** objnames = malloc(lensed.nobjects*sizeof(const char*));
    for(size_t i = 0; i < nlenses; ++i)
        objnames[i] = lenses[i];
    for(size_t i = 0; i < nsources; ++i)
        objnames[nlenses+i] = sources[i];
    
    
    /****************
     * worker setup *
     ****************/
    
    verbose("create worker");
    
    cl_device_id device;
    cl_context context;
    cl_program program;
    
    {
        // TODO decide the type of worker
        int gpu = 1;
        
        verbose("  device: %s", gpu ? "GPU" : "CPU");
        
        err = clGetDeviceIDs(NULL, gpu ? CL_DEVICE_TYPE_GPU : CL_DEVICE_TYPE_CPU, 1, &device, NULL);
        if(err != CL_SUCCESS)
            error("failed to get device");
        
        context = clCreateContext(0, 1, &device, opencl_notify, NULL, &err);
        if(!context || err != CL_SUCCESS)
            error("failed to create device context");
        
        lensed.queue = clCreateCommandQueue(context, device, 0, &err);
        if(!lensed.queue || err != CL_SUCCESS)
            error("failed to create command queue");
        
        // load kernels
        size_t nkernels;
        const char** kernels;
        
        verbose("  load kernels");
        load_kernels(lensed.nobjects, objnames, &nkernels, &kernels);
        
        // create program and build kernels
        verbose("  build kernels");
        
        program = clCreateProgramWithSource(context, nkernels, kernels, NULL, &err);
        if(!program || err != CL_SUCCESS)
            error("failed to create program");
        
        const char* options =
            " -cl-denorms-are-zero"
            " -cl-strict-aliasing"
            " -cl-mad-enable"
            " -cl-no-signed-zeros"
            " -cl-fast-relaxed-math";
        
        err = clBuildProgram(program, 1, &device, options, NULL, NULL);
        if(LOG_LEVEL <= LOG_VERBOSE)
        {
            char buf[65536];
            size_t siz;
            clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, sizeof(buf), buf, &siz);
            if(buf[0])
                verbose("%s", buf);
        }
        if(err != CL_SUCCESS)
            error("failed to build program%s", LOG_LEVEL > LOG_VERBOSE ? " (use --verbose to see build log)" : "");
        
        // free kernel codes
        for(int i = 0; i < nkernels; ++i)
            free((void*)kernels[i]);
    }
    
    // get maximum work group size
    size_t max_wg_size;
    err = clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(max_wg_size), &max_wg_size, NULL);
    if(err != CL_SUCCESS)
        error("failed to get max work group size");
    verbose("  maximum work-group size: %zu", max_wg_size);
    
    // allocate device memory for data
    {
        // number of data pixels
        lensed.nd = lensed.size;
        
        // pad number of data pixels to work-group size
        if(lensed.nd % max_wg_size)
            lensed.nd += max_wg_size - (lensed.nd % max_wg_size);
        
        verbose("  number of pixels: %zu -> %zu", lensed.size, lensed.nd);
        
        // allocate data fields
        lensed.model = clCreateBuffer(context, CL_MEM_READ_WRITE, lensed.nd*sizeof(cl_float), NULL, NULL);
        lensed.error = clCreateBuffer(context, CL_MEM_READ_WRITE, lensed.nd*sizeof(cl_float), NULL, NULL);
        lensed.mean = clCreateBuffer(context, CL_MEM_READ_ONLY, lensed.nd*sizeof(cl_float), NULL, NULL);
        lensed.variance = clCreateBuffer(context, CL_MEM_READ_ONLY, lensed.nd*sizeof(cl_float), NULL, NULL);
        lensed.chi_sq = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, lensed.nd*sizeof(cl_float), NULL, NULL);
        lensed.log_norm = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, lensed.nd*sizeof(cl_float), NULL, NULL);
        
        cl_float zero = 0;
        cl_float one = 1;
        
        err = 0;
        err |= clEnqueueFillBuffer(lensed.queue, lensed.mean, &zero, sizeof(cl_float2), 0, lensed.nd*sizeof(cl_float), 0, NULL, NULL);
        err |= clEnqueueFillBuffer(lensed.queue, lensed.variance, &one, sizeof(cl_float), 0, lensed.nd*sizeof(cl_float), 0, NULL, NULL);
        if(err != CL_SUCCESS)
            error("failed to fill device memory for worker");
        
        // get array for mean
        cl_float* mean = clEnqueueMapBuffer(lensed.queue, lensed.mean, CL_TRUE, CL_MAP_WRITE, 0, lensed.nd*sizeof(cl_float), 0, NULL, NULL, &err);
        if(err != CL_SUCCESS)
            error("failed to map array for mean");
        
        // get array for variance
        cl_float* variance = clEnqueueMapBuffer(lensed.queue, lensed.variance, CL_TRUE, CL_MAP_WRITE, 0, lensed.nd*sizeof(cl_float), 0, NULL, NULL, &err);
        if(err != CL_SUCCESS)
            error("failed to map array for variance");
        
        // set mean and variance
        for(size_t i = 0; i < lensed.size; ++i)
        {
            mean[i] = data.data[i];
            variance[i] = (data.data[i] + options.offset)/options.gain;
        }
        
        // unmap arrays
        clEnqueueUnmapMemObject(lensed.queue, lensed.mean, mean, 0, NULL, NULL);
        clEnqueueUnmapMemObject(lensed.queue, lensed.variance, variance, 0, NULL, NULL);
        
        // set global log-likelihood normalisation
        lensed.log_norm_glob = -log(options.gain);
    }
    
    // allocate device memory for points
    {
        // get the number of nodes of quadrature rule
        lensed.nq = quad_points();
        
        verbose("  quadrature points: %zu", lensed.nq);
        
        // get number of points needed for quadrature 
        lensed.np = lensed.nd*lensed.nq;
        
        // pad to multiple of work-group size
        if(lensed.np % max_wg_size)
            lensed.np += max_wg_size - (lensed.np % max_wg_size);
        
        verbose("  number of points: %zu -> %zu", lensed.size*lensed.nq, lensed.np);
        
        verbose("  allocate memory");
        
        lensed.xx = clCreateBuffer(context, CL_MEM_READ_ONLY, lensed.np*sizeof(cl_float2), NULL, NULL);
        lensed.yy = clCreateBuffer(context, CL_MEM_READ_WRITE, lensed.np*sizeof(cl_float2), NULL, NULL);
        lensed.aa = clCreateBuffer(context, CL_MEM_READ_WRITE, lensed.np*sizeof(cl_float2), NULL, NULL);
        lensed.ff = clCreateBuffer(context, CL_MEM_READ_WRITE, lensed.np*sizeof(cl_float), NULL, NULL);
        lensed.ww = clCreateBuffer(context, CL_MEM_READ_ONLY, lensed.np*sizeof(cl_float), NULL, NULL);
        lensed.ee = clCreateBuffer(context, CL_MEM_READ_ONLY, lensed.np*sizeof(cl_float), NULL, NULL);
        if(!lensed.xx || !lensed.yy || !lensed.aa || !lensed.ff || !lensed.ww || !lensed.ee)
            error("failed to allocate device memory for worker");
        
        cl_float zero = 0;
        cl_float2 zero2 = {{ 0, 0 }};
        
        err = 0;
        err |= clEnqueueFillBuffer(lensed.queue, lensed.xx, &zero2, sizeof(cl_float2), 0, lensed.np*sizeof(cl_float2), 0, NULL, NULL);
        err |= clEnqueueFillBuffer(lensed.queue, lensed.yy, &zero2, sizeof(cl_float2), 0, lensed.np*sizeof(cl_float2), 0, NULL, NULL);
        err |= clEnqueueFillBuffer(lensed.queue, lensed.aa, &zero2, sizeof(cl_float2), 0, lensed.np*sizeof(cl_float2), 0, NULL, NULL);
        err |= clEnqueueFillBuffer(lensed.queue, lensed.ff, &zero, sizeof(cl_float), 0, lensed.np*sizeof(cl_float), 0, NULL, NULL);
        err |= clEnqueueFillBuffer(lensed.queue, lensed.ww, &zero, sizeof(cl_float), 0, lensed.np*sizeof(cl_float), 0, NULL, NULL);
        err |= clEnqueueFillBuffer(lensed.queue, lensed.ee, &zero, sizeof(cl_float), 0, lensed.np*sizeof(cl_float), 0, NULL, NULL);
        if(err != CL_SUCCESS)
            error("failed to fill device memory for worker");
    }
    
    // generate quadrature scheme
    {
        // get array of points
        cl_float2* xx = clEnqueueMapBuffer(lensed.queue, lensed.xx, CL_TRUE, CL_MAP_WRITE, 0, lensed.np*sizeof(cl_float2), 0, NULL, NULL, &err);
        if(err != CL_SUCCESS)
            error("failed to map points array for quadrature rule");
        
        // get array of weights
        cl_float* ww = clEnqueueMapBuffer(lensed.queue, lensed.ww, CL_TRUE, CL_MAP_WRITE, 0, lensed.nq*sizeof(cl_float), 0, NULL, NULL, &err);
        if(err != CL_SUCCESS)
            error("failed to map weights array for quadrature rule");
        
        // get array of error weights
        cl_float* ee = clEnqueueMapBuffer(lensed.queue, lensed.ee, CL_TRUE, CL_MAP_WRITE, 0, lensed.nq*sizeof(cl_float), 0, NULL, NULL, &err);
        if(err != CL_SUCCESS)
            error("failed to map error weights array for quadrature rule");
        
        // now generate the quadrature rules
        quad_rule(lensed.size, data.indices, xx, ww, ee);
        
        // unmap the arrays
        clEnqueueUnmapMemObject(lensed.queue, lensed.xx, xx, 0, NULL, NULL);
        clEnqueueUnmapMemObject(lensed.queue, lensed.ww, ww, 0, NULL, NULL);
        clEnqueueUnmapMemObject(lensed.queue, lensed.ee, ee, 0, NULL, NULL);
    }
    
    
    /*******************
     * object creation *
     *******************/
    
    verbose("create objects");
    
    // array of object memory
    lensed.objects = malloc(lensed.nobjects*sizeof(cl_mem));
    
    // get size for each object and allocate it
    for(size_t i = 0; i < lensed.nobjects; ++i)
    {
        // memory for object info
        cl_mem objsize_mem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(size_t), NULL, &err);
        if(err != CL_SUCCESS)
            error("failed to allocate memory for object info");
        
        // setup the kernel that will deliver object info
        char* kernname = kernel_name("size_", objnames[i]);
        cl_kernel objsize_kernel = clCreateKernel(program, kernname, &err);
        if(!objsize_kernel || err != CL_SUCCESS)
            error("failed to create kernel for object size");
        free(kernname);
        
        err = clSetKernelArg(objsize_kernel, 0, sizeof(cl_mem), &objsize_mem);
        if(err != CL_SUCCESS)
           error("failed to set object info kernel arguments");
        
        // run kernel
        err = clEnqueueTask(lensed.queue, objsize_kernel, 0, NULL, NULL);
        if(err != CL_SUCCESS)
            error("failed to run object info kernel");
        clFinish(lensed.queue);
        
        // get object size from memory
        size_t objsize;
        err = clEnqueueReadBuffer(lensed.queue, objsize_mem, CL_TRUE, 0, sizeof(size_t), &objsize, 0, NULL, NULL);
        if(err != CL_SUCCESS)
            error("failed to get object sizes");
        
        // an objsize of 1 usually means something is off
        if(objsize == 1)
            warn("You have an object of type \"%s\" that reports its size "
                 "as 1. Did you forget the `struct` in `sizeof(struct %s)` "
                 "inside `size_%s()`?", objnames[i], objnames[i], objnames[i]);
        
        // now create a buffer for the object
        lensed.objects[i] = clCreateBuffer(context, CL_MEM_READ_WRITE, objsize, NULL, &err);
        if(err != CL_SUCCESS)
            error("failed to allocate memory for objects");
        
        // clean up
        clFinish(lensed.queue);
        clReleaseKernel(objsize_kernel);
        clReleaseMemObject(objsize_mem);
    }
    
    
    /*******************
     * kernel creation *
     *******************/
    
    verbose("create kernels");
    
    // worker kernels
    {
        lensed.nkernels = nlenses + 1 + nsources;
        lensed.kernels = malloc(lensed.nkernels*sizeof(cl_kernel));
        
        size_t k = 0;
        size_t o = 0;
        
        // kernels for lenses
        for(size_t i = 0; i < nlenses; ++i)
        {
            lensed.kernels[k] = clCreateKernel(program, lenses[i], &err);
            if(err != CL_SUCCESS)
                error("failed to create kernel for lens");
            
            err = 0;
            err |= clSetKernelArg(lensed.kernels[k], 0, sizeof(cl_mem), &lensed.objects[o]);
            err |= clSetKernelArg(lensed.kernels[k], 1, sizeof(cl_mem), &lensed.xx);
            err |= clSetKernelArg(lensed.kernels[k], 2, sizeof(cl_mem), &lensed.aa);
            if(err != CL_SUCCESS)
                error("failed to set kernel arguments for lens");
            
            k += 1;
            o += 1;
        }
        
        // kernel to calculate source plane positions from x and alpha
        {
            lensed.kernels[k] = clCreateKernel(program, "deflect", &err);
            if(err != CL_SUCCESS)
                error("failed to create kernel for deflection");
            
            err = 0;
            err |= clSetKernelArg(lensed.kernels[k], 0, sizeof(cl_mem), &lensed.xx);
            err |= clSetKernelArg(lensed.kernels[k], 1, sizeof(cl_mem), &lensed.aa);
            err |= clSetKernelArg(lensed.kernels[k], 2, sizeof(cl_mem), &lensed.yy);
            if(err != CL_SUCCESS)
                error("failed to set kernel arguments for deflection");
            
            k += 1;
        }
        
        // kernels for sources
        for(size_t i = 0; i < nsources; ++i)
        {
            lensed.kernels[k] = clCreateKernel(program, sources[i], &err);
            if(err != CL_SUCCESS)
                error("failed to create kernel for source");
            
            err = 0;
            err |= clSetKernelArg(lensed.kernels[k], 0, sizeof(cl_mem), &lensed.objects[o]);
            err |= clSetKernelArg(lensed.kernels[k], 1, sizeof(cl_mem), &lensed.yy);
            err |= clSetKernelArg(lensed.kernels[k], 2, sizeof(cl_mem), &lensed.ff);
            if(err != CL_SUCCESS)
                error("failed to set kernel arguments for source");
            
            k += 1;
            o += 1;
        }
    }
    
    // reduce kernels
    {
        lensed.nreduce = 2;
        lensed.reduce = malloc(lensed.nreduce*sizeof(cl_kernel));
        
        size_t k = 0;
        
        // kernel to sum quadrature rule
        {
            lensed.reduce[k] = clCreateKernel(program, "quadrature", &err);
            if(err != CL_SUCCESS)
                error("failed to create kernel for quadrature");
            
            err = 0;
            err |= clSetKernelArg(lensed.reduce[k], 0, sizeof(size_t), &lensed.nq);
            err |= clSetKernelArg(lensed.reduce[k], 1, sizeof(cl_mem), &lensed.ff);
            err |= clSetKernelArg(lensed.reduce[k], 2, sizeof(cl_mem), &lensed.ww);
            err |= clSetKernelArg(lensed.reduce[k], 3, sizeof(cl_mem), &lensed.ee);
            err |= clSetKernelArg(lensed.reduce[k], 4, sizeof(cl_mem), &lensed.model);
            err |= clSetKernelArg(lensed.reduce[k], 5, sizeof(cl_mem), &lensed.error);
            if(err != CL_SUCCESS)
                error("failed to set kernel arguments for quadrature");
            
            k += 1;
        }
        
        // kernel to calculate log-likelihood
        {
            lensed.reduce[k] = clCreateKernel(program, "loglike", &err);
            if(err != CL_SUCCESS)
                error("failed to create kernel for log-likelihood");
            
            err = 0;
            err |= clSetKernelArg(lensed.reduce[k], 0, sizeof(cl_mem), &lensed.model);
            err |= clSetKernelArg(lensed.reduce[k], 1, sizeof(cl_mem), &lensed.error);
            err |= clSetKernelArg(lensed.reduce[k], 2, sizeof(cl_mem), &lensed.mean);
            err |= clSetKernelArg(lensed.reduce[k], 3, sizeof(cl_mem), &lensed.variance);
            err |= clSetKernelArg(lensed.reduce[k], 4, sizeof(cl_mem), &lensed.chi_sq);
            err |= clSetKernelArg(lensed.reduce[k], 5, sizeof(cl_mem), &lensed.log_norm);
            if(err != CL_SUCCESS)
                error("failed to set kernel arguments for log-likelihood");
            
            k += 1;
        }
    }
    
    
    /*******************
     * parameter space *
     *******************/
    
    verbose("parameter space");
    
    lensed.ndim = 0;
    
    {
        // array of dimensions
        size_t* ndims = malloc(lensed.nobjects*sizeof(size_t));
        
        // get dimensions for each object
        for(size_t i = 0; i < lensed.nobjects; ++i)
        {
            // memory for object info
            cl_mem objndim_mem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(size_t), NULL, &err);
            if(err != CL_SUCCESS)
                error("failed to allocate memory for object dimensions");
            
            // setup the kernel that will deliver dimensions
            char* kernname = kernel_name("ndim_", objnames[i]);
            cl_kernel objndim_kernel = clCreateKernel(program, kernname, &err);
            if(err != CL_SUCCESS)
                error("failed to create kernel for object dimensions");
            free(kernname);
            
            err = clSetKernelArg(objndim_kernel, 0, sizeof(cl_mem), &objndim_mem);
            if(err != CL_SUCCESS)
                error("failed to set object dimensions kernel arguments");
            
            // run kernel
            err = clEnqueueTask(lensed.queue, objndim_kernel, 0, NULL, NULL);
            if(err != CL_SUCCESS)
                error("failed to run object dimensions kernel");
            clFinish(lensed.queue);
            
            // get object dimensions from memory
            err = clEnqueueReadBuffer(lensed.queue, objndim_mem, CL_TRUE, 0, sizeof(size_t), &ndims[i], 0, NULL, NULL);
            if(err != CL_SUCCESS)
                error("failed to get object dimensions");
            
            // add to total number of dimensions
            lensed.ndim += ndims[i];
            
            // clean up
            clFinish(lensed.queue);
            clReleaseKernel(objndim_kernel);
            clReleaseMemObject(objndim_mem);
        }
        
        verbose("  number of dimensions: %d", lensed.ndim);
        
        // create the memory containing physical parameters on the device
        lensed.pspace = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, lensed.ndim*sizeof(cl_float), NULL, &err);
        if(err != CL_SUCCESS)
            error("failed to allocate parameter space");
        
        // create setters for parameters
        lensed.setters = malloc(lensed.nobjects*sizeof(cl_kernel));
        
        size_t offset;
        
        offset = 0;
        for(size_t i = 0; i < lensed.nobjects; ++i)
        {
            char* kernname = kernel_name("set_", objnames[i]);
            lensed.setters[i] = clCreateKernel(program, kernname, &err);
            if(err != CL_SUCCESS)
                error("failed to create `%s()` kernel", kernname);
            
            err = 0;
            err |= clSetKernelArg(lensed.setters[i], 0, sizeof(cl_mem), &lensed.objects[i]);
            err |= clSetKernelArg(lensed.setters[i], 1, sizeof(cl_mem), &lensed.pspace);
            err |= clSetKernelArg(lensed.setters[i], 2, sizeof(size_t), &offset);
            if(err != CL_SUCCESS)
                error("failed to set arguments for kernel `%s()`", kernname);
            
            offset += ndims[i];
            
            free(kernname);
        }
        
        // array for wrap
        wrap = malloc(lensed.ndim*sizeof(int));
        
        // fill wrap array from device
        offset = 0;
        for(size_t i = 0; i < lensed.nobjects; ++i)
        {
            // device memory for wrap array
            cl_mem wrap_mem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, ndims[i]*sizeof(cl_int), NULL, &err);
            if(err != CL_SUCCESS)
                error("failed to allocate memory for wrap array");
            
            char* kernname = kernel_name("wrap_", objnames[i]);
            cl_kernel kern = clCreateKernel(program, kernname, &err);
            if(err != CL_SUCCESS)
                error("failed to create kernel `%s()`", kernname);
            
            err = 0;
            err |= clSetKernelArg(kern, 0, sizeof(cl_mem), &wrap_mem);
            if(err != CL_SUCCESS)
                error("failed to set arguments for kernel `%s()`", kernname);
            
            err = clEnqueueTask(lensed.queue, kern, 0, NULL, NULL);
            if(err != CL_SUCCESS)
                error("failed to run kernel `%s()`", kernname);
            
            // map wrap array from memory
            cl_int* wrap_dev = clEnqueueMapBuffer(lensed.queue, wrap_mem, CL_TRUE, CL_MAP_READ, 0, lensed.ndim*sizeof(cl_int), 0, NULL, NULL, &err);
            if(err != CL_SUCCESS)
                error("failed to get wrap array");
            
            // copy wrap array
            for(int i = 0; i < ndims[i]; ++i)
                wrap[offset+i] = wrap_dev[i];
            offset += ndims[i];
            
            // done with oject
            clEnqueueUnmapMemObject(lensed.queue, wrap_mem, wrap_dev, 0, NULL, NULL);
            clFinish(lensed.queue);
            clReleaseMemObject(wrap_mem);
            clReleaseKernel(kern);
            free(kernname);
        }
        
        // done with dimensions
        free(ndims);
    }
    
    
    /****************
     * dumper setup *
     ****************/
    
    // set up the dumper routine
    {
        // name of model output file
        const char fits[] = "dump.fits";
        
        // allocate space for filename
        lensed.fits = malloc(strlen(options.root) + strlen(fits) + 2);
        
        // create model file name
        strcpy(lensed.fits, "!");
        strcat(lensed.fits, options.root);
        strcat(lensed.fits, fits);
    }
    
    
    /***************
     * ready to go *
     ***************/
    
    // gather MultiNest options
    int ndim = lensed.ndim;
    int npar = ndim;
    int nclspar = ndim;
    double ztol = -1E90;
    char root[100] = {0};
    strncpy(root, options.root, 99);
    int initmpi = 1;
    double logzero = -DBL_MAX;
    
    info("start MultiNest");
    
    /* run MultiNest */
    run(options.ins, options.mmodal, options.ceff, options.nlive,
        options.tol, options.efr, ndim, npar, nclspar, options.maxmodes,
        options.updint, ztol, root, options.seed, wrap, options.fb,
        options.resume, options.outfile, initmpi, logzero, options.maxiter,
        loglike, dumper, &lensed);
    
    // free dumper settings
    free(lensed.fits);
    
    // free parameter space
    clReleaseMemObject(lensed.pspace);
    for(size_t i = 0; i < lensed.nobjects; ++i)
        clReleaseKernel(lensed.setters[i]);
    free(lensed.setters);
    free(wrap);
    
    // free kernels
    for(size_t i = 0; i < lensed.nkernels; ++i)
        clReleaseKernel(lensed.kernels[i]);
    free(lensed.kernels);
    for(size_t i = 0; i < lensed.nreduce; ++i)
        clReleaseKernel(lensed.reduce[i]);
    free(lensed.reduce);
    
    // free objects
    for(size_t i = 0; i < lensed.nobjects; ++i)
        clReleaseMemObject(lensed.objects[i]);
    free(lensed.objects);
    
    // free quadrature
    clReleaseMemObject(lensed.ww);
    clReleaseMemObject(lensed.ee);
    
    // free memory
    clReleaseMemObject(lensed.xx);
    clReleaseMemObject(lensed.aa);
    clReleaseMemObject(lensed.yy);
    clReleaseMemObject(lensed.ff);
    
    // free data
    clReleaseMemObject(lensed.model);
    clReleaseMemObject(lensed.error);
    clReleaseMemObject(lensed.mean);
    clReleaseMemObject(lensed.variance);
    
    // free worker
    clReleaseProgram(program);
    clReleaseCommandQueue(lensed.queue);
    clReleaseContext(context);
    
    // free lenses and sources
    free(lenses);
    free(sources);
    free(objnames);
    
    /* free options */
    free(options.image);
    free(options.mask);
    free(options.root);
    
    /* free input */
    free(data.data);
    free(data.indices);
    
    return EXIT_SUCCESS;
}
