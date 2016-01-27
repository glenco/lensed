#include <stdlib.h>
#include <string.h>

#include "fitsio.h"

#include "opencl.h"
#include "input.h"
#include "data.h"
#include "log.h"
#include "version.h"

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

void write_fits(fitsfile* fptr, int datatype, size_t width, size_t height,
                size_t num, void** images, const char* names[], int* status)
{
    // total number of pixels
    long npix = width*height;
    
    // dimensions
    int naxis = 2;
    long naxes[2] = { width, height };
    
    // writing offset
    long fpixel[2] = { 1, 1 };
    
    // write primary HDU
    fits_create_img(fptr, SHORT_IMG, 0, NULL, status);
    
    // record file origin
    fits_write_key(fptr, TSTRING, "ORIGIN", "Lensed " LENSED_VERSION, "FITS file originator", status);
    fits_write_comment(fptr, "for more information, see http://glenco.github.io/lensed/", status);
    
    // record the date of FITS creation
    fits_write_date(fptr, status);
    
    // write images
    for(size_t n = 0; n < num; ++n)
    {
        // create image extension
        fits_create_img(fptr, FLOAT_IMG, naxis, naxes, status);
        
        // write pixels
        fits_write_pix(fptr, datatype, fpixel, npix, images[n], status);
        
        // give extension name
        if(names && names[n])
            fits_write_key(fptr, TSTRING, "EXTNAME", (void*)names[n], "extension name", status);
    }
}

void read_image(const char* filename, size_t* width, size_t* height, cl_float** image)
{
    // read FITS file
    read_fits(filename, TFLOAT, width, height, (void**)image);
    
    // TODO process depending on format
}

void read_pcs(const char* filename, pcsdata* pcs)
{
    int status;
    char* url;
    char rowfilter[FLEN_FILENAME];
    
    // FITSIO goes crazy if initial status is not zero
    status = 0;
    
    // copy filename so that FITSIO can do whatever it wants
    url = malloc(strlen(filename) + 1);
    strcpy(url, filename);
    
    // parse the input file specification
    ffifile(url, 0, 0, 0, 0, rowfilter, 0, 0, 0, &status);
    
    // check if a row filter was given
    if(*rowfilter)
    {
        long smin, smax, sinc;
        char* cptr = rowfilter;
        
        // parse expressions for x axis
        fits_get_section_range(&cptr, &smin, &smax, &sinc, &status);
        pcs->rx = smin;
        pcs->sx = (smax > smin ? +1 : -1)*sinc;
        
        // parse expressions for y axis
        fits_get_section_range(&cptr, &smin, &smax, &sinc, &status);
        pcs->ry = smin;
        pcs->sy = (smax > smin ? +1 : -1)*sinc;
    }
    else
    {
        // set standard pixel coordinate system
        pcs->rx = 1;
        pcs->ry = 1;
        pcs->sx = 1;
        pcs->sy = 1;
    }
    
    free(url);
}

void read_gain(const char* filename, size_t width, size_t height, double** gain)
{
    // gain width and height
    size_t gw, gh;
    
    // read gain from FITS file
    read_fits(filename, TDOUBLE, &gw, &gh, (void**)gain);
    
    // make sure dimensions agree
    if(gw != width || gh != height)
        errorf(filename, 0, "wrong dimensions %zu x %zu for gain (should be %zu x %zu)", gw, gh, width, height);
    
    // TODO process depending on format
}

void make_gain(double value, size_t width, size_t height, double** gain)
{
    // total size of gain
    size_t size = width*height;
    
    // make array for gain
    double* g = malloc(size*sizeof(double));
    if(!g)
        errori(NULL);
    
    // set uniform gain for each pixel
    for(size_t i = 0; i < size; ++i)
        g[i] = value;
    
    // output gain
    *gain = g;
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

void make_weight(const cl_float* image, const double* gain, double offset, size_t width, size_t height, cl_float** weight)
{
    // total size of weights
    size_t size = width*height;
    
    // make array for weights
    cl_float* w = malloc(size*sizeof(cl_float));
    if(!w)
        errori(NULL);
    
    // calculate inverse variance for each pixel
    for(size_t i = 0; i < size; ++i)
        w[i] = gain[i]/(image[i] + offset);
    
    // output weights
    *weight = w;
}

void read_xweight(const char* filename, size_t width, size_t height, double** xweight)
{
    // extra weight width and height
    size_t xwht_w, xwht_h;
    
    // read extra weights from FITS file
    read_fits(filename, TDOUBLE, &xwht_w, &xwht_h, (void**)xweight);
    
    // make sure dimensions agree
    if(xwht_w != width || xwht_h != height)
        errorf(filename, 0, "wrong dimensions %zu x %zu for extra weights (should be %zu x %zu)", xwht_w, xwht_h, width, height);
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

void read_psf(const char* filename, size_t* width, size_t* height, cl_float** psf)
{
    // normalisation
    double norm;
    size_t size, i;
    
    // read PSF from FITS file
    read_fits(filename, TFLOAT, width, height, (void**)psf);
    
    // make PSF has odd number of pixels
    if(!(*width % 2) || !(*height % 2))
        errorf(filename, 0, "wrong dimensions %zu x %zu for PSF (should be odd numbers)", *width, *height);
    
    // normalise PSF
    norm = 0;
    size = (*width)*(*height);
    for(i = 0; i < size; ++i)
        norm += (*psf)[i];
    for(i = 0; i < size; ++i)
        (*psf)[i] /= norm;
}

void write_output(const char* filename, size_t width, size_t height, size_t noutput, cl_float* output[], const char* names[])
{
    int status = 0;
    
    // the FITS file
    fitsfile* fptr;
    
    // create FITS file
    fits_create_file(&fptr, filename, &status);
    
    // write FITS file
    write_fits(fptr, TFLOAT, width, height, noutput, (void**)output, names, &status);
               
    // close FITS file
    fits_close_file(fptr, &status);
    
    // report FITS errors
    if(status)
        fits_error(filename, status);
}

size_t write_memory(void** mem, size_t width, size_t height, size_t noutput, cl_float* output[], const char* names[])
{
    int status = 0;
    
    // memory block
    *mem = NULL;
    size_t siz = 0;
    
    // the FITS file
    fitsfile* fptr;
    
    // create in-memory FITS
    fits_create_memfile(&fptr, mem, &siz, 0, realloc, &status);
    
    // write in-memory FITS
    write_fits(fptr, TFLOAT, width, height, noutput, (void**)output, names, &status);
    
    // close in-memory FITS
    fits_close_file(fptr, &status);
    
    // report FITS errors
    if(status)
        fits_error(NULL, status);
    
    // return the in-memory FITS
    return siz;
}

void find_mode(size_t nvalues, const cl_float values[], const cl_float mask[], double* mode, double* fwhm)
{
    double min, max, dx;
    size_t* counts;
    long i, j;
    
    // make sure there are values
    if(nvalues == 0)
    {
        *mode = MODE_NAN;
        *fwhm = 0;
        return;
    }
    
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
    {
        *mode = min;
        *fwhm = 0;
        return;
    }
    
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
    
    // mode is at middle of most common bin
    *mode = min + (j + 0.5)*dx;
    
    // find right half maximum
    for(i = j+1; i < MODE_BINS; ++i)
        if(counts[i] < 0.5*counts[j])
            break;
    
    // right half maximum
    *fwhm = i*dx;
    
    // find left half maximum
    for(i = j; i > 0; --i)
        if(counts[i-1] < 0.5*counts[j])
            break;
    
    // full width at half maximum
    *fwhm -= (i-1)*dx;
    
    // done with counts
    free(counts);
}
