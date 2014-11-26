#pragma once

// add a new object to input
void add_object(input* inp, const char* id, const char* name);

// find object with given name, or return NULL
object* find_object(const input* inp, const char* id);

// free all memory allocated for object
void free_object(object* obj);

// find parameter with given name, or return NULL
param* find_param(object* obj, const char* name);

// free all memory allocated for param
void free_param(param* par);

// set parameter label
void set_param_label(param* par, const char* label);

// set parameter prior
void set_param_prior(param* par, const char* prior);
