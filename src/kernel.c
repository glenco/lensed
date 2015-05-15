#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "input.h"
#include "config.h"
#include "kernel.h"
#include "log.h"

// parts of kernel files
static const char KERNEL_DIR[] = "kernel/";
static const char KERNEL_EXT[] = ".cl";

// marker for individual files in kernel code
static const char FILEHEAD[] = 
    "//----------------------------------------------------------------------------\n"
    "// %s\n"
    "//----------------------------------------------------------------------------\n"
    "\n"
;
static const char FILEFOOT[] =
    "\n"
;

// kernels that are needed for initialising programs
static const char* INITKERNS[] = {
    "object",
    "constants"
};
static const size_t NINITKERNS = sizeof(INITKERNS)/sizeof(INITKERNS[0]);

// kernels that are needed for main program
static const char* MAINKERNS[] = {
    "lensed"
};
static const size_t NMAINKERNS = sizeof(MAINKERNS)/sizeof(MAINKERNS[0]);

// kernel to get meta-data for object
static const char METAKERN[] = 
    "kernel void meta_<name>(global int* type, global ulong* size, global ulong* npars)\n"
    "{\n"
    "    *type = object_<name>;\n"
    "    *size = sizeof(struct <name>);\n"
    "    *npars = NPARAMS(<name>);\n"
    "}\n"
;

// kernel to get parameters for object
static const char PARSKERN[] = 
    "kernel void params_<name>(global struct param* params)\n"
    "{\n"
    "    for(size_t i = 0; i < NPARAMS(<name>); ++i)\n"
    "    {\n"
    "        for(size_t j = 0; j < sizeof(params[i].name); ++j)\n"
    "            params[i].name[j] = PARAM(<name>, i).name[j];\n"
    "        \n"
    "        params[i].wrap = PARAM(<name>, i).wrap;\n"
    "    }\n"
    "}\n"
;

// kernel to compute images
static const char COMPHEAD[] =
    "static float compute(constant char* data, float2 x)\n"
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
    "    a += %s((constant void*)(data + %zu), y);\n"
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
    "    f += %s((constant void*)(data + %zu), y);\n"
;
static const char COMPFHED[] =
    "    \n"
    "    // add foreground\n"
;
static const char COMPFGND[] =
    "    f += %s((constant void*)(data + %zu), x);\n"
;
static const char COMPFOOT[] =
    "    \n"
    "    // return total surface brightness\n"
    "    return f;\n"
    "}\n"
;

// kernel to set parameters
static const char SETPHEAD[] =
    "kernel void set_params(global char* data, constant float* params)\n"
    "{\n"
;
static const char SETPLEFT[] = "    set_%s((global void*)(data + %zu)";
static const char SETPARGS[] = ", params[%zu]";
static const char SETPRGHT[] = ");\n";
static const char SETPFOOT[] =
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
        errori(NULL);
    
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
    
    // trigger for changing lens planes
    int trigger;
    
    // buffer for kernel
    size_t buf_size;
    char* buf;
    
    // current output position
    char* out;
    
    // offset in data block
    size_t d;
    
    // number of characters added
    int wri;
    
    // calculate buffer size
    d = 0;
    type = trigger = 0;
    buf_size = sizeof(FILEHEAD) + strlen("compute");
    buf_size += sizeof(COMPHEAD);
    for(size_t i = 0; i < nobjs; ++i)
    {
        if(objs[i].type != trigger && objs[i].type != OBJ_FOREGROUND)
        {
            if(trigger == OBJ_LENS)
                buf_size += sizeof(COMPDEFL);
            trigger = objs[i].type;
        }
        if(objs[i].type != type)
        {
            if(objs[i].type == OBJ_LENS)
                buf_size += sizeof(COMPLHED);
            else if(objs[i].type == OBJ_SOURCE)
                buf_size += sizeof(COMPSHED);
            else if(objs[i].type == OBJ_FOREGROUND)
                buf_size += sizeof(COMPFHED);
            type = objs[i].type;
        }
        if(type == OBJ_LENS)
            buf_size += sizeof(COMPLENS);
        else if(type == OBJ_SOURCE)
            buf_size += sizeof(COMPSRCE);
        else if(type == OBJ_FOREGROUND)
            buf_size += sizeof(COMPFGND);
        buf_size += strlen(objs[i].name);
        buf_size += log10(1+d);
        d += objs[i].size;
    }
    buf_size += sizeof(COMPFOOT);
    buf_size += sizeof(FILEFOOT);
    
    // allocate buffer
    buf = malloc(buf_size);
    if(!buf)
        errori(NULL);
    
    // start at beginning of data block
    d = 0;
    
    // start with invalid type
    type = trigger = 0;
    
    // output tracks current writing position on buffer
    out = buf;
    
    // write file header
    wri = sprintf(out, FILEHEAD, "compute");
    if(wri < 0)
        errori(NULL);
    out += wri;
    
    // write header
    wri = sprintf(out, COMPHEAD);
    if(wri < 0)
        errori(NULL);
    out += wri;
    
    // write body
    for(size_t i = 0; i < nobjs; ++i)
    {
        // check if lens plane change is triggered
        if(objs[i].type != trigger && objs[i].type != OBJ_FOREGROUND)
        {
            // when triggering from lenses to sources, apply deflection
            if(trigger == OBJ_LENS)
            {
                wri = sprintf(out, COMPDEFL);
                if(wri < 0)
                    errori(NULL);
                out += wri;
            }
            
            // new trigger
            trigger = objs[i].type;
        }
        
        // check if type of object changed
        if(objs[i].type != type)
        {
            // write header
            if(objs[i].type == OBJ_LENS)
                wri = sprintf(out, COMPLHED);
            else if(objs[i].type == OBJ_SOURCE)
                wri = sprintf(out, COMPSHED);
            else if(objs[i].type == OBJ_FOREGROUND)
                wri = sprintf(out, COMPFHED);
            else
                wri = 0;
            if(wri < 0)
                errori(NULL);
            out += wri;
            
            // new type
            type = objs[i].type;
        }
        
        // write line for current object
        if(type == OBJ_LENS)
            wri = sprintf(out, COMPLENS, objs[i].name, d);
        else if(type == OBJ_SOURCE)
            wri = sprintf(out, COMPSRCE, objs[i].name, d);
        else if(type == OBJ_FOREGROUND)
            wri = sprintf(out, COMPFGND, objs[i].name, d);
        else
            wri = 0;
        if(wri < 0)
            errori(NULL);
        out += wri;
        
        // advance data pointer
        d += objs[i].size;
    }
    
    // write footer
    wri = sprintf(out, COMPFOOT);
    if(wri < 0)
        errori(NULL);
    out += wri;
    
    // write file footer
    wri = sprintf(out, FILEFOOT);
    if(wri < 0)
        errori(NULL);
    out += wri;
    
    // this is our code
    return buf;
}

