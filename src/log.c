// provide POSIX standard in strict C99 mode
#define _XOPEN_SOURCE 600

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>

#include "log.h"

// output for logging, or 0 for stdout
static FILE* logout = 0;

// global log-level
enum log_level LOG_LEVEL = LOG_INFO;

void log_level(enum log_level log_level)
{
    LOG_LEVEL = log_level;
}

void verbose(const char* msg, ...)
{
    if(LOG_LEVEL <= LOG_VERBOSE)
    {
        va_list args;
        va_start(args, msg);
        
        fprintf(logout ? logout : stdout, LOG_DARK);
        vfprintf(logout ? logout : stdout, msg, args);
        fprintf(logout ? logout : stdout, LOG_RESET "\n");
        
        va_end(args);
    }
}

void info(const char* msg, ...)
{
    if(LOG_LEVEL <= LOG_INFO)
    {
        va_list args;
        va_start(args, msg);
        
        vfprintf(logout ? logout : stdout, msg, args);
        fprintf(logout ? logout : stdout, "\n");
        
        va_end(args);
    }
}

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

void errorfi(const char* file, size_t line, const char* msg, ...)
{
    if(LOG_LEVEL <= LOG_ERROR)
    {
        fprintf(stderr, "%s:", file);
        if(line)
            fprintf(stderr, "%zu:", line);
        fprintf(stderr, " ");
        
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

void logfile(FILE* f)
{
    // redirect if file is given, else restore stdout
    if(f)
    {
        // flush standard output
        fflush(stdout);
        
        // copy standard output
        logout = fdopen(dup(STDOUT_FILENO), "w");
        if(!logout)
            errori("could not redirect output");
        
        // redirect standard output to logfile
        dup2(fileno(f), STDOUT_FILENO);
    }
    else
    {
        // flush redirected output
        fflush(stdout);
        
        // restore standard output
        dup2(fileno(logout), STDOUT_FILENO);
        
        // close copy of standard output
        fclose(logout);
        
        // log to stdout
        logout = 0;
    }
}
