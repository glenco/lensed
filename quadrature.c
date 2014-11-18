#include <OpenCL/opencl.h>

#include "quadrature.h"

/*

The rule below is currently a practical Clenshaw-Curtis rule. In contrast to
the classical Clenshaw-Curtis quadrature rules, a practical rule uses extrema
of the Chebyshev polynomials as abscissae, so that a rule of order 2n - 1 will
contain the points of order n as a subset. This is used to estimate the error
of the integration.

References:
-   P. J. Davis, P. Rabinowitz, Methods of Numerical Integration, 2nd ed.
    Academic Press, 1984
-   A. R. Krommer, C. W. Ueberhuber, Computational Integration. SIAM
    Publications, 1998

The Wolfram Mathematica documentation provides a good overview of rules for
numerical integration and their relative strengths and weaknesses:
http://reference.wolfram.com/language/tutorial/NIntegrateIntegrationRules.html

*/

/* number of points in quadrature rule */
#define QUAD_N 7

/* abscissae of rule for [-0.5, 0.5] integration domain */
static const double QUAD_ABSC[QUAD_N] = {
    -0.50000000000000000000,
    -0.43301270189221932338,
    -0.25000000000000000000,
     0.00000000000000000000,
     0.25000000000000000000,
     0.43301270189221932338,
     0.50000000000000000000
};

/* weights of rule */
static const double QUAD_WEIG[QUAD_N] = {
    0.01428571428571428571, 0.12698412698412698413, \
    0.22857142857142857143, 0.26031746031746031746, \
    0.22857142857142857143, 0.12698412698412698413, 0.01428571428571428571
};

/* error estimation weights of rule */
static const double QUAD_ERRW[QUAD_N] = {
    -0.04126984126984126984, 0.12698412698412698413, \
    -0.2158730158730158730, 0.26031746031746031746, \
    -0.2158730158730158730, 0.12698412698412698413, \
    -0.04126984126984126984
};

int quad_points()
{
    return QUAD_N*QUAD_N;
}

void quad_rule(size_t n, const cl_uint2 indices[],
               cl_float2 xx[], cl_float ww[], cl_float ee[])
{
    size_t m = 0;
    for(size_t i = 0; i < n; ++i)
    {
        for(size_t j = 0; j < QUAD_N; ++j)
        {
            for(size_t k = 0; k < QUAD_N; ++k)
            {
                xx[m].s[0] = indices[i].s[0] + QUAD_ABSC[k];
                xx[m].s[1] = indices[i].s[1] + QUAD_ABSC[j];
                ww[m] = QUAD_WEIG[j]*QUAD_WEIG[k];
                ee[m] = QUAD_ERRW[j]*QUAD_ERRW[k];
                m += 1;
            }
        }
    }
}
