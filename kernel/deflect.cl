kernel void deflect(global const float2* xx, global const float2* aa, global float2* yy)
{
    size_t i = get_global_id(0);
    yy[i] = xx[i] - aa[i];
}
