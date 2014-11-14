#include <fitsio.h>

#include "lensed.h"
#include "log.h"

void fits_error(const char* filename, int status)
{
    char err_text[32];
    
    fits_get_errstatus(status, err_text);
    
    error("error reading \"%s\": %s", filename, err_text);
}

void read_fits(const char* filename, int* nx, int* ny, double** image)
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
    *nx = naxes[0];
    *ny = naxes[1];
    
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
