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

// tag for build type
#ifdef LENSED_DEBUG
const char* LENSED_BUILD_TAG = " (debug build)";
#else
const char* LENSED_BUILD_TAG = "";
#endif

// print usage help
void usage(int help)
{
    if(help)
    {
        printf("Lensed %s%s\n", LENSED_VERSION, LENSED_BUILD_TAG);
        printf("\n");
        printf("Reconstruct gravitational lenses and sources from observations.\n");
        printf("\n");
    }
    
    printf("Usage:\n");
    printf("  lensed [flags] ([<file>] | [options])...\n");
    printf("  lensed -h | --help\n");
    
    if(help)
    {
        printf("\n");
        printf("Flags:\n");
        printf("  %-20s  %s\n", "-h, --help", "Show this help message.");
        printf("  %-20s  %s\n", "-v, --verbose", "Verbose output.");
        printf("  %-20s  %s\n", "--warn", "Show only warnings and errors.");
        printf("  %-20s  %s\n", "--error", "Show only errors.");
        printf("  %-20s  %s\n", "-b, --batch", "Batch output.");
        printf("  %-20s  %s\n", "-q, --quiet", "Suppress all output.");
        printf("  %-20s  %s\n", "--version", "Show version number.");
        printf("  %-20s  %s\n", "--devices", "List computation devices.");
        printf("  %-20s  %s\n", "--profile", "Enable OpenCL profiling.");
        printf("  %-20s  %s\n", "--batch-header", "Batch output header.");
        printf("  %-20s  %s\n", "--rules", "List known quadrature rules.");
        
        printf("\n");
        printf("Options:\n");
        for(size_t i = 0, n = noptions(); i < n; ++i)
        {
            char opt[50];
            char def[100];
            sprintf(opt, "--%.20s=<%s>", option_name(i), option_type(i));
            printf("  %-20s  %s", opt, option_help(i));
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
    printf("Lensed %s%s\n", LENSED_VERSION, LENSED_BUILD_TAG);
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
    
    // get option value
    val = arg + sep + ((sep < end) ? 1 : 0);
    
    // try to parse option
    int err = read_option_n(inp, arg, sep, val);
    if(err)
        error("%s", options_error());
}

input* read_input(int argc, char* argv[])
{
    // flag when lensed runs in a special mode
    int is_special;
    
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
                else if(strcmp(argv[i]+2, "profile") == 0)
                    inp->opts->profile = 1;
                else if(strcmp(argv[i]+2, "batch-header") == 0)
                    inp->opts->batch_header = 1;
                else if(strcmp(argv[i]+2, "rules") == 0)
                    inp->opts->show_rules = 1;
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
    
    // check if lensed runs in a special mode
    is_special = inp->opts->devices || inp->opts->batch_header ||
                 inp->opts->show_rules;
    
    // check input if not in a special mode
    if(!is_special)
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
                    error("missing prior: %s (check [priors] section)", inp->objs[i].pars[j].id);
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
        for(size_t i = 0, p = 0; i < inp->nobjs; ++i)
        {
            for(size_t j = 0; j < inp->objs[i].npars; ++j)
            {
                const char* label;
                char rel;
                char buf[256] = {0};
                char tag[256] = {0};
                
                // current parameter
                const param* par = &inp->objs[i].pars[j];
                
                // skip default values
                if(par->defval)
                    continue;
                
                // set label, or id if no label set
                label = par->label ? par->label : par->id;
                
                // relation between variable and prior
                rel = par->derived ? '=' : '~';
                
                // pretty-print prior
                prior_print(par->pri, buf, 255);
                
                // collect tags
                snprintf(tag, 255, " [%s%s%s%s%s%s%s%s%s%s",
                    par->type == PAR_POSITION_X ? "position x, " : "",
                    par->type == PAR_POSITION_Y ? "position y, " : "",
                    par->type == PAR_RADIUS     ? "radius, "     : "",
                    par->type == PAR_MAGNITUDE  ? "magnitude, "  : "",
                    par->type == PAR_AXIS_RATIO ? "axis ratio, " : "",
                    par->type == PAR_POS_ANGLE  ? "pos. angle, " : "",
                    par->type == PAR_SCALE      ? "scale, "      : "",
                    par->bounded                ? "bounded, "    : "",
                    par->wrap                   ? "wrap, "       : "",
                    par->ipp                    ? "IPP, "        : ""
                );
                
                // check if tags were set
                if(strcmp(tag, " [") == 0)
                {
                    // no tags, empty string
                    *tag = '\0';
                }
                else
                {
                    // terminate tags
                    size_t len = strlen(tag);
                    tag[len-1] = '\0';
                    tag[len-2] = ']';
                }
                
                // output line for parameter
                verbose("  %zu: %s %c %s%s", ++p, label, rel, buf, tag);
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
