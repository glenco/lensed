#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include "../input.h"
#include "objects.h"
#include "../kernel.h"
#include "../log.h"

#pragma pack(push, 4)
struct kernel_param
{
    cl_char name[32];
    cl_int  wrap;
};
#pragma pack(pop)

static void opencl_notify(const char* errinfo, const void* private_info,  size_t cb, void* user_data)
{
    verbose("%s", errinfo);
}

void add_object(input* inp, const char* id, const char* name)
{
    object* obj;
    
    // OpenCL
    cl_int err;
    cl_device_id device;
    cl_context context;
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
    cl_mem params_mem;
    char* params_name;
    cl_kernel params_kernel;
    struct kernel_param* params;
    
    // realloc space for one more object
    inp->nobjs += 1;
    inp->objs = realloc(inp->objs, inp->nobjs*sizeof(object));
    if(!inp->objs)
        error("object %s: %s", name, strerror(errno));
    
    // realloc was successful, get new object
    obj = &inp->objs[inp->nobjs-1];
    
    // allocate space and copy id and name into object
    obj->id = malloc(strlen(id) + 1);
    obj->name = malloc(strlen(name) + 1);
    if(!obj->id || !obj->name)
        error("object %s: %s", id, strerror(errno));
    strcpy((char*)obj->id, id);
    strcpy((char*)obj->name, name);
    
    // set up host device
    err = clGetDeviceIDs(NULL, CL_DEVICE_TYPE_CPU, 1, &device, NULL);
    if(err != CL_SUCCESS)
        error("object %s: failed to get device", id);
    
    context = clCreateContext(0, 1, &device, opencl_notify, NULL, &err);
    if(err != CL_SUCCESS)
        error("object %s: failed to create device context", id);
    
    queue = clCreateCommandQueue(context, device, 0, &err);
    if(err != CL_SUCCESS)
        error("object %s: failed to create command queue", id);
    
    // load kernel for object meta-data
    object_program(name, &nkernels, &kernels);
    
    program = clCreateProgramWithSource(context, nkernels, kernels, NULL, &err);
    if(err != CL_SUCCESS)
        error("object %s: failed to create program", id);
    
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if(err != CL_SUCCESS)
        error("object %s: failed to build program", id);
    
    // buffers for object meta-data
    meta_type_mem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(cl_int), NULL, NULL);
    meta_size_mem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(cl_ulong), NULL, NULL);
    meta_npars_mem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(cl_ulong), NULL, NULL);
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
    if(obj->type != OBJ_LENS && obj->type != OBJ_SOURCE)
        error("object %s: invalid type (should be LENS or SOURCE)", id);
    
    // buffer for kernel parameters
    params_mem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, obj->npars*sizeof(struct kernel_param), NULL, &err);
    if(err != CL_SUCCESS)
        error("object %s: failed to create buffer for parameters", id);
    
    // setup and run kernel to get parameters
    params_name = kernel_name("params_", name);
    params_kernel = clCreateKernel(program, params_name, &err);
    if(err != CL_SUCCESS)
        error("object %s: failed to create kernel for parameters", id);
    err = clSetKernelArg(params_kernel, 0, sizeof(cl_mem), &params_mem);
    if(err != CL_SUCCESS)
        error("object %s: failed to set kernel arguments for parameters", id);
    err = clEnqueueTask(queue, params_kernel, 0, NULL, NULL);
    if(err != CL_SUCCESS)
        error("object %s: failed to run kernel for parameters", id);
    
    // array for kernel parameters
    params = malloc(obj->npars*sizeof(struct kernel_param));
    if(!params)
        error("object %s: %s", strerror(errno));
    
    // get kernel parameters from buffer
    err = clEnqueueReadBuffer(queue, params_mem, CL_TRUE, 0, obj->npars*sizeof(struct kernel_param), params, 0, NULL, NULL);
    if(err != CL_SUCCESS)
        error("object %s: failed to get parameters", id);
    
    // create array for params
    obj->pars = malloc(obj->npars*sizeof(param));
    if(!obj->pars)
        error("object %s: %s", id, strerror(errno));
    
    // set up parameter array
    for(size_t i = 0; i < obj->npars; ++i)
    {
        // copy name into param
        for(size_t j = 0; j <= sizeof(params[i].name); ++j)
            obj->pars[i].name[j] = params[i].name[j];
        
        // no label is set
        obj->pars[i].label = NULL;
        
        // no prior is set
        obj->pars[i].prior = NULL;
        
        // wrap-around
        obj->pars[i].wrap = params[i].wrap;
    }
    
    // clean up
    clFinish(queue);
    free(params);
    clReleaseKernel(params_kernel);
    free(params_name);
    clReleaseMemObject(params_mem);
    clReleaseKernel(meta_kernel);
    free(meta_name);
    clReleaseMemObject(meta_type_mem);
    clReleaseMemObject(meta_size_mem);
    clReleaseMemObject(meta_npars_mem);
    for(int i = 0; i < nkernels; ++i)
        free((void*)kernels[i]);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
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
    free((char*)par->label);
    free((char*)par->prior);
}

void set_param_label(param* par, const char* label)
{
    // allocate space for string
    par->label = realloc((char*)par->label, strlen(label) + 1);
    if(!par->label)
        error("could not set parameter label \"%s\": %s", label, strerror(errno));
    
    // copy label to param
    strcpy((char*)par->label, label);
}

void set_param_prior(param* par, const char* prior)
{
    // allocate space for string
    par->prior = realloc((char*)par->prior, strlen(prior) + 1);
    if(!par->prior)
        error("could not set parameter prior \"%s\": %s", prior, strerror(errno));
    
    // copy prior to param
    strcpy((char*)par->prior, prior);
}
