#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "kernel.h"
#include "log.h"

static const char* BEFORE[] = {
    "object",
    "constants"
};

static const char* AFTER[] = {
    "lensed"
};

static const char* load_kernel(const char* name)
{
    char* filename;
    char* contents;
    long size;
    FILE* f;
    
    filename = malloc(strlen(KERNEL_PATH) + strlen(name) + strlen(KERNEL_EXT) + 1);
    sprintf(filename, "%s%s%s", KERNEL_PATH, name, KERNEL_EXT);
    
    f = fopen(filename, "rb");
    
    if(!f)
    {
        verbose("file not found: %s", filename);
        error("could not load kernel \"%s\"", name);
    }
    
    fseek(f, 0, SEEK_END);
    
    size = ftell(f);
    contents = malloc(size + 1);
    
    if(!contents)
        error("could not allocate memory for kernel \"%s\"", name);
    
    fseek(f, 0, SEEK_SET);
    fread(contents, 1, size, f);
    contents[size] = 0;
    fclose(f);
    
    free(filename);
    
    return contents;
}

// load kernels for objects
void load_kernels(size_t nobjects, const char* objects[],
                  size_t* nkernels, const char*** kernels)
{
    // create an array of unique object names
    size_t nunique = nobjects;
    const char** unique = malloc(nobjects*sizeof(const char*));
    memcpy(unique, objects, nobjects*sizeof(const char*));
    qsort(unique, nunique, sizeof(const char*), (int (*)(const void*, const void*))strcmp);
    for(size_t i = 0; i < nunique; ++i)
    {
        size_t end = i + 1;
        while(end < nunique && strcmp(unique[i], unique[end]) == 0) ++end;
        if(end == i + 1)
            continue;
        for(size_t j = 0; j < (nunique - end); ++j)
            unique[i + 1 + j] = unique[end + j];
        nunique -= end - (i + 1);
    }
    
    // number of system kernels
    size_t nbefore = sizeof(BEFORE)/sizeof(BEFORE[0]);
    size_t nafter = sizeof(AFTER)/sizeof(AFTER[0]);
    
    // create kernel array with space for object and system kernels
    *nkernels = nbefore + nunique + nafter;
    *kernels = malloc((*nkernels)*sizeof(const char*));
    
    const char** k = *kernels;
    
    // load system kernels
    for(size_t i = 0; i < nbefore; ++i)
    {
        verbose("    load %s", BEFORE[i]);
        *(k++) = load_kernel(BEFORE[i]);
    }
    
    // load kernels for objects
    for(size_t i = 0; i < nunique; ++i)
    {
        verbose("    load %s", unique[i]);
        *(k++) = load_kernel(unique[i]);
    }
    
    // load system kernels
    for(size_t i = 0; i < nafter; ++i)
    {
        verbose("    load %s", AFTER[i]);
        *(k++) = load_kernel(AFTER[i]);
    }
    
    // free array of unique object names
    free(unique);
}

char* kernel_name(const char* prefix, const char* name)
{
    char* buf = malloc(strlen(prefix) + strlen(name) + 1);
    strcpy(buf, prefix);
    strcat(buf, name);
    return buf;
}
