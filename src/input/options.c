#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../input.h"
#include "options.h"
#include "../parse.h"
#include "../log.h"

// information about input option
struct option
{
    const char* name;
    const char* help;
    int required;
    size_t depend;
    const char* type;
    int (*read)(void*, const char*);
    int (*write)(char*, const void*, size_t);
    void (*free)(void*);
    union {
        void* default_null;
        const char* default_string;
        int default_bool;
        int default_int;
        double default_real;
        const char* default_path;
        struct gain* default_gain;
    } default_value;
    size_t offset;
    size_t size;
};

// mark option as required or optional
#define OPTION_REQUIRED(type) 1, 0, #type, option_read_##type, option_write_##type, option_free_##type, { .default_null = NULL }
#define OPTION_OPTIONAL(type, value) 0, 0, #type, option_read_##type, option_write_##type, option_free_##type, { .default_##type = value }
#define OPTION_REQIFSET(type, value, depend) 2, offsetof(options, depend), #type, option_read_##type, option_write_##type, option_free_##type, { .default_##type = value }
#define OPTION_REQIFNOT(type, value, depend) 3, offsetof(options, depend), #type, option_read_##type, option_write_##type, option_free_##type, { .default_##type = value }

// get offset of option field in input
#define OPTION_FIELD(field) offsetof(options, field), sizeof(((options*)0)->field)

// get status of dependency
#define OPTION_ISSET(options, offset) (*(int*)((char*)options + offset))
#define OPTION_ISNOT(options, offset) (!*(int*)((char*)options + offset))

// declare option types
#define OPTION_TYPE(type) \
    static int option_read_##type(void*, const char*); \
    static int option_write_##type(char*, const void*, size_t); \
    static void option_free_##type(void*);

// list known option types
OPTION_TYPE(string)
OPTION_TYPE(bool)
OPTION_TYPE(int)
OPTION_TYPE(real)
OPTION_TYPE(path)
OPTION_TYPE(gain)

// list of known options
struct option OPTIONS[] = {
    {
        "gpu",
        "Enable computations on GPU",
        OPTION_OPTIONAL(bool, 1),
        OPTION_FIELD(gpu)
    },
    {
        "output",
        "Output results",
        OPTION_OPTIONAL(bool, 1),
        OPTION_FIELD(output)
    },
    {
        "root",
        "Root element for all output paths",
        OPTION_REQIFSET(string, NULL, output),
        OPTION_FIELD(root)
    },
    {
        "image",
        "Input image, FITS file in counts/sec",
        OPTION_REQUIRED(path),
        OPTION_FIELD(image)
    },
    {
        "gain",
        "Conversion factor to counts",
        OPTION_REQIFNOT(gain, NULL, weight),
        OPTION_FIELD(gain)
    },
    {
        "offset",
        "Subtracted flat-field offset",
        OPTION_OPTIONAL(real, 0),
        OPTION_FIELD(offset)
    },
    {
        "weight",
        "Weight map in 1/(counts/sec)^2",
        OPTION_OPTIONAL(path, NULL),
        OPTION_FIELD(weight)
    },
    {
        "mask",
        "Input mask, FITS file",
        OPTION_OPTIONAL(path, NULL),
        OPTION_FIELD(mask)
    },
    {
        "psf",
        "Point-spread function, FITS file",
        OPTION_OPTIONAL(path, NULL),
        OPTION_FIELD(psf)
    },
    {
        "nquad",
        "Number of quadrature points",
        OPTION_OPTIONAL(int, 7),
        OPTION_FIELD(nquad)
    },
    {
        "nlive",
        "Number of live points",
        OPTION_OPTIONAL(int, 300),
        OPTION_FIELD(nlive)
    },
    {
        "ins",
        "Use importance nested sampling",
        OPTION_OPTIONAL(bool, 1),
        OPTION_FIELD(ins)
    },
    {
        "mmodal",
        "Mode separation (if ins = false)",
        OPTION_OPTIONAL(bool, 1),
        OPTION_FIELD(mmodal)
    },
    {
        "ceff",
        "Constant efficiency mode",
        OPTION_OPTIONAL(bool, 1),
        OPTION_FIELD(ceff)
    },
    {
        "acc",
        "Target acceptance rate",
        OPTION_OPTIONAL(real, 0.05),
        OPTION_FIELD(acc)
    },
    {
        "tol",
        "Tolerance in log-evidence",
        OPTION_OPTIONAL(real, 0.1),
        OPTION_FIELD(tol)
    },
    {
        "shf",
        "Shrinking factor",
        OPTION_OPTIONAL(real, 0.8),
        OPTION_FIELD(shf)
    },
    {
        "maxmodes",
        "Maximum number of expected modes",
        OPTION_OPTIONAL(int, 100),
        OPTION_FIELD(maxmodes)
    },
    {
        "updint",
        "Update interval for output",
        OPTION_OPTIONAL(int, 1000),
        OPTION_FIELD(updint)
    },
    {
        "seed",
        "Random number seed for sampling",
        OPTION_OPTIONAL(int, -1),
        OPTION_FIELD(seed)
    },
    {
        "resume",
        "Resume from last checkpoint",
        OPTION_OPTIONAL(bool, 0),
        OPTION_FIELD(resume),
    },
    {
        "maxiter",
        "Maximum number of iterations",
        OPTION_OPTIONAL(int, 0),
        OPTION_FIELD(maxiter)
    }
};

