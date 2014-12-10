#pragma once

// create options
options* create_options();

// free all memory allocated for options
void free_options(options* opts);

// set default options in input
void default_options(input* inp);

// try to read option into input and return status
int read_option(input* inp, const char* name, const char* value);

// try to read option (n chars) into input and return status
int read_option_n(input* inp, const char* name, int n, const char* value);

// return the last error message
const char* options_error();

// get number of options
size_t noptions();

// get name of n'th option
const char* option_name(size_t n);

// get type of n'th option
const char* option_type(size_t n);

// get help of n'th option
const char* option_help(size_t n);

// return whether n'th option is required (true) or optional (false)
int option_required(size_t n);

// check whether option was not resolved
int option_resolved(size_t n, options* opts, int reqs[]);

// write default value of n'th option to buffer
int option_default_value(char* buf, size_t buf_size, size_t n);

// write value of n'th option to buffer
int option_value(char* buf, size_t buf_size, const input* inp, size_t n);

// set current working directory for option reading and return old cwd
const char* options_cwd(const char* cwd);
