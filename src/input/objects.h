#pragma once

// add a new object to input
void add_object(input* inp, const char* name, const char* type);

// find object with given name, or return NULL
object* find_object(const input* inp, const char* name);

// free all memory allocated for object
void free_object(object* obj);

// get existing param, or add new param to object
param* get_param(object* obj, const char* name);

// free all memory allocated for param
void free_param(param* par);

// set parameter label
void set_param_label(param* par, const char* label);

// set parameter prior
void set_param_prior(param* par, const char* prior);
