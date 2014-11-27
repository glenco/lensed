#pragma once

// read a prior from string
prior* read_prior(const char* str);

// free all memory allocated by prior
void free_prior(prior* pri);

// output prior to string
void print_prior(const prior* pri, char* buf, size_t n);

// apply prior to unit variate
void apply_prior(const prior* pri, double* u);
