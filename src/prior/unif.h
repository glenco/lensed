#pragma once

void*   prior_read_unif(size_t nargs, const char* args[]);
void    prior_free_unif(void* data);
void    prior_print_unif(const void* data, char* buf, size_t n);
double  prior_apply_unif(const void* data, double u);
double  prior_lower_unif(const void* data);
double  prior_upper_unif(const void* data);
