// provide POSIX standard in strict C99 mode
#define _XOPEN_SOURCE 600

#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#include "log.h"

const char* LENSED_PATH = NULL;

#if defined(__APPLE__)
#include <stdint.h>
#include <mach-o/dyld.h>
static char* find_lensed_exe()
{
    char* buf = NULL;
    uint32_t bufsiz = 0;
    _NSGetExecutablePath(buf, &bufsiz);
    buf = malloc(bufsiz);
    if(!buf)
        errori(NULL);
    if(_NSGetExecutablePath(buf, &bufsiz) != 0)
        return NULL;
    return buf;
}
#elif 1 || defined(__linux__) || defined(__unix__) || defined(__unix)
#include <unistd.h>
static char* find_lensed_exe()
{
    const char* symlinks[] = {
        "/proc/self/exe",
        "/proc/curproc/file",
        "/proc/self/path/a.out",
        NULL
    };
    
    ssize_t len = -1;
    size_t bufsiz = 1024;
    char* buf = malloc(bufsiz);
    if(!buf)
        errori(NULL);
    
    for(const char* s = *symlinks; len == -1 && s != NULL; ++s)
        len = readlink(s, buf, bufsiz);
    
    if(len == -1)
        return NULL;
    
    buf[len] = '\0';
    
    return buf;
}
#endif

void init_lensed_path()
{
    char* dir;
    
    // get path to executable
    char* path = find_lensed_exe();
    if(!path)
        error("could not get path to lensed");
    
    // get bin folder
    dir = dirname(path);
    strcpy(path, dir);
    
    // get lensed folder
    dir = dirname(path);
    strcpy(path, dir);
    
    // append trailing slash
    strcat(path, "/");
    
    // store lensed's path
    LENSED_PATH = path;
}
