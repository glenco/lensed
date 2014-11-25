#pragma once

// possible error codes for reading options
enum
{
    OPTION_OK,
    ERR_OPTION_NAME,
    ERR_OPTION_VALUE
};

// object that encapsulates options for a given input
struct input_options;

// create options for input
struct input_options* get_options(struct input* input);

// make sure all required options are set
size_t check_options(const struct input_options* options);

// release the input options object
void free_options(struct input_options* options);

// set default options in input
void default_options(struct input_options* options);

// return message for last error
const char* options_error(const struct input_options* options);

// try to read option into input and return status
int read_option(const char* name, const char* value, struct input_options* options);

// try to read option (n chars) into input and return status
int read_option_n(const char* name, int n, const char* value, struct input_options* options);

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

// write default value of n'th option to buffer
int option_default_value(char* buf, size_t buf_size, size_t n);

// write value of n'th option to buffer
int option_value(char* buf, size_t buf_size, const struct input* input, size_t n);
