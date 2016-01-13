#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "../opencl.h"
#include "../input.h"
#include "../prior.h"
#include "objects.h"
#include "../kernel.h"
#include "../log.h"

static const char* WS = " \t\n\v\f\r";

void add_object(input* inp, const char* id, const char* name)
{
    object* obj;
    
    // OpenCL
    cl_int err;
    lensed_cl* lcl;
    cl_command_queue queue;
    cl_program program;
    size_t nkernels;
    const char** kernels;
    
    // object metadata kernel
    char*       meta_kernam;
    cl_kernel   meta_kernel;
    
    // object metadata
    cl_mem      meta_type_mem;
    cl_int      meta_type;
    cl_mem      meta_size_mem;
    cl_ulong    meta_size;
    cl_mem      meta_npar_mem;
    cl_ulong    meta_npar;
    
    // parameter info kernel
    char*       param_kernam;
    cl_kernel   param_kernel;
    size_t      param_gws;
    
    // parameter information
    cl_mem      param_names_mem;
    cl_char16*  param_names;
    cl_mem      param_types_mem;
    cl_int*     param_types;
    cl_mem      param_bounds_mem;
    cl_float2*  param_bounds;
    cl_mem      param_defvals_mem;
    cl_float*   param_defvals;
    
    // realloc space for one more object
    inp->nobjs += 1;
    inp->objs = realloc(inp->objs, inp->nobjs*sizeof(object));
    if(!inp->objs)
        errori("object %s", name);
    
    // realloc was successful, get new object
    obj = &inp->objs[inp->nobjs-1];
    
    // allocate space and copy id and name into object
    obj->id = malloc(strlen(id) + 1);
    obj->name = malloc(strlen(name) + 1);
    if(!obj->id || !obj->name)
        errori("object %s", id);
    strcpy((char*)obj->id, id);
    strcpy((char*)obj->name, name);
    
    // set up host device
    lcl = get_lensed_cl(NULL);
    queue = clCreateCommandQueue(lcl->context, lcl->device_id, 0, &err);
    if(err != CL_SUCCESS)
        error("object %s: failed to create command queue", id);
    
    // load kernel for object metadata
    object_program(name, &nkernels, &kernels);
    
    // create the program from kernel sources
    program = clCreateProgramWithSource(lcl->context, nkernels, kernels, NULL, &err);
    if(err != CL_SUCCESS)
        error("object %s: failed to create program", id);
    
    // build the program
    {
        // this is to satisfy the preprocessor
        const char* build_flags[] = { 0 };
        const char* build_options = kernel_options(0, 0, 0, 0, 0, 0, build_flags);
        
        err = clBuildProgram(program, 1, &lcl->device_id, build_options, NULL, NULL);
        if(err != CL_SUCCESS)
        {
// build log is reported in the notifications on Apple's implementation
#ifndef __APPLE__
            if(LOG_LEVEL <= LOG_VERBOSE)
            {
                char log[4096];
                clGetProgramBuildInfo(program, lcl->device_id, CL_PROGRAM_BUILD_LOG, sizeof(log), log, NULL);
                verbose("build log:");
                verbose("%s", log);
            }
#endif
            error("object %s: failed to build program", id);
        }
        
        free((char*)build_options);
    }
    
    // buffers for object metadata
    meta_type_mem = clCreateBuffer(lcl->context, CL_MEM_WRITE_ONLY, sizeof(cl_int), NULL, NULL);
    meta_size_mem = clCreateBuffer(lcl->context, CL_MEM_WRITE_ONLY, sizeof(cl_ulong), NULL, NULL);
    meta_npar_mem = clCreateBuffer(lcl->context, CL_MEM_WRITE_ONLY, sizeof(cl_ulong), NULL, NULL);
    if(!meta_type_mem || !meta_size_mem || !meta_npar_mem)
        error("object %s: failed to create buffer for metadata", id);
    
    // setup and run kernel to get meta_data
    meta_kernam = kernel_name("meta_", name);
    meta_kernel = clCreateKernel(program, meta_kernam, &err);
    if(err != CL_SUCCESS)
        error("object %s: failed to create kernel for metadata", id);
    err |= clSetKernelArg(meta_kernel, 0, sizeof(cl_mem), &meta_type_mem);
    err |= clSetKernelArg(meta_kernel, 1, sizeof(cl_mem), &meta_size_mem);
    err |= clSetKernelArg(meta_kernel, 2, sizeof(cl_mem), &meta_npar_mem);
    if(err != CL_SUCCESS)
        error("object %s: failed to set kernel arguments for metadata", id);
    err = clEnqueueTask(queue, meta_kernel, 0, NULL, NULL);
    if(err != CL_SUCCESS)
        error("object %s: failed to run kernel for metadata", id);
    
    // get metadata from buffer
    err |= clEnqueueReadBuffer(queue, meta_type_mem, CL_TRUE, 0, sizeof(cl_int),   &meta_type, 0, NULL, NULL);
    err |= clEnqueueReadBuffer(queue, meta_size_mem, CL_TRUE, 0, sizeof(cl_ulong), &meta_size, 0, NULL, NULL);
    err |= clEnqueueReadBuffer(queue, meta_npar_mem, CL_TRUE, 0, sizeof(cl_ulong), &meta_npar, 0, NULL, NULL);
    if(err != CL_SUCCESS)
        error("object %s: failed to get metadata", id);
    
    // convert size in sizeof(cl_char) to size in sizeof(cl_float), rounding up
    meta_size = ((meta_size*sizeof(cl_char))/sizeof(cl_float)) + ((meta_size*sizeof(cl_char))%sizeof(cl_float) ? 1 : 0);
    
    // set metadata for object
    obj->type  = meta_type;
    obj->size  = meta_size;
    obj->npars = meta_npar;
    
    // check metadata
    if(obj->type != OBJ_LENS && obj->type != OBJ_SOURCE && obj->type != OBJ_FOREGROUND && obj->type != OBJ_PLANE)
        error("object %s: invalid type (should be LENS, SOURCE, FOREGROUND or PLANE)", id);
    
    // buffers for kernel parameters
    param_names_mem   = clCreateBuffer(lcl->context, CL_MEM_WRITE_ONLY, obj->npars*sizeof(cl_char16), NULL, NULL);
    param_types_mem   = clCreateBuffer(lcl->context, CL_MEM_WRITE_ONLY, obj->npars*sizeof(cl_int),    NULL, NULL);
    param_bounds_mem  = clCreateBuffer(lcl->context, CL_MEM_WRITE_ONLY, obj->npars*sizeof(cl_float2), NULL, NULL);
    param_defvals_mem = clCreateBuffer(lcl->context, CL_MEM_WRITE_ONLY, obj->npars*sizeof(cl_float),  NULL, NULL);
    if(!param_names_mem || !param_types_mem || !param_bounds_mem || !param_defvals_mem)
        error("object %s: failed to create buffer for parameters", id);
    
    // the work size of the parameters kernel is the number of parameters
    param_gws = meta_npar;
    
    // setup and run kernel to get parameters
    param_kernam = kernel_name("params_", name);
    param_kernel = clCreateKernel(program, param_kernam, &err);
    if(err != CL_SUCCESS)
        error("object %s: failed to create kernel for parameters", id);
    err |= clSetKernelArg(param_kernel, 0, sizeof(cl_mem), &param_names_mem  );
    err |= clSetKernelArg(param_kernel, 1, sizeof(cl_mem), &param_types_mem  );
    err |= clSetKernelArg(param_kernel, 2, sizeof(cl_mem), &param_bounds_mem );
    err |= clSetKernelArg(param_kernel, 3, sizeof(cl_mem), &param_defvals_mem);
    if(err != CL_SUCCESS)
        error("object %s: failed to set kernel arguments for parameters", id);
    err = clEnqueueNDRangeKernel(queue, param_kernel, 1, NULL, &param_gws, NULL, 0, NULL, NULL);
    if(err != CL_SUCCESS)
        error("object %s: failed to run kernel for parameters", id);
    
    // arrays for kernel parameters
    param_names   = malloc(obj->npars*sizeof(cl_char16));
    param_types   = malloc(obj->npars*sizeof(cl_int));
    param_bounds  = malloc(obj->npars*sizeof(cl_float2));
    param_defvals = malloc(obj->npars*sizeof(cl_float));
    if(!param_types || !param_names || !param_bounds || !param_defvals)
        errori("object %s", id);
    
    // get kernel parameters from buffer
    err |= clEnqueueReadBuffer(queue, param_names_mem,   CL_TRUE, 0, obj->npars*sizeof(cl_char16), param_names,   0, NULL, NULL);
    err |= clEnqueueReadBuffer(queue, param_types_mem,   CL_TRUE, 0, obj->npars*sizeof(cl_int),    param_types,   0, NULL, NULL);
    err |= clEnqueueReadBuffer(queue, param_bounds_mem,  CL_TRUE, 0, obj->npars*sizeof(cl_float2), param_bounds,  0, NULL, NULL);
    err |= clEnqueueReadBuffer(queue, param_defvals_mem, CL_TRUE, 0, obj->npars*sizeof(cl_float),  param_defvals, 0, NULL, NULL);
    if(err != CL_SUCCESS)
        error("object %s: failed to get parameters", id);
    
    // create array for params
    obj->pars = malloc(obj->npars*sizeof(param));
    if(!obj->pars)
        errori("object %s", id);
    
    // set up parameter array
    for(size_t i = 0; i < obj->npars; ++i)
    {
        // name of parameter
        char* name;
        
        // id of parameter
        char* id;
        
        // prior of parameter
        prior* pri = NULL;
        
        // allocate name
        name = malloc(16);
        if(!name)
            errori(NULL);
        
        // copy name
        for(size_t j = 0; j < 16; ++j)
            name[j] = param_names[i].s[j];
        
        // create id
        id = malloc(strlen(obj->id) + 1 + strlen(name) + 1);
        if(!id)
            errori(NULL);
        sprintf(id, "%s.%s", obj->id, name);
        
        // create default value prior
        if(param_defvals[i] > 0 || signbit(param_defvals[i]))
            pri = prior_default(param_defvals[i]);
        
        // set parameter information
        obj->pars[i].name   = name;
        obj->pars[i].id     = id;
        obj->pars[i].type   = param_types[i];
        obj->pars[i].lower  = param_bounds[i].s[0];
        obj->pars[i].upper  = param_bounds[i].s[1];
        obj->pars[i].pri    = pri;
        obj->pars[i].wrap   = 0;
        obj->pars[i].ipp    = 0;
        obj->pars[i].defval = pri ? 1 : 0;
        obj->pars[i].label  = NULL;
    }
    
    // clean up
    clFinish(queue);
    
    clReleaseMemObject(param_names_mem);
    free(param_names);
    clReleaseMemObject(param_types_mem);
    free(param_types);
    clReleaseMemObject(param_bounds_mem);
    free(param_bounds);
    
    free(param_kernam);
    clReleaseKernel(param_kernel);
    
    free(meta_kernam);
    clReleaseKernel(meta_kernel);
    
    clReleaseMemObject(meta_type_mem);
    clReleaseMemObject(meta_size_mem);
    clReleaseMemObject(meta_npar_mem);
    
    for(int i = 0; i < nkernels; ++i)
        free((void*)kernels[i]);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    free_lensed_cl(lcl);
}

