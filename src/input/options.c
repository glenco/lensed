#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

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
    union {
        void* default_null;
        const char* default_string;
        int default_bool;
        int default_int;
        double default_real;
    } default_value;
    size_t offset;
    size_t size;
};

// mark option as required or optional
#define OPTION_REQUIRED(type) 1, 0, #type, (int (*)(void*, const char*))read_##type, (int (*)(char*, const void*, size_t))write_##type, { .default_null = NULL }
#define OPTION_OPTIONAL(type, value) 0, 0, #type, (int (*)(void*, const char*))read_##type, (int (*)(char*, const void*, size_t))write_##type, { .default_##type = value }
#define OPTION_REQIFSET(type, value, depend) 2, offsetof(options, depend), #type, (int (*)(void*, const char*))read_##type, (int (*)(char*, const void*, size_t))write_##type, { .default_##type = value }
#define OPTION_REQIFNOT(type, value, depend) 3, offsetof(options, depend), #type, (int (*)(void*, const char*))read_##type, (int (*)(char*, const void*, size_t))write_##type, { .default_##type = value }

// get offset of option field in input
#define OPTION_FIELD(field) offsetof(options, field), sizeof(((options*)0)->field)

// get status of dependency
#define OPTION_ISSET(options, offset) (*(int*)((char*)options + offset))
#define OPTION_ISNOT(options, offset) (!*(int*)((char*)options + offset))

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
        OPTION_REQUIRED(string),
        OPTION_FIELD(image)
    },
    {
        "gain",
        "Conversion factor to counts",
        OPTION_REQUIRED(real),
        OPTION_FIELD(gain)
    },
    {
        "offset",
        "Subtracted flat-field offset",
        OPTION_REQUIRED(real),
        OPTION_FIELD(offset)
    },
    {
        "mask",
        "Input mask, FITS file",
        OPTION_OPTIONAL(string, NULL),
        OPTION_FIELD(mask)
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
        "tol",
        "Tolerance for evidence",
        OPTION_OPTIONAL(real, 0.1),
        OPTION_FIELD(tol)
    },
    {
        "eff",
        "Sampling efficiency",
        OPTION_OPTIONAL(real, 0.05),
        OPTION_FIELD(eff)
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
        "fb",
        "Show MultiNest feedback",
        OPTION_OPTIONAL(bool, 0),
        OPTION_FIELD(fb)
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

options* create_options()
{
    options* opts = malloc(sizeof(options));
    if(!opts)
        error("%s", strerror(errno));
    memset(opts, 0, sizeof(options));
    return opts;
}

void free_options(options* opts)
{
    free(opts->image);
    free(opts->mask);
    free(opts->root);
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
