#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <string.h>
#include <math.h>
#include <time.h>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include "multinest.h"

#include "input.h"
#include "data.h"
#include "lensed.h"
#include "kernel.h"
#include "nested.h"
#include "quadrature.h"
#include "log.h"
#include "version.h"

#define OBJECT_SIZE 64

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
    
    // OpenCL structures
    cl_device_id device;
    cl_context context;
    cl_program program;
    
    // maximum work-group size
    size_t max_wg_size;
    
    // buffer for objects
    cl_mem object_mem;
    
    // size of quadrature rule
    size_t nq;
    
    
    /*****************
     * configuration *
     *****************/
    
    // read input
    input* inp = read_input(argc, argv);
    
    // print banner
    info(LOG_BOLD "  _                         _ " LOG_DARK " ___" LOG_RESET);
    info(LOG_BOLD " | |                       | |" LOG_DARK "/   \\" LOG_RESET);
    info(LOG_BOLD " | | ___ _ __  ___  ___  __| |" LOG_DARK "  A  \\" LOG_RESET "  " LOG_BOLD "lensed" LOG_RESET " %d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    info(LOG_BOLD " | |/ _ \\ '_ \\/ __|/ _ \\/ _` |" LOG_DARK " < > |" LOG_RESET);
    info(LOG_BOLD " | |  __/ | | \\__ \\  __/ (_| |" LOG_DARK "  V  /" LOG_RESET);
    info(LOG_BOLD " |_|\\___|_| |_|___/\\___|\\__,_|" LOG_DARK "\\___/ " LOG_RESET);
    info(LOG_BOLD "                              " LOG_RESET);
    
    // print input
    print_input(inp);
    
    
    /**************
     * input data *
     **************/
    
    // read data given in input
    data* dat = read_data(inp);
    
    
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
    size_t nobjects = nlenses + nsources;
    
    verbose("  number of objects: %zu", nobjects);
    
    // create a list with all object names
    const char** objnames = malloc(nobjects*sizeof(const char*));
    for(size_t i = 0; i < nlenses; ++i)
        objnames[i] = lenses[i];
    for(size_t i = 0; i < nsources; ++i)
        objnames[nlenses+i] = sources[i];
    
    
    /****************
     * OpenCL setup *
     ****************/
    
    verbose("OpenCL setup");
    
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
        
        // load program, ...
        size_t nkernels;
        const char** kernels;
        
        verbose("  load program");
        load_kernels(nobjects, objnames, &nkernels, &kernels);
        
        // create program, ...
        verbose("  create program");
        program = clCreateProgramWithSource(context, nkernels, kernels, NULL, &err);
        if(!program || err != CL_SUCCESS)
            error("failed to create program");
        
        // build options
        const char* build_options =
            " -cl-denorms-are-zero"
            " -cl-strict-aliasing"
            " -cl-mad-enable"
            " -cl-no-signed-zeros"
            " -cl-fast-relaxed-math";
        
        // and build program
        verbose("  build program");
        err = clBuildProgram(program, 1, &device, build_options, NULL, NULL);
        if(err != CL_SUCCESS)
            error("failed to build program%s", LOG_LEVEL > LOG_VERBOSE ? " (use --verbose to see build log)" : "");
        
        // free program codes
        for(int i = 0; i < nkernels; ++i)
            free((void*)kernels[i]);
    }
    
    // get maximum work group size
    {
        err = clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), &max_wg_size, NULL);
        if(err != CL_SUCCESS)
            error("failed to get max work group size");
        verbose("  maximum work-group size: %zu", max_wg_size);
    }
    
    // allocate device memory for data
    {
        // number of pixels
        lensed.size = dat->size;
        
        // number of work-items
        lensed.nd = lensed.size;
        
        // pad number of work-items to work-group size
        if(lensed.nd % max_wg_size)
            lensed.nd += max_wg_size - (lensed.nd % max_wg_size);
        
        verbose("  number of pixels: %zu -> %zu", lensed.size, lensed.nd);
        
        // allocate data buffers
        lensed.indices = clCreateBuffer(context, CL_MEM_READ_ONLY, lensed.nd*sizeof(cl_int2), NULL, NULL);
        lensed.mean = clCreateBuffer(context, CL_MEM_READ_ONLY, lensed.nd*sizeof(cl_float), NULL, NULL);
        lensed.variance = clCreateBuffer(context, CL_MEM_READ_ONLY, lensed.nd*sizeof(cl_float), NULL, NULL);
        lensed.loglike = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, lensed.nd*sizeof(cl_float), NULL, NULL);
        if(!lensed.indices || !lensed.mean || !lensed.variance || !lensed.loglike)
            error("failed to allocate data buffers");
        
        // write data buffers
        err = 0;
        err |= clEnqueueWriteBuffer(lensed.queue, lensed.indices, CL_FALSE, 0, lensed.nd*sizeof(cl_int2), dat->indices, 0, NULL, NULL);
        err |= clEnqueueWriteBuffer(lensed.queue, lensed.mean, CL_FALSE, 0, lensed.nd*sizeof(cl_float), dat->mean, 0, NULL, NULL);
        err |= clEnqueueWriteBuffer(lensed.queue, lensed.variance, CL_FALSE, 0, lensed.nd*sizeof(cl_float), dat->variance, 0, NULL, NULL);
        if(err != CL_SUCCESS)
            error("failed to write data buffers");
        
        // set global log-likelihood normalisation
        lensed.lognorm = -log(inp->opts->gain);
    }
    
    // generate quadrature rule
    {
        // get the number of nodes of quadrature rule
        nq = quad_points();
        
        verbose("  quadrature points: %zu", nq);
        
        verbose("  create quadrature buffers");
        
        // allocate buffers for quadrature rule
        lensed.qq = clCreateBuffer(context, CL_MEM_READ_ONLY, nq*sizeof(cl_float2), NULL, NULL);
        lensed.ww = clCreateBuffer(context, CL_MEM_READ_ONLY, nq*sizeof(cl_float), NULL, NULL);
        lensed.ee = clCreateBuffer(context, CL_MEM_READ_ONLY, nq*sizeof(cl_float), NULL, NULL);
        if(!lensed.qq || !lensed.ww || !lensed.ee)
            error("failed to allocate quadrature buffers");
        
        // get array of abscissae
        cl_float2* qq = clEnqueueMapBuffer(lensed.queue, lensed.qq, CL_TRUE, CL_MAP_WRITE, 0, nq*sizeof(cl_float2), 0, NULL, NULL, &err);
        if(err != CL_SUCCESS)
            error("failed to map abscissa array for quadrature");
        
        // get array of weights
        cl_float* ww = clEnqueueMapBuffer(lensed.queue, lensed.ww, CL_TRUE, CL_MAP_WRITE, 0, nq*sizeof(cl_float), 0, NULL, NULL, &err);
        if(err != CL_SUCCESS)
            error("failed to map weight array for quadrature");
        
        // get array of error weights
        cl_float* ee = clEnqueueMapBuffer(lensed.queue, lensed.ee, CL_TRUE, CL_MAP_WRITE, 0, nq*sizeof(cl_float), 0, NULL, NULL, &err);
        if(err != CL_SUCCESS)
            error("failed to map error weight array for quadrature");
        
        // now generate the quadrature rules
        quad_rule(qq, ww, ee);
        
        // unmap the arrays
        clEnqueueUnmapMemObject(lensed.queue, lensed.qq, qq, 0, NULL, NULL);
        clEnqueueUnmapMemObject(lensed.queue, lensed.ww, ww, 0, NULL, NULL);
        clEnqueueUnmapMemObject(lensed.queue, lensed.ee, ee, 0, NULL, NULL);
    }
    
    verbose("  create object buffer");
    
    // allocate buffer for objects
    object_mem = clCreateBuffer(context, CL_MEM_READ_WRITE, nobjects*OBJECT_SIZE, NULL, &err);
    if(err != CL_SUCCESS)
        error("failed to create object buffer");
    
    verbose("  create kernel");
    
    // main kernel 
    lensed.kernel = clCreateKernel(program, "lensed", &err);
    if(err != CL_SUCCESS)
        error("failed to create lensed kernel");
    
    // main kernel arguments
    err = 0;
    err |= clSetKernelArg(lensed.kernel, 0, sizeof(cl_mem), &object_mem);
    err |= clSetKernelArg(lensed.kernel, 1, sizeof(cl_mem), &lensed.indices);
    err |= clSetKernelArg(lensed.kernel, 2, sizeof(size_t), &nq);
    err |= clSetKernelArg(lensed.kernel, 3, sizeof(cl_mem), &lensed.qq);
    err |= clSetKernelArg(lensed.kernel, 4, sizeof(cl_mem), &lensed.ww);
    err |= clSetKernelArg(lensed.kernel, 5, sizeof(cl_mem), &lensed.ee);
    err |= clSetKernelArg(lensed.kernel, 6, sizeof(cl_mem), &lensed.mean);
    err |= clSetKernelArg(lensed.kernel, 7, sizeof(cl_mem), &lensed.variance);
    err |= clSetKernelArg(lensed.kernel, 8, sizeof(cl_mem), &lensed.loglike);
    if(err != CL_SUCCESS)
        error("failed to set kernel arguments");
    
    
    /*******************
     * parameter space *
     *******************/
    
    verbose("parameter space");
    
    lensed.ndim = 0;
    
    {
        // array of dimensions
        size_t* ndims = malloc(nobjects*sizeof(size_t));
        
        // get dimensions for each object
        for(size_t i = 0; i < nobjects; ++i)
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
        
        verbose("  create parameter space buffer");
        
        // create the memory containing physical parameters on the device
        lensed.pspace = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, lensed.ndim*sizeof(cl_float), NULL, &err);
        if(err != CL_SUCCESS)
            error("failed to allocate parameter space");
        
        // params kernel
        lensed.set = clCreateKernel(program, "params", &err);
        if(err != CL_SUCCESS)
            error("failed to create params kernel");
        
        // params kernel arguments
        err = 0;
        err |= clSetKernelArg(lensed.set, 0, sizeof(cl_mem), &lensed.pspace);
        err |= clSetKernelArg(lensed.set, 1, sizeof(cl_mem), &object_mem);
        if(err != CL_SUCCESS)
            error("failed to set params kernel arguments");
        
        verbose("  gather parameter info");
        
        // array for wrap
        wrap = malloc(lensed.ndim*sizeof(int));
        
        // fill wrap array from device
        size_t offset = 0;
        for(size_t i = 0; i < nobjects; ++i)
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
    
    
    /***************
     * ready to go *
     ***************/
    
    info("run MultiNest");
    
    // gather MultiNest options
    int ndim = lensed.ndim;
    int npar = ndim;
    int nclspar = ndim;
    double ztol = -1E90;
    char root[100] = {0};
    strncpy(root, inp->opts->root, 99);
    int initmpi = 1;
    double logzero = -DBL_MAX;
    
    // take start time
    time_t start = time(0);
    
    // run MultiNest
    run(inp->opts->ins, inp->opts->mmodal, inp->opts->ceff, inp->opts->nlive,
        inp->opts->tol, inp->opts->eff, ndim, npar, nclspar,
        inp->opts->maxmodes, inp->opts->updint, ztol, root, inp->opts->seed,
        wrap, inp->opts->fb, inp->opts->resume, inp->opts->outfile, initmpi,
        logzero, inp->opts->maxiter, loglike, dumper, &lensed);
    
    // take end time
    time_t end = time(0);
    
    // duration
    double dur = difftime(end, start);
    
    // output duration
    info("run took %02d:%02d:%02d",
         (int)(dur/3600),
         (int)(fmod(dur, 3600)/60),
         (int)fmod(dur, 60));
    
    // free parameter space
    clReleaseMemObject(lensed.pspace);
    clReleaseKernel(lensed.set);
    free(wrap);
    
    // free kernel
    clReleaseKernel(lensed.kernel);
    
    // free objects
    clReleaseMemObject(object_mem);
    
    // free points
    clReleaseMemObject(lensed.qq);
    clReleaseMemObject(lensed.ww);
    clReleaseMemObject(lensed.ee);
    
    // free data
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
    
    // free input
    free_input(inp);
    
    // free data
    free_data(dat);
    
    return EXIT_SUCCESS;
}
