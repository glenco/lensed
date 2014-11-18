#include <stdlib.h>
#include <string.h>
#include <OpenCL/opencl.h>

#include "fitsio.h"

#include "config.h"
#include "data.h"
#include "constants.h"
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

void read_data(const char* imagefile, const char* maskfile, struct data* data)
{
    // image and mask arrays
    double* image;
    size_t mx, my;
    double* mask;
    
    /* total and active number of pixels */
    size_t ntot, nact;
    
    /* read image */
    read_fits(imagefile, &data->width, &data->height, &image);
    
    info("image");
    info("  dimensions = (%zu, %zu)", data->width, data->height);
    
    /* total number of pixels */
    ntot = data->width*data->height;
    
    info("  total pixels = %zu", ntot);
    
    /* use mask if given */
    if(maskfile)
    {
        /* read mask */
        read_fits(maskfile, &mx, &my, &mask);
        
        /* make sure mask and image dimensions match if given */
        if(mask && (mx != data->width || my != data->height))
            error("mask dimensions (%zu, %zu) should match image dimensions (%zu, %zu)", mx, my, data->width, data->height);
        
        info("  masked");
        
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
    
    info("  active pixels = %zu", nact);
    
    /* allocate arrays for data */
    data->data = malloc(nact*sizeof(double));
    data->indices = malloc(nact*sizeof(cl_int2));
    
    /* set data pixels */
    data->size = 0;
    for(size_t i = 0; i < ntot; ++i)
    {
        /* skip masked */
        if(mask && !mask[i])
            continue;
        
        /* pixel value */
        data->data[data->size] = image[i];
        
        /* pixel indices */
        data->indices[data->size].s[0] = 1 + i%data->width;
        data->indices[data->size].s[1] = 1 + i/data->width;
        
        /* next pixel */
        data->size += 1;
    }
    
    /* free image and mask arrays */
    free(image);
    free(mask);
}

void write_data(const char* filename, size_t width, size_t height,
                cl_uint2 indices[], size_t num, size_t size, cl_float* data[])
{
    double** images = malloc(num*sizeof(double*));
    
    for(size_t n = 0; n < num; ++n)
    {
        // allocate image filled with zeros
        images[n] = calloc(width*height, sizeof(double));
        
        // set pixels
        for(size_t i = 0; i < size; ++i)
            images[n][(indices[i].s[1]-1)*width+(indices[i].s[0]-1)] = data[n][i];
    }
    
    // write FITS file
    write_fits(filename, width, height, num, images);
    
    for(size_t n = 0; n < num; ++n)
        free(images[n]);
    free(images);
}
