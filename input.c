#include <stdlib.h>
#include <math.h>

#include "lensed.h"
#include "config.h"
#include "log.h"

/* read FITS file */
extern void read_fits(const char*, int*, int*, double**);

void read_input(const struct config* config, struct data* data)
{
    /* image */
    int nx, ny;
    double* image;
    
    /* mask */
    int mx, my;
    double* mask;
    
    /* total and active number of pixels */
    int ntot, nact;
    
    /* read image */
    read_fits(config->image, &nx, &ny, &image);
    
    info("image:");
    info("  dimensions = (%d, %d)", nx, ny);
    
    /* total number of pixels */
    ntot = nx*ny;
    
    info("  total pixels = %d", ntot);
    
    /* use mask if given */
    if(config->mask)
    {
        /* read mask */
        read_fits(config->mask, &mx, &my, &mask);
        
        /* make sure mask and image dimensions match if given */
        if(mask && (mx != nx || my != ny))
            error("mask dimensions (%d, %d) should match image dimensions (%d, %d)", mx, my, nx, ny);
        
        info("  masked");
        
        /* count active pixels */
        nact = 0;
        for(int i = 0; i < ntot; ++i)
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
    
    info("  active pixels = %d", nact);
    
    /* allocate arrays for data */
    data->model = malloc(nact*sizeof(double));
    data->error = malloc(nact*sizeof(double));
    data->image = malloc(nact*sizeof(double));
    data->variance = malloc(nact*sizeof(double));
    data->xmin = malloc(2*nact*sizeof(double));
    data->xmax = malloc(2*nact*sizeof(double));
    
    /* set data pixels */
    data->size = 0;
    for(int i = 0; i < ntot; ++i)
    {
        /* skip masked */
        if(mask && !mask[i])
            continue;
        
        /* pixel value */
        data->image[data->size] = image[i];
        data->variance[data->size] = (image[i] + config->offset)/config->gain;
        
        /* pixel boundaries */
        data->xmin[2*data->size+0] = (i%nx + 1) - 0.5;
        data->xmin[2*data->size+1] = (i/nx + 1) - 0.5;
        data->xmax[2*data->size+0] = (i%nx + 1) + 0.5;
        data->xmax[2*data->size+1] = (i/nx + 1) + 0.5;
        
        /* next pixel */
        data->size += 1;
    }
    
    /* set the normalisation */
    data->norm = -0.5*data->size*(LOG_2PI + 2*log(config->gain));
    
    /* free image and mask arrays */
    free(image);
    free(mask);
}
