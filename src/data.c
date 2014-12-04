#include <stdlib.h>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include "fitsio.h"

#include "input.h"
#include "data.h"
#include "log.h"

// number of bins for find_mode
#ifndef MODE_BINS
#define MODE_BINS 100
#endif

// return value for invalid find_mode
#ifndef MODE_NAN
#ifdef NAN
#define MODE_NAN NAN
#else
#define MODE_NAN 0
#endif
#endif

void fits_error(const char* filename, int status)
{
    char err_text[32];
    
    fits_get_errstatus(status, err_text);
    
    errorf(filename, 0, "%s", err_text);
}

void read_fits(const char* filename, int datatype, size_t* width, size_t* height, void** image)
{
    int status = 0;
    
    // the FITS file
    fitsfile* fptr;
    
    // metadata
    int bitpix;
    int naxis;
    long naxes[2];
    
    // total number of pixels
    long npix;
    
    // reading offset
    long fpixel[2] = { 1, 1 };
    
    // open FITS file
    fits_open_image(&fptr, filename, READONLY, &status);
    if(status)
        fits_error(filename, status);
    
    // get metadata
    fits_get_img_param(fptr, 2, &bitpix, &naxis, naxes, &status);
    if(status)
        fits_error(filename, status);
    
    // check dimension of image
    if(naxis != 2)
        errorf(filename, 0, "file has %d axes (should be 2)", naxis);
    
    // set dimensions
    *width = naxes[0];
    *height = naxes[1];
    
    // create array for pixels
    npix = naxes[0]*naxes[1];
    *image = malloc(npix*sizeof(double));
    if(!*image)
        errori(NULL);
    
    // read pixels into array
    fits_read_pix(fptr, datatype, fpixel, npix, NULL, *image, NULL, &status);
    if(status)
        fits_error(filename, status);
    
    // close file again
    fits_close_file(fptr, &status);
    if(status)
        fits_error(filename, status);
}

void write_fits(const char* filename, size_t width, size_t height,
                size_t num, double** images)
{
    int status = 0;
    
    // the FITS file
    fitsfile* fptr;
    
    // total number of pixels
    long npix = width*height;
    
    // dimensions
    int naxis = 2;
    long naxes[2] = { width, height };
    
    // writing offset
    long fpixel[2] = { 1, 1 };
    
    // create FITS file
    fits_create_file(&fptr, filename, &status);
    if(status)
        fits_error(filename, status);
    
    for(size_t n = 0; n < num; ++n)
    {
        // create image extension
        fits_create_img(fptr, DOUBLE_IMG, naxis, naxes, &status);
        if(status)
            fits_error(filename, status);
        
        // write pixels
        fits_write_pix(fptr, TDOUBLE, fpixel, npix, images[n], &status);
        if(status)
            fits_error(filename, status);
    }
    
    // close file
    fits_close_file(fptr, &status);
    if(status)
        fits_error(filename, status);
}

void read_image(const char* filename, size_t* width, size_t* height, cl_float** image)
{
    // read FITS file
    read_fits(filename, TFLOAT, width, height, (void**)image);
    
    // TODO process depending on format
}

void read_weight(const char* filename, size_t width, size_t height, cl_float** weight)
{
    // weight width and height
    size_t wht_w, wht_h;
    
    // read weights from FITS file
    read_fits(filename, TFLOAT, &wht_w, &wht_h, (void**)weight);
    
    // make sure dimensions agree
    if(wht_w != width || wht_h != height)
        errorf(filename, 0, "wrong dimensions %zu x %zu for weights (should be %zu x %zu)", wht_w, wht_h, width, height);
    
    // TODO process depending on format
}

void make_weight(const cl_float* image, size_t width, size_t height, double gain, double offset, cl_float** weight)
{
    // total size of weights
    size_t size = width*height;
    
    // make array for weights
    cl_float* w = malloc(size*sizeof(cl_float));
    if(!w)
        errori(NULL);
    
    // calculate inverse variance for each pixel
    for(size_t i = 0; i < size; ++i)
        w[i] = gain/(image[i] + offset);
    
    // output weights
    *weight = w;
}

void read_mask(const char* filename, size_t width, size_t height, int** mask)
{
    // mask width and height
    size_t msk_w, msk_h;
    
    // read mask from FITS file
    read_fits(filename, TINT, &msk_w, &msk_h, (void**)mask);
    
    // make sure dimensions agree
    if(msk_w != width || msk_h != height)
        errorf(filename, 0, "wrong dimensions %zu x %zu for mask (should be %zu x %zu)", msk_w, msk_h, width, height);
}

void write_output(const char* filename, size_t width, size_t height, size_t num, cl_float4* output)
{
    size_t size = width*height;
    
    double** images = malloc(num*sizeof(double*));
    
    for(size_t n = 0; n < num; ++n)
    {
        // allocate image filled with zeros
        images[n] = calloc(size, sizeof(double));
        
        // set pixels
        for(size_t i = 0; i < size; ++i)
            images[n][i] = output[i].s[n];
    }
    
    // write FITS file
    write_fits(filename, width, height, num, images);
    
    for(size_t n = 0; n < num; ++n)
        free(images[n]);
    free(images);
}

cl_float find_mode(size_t nvalues, const cl_float values[], const cl_float mask[])
{
    double min, max, dx;
    size_t* counts;
    size_t i, j;
    
    // make sure there are values
    if(nvalues == 0)
        return MODE_NAN;
    
    // find minimum and maximum of values
    min = max = values[0];
    for(i = 1; i < nvalues; ++i)
    {
        if(mask && !mask[i])
            continue;
        if(values[i] < min)
            min = values[i];
        else if(values[i] > max)
            max = values[i];
    }
    
    // if the values are amodal, return
    if(min == max)
        return min;
    
    // bin width
    dx = (max - min)/MODE_BINS;
    
    // create array for bin counts
    counts = calloc(MODE_BINS, sizeof(size_t));
    if(!counts)
        errori(NULL);
    
    // count frequency in each bin
    for(i = 0; i < nvalues; ++i)
    {
        if(mask && !mask[i])
            continue;
        j = (values[i] - min)/dx;
        if(j == MODE_BINS)
            j -= 1;
        counts[j] += 1;
    }
    
    // find bin with most counts
    j = 0;
    for(i = 1; i < MODE_BINS; ++i)
        if(counts[i] > counts[j])
            j = i;
    
    // done with counts
    free(counts);
    
    // return counts at middle of mode bin
    return min + (j + 0.5)*dx;
}
