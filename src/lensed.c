#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <time.h>
#include <signal.h>
#include <setjmp.h>

#ifdef LENSED_XPA
#include "xpa.h"
#endif

#include "multinest.h"

#include "opencl.h"
#include "input.h"
#include "data.h"
#include "profile.h"
#include "lensed.h"
#include "kernel.h"
#include "nested.h"
#include "quadrature.h"
#include "prior.h"
#include "log.h"
#include "path.h"
#include "version.h"

// jump buffer to exit run
static jmp_buf jmp;

// signal handler for keyboard interrupts
static void handler(int sig)
{
    longjmp(jmp, sig);
}

int main(int argc, char* argv[])
{
    // program data
    struct lensed* lensed;
    
    // data
    pcsdata* pcs;
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
    lensed_cl* lcl;
    cl_command_queue_properties queue_properties;
    cl_program program;
    
    // OpenCL device info
    unsigned opencl_version;
    cl_uint work_item_dims;
    size_t* work_item_sizes;
    cl_ulong local_mem_size;
    
    // buffer for objects
    cl_ulong object_size;
    cl_mem object_mem;
    
    // buffers for quadrature rule
    cl_mem qq_mem;
    cl_mem ww_mem;
    
    // buffers for data
    cl_mem image_mem;
    cl_mem weight_mem;
    cl_mem psf_mem;
    
    // timer for duration
    time_t start, end;
    double dur;
    
    // chi^2/dof value for maximum likelihood result
    double chi2_dof;
    
    
    /******************
     * initialisation *
     ******************/
    
    // initialise the path to Lensed
    init_lensed_path();
    
    // create the lensed struct
    lensed = malloc(sizeof(struct lensed));
    if(!lensed)
        errori(NULL);
    
    
    /*********
     * input *
     *********/
    
    // read input
    input* inp = read_input(argc, argv);
    
    // parameter initialisation
    {
        // number of derived parameters
        size_t nderi;
        
        // temporary list of derived parameters
        size_t* derived;
        
        // sum number of parameters
        lensed->npars = 0;
        for(size_t i = 0; i < inp->nobjs; ++i)
            lensed->npars += inp->objs[i].npars;
        
        // list of all parameters
        lensed->pars = malloc(lensed->npars*sizeof(param*));
        if(!lensed->pars)
            errori(NULL);
        
        // parameter map; MultiNest keeps derived parameters at the end
        lensed->pmap = malloc(lensed->npars*sizeof(size_t));
        if(!lensed->pmap)
            errori(NULL);
        
        // space for list of derived parameters
        derived = malloc(lensed->npars*sizeof(size_t));
        if(!derived)
            errori(NULL);
        
        // start with no dimensions and no derived parameters
        lensed->ndims = 0;
        nderi = 0;
        
        // collect and check all parameters
        for(size_t i = 0, p = 0; i < inp->nobjs; ++i)
        {
            // current object
            object* obj = &inp->objs[i];
            
            for(size_t j = 0; j < inp->objs[i].npars; ++j, ++p)
            {
                // current parameter
                param* par = &obj->pars[j];
                
                // apply default bounds if none provided
                if(!par->lower && !par->upper)
                {
                    switch(par->type)
                    {
                    case PAR_RADIUS:
                        par->lower = 0;
                        par->upper = HUGE_VAL;
                        break;
                    
                    case PAR_AXIS_RATIO:
                        par->lower = 0;
                        par->upper = 1;
                        break;
                    
                    default:
                        break;
                    }
                }
                
                // mark if parameter is bounded
                par->bounded = (par->lower || par->upper);
                
                // check that prior is compatible with bounds
                if(par->bounded && (prior_lower(par->pri) < par->lower || prior_upper(par->pri) > par->upper))
                {
                    // check if there is overlap between the bounds
                    if(prior_lower(par->pri) >= par->upper || prior_upper(par->pri) <= par->lower)
                    {
                        // prior falls completely outside of allowed range
                        error("%s: prior does not include parameter "
                              "bounds [%g, %g]\n"
                              "The prior does not include the allowed range "
                              "of values for the parameter. It is impossible "
                              "to draw a valid parameter value. Please fix "
                              "the prior to include the range of values "
                              "indicated above.",
                              par->id, par->lower, par->upper);
                    }
                    else
                    {
                        // prior overlaps parameter bounds, issue a warning
                        warn("%s: prior partially outside parameter "
                             "bounds [%g, %g]\n"
                             "Part of the prior lies outside of the allowed "
                             "range of parameter values. Values will be drawn "
                             "from the prior until a valid one is found. If "
                             "the bounds of prior and parameter do not "
                             "overlap significantly, this can be slow. You "
                             "might want to change the prior accordingly.",
                             par->id, par->lower, par->upper);
                    }
                }
                
                // check image plane priors
                if(par->ipp)
                {
                    // not all parameters can have image plane priors
                    switch(par->type)
                    {
                        // check that X is followed by IPP Y
                        case PAR_POSITION_X:
                            if(j + 1 == obj->npars || obj->pars[j + 1].type != PAR_POSITION_Y)
                                error("object `%s`: image plane prior "
                                      "requires pair (X,Y) of parameters",
                                      obj->id);
                            if(!obj->pars[j + 1].ipp)
                                error("object `%s`: pair (X,Y) must have "
                                      "image plane priors",
                                      obj->id);
                            break;
                        
                        // check that Y is preceded by IPP X
                        case PAR_POSITION_Y:
                            if(j == 0 || obj->pars[j - 1].type != PAR_POSITION_X)
                                error("object `%s`: image plane prior "
                                      "requires pair (X,Y) of parameters",
                                      obj->id);
                            if(!obj->pars[j - 1].ipp)
                                error("object `%s`: pair (X,Y) must have "
                                      "image plane priors",
                                      obj->id);
                            break;
                        
                        // all other parameters cannot have image plane priors
                        default:
                            error("prior `%s`: cannot give image plane prior "
                                  "for this parameter",
                                  par->id);
                    }
                }
                
                // store parameter
                lensed->pars[p] = par;
                
                // mark parameter as derived if it has a pseudo-prior
                par->derived = prior_pseudo(par->pri);
                
                // collect derived parameters for end of list
                if(par->derived)
                {
                    // add to derived list
                    derived[nderi] = p;
                    nderi += 1;
                }
                else
                {
                    // add to parameter list
                    lensed->pmap[lensed->ndims] = p;
                    
                    // add dimension
                    lensed->ndims += 1;
                }
            }
        }
        
        // add derived parameters to end of list
        for(size_t i = 0; i < nderi; ++i)
            lensed->pmap[lensed->ndims + i] = derived[i];
        
        // done with list of derived parameters
        free(derived);
    }
    
    
    /*****************
     * special modes *
     *****************/
    
    // list devices
    if(inp->opts->devices)
    {
        char device_name[128];
        char device_vendor[128];
        char device_version[128];
        char device_compiler[128];
        char driver_version[128];
        char platform_name[128];
        cl_uint compute_units;
        
        printf("\n");
        
        // go through devices and show info
        for(lensed_device* d = get_lensed_devices(); d->device_id; ++d)
        {
            // show device
            printf("device: %s\n", d->name);
            
            // query device name
            err = clGetDeviceInfo(d->device_id, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
            printf("  name:     %s\n", err == CL_SUCCESS ? device_name : "(unknown)");
            
            // query device vendor
            err = clGetDeviceInfo(d->device_id, CL_DEVICE_VENDOR, sizeof(device_vendor), device_vendor, NULL);
            printf("  vendor:   %s\n", err == CL_SUCCESS ? device_vendor : "(unknown)");
            
            // query device version
            err = clGetDeviceInfo(d->device_id, CL_DEVICE_VERSION, sizeof(device_version), device_version, NULL);
            printf("  version:  %s\n", err == CL_SUCCESS ? device_version : "(unknown)");
            
            // query device compiler
            err = clGetDeviceInfo(d->device_id, CL_DEVICE_OPENCL_C_VERSION, sizeof(device_compiler), device_compiler, NULL);
            printf("  compiler: %s\n", err == CL_SUCCESS ? device_compiler : "(unknown)");
            
            // query driver version
            err = clGetDeviceInfo(d->device_id, CL_DRIVER_VERSION, sizeof(driver_version), driver_version, NULL);
            printf("  driver:   %s\n", err == CL_SUCCESS ? driver_version : "(unknown)");
            
            // query platform name
            err = clGetPlatformInfo(d->platform_id, CL_PLATFORM_NAME, sizeof(platform_name), platform_name, NULL);
            printf("  platform: %s\n", err == CL_SUCCESS ? platform_name : "(unknown)");
            
            // query maximum compute units
            err = clGetDeviceInfo(d->device_id, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(compute_units), &compute_units, NULL);
            printf("  units:    ");
            if(err == CL_SUCCESS)
                printf("%u\n", compute_units);
            else
                printf("%s\n", "(unknown)");
            
            printf("\n");
        }
        
        exit(0);
    }
    
    // output header for batch mode
    if(inp->opts->batch_header)
    {
        // write fields row
        printf("%-60s", "summary");
        printf("%-*s", (int)(lensed->npars*12), "mean");
        printf("%-*s", (int)(lensed->npars*12), "sigma");
        printf("%-*s", (int)(lensed->npars*12), "ML");
        printf("%-*s", (int)(lensed->npars*12), "MAP");
        printf("\n");
        
        // write summary header
        printf("%-18s  ", "log-ev");
        printf("%-18s  ", "log-lh");
        printf("%-18s  ", "chi2/n");
        
        // write parameter headers
        for(size_t j = 0; j < 4; ++j)
            for(size_t i = 0; i < lensed->npars; ++i)
                printf("%-10s  ", lensed->pars[i]->label ? lensed->pars[i]->label : lensed->pars[i]->id);
        
        // batch file header is done
        printf("\n");
        exit(0);
    }
    
    // list known quadrature rules and exit
    if(inp->opts->show_rules)
    {
        int len = 0;
        for(int i = 0; QUAD_RULES[i].name; ++i)
        {
            int l = strlen(QUAD_RULES[i].name);
            if(l > len)
                len = l;
        }
        for(int i = 0; QUAD_RULES[i].name; ++i)
        {
            int sel = (strcmp(inp->opts->rule, QUAD_RULES[i].name) == 0);
            printf("%c %-*s  %s\n", sel ? '*' : ' ',
                   len, QUAD_RULES[i].name, QUAD_RULES[i].info);
        }
        exit(0);
    }
    
    
    /*****************
     * status output *
     *****************/
    
    // print banner
    info(LOG_BOLD "  _                             _ " LOG_DARK " ___" LOG_RESET);
    info(LOG_BOLD " | |                           | |" LOG_DARK "/   \\" LOG_RESET);
    info(LOG_BOLD " | |     ___ _ __  ___  ___  __| |" LOG_DARK "  A  \\" LOG_RESET "  " LOG_BOLD "Lensed" LOG_RESET " %s", LENSED_VERSION);
    info(LOG_BOLD " | |    / _ \\ '_ \\/ __|/ _ \\/ _` |" LOG_DARK " < > |" LOG_RESET);
    info(LOG_BOLD " | |___|  __/ | | \\__ \\  __/ (_| |" LOG_DARK "  V  /" LOG_RESET "  " LOG_DARK "http://glenco.github.io/lensed/" LOG_RESET);
    info(LOG_BOLD " |______\\___|_| |_|___/\\___|\\__,_|" LOG_DARK "\\___/ " LOG_RESET);
    info(LOG_BOLD "                                  " LOG_RESET);
    
    // print input
    print_input(inp);
    
    
    /********
     * data *
     ********/
    
    verbose("data");
    
    // read input image
    read_image(inp->opts->image, &lensed->width, &lensed->height, &lensed->image);
    
    verbose("  image size: %zu x %zu", lensed->width, lensed->height);
    
    // total size of image
    lensed->size = lensed->width*lensed->height;
    
    verbose("  image pixels: %zu", lensed->size);
    
    // read image pixel coordinate system
    pcs = malloc(sizeof(pcsdata));
    if(!pcs)
        errori(NULL);
    read_pcs(inp->opts->image, pcs);
    
    verbose("  pixel origin: ( %ld, %ld )", pcs->rx, pcs->ry);
    verbose("  pixel scale: ( %f, %f )", pcs->sx, pcs->sy);
    
    // check if weight map is given
    if(inp->opts->weight)
    {
        // read weight map from file as it is
        read_weight(inp->opts->weight, lensed->width, lensed->height, &lensed->weight);
    }
    else
    {
        double* gain;
        
        // read gain if given, else make uniform gain map
        if(inp->opts->gain->file)
            read_gain(inp->opts->gain->file, lensed->width, lensed->height, &gain);
        else
            make_gain(inp->opts->gain->value, lensed->width, lensed->height, &gain);
        
        // make weight map from image, gain and offset
        make_weight(lensed->image, gain, inp->opts->offset, lensed->width, lensed->height, &lensed->weight);
        
        free(gain);
    }
    
    // apply extra weights if given
    if(inp->opts->xweight)
    {
        double* xweight;
        
        // read extra weight map from file
        read_xweight(inp->opts->xweight, lensed->width, lensed->height, &xweight);
        
        // multiply extra weights
        for(size_t i = 0; i < lensed->size; ++i)
            lensed->weight[i] *= xweight[i];
        
        // done with extra weights
        free(xweight);
    }
    
    // apply mask if given
    if(inp->opts->mask)
    {
        int* mask;
        
        // read mask from file
        read_mask(inp->opts->mask, lensed->width, lensed->height, &mask);
        
        // mask individual pixels by setting their weight to zero
        for(size_t i = 0; i < lensed->size; ++i)
            if(mask[i])
                lensed->weight[i] = 0;
        
        // done with mask
        free(mask);
    }
    
    // count masked pixels
    masked = 0;
    for(size_t i = 0; i < lensed->size; ++i)
        if(lensed->weight[i] == 0)
            masked += 1;
    
    if(masked > 0)
        verbose("  masked pixels: %zu", masked);
    
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
        find_mode(lensed->size, lensed->image, lensed->weight, &mode, &fwhm);
        
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
        lensed->fits = fits;
    }
    else
    {
        // no output
        lensed->fits = NULL;
    }
    
    // MultiNest tolerance
    lensed->tol = inp->opts->tol;
    
    // arrays for parameters
    lensed->mean  = calloc(lensed->npars, sizeof(double));
    lensed->sigma = calloc(lensed->npars, sizeof(double));
    lensed->ml    = calloc(lensed->npars, sizeof(double));
    lensed->map   = calloc(lensed->npars, sizeof(double));
    if(!lensed->mean || !lensed->sigma || !lensed->ml || !lensed->map)
        errori(NULL);
    
    
    /*******************
     * quadrature rule *
     *******************/
    
    verbose("quadrature");
    
    {
        // chosen quadrature rule
        int rule;
        
        // find quadrature rule from options
        for(rule = 0; QUAD_RULES[rule].name; ++rule)
            if(strcmp(inp->opts->rule, QUAD_RULES[rule].name) == 0)
                break;
        
        // make sure rule is valid
        if(!QUAD_RULES[rule].name)
            error("invalid quadrature rule: %s (see `lensed --rules` for a list)",
                  inp->opts->rule);
        
        // get the number of nodes of quadrature rule
        nq = QUAD_RULES[rule].size;
        
        verbose("  quadrature rule: %s", QUAD_RULES[rule].name);
        verbose("  number of points: %zu", nq);
        
        // allocate space for quadrature points and weights
        qq = malloc(nq*sizeof(cl_float2));
        ww = malloc(nq*sizeof(cl_float2));
        if(!qq || !ww)
            errori(NULL);
        
        // get quadrature rule
        quad_rule(rule, qq, ww, pcs->sx, pcs->sy);
    }
    
    /****************
     * kernel setup *
     ****************/
    
    verbose("kernel");
    
    {
        // get the OpenCL environment
        lcl = get_lensed_cl(inp->opts->device);
        
        // output device info
        if(LOG_LEVEL <= LOG_VERBOSE)
        {
            cl_device_type device_type;
            char device_name[128];
            char device_vendor[128];
            char device_version[128];
            char device_compiler[128];
            char driver_version[128];
            cl_uint compute_units;
            
            // query device name
            err = clGetDeviceInfo(lcl->device_id, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
            verbose("  device: %s", err == CL_SUCCESS ? device_name : "(unknown)");
            
            // query device type
            err = clGetDeviceInfo(lcl->device_id, CL_DEVICE_TYPE, sizeof(device_type), &device_type, NULL);
            verbose("    type: %s", device_type == CL_DEVICE_TYPE_CPU ? "CPU" : (device_type == CL_DEVICE_TYPE_GPU ? "GPU" : "(unknown)"));
            
            // query device vendor
            err = clGetDeviceInfo(lcl->device_id, CL_DEVICE_VENDOR, sizeof(device_vendor), device_vendor, NULL);
            verbose("    vendor: %s", err == CL_SUCCESS ? device_vendor : "(unknown)");
            
            // query device version
            err = clGetDeviceInfo(lcl->device_id, CL_DEVICE_VERSION, sizeof(device_version), device_version, NULL);
            verbose("    version: %s", err == CL_SUCCESS ? device_version : "(unknown)");
            
            // query device compiler
            err = clGetDeviceInfo(lcl->device_id, CL_DEVICE_OPENCL_C_VERSION, sizeof(device_compiler), device_compiler, NULL);
            verbose("    compiler: %s", err == CL_SUCCESS ? device_compiler : "(unknown)");
            
            // query driver version
            err = clGetDeviceInfo(lcl->device_id, CL_DRIVER_VERSION, sizeof(driver_version), driver_version, NULL);
            verbose("    driver: %s", err == CL_SUCCESS ? driver_version : "(unknown)");
            
            // query maximum compute units
            err = clGetDeviceInfo(lcl->device_id, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(compute_units), &compute_units, NULL);
            verbose("    units: %u", err == CL_SUCCESS ? compute_units : 0);
        }
        
        queue_properties = 0;
        if(inp->opts->profile)
            queue_properties |= CL_QUEUE_PROFILING_ENABLE;
        
        lensed->queue = clCreateCommandQueue(lcl->context, lcl->device_id, queue_properties, &err);
        if(!lensed->queue || err != CL_SUCCESS)
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
        program = clCreateProgramWithSource(lcl->context, nkernels, kernels, NULL, &err);
        if(err != CL_SUCCESS)
            error("failed to create program");
        
        // flags for building, zero-terminated
        const char* build_flags[] = {
            "-cl-denorms-are-zero",
            "-cl-strict-aliasing",
            "-cl-fast-relaxed-math",
            NULL
        };
        
        // make build options string
        const char* build_options = kernel_options(lensed->width, lensed->height, !!psf, psfw, psfh, nq, build_flags);
        
        // and build program
        verbose("  build program");
        err = clBuildProgram(program, 1, &lcl->device_id, build_options, NULL, NULL);
// build log is reported in the notifications on Apple's implementation
#ifndef __APPLE__
        if(LOG_LEVEL <= LOG_VERBOSE)
        {
            char log[4096];
            clGetProgramBuildInfo(program, lcl->device_id, CL_PROGRAM_BUILD_LOG, sizeof(log), log, NULL);
            if(strlen(log) > 0)
                verbose("  build log: %s", log);
        }
#endif
        if(err != CL_SUCCESS)
            error("failed to build program%s", LOG_LEVEL > LOG_VERBOSE ? " (use --verbose to see build log)" : "");
        
        // free program codes
        for(int i = 0; i < nkernels; ++i)
            free((void*)kernels[i]);
        free(kernels);
        
        // free build options
        free((char*)build_options);
    }
    
    // gather device info
    {
        char version_string[128];
        unsigned major, minor;
        
        // get version string
        err = clGetDeviceInfo(lcl->device_id, CL_DEVICE_VERSION, sizeof(version_string), version_string, NULL);
        if(err != CL_SUCCESS || sscanf(version_string, "OpenCL %u.%u", &major, &minor) != 2)
            opencl_version = 100;
        else
            opencl_version = 100*major + 10*minor;
        
        // get number of work item dimensions for device
        err = clGetDeviceInfo(lcl->device_id, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(work_item_dims), &work_item_dims, NULL);
        if(err != CL_SUCCESS)
            error("failed to get maximum work item dimensions");
        
        // allocate space for work item sizes
        work_item_sizes = malloc(work_item_dims*sizeof(size_t));
        if(!work_item_sizes)
            errori(NULL);
        
        // get maximum work group size supported by device
        err = clGetDeviceInfo(lcl->device_id, CL_DEVICE_MAX_WORK_ITEM_SIZES, work_item_dims*sizeof(size_t), work_item_sizes, NULL);
        if(err != CL_SUCCESS)
            error("failed to get maximum work item sizes");
        
        // get size of local memory
        err = clGetDeviceInfo(lcl->device_id, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(local_mem_size), &local_mem_size, NULL);
        if(err != CL_SUCCESS)
            error("failed to get local memory size");
    }
    
    // allocate device memory for data
    {
        verbose("  create data buffers");
        
        image_mem = clCreateBuffer(lcl->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS, lensed->size*sizeof(cl_float), lensed->image, NULL);
        weight_mem = clCreateBuffer(lcl->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS, lensed->size*sizeof(cl_float), lensed->weight, NULL);
        if(psf)
            psf_mem = clCreateBuffer(lcl->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS, psfw*psfh*sizeof(cl_float), psf, &err);
        if(!image_mem || !weight_mem || err)
            error("failed to allocate data buffers");
    }
    
    // create buffers for quadrature rule
    {
        verbose("  create quadrature buffers");
        
        // allocate buffers for quadrature rule
        qq_mem = clCreateBuffer(lcl->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS, nq*sizeof(cl_float2), qq, NULL);
        ww_mem = clCreateBuffer(lcl->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS, nq*sizeof(cl_float2), ww, NULL);
        if(!qq_mem || !ww_mem)
            error("failed to allocate quadrature buffers");
    }
    
    // create buffer that contains object data
    {
        // collect total size of object data, in units of sizeof(cl_float)
        object_size = 0;
        for(size_t i = 0; i < inp->nobjs; ++i)
            object_size += inp->objs[i].size;
        
        verbose("  create object buffer");
        
        // allocate buffer for object data
        object_mem = clCreateBuffer(lcl->context, CL_MEM_READ_WRITE, object_size*sizeof(cl_float), NULL, &err);
        if(err != CL_SUCCESS)
            error("failed to create object buffer");
    }
    
    // create the buffer that will pass parameter values to objects
    {
        verbose("  create parameter buffer");
        
        // create the memory containing physical parameters on the device
        lensed->params = clCreateBuffer(lcl->context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, lensed->npars*sizeof(cl_float), NULL, &err);
        if(err != CL_SUCCESS)
            error("failed to create buffer for parameters");
        
        verbose("  create parameter kernel");
        
        // create kernel
        lensed->set_params = clCreateKernel(program, "set_params", &err);
        if(err != CL_SUCCESS)
            error("failed to create kernel for parameters");
        
        // set kernel arguments
        err = 0;
        err |= clSetKernelArg(lensed->set_params, 0, sizeof(cl_ulong), &object_size);
        err |= clSetKernelArg(lensed->set_params, 1, sizeof(cl_mem), &object_mem);
        err |= clSetKernelArg(lensed->set_params, 2, object_size*sizeof(cl_uint), NULL);
        err |= clSetKernelArg(lensed->set_params, 3, sizeof(cl_mem), &lensed->params);
        if(err != CL_SUCCESS)
            error("failed to set kernel arguments for parameters");
    }
    
    // render kernel
    verbose("  render");
    {
        cl_float4 pcs4;
        size_t wgs, wgm;
        
        verbose("    buffer");
        
        lensed->value_mem = clCreateBuffer(lcl->context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR | CL_MEM_HOST_READ_ONLY, lensed->size*sizeof(cl_float), NULL, NULL);
        lensed->error_mem = clCreateBuffer(lcl->context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR | CL_MEM_HOST_READ_ONLY, lensed->size*sizeof(cl_float), NULL, NULL);
        if(!lensed->value_mem || !lensed->error_mem)
            error("failed to create render buffer");
        
        // pixel coordinate system
        pcs4.s[0] = pcs->rx;
        pcs4.s[1] = pcs->ry;
        pcs4.s[2] = pcs->sx;
        pcs4.s[3] = pcs->sy;
        
        verbose("    kernel");
        
        // render kernel 
        lensed->render = clCreateKernel(program, "render", &err);
        if(err != CL_SUCCESS)
            error("failed to create render kernel");
        
        verbose("    arguments");
        
        // set kernel arguments
        err = 0;
        err |= clSetKernelArg(lensed->render, 0, sizeof(cl_ulong), &object_size);
        err |= clSetKernelArg(lensed->render, 1, sizeof(cl_mem), &object_mem);
        err |= clSetKernelArg(lensed->render, 2, object_size*sizeof(cl_uint), NULL);
        err |= clSetKernelArg(lensed->render, 3, sizeof(cl_float4), &pcs4);
        err |= clSetKernelArg(lensed->render, 4, sizeof(cl_mem), &qq_mem);
        err |= clSetKernelArg(lensed->render, 5, sizeof(cl_mem), &ww_mem);
        err |= clSetKernelArg(lensed->render, 6, sizeof(cl_mem), &lensed->value_mem);
        err |= clSetKernelArg(lensed->render, 7, sizeof(cl_mem), &lensed->error_mem);
        if(err != CL_SUCCESS)
            error("failed to set render kernel arguments");
        
        verbose("    info");
        
        // get work group size for kernel
        err = clGetKernelWorkGroupInfo(lensed->render, lcl->device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(wgs), &wgs, NULL);
        if(err != CL_SUCCESS)
            error("failed to get render kernel work group size");
        
        // get work group size multiple for kernel if OpenCL version > 1.0
        if(opencl_version > 100)
        {
            err = clGetKernelWorkGroupInfo(lensed->render, lcl->device_id, CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, sizeof(wgm), &wgm, NULL);
            if(err != CL_SUCCESS)
                error("failed to get render kernel work group size multiple");
        }
        else
        {
            // fixed work group size multiple of 16 for OpenCL 1.0
            wgm = 16;
        }
        
        verbose("    work size");
        
        // local work size
        lensed->render_lws[0] = wgs;
        
        // make sure work group size is allowed
        if(lensed->render_lws[0] > work_item_sizes[0])
            lensed->render_lws[0] = work_item_sizes[0];
        
        // make sure work group size is a multiple of the preferred size
        lensed->render_lws[0] = (lensed->render_lws[0]/wgm)*wgm;
        
        // global work size
        lensed->render_gws[0] = lensed->size + (lensed->render_lws[0] - lensed->size%lensed->render_lws[0])%lensed->render_lws[0];
        
        verbose("      local:  %zu", lensed->render_lws[0]);
        verbose("      global: %zu", lensed->render_gws[0]);
    }
    
    // convolution kernel if there is a PSF
    if(psf)
    {
        size_t wgs, wgm;
        cl_ulong lm;
        size_t cache_size;
        
        verbose("  convolve");
        
        verbose("    buffer");
        
        lensed->convolve_mem = clCreateBuffer(lcl->context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR | CL_MEM_HOST_READ_ONLY, lensed->size*sizeof(cl_float), NULL, &err);
        if(err != CL_SUCCESS)
            error("failed to create convolve buffer");
        
        verbose("    kernel");
        
        // convolve kernel 
        lensed->convolve = clCreateKernel(program, "convolve", &err);
        if(err != CL_SUCCESS)
            error("failed to create convolve kernel");
        
        verbose("    arguments");
        
        // set kernel arguments
        err = 0;
        err |= clSetKernelArg(lensed->convolve, 0, sizeof(cl_mem), &lensed->value_mem);
        err |= clSetKernelArg(lensed->convolve, 1, sizeof(cl_mem), &psf_mem);
        err |= clSetKernelArg(lensed->convolve, 3, psfw*psfh*sizeof(cl_float), NULL);
        err |= clSetKernelArg(lensed->convolve, 4, sizeof(cl_mem), &lensed->convolve_mem);
        if(err != CL_SUCCESS)
            error("failed to set convolve kernel arguments");
        
        verbose("    info");
        
        // get work group size for kernel
        err = clGetKernelWorkGroupInfo(lensed->convolve, lcl->device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(wgs), &wgs, NULL);
        if(err != CL_SUCCESS)
            error("failed to get convolve kernel work group size");
        
        // get work group size multiple for kernel if OpenCL version > 1.0
        if(opencl_version > 100)
        {
            err = clGetKernelWorkGroupInfo(lensed->convolve, lcl->device_id, CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, sizeof(wgm), &wgm, NULL);
            if(err != CL_SUCCESS)
                error("failed to get convolve kernel work group size multiple");
        }
        else
        {
            // fixed work group size multiple of 16 for OpenCL 1.0
            wgm = 16;
        }
        
        // get local memory size for kernel
        err = clGetKernelWorkGroupInfo(lensed->convolve, lcl->device_id, CL_KERNEL_LOCAL_MEM_SIZE, sizeof(lm), &lm, NULL);
        if(err != CL_SUCCESS)
            error("failed to get convolve kernel local memory size");
        
        verbose("    work size");
        
        // local work size, start at maximum
        lensed->convolve_lws[0] = work_item_sizes[0];
        lensed->convolve_lws[1] = work_item_sizes[1];
        
        // reduce local work size until it fits into work group
        while(lensed->convolve_lws[0]*lensed->convolve_lws[1] > wgs)
        {
            if(lensed->convolve_lws[0] > lensed->convolve_lws[1])
                lensed->convolve_lws[0] /= 2;
            else
                lensed->convolve_lws[1] /= 2;
        }
        
        // size of local memory that stores part of the model
        cache_size = (psfw/2 + lensed->convolve_lws[0] + psfw/2)*(psfh/2 + lensed->convolve_lws[1] + psfh/2)*sizeof(cl_float);
        
        // reduce local work size until cache fits into local memory
        while(2*cache_size > local_mem_size - lm)
        {
            if(lensed->convolve_lws[0] > lensed->convolve_lws[1])
                lensed->convolve_lws[0] /= 2;
            else
                lensed->convolve_lws[1] /= 2;
            
            // make sure that PSF fits into local memory at all
            if(lensed->convolve_lws[0]*lensed->convolve_lws[1] < 1)
                error("PSF too large for local memory on device (%zukB)", local_mem_size/1024);
            
            cache_size = (psfw/2 + lensed->convolve_lws[0] + psfw/2)*(psfh/2 + lensed->convolve_lws[1] + psfh/2)*sizeof(cl_float);
        }
        
        // global work size must be padded to block size
        lensed->convolve_gws[0] = lensed->width + (lensed->convolve_lws[0] - lensed->width%lensed->convolve_lws[0])%lensed->convolve_lws[0];
        lensed->convolve_gws[1] = lensed->height + (lensed->convolve_lws[1] - lensed->height%lensed->convolve_lws[1])%lensed->convolve_lws[1];
        
        verbose("      local:  %zu x %zu", lensed->convolve_lws[0], lensed->convolve_lws[1]);
        verbose("      global: %zu x %zu", lensed->convolve_gws[0], lensed->convolve_gws[1]);
        
        verbose("    cache");
        
        // set cache size
        err = clSetKernelArg(lensed->convolve, 2, cache_size, NULL);
        if(err != CL_SUCCESS)
            error("failed to set convolve kernel cache");
    }
    else
    {
        // no kernel: used to determine whether to convolve
        lensed->convolve = 0;
    }
    
    // loglike kernel
    verbose("  loglike");
    {
        size_t wgs, wgm;
        
        verbose("    buffer");
        
        lensed->loglike_mem = clCreateBuffer(lcl->context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR | CL_MEM_HOST_READ_ONLY, lensed->size*sizeof(cl_float), NULL, &err);
        if(err != CL_SUCCESS)
            error("failed to create loglike buffer");
        
        verbose("    kernel");
        
        // loglike kernel, take care: the buffer it works on depends on PSF
        lensed->loglike = clCreateKernel(program, "loglike", &err);
        if(err != CL_SUCCESS)
            error("failed to create loglike kernel");
        
        verbose("    arguments");
        
        // set kernel arguments
        err = 0;
        err |= clSetKernelArg(lensed->loglike, 0, sizeof(cl_mem), &image_mem);
        err |= clSetKernelArg(lensed->loglike, 1, sizeof(cl_mem), &weight_mem);
        err |= clSetKernelArg(lensed->loglike, 2, sizeof(cl_mem), psf ? &lensed->convolve_mem : &lensed->value_mem);
        err |= clSetKernelArg(lensed->loglike, 3, sizeof(cl_mem), &lensed->loglike_mem);
        if(err != CL_SUCCESS)
            error("failed to set loglike kernel arguments");
        
        verbose("    info");
        
        // get work group size for kernel
        err = clGetKernelWorkGroupInfo(lensed->loglike, lcl->device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(wgs), &wgs, NULL);
        if(err != CL_SUCCESS)
            error("failed to get loglike kernel work group information");
        
        // get work group size multiple for kernel if OpenCL version > 1.0
        if(opencl_version > 100)
        {
            err = clGetKernelWorkGroupInfo(lensed->loglike, lcl->device_id, CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, sizeof(wgm), &wgm, NULL);
            if(err != CL_SUCCESS)
                error("failed to get loglike kernel work group size multiple");
        }
        else
        {
            // fixed work group size multiple of 16 for OpenCL 1.0
            wgm = 16;
        }
        
        verbose("    work size");
        
        // local work size
        lensed->loglike_lws[0] = wgs;
        
        // make sure work group size is allowed
        if(lensed->loglike_lws[0] > work_item_sizes[0])
            lensed->loglike_lws[0] = work_item_sizes[0];
        
        // make sure work group size is a multiple of the preferred size
        lensed->loglike_lws[0] = (lensed->loglike_lws[0]/wgm)*wgm;
        
        // global work size for kernel
        lensed->loglike_gws[0] = lensed->size + (lensed->loglike_lws[0] - lensed->size%lensed->loglike_lws[0])%lensed->loglike_lws[0];
        
        verbose("      local:  %zu", lensed->loglike_lws[0]);
        verbose("      global: %zu", lensed->loglike_gws[0]);
    }
    
    // profiling information
    if(inp->opts->profile)
    {
        verbose("  profiler");
        
        // create the struct containing profiles
        lensed->profile = malloc(sizeof(*lensed->profile));
        if(!lensed->profile)
            errori(NULL);
        
        // create the profiles
        lensed->profile->map_params        = profile_create("+params");
        lensed->profile->unmap_params      = profile_create("-params");
        lensed->profile->set_params        = profile_create("set_params");
        lensed->profile->render            = profile_create("render");
        lensed->profile->convolve          = profile_create("convolve");
        lensed->profile->loglike           = profile_create("loglike");
        lensed->profile->map_loglike_mem   = profile_create("+loglike_mem");
        lensed->profile->unmap_loglike_mem = profile_create("-loglike_mem");
    }
    else
    {
        // no profiling
        lensed->profile = NULL;
    }
    
    
    /***********
     * DS9 XPA *
     ***********/
    
#ifdef LENSED_XPA
    verbose("XPA");
    
    // create a persistent XPA handle
    lensed->xpa = XPAOpen(NULL);
    
    if(lensed->xpa)
        verbose("  handle created");
    else
        verbose("  could not create handle!");
        
    // set up DS9 for Lensed if successful
    if(inp->opts->ds9)
    {
        // XPA response
        char* buf = NULL;
        size_t len = 0;
        
        verbose("  DS9 enabled");
        
        // store XPA name for DS9
        lensed->ds9 = inp->opts->ds9_name;
        
        verbose("    XPA name: %s", lensed->ds9);
        
        // set view to tile
        XPASet(lensed->xpa, lensed->ds9, "tile yes", NULL, NULL, 0, NULL, NULL, 1);
        
        // create frame for images
        XPASet(lensed->xpa, lensed->ds9, "frame new", NULL, NULL, 0, NULL, NULL, 1);
        
        // get newly created frame
        XPAGet(lensed->xpa, lensed->ds9, "frame", NULL, &buf, &len, NULL, NULL, 1);
        if(buf)
            lensed->ds9_frame = atoi(buf);
        else
            lensed->ds9_frame = 999;
        
        verbose("    frame no: %d", lensed->ds9_frame);
        
        // free response buffer
        free(buf);
    }
    else
    {
        verbose("  DS9 disabled");
    }
#else
    // XPA is not supported
    lensed->xpa = NULL;
    lensed->ds9 = 0;
#endif
    
    
    /***************
     * ready to go *
     ***************/
    
    info("find posterior");
    
    // take start time
    start = time(0);
    
    // some space
    info("  ");
    
    // call MultiNest
    {
        // MultiNest options
        int ndim = lensed->ndims;
        int npar = lensed->npars;
        int ncdim = ndim;
        double ztol = -1E90;
        char root[100] = {0};
        int initmpi = 1;
        double logzero = -DBL_MAX;
        int* wrap;
        
        // efficiency rating can mean different things, depending on ceff
        double efr = inp->opts->ceff ? inp->opts->acc : inp->opts->shf;
        
        // copy root element for file output if given
        if(inp->opts->root)
            strncpy(root, inp->opts->root, 99);
        
        // create array for parameter wrap-around
        wrap = malloc(lensed->npars*sizeof(int));
        if(!wrap)
            errori(NULL);
        for(size_t i = 0; i < lensed->npars; ++i)
            wrap[i] = lensed->pars[lensed->pmap[i]]->wrap;
        
        // mute MultiNest when necessary
        if(LOG_LEVEL == LOG_QUIET || LOG_LEVEL == LOG_BATCH)
            mute();
        
        // set signal handler for keyboard interrupts
        signal(SIGINT, handler);
        
        // run MultiNest, re-entry point for interrupts
        if(setjmp(jmp) == 0)
            run(inp->opts->ins, inp->opts->mmodal, inp->opts->ceff,
                inp->opts->nlive, inp->opts->tol, efr, ndim, npar, ncdim,
                inp->opts->maxmodes, inp->opts->updint, ztol, root,
                inp->opts->seed, wrap, inp->opts->feedback, inp->opts->resume,
                inp->opts->output, initmpi, logzero, inp->opts->maxiter,
                loglike, dumper, lensed);
        else
            info("\ninterrupted!");
        
        // restore signal handling
        signal(SIGINT, SIG_DFL);
        
        // unmute
        if(LOG_LEVEL == LOG_QUIET || LOG_LEVEL == LOG_BATCH)
            unmute();
        
        // free MultiNest data
        free(wrap);
    }
    
    // some more space
    info("  ");
    
    // take end time
    end = time(0);
    
    // show duration
    dur = difftime(end, start);
    info("  done in %02d:%02d:%02d", (int)(dur/3600), (int)(fmod(dur, 3600)/60), (int)fmod(dur, 60));
    info("  ");
    
    
    /***********
     * results *
     ***********/
    
    // compute chi^2/dof
    chi2_dof = -2*lensed->max_loglike / (lensed->size - masked - lensed->npars);
    
    // summary statistics
    info("summary");
    info("  ");
    info(LOG_BOLD "  log-evidence: " LOG_RESET "%.4f ± %.4f", inp->opts->ins ? lensed->logev_ins : lensed->logev, lensed->logev_err);
    info(LOG_BOLD "  max log-like: " LOG_RESET "%.4f", lensed->max_loglike);
    info(LOG_BOLD "  min chi²/dof: " LOG_RESET "%.4f", chi2_dof);
    info("  ");
    
    // parameter table
    info("parameters");
    info("  ");
    info(LOG_BOLD "  %-12s  %10s  %10s  %10s  %10s" LOG_RESET,
         "parameter", "mean", "sigma", "ML", "MAP");
    info("  ------------------------------------------------------------");
    for(size_t i = 0; i < lensed->npars; ++i)
        if(!lensed->pars[i]->defval)
            info("  %-12s  %10.4f  %10.4f  %10.4f  %10.4f",
                lensed->pars[i]->label ? lensed->pars[i]->label : lensed->pars[i]->id,
                lensed->mean[i], lensed->sigma[i], lensed->ml[i], lensed->map[i]);
    info("  ");
    
    // profiling results
    if(lensed->profile)
    {
        // the list of profiles
        profile* profv[] = {
            lensed->profile->map_params,
            lensed->profile->unmap_params,
            lensed->profile->set_params,
            lensed->profile->render,
            lensed->profile->convolve,
            lensed->profile->loglike,
            lensed->profile->map_loglike_mem,
            lensed->profile->unmap_loglike_mem
        };
        int profc = sizeof(profv)/sizeof(profv[0]);
        
        info("profiler");
        info("  ");
        profile_print(profc, profv);
        info("  ");
    }
    
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
        
        for(size_t i = 0; i < lensed->npars; ++i)
        {
            // get parameter using map
            const param* par = lensed->pars[lensed->pmap[i]];
            
            // prior range
            double lower = prior_lower(par->pri);
            double upper = prior_upper(par->pri);
            
            // output parameter id and, if set, label
            fprintf(paramfile, "%-20s  %s\n", par->id, par->label ? par->label : "");
            
            // output parameter range
            fprintf(rangefile, "%-20s  ", par->id);
            if(isfinite(lower))
                fprintf(rangefile, "%10.4f  ", lower);
            else
                fprintf(rangefile, "%10s  ", "N");
            if(isfinite(upper))
                fprintf(rangefile, "%10.4f\n", upper);
            else
                fprintf(rangefile, "%10s\n", "N");
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
        printf("%-18.4f  ", inp->opts->ins ? lensed->logev_ins : lensed->logev);
        printf("%-18.4f  ", lensed->max_loglike);
        printf("%-18.4f  ", chi2_dof);
        
        // write parameter results
        for(size_t i = 0; i < lensed->npars; ++i)
            if(!lensed->pars[i]->defval)
                printf("%-10.4f  ", lensed->mean[i]);
        for(size_t i = 0; i < lensed->npars; ++i)
            if(!lensed->pars[i]->defval)
                printf("%-10.4f  ", lensed->sigma[i]);
        for(size_t i = 0; i < lensed->npars; ++i)
            if(!lensed->pars[i]->defval)
                printf("%-10.4f  ", lensed->ml[i]);
        for(size_t i = 0; i < lensed->npars; ++i)
            if(!lensed->pars[i]->defval)
                printf("%-10.4f  ", lensed->map[i]);
        
        // output is done
        printf("\n");
    }
    
    
    /************
     * cleaning *
     ************/
    
#ifdef LENSED_XPA
    // close XPA handle
    if(lensed->xpa)
        XPAClose(lensed->xpa);
#endif
    
    // free profile
    if(lensed->profile)
    {
        profile_free(lensed->profile->map_params);
        profile_free(lensed->profile->unmap_params);
        profile_free(lensed->profile->set_params);
        profile_free(lensed->profile->render);
        profile_free(lensed->profile->convolve);
        profile_free(lensed->profile->loglike);
        profile_free(lensed->profile->map_loglike_mem);
        profile_free(lensed->profile->unmap_loglike_mem);
        free(lensed->profile);
    }
    
    // free render kernel
    clReleaseKernel(lensed->render);
    clReleaseMemObject(lensed->value_mem);
    clReleaseMemObject(lensed->error_mem);
    
    // free convolve kernel
    if(psf)
    {
        clReleaseKernel(lensed->convolve);
        clReleaseMemObject(lensed->convolve_mem);
    }
    
    // free loglike kernel
    clReleaseKernel(lensed->loglike);
    clReleaseMemObject(lensed->loglike_mem);
    
    // free parameter space
    clReleaseMemObject(lensed->params);
    clReleaseKernel(lensed->set_params);
    
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
    
    // free device info
    free(work_item_sizes);
    
    // free worker
    clReleaseProgram(program);
    clReleaseCommandQueue(lensed->queue);
    free_lensed_cl(lcl);
    
    // free quadrature rule
    free(qq);
    free(ww);
    
    // free results
    free((char*)lensed->fits);
    free(lensed->mean);
    free(lensed->sigma);
    free(lensed->ml);
    free(lensed->map);
    
    // free parameter space
    free(lensed->pars);
    free(lensed->pmap);
    
    // free data
    free(lensed->image);
    free(pcs);
    free(lensed->weight);
    free(psf);
    
    // free input
    free_input(inp);
    
    // free lensed struct
    free(lensed);
    
    // there might be unflushed output from Fortran on stdout
    if(LOG_LEVEL == LOG_QUIET || LOG_LEVEL == LOG_BATCH)
        mute();
    
    return EXIT_SUCCESS;
}
