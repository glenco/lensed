// integrate flux in pixel and return value and error estimate
static float2 flux(constant char* data, float2 x,
    constant float2* qq, constant float2* ww)
{
    // flux value and error of pixel
    float2 flux = 0;
    
    // apply quadrature rule to computed surface brightness
    for(size_t j = 0; j < QUAD_POINTS; ++j)
        flux += ww[j]*compute(data, x + qq[j]);
    
    // done
    return flux;
}

// compute image and calculate log-likelihood
kernel void loglike(constant char* data, constant float2* qq, constant float2* ww,
    global const float* mean, global const float* variance, global float* loglike)
{
    // get pixel index
    size_t i = get_global_id(0);
    
    // make sure pixel is in image
    if(i < IMAGE_SIZE)
    {
        // pixel position
        float2 x = (float2)(1 + i % IMAGE_WIDTH, 1 + i / IMAGE_WIDTH);
        
        // integrate flux in pixel
        float2 f = flux(data, x, qq, ww);
        
        // statistics
        float d = f.s0 - mean[i];
        float v = variance[i] + f.s1*f.s1;
        
        // log-likelihood
        loglike[i] = -0.5f*(d*d/v + LOG_2PI + log(v));
    }
}

// generate image and errors for dumper output
kernel void dumper(constant char* data, constant float2* qq, constant float2* ww,
    global const float* mean, global const float* variance, global float4* output)
{
    // get pixel index
    size_t i = get_global_id(0);
    
    // make sure pixel is in image
    if(i < IMAGE_SIZE)
    {
        // pixel position
        float2 x = (float2)(1 + i % IMAGE_WIDTH, 1 + i / IMAGE_WIDTH);
        
        // integrate flux in pixel
        float2 f = flux(data, x, qq, ww);
        
        // statistics
        float d = f.s0 - mean[i];
        float v = variance[i] + f.s1*f.s1;
        
        // return flux, error, residual, chi^2
        output[i] = (float4)(f, d, d*d/v);
    }
}
