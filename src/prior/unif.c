#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "unif.h"
#include "../parse.h"
#include "../log.h"

struct uniform
{
    double a;
    double b;
};

void* read_prior_unif(size_t nargs, const char* argv[])
{
    struct uniform* unif;
    
    int err;
    double a, b;
    
    if(nargs != 3)
        return NULL;
    
    err = 0;
    err |= read_real(&a, argv[1]);
    err |= read_real(&b, argv[2]);
    
    if(err)
        return NULL;
    
    unif = malloc(sizeof(struct uniform));
    if(!unif)
        error("%s", strerror(errno));
    
    unif->a = a;
    unif->b = b;
    
    return unif;
}

void free_prior_unif(void* data)
{
    free(data);
}

void print_prior_unif(const void* data, char* buf, size_t n)
{
    const struct uniform* unif = data;
    
    snprintf(buf, n, "U(%g, %g)", unif->a, unif->b);
}

double prior_unif(const void* data, double u)
{
    const struct uniform* unif = data;
    
    return unif->a + u*(unif->b - unif->a);
}
