#pragma once

void* read_prior_delta(size_t nargs, const char* args[]);
void free_prior_delta(void* data);
void print_prior_delta(const void* data, char* buf, size_t n);
double prior_delta(const void* data, double u);
double prior_lower_delta(const void* data);
double prior_upper_delta(const void* data);
