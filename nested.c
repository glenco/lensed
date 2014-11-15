#include <string.h>
#include <float.h>
#include <math.h>

#include "lensed.h"
#include "nested.h"
#include "cubature/cubature.h"

/* TODO dynamic prior selection */
void priors(double cube[])
{
#if 1
    /* lens */
    cube[0] = 50.5; //, #45. + 10u,
    cube[1] = 50.5; //, #45. + 10u,
    cube[2] = 17.4381; //, #12. + 10u,
    cube[3] = 0.75; //, #u,
    cube[4] = 2.35619; //, #pi*u,
    /* source */
    cube[5] = 40. + 20*cube[5];
    cube[6] = 40. + 20*cube[6];
    cube[7] = 20*cube[7];
    cube[8] = 5*cube[8];
    cube[9] = 0.5 + 7.5*cube[9];
    cube[10] = cube[10];
    cube[11] = PI*cube[11];
#else
    /* lens */
    cube[0] = 50.5;
    cube[1] = 50.5;
    cube[2] = 17.4381;
    cube[3] = 0.75;
    cube[4] = 2.35619;
    /* source */
    cube[5] = 51.7637;
    cube[6] = 49.933;
    cube[7] = 12.6527;
    cube[8] = 3.074;
    cube[9] = 7.159;
    cube[10] = 0.923181;
    cube[11] = 2.43091;
#endif
}

int surface_brightness(unsigned ndim, size_t npt, const double x[],
                       void* lensed_, unsigned fdim, double fval[])
{
    const struct lensed* lensed = lensed_;
    
    /* lens points */
    for(int i = 0; i < lensed->nlenses; ++i)
        lensed->lenses[i].func(lensed->lenses[i].ptr, npt, x, lensed->z);
    
    /* calculate surface brightness */
    memset(fval, 0, fdim*npt*sizeof(double));
    for(int i = 0; i < lensed->nsources; ++i)
        lensed->sources[i].func(lensed->sources[i].ptr, npt, lensed->z, fval);
    
    /* continue integration */
    return 0;
}

void loglike(double cube[], int* ndim, int* npar, double* lnew, void* lensed_)
{
    const struct lensed* lensed = lensed_;
    const struct config* config = &lensed->config;
    const struct data* data = &lensed->data;
    
    /* transform from unit cube to physical */
    priors(cube);
    
    /* set parameters */
    for(int i = 0, p = 0; i < lensed->npspace; p += lensed->pspace[i].ndim, ++i)
        lensed->pspace[i].set(lensed->pspace[i].ptr, &cube[p]);
    
    /* pixel by pixel integration */
    for(int i = 0; i < data->size; ++i)
        hcubature_v(1, surface_brightness, lensed_, 2,
                    &data->xmin[2*i], &data->xmax[2*i],
                    config->maxevals, config->abstol, DBL_EPSILON, 0,
                    &data->model[i], &data->error[i]);
    
    /* sum likelihood */
    double sum = 0.0;
    for(int i = 0; i < data->size; ++i)
    {
        double x = data->model[i] - data->image[i];
        double s2 = data->variance[i] + data->error[i]*data->error[i];
        sum += -0.5*x*x/s2 - 0.5*log(s2);
    }
    
    /* set likelihood */
    *lnew = sum + data->norm;
}

void dumper(int* nsamples, int* nlive, int* npar, double** physlive,
            double** posterior, double** constraints, double* maxloglike,
            double* logz, /* double* inslogz, */ double* logzerr, void* lensed_)
{
}
