#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "config.h"
#include "kernel.h"
#include "log.h"

static const char* BEFORE[] = {
    "object",
    "constants",
    "params"
};

static const size_t NBEFORE = sizeof(BEFORE)/sizeof(BEFORE[0]);

static const char* AFTER[] = {
    "lensed"
};

static const size_t NAFTER = sizeof(AFTER)/sizeof(AFTER[0]);

static const char META_KERNEL[] = 
    "kernel void meta_<name>(global int* type, global ulong* npars)\n"
    "{\n"
    "    *type = object_<name>;\n"
    "    *npars = NPARAMS(<name>);\n"
    "}\n"
;

static const char PARAMS_KERNEL[] = 
    "kernel void params_<name>(global struct param* params)\n"
    "{\n"
    "    for(size_t i = 0; i < NPARAMS(<name>); ++i)\n"
    "        parcpy(&params[i], &PARAM(<name>, i));\n"
    "}\n"
;

static const char* str_replace(const char* str, const char* search, const char* replace)
{
    char* buf;
    const char* pos;
    const char* in;
    char* out;
    size_t count;
    size_t buf_size;
    
    long slen = strlen(search);
    long rlen = strlen(replace);
    
    // step 1: count how often search occurs in str
    count = 0;
    pos = str;
    while((pos = strstr(pos, search)))
    {
        count += 1;
        pos += slen;
    }
    
    // step 2: calculate size of new string
    buf_size = (long)strlen(str) + count*(rlen - slen) + 1;
    
    // step 3: allocate buffer
    buf = malloc(buf_size);
    if(!buf)
        error("%s", strerror(errno));
    
    // step 4: copy string and replace
    pos = in = str;
    out = buf;
    while((pos = strstr(pos, search)))
    {
        // copy str from input position to newly found position
        strncpy(out, in, pos - in);
        
        // advance output pointer
        out += pos - in;
        
        // copy replace part
        strcpy(out, replace);
        
        // skip replace part
        out += rlen;
        
        // skip search part
        pos += slen;
        
        // set new input position
        in = pos;
    }
    
    // step 5: copy remaining part of str
    strcpy(out, in);
    
    // done
    return buf;
}

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

void object_kernel(const char* name, size_t* nkernels, const char*** kernels)
{
    // create kernel array with space for object and system kernels
    *nkernels = NBEFORE + 1 + 2;
    *kernels = malloc((*nkernels)*sizeof(const char*));
    
    const char** k = *kernels;
    
    // load system kernels
    for(size_t i = 0; i < NBEFORE; ++i)
        *(k++) = load_kernel(BEFORE[i]);
    
    // load kernel for object
    *(k++) = load_kernel(name);
    
    // add kernels for object meta-data and parameters
    *(k++) = str_replace(META_KERNEL, "<name>", name);
    *(k++) = str_replace(PARAMS_KERNEL, "<name>", name);
}

void load_kernels(size_t nobjects, const char* objects[], size_t* nkernels, const char*** kernels)
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
    
    // create kernel array with space for object and system kernels
    *nkernels = NBEFORE + nunique + NAFTER;
    *kernels = malloc((*nkernels)*sizeof(const char*));
    
    const char** k = *kernels;
    
    // load system kernels
    for(size_t i = 0; i < NBEFORE; ++i)
        *(k++) = load_kernel(BEFORE[i]);
    
    // load kernels for objects
    for(size_t i = 0; i < nunique; ++i)
        *(k++) = load_kernel(unique[i]);
    
    // load system kernels
    for(size_t i = 0; i < NAFTER; ++i)
        *(k++) = load_kernel(AFTER[i]);
    
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
