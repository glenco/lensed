#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "../input.h"
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

// known groups in ini file
enum
{
    GRP_OPTIONS
};

// assign group id to group name
static const struct { const char* name; const int grp; } GROUPS[] = {
    { "options", GRP_OPTIONS }
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

void read_ini(const char* ini, struct input_options* options)
{
    FILE* file;
    char buf[LINE_LEN];
    size_t len, line;
    char* name;
    char* value;
    int grp;
    int err;
    
    // try to open file
    file = fopen(ini, "r");
    if(!file)
        errorf(ini, 0, 0, "%s", strerror(errno));
    
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
        if(buf[len] == '\0')
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
                errorf(ini, line, "unknown group \"%s\"", name);
            
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
        
        // use name and value according to current group
        switch(grp)
        {
        case GRP_OPTIONS:
            err = read_option(name, value, options);
            if(err != OPTION_OK)
                errorf(ini, line, "%s", options_error(options));
            break;
        }
    }
    
    // check abort condition for errors
    if(ferror(file))
        errorf(ini, line, "%s", strerror(errno));
    
    // close ini file
    fclose(file);
}
