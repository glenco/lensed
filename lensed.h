#pragma once


/*************
 * constants *
 *************/

/* we define our own pi */
#ifdef PI
#undef PI
#endif

/* common constants */
#define PI      3.1415926535897932384626433832795028841971693993751
#define LOG_10  2.3025850929940456840179914546843642076011014886288
#define LOG_PI  1.1447298858494001741434273513530587116472948129153
#define LOG_2PI 1.8378770664093454835606594728112352797227949472756


/******************
 * parametrizable *
 ******************/

typedef void (*params_set)(void*, const double[]);
typedef void (*params_wrap)(int[]);

struct parametrizable
{
    void* ptr;
    void* func;
    int ndim;
    params_set set;
    params_wrap wrap;
};


/********
 * lens *
 ********/

typedef void* lens_ptr;
typedef void (*lens_func)(lens_ptr, int, const double[], double[]);

struct lens
{
    lens_ptr ptr;
    lens_func func;
    int ndim;
    params_set set;
    params_wrap wrap;
};


/**********
 * source *
 **********/

typedef void* source_ptr;
typedef void (*source_func)(source_ptr, int, const double[], double[]);

struct source
{
    source_ptr ptr;
    source_func func;
    int ndim;
    params_set set;
    params_wrap wrap;
};


/****************
 * program data *
 ****************/

/* input data */
struct data
{
    int size;
    double norm;
    double* model;
    double* error;
    double* image;
    double* variance;
    double* xmin;
    double* xmax;
};
