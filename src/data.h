#pragma once

// read image from file
void read_image(const char* filename, size_t* width, size_t* height, cl_float** image);

// read weights from file
void read_weight(const char* filename, size_t width, size_t height, cl_float** weight);

// generate weights from image
void make_weight(const cl_float* image, size_t width, size_t height, double gain, double offset, cl_float** weight);

// read mask from file
void read_mask(const char* filename, size_t width, size_t height, int** mask);

// write output to FITS file
void write_output(const char* filename, size_t width, size_t height, size_t noutput, cl_float* output[]);

// find most common value
void find_mode(size_t nvalues, const cl_float values[], const cl_float mask[], double* mode, double* fwhm);
