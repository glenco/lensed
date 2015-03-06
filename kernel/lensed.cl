// compute image
kernel void render(constant char* data, float4 pcs,
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
        float2 x = pcs.xy + pcs.zw*(float2)(i, j);
        
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

// convolve input with PSF
kernel void convolve(global float* input, constant float* psf,
                     local float* input2, local float* psf2,
                     global float* output)
{
    int i, j;
    
    // pixel indices for output
    int gi = get_global_id(0);
    int gj = get_global_id(1);
    
    // local indices, size and origin
    int li = get_local_id(0);
    int lj = get_local_id(1);
    int lw = get_local_size(0);
    int lh = get_local_size(1);
    int ls = lw*lh;
    
    // cache size and origin
    int cw = PSF_WIDTH/2 + lw + PSF_WIDTH/2;
    int ch = PSF_HEIGHT/2 + lh + PSF_HEIGHT/2;
    int cs = cw*ch;
    int cx = mad24((int)get_group_id(0), lw, -PSF_WIDTH/2);
    int cy = mad24((int)get_group_id(1), lh, -PSF_HEIGHT/2);
    
    // fill cache
    for(i = mad24(lj, lw, li); i < cs; i += ls)
        input2[i] = input[clamp(cy + i/cw, 0, IMAGE_HEIGHT-1)*IMAGE_WIDTH + clamp(cx + i%cw, 0, IMAGE_WIDTH-1)];
    for(i = mad24(lj, lw, li); i < PSF_WIDTH*PSF_HEIGHT; i += ls)
        psf2[i] = psf[i];
    
    // wait for all items to finish cache filling
    barrier(CLK_LOCAL_MEM_FENCE);
    
    // check if pixel is in image
    if(gi < IMAGE_WIDTH && gj < IMAGE_HEIGHT)
    {
        // convolved value for pixel 
        float x = 0;
        
        // convolve
        for(j = 0; j < PSF_HEIGHT; ++j)
            for(i = 0; i < PSF_WIDTH; ++i)
                x += psf2[mad24(j, PSF_WIDTH, i)]*input2[mad24(lj + j, cw, li + i)];
        
        // store convolved value
        output[mad24(gj, IMAGE_WIDTH, gi)] = x;
    }
}
