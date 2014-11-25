#pragma once

void load_kernels(size_t nobjects, const char* objects[],
                  size_t* nkernels, const char*** kernels);

char* kernel_name(const char* prefix, const char* name);
