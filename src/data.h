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

// write output to FITS file
void write_output(const char* filename, const data* dat, size_t num, cl_float4* output);
