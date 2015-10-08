#pragma once

// read a prior from string
prior* prior_read(const char* str);

// default prior with value
prior* prior_default(double value);

// free all memory allocated by prior
void prior_free(prior* pri);

// output prior to string
void prior_print(const prior* pri, char* buf, size_t n);

// apply prior to unit variate
double prior_apply(const prior* pri, double u);

// get lower bound for prior
double prior_lower(const prior* pri);

// get upper bound for prior
double prior_upper(const prior* pri);

// flag whether this is a pseudo-prior (= calculated instead of drawn)
int prior_pseudo(const prior* pri);
