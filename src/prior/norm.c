#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "norm.h"
#include "../parse.h"
#include "../log.h"

double gauss(double u)
{
    // transform u using CDF, cf. Abramowitz and Stegun
    double t = (u < 0.5) ? sqrt(-2.0*log(u)) : sqrt(-2.0*log(1-u));
    t = t - ((0.010328*t + 0.802853)*t + 2.515517)/(((0.001308*t + 0.189269)*t + 1.432788)*t + 1.);
    return (u < 0.5) ? -t : t;
}

struct norm
{
    double m;
    double s;
};

void* prior_read_norm(size_t nargs, const char* argv[])
{
    struct norm* n;
    
    int err;
    double m, s;
    
    if(nargs != 3)
        return NULL;
    
    err = 0;
    err |= read_real(&m, argv[1]);
    err |= read_real(&s, argv[2]);
    
    if(err)
        return NULL;
    
    n = malloc(sizeof(struct norm));
    if(!n)
        errori(NULL);
    
    n->m = m;
    n->s = s;
    
    return n;
}

void prior_free_norm(void* data)
{
    free(data);
}

void prior_print_norm(const void* data, char* buf, size_t n)
{
    const struct norm* norm = data;
    
    size_t len = 0;
    if(len + 2 > n)
        return;
    strcpy(buf, "N(");
    len += 2;
    write_real(buf + len, norm->m, n - len);
    len = strlen(buf);
    if(len + 2 > n)
        return;
    strcat(buf, ", ");
    len += 2;
    write_real(buf + len, norm->s, n - len);
    len = strlen(buf);
    if(len + 2 > n)
        return;
    strcat(buf, ")");
}

double prior_apply_norm(const void* data, double u)
{
    const struct norm* norm = data;
    
    return norm->m + norm->s*gauss(u);
}

double prior_lower_norm(const void* data)
{
    return -HUGE_VAL;
}

double prior_upper_norm(const void* data)
{
    return +HUGE_VAL;
}
