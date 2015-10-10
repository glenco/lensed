#include <stdlib.h>
#include <string.h>

#include "input.h"
#include "parse.h"
#include "prior.h"
#include "log.h"

// function pointers for priors
typedef void*   (*read_func)(size_t, const char*[]);
typedef void    (*free_func)(void*);
typedef void    (*print_func)(const void*, char*, size_t);
typedef double  (*apply_func)(const void*, double);
typedef double  (*value_func)(const void*);

// item of prior list
typedef struct
{
    const char* name;
    read_func   read;
    free_func   free;
    print_func  print;
    apply_func  apply;
    value_func  lower;
    value_func  upper;
    int         pseudo;
} prior_list;

// macro to simplify definition of prior list
#define PRIOR(x, d) {   \
    #x,                 \
    prior_read_##x,     \
    prior_free_##x,     \
    prior_print_##x,    \
    prior_apply_##x,    \
    prior_lower_##x,    \
    prior_upper_##x,    \
    d                   \
}

// include all prior headers here
#include "prior/delta.h"
#include "prior/unif.h"
#include "prior/norm.h"

// define known priors here
static const prior_list PRIORS[] = {
    PRIOR(delta, 1),
    PRIOR(unif, 0),
    PRIOR(norm, 0)
};

// number of priors
static const size_t NPRIORS = sizeof(PRIORS)/sizeof(PRIORS[0]);

// this is the structure behind the opaque prior type
struct prior
{
    free_func   free;
    print_func  print;
    apply_func  apply;
    value_func  lower;
    value_func  upper;
    int         pseudo;
    void*       data;
};

// whitespace characters
static const char* WS = " \t\n\v\f\r";

prior* prior_read(const char* str)
{
    prior* pri;
    
    size_t nargs;
    const char** args;
    char* arg;
    
    size_t pos, spn, len;
    
    double dummy;
    
    // get length of string
    len = strlen(str);
    
    // count number of arguments in string
    nargs = 0;
    pos = 0;
    while(pos < len)
    {
        pos += strcspn(str + pos, WS);
        nargs += 1;
        pos += strspn(str + pos, WS);
    }
    
    // allocate array for arguments
    args = malloc(nargs*sizeof(const char*));
    if(!args)
        errori(NULL);
    
    // tokenize string into arguments
    nargs = 0;
    pos = 0;
    while(pos < len)
    {
        spn = strcspn(str + pos, WS);
        arg = malloc(spn + 1);
        if(!arg)
            errori(NULL);
        strncpy(arg, str + pos, spn);
        arg[spn] = '\0';
        
        args[nargs++] = arg;
        
        pos += spn;
        pos += strspn(str + pos, WS);
    }
    
    // single-argument prior with real is delta
    if(nargs == 1 && read_real(&dummy, args[0]) == 0)
    {
        // delta function prior
        pos = 0;
    }
    else
    {
        // find prior in list
        for(pos = 1; pos < NPRIORS; ++pos)
            if(strcmp(args[0], PRIORS[pos].name) == 0)
                break;
    }
    
    // make sure prior was found
    if(pos == NPRIORS)
        error("unknown prior: %s", args[0]);
    
    // create prior
    pri = malloc(sizeof(prior));
    if(!pri)
        errori(NULL);
    
    // set up prior functions
    pri->free       = PRIORS[pos].free;
    pri->print      = PRIORS[pos].print;
    pri->apply      = PRIORS[pos].apply;
    pri->lower      = PRIORS[pos].lower;
    pri->upper      = PRIORS[pos].upper;
    pri->pseudo     = PRIORS[pos].pseudo;
    
    // try to parse prior data
    pri->data = PRIORS[pos].read(nargs, args);
    if(!pri->data)
        error("invalid prior definition: %s", str);
    
    // clean up
    for(size_t i = 0; i < nargs; ++i)
        free((char*)args[i]);
    free(args);
    
    // prior is ready
    return pri;
}

prior* prior_default(double value)
{
    // create prior
    prior* pri = malloc(sizeof(prior));
    if(!pri)
        errori(NULL);
    
    // set up prior functions
    pri->free       = prior_free_delta;
    pri->print      = prior_print_delta;
    pri->apply      = prior_apply_delta;
    pri->lower      = prior_lower_delta;
    pri->upper      = prior_upper_delta;
    pri->pseudo     = 1;
    
    // make prior data
    pri->data       = prior_make_delta(value);
    
    // prior is ready
    return pri;
}

void prior_free(prior* pri)
{
    if(pri)
    {
        pri->free(pri->data);
        free(pri);
    }
}

void prior_print(const prior* pri, char* buf, size_t n)
{
    pri->print(pri->data, buf, n);
}

double prior_apply(const prior* pri, double u)
{
    return pri->apply(pri->data, u);
}

double prior_lower(const prior* pri)
{
    return pri->lower(pri->data);
}

double prior_upper(const prior* pri)
{
    return pri->upper(pri->data);
}

int prior_pseudo(const prior* pri)
{
    return pri->pseudo;
}
