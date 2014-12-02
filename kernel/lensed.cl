// integrate flux in pixel and return value and error estimate
static float2 flux(constant char* data, float2 x,
    constant float2* qq, constant float2* ww)
{
    // flux value and error of pixel
    float2 flux = 0;
    
    // apply quadrature rule to computed surface brightness
    for(size_t j = 0; j < NQ; ++j)
        flux += ww[j]*compute(data, x + qq[j]);
    
    // done
    return flux;
}

// compute image and calculate log-likelihood
kernel void loglike(constant char* data, constant float2* qq, constant float2* ww,
    global const float* mean, global const float* variance, global float* loglike)
{
    // get work-item indices
    size_t i = get_global_id(0);
    size_t j = get_global_id(1);
    size_t k = j*WIDTH + i;
    
    if(i < WIDTH && j < HEIGHT)
    {
        // pixel position
        float2 x = (float2)(1 + i, 1 + j);
        
        // integrate flux in pixel
        float2 f = flux(data, x, qq, ww);
        
        // statistics
        float d = f.s0 - mean[k];
        float v = variance[k] + f.s1*f.s1;
        
        // log-likelihood
        loglike[k] = -0.5f*(d*d/v + LOG_2PI + log(v));
    }
}

// generate image and errors for dumper output
kernel void dumper(constant char* data, constant float2* qq, constant float2* ww,
    global const float* mean, global const float* variance, global float4* output)
{
    // get work-item indices
    size_t i = get_global_id(0);
    size_t j = get_global_id(1);
    size_t k = j*WIDTH + i;
    
    if(i < WIDTH && j < HEIGHT)
    {
        // pixel position
        float2 x = (float2)(1 + i, 1 + j);
        
        // integrate flux in pixel
        float2 f = flux(data, x, qq, ww);
        
        // statistics
        float d = f.s0 - mean[k];
        float v = variance[k] + f.s1*f.s1;
        
        // return flux, error, residual, chi^2
        output[k] = (float4)(f, d, d*d/v);
    }
}
