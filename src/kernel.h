#pragma once

// kernel for object data
void object_kernel(const char* name, size_t* nkernels, const char*** kernels);

// kernels for all objects
void load_kernels(size_t nobjects, const char* objects[], size_t* nkernels, const char*** kernels);

// combine prefix and name into kernel name
char* kernel_name(const char* prefix, const char* name);
