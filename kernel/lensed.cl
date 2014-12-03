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
    global const float* image, global const float* weight, global float* output)
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
        
        // chi^2 value for pixel
        float d = f.s0 - image[i];
        output[i] = weight[i]*d*d;
    }
}

// generate image and errors for dumper output
kernel void dumper(constant char* data, constant float2* qq, constant float2* ww,
    global const float* image, global const float* weight, global float4* output)
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
        
        // return flux, error, residual, chi^2
        float d = f.s0 - image[i];
        output[i] = (float4)(f, d, weight[i]*d*d);
    }
}
