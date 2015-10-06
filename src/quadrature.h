#pragma once

// quadrature rule data
typedef struct {
    const char* name;
    const char* info;
    int size;
    const double (*absc)[2];
    const double* weig;
    const double* errw;
} quad_rule_data;

// available quadrature rules, null-terminated
extern const quad_rule_data QUAD_RULES[];

// generate n quadrature rules
void quad_rule(int rule, cl_float2 xx[], cl_float2 ww[], double sx, double sy);
