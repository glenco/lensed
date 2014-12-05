// compute image
kernel void render(constant char* data,
                   constant float2* qq, constant float2* ww,
                   global float* value, global float* error)
{
    // get pixel indices
    int i = get_global_id(0);
    int j = get_global_id(1);
    int k = mad24(j, IMAGE_WIDTH, i);
    
    // compute pixel flux if pixel is in image
    if(i < IMAGE_WIDTH && j < IMAGE_HEIGHT)
    {
        // pixel position
        float2 x = (float2)(1 + i, 1 + j);
        
        // value and error of quadrature
        float2 f = 0;
        
        // apply quadrature rule to computed surface brightness
        for(size_t n = 0; n < QUAD_POINTS; ++n)
            f += ww[n]*compute(data, x + qq[n]);
        
        // done
        value[k] = f.s0;
        error[k] = f.s1;
    }
}

// calculate log-likelihood of computed model
kernel void loglike(global const float* image, global const float* weight,
                    global const float* model, global float* loglike)
{
    // get pixel index
    int i = get_global_id(0);
    int j = get_global_id(1);
    int k = mad24(j, IMAGE_WIDTH, i);
    
    // compute chi^2 value if pixel is in image
    if(i < IMAGE_WIDTH && j < IMAGE_HEIGHT)
    {
        float d = model[k] - image[k];
        loglike[k] = weight[k]*d*d;
    }
}
