#include <stdlib.h>
#include <math.h>

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
    
    error("\"%s\": %s", filename, err_text);
}

void read_fits(const char* filename, size_t* width, size_t* height, double** image)
{
    int status = 0;
    
    /* the FITS file */
    fitsfile* fptr;
    
    /* metadata */
    int bitpix;
    int naxis;
    long naxes[2];
    
    /* total number of pixels */
    long npix;
    
    /* reading offset */
    long fpixel[2] = { 1, 1 };
    
    /* open FITS file */
    fits_open_image(&fptr, filename, READONLY, &status);
    if(status)
        fits_error(filename, status);
    
    /* get metadata */
    fits_get_img_param(fptr, 2, &bitpix, &naxis, naxes, &status);
    if(status)
        fits_error(filename, status);
    
    /* check dimension of image */
    if(naxis != 2)
        error("file \"%s\" has %d instead of 2 axes", filename, naxis);
    
    /* set dimensions */
    *width = naxes[0];
    *height = naxes[1];
    
    /* create array for pixels */
    npix = naxes[0]*naxes[1];
    *image = malloc(npix*sizeof(double));
    
    /* read pixels */
    fits_read_pix(fptr, TDOUBLE, fpixel, npix, NULL, *image, NULL, &status);
    if(status)
        fits_error(filename, status);
    
    /* close file again */
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
    
    /* reading offset */
    long fpixel[2] = { 1, 1 };
    
    /* create FITS file */
    fits_create_file(&fptr, filename, &status);
    if(status)
        fits_error(filename, status);
    
    for(size_t n = 0; n < num; ++n)
    {
        /* create image extension */
        fits_create_img(fptr, DOUBLE_IMG, naxis, naxes, &status);
        if(status)
            fits_error(filename, status);
        
        /* write pixels */
        fits_write_pix(fptr, TDOUBLE, fpixel, npix, images[n], &status);
        if(status)
            fits_error(filename, status);
    }
    
    /* close file */
    fits_close_file(fptr, &status);
    if(status)
        fits_error(filename, status);
}

data* read_data(const input* inp)
{
    // try to allocate memory for data
    data* d = malloc(sizeof(data));
    if(!d)
        errori("could not create data");
    
    // image and mask arrays
    double* image;
    size_t mx, my;
    double* mask;
    
    /* total and active number of pixels */
    size_t ntot, nact;
    
    /* read image */
    read_fits(inp->opts->image, &d->width, &d->height, &image);
    
    verbose("image");
    verbose("  dimensions = %zux%zu", d->width, d->height);
    
    /* total number of pixels */
    ntot = d->width*d->height;
    
    verbose("  total pixels = %zu", ntot);
    
    /* use mask if given */
    if(inp->opts->mask)
    {
        /* read mask */
        read_fits(inp->opts->mask, &mx, &my, &mask);
        
        /* make sure mask and image dimensions match if given */
        if(mask && (mx != d->width || my != d->height))
            error("mask dimensions %zux%zu should match image dimensions %zux%zu", mx, my, d->width, d->height);
        
        verbose("  masked");
        
        /* count active pixels */
        nact = 0;
        for(size_t i = 0; i < ntot; ++i)
            if(mask[i])
                nact += 1;
    }
    else
    {
        /* no mask */
        mask = 0;
        
        /* all pixels are active */
        nact = ntot;
    }
    
    verbose("  active pixels = %zu", nact);
    
    /* allocate arrays for data */
    d->mean = malloc(nact*sizeof(cl_float));
    d->variance = malloc(nact*sizeof(cl_float));
    d->indices = malloc(nact*sizeof(cl_int2));
    
    /* set data pixels */
    d->size = 0;
    for(size_t i = 0; i < ntot; ++i)
    {
        /* skip masked */
        if(mask && !mask[i])
            continue;
        
        /* mean value */
        d->mean[d->size] = image[i];
        
        /* variance */
        d->variance[d->size] = (image[i] + inp->opts->offset)/inp->opts->gain;
        
        /* pixel indices */
        d->indices[d->size].s[0] = 1 + i%d->width;
        d->indices[d->size].s[1] = 1 + i/d->width;
        
        /* next pixel */
        d->size += 1;
    }
    
    /* free image and mask arrays */
    free(image);
    free(mask);
    
    // done
    return d;
}

void free_data(data* d)
{
    free(d->mean);
    free(d->variance);
    free(d->indices);
    free(d);
}

void write_output(const char* filename, const data* dat, size_t num, cl_float4* output)
{
    double** images = malloc(num*sizeof(double*));
    
    for(size_t n = 0; n < num; ++n)
    {
        // allocate image filled with zeros
        images[n] = calloc(dat->width*dat->height, sizeof(double));
        
        // set pixels
        for(size_t i = 0; i < dat->size; ++i)
            images[n][(dat->indices[i].s[1]-1)*dat->width+(dat->indices[i].s[0]-1)] = output[i].s[n];
    }
    
    // write FITS file
    write_fits(filename, dat->width, dat->height, num, images);
    
    for(size_t n = 0; n < num; ++n)
        free(images[n]);
    free(images);
}

double find_mode(size_t nvalues, cl_float values[])
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
        if(values[i] < min)
            min = values[i];
        else if(values[i] > max)
            max = values[i];
    }
    
    // if the values are amodal, return
    if(min == max)
        return min;
    
    // bin width, two bins on end points are added for padding
    dx = (max - min)/(MODE_BINS-2);
    
    // move minimum further out so that 0 is in the middle of a bin
    min = (floor(min/dx - 0.5)+0.5)*dx;
    
    // create array for bin counts
    counts = calloc(MODE_BINS, sizeof(size_t));
    if(!counts)
        errori(NULL);
    
    // count frequency in each bin
    for(i = 0; i < nvalues; ++i)
    {
        j = (values[i] - min)/dx;
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
