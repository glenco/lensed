#pragma once

int read_bool(int* out, const char* in);
int write_bool(char* out, int in, size_t n);

int read_int(int* out, const char* in);
int write_int(char* out, int in, size_t n);

int read_real(double* out, const char* in);
int write_real(char* out, double real, size_t n);
