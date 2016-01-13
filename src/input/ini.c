#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../input.h"
#include "objects.h"
#include "options.h"
#include "ini.h"
#include "../log.h"

#ifndef LINE_LEN
#define LINE_LEN 1024
#endif

// whitespace characters
static const char* WS = " \t\n\v\f\r";

// assignment characters
static const char* ASSIGN = "=:";

// line-ending characters
static const char* EOL = ";#\n";

// identifier separator
static const char* SEP = ".";

// trim characters from beginning of string
static void ltrim(char* str, const char* trim)
{
    size_t len = strlen(str);
    size_t beg = strspn(str, trim);
    if(beg > 0)
        for(size_t i = beg; i <= len; ++i)
            str[i-beg] = str[i];
}

// trim characters from end of string
static void rtrim(char* str, const char* trim)
{
    size_t len = strlen(str);
    while(len > 0 && strchr(trim, str[len-1]))
        str[--len] = '\0';
}

// split str using any of the characters spl, or return NULL if unsuccessful
static char* split(char* str, const char* spl)
{
    char* pos = str + strcspn(str, spl);
    if(*pos == '\0')
        return NULL;
    *pos = '\0';
    return pos + 1;
}

// get directory part from path
char* dirname(const char* path)
{
    char* dir;
    char* sep;
    
    // make a copy of input string
    dir = malloc(strlen(path) + 1);
    if(!dir)
        errori(NULL);
    strcpy(dir, path);
    
    // find last directory separator in path
    sep = strrchr(dir, '/');
    
    // no directory if separator was not found
    if(!sep)
        return NULL;
    
    // terminate directory at last separator
    *sep = '\0';
    
    // done
    return dir;
}

// known groups in ini file
enum
{
    GRP_OPTIONS,
    GRP_OBJECTS,
    GRP_PRIORS,
    GRP_LABELS
};

// assign group id to group name
static const struct { const char* name; const int grp; } GROUPS[] = {
    { "options", GRP_OPTIONS },
    { "objects", GRP_OBJECTS },
    { "priors", GRP_PRIORS },
    { "labels", GRP_LABELS }
};

// find group id by name
int findgrp(const char* name)
{
    int i = 0;
    int n = sizeof(GROUPS)/sizeof(GROUPS[0]);
    for(; i < n; ++i)
        if(strcmp(name, GROUPS[i].name) == 0)
            return GROUPS[i].grp;
    return -1;
}

void read_ini(const char* ini, input* inp)
{
    const char* cwd;
    char* path;
    FILE* file;
    char buf[LINE_LEN];
    size_t len, line;
    char* name;
    char* value;
    char* sub;
    object* obj;
    param* par;
    int grp;
    int err;
    
    // no initial object or param
    obj = NULL;
    par = NULL;
    
    // try to open file
    file = fopen(ini, "r");
    if(!file)
        errorf(ini, 0, "could not open file");
    
    // set path of file as working directory for options
    path = dirname(ini);
    cwd = options_cwd(path);
    
    // start with first line
    line = 0;
    
    // initial group is "options"
    grp = GRP_OPTIONS;
    
    // read file line by line
    while(fgets(buf, sizeof(buf), file))
    {
        // next line
        line += 1;
        
        // get length until end-of-line token
        len = strcspn(buf, EOL);
        
        // make sure a whole line was read (i.e. line ends with EOL character)
        if(buf[len] == '\0' && !feof(file))
            errorf(ini, line, "line too long (max. %zu characters)", sizeof(buf));
        
        // terminate string at EOL
        buf[len] = '\0';
        
        // trim whitespace from both sides of line
        rtrim(buf, WS);
        ltrim(buf, WS);
        
        // get current length of line
        len = strlen(buf);
        
        // skip empty lines
        if(buf[0] == '\0')
            continue;
        
        // check if line starts group
        if(buf[0] == '[')
        {
            // make sure group name is closed
            if(buf[len-1] != ']')
                errorf(ini, line, "missing closing ']' character for group");
            
            // group name starts after [ and ends before ]
            buf[len-1] = '\0';
            name = buf + 1;
            
            // trim whitespace from group name
            rtrim(name, WS);
            ltrim(name, WS);
            
            // get id of new group
            grp = findgrp(name);
            
            // make sure group is valid
            if(grp < 0)
                errorf(ini, line, "unknown group: %s", name);
            
            // done changing group
            continue;
        }
        
        // name starts at beginning of line
        name = buf;
        
        // split line into name and value by one of the assignment characters
        value = split(buf, ASSIGN);
        
        // make sure a value was assigned
        if(!value)
            errorf(ini, line, "line does not assign anything to \"%s\"", name);
        
        // trim whitespace from name and value
        rtrim(name, WS);
        ltrim(value, WS);
        
        // if group requires it, get object and parameter from name
        if(grp == GRP_PRIORS || grp == GRP_LABELS)
        {
            sub = split(name, SEP);
            if(!sub)
                errorf(ini, line, "invalid parameter name (should be <object>.<param>)");
            rtrim(name, WS);
            ltrim(sub, WS);
            if(!*sub)
                errorf(ini, line, "object %s: no parameter given (should be %s.<param>)", name, name);
            obj = find_object(inp, name);
            if(!obj)
                errorf(ini, line, "unknown object: %s (check [objects] group)", name);
            par = find_param(obj, sub);
            if(!par)
                errorf(ini, line, "object %s: unknown parameter %s", name, sub);
        }
        
        // use name and value according to current group
        switch(grp)
        {
        case GRP_OPTIONS:
            err = read_option(inp, name, value);
            if(err)
                errorf(ini, line, "%s", options_error());
            break;
            
        case GRP_OBJECTS:
            obj = find_object(inp, name);
            if(obj)
                errorf(ini, line, "duplicate object name: %s", name);
            add_object(inp, name, value);
            break;
            
        case GRP_PRIORS:
            set_param_prior(par, value);
            break;
            
        case GRP_LABELS:
            set_param_label(par, value);
            break;
        }
    }
    
    // check abort condition for errors
    if(ferror(file))
        errori(NULL);
    
    // close ini file
    fclose(file);
    
    // reset working directory for options
    options_cwd(cwd);
    free(path);
}
