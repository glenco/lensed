#pragma once

struct data
{
    size_t width;
    size_t height;
    cl_uint2* indices;
    size_t size;
    double* data;
};

// read image from file, with optional mask
void read_data(const char* imagefile, const char* maskfile, struct data*);

// write data to FITS file
void write_data(const char* filename, size_t width, size_t height,
                cl_uint2 indices[], size_t num, size_t size, cl_float* data[]);
