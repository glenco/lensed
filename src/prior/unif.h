#pragma once

void* read_prior_unif(size_t nargs, const char* args[]);
void free_prior_unif(void* data);
void print_prior_unif(const void* data, char* buf, size_t n);
double prior_unif(const void* data, double u);
double prior_lower_unif(const void* data);
double prior_upper_unif(const void* data);