// number of known options
#define NOPTIONS (sizeof(OPTIONS)/sizeof(struct option))

// last error message
static char ERROR_MSG[1024] = {0};

// current path for option reading
static const char* OPTIONS_CWD = NULL;

options* create_options()
{
    options* opts = malloc(sizeof(options));
    if(!opts)
        errori(NULL);
    memset(opts, 0, sizeof(options));
    return opts;
}

void free_options(options* opts)
{
    for(int i = 0; i < NOPTIONS; ++i)
        OPTIONS[i].free((char*)opts + OPTIONS[i].offset);
    free(opts);
}

void default_options(input* inp)
{
    for(int i = 0; i < NOPTIONS; ++i)
    {
        // copy default value to input
        memcpy((char*)inp->opts + OPTIONS[i].offset, &OPTIONS[i].default_value, OPTIONS[i].size);
        
        // set required flag for option
        inp->reqs[i] = OPTIONS[i].required;
    }
}

int read_option(input* inp, const char* name, const char* value)
{
    // read all chars in name
    return read_option_n(inp, name, strlen(name), value);
}

int read_option_n(input* inp, const char* name, int n, const char* value)
{
    int opt;
    
    // error if no name was given
    if(n == 0)
    {
        snprintf(ERROR_MSG, sizeof(ERROR_MSG)-1, "no option given");
        return 1;
    }
    
    // find option
    for(opt = 0; opt < NOPTIONS; ++opt)
        if(strncmp(name, OPTIONS[opt].name, n) == 0)
            break;
    
    // error if option was not found
    if(opt == NOPTIONS)
    {
        snprintf(ERROR_MSG, sizeof(ERROR_MSG)-1, "invalid option: %.*s", n, name);
        return 1;
    }
    
    // error if no value was given
    if(!value || !*value)
    {
        snprintf(ERROR_MSG, sizeof(ERROR_MSG)-1, "option %.*s: no value given", n, name);
        return 1;
    }
    
    // try to read option and return eventual errors
    if(OPTIONS[opt].read((char*)inp->opts + OPTIONS[opt].offset, value))
    {
        snprintf(ERROR_MSG, sizeof(ERROR_MSG)-1, "option %.*s: invalid value: %s (should be %s)", n, name, value, OPTIONS[opt].type);
        return 1;
    }
    
    // option is no longer required
    inp->reqs[opt] = 0;
    
    // report success
    return 0;
}

const char* options_error()
{
    return ERROR_MSG;
}

size_t noptions()
{
    return NOPTIONS;
}

const char* option_name(size_t n)
{
    return OPTIONS[n].name;
}

const char* option_type(size_t n)
{
    return OPTIONS[n].type;
}

const char* option_help(size_t n)
{
    return OPTIONS[n].help;
}

int option_required(size_t n)
{
    return OPTIONS[n].required;
}

