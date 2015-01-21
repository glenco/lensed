#include <stdlib.h>
#include <string.h>

#include "delta.h"
#include "../parse.h"
#include "../log.h"

void* read_prior_delta(size_t nargs, const char* argv[])
{
    double* x;
    
    if(nargs != 1)
        return NULL;
    
    x = malloc(sizeof(double));
    if(!x)
        errori(NULL);
    
    if(read_real(x, argv[0]))
        return NULL;
    
    return x;
}

void free_prior_delta(void* data)
{
    free(data);
}

void print_prior_delta(const void* data, char* buf, size_t n)
{
    const double* x = data;
    
    write_real(buf, *x, n);
}

double prior_delta(const void* data, double u)
{
    return *(double*)data;
}


double prior_lower_delta(const void* data)
{
    return *(double*)data;
}


double prior_upper_delta(const void* data)
{
    return *(double*)data;
}
