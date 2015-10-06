#include <stddef.h>
#include <string.h>

#include "opencl.h"
#include "quadrature.h"

// quadrature rules
#include "quad/point.h"
#include "quad/sub2.h"
#include "quad/sub4.h"
#include "quad/gm75.h"
#include "quad/g3k7.h"
#include "quad/g5k11.h"
#include "quad/g7k15.h"

// macro to quickly add a rule
#define ADD_RULE(x, n, s) \
    { n, s, QUAD_##x##_N, QUAD_##x##_PTS, QUAD_##x##_WHT, QUAD_##x##_ERR }

// list of known rules
const quad_rule_data QUAD_RULES[] = {
    ADD_RULE(POINT, "point", "single point sampling without error estimate"),
    ADD_RULE(SUB2,  "sub2",  "2x2 subsampling without error estimate"      ),
    ADD_RULE(SUB4,  "sub4",  "4x4 subsampling without error estimate"      ),
    ADD_RULE(GM75,  "gm75",  "Genz-Malik (7, 5) fully symmetric rule"      ),
    ADD_RULE(G3K7,  "g3k7",  "Gauß-Kronrod (7, 3) Cartesian rule"          ),
    ADD_RULE(G5K11, "g5k11", "Gauß-Kronrod (11, 5) Cartesian rule"         ),
    ADD_RULE(G7K15, "g7k15", "Gauß-Kronrod (15, 7) Cartesian rule"         ),
    {0}
};

void quad_rule(int r, cl_float2 xx[], cl_float2 ww[], double sx, double sy)
{
    const quad_rule_data* rd = QUAD_RULES + r;
    
    for(size_t i = 0; i < rd->size; ++i)
    {
        xx[i].s[0] = sx*rd->absc[i][0];
        xx[i].s[1] = sy*rd->absc[i][1];
        ww[i].s[0] = rd->weig[i];
        ww[i].s[1] = rd->errw[i];
    }
}
