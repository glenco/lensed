struct sie
{
    float x;
    float y;
    float c;
    float s;
    
    float f2;
    float e;
    float d;
};

kernel void sie(global const struct sie* sie,
                global const float2* xx, global float2* aa)
{
    float x, y, r;
    
    size_t i = get_global_id(0);
    
    x = (xx[i].x - sie->x)*sie->c - (xx[i].y - sie->y)*sie->s;
    y = (xx[i].y - sie->y)*sie->c + (xx[i].x - sie->x)*sie->s;
    
    r = sie->e/sqrt(sie->f2*x*x + y*y);
    x = sie->d*atan(x*r);
    y = sie->d*atanh(y*r);
    
    aa[i].x += x*sie->c + y*sie->s;
    aa[i].y += y*sie->c - x*sie->s;
}

kernel void size_sie(global size_t* size)
{
    *size = sizeof(struct sie);
}

kernel void ndim_sie(global size_t* ndim)
{
    *ndim = 5;
}

kernel void set_sie(global struct sie* sie, global const float* params, size_t off)
{
    enum { X, Y, R_E, F, PA };
    
    global const float* P = params + off;
    
    sie->x = P[X];
    sie->y = P[Y];
    sie->c = cos(PI_HALF - P[PA]);
    sie->s = sin(PI_HALF - P[PA]);
    sie->f2 = P[F]*P[F];
    sie->e = sqrt(1.0f - sie->f2);
    sie->d = P[R_E]*sqrt(P[F])/sie->e;
}

kernel void wrap_sie(global int* wrap)
{
    enum { X, Y, R_E, F, PA };
    
    wrap[X] = 0;
    wrap[Y] = 0;
    wrap[R_E] = 0;
    wrap[F] = 0;
    wrap[PA] = 1;
}
