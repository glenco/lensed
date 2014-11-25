#pragma once

// add a new object to input
int add_object(input* inp, const char* name, const char* type);

// find object with given name, or return NULL
object* find_object(const input* inp, const char* name);

// free all memory allocated for object
void free_object(object* obj);

// get message for last error
const char* objects_error();
