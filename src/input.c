#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "input.h"
#include "input/objects.h"
#include "input/options.h"
#include "input/ini.h"
#include "prior.h"
#include "log.h"
#include "path.h"
#include "version.h"

// default config file
const char DEFAULT_INI[] = "default.ini";

// print usage help
void usage(int help)
{
    if(help)
    {
        printf("Lensed %s\n", LENSED_VERSION);
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
        printf("  %-16s  %s\n", "-b, --batch", "Batch output.");
        printf("  %-16s  %s\n", "-q, --quiet", "Suppress all output.");
        printf("  %-16s  %s\n", "--version", "Show version number.");
        printf("  %-16s  %s\n", "--devices", "List computation devices.");
        printf("  %-16s  %s\n", "--batch-header", "Batch output header.");
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
    printf("Lensed %s\n", LENSED_VERSION);
    exit(EXIT_SUCCESS);
}

// parse command line argument
void read_arg(const char* arg, input* inp)
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
    int err = read_option_n(inp, arg, sep, val);
    if(err)
        error("%s", options_error());
}

input* read_input(int argc, char* argv[])
{
    // print usage if no options are given
    if(argc < 2)
        usage(0);
    
    // allocate memory for input
    input* inp = malloc(sizeof(input));
    if(!inp)
        errori("could not create input");
    
    // create options
    inp->opts = create_options();
    inp->reqs = malloc(noptions()*sizeof(int));
    
    // create objects
    inp->nobjs = 0;
    inp->objs = NULL;
    
    // set default options
    default_options(inp);
    
    // check for a default config file
    {
        char* filename;
        FILE* fp;
        
        // get filename of default config file
        filename = malloc(strlen(LENSED_PATH) + strlen(DEFAULT_INI) + 1);
        if(!filename)
            errori(NULL);
        sprintf(filename, "%s%s", LENSED_PATH, DEFAULT_INI);
        
        // check if default config file exists
        fp = fopen(filename, "r");
        
        // if file exists, read it
        if(fp)
        {
            fclose(fp);
            read_ini(filename, inp);
        }
        
        free(filename);
    }
    
    // go through arguments
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
                else if(strcmp(argv[i]+2, "batch") == 0)
                    log_level(LOG_BATCH);
                else if(strcmp(argv[i]+2, "quiet") == 0)
                    log_level(LOG_QUIET);
                else if(strcmp(argv[i]+2, "version") == 0)
                    version();
                else if(strcmp(argv[i]+2, "devices") == 0)
                    inp->opts->devices = 1;
                else if(strcmp(argv[i]+2, "batch-header") == 0)
                    inp->opts->batch_header = 1;
                else
                    read_arg(argv[i]+2, inp);
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
                    
                    case 'b':
                        log_level(LOG_BATCH);
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
            // parse ini file
            read_ini(argv[i], inp);
        }
    }
    
    // check input if not in a special mode
    if(!(inp->opts->devices || inp->opts->batch_header))
    {
        // make sure all required options are resolved
        for(size_t i = 0, n = noptions(); i < n; ++i)
            if(!option_resolved(i, inp->opts, inp->reqs))
                error("missing required option: %s", option_name(i));
        
        // make sure that some objects are given
        if(inp->nobjs == 0)
            error("no objects were given (check [objects] section)");
        
        // make sure that all parameters have priors
        for(size_t i = 0; i < inp->nobjs; ++i)
            for(size_t j = 0; j < inp->objs[i].npars; ++j)
                if(!inp->objs[i].pars[j].pri)
                    error("missing prior: %s.%s (check [priors] section)", inp->objs[i].id, inp->objs[i].pars[j].name);
    }
    
    // everything is fine
    return inp;
}

void print_input(const input* inp)
{
    // print options
    if(LOG_LEVEL <= LOG_VERBOSE)
    {
        char value[100];
        
        verbose("options");
        for(size_t i = 0, n = noptions(); i < n; ++i)
        {
            option_value(value, sizeof(value), inp, i);
            verbose("  %s = %s", option_name(i), value);
        }
        
        verbose("objects");
        for(size_t i = 0; i < inp->nobjs; ++i)
            verbose("  %s = %s", inp->objs[i].id, inp->objs[i].name);
        
        verbose("parameters");
        for(size_t i = 0, p = 1; i < inp->nobjs; ++i)
        {
            for(size_t j = 0; j < inp->objs[i].npars; ++j, ++p)
            {
                char buf[100] = {0};
                print_prior(inp->objs[i].pars[j].pri, buf, 99);
                
                verbose("  %zu: %s ~ %s", p, inp->objs[i].pars[j].label ? inp->objs[i].pars[j].label : inp->objs[i].pars[j].id, buf);
            }
        }
    }
}

void free_input(input* inp)
{
    free_options(inp->opts);
    free(inp->reqs);
    
    for(size_t i = 0; i < inp->nobjs; ++i)
        free_object(&inp->objs[i]);
    free(inp->objs);
    
    free(inp);
}
