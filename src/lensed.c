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
#include "prior.h"
#include "log.h"
#include "version.h"

// TODO: should be "NUL" on Windows
static const char NUL_DEV[] = "/dev/null";

#define BLOCK_SIZE 16

static void opencl_notify(const char* errinfo, const void* private_info,  size_t cb, void* user_data)
{
    verbose("%s", errinfo);
}

int main(int argc, char* argv[])
{
    // program data
    struct lensed lensed;
    
    // data
    size_t masked;
    cl_float* psf;
    size_t psfw;
    size_t psfh;
    
    // quadrature rule
    cl_ulong nq;
    cl_float2* qq;
    cl_float2* ww;
    
    // OpenCL error code
    cl_int err;
    
    // OpenCL structures
    cl_device_id device;
    cl_context context;
    cl_program program;
    
    // buffer for objects
    cl_mem object_mem;
    
    // buffers for quadrature rule
    cl_mem qq_mem;
    cl_mem ww_mem;
    
    // buffers for data
    cl_mem image_mem;
    cl_mem weight_mem;
    cl_mem psf_mem;
    
    // log file for capturing library output
    char* logfil;
    
    // timer for duration
    time_t start, end;
    double dur;
    
    // chi^2/dof value for maximum likelihood result
    double chi2_dof;
    
    
    /*********
     * input *
     *********/
    
    // read input
    input* inp = read_input(argc, argv);
    
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
    
    
    /*****************
     * special modes *
     *****************/
    
    // output header for batch mode
    if(inp->opts->batch_header)
    {
        // write fields row
        printf("%-60s", "summary");
        printf("%-*s", (int)(lensed.npars*12), "mean");
        printf("%-*s", (int)(lensed.npars*12), "sigma");
        printf("%-*s", (int)(lensed.npars*12), "ML");
        printf("%-*s", (int)(lensed.npars*12), "MAP");
        printf("\n");
        
        // write summary header
        printf("%-18s  ", "log-ev");
        printf("%-18s  ", "log-lh");
        printf("%-18s  ", "chi2/n");
        
        // write parameter headers
        for(size_t j = 0; j < 4; ++j)
            for(size_t i = 0; i < lensed.npars; ++i)
                printf("%-10s  ", lensed.pars[i]->label ? lensed.pars[i]->label : lensed.pars[i]->id);
        
        // batch file header is done
        printf("\n");
        exit(0);
    }
    
    
    /*****************
     * status output *
     *****************/
    
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
    
    
    /********
     * data *
     ********/
    
    verbose("data");
    
    // read input image
    read_image(inp->opts->image, &lensed.width, &lensed.height, &lensed.image);
    
    verbose("  image size: %zu x %zu", lensed.width, lensed.height);
    
    // total size of image
    lensed.size = lensed.width*lensed.height;
    
    verbose("  image pixels: %zu", lensed.size);
    
    // check if weight map is given
    if(inp->opts->weight)
    {
        // read weight map from file as it is
        read_weight(inp->opts->weight, lensed.width, lensed.height, &lensed.weight);
    }
    else
    {
        double* gain;
        
        // read gain if given, else make uniform gain map
        if(inp->opts->gain->file)
            read_gain(inp->opts->gain->file, lensed.width, lensed.height, &gain);
        else
            make_gain(inp->opts->gain->value, lensed.width, lensed.height, &gain);
        
        // make weight map from image, gain and offset
        make_weight(lensed.image, gain, inp->opts->offset, lensed.width, lensed.height, &lensed.weight);
        
        free(gain);
    }
    
    // start without masked pixels
    masked = 0;
    
    // apply mask if given
    if(inp->opts->mask)
    {
        int* mask;
        
        // read mask from file
        read_mask(inp->opts->mask, lensed.width, lensed.height, &mask);
        
        // mask individual pixels by setting their weight to zero
        for(size_t i = 0; i < lensed.size; ++i)
        {
            if(mask[i])
            {
                lensed.weight[i] = 0;
                masked += 1;
            }
        }
        
        // done with mask
        free(mask);
        
        verbose("  masked pixels: %zu", masked);
    }
    
    // load PSF if given
    if(inp->opts->psf)
    {
        // read psf
        read_psf(inp->opts->psf, &psfw, &psfh, &psf);
        
        verbose("  PSF size: %zu x %zu", psfw, psfh);
    }
    else
    {
        // no psf
        psf = NULL;
        psfw = 0;
        psfh = 0;
    }
    
    // check flat-fielding
    {
        double mode, fwhm;
        
        // get mode of pixel values
        find_mode(lensed.size, lensed.image, lensed.weight, &mode, &fwhm);
        
        verbose("  background: %f ± %f", mode, 0.5*fwhm);
        
        // check if mode contains zero
        if(fabs(mode) > 0.5*fwhm)
        {
            size_t i;
            
            // try to locate sky object
            for(i = 0; i < inp->nobjs; ++i)
                if(strcmp(inp->objs[i].name, "sky") == 0)
                    break;
            
            // warn if there is no sky
            if(i == inp->nobjs)
                warn("background sky is not zero\n"
                     "The mode of the image is far from zero. This might "
                     "indicate that the background sky was not removed from "
                     "the image. In this case, you should add a \"sky\" object "
                     "or the reconstruction might fail.");
        }
        else
        {
            // warn if there is no offset given
            if(!inp->opts->offset && !inp->opts->weight)
                warn("background sky is zero but no offset was given\n"
                     "The mode of the image is close to zero. This might "
                     "indicate that the background sky was removed from the "
                     "image. In this case, the \"offset\" option must be given "
                     "to get the correct pixel weights.");
        }
    }
    
    
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
    
    
    /*******************
     * quadrature rule *
     *******************/
    
    verbose("quadrature");
    
    // get the number of nodes of quadrature rule
    nq = quad_points();
    
    verbose("  number of points: %zu", nq);
    
    // allocate space for quadrature points and weights
    qq = malloc(nq*sizeof(cl_float2));
    ww = malloc(nq*sizeof(cl_float2));
    if(!qq || !ww)
        errori(NULL);
    
    // get quadrature rule
    quad_rule(qq, ww);
    
    
    /****************
     * kernel setup *
     ****************/
    
    verbose("kernel");
    
    {
        char device_name[128];
        char device_vendor[128];
        char device_version[128];
        char device_compiler[128];
        char driver_version[128];
        
        verbose("  device: %s", inp->opts->gpu ? "GPU" : "CPU");
        
        err = clGetDeviceIDs(NULL, inp->opts->gpu ? CL_DEVICE_TYPE_GPU : CL_DEVICE_TYPE_CPU, 1, &device, NULL);
        if(err != CL_SUCCESS)
            error("failed to get device");
        
        // query device name
        err = clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
        if(err != CL_SUCCESS)
            error("failed to get device name");
        
        verbose("  device name: %s", device_name);
        
        // query device vendor
        err = clGetDeviceInfo(device, CL_DEVICE_VENDOR, sizeof(device_vendor), device_vendor, NULL);
        if(err != CL_SUCCESS)
            error("failed to get device vendor");
        
        verbose("  device vendor: %s", device_vendor);
        
        // query device version
        err = clGetDeviceInfo(device, CL_DEVICE_VERSION, sizeof(device_version), device_version, NULL);
        if(err != CL_SUCCESS)
            error("failed to get device version");
        
        verbose("  device version: %s", device_version);
        
        // query device compiler
        err = clGetDeviceInfo(device, CL_DEVICE_OPENCL_C_VERSION, sizeof(device_compiler), device_compiler, NULL);
        if(err != CL_SUCCESS)
            error("failed to get device compiler");
        
        verbose("  device compiler: %s", device_compiler);
        
        // query driver version
        err = clGetDeviceInfo(device, CL_DRIVER_VERSION, sizeof(driver_version), driver_version, NULL);
        if(err != CL_SUCCESS)
            error("failed to get driver version");
        
        verbose("  driver version: %s", driver_version);
        
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
        if(err != CL_SUCCESS)
            error("failed to create program");
        
        // flags for building, zero-terminated
        const char* build_flags[] = {
            "-cl-denorms-are-zero",
            "-cl-strict-aliasing",
            "-cl-mad-enable",
            "-cl-no-signed-zeros",
            "-cl-fast-relaxed-math",
            NULL
        };
        
        // make build options string
        const char* build_options = kernel_options(lensed.width, lensed.height, !!psf, psfw, psfh, nq, build_flags);
        
        // and build program
        verbose("  build program");
        err = clBuildProgram(program, 1, &device, build_options, NULL, NULL);
        if(err != CL_SUCCESS)
            error("failed to build program%s", LOG_LEVEL > LOG_VERBOSE ? " (use --verbose to see build log)" : "");
        
        // free program codes
        for(int i = 0; i < nkernels; ++i)
            free((void*)kernels[i]);
        free(kernels);
        
        // free build options
        free((char*)build_options);
    }
    
    // set up work-groups
    {
        // local work size is size of a block of pixels processed together
        lensed.lws[0] = BLOCK_SIZE;
        lensed.lws[1] = BLOCK_SIZE;
        
        // global work size must be padded to block size
        lensed.gws[0] = lensed.width + (lensed.lws[0] - lensed.width%lensed.lws[0])%lensed.lws[0];
        lensed.gws[1] = lensed.height + (lensed.lws[1] - lensed.height%lensed.lws[1])%lensed.lws[1];
        
        verbose("  block size: %zu x %zu", lensed.lws[0], lensed.lws[1]);
        verbose("  work size: %zu x %zu", lensed.gws[0], lensed.gws[1]);
    }
    
    // allocate device memory for data
    {
        verbose("  create data buffers");
        
        image_mem = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS, lensed.size*sizeof(cl_float), lensed.image, NULL);
        weight_mem = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS, lensed.size*sizeof(cl_float), lensed.weight, NULL);
        if(psf)
            psf_mem = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS, lensed.size*sizeof(cl_float), psf, &err);
        if(!image_mem || !weight_mem || err)
            error("failed to allocate data buffers");
    }
    
    // create buffers for quadrature rule
    {
        verbose("  create quadrature buffers");
        
        // allocate buffers for quadrature rule
        qq_mem = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS, nq*sizeof(cl_float2), qq, NULL);
        ww_mem = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS, nq*sizeof(cl_float2), ww, NULL);
        if(!qq_mem || !ww_mem)
            error("failed to allocate quadrature buffers");
    }
    
    // create buffer that contains object data
    {
        // collect total size of object data
        size_t object_size = 0;
        for(size_t i = 0; i < inp->nobjs; ++i)
            object_size += inp->objs[i].size;
        
        verbose("  create object buffer");
        
        // allocate buffer for object data
        object_mem = clCreateBuffer(context, CL_MEM_READ_WRITE, object_size, NULL, &err);
        if(err != CL_SUCCESS)
            error("failed to create object buffer");
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
        err |= clSetKernelArg(lensed.set_params, 0, sizeof(cl_mem), &object_mem);
        err |= clSetKernelArg(lensed.set_params, 1, sizeof(cl_mem), &lensed.params);
        if(err != CL_SUCCESS)
            error("failed to set kernel arguments for parameters");
    }
    
    // create kernel
    {
        verbose("  create render buffer");
        
        lensed.value_mem = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR | CL_MEM_HOST_READ_ONLY, lensed.size*sizeof(cl_float), NULL, NULL);
        lensed.error_mem = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR | CL_MEM_HOST_READ_ONLY, lensed.size*sizeof(cl_float), NULL, NULL);
        if(!lensed.value_mem || !lensed.error_mem)
            error("failed to create render buffer");
        
        verbose("  create render kernel");
        
        // render kernel 
        lensed.render = clCreateKernel(program, "render", &err);
        if(err != CL_SUCCESS)
            error("failed to create render kernel");
        err = 0;
        err |= clSetKernelArg(lensed.render, 0, sizeof(cl_mem), &object_mem);
        err |= clSetKernelArg(lensed.render, 1, sizeof(cl_mem), &qq_mem);
        err |= clSetKernelArg(lensed.render, 2, sizeof(cl_mem), &ww_mem);
        err |= clSetKernelArg(lensed.render, 3, sizeof(cl_mem), &lensed.value_mem);
        err |= clSetKernelArg(lensed.render, 4, sizeof(cl_mem), &lensed.error_mem);
        if(err != CL_SUCCESS)
            error("failed to set render kernel arguments");
        
        // convolution kernel if there is a PSF
        if(psf)
        {
            size_t cache_size;
            
            verbose("  create convolve buffer");
            
            lensed.convolve_mem = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR | CL_MEM_HOST_READ_ONLY, lensed.size*sizeof(cl_float), NULL, &err);
            if(err != CL_SUCCESS)
                error("failed to create convolve buffer");
            
            verbose("  create convolve kernel");
            
            // size of local memory that stores part of the model
            cache_size = (psfw/2 + lensed.lws[0] + psfw/2)*(psfh/2 + lensed.lws[1] + psfh/2);
            
            // convolve kernel 
            lensed.convolve = clCreateKernel(program, "convolve", &err);
            if(err != CL_SUCCESS)
                error("failed to create convolve kernel");
            err = 0;
            err |= clSetKernelArg(lensed.convolve, 0, sizeof(cl_mem), &lensed.value_mem);
            err |= clSetKernelArg(lensed.convolve, 1, sizeof(cl_mem), &psf_mem);
            err |= clSetKernelArg(lensed.convolve, 2, cache_size*sizeof(cl_float), NULL);
            err |= clSetKernelArg(lensed.convolve, 3, psfw*psfh*sizeof(cl_float), NULL);
            err |= clSetKernelArg(lensed.convolve, 4, sizeof(cl_mem), &lensed.convolve_mem);
            if(err != CL_SUCCESS)
                error("failed to set convolve kernel arguments");
        }
        else
        {
            // no kernel: used to determine whether to convolve
            lensed.convolve = 0;
        }
        
        verbose("  create loglike buffer");
        
        lensed.loglike_mem = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR | CL_MEM_HOST_READ_ONLY, lensed.size*sizeof(cl_float), NULL, &err);
        if(err != CL_SUCCESS)
            error("failed to create loglike buffer");
        
        verbose("  create loglike kernel");
        
        // loglike kernel, take care: the buffer it works on depends on PSF
        lensed.loglike = clCreateKernel(program, "loglike", &err);
        if(err != CL_SUCCESS)
            error("failed to create loglike kernel");
        err = 0;
        err |= clSetKernelArg(lensed.loglike, 0, sizeof(cl_mem), &image_mem);
        err |= clSetKernelArg(lensed.loglike, 1, sizeof(cl_mem), &weight_mem);
        err |= clSetKernelArg(lensed.loglike, 2, sizeof(cl_mem), psf ? &lensed.convolve_mem : &lensed.value_mem);
        err |= clSetKernelArg(lensed.loglike, 3, sizeof(cl_mem), &lensed.loglike_mem);
        if(err != CL_SUCCESS)
            error("failed to set loglike kernel arguments");
    }
    
    
    /***************
     * ready to go *
     ***************/
    
    info("find posterior");
    
    // name of log file
    if(inp->opts->output)
    {
        logfil = malloc(strlen(inp->opts->root) + strlen("log.txt") + 1);
        if(!logfil)
            errori(NULL);
        sprintf(logfil, "%slog.txt", inp->opts->root);
    }
    else
    {
        logfil = malloc(sizeof(NUL_DEV));
        if(!logfil)
            errori(NULL);
        strcpy(logfil, NUL_DEV);
    }
    
    // take start time
    start = time(0);
    
    // call MultiNest
    {
        // MultiNest options
        int ndim = lensed.npars;
        int npar = ndim;
        int nclspar = ndim;
        double ztol = -1E90;
        char root[100] = {0};
        int initmpi = 1;
        int fb = (LOG_LEVEL <= LOG_VERBOSE);
        double logzero = -DBL_MAX;
        int* wrap;
        
        // copy root element for file output if given
        if(inp->opts->root)
            strncpy(root, inp->opts->root, 99);
        
        // create array for parameter wrap-around
        wrap = malloc(lensed.npars*sizeof(int));
        if(!wrap)
            errori(NULL);
        for(size_t i = 0; i < lensed.npars; ++i)
            wrap[i] = lensed.pars[i]->wrap;
        
        // redirect MultiNest's output to log file
        logfile(logfil);
        
        // run MultiNest
        run(inp->opts->ins, inp->opts->mmodal, inp->opts->ceff,
            inp->opts->nlive, inp->opts->tol, inp->opts->eff,
            ndim, npar, nclspar, inp->opts->maxmodes, inp->opts->updint,
            ztol, root, inp->opts->seed, wrap, fb, inp->opts->resume,
            inp->opts->output, initmpi, logzero, inp->opts->maxiter,
            loglike, dumper, &lensed);
        
        // restore standard output
        logfile(NULL);
        
        // free MultiNest data
        free(wrap);
    }
    
    // take end time
    end = time(0);
    
    
    /***********
     * results *
     ***********/
    
    // compute chi^2/dof
    chi2_dof = -2*lensed.max_loglike / (lensed.size - masked - lensed.npars);
    
    // duration
    dur = difftime(end, start);
    info("done in %02d:%02d:%02d", (int)(dur/3600), (int)(fmod(dur, 3600)/60), (int)fmod(dur, 60));
    
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
    
    // write parameter names, labels and ranges to file
    if(inp->opts->output)
    {
        FILE* paramfile;
        FILE* rangefile;
        char* paramname;
        char* rangename;
        
        paramname = malloc(strlen(inp->opts->root) + strlen(".paramnames") + 1);
        rangename = malloc(strlen(inp->opts->root) + strlen(".ranges") + 1);
        if(!paramname || !rangename)
            errori(NULL);
        
        strcpy(paramname, inp->opts->root);
        strcat(paramname, ".paramnames");
        strcpy(rangename, inp->opts->root);
        strcat(rangename, ".ranges");
        
        paramfile = fopen(paramname, "w");
        rangefile = fopen(rangename, "w");
        if(!paramfile)
            errori("could not write %s", paramname);
        if(!rangefile)
            errori("could not write %s", rangename);
        
        for(size_t i = 0; i < inp->nobjs; ++i)
        {
            for(size_t j = 0; j < inp->objs[i].npars; ++j)
            {
                // output parameter id and, if set, label
                fprintf(paramfile, "%-20s  %s\n", inp->objs[i].pars[j].id, inp->objs[i].pars[j].label ? inp->objs[i].pars[j].label : "");
                
                // output parameter range
                fprintf(rangefile, "%-20s  %10.4f  %10.4f\n", inp->objs[i].pars[j].id, prior_lower(inp->objs[i].pars[j].pri), prior_upper(inp->objs[i].pars[j].pri));
            }
        }
        
        fclose(paramfile);
        fclose(rangefile);
        free(paramname);
        free(rangename);
    }
    
    // batch output
    if(LOG_LEVEL == LOG_BATCH)
    {
        // write summary results
        printf("%-18.4f  ", inp->opts->ins ? lensed.logev_ins : lensed.logev);
        printf("%-18.4f  ", lensed.max_loglike);
        printf("%-18.4f  ", chi2_dof);
        
        // write parameter results
        for(size_t i = 0; i < lensed.npars; ++i)
            printf("%-10.4f  ", lensed.mean[i]);
        for(size_t i = 0; i < lensed.npars; ++i)
            printf("%-10.4f  ", lensed.sigma[i]);
        for(size_t i = 0; i < lensed.npars; ++i)
            printf("%-10.4f  ", lensed.ml[i]);
        for(size_t i = 0; i < lensed.npars; ++i)
            printf("%-10.4f  ", lensed.map[i]);
        
        // output is done
        printf("\n");
    }
    
    // free kernel
    clReleaseKernel(lensed.render);
    clReleaseMemObject(lensed.value_mem);
    clReleaseMemObject(lensed.error_mem);
    if(psf)
    {
        clReleaseKernel(lensed.convolve);
        clReleaseMemObject(lensed.convolve_mem);
    }
    clReleaseKernel(lensed.loglike);
    clReleaseMemObject(lensed.loglike_mem);
    
    // free parameter space
    clReleaseMemObject(lensed.params);
    clReleaseKernel(lensed.set_params);
    
    // free object buffer
    clReleaseMemObject(object_mem);
    
    // free quadrature buffers
    clReleaseMemObject(qq_mem);
    clReleaseMemObject(ww_mem);
    
    // free data
    clReleaseMemObject(image_mem);
    clReleaseMemObject(weight_mem);
    if(psf)
        clReleaseMemObject(psf_mem);
    
    // free worker
    clReleaseProgram(program);
    clReleaseCommandQueue(lensed.queue);
    clReleaseContext(context);
    
    // free quadrature rule
    free(qq);
    free(ww);
    
    // free results
    free((char*)lensed.fits);
    free(lensed.mean);
    free(lensed.sigma);
    free(lensed.ml);
    free(lensed.map);
    
    // free data
    free(lensed.image);
    free(lensed.weight);
    free(psf);
    
    // free input
    free_input(inp);
    
    // there might be output left in Fortran's buffer, so redirect again
    // to log file, which is not closed on purpose
    logfile(logfil);
    free(logfil);
    
    return EXIT_SUCCESS;
}
