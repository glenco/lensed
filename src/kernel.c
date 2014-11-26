#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include "input.h"
#include "config.h"
#include "kernel.h"
#include "log.h"

// kernels that are needed for initialising programs
static const char* INITKERNS[] = {
    "object",
    "constants",
    "params"
};
static const size_t NINITKERNS = sizeof(INITKERNS)/sizeof(INITKERNS[0]);

// kernels that are needed for main program
static const char* MAINKERNS[] = {
    "lensed"
};
static const size_t NMAINKERNS = sizeof(MAINKERNS)/sizeof(MAINKERNS[0]);

// kernel to get meta-data for object
static const char METAKERN[] = 
    "kernel void meta_<name>(global int* type, global ulong* npars)\n"
    "{\n"
    "    *type = object_<name>;\n"
    "    *npars = NPARAMS(<name>);\n"
    "}\n"
;

// kernel to get parameters for object
static const char PARSKERN[] = 
    "kernel void params_<name>(global struct param* params)\n"
    "{\n"
    "    for(size_t i = 0; i < NPARAMS(<name>); ++i)\n"
    "        parcpy(&params[i], &PARAM(<name>, i));\n"
    "}\n"
;

// kernel to compute images
static const char COMPHEAD[] =
    "static float compute(constant object* objects, float2 x)\n"
    "{\n"
    "    // initial ray position\n"
    "    float2 y = x;\n"
    "    \n"
    "    // initial deflection is zero\n"
    "    float2 a = 0;\n"
    "    \n"
    "    // initial surface brightness is zero\n"
    "    float f = 0;\n"
;
static const char COMPLHED[] =
    "    \n"
    "    // calculate deflection\n"
;
static const char COMPLENS[] =
    "    a += %s(objects + %zu, y);\n"
;
static const char COMPDEFL[] =
    "    \n"
    "    // apply deflection to ray\n"
    "    y -= a;\n"
;
static const char COMPSHED[] =
    "    \n"
    "    // calculate surface brightness\n"
;
static const char COMPSRCE[] =
    "    f += %s(objects + %zu, y);\n"
;
static const char COMPFOOT[] =
    "    \n"
    "    // return total surface brightness\n"
    "    return f;\n"
    "}\n"
;

// replace substring, used to fill in object names in kernels
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

static const char* compute_kernel(size_t nobjs, object objs[])
{
    // object type currently processed
    int type;
    
    // buffer for kernel
    size_t buf_size;
    char* buf;
    
    // current output position
    char* out;
    
    // number of characters added
    int wri;
    
    // calculate buffer size
    type = 0;
    buf_size = sizeof(COMPHEAD);
    for(size_t i = 0; i < nobjs; ++i)
    {
        if(objs[i].type != type)
        {
            if(type == OBJ_LENS)
                buf_size += sizeof(COMPDEFL);
            if(objs[i].type == OBJ_LENS)
                buf_size += sizeof(COMPLHED);
            else
                buf_size += sizeof(COMPSHED);
            type = objs[i].type;
        }
        if(type == OBJ_LENS)
            buf_size += sizeof(COMPLENS);
        else
            buf_size += sizeof(COMPSRCE);
        buf_size += strlen(objs[i].name);
        buf_size += log10(1+i);
    }
    buf_size += sizeof(COMPFOOT);
    
    // allocate buffer
    buf = malloc(buf_size);
    if(!buf)
        error("%s", strerror(errno));
    
    // start with invalid type
    type = 0;
    
    // output tracks current writing position on buffer
    out = buf;
    
    // write header
    wri = sprintf(out, COMPHEAD);
    if(wri < 0)
        error("%s", strerror(errno));
    out += wri;
    
    // write body
    for(size_t i = 0; i < nobjs; ++i)
    {
        // check if type of object changed
        if(objs[i].type != type)
        {
            // when going from lenses to sources, apply deflection
            if(type == OBJ_LENS)
            {
                wri = sprintf(out, COMPDEFL);
                if(wri < 0)
                    error("%s", strerror(errno));
                out += wri;
            }
            
            // write header
            if(objs[i].type == OBJ_LENS)
                wri = sprintf(out, COMPLHED);
            else
                wri = sprintf(out, COMPSHED);
            if(wri < 0)
                error("%s", strerror(errno));
            out += wri;
            
            // new type
            type = objs[i].type;
        }
        
        // write line for current object
        if(type == OBJ_LENS)
            wri = sprintf(out, COMPLENS, objs[i].name, i);
        else
            wri = sprintf(out, COMPSRCE, objs[i].name, i);
        if(wri < 0)
            error("%s", strerror(errno));
        out += wri;
    }
    
    // write footer
    wri = sprintf(out, COMPFOOT);
    if(wri < 0)
        error("%s", strerror(errno));
    out += wri;
    
    // this is our code
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

void object_program(const char* name, size_t* nkernels, const char*** kernels)
{
    // create kernel array with space for object and system kernels
    *nkernels = NINITKERNS + 1 + 2;
    *kernels = malloc((*nkernels)*sizeof(const char*));
    
    const char** k = *kernels;
    
    // load initialisation kernels
    for(size_t i = 0; i < NINITKERNS; ++i)
        *(k++) = load_kernel(INITKERNS[i]);
    
    // load kernel for object
    *(k++) = load_kernel(name);
    
    // add kernels for object meta-data and parameters
    *(k++) = str_replace(METAKERN, "<name>", name);
    *(k++) = str_replace(PARSKERN, "<name>", name);
}

void main_program(size_t nobjs, object objs[], size_t* nkernels, const char*** kernels)
{
    // create an array of unique object names
    size_t nunique = nobjs;
    const char** unique = malloc(nobjs*sizeof(const char*));
    for(size_t i = 0; i < nobjs; ++i)
        unique[i] = objs[i].name;
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
    
    // create kernel array
    *nkernels = NINITKERNS + nunique + 1 + NMAINKERNS;
    *kernels = malloc((*nkernels)*sizeof(const char*));
    
    const char** k = *kernels;
    
    // load initialisation kernels
    for(size_t i = 0; i < NINITKERNS; ++i)
        *(k++) = load_kernel(INITKERNS[i]);
    
    // load kernels for objects
    for(size_t i = 0; i < nunique; ++i)
        *(k++) = load_kernel(unique[i]);
    
    // load compute kernel
    *(k++) = compute_kernel(nobjs, objs);
    
    // load main kernels
    for(size_t i = 0; i < NMAINKERNS; ++i)
        *(k++) = load_kernel(MAINKERNS[i]);
    
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