static const char* set_params_kernel(size_t nobjs, object objs[])
{
    // buffer for kernel
    size_t buf_size;
    char* buf;
    
    // current output position
    char* out;
    
    // number of characters added
    int wri;
    
    // data offset
    size_t d;
    
    // parameter offset
    size_t p;
    
    // calculate buffer size
    d = p = 0;
    buf_size = sizeof(FILEHEAD) + strlen("set_params");
    buf_size += sizeof(SETPHEAD);
    for(size_t i = 0; i < nobjs; ++i)
    {
        buf_size += sizeof(SETPLEFT);
        buf_size += strlen(objs[i].name);
        buf_size += log10(1+d);
        for(size_t j = 0; j < objs[i].npars; ++j)
            buf_size += sizeof(SETPARGS) + log10(1+p+j);
        buf_size += sizeof(SETPRGHT);
        d += objs[i].size;
        p += objs[i].npars;
    }
    buf_size += sizeof(SETPFOOT);
    buf_size += sizeof(FILEFOOT);
    
    // allocate buffer
    buf = malloc(buf_size);
    if(!buf)
        errori(NULL);
    
    // start at beginning of data and parameters
    p = d = 0;
    
    // output tracks current writing position on buffer
    out = buf;
    
    // write file header
    wri = sprintf(out, FILEHEAD, "set_params");
    if(wri < 0)
        errori(NULL);
    out += wri;
    
    // write header
    wri = sprintf(out, SETPHEAD);
    if(wri < 0)
        errori(NULL);
    out += wri;
    
    // write body
    for(size_t i = 0; i < nobjs; ++i)
    {
        // write left side of line
        wri = sprintf(out, SETPLEFT, objs[i].name, d);
        if(wri < 0)
            errori(NULL);
        out += wri;
        
        // write arguments
        for(size_t j = 0; j < objs[i].npars; ++j)
        {
            wri = sprintf(out, SETPARGS, p+j);
            if(wri < 0)
                errori(NULL);
            out += wri;
        }
        
        // write left side of line
        wri = sprintf(out, SETPRGHT);
        if(wri < 0)
            errori(NULL);
        out += wri;
        
        // increase offsets
        d += objs[i].size;
        p += objs[i].npars;
    }
    
    // write footer
    wri = sprintf(out, SETPFOOT);
    if(wri < 0)
        errori(NULL);
    out += wri;
    
    // write file footer
    wri = sprintf(out, FILEFOOT);
    if(wri < 0)
        errori(NULL);
    out += wri;
    
    // this is our code
    return buf;
}

