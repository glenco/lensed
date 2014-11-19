#pragma once

/* number of points for a single quadrature rule */
size_t quad_points();

/* generate n quadrature rules */
void quad_rule(cl_float2 xx[], cl_float ww[], cl_float ee[]);
