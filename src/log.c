#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "log.h"

// global log-level
enum log_level LOG_LEVEL = LOG_INFO;

// set log level
void log_level(enum log_level log_level)
{
    LOG_LEVEL = log_level;
}

// log verbose
void verbose(const char* msg, ...)
{
    if(LOG_LEVEL <= LOG_VERBOSE)
    {
        va_list args;
        va_start(args, msg);
        
        fprintf(stdout, LOG_DARK);
        vfprintf(stdout, msg, args);
        fprintf(stdout, LOG_RESET "\n");
        
        va_end(args);
    }
}

// log info
void info(const char* msg, ...)
{
    if(LOG_LEVEL <= LOG_INFO)
    {
        va_list args;
        va_start(args, msg);
        
        vfprintf(stdout, msg, args);
        fprintf(stdout, "\n");
        
        va_end(args);
    }
}

// log warning
void warn(const char* msg, ...)
{
    if(LOG_LEVEL <= LOG_WARN)
    {
        va_list args;
        va_start(args, msg);
        
        fprintf(stderr, LOG_BOLD "warning: " LOG_RESET);
        vfprintf(stderr, msg, args);
        fprintf(stderr, "\n");
        
        va_end(args);
        
        if(LOG_LEVEL <= LOG_INFO)
        {
            fprintf(stderr, "enter 'x' to abort or nothing to continue: ");
            fflush(stderr);
            
            if(getchar() == 'x')
            {
                fprintf(stderr, "aborted!\n");
                exit(1);
            }
        }
    }
}

// log error and exit
void error(const char* msg, ...)
{
    if(LOG_LEVEL <= LOG_ERROR)
    {
        va_list args;
        va_start(args, msg);
        
        fprintf(stderr, LOG_BOLD "error: " LOG_RESET);
        vfprintf(stderr, msg, args);
        fprintf(stderr, "\n");
        
        va_end(args);
    }
    
    exit(EXIT_FAILURE);
}

// log file error and exit
void errorf(const char* file, size_t line, const char* msg, ...)
{
    if(LOG_LEVEL <= LOG_ERROR)
    {
        fprintf(stderr, "%s:", file);
        if(line)
            fprintf(stderr, "%zu:", line);
        fprintf(stderr, " ");
        
        va_list args;
        va_start(args, msg);
        
        fprintf(stderr, LOG_BOLD "error: " LOG_RESET);
        vfprintf(stderr, msg, args);
        fprintf(stderr, "\n");
        
        va_end(args);
    }
    
    exit(EXIT_FAILURE);
}

// log internal error and exit
void errori(const char* msg, ...)
{
    if(LOG_LEVEL <= LOG_ERROR)
    {
        fprintf(stderr, LOG_BOLD "error: " LOG_RESET);
        
        if(msg)
        {
            va_list args;
            va_start(args, msg);
            vfprintf(stderr, msg, args);
            fprintf(stderr, ": ");
            va_end(args);
        }
        
        fprintf(stderr, "%s", strerror(errno));
        fprintf(stderr, "\n");
    }
    
    exit(EXIT_FAILURE);
}
