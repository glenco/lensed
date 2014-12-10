#pragma once

// program for getting object information
void object_program(const char* name, size_t* nkernels, const char*** kernels);

// main program to compute images
void main_program(size_t nobjs, object objs[], size_t* nkernels, const char*** kernels);

// get options for building kernels
const char* kernel_options(size_t width, size_t height,
                           int psf, size_t psfw, size_t psfh,
                           size_t nq,
                           const char* flags[]);

// combine prefix and name into kernel name
char* kernel_name(const char* prefix, const char* name);
