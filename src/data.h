#pragma once

typedef struct
{
    size_t width;
    size_t height;
    size_t size;
    cl_uint2* indices;
    cl_float* mean;
    cl_float* variance;
    double lognorm;
} data;

// read image from file, with optional mask
data* read_data(const input* inp);

// free all memory allocated by data
void free_data(data* dat);

// write data to FITS file
void write_data(const char* filename, size_t width, size_t height,
                size_t size, size_t num, cl_float* data[], cl_uint2 indices[]);
