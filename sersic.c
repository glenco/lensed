#include <stdlib.h>
#include <math.h>

#include "lensed.h"

enum params { X, Y, K, MAG, N, F, PA };

struct sersic
{
    double cxx;
    double cyy;
    double cxy;
    double cx;
    double cy;
    double c;
    
    double log0;
    double log1;
    double m;
};

void sersic_func(const source_ptr src_, int n, const double x[], double v[])
{
    const struct sersic* src = src_;
    
    for(int i = 0; i < n; ++i)
        v[i] += exp(src->log0 - exp(src->log1 + src->m*log(src->cxx*x[2*i+0]*x[2*i+0] + src->cyy*x[2*i+1]*x[2*i+1] + src->cx*x[2*i+0] + src->cy*x[2*i+1] + src->cxy*x[2*i+0]*x[2*i+1] + src->c)));
}

void sersic_set(source_ptr src_, const double P[])
{
    struct sersic* src = src_;
    
    double xx = P[X]*P[X];
    double yy = P[Y]*P[Y];
    
    double c = cos(P[PA]);
    double s = sin(P[PA]);
    double cc = c*c;
    double ss = s*s;
    double s2 = sin(2*P[PA]);
    
    double ff = P[F]*P[F];
    double ffi = 1.0/ff;
    double g = 1.0 - ff;
    
    src->cxx = cc + ffi*ss;
    src->cyy = ffi*cc + ss;
    src->cxy = g*ffi*s2;
    src->cx = (-2*P[X]*(ff*cc + ss) - g*P[Y]*s2)*ffi;
    src->cy = (-2*P[Y]*(cc + ff*ss) - g*P[X]*s2)*ffi;
    src->c = ((ff*xx + yy)*cc + (xx + ff*yy)*ss + g*P[X]*P[Y]*s2)*ffi;
    
    src->log0 = 0.4*P[MAG]*LOG_10 + 2*P[N]*log(P[K]) - log(P[F]) - LOG_PI - lgamma(2*P[N] + 1);
    src->log1 = log(P[K]);
    src->m = 0.5/P[N];
}

void sersic_wrap(int wrap[])
{
    wrap[X] = 0;
    wrap[Y] = 0;
    wrap[K] = 0;
    wrap[MAG] = 0;
    wrap[N] = 0;
    wrap[F] = 0;
    wrap[PA] = 1;
}

void make_sersic(struct source* s)
{
    s->ptr = malloc(sizeof(struct sersic));
    s->func = sersic_func;
    s->ndim = 7;
    s->set = sersic_set;
    s->wrap = sersic_wrap;
}
