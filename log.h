#pragma once

/* the diverse levels of logging */
extern enum log_level
{
    LOG_VERBOSE,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_QUIET
} LOG_LEVEL;

/* set the log level */
void log_level(enum log_level);

/* print a verbose message */
void verbose(const char* msg, ...);

/* print an informative message */
void info(const char* msg, ...);

/* print a warning message */
void warn(const char* msg, ...);

/* print an error message and exit */
void error(const char* msg, ...);
