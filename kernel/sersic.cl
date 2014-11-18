struct sersic
{
    float x;
    float y;
    float q;
    float cos;
    float sin;
    float log0;
    float log1;
    float m;
};

kernel void sersic(global const struct sersic* src,
                   global float2* yy, global float* ff)
{
    size_t i = get_global_id(0);
    
    float x = (yy[i].x - src->x)*src->cos - (yy[i].y - src->y)*src->sin;
    float y = (yy[i].y - src->y)*src->cos + (yy[i].x - src->x)*src->sin;
    
    float r2 = x*x + y*y/src->q/src->q;
    
    ff[i] += exp(src->log0 - exp(src->log1 + src->m*log(r2)));
}

kernel void size_sersic(global size_t* size)
{
    *size = sizeof(struct sersic);
}

kernel void ndim_sersic(global size_t* ndim)
{
    *ndim = 7;
}

kernel void set_sersic(global struct sersic* src, global const float* params, size_t off)
{
    enum { X, Y, R_EFF, MAG, N, Q, PA };
    
    global const float* P = params + off;
    
    float b = 1.9992f*P[N] - 0.3271f; // approximation valid for 0.5 < n < 8
    
    src->x = P[X];
    src->y = P[Y];
    src->q = P[Q];
    
    src->cos = cos(P[PA]);
    src->sin = sin(P[PA]);
    src->log0 = 0.4f*P[MAG]*LOG_10 + 2*P[N]*log(b) - LOG_PI - log(P[Q]) - 2*log(P[R_EFF]) - log(tgamma(2*P[N]+1));
    src->log1 = log(b) - log(P[R_EFF])/P[N];
    src->m = 0.5f/P[N];
}

kernel void wrap_sersic(global int* wrap)
{
    enum { X, Y, R_EFF, MAG, N, F, PA };
    
    wrap[X] = 0;
    wrap[Y] = 0;
    wrap[R_EFF] = 0;
    wrap[MAG] = 0;
    wrap[N] = 0;
    wrap[F] = 0;
    wrap[PA] = 1;
}
