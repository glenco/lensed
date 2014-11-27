// integrate flux in pixel and return value and error estimate
static float2 flux(constant char* data, int2 index,
    ulong nq, constant float2* aa, constant float* ww, constant float* ee)
{
    // pixel position
    float2 x = convert_float2(index);
    
    // surface brightness
    float sb;
    
    // flux value and error of pixel
    float2 flux = 0;
    
    // go through quadrature points
    for(size_t j = 0; j < nq; ++j)
    {
        // compute surface brightness at point
        sb = compute(data, x + aa[j]);
        
        // add to value and error
        flux.s0 += ww[j]*sb;
        flux.s1 += ee[j]*sb;
    }
    
    // done
    return flux;
}

// compute image and calculate log-likelihood
kernel void loglike(constant char* data, global const int2* indices,
    ulong nq, constant float2* aa, constant float* ww, constant float* ee,
    global const float* mean, global const float* variance,
    global float* loglike)
{
    // get pixel index
    size_t i = get_global_id(0);
    
    // integrate flux in pixel
    float2 f = flux(data, indices[i], nq, aa, ww, ee);
    
    // statistics
    float d = f.s0 - mean[i];
    float v = variance[i] + f.s1*f.s1;
    
    // log-likelihood
    loglike[i] = -0.5f*(d*d/v + LOG_2PI + log(v));
}

// generate image and errors for dumper output
kernel void dumper(constant char* data, global const int2* indices,
    ulong nq, constant float2* aa, constant float* ww, constant float* ee,
    global const float* mean, global const float* variance,
    global float4* output)
{
    // get pixel index
    size_t i = get_global_id(0);
    
    // integrate flux in pixel
    float2 f = flux(data, indices[i], nq, aa, ww, ee);
    
    // statistics
    float d = f.s0 - mean[i];
    float v = variance[i] + f.s1*f.s1;
    
    // return flux, error, residual, chi^2
    output[i] = (float4)(f, d, d*d/v);
}
