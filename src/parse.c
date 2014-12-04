#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include "parse.h"
#include "constants.h"

// list of constants for reals
static const struct { const char* name; double value; } REAL_CONSTS[] = {
    { "pi", PI }
};
static const size_t NREAL_CONSTS = sizeof(REAL_CONSTS)/sizeof(REAL_CONSTS[0]);

int read_bool(int* out, const char* in)
{
    if(strcmp(in, "true") == 0 || strcmp(in, "TRUE") == 0 || strcmp(in, "1") == 0)
        *out = 1;
    else if(strcmp(in, "false") == 0 || strcmp(in, "FALSE") == 0 || strcmp(in, "0") == 0)
        *out = 0;
    else
        return 1;
    return 0;
}

int write_bool(char* out, int in, size_t n)
{
    snprintf(out, n, "%s", in ? "true" : "false");
    return 0;
}

int read_int(int* out, const char* in)
{
    char* end;
    long l = strtol(in, &end, 10);
    if(*end || l < INT_MIN || l > INT_MAX)
        return 1;
    *out = l;
    return 0;
}

int write_int(char* out, int in, size_t n)
{
    snprintf(out, n, "%d", in);
    return 0;
}

int read_real(double* out, const char* in)
{
    char* end;
    
    *out = strtod(in, &end);
    if(*end)
    {
        size_t i;
        for(i = 0; i < NREAL_CONSTS; ++i)
            if(strcmp(end, REAL_CONSTS[i].name) == 0)
                break;
        if(i == NREAL_CONSTS)
            return 1;
        if(end == in)
            *out = REAL_CONSTS[i].value;
        else
            *out *= REAL_CONSTS[i].value;
    }
    return 0;
}

int write_real(char* out, double in, size_t n)
{
    size_t i;
    
    if(in == 0)
    {
        snprintf(out, n, "0");
        return 0;
    }
    
    for(i = 0; i < NREAL_CONSTS; ++i)
    {
        double f = in/REAL_CONSTS[i].value;
        
        if(f == 1.0)
        {
            snprintf(out, n, "%s", REAL_CONSTS[i].name);
            return 0;
        }
        
        if(100*f == floor(100*f))
        {
            snprintf(out, n, "%g%s", f, REAL_CONSTS[i].name);
            return 0;
        }
    }
    
    snprintf(out, n, "%g", in);
    
    return 0;
}
