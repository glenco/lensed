kernel void params(
    constant float*  params,
    global   object* objects
)
{
    set_sie(&objects[0], &params[0]);
    set_sersic(&objects[1], &params[5]);
}

kernel void lensed(
    constant object* objects,
    constant int2*   indices,
    private  size_t  nq,
    constant float2* aa,
    constant float*  ww,
    constant float*  ee,
    constant float*  mean,
    constant float*  variance,
    global   float*  loglike
)
{
    // pixel index
    size_t i = get_global_id(0);
    
    // pixel position
    float2 x = convert_float2(indices[i]);
    
    // ray
    float2 y;
    
    // deflection
    float2 a;
    
    // surface brightness
    float f;
    
    // value and error of pixel
    float val = 0;
    float err = 0;
    
    // go through quadrature points
    for(size_t j = 0; j < nq; ++j)
    {
        // initial ray position
        y = x + aa[j];
        
        // initial deflection is zero
        a = 0;
        
        // sum deflection
        a += sie(&objects[0], y);
        
        // apply deflection to ray
        y -= a;
        
        // initial surface brightness is zero
        f = 0;
        
        // sum surface brightness at ray position
        f += sersic(&objects[1], y);
        
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
