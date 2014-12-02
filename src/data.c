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
    
    /* read image */
    read_fits(inp->opts->image, &d->width, &d->height, &image);
    
    /* total number of pixels */
    d->size = d->width*d->height;
    
    /* use mask if given */
    if(inp->opts->mask)
    {
        /* read mask */
        read_fits(inp->opts->mask, &mx, &my, &mask);
        
        /* make sure mask and image dimensions match if given */
        if(mask && (mx != d->width || my != d->height))
            error("mask dimensions %zux%zu should match image dimensions %zux%zu", mx, my, d->width, d->height);
        
        // count masked pixels
        d->nmask = 0;
        for(size_t i = 0; i < d->size; ++i)
            if(mask[i])
                d->nmask += 1;
    }
    else
    {
        // zero mask
        mask = calloc(d->size, sizeof(cl_int));
        d->nmask = 0;
    }
    
    /* allocate arrays for data */
    d->mean = malloc(d->size*sizeof(cl_float));
    d->variance = malloc(d->size*sizeof(cl_float));
    d->mask = malloc(d->size*sizeof(cl_int));
    
    /* set data pixels */
    for(size_t i = 0; i < d->size; ++i)
    {
        /* mean value */
        d->mean[i] = image[i];
        
        /* variance */
        d->variance[i] = (image[i] + inp->opts->offset)/inp->opts->gain;
        
        /* mask */
        d->mask[i] = mask[i];
    }
    
    /* free image and mask arrays */
    free(image);
    free(mask);
    
    // done
    return d;
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
            images[n][i] = output[i].s[n];
    }
    
    // write FITS file
    write_fits(filename, dat->width, dat->height, num, images);
    
    for(size_t n = 0; n < num; ++n)
        free(images[n]);
    free(images);
}

void free_data(data* d)
{
    free(d->mean);
    free(d->variance);
    free(d->mask);
    free(d);
}