static const char* load_kernel(const char* name)
{
    // file for kernel
    char* filename;
    FILE* f;
    long file_size;
    
    // buffer for kernel code
    char* buf;
    size_t buf_size;
    
    // current output position
    char* out;
    
    // number of characters added
    int wri;
    
    // construct filename for kernel
    filename = malloc(strlen(LENSED_PATH) + strlen(KERNEL_DIR) + strlen(name) + strlen(KERNEL_EXT) + 1);
    sprintf(filename, "%s%s%s%s", LENSED_PATH, KERNEL_DIR, name, KERNEL_EXT);
    
    // try to read file
    f = fopen(filename, "r");
    if(!f)
    {
        verbose("file not found: %s", filename);
        error("could not load kernel \"%s\"", name);
    }
    
    // go to end of file and get its size
    fseek(f, 0, SEEK_END);
    file_size = ftell(f);
    
    // calculate size of buffer
    buf_size = file_size + sizeof(FILEHEAD) + strlen(name) + sizeof(FILEFOOT);
    
    // try to allocate buffer
    buf = malloc(buf_size);
    if(!buf)
        errori("kernel %s", name);
    
    // start at beginning
    out = buf;
    
    // write file header
    wri = sprintf(out, FILEHEAD, name);
    if(wri < 0)
        errori("kernel %s", name);
    out += wri;
    
    // write file
    fseek(f, 0, SEEK_SET);
    fread(out, 1, file_size, f);
    out += file_size;
    
    // write file footer
    wri = sprintf(out, FILEFOOT);
    if(wri < 0)
        errori("kernel %s", name);
    out += wri;
    
    // clean up
    fclose(f);
    free(filename);
    
    // done
    return buf;
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
    size_t nuniq = 0;
    const char** uniq = malloc(nobjs*sizeof(const char*));
    for(size_t i = 0; i < nobjs; ++i)
    {
        int isuniq = 1;
        for(size_t j = 0; isuniq && j < nuniq; ++j)
            if(strcmp(uniq[j], objs[i].name) == 0)
                isuniq = 0;
        if(isuniq)
            uniq[nuniq++] = objs[i].name;
    }
    
    // create kernel array
    *nkernels = NINITKERNS + nuniq + 2 + NMAINKERNS;
    *kernels = malloc((*nkernels)*sizeof(const char*));
    
    const char** k = *kernels;
    
    // load initialisation kernels
    for(size_t i = 0; i < NINITKERNS; ++i)
        *(k++) = load_kernel(INITKERNS[i]);
    
    // load kernels for objects
    for(size_t i = 0; i < nuniq; ++i)
        *(k++) = load_kernel(uniq[i]);
    
    // load compute kernel
    *(k++) = compute_kernel(nobjs, objs);
    
    // load parameter setter kernel
    *(k++) = set_params_kernel(nobjs, objs);
    
    // load main kernels
    for(size_t i = 0; i < NMAINKERNS; ++i)
        *(k++) = load_kernel(MAINKERNS[i]);
    
    // free array of unique object names
    free(uniq);
}

const char* kernel_options(size_t width, size_t height, int psf, size_t psfw, size_t psfh, size_t nq, const char* flags[])
{
    size_t nopts;
    size_t opts_size;
    char* opts;
    char* cur;
    const char** f;
    
    // hard-coded options
    const char* size_opt = " -DIMAGE_SIZE=%zu";
    const char* width_opt = " -DIMAGE_WIDTH=%zu";
    const char* height_opt = " -DIMAGE_HEIGHT=%zu";
    const char* psf_opt = " -DPSF=%d";
    const char* psfw_opt = " -DPSF_WIDTH=%zu";
    const char* psfh_opt = " -DPSF_HEIGHT=%zu";
    const char* nq_opt = " -DQUAD_POINTS=%zu";
    
    // get number of options and their sizes
    opts_size = 0;
    nopts = 0;
    opts_size += strlen(size_opt) + log10(width*height + 1) + 1;
    nopts += 1;
    opts_size += strlen(width_opt) + log10(width + 1) + 1;
    nopts += 1;
    opts_size += strlen(height_opt) + log10(height + 1) + 1;
    nopts += 1;
    opts_size += strlen(psf_opt) + 1 + 1;
    nopts += 1;
    opts_size += strlen(psfw_opt) + log10(psfw + 1) + 1;
    nopts += 1;
    opts_size += strlen(psfh_opt) + log10(psfh + 1) + 1;
    nopts += 1;
    opts_size += strlen(nq_opt) + log10(nq + 1) + 1;
    nopts += 1;
    for(f = flags; *f; ++f)
    {
        opts_size += strlen(*f) + 1;
        nopts += 1;
    }
    
    // allocate buffer for options
    opts = malloc(opts_size + 1);
    if(!opts)
        errori(NULL);
    
    // pointer to current writing position
    cur = opts;
    
    // write hard-coded options
    cur += sprintf(cur, size_opt, width*height);
    cur += sprintf(cur, width_opt, width);
    cur += sprintf(cur, height_opt, height);
    cur += sprintf(cur, psf_opt, psf ? 1 : 0);
    cur += sprintf(cur, psfw_opt, psfw);
    cur += sprintf(cur, psfh_opt, psfh);
    cur += sprintf(cur, nq_opt, nq);
    
    // add flags
    for(f = flags; *f; ++f)
        cur += sprintf(cur, " %s", *f);
    
    // return build options string
    return opts;
}

char* kernel_name(const char* prefix, const char* name)
{
    char* buf = malloc(strlen(prefix) + strlen(name) + 1);
    strcpy(buf, prefix);
    strcat(buf, name);
    return buf;
}