int option_resolved(size_t n, options* opts, int reqs[])
{
    int res = 0;
    
    if(reqs[n] == 0)
        res = 1;
    else if(reqs[n] == 1)
        res = 0;
    else if(reqs[n] == 2)
        res = !OPTION_ISSET(opts, OPTIONS[n].depend);
    else if(reqs[n] == 3)
        res = !OPTION_ISNOT(opts, OPTIONS[n].depend);
    
    return res;
}

int option_default_value(char* buf, size_t buf_size, size_t n)
{
    return OPTIONS[n].write(buf, &OPTIONS[n].default_value, buf_size);
}

int option_value(char* buf, size_t buf_size, const input* inp, size_t n)
{
    return OPTIONS[n].write(buf, (char*)inp->opts + OPTIONS[n].offset, buf_size);
}

const char* options_cwd(const char* cwd)
{
    const char* old = OPTIONS_CWD;
    OPTIONS_CWD = cwd;
    return old;
}

// wrappers for option reading and writing

int option_read_string(void* out, const char* in)
{
    char** out_char = out;
    *out_char = malloc(strlen(in) + 1);
    strcpy(*out_char, in);
    return 0;
}

int option_write_string(char* out, const void* in, size_t n)
{
    char* const* in_char = in;
    snprintf(out, n, "%s", *in_char ? *in_char : "none");
    return 0;
}

void option_free_string(void* p)
{
    char** str = p;
    free(*str);
}

int option_read_bool(void* out, const char* in)
{
    int* out_bool = out;
    return read_bool(out_bool, in);
}

int option_write_bool(char* out, const void* in, size_t n)
{
    const int* in_bool = in;
    return write_bool(out, *in_bool, n);
}

void option_free_bool(void* p)
{
}

int option_read_int(void* out, const char* in)
{
    int* out_int = out;
    return read_int(out_int, in);
}

int option_write_int(char* out, const void* in, size_t n)
{
    const int* in_int = in;
    return write_int(out, *in_int, n);
}

void option_free_int(void* p)
{
}

int option_read_real(void* out, const char* in)
{
    double* out_real = out;
    return read_real(out_real, in);
}

int option_write_real(char* out, const void* in, size_t n)
{
    const double* in_real = in;
    return write_real(out, *in_real, n);
}

void option_free_real(void* p)
{
}

int option_read_path(void* out, const char* in)
{
    int r;
    char* str;
    char* abs;
    
    // read string
    r = option_read_string(out, in);
    if(r != 0)
        return r;
    str = *(char**)out;
    
    // check for absolute path
    if(str[0] == '/')
        return 0;
    
    // check if there is a cwd for options
    if(OPTIONS_CWD == NULL)
        return 0;
    
    // append relative path to current path
    abs = malloc(strlen(OPTIONS_CWD) + strlen("/") + strlen(str) + 1);
    if(!abs)
        errori(NULL);
    sprintf(abs, "%s%s%s", OPTIONS_CWD, "/", str);
    
    // swap strings
    *(char**)out = abs;
    free(str);
    
    return 0;
}

int option_write_path(char* out, const void* in, size_t n)
{
    return option_write_string(out, in, n);
}

void option_free_path(void* p)
{
    char** str = p;
    free(*str);
}

int option_read_gain(void* out, const char* in)
{
    int err;
    struct gain** g = out;
    
    // allocate space for struct
    *g = malloc(sizeof(struct gain));
    if(!*g)
        errori(NULL);
    
    // try to read real
    err = option_read_real(&(*g)->value, in);
    
    // check if there was a number
    if(!err)
    {
        // no file was given
        (*g)->file = NULL;
        return 0;
    }
    
    // no number was given
    (*g)->value = 0;
    
    // read path
    return option_read_path(&(*g)->file, in);
}

int option_write_gain(char* out, const void* in, size_t n)
{
    struct gain* const* g = in;
    if((*g)->file)
        return option_write_string(out, &(*g)->file, n);
    else
        return option_write_real(out, &(*g)->value, n);
}

void option_free_gain(void* p)
{
    struct gain** g = p;
    if((*g)->file)
        option_free_path(&(*g)->file);
    else
        option_free_real(&(*g)->value);
    free(*g);
}
