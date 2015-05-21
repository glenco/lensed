#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "opencl.h"
#include "log.h"

static void notify(const char* errinfo, const void* private_info,  size_t cb, void* user_data)
{
    verbose("%s", errinfo);
}

static lensed_device* list_devices()
{
    // OpenCL error code
    cl_int err;
    
    // list of devices for lensed
    size_t nlist;
    lensed_device* list;
    lensed_device* item;
    
    // list of platforms and devices
    cl_uint nplatforms;
    cl_platform_id* platforms;
    cl_uint ndevices;
    cl_device_id* devices;
    
    // counter for GPUs and CPUs
    cl_device_type device_type;
    unsigned ncpu = 0;
    unsigned ngpu = 0;
    
    // get all platforms
    clGetPlatformIDs(0, NULL, &nplatforms);
    platforms = malloc(nplatforms*sizeof(cl_platform_id));
    if(!platforms)
        errori(NULL);
    clGetPlatformIDs(nplatforms, platforms, NULL);
    
    // get total number of list items
    nlist = 0;
    for(int i = 0; i < nplatforms; ++i)
    {
        // get number of devices for platform
        err = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_CPU, 0, NULL, &ndevices);
        if(err != CL_SUCCESS)
            continue;
        nlist += ndevices;
    }
    
    // terminating device
    nlist += 1;
    
    // allocate list for all devices
    list = malloc(nlist*sizeof(lensed_device));
    if(!list)
        errori(NULL);
    
    // current list item
    item = list;
    
    // go through platforms and build list
    for(int i = 0; i < nplatforms; ++i)
    {
        // get all devices for platform
        err = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_CPU, 0, NULL, &ndevices);
        if(err != CL_SUCCESS)
            continue;
        devices = malloc(ndevices*sizeof(cl_device_id));
        if(!devices)
            errori(NULL);
        err = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_CPU, ndevices, devices, NULL);
        if(err != CL_SUCCESS)
        {
            free(devices);
            continue;
        }
        
        // query each devices
        for(int j = 0; j < ndevices; ++j)
        {
            // query device type
            err = clGetDeviceInfo(devices[j], CL_DEVICE_TYPE, sizeof(device_type), &device_type, NULL);
            if(err != CL_SUCCESS)
                continue;
            
            // create list item name
            switch(device_type)
            {
                case CL_DEVICE_TYPE_GPU: sprintf(item->name, "gpu%u", ngpu++); break;
                case CL_DEVICE_TYPE_CPU: sprintf(item->name, "cpu%u", ncpu++); break;
                default: break;
            }
            
            // set list item properties
            item->platform_id = platforms[i];
            item->device_id = devices[j];
            item->device_type = device_type;
            
            // next item
            item += 1;
        }
        
        // done with devices for this platform
        free(devices);
    }
    
    // done with platforms
    free(platforms);
    
    // terminate list of devices
    item->platform_id = NULL;
    item->device_id = NULL;
    
    // return list of devices for lensed
    return list;
}

lensed_device* get_lensed_devices()
{
    static lensed_device* list = NULL;
    if(list == NULL)
        list = list_devices();
    return list;
}

lensed_cl* get_lensed_cl(const char* device_str)
{
    // OpenCL error code
    cl_int err;
    
    // result
    lensed_cl* lcl = malloc(sizeof(lensed_cl));
    if(!lcl)
        errori(NULL);
    
    // get list of devices
    lensed_device* device = get_lensed_devices();
    
    // make sure there is a device
    if(!device->device_id)
        error("no devices found");
    
    // if name was given, look for specific device
    if(device_str)
    {
        // check for auto-selection
        if(strcmp(device_str, "auto") == 0)
        {
            // try to find GPU device
            for(; device->device_id; ++device)
                if(device->device_type == CL_DEVICE_TYPE_GPU)
                    break;
            
            // if no GPU was found, use first device
            if(!device->device_id)
                device = get_lensed_devices();
        }
        else
        {
            // find particular device
            for(; device->device_id; ++device)
                if(strcmp(device_str, device->name) == 0)
                    break;
            
            // check if device was found
            if(!device->device_id)
                error("no such device: %s (see `lensed --devices` for a list of devices)", device_str);
        }
    }
    
    // set platform and device IDs of device
    lcl->platform_id = device->platform_id;
    lcl->device_id = device->device_id;
    
    // selected platform
    cl_context_properties properties[] = {
        CL_CONTEXT_PLATFORM,
        (cl_context_properties)lcl->platform_id,
        0
    };
    
    // create the context
    lcl->context = clCreateContext(properties, 1, &lcl->device_id, notify, NULL, &err);
    if(err != CL_SUCCESS)
        error("failed to create device context");
    
    // return the OpenCL environment
    return lcl;
}

void free_lensed_cl(lensed_cl* lcl)
{
    clReleaseContext(lcl->context);
}
