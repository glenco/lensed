#include <stdlib.h>
#include <math.h>

#include "lensed.h"

enum params { X, Y, R_E, F, PA };

struct sie
{
    double x;
    double y;
    double c;
    double s;
    
    double f2;
    double e;
    double d;
};

void sie_func(lens_ptr sie_, int n, const double x[], double y[])
{
    struct sie* sie = sie_;
    double y1, y2, y3;
    
    for(int i = 0; i < n; ++i)
    {
        y1 = (x[2*i+0] - sie->x)*sie->c - (x[2*i+1] - sie->y)*sie->s;
        y2 = (x[2*i+1] - sie->y)*sie->c + (x[2*i+0] - sie->x)*sie->s;
        y3 = sie->e/sqrt(sie->f2*y1*y1 + y2*y2);
        y1 = sie->d*atan(y1*y3);
        y2 = sie->d*atanh(y2*y3);
        y[2*i+0] = x[2*i+0] - (y1*sie->c + y2*sie->s);
        y[2*i+1] = x[2*i+1] - (y2*sie->c - y1*sie->s);
    }
}

void sie_set(lens_ptr sie_, const double P[])
{
    struct sie* sie = sie_;
    
    sie->x = P[X];
    sie->y = P[Y];
    sie->c = cos(PI/2 - P[PA]);
    sie->s = sin(PI/2 - P[PA]);
    sie->f2 = P[F]*P[F];
    sie->e = sqrt(1.0 - sie->f2);
    sie->d = P[R_E]*sqrt(P[F])/sie->e;
}

void sie_wrap(int wrap[])
{
    wrap[X] = 0;
    wrap[Y] = 0;
    wrap[R_E] = 0;
    wrap[F] = 0;
    wrap[PA] = 1;
}

void make_sie(struct lens* lens)
{
    lens->ptr = malloc(sizeof(struct sie));
    lens->func = sie_func;
    lens->ndim = 5;
    lens->set = sie_set;
    lens->wrap = sie_wrap;
}
