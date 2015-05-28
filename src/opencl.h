#pragma once

// OpenCL headers
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

// compatibility
#ifndef CL_VERSION_1_2
#define CL_MEM_HOST_WRITE_ONLY 0
#define CL_MEM_HOST_READ_ONLY 0
#define CL_MEM_HOST_NO_ACCESS 0
#endif

// OpenCL device
typedef struct
{
    char name[10];
    cl_platform_id platform_id;
    cl_device_id device_id;
    cl_device_type device_type;
} lensed_device;

// OpenCL environment
typedef struct
{
    cl_platform_id platform_id;
    cl_device_id device_id;
    cl_context context;
} lensed_cl;

// list available OpenCL devices
lensed_device* get_lensed_devices();

// get Lensed OpenCL environment
lensed_cl* get_lensed_cl(const char* device_str);

// free Lensed OpenCL environment
void free_lensed_cl(lensed_cl*);
