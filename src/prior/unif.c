#include <stdlib.h>
#include <string.h>

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
        errori(NULL);
    
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
    
    size_t len = 0;
    if(len + 2 > n)
        return;
    strcpy(buf, "U(");
    len += 2;
    write_real(buf + len, unif->a, n - len);
    len = strlen(buf);
    if(len + 2 > n)
        return;
    strcat(buf, ", ");
    len += 2;
    write_real(buf + len, unif->b, n - len);
    len = strlen(buf);
    if(len + 2 > n)
        return;
    strcat(buf, ")");
}

double prior_unif(const void* data, double u)
{
    const struct uniform* unif = data;
    
    return unif->a + u*(unif->b - unif->a);
}

double prior_lower_unif(const void* data)
{
    const struct uniform* unif = data;
    
    return unif->a;
}

double prior_upper_unif(const void* data)
{
    const struct uniform* unif = data;
    
    return unif->b;
}
