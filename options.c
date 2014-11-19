#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "options.h"
#include "log.h"
#include "version.h"
#include "inih/ini.h"

/* option meta */
struct option
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
#define option_required(type) 1, #type, read_##type, write_##type, { .default_null = NULL }
#define option_optional(type, value) 0, #type, read_##type, write_##type, { .default_##type = value }

/* get offset of field in options */
#define option_field(field) offsetof(struct options, field), sizeof(((struct options*)0)->field)

/* declare a new type */
#define declare_type(type) \
    int read_##type(const char*, void*); \
    int write_##type(char*, const void*);

/* option types */
declare_type(string)
declare_type(bool)
declare_type(int)
declare_type(real)

/* list of known options */
struct option OPTIONS[] = {
    {
        "image",
        "Input image, FITS file in counts/sec",
        option_required(string),
        option_field(image)
    },
    {
        "gain",
        "Conversion factor to counts",
        option_required(real),
        option_field(gain)
    },
    {
        "offset",
        "Subtracted flat-field offset",
        option_required(real),
        option_field(offset)
    },
    {
        "root",
        "Root element for all output paths",
        option_required(string),
        option_field(root)
    },
    {
        "mask",
        "Input mask, FITS file",
        option_optional(string, NULL),
        option_field(mask)
    },
    {
        "nlive",
        "Number of live points",
        option_optional(int, 1000),
        option_field(nlive)
    },
    {
        "ins",
        "Use importance nested sampling",
        option_optional(bool, 1),
        option_field(ins)
    },
    {
        "mmodal",
        "Multi-modal posterior (if ins = false)",
        option_optional(bool, 1),
        option_field(mmodal)
    },
    {
        "ceff",
        "Constant efficiency mode",
        option_optional(bool, 0),
        option_field(ceff)
    },
    {
        "tol",
        "Tolerance for evidence",
        option_optional(real, 0.5),
        option_field(tol)
    },
    {
        "efr",
        "Sampling efficiency",
        option_optional(real, 0.8),
        option_field(efr)
    },
    {
        "maxmodes",
        "Maximum number of expected modes",
        option_optional(int, 100),
        option_field(maxmodes)
    },
    {
        "updint",
        "Update interval for output",
        option_optional(int, 1000),
        option_field(updint)
    },
    {
        "seed",
        "Random number seed for sampling",
        option_optional(int, -1),
        option_field(seed)
    },
    {
        "fb",
        "Show MultiNest feedback",
        option_optional(bool, 0),
        option_field(fb)
    },
    {
        "resume",
        "Resume from last checkpoint",
        option_optional(bool, 0),
        option_field(resume),
    },
    {
        "outfile",
        "Output MultiNest files",
        option_optional(bool, 0),
        option_field(outfile)
    },
    {
        "maxiter",
        "Maximum number of iterations",
        option_optional(int, 0),
        option_field(maxiter)
    }
};

/* number of known options */
#define NOPTIONS (sizeof(OPTIONS)/sizeof(OPTIONS[0]))

/* default options */
void default_options(struct options* options)
{
    for(int i = 0; i < NOPTIONS; ++i)
        memcpy((char*)options + OPTIONS[i].offset, &OPTIONS[i].default_value, OPTIONS[i].size);
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

void read_arg(const char* arg, struct options* options, int resolved[])
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
    if(OPTIONS[opt].read(val, (char*)options + OPTIONS[opt].offset))
        error("invalid value \"%s\" for option \"%.*s\"", val, sep, arg);
    
    /* mark option as resolved */
    resolved[opt] = 1;
}

struct handler_data
{
    struct options* options;
    int* resolved;
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
    if(OPTIONS[opt].read(value, (char*)data->options + OPTIONS[opt].offset))
        return 0;
    
    /* mark option as resolved */
    data->resolved[opt] = 1;
    
    /* success */
    return 1;
}

void read_ini(const char* ini, struct options* options, int resolved[])
{
    struct handler_data data = { options, resolved };
    
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

void read_options(int argc, char* argv[], struct options* options)
{
    /* track which options are resolved */
    int resolved[NOPTIONS] = { 0 };
    
    /* print usage if no options are given */
    if(argc < 2)
        usage(0);
    
    /* zero options */
    memset(options, 0, sizeof(struct options));
    
    /* default options */
    default_options(options);
    
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
                    read_arg(argv[i]+2, options, resolved);
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
            read_ini(argv[i], options, resolved);
        }
    }
    
    /* make sure all required options are resolved */
    for(int i = 0; i < NOPTIONS; ++i)
        if(OPTIONS[i].required && !resolved[i])
            error("required option \"%s\" not set", OPTIONS[i].name);
}

void print_options(const struct options* options)
{
    if(LOG_LEVEL <= LOG_VERBOSE)
    {
        char value[100];
        verbose("options");
        for(int i = 0; i < NOPTIONS; ++i)
        {
            OPTIONS[i].write(value, (char*)options + OPTIONS[i].offset);
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
