#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../opencl.h"
#include "../input.h"
#include "../prior.h"
#include "objects.h"
#include "../kernel.h"
#include "../log.h"

static const char* WS = " \t\n\v\f\r";

static const cl_uint MAX_PAR_NAME = 32;

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
    cl_mem meta_type_mem;
    cl_mem meta_size_mem;
    cl_mem meta_npars_mem;
    char* meta_name;
    cl_kernel meta_kernel;
    cl_int meta_type;
    cl_ulong meta_size;
    cl_ulong meta_npars;
    char* param_kernam;
    cl_kernel param_kernel;
    cl_mem param_names_mem;
    cl_char* param_names;
    cl_char* param_cur_name;
    
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
    
    // load kernel for object meta-data
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
    
    // buffers for object meta-data
    meta_type_mem = clCreateBuffer(lcl->context, CL_MEM_WRITE_ONLY, sizeof(cl_int), NULL, NULL);
    meta_size_mem = clCreateBuffer(lcl->context, CL_MEM_WRITE_ONLY, sizeof(cl_ulong), NULL, NULL);
    meta_npars_mem = clCreateBuffer(lcl->context, CL_MEM_WRITE_ONLY, sizeof(cl_ulong), NULL, NULL);
    if(!meta_type_mem || !meta_size_mem || !meta_npars_mem)
        error("object %s: failed to create buffer for meta-data", id);
    
    // setup and run kernel to get meta_data
    meta_name = kernel_name("meta_", name);
    meta_kernel = clCreateKernel(program, meta_name, &err);
    if(err != CL_SUCCESS)
        error("object %s: failed to create kernel for meta-data", id);
    err |= clSetKernelArg(meta_kernel, 0, sizeof(cl_mem), &meta_type_mem);
    err |= clSetKernelArg(meta_kernel, 1, sizeof(cl_mem), &meta_size_mem);
    err |= clSetKernelArg(meta_kernel, 2, sizeof(cl_mem), &meta_npars_mem);
    if(err != CL_SUCCESS)
        error("object %s: failed to set kernel arguments for meta-data", id);
    err = clEnqueueTask(queue, meta_kernel, 0, NULL, NULL);
    if(err != CL_SUCCESS)
        error("object %s: failed to run kernel for meta-data", id);
    
    // get meta-data from buffer
    err |= clEnqueueReadBuffer(queue, meta_type_mem, CL_TRUE, 0, sizeof(cl_int), &meta_type, 0, NULL, NULL);
    err |= clEnqueueReadBuffer(queue, meta_size_mem, CL_TRUE, 0, sizeof(cl_ulong), &meta_size, 0, NULL, NULL);
    err |= clEnqueueReadBuffer(queue, meta_npars_mem, CL_TRUE, 0, sizeof(cl_ulong), &meta_npars, 0, NULL, NULL);
    if(err != CL_SUCCESS)
        error("object %s: failed to get meta-data", id);
    
    // set meta-data for object
    obj->type = meta_type;
    obj->size = meta_size;
    obj->npars = meta_npars;
    
    // check meta-data
    if(obj->type != OBJ_LENS && obj->type != OBJ_SOURCE && obj->type != OBJ_FOREGROUND)
        error("object %s: invalid type (should be LENS, SOURCE or FOREGROUND)", id);
    
    // buffers for kernel parameters
    param_names_mem = clCreateBuffer(lcl->context, CL_MEM_WRITE_ONLY, obj->npars*MAX_PAR_NAME*sizeof(cl_char), NULL, NULL);
    if(!param_names_mem)
        error("object %s: failed to create buffer for parameters", id);
    
    // setup and run kernel to get parameters
    param_kernam = kernel_name("params_", name);
    param_kernel = clCreateKernel(program, param_kernam, &err);
    if(err != CL_SUCCESS)
        error("object %s: failed to create kernel for parameters", id);
    err  = clSetKernelArg(param_kernel, 0, sizeof(cl_mem), &param_names_mem);
    err |= clSetKernelArg(param_kernel, 1, sizeof(cl_uint), &MAX_PAR_NAME);
    if(err != CL_SUCCESS)
        error("object %s: failed to set kernel arguments for parameters", id);
    err = clEnqueueTask(queue, param_kernel, 0, NULL, NULL);
    if(err != CL_SUCCESS)
        error("object %s: failed to run kernel for parameters", id);
    
    // arrays for kernel parameters
    param_names = malloc(obj->npars*MAX_PAR_NAME*sizeof(cl_char));
    if(!param_names)
        errori("object %s", id);
    
    // get kernel parameters from buffer
    err = clEnqueueReadBuffer(queue, param_names_mem, CL_TRUE, 0, obj->npars*MAX_PAR_NAME*sizeof(cl_char), param_names, 0, NULL, NULL);
    if(err != CL_SUCCESS)
        error("object %s: failed to get parameters", id);
    
    // create array for params
    obj->pars = malloc(obj->npars*sizeof(param));
    if(!obj->pars)
        errori("object %s", id);
    
    // the current parameter name
    param_cur_name = param_names;
    
    // set up parameter array
    for(size_t i = 0; i < obj->npars; ++i)
    {
        // name of parameter
        char* name;
        
        // id of parameter
        char* id;
        
        // length of parameter name
        size_t len = 0;
        
        // get length of name
        while(param_cur_name[len] != '\0')
            ++len;
        
        // allocate name
        name = malloc(len + 1);
        if(!name)
            errori(NULL);
        
        // copy name and set in param
        for(size_t i = 0; i <= len; ++i)
            name[i] = param_cur_name[i];
        obj->pars[i].name = name;
        
        // skip name
        param_cur_name += len + 1;
        
        // create id
        id = malloc(strlen(obj->id) + 1 + len + 1);
        if(!id)
            errori(NULL);
        sprintf(id, "%s.%s", obj->id, obj->pars[i].name);
        obj->pars[i].id = id;
        
        // no label is set
        obj->pars[i].label = NULL;
        
        // no prior is set
        obj->pars[i].pri = NULL;
        
        // no wrap-around
        obj->pars[i].wrap = 0;
    }
    
    // clean up
    clFinish(queue);
    clReleaseMemObject(param_names_mem);
    free(param_names);
    clReleaseKernel(param_kernel);
    free(param_kernam);
    clReleaseKernel(meta_kernel);
    free(meta_name);
    clReleaseMemObject(meta_type_mem);
    clReleaseMemObject(meta_size_mem);
    clReleaseMemObject(meta_npars_mem);
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
    free_prior(par->pri);
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
    free_prior(par->pri);
    
    // get length of first word
    len = strcspn(str, WS);
    
    // check for wrap keyword
    if(strncmp(str, "wrap", len) == 0)
    {
        // set wrap flag
        par->wrap = 1;
        
        // skip word
        str += len;
        str += strspn(str, WS);
    }
    
    // parse prior
    par->pri = *str ? read_prior(str) : NULL;
}
