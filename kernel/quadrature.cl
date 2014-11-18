kernel void quadrature(size_t nq, global const float* ff,
                       global const float* ww, global const float* ee,
                       global float* qq, global float* pp)
{
    size_t i = get_global_id(0);
    
    global const float* f = ff + i*nq;
    global const float* w = ww + i*nq;
    global const float* e = ee + i*nq;
    
    for(size_t j = 0; j < nq; ++j)
    {
        qq[i] += w[j]*f[j];
        pp[i] += e[j]*f[j];
    }
}
