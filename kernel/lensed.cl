// compute image
kernel void render(constant char* data, float4 pcs, global float2* model)
{
    // get pixel index
    int k = get_global_id(0);
    
    // compute pixel flux if pixel is in image
    if(k < IMAGE_SIZE)
    {
        // base position on quadrature grid
        float2 x = pcs.xy + pcs.zw*(float2)(k%IMAGE_WIDTH - 0.5f + 0.5f/NQ, k/IMAGE_WIDTH - 0.5f + 0.5f/NQ);
        
        // number of points, mean and variance
        size_t c = 0;
        float m = 0;
        float s = 0;
        
        // compute mean and variance on quadrature grid
        for(size_t u = 0; u < NQ; ++u)
        for(size_t v = 0; v < NQ; ++v)
        {
            // function value at grid point
            float f = compute(data, x + pcs.zw*(float2)(u, v)/NQ);
            
            // difference to current mean
            float df = f - m;
            
            // running mean and variance calculation
            c += 1;
            m += df/c;
            s += df*(f - m);
        }
        
        // store model mean and variance of mean
        model[k] = (float2)(m, s/(c - 1)/c);
    }
}

// calculate log-likelihood of computed model
kernel void loglike(global const float* image, global const float* weight,
                    global const float2* model, global float* loglike)
{
    // get pixel index
    int k = get_global_id(0);
    
    // compute log-likelihood value if pixel is in image
    if(k < IMAGE_SIZE)
    {
        float i = image[k];
        float w = weight[k];
        float2 m = model[k];
        float d = i - m.x;
        float s = 1 + w*m.y;
        loglike[k] = -0.5f*(w*d*d/s + log(s));
    }
}

// convolve input with PSF
kernel void convolve(global const float2* input, constant float* psf,
                     local float2* input2, local float* psf2,
                     global float2* output)
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
        float2 x = 0;
        
        // convolve
        for(j = 0; j < PSF_HEIGHT; ++j)
        for(i = 0; i < PSF_WIDTH; ++i)
        {
            float w = psf2[mad24(j, PSF_WIDTH, i)];
            x += (float2)(w, w*w)*input2[mad24(lj + j, cw, li + i)];
        }
        
        // store convolved value
        output[mad24(gj, IMAGE_WIDTH, gi)] = x;
    }
}
