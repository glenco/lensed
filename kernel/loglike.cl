kernel void loglike(global const float* model, global const float* error,
                    global const float* mean, global const float* variance,
                    global float* chi_sq, global float* log_norm)
{
    size_t i = get_global_id(0);
    
    float x = model[i] - mean[i];
    float v = variance[i] + error[i]*error[i];
    chi_sq[i] = x*x/v;
    log_norm[i] = -0.5f*(LOG_2PI + log(v));
}
