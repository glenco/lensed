#pragma once

/* number of points for a single quadrature rule */
int quad_points();

/* generate n quadrature rules */
void quad_rule(size_t n, const cl_uint2 indices[],
               cl_float2 xx[], cl_float ww[], cl_float ee[]);
