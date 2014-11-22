#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "log.h"

/* global log-level */
enum log_level LOG_LEVEL = LOG_INFO;

/* set log level */
void log_level(enum log_level log_level)
{
    LOG_LEVEL = log_level;
}

/* log verbose */
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

/* log info */
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

/* log warning */
void warn(const char* msg, ...)
{
    if(LOG_LEVEL <= LOG_WARN)
    {
        va_list args;
        va_start(args, msg);
        
        fprintf(stderr, "WARNING: ");
        vfprintf(stderr, msg, args);
        fprintf(stderr, "\n");
        
        va_end(args);
        
        fprintf(stderr, "Enter 'x' to abort or nothing to continue: ");
        fflush(stderr);
        char c = getchar();
        if(c == 'x')
            exit(1);
    }
}

/* log error and exit */
void error(const char* msg, ...)
{
    if(LOG_LEVEL <= LOG_ERROR)
    {
        va_list args;
        va_start(args, msg);
        
        fprintf(stderr, "ERROR: ");
        vfprintf(stderr, msg, args);
        fprintf(stderr, "\n");
        
        va_end(args);
    }
    
    exit(EXIT_FAILURE);
}
