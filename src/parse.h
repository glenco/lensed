#pragma once

int read_string(char** out, const char* in);
int write_string(char* out, const char** in, size_t n);

int read_bool(int* out, const char* in);
int write_bool(char* out, const int* in, size_t n);

int read_int(int* out, const char* in);
int write_int(char* out, const int* in, size_t n);

int read_real(double* out, const char* in);
int write_real(char* out, const double* in, size_t n);
