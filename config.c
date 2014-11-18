#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "config.h"
#include "log.h"
#include "version.h"
#include "inih/ini.h"

/* config option meta */
struct config_option
{
    const char* name;
    const char* help;
    int required;
    const char* type;
    int (*read)(const char*, void*);
    int (*write)(char*, const void*);
    union {
        void* default_null;
        const char* default_string;
        int default_bool;
        int default_int;
        double default_real;
    } default_value;
    size_t offset;
    size_t size;
};

/* required or optional fields */
#define config_required(type) 1, #type, read_##type, write_##type, { .default_null = NULL }
#define config_optional(type, value) 0, #type, read_##type, write_##type, { .default_##type = value }

/* get offset of field in config */
#define config_field(field) offsetof(struct config, field), sizeof(((struct config*)0)->field)

/* declare a new type */
#define declare_type(type) \
    int read_##type(const char*, void*); \
    int write_##type(char*, const void*);

/* config option types */
declare_type(string)
declare_type(bool)
declare_type(int)
declare_type(real)

/* list of known config options */
struct config_option OPTIONS[] = {
    {
        "image",
        "Input image, FITS file in counts/sec",
        config_required(string),
        config_field(image)
    },
    {
        "gain",
        "Conversion factor to counts",
        config_required(real),
        config_field(gain)
    },
    {
        "offset",
        "Subtracted flat-field offset",
        config_required(real),
        config_field(offset)
    },
    {
        "root",
        "Root element for all output paths",
        config_required(string),
        config_field(root)
    },
    {
        "mask",
        "Input mask, FITS file",
        config_optional(string, NULL),
        config_field(mask)
    },
    {
        "nlive",
        "Number of live points",
        config_optional(int, 1000),
        config_field(nlive)
    },
    {
        "ins",
        "Use importance nested sampling",
        config_optional(bool, 1),
        config_field(ins)
    },
    {
        "mmodal",
        "Multi-modal posterior (if ins = false)",
        config_optional(bool, 1),
        config_field(mmodal)
    },
    {
        "ceff",
        "Constant efficiency mode",
        config_optional(bool, 0),
        config_field(ceff)
    },
    {
        "evitol",
        "Tolerance for evidence",
        config_optional(real, 0.5),
        config_field(evitol)
    },
    {
        "efr",
        "Sampling efficiency",
        config_optional(real, 0.8),
        config_field(efr)
    },
    {
        "maxmodes",
        "Maximum number of expected modes",
        config_optional(int, 100),
        config_field(maxmodes)
    },
    {
        "updint",
        "Update interval for output",
        config_optional(int, 1000),
        config_field(updint)
    },
    {
        "seed",
        "Random number seed for sampling",
        config_optional(int, -1),
        config_field(seed)
    },
    {
        "fb",
        "Show MultiNest feedback",
        config_optional(bool, 0),
        config_field(fb)
    },
    {
        "resume",
        "Resume from last checkpoint",
        config_optional(bool, 0),
        config_field(resume),
    },
    {
        "outfile",
        "Output MultiNest files",
        config_optional(bool, 0),
        config_field(outfile)
    },
    {
        "maxiter",
        "Maximum number of iterations",
        config_optional(int, 0),
        config_field(maxiter)
    }
};

/* number of known options */
#define NOPTIONS (sizeof(OPTIONS)/sizeof(OPTIONS[0]))

/* default options */
void default_options(struct config* config)
{
    for(int i = 0; i < NOPTIONS; ++i)
        memcpy((char*)config + OPTIONS[i].offset, &OPTIONS[i].default_value, OPTIONS[i].size);
}

