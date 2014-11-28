#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
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

static void opencl_notify(const char* errinfo, const void* private_info,  size_t cb, void* user_data)
{
    verbose("%s", errinfo);
}

int main(int argc, char* argv[])
{
    // program data
    struct lensed lensed;
    
    // OpenCL error code
    cl_int err;
    
    // OpenCL structures
    cl_device_id device;
    cl_context context;
    cl_program program;
    
    // buffer for object data
    cl_mem data_mem;
    
    // maximum work-group size
    size_t max_wg_size;
    
    // size of quadrature rule
    cl_ulong nq;
    
    // buffers for main kernel
    cl_mem indices;
    cl_mem mean;
    cl_mem variance;
    
    
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
    lensed.dat = read_data(inp);
    
    // set global log-likelihood normalisation
    lensed.lognorm = -log(inp->opts->gain);
    
    
    /*******************
     * parameter space *
     *******************/
    
    // sum number of parameters
    lensed.npars = 0;
    for(size_t i = 0; i < inp->nobjs; ++i)
        lensed.npars += inp->objs[i].npars;
    
    // get all parameters
    lensed.pars = malloc(lensed.npars*sizeof(param*));
    if(!lensed.pars)
        errori(NULL);
    for(size_t i = 0, p = 0; i < inp->nobjs; ++i)
        for(size_t j = 0; j < inp->objs[i].npars; ++j, ++p)
            lensed.pars[p] = &inp->objs[i].pars[j];
    
    // get all priors that will be needed when running
    lensed.pris = malloc(lensed.npars*sizeof(prior*));
    if(!lensed.pris)
        errori(NULL);
    for(size_t i = 0, p = 0; i < inp->nobjs; ++i)
        for(size_t j = 0; j < inp->objs[i].npars; ++j, ++p)
            lensed.pris[p] = inp->objs[i].pars[j].pri;
    
    
    /***********
     * results *
     ***********/
    
    // results file if output is enabled
    if(inp->opts->output)
    {
        // prefix and suffix of results output file
        const char prefix[] = "!";
        const char suffix[] = ".fits";
        
        // allocate space for filename
        char* fits = malloc(strlen(prefix) + strlen(inp->opts->root) + strlen(suffix) + 1);
        
        // create model filename
        strcpy(fits, prefix);
        strcat(fits, inp->opts->root);
        strcat(fits, suffix);
        
        // set filename
        lensed.fits = fits;
    }
    else
    {
        // no output
        lensed.fits = NULL;
    }
    
    // arrays for parameters
    lensed.mean = malloc(lensed.npars*sizeof(double));
    lensed.sigma = malloc(lensed.npars*sizeof(double));
    lensed.ml = malloc(lensed.npars*sizeof(double));
    lensed.map = malloc(lensed.npars*sizeof(double));
    if(!lensed.mean || !lensed.sigma || !lensed.ml || !lensed.map)
        errori(NULL);
    
    
    /****************
     * kernel setup *
     ****************/
    
    verbose("kernel");
    
    {
        verbose("  device: %s", inp->opts->gpu ? "GPU" : "CPU");
        
        err = clGetDeviceIDs(NULL, inp->opts->gpu ? CL_DEVICE_TYPE_GPU : CL_DEVICE_TYPE_CPU, 1, &device, NULL);
        if(err != CL_SUCCESS)
            error("failed to get device");
        
        context = clCreateContext(0, 1, &device, opencl_notify, NULL, &err);
        if(!context || err != CL_SUCCESS)
            error("failed to create device context");
        
        lensed.queue = clCreateCommandQueue(context, device, 0, &err);
        if(!lensed.queue || err != CL_SUCCESS)
            error("failed to create command queue");
        
        // load program
        size_t nkernels;
        const char** kernels;
        
        verbose("  load program");
        main_program(inp->nobjs, inp->objs, &nkernels, &kernels);
        
        // output program
        if(inp->opts->output)
        {
            FILE* file;
            char* name;
            
            name = malloc(strlen(inp->opts->root) + strlen("kernel.cl") + 1);
            if(!name)
                errori(NULL);
            
            strcpy(name, inp->opts->root);
            strcat(name, "kernel.cl");
            
            file = fopen(name, "w");
            if(!file)
                errori("could not write %s", name);
            
            for(size_t i = 0; i < nkernels; ++i)
                fputs(kernels[i], file);
            
            fclose(file);
            free(name);
        }
        
        // create program
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
        // number of work-items
        lensed.nd = lensed.dat->size;
        
        // pad number of work-items to work-group size
        if(lensed.nd % max_wg_size)
            lensed.nd += max_wg_size - (lensed.nd % max_wg_size);
        
        verbose("  number of pixels: %zu -> %zu", lensed.dat->size, lensed.nd);
        
        // allocate data buffers
        indices = clCreateBuffer(context, CL_MEM_READ_ONLY, lensed.nd*sizeof(cl_int2), NULL, NULL);
        mean = clCreateBuffer(context, CL_MEM_READ_ONLY, lensed.nd*sizeof(cl_float), NULL, NULL);
        variance = clCreateBuffer(context, CL_MEM_READ_ONLY, lensed.nd*sizeof(cl_float), NULL, NULL);
        lensed.loglike = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, lensed.nd*sizeof(cl_float), NULL, NULL);
        if(!indices || !mean || !variance || !lensed.loglike)
            error("failed to allocate data buffers");
        
        // write data buffers
        err = 0;
        err |= clEnqueueWriteBuffer(lensed.queue, indices, CL_FALSE, 0, lensed.nd*sizeof(cl_int2), lensed.dat->indices, 0, NULL, NULL);
        err |= clEnqueueWriteBuffer(lensed.queue, mean, CL_FALSE, 0, lensed.nd*sizeof(cl_float), lensed.dat->mean, 0, NULL, NULL);
        err |= clEnqueueWriteBuffer(lensed.queue, variance, CL_FALSE, 0, lensed.nd*sizeof(cl_float), lensed.dat->variance, 0, NULL, NULL);
        if(err != CL_SUCCESS)
            error("failed to write data buffers");
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
    
    // create buffer that contains object data
    {
        // collect total size of object data
        size_t data_size = 0;
        for(size_t i = 0; i < inp->nobjs; ++i)
            data_size += inp->objs[i].size;
        
        verbose("  create object data buffer");
        
        // allocate buffer for object data
        data_mem = clCreateBuffer(context, CL_MEM_READ_WRITE, data_size, NULL, &err);
        if(err != CL_SUCCESS)
            error("failed to create object buffer");
    }
    
    // create kernel
    {
        verbose("  create kernel");
        
        // main kernel 
        lensed.kernel = clCreateKernel(program, "loglike", &err);
        if(err != CL_SUCCESS)
            error("failed to create loglike kernel");
        
        // main kernel arguments
        err = 0;
        err |= clSetKernelArg(lensed.kernel, 0, sizeof(cl_mem), &data_mem);
        err |= clSetKernelArg(lensed.kernel, 1, sizeof(cl_mem), &indices);
        err |= clSetKernelArg(lensed.kernel, 2, sizeof(cl_ulong), &nq);
        err |= clSetKernelArg(lensed.kernel, 3, sizeof(cl_mem), &lensed.qq);
        err |= clSetKernelArg(lensed.kernel, 4, sizeof(cl_mem), &lensed.ww);
        err |= clSetKernelArg(lensed.kernel, 5, sizeof(cl_mem), &lensed.ee);
        err |= clSetKernelArg(lensed.kernel, 6, sizeof(cl_mem), &mean);
        err |= clSetKernelArg(lensed.kernel, 7, sizeof(cl_mem), &variance);
        err |= clSetKernelArg(lensed.kernel, 8, sizeof(cl_mem), &lensed.loglike);
        if(err != CL_SUCCESS)
            error("failed to set loglike kernel arguments");
    }
    
    // create the buffer that will pass parameter values to objects
    {
        verbose("  create parameter buffer");
        
        // create the memory containing physical parameters on the device
        lensed.params = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, lensed.npars*sizeof(cl_float), NULL, &err);
        if(err != CL_SUCCESS)
            error("failed to create buffer for parameters");
        
        verbose("  create parameter kernel");
        
        // create kernel
        lensed.set_params = clCreateKernel(program, "set_params", &err);
        if(err != CL_SUCCESS)
            error("failed to create kernel for parameters");
        
        // set kernel arguments
        err = 0;
        err |= clSetKernelArg(lensed.set_params, 0, sizeof(cl_mem), &data_mem);
        err |= clSetKernelArg(lensed.set_params, 1, sizeof(cl_mem), &lensed.params);
        if(err != CL_SUCCESS)
            error("failed to set kernel arguments for parameters");
    }
    
    // create dumper
    {
        verbose("  create dumper");
        
        // buffer for dumper data
        lensed.dumper_mem = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, lensed.nd*sizeof(cl_float4), NULL, &err);
        if(err != CL_SUCCESS)
            error("failed to allocate dumper buffer");
        
        // dumper kernel 
        lensed.dumper = clCreateKernel(program, "dumper", &err);
        if(err != CL_SUCCESS)
            error("failed to create dumper kernel");
        
        // dumper kernel arguments
        err = 0;
        err |= clSetKernelArg(lensed.dumper, 0, sizeof(cl_mem), &data_mem);
        err |= clSetKernelArg(lensed.dumper, 1, sizeof(cl_mem), &indices);
        err |= clSetKernelArg(lensed.dumper, 2, sizeof(cl_ulong), &nq);
        err |= clSetKernelArg(lensed.dumper, 3, sizeof(cl_mem), &lensed.qq);
        err |= clSetKernelArg(lensed.dumper, 4, sizeof(cl_mem), &lensed.ww);
        err |= clSetKernelArg(lensed.dumper, 5, sizeof(cl_mem), &lensed.ee);
        err |= clSetKernelArg(lensed.dumper, 6, sizeof(cl_mem), &mean);
        err |= clSetKernelArg(lensed.dumper, 7, sizeof(cl_mem), &variance);
        err |= clSetKernelArg(lensed.dumper, 8, sizeof(cl_mem), &lensed.dumper_mem);
        if(err != CL_SUCCESS)
            error("failed to set dumper kernel arguments");
    }
    
    
    /***************
     * ready to go *
     ***************/
    
    info("find posterior");
    
    // create array for parameter wrap-around
    int* wrap = malloc(lensed.npars*sizeof(int));
    
    // collect wrap-around info from parameters
    {
        int* w = wrap;
        for(size_t i = 0; i < inp->nobjs; ++i)
            for(size_t j = 0; j < inp->objs[i].npars; ++j, ++w)
                *w = inp->objs[i].pars[j].wrap;
    }
    
    // gather MultiNest options
    int ndim = lensed.npars;
    int npar = ndim;
    int nclspar = ndim;
    double ztol = -1E90;
    char root[100] = {0};
    if(inp->opts->root)
        strncpy(root, inp->opts->root, 99);
    int initmpi = 1;
    double logzero = -DBL_MAX;
    
    // take start time
    time_t start = time(0);
    
    // run MultiNest
    run(inp->opts->ins, inp->opts->mmodal, inp->opts->ceff, inp->opts->nlive,
        inp->opts->tol, inp->opts->eff, ndim, npar, nclspar,
        inp->opts->maxmodes, inp->opts->updint, ztol, root, inp->opts->seed,
        wrap, inp->opts->fb, inp->opts->resume, inp->opts->output, initmpi,
        logzero, inp->opts->maxiter, loglike, dumper, &lensed);
    
    // take end time
    time_t end = time(0);
    
    // map output from device
    cl_float4* output = clEnqueueMapBuffer(lensed.queue, lensed.dumper_mem, CL_TRUE, CL_MAP_READ, 0, lensed.nd*sizeof(cl_float4), 0, NULL, NULL, &err);
    if(err != CL_SUCCESS)
        error("failed to map dumper buffer");
    
    // compute chi^2/dof
    double chi2_dof = 0;
    for(size_t i = 0; i < lensed.dat->size; ++i)
        chi2_dof += output[i].s[3];
    chi2_dof /= lensed.dat->size - lensed.npars;
    
    // unmap output
    clEnqueueUnmapMemObject(lensed.queue, lensed.dumper_mem, output, 0, NULL, NULL);
    
    // duration
    double dur = difftime(end, start);
    
    // output duration
    info("done in %02d:%02d:%02d",
         (int)(dur/3600),
         (int)(fmod(dur, 3600)/60),
         (int)fmod(dur, 60));
    
    // summary statistics
    info("summary");
    info("  ");
    info(LOG_BOLD "  log-evidence: " LOG_RESET "%.4f ± %.4f", inp->opts->ins ? lensed.logev_ins : lensed.logev, lensed.logev_err);
    info(LOG_BOLD "  max log-like: " LOG_RESET "%.4f", lensed.max_loglike);
    info(LOG_BOLD "  min chi²/dof: " LOG_RESET "%.4f", chi2_dof);
    info("  ");
    
    // parameter table
    info("parameters");
    info("  ");
    info(LOG_BOLD "  %-10s  %10s  %10s  %10s  %10s" LOG_RESET, "parameter", "mean", "sigma", "ML", "MAP");
    info("  ----------------------------------------------------------");
    for(size_t i = 0; i < lensed.npars; ++i)
        info("  %-10s  %10.4f  %10.4f  %10.4f  %10.4f", lensed.pars[i]->label ? lensed.pars[i]->label : lensed.pars[i]->id, lensed.mean[i], lensed.sigma[i], lensed.ml[i], lensed.map[i]);
    info("  ");
    
    // write parameter names and labels to file
    if(inp->opts->output)
    {
        FILE* file;
        char* name;
        
        name = malloc(strlen(inp->opts->root) + strlen(".paramnames") + 1);
        if(!name)
            errori(NULL);
        
        strcpy(name, inp->opts->root);
        strcat(name, ".paramnames");
        
        file = fopen(name, "w");
        if(!file)
            errori("could not write %s", name);
        
        for(size_t i = 0; i < inp->nobjs; ++i)
            for(size_t j = 0; j < inp->objs[i].npars; ++j)
                fprintf(file, "%-20s  %s\n", inp->objs[i].pars[j].id, inp->objs[i].pars[j].label ? inp->objs[i].pars[j].label : "");
        
        fclose(file);
        free(name);
    }
    
    // free MultiNest data
    free(wrap);
    
    // free dumper
    clReleaseKernel(lensed.dumper);
    clReleaseMemObject(lensed.dumper_mem);
    
    // free parameter space
    clReleaseMemObject(lensed.params);
    clReleaseKernel(lensed.set_params);
    
    // free kernel
    clReleaseKernel(lensed.kernel);
    
    // free object data buffer
    clReleaseMemObject(data_mem);
    
    // free points
    clReleaseMemObject(lensed.qq);
    clReleaseMemObject(lensed.ww);
    clReleaseMemObject(lensed.ee);
    
    // free data
    clReleaseMemObject(indices);
    clReleaseMemObject(mean);
    clReleaseMemObject(variance);
    
    // free worker
    clReleaseProgram(program);
    clReleaseCommandQueue(lensed.queue);
    clReleaseContext(context);
    
    // free results
    free((char*)lensed.fits);
    free(lensed.mean);
    free(lensed.sigma);
    free(lensed.ml);
    free(lensed.map);
    
    // free data
    free_data(lensed.dat);
    
    // free input
    free_input(inp);
    
    return EXIT_SUCCESS;
}
