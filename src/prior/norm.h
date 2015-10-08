#pragma once

void*   prior_read_norm(size_t nargs, const char* args[]);
void    prior_free_norm(void* data);
void    prior_print_norm(const void* data, char* buf, size_t n);
double  prior_apply_norm(const void* data, double u);
double  prior_lower_norm(const void* data);
double  prior_upper_norm(const void* data);