/* print usage help */
void usage(int help)
{
    if(help)
    {
        printf("lensed %d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
        printf("\n");
        printf("Reconstruct lenses and sources from observations.\n");
        printf("\n");
    }
    
    printf("Usage:\n");
    printf("  lensed ([<file>] | [options])...\n");
    printf("  lensed -h | --help\n");
    
    if(help)
    {
        printf("\n");
        printf("Options:\n");
        printf("  %-16s  %s\n", "-h, --help", "Show this help message.");
        printf("  %-16s  %s\n", "-v, --verbose", "Verbose output.");
        printf("  %-16s  %s\n", "--warn", "Show only warnings and errors.");
        printf("  %-16s  %s\n", "--error", "Show only errors.");
        printf("  %-16s  %s\n", "-q, --quiet", "Suppress all output.");
        printf("  %-16s  %s\n", "--version", "Show version number.");
        for(int i = 0; i < NOPTIONS; ++i)
        {
            char opt[50];
            char def[100];
            sprintf(opt, "--%.20s=<%s>", OPTIONS[i].name, OPTIONS[i].type);
            printf("  %-16s  %s", opt, OPTIONS[i].help);
            if(!OPTIONS[i].required)
            {
                OPTIONS[i].write(def, &OPTIONS[i].default_value);
                printf(" [default: %s]", def);
            }
            printf(".\n");
        }
    }
    
    exit(EXIT_FAILURE);
}

/* show version number */
void version()
{
    printf("lensed %d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    exit(EXIT_SUCCESS);
}

void read_arg(const char* arg, struct config* config, int options[])
{
    size_t end = strlen(arg);
    size_t sep;
    int opt;
    const char* val;
    
    /* find equal sign */
    for(sep = 1; sep < end; ++sep)
        if(arg[sep] == '=')
            break;
    
    /* find option */
    for(opt = 0; opt < NOPTIONS; ++opt)
        if(strncmp(arg, OPTIONS[opt].name, sep) == 0)
            break;
    
    /* error if option was not found */
    if(opt == NOPTIONS)
        error("invalid option \"%.*s\"", sep, arg);
    
    /* error if no equal sign was found */
    if(sep == end)
        error("option \"%.*s\" should be given as \"%.*s\"=<value>", sep, arg, sep, arg);
    
    /* get value */
    val = arg + sep + 1;
    
    /* try to read option */
    if(OPTIONS[opt].read(val, (char*)config + OPTIONS[opt].offset))
        error("invalid value \"%s\" for option \"%.*s\"", val, sep, arg);
    
    /* mark option as set */
    options[opt] = 1;
}

struct handler_data
{
    struct config* config;
    int* options;
};

int ini_handler(void* data_, const char* section, const char* name, const char* value)
{
    struct handler_data* data = data_;
    
    int opt;
    
    /* find option */
    for(opt = 0; opt < NOPTIONS; ++opt)
        if(strcmp(name, OPTIONS[opt].name) == 0)
            break;
    
    /* error if option was not found */
    if(opt == NOPTIONS)
        return 0;
    
    /* error if no value was given */
    if(!*value)
        return 0;
    
    /* try to read option */
    if(OPTIONS[opt].read(value, (char*)data->config + OPTIONS[opt].offset))
        return 0;
    
    /* mark option as set */
    data->options[opt] = 1;
    
    /* success */
    return 1;
}

void read_ini(const char* ini, struct config* config, int options[])
{
    struct handler_data data = { config, options };
    
    int status = ini_parse(ini, ini_handler, &data);
    
    switch(status)
    {
        case -2: /* memory error */
            error("out of memory while reading \"%s\"", ini);
            
        case -1: /* file error */
            error("could not open file \"%s\"", ini);
            
        case 0: /* done */
            break;
            
        default:
            error("could not read file \"%s\" on line %d", ini, status);
    }
}

void read_config(int argc, char* argv[], struct config* config)
{
    /* track which options are set */
    int options[NOPTIONS] = { 0 };
    
    /* print usage if no config is given */
    if(argc < 2)
        usage(0);
    
    /* zero options */
    memset(config, 0, sizeof(struct config));
    
    /* default options */
    default_options(config);
    
    /* go through arguments */
    for(int i = 1; i < argc; ++i)
    {
        /* check for initial flag */
        if(argv[i][0] == '-' && argv[i][1])
        {
            /* check for long option */
            if(argv[i][1] == '-' && argv[i][2])
            {
                /* parse long options */
                if(strcmp(argv[i]+2, "help") == 0)
                    usage(1);
                else if(strcmp(argv[i]+2, "verbose") == 0)
                    log_level(LOG_VERBOSE);
                else if(strcmp(argv[i]+2, "warn") == 0)
                    log_level(LOG_WARN);
                else if(strcmp(argv[i]+2, "error") == 0)
                    log_level(LOG_ERROR);
                else if(strcmp(argv[i]+2, "quiet") == 0)
                    log_level(LOG_QUIET);
                else if(strcmp(argv[i]+2, "version") == 0)
                    version();
                else
                    read_arg(argv[i]+2, config, options);
            }
            else
            {
                /* parse short options */
                for(int j = 1; argv[i][j]; ++j)
                {
                    switch(argv[i][j])
                    {
                    case 'h':
                        usage(1);
                        break;
                    
                    case 'v':
                        log_level(LOG_VERBOSE);
                        break;
                    
                    case 'q':
                        log_level(LOG_QUIET);
                        break;
                    
                    default:
                        usage(0);
                        break;
                    }
                }
            }
        }
        else
        {
            /* parse *.ini file */
            read_ini(argv[i], config, options);
        }
    }
    
    /* make sure all required options are set */
    for(int i = 0; i < NOPTIONS; ++i)
        if(OPTIONS[i].required && !options[i])
            error("required option \"%s\" not set", OPTIONS[i].name);
}

void print_config(const struct config* config)
{
    if(LOG_LEVEL <= LOG_VERBOSE)
    {
        char value[100];
        verbose("config:");
        for(int i = 0; i < NOPTIONS; ++i)
        {
            OPTIONS[i].write(value, (char*)config + OPTIONS[i].offset);
            verbose("  %s = %s", OPTIONS[i].name, value);
        }
    }
}

int read_string(const char* in, void* out)
{
    char** out_str = out;
    *out_str = malloc(strlen(in) + 1);
    strcpy(*out_str, in);
    return 0;
}

int write_string(char* out, const void* in)
{
    char* const* in_str = in;
    sprintf(out, "%.99s", *in_str ? *in_str : "(none)");
    return 0;
}

int read_bool(const char* in, void* out)
{
    int* out_bool = out;
    if(
        strcmp(in, "true") == 0 ||
        strcmp(in, "TRUE") == 0 ||
        strcmp(in, "1") == 0
    )
        *out_bool = 1;
    else if(
        strcmp(in, "false") == 0 ||
        strcmp(in, "FALSE") == 0 ||
        strcmp(in, "0") == 0
    )
        *out_bool = 0;
    else
        return 1;
    return 0;
}

int write_bool(char* out, const void* in)
{
    const int* in_bool = in;
    sprintf(out, "%.99s", *in_bool ? "true" : "false");
    return 0;
}

int read_int(const char* in, void* out)
{
    int* out_int = out;
    char* end;
    long l = strtol(in, &end, 10);
    if(l < INT_MIN || l > INT_MAX)
        return 1;
    *out_int = l;
    return 0;
}

int write_int(char* out, const void* in)
{
    const int* in_int = in;
    sprintf(out, "%d", *in_int);
    return 0;
}

int read_real(const char* in, void* out)
{
    double* out_double = out;
    char* end;
    *out_double = strtod(in, &end);
    return end != in + strlen(in);
}

int write_real(char* out, const void* in)
{
    const double* in_double = in;
    sprintf(out, "%g", *in_double);
    return 0;
}
