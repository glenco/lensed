#pragma once

struct config
{
    /* input */
    char* image;
    char* mask;
    double gain;
    double offset;
    
    /* integration */
    double abstol;
    int maxevals;
    
    /* MultiNest */
    char* root;
    int nlive;
    int ins;
    int mmodal;
    int ceff;
};
