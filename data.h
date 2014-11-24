#pragma once

struct data
{
    size_t width;
    size_t height;
    size_t size;
    cl_uint2* indices;
    cl_float* mean;
    cl_float* variance;
    double lognorm;
};

// read image from file, with optional mask
void read_data(const struct input* input, struct data*);

// write data to FITS file
void write_data(const char* filename, size_t width, size_t height,
                size_t size, size_t num, cl_float* data[], cl_uint2 indices[]);
