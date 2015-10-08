#pragma once

void*   prior_make_delta(double value);
void*   prior_read_delta(size_t nargs, const char* args[]);
void    prior_free_delta(void* data);
void    prior_print_delta(const void* data, char* buf, size_t n);
double  prior_apply_delta(const void* data, double u);
double  prior_lower_delta(const void* data);
double  prior_upper_delta(const void* data);