object* find_object(const input* inp, const char* id)
{
    // position of found object
    size_t pos;
    
    // look for name in each object
    for(pos = 0; pos < inp->nobjs; ++pos)
        if(strcmp(id, inp->objs[pos].id) == 0)
            break;
    
    // return object if found, or else NULL
    if(pos < inp->nobjs)
        return &inp->objs[pos];
    return NULL;
}

void free_object(object* obj)
{
    free((char*)obj->id);
    free((char*)obj->name);
    for(size_t i = 0; i < obj->npars; ++i)
        free_param(&obj->pars[i]);
    free(obj->pars);
}

param* find_param(object* obj, const char* name)
{
    // position of found param
    size_t pos;
    
    // look for name in each param
    for(pos = 0; pos < obj->npars; ++pos)
        if(strcmp(name, obj->pars[pos].name) == 0)
            break;
    
    // return param if found, or else NULL
    if(pos < obj->npars)
        return &obj->pars[pos];
    return NULL;
}

void free_param(param* par)
{
    free((char*)par->name);
    free((char*)par->id);
    free((char*)par->label);
    prior_free(par->pri);
}

void set_param_label(param* par, const char* label)
{
    // allocate space for string
    par->label = realloc((char*)par->label, strlen(label) + 1);
    if(!par->label)
        errori("could not set parameter label \"%s\"", label);
    
    // copy label to param
    strcpy((char*)par->label, label);
}

void set_param_prior(param* par, const char* str)
{
    size_t len;
    
    // free existing prior
    prior_free(par->pri);
    
    // parse possible keywords
    while(1)
    {
        // get length of first word
        len = strcspn(str, WS);
        
        // check for keywords or break loop
        if(strncmp(str, "wrap", len) == 0)
            par->wrap = 1;
        else if(strncmp(str, "image", len) == 0)
            par->ipp = 1;
        else
            break;
        
        // skip word and whitespace
        str += len;
        str += strspn(str, WS);
    }
    
    // parse prior
    par->pri = *str ? prior_read(str) : NULL;
    
    // no longer default value
    par->defval = 0;
}
