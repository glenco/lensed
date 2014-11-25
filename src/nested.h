#pragma once

void loglike(double cube[], int* ndim, int* npar, double* lnew, void* lensed);
void dumper(int* nsamples, int* nlive, int* npar, double** physlive,
            double** posterior, double** constraints, double* maxloglike,
            double* logz, double* inslogz, double* logzerr, void* lensed);
