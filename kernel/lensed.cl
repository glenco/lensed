kernel void lensed(constant object* objects, constant int2* indices,
    ulong nq, constant float2* aa, constant float* ww, constant float* ee,
    constant float* mean, constant float* variance, global float* loglike)
{
    // pixel index
    size_t i = get_global_id(0);
    
    // pixel position
    float2 x = convert_float2(indices[i]);
    
    // surface brightness
    float f;
    
    // value and error of pixel
    float val = 0;
    float err = 0;
    
    // go through quadrature points
    for(size_t j = 0; j < nq; ++j)
    {
        // compute surface brightness at point
        f = compute(objects, x + aa[j]);
        
        // add to value and error
        val += ww[j]*f;
        err += ee[j]*f;
    }
    
    // statistics
    float d = val - mean[i];
    float v = variance[i] + err*err;
    
    // log-likelihood
    loglike[i] = -0.5f*(d*d/v + LOG_2PI + log(v));
}
