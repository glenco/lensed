#pragma once

void* read_prior_norm(size_t nargs, const char* args[]);
void free_prior_norm(void* data);
void print_prior_norm(const void* data, char* buf, size_t n);
double prior_norm(const void* data, double u);
double prior_lower_norm(const void* data);
double prior_upper_norm(const void* data);
