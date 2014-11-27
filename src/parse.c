#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "parse.h"

int read_string(char** out, const char* in)
{
    *out = malloc(strlen(in) + 1);
    strcpy(*out, in);
    return 0;
}

int write_string(char* out, const char** in, size_t n)
{
    snprintf(out, n, "%s", *in ? *in : "(none)");
    return 0;
}

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

int write_bool(char* out, const int* in, size_t n)
{
    snprintf(out, n, "%s", *in ? "true" : "false");
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

int write_int(char* out, const int* in, size_t n)
{
    snprintf(out, n, "%d", *in);
    return 0;
}

int read_real(double* out, const char* in)
{
    char* end;
    *out = strtod(in, &end);
    if(*end)
        return 1;
    return 0;
}

int write_real(char* out, const double* in, size_t n)
{
    snprintf(out, n, "%g", *in);
    return 0;
}
