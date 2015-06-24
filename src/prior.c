#include <stdlib.h>
#include <string.h>

#include "input.h"
#include "parse.h"
#include "prior.h"
#include "log.h"

// function pointers for priors
typedef void* (*read_func)(size_t, const char*[]);
typedef void (*free_func)(void*);
typedef double (*prior_func)(const void*, double);
typedef void (*print_func)(const void*, char*, size_t);
typedef double (*lower_func)(const void*);
typedef double (*upper_func)(const void*);

// item of prior list
typedef struct
{
    const char* name;
    read_func read;
    free_func free;
    print_func print;
    prior_func prior;
    lower_func lower;
    upper_func upper;
} prior_list;

// macro to simplify definition of prior list
#define PRIOR(x) { #x, read_prior_##x, free_prior_##x, print_prior_##x, prior_##x, prior_lower_##x, prior_upper_##x }

// include all prior headers here
#include "prior/delta.h"
#include "prior/unif.h"
#include "prior/norm.h"

// define known priors here
static const prior_list PRIORS[] = {
    PRIOR(delta),
    PRIOR(unif),
    PRIOR(norm)
};

// number of priors
static const size_t NPRIORS = sizeof(PRIORS)/sizeof(PRIORS[0]);

// this is the structure behind the opaque prior type
struct prior
{
    prior_func prior;
    print_func print;
    lower_func lower;
    upper_func upper;
    free_func free;
    void* data;
};

// whitespace characters
static const char* WS = " \t\n\v\f\r";

prior* read_prior(const char* str)
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
    pri->prior = PRIORS[pos].prior;
    pri->print = PRIORS[pos].print;
    pri->lower = PRIORS[pos].lower;
    pri->upper = PRIORS[pos].upper;
    pri->free = PRIORS[pos].free;
    
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

void free_prior(prior* pri)
{
    if(pri)
    {
        pri->free(pri->data);
        free(pri);
    }
}

void print_prior(const prior* pri, char* buf, size_t n)
{
    pri->print(pri->data, buf, n);
}

double apply_prior(const prior* pri, double u)
{
    return pri->prior(pri->data, u);
}

double prior_lower(const prior* pri)
{
    return pri->lower(pri->data);
}

double prior_upper(const prior* pri)
{
    return pri->upper(pri->data);
}
