#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "input.h"
#include "input/options.h"
#include "input/ini.h"
#include "log.h"
#include "version.h"

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
        for(size_t i = 0, n = noptions(); i < n; ++i)
        {
            char opt[50];
            char def[100];
            sprintf(opt, "--%.20s=<%s>", option_name(i), option_type(i));
            printf("  %-16s  %s", opt, option_help(i));
            if(!option_required(i))
            {
                option_default_value(def, sizeof(def), i);
                printf(" [default: %s]", def);
            }
            printf(".\n");
        }
    }
    
    exit(EXIT_FAILURE);
}

// show version number and exit
void version()
{
    printf("lensed %d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    exit(EXIT_SUCCESS);
}

// parse command line argument
void read_arg(const char* arg, struct input_options* options)
{
    size_t sep, end = strlen(arg);
    const char* val;
    
    // find equal sign
    for(sep = 0; sep < end; ++sep)
        if(arg[sep] == '=')
            break;
    
    // error if no equal sign was found
    if(sep == end)
        error("option \"--%s\" should be in the form \"--<name>=<value>\"", arg, arg);
    
    // get option value
    val = arg + sep + 1;
    
    // try to parse option
    int err = read_option_n(arg, sep, val, options);
    if(err != OPTION_OK)
        error("%s", options_error(options));
}

void read_input(int argc, char* argv[], struct input* input)
{
    // print usage if no options are given
    if(argc < 2)
        usage(0);
    
    // set input to zero
    memset(input, 0, sizeof(struct input));
    
    // options wrapper for input
    struct input_options* options = get_options(input);
    
    // set default options
    default_options(options);
    
    /* go through arguments */
    for(int i = 1; i < argc; ++i)
    {
        // check for option
        if(argv[i][0] == '-' && argv[i][1])
        {
            // check for long option
            if(argv[i][1] == '-' && argv[i][2])
            {
                // parse long option
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
                    read_arg(argv[i]+2, options);
            }
            else
            {
                // parse short options
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
            // parse *.ini file
            read_ini(argv[i], options);
        }
    }
    
    // make sure all required options are set
    size_t err = check_options(options);
    if(err != -1)
        error("required option \"%s\" not set", option_name(err));
    
    // done with options for input
    free_options(options);
}

void print_input(const struct input* input)
{
    // print options
    if(LOG_LEVEL <= LOG_VERBOSE)
    {
        char value[100];
        verbose("options");
        for(size_t i = 0, n = noptions(); i < n; ++i)
        {
            option_value(value, sizeof(value), input, i);
            verbose("  %s = %s", option_name(i), value);
        }
    }
}
