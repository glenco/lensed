#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "../input.h"
#include "options.h"
#include "ini.h"
#include "../log.h"

// whitespace characters
static const char* WS = " \t\n\v\f\r";

// assignment characters
static const char* ASSIGN = "=:";

// line-ending characters
static const char* EOL = ";#\n";

static void rtrim(char* str)
{
    size_t len = strlen(str);
    while(len > 0 && strchr(WS, str[len-1]))
        str[--len] = 0;
}

void read_ini(const char* ini, struct input_options* options)
{
    FILE* file;
    char buf[1024];
    size_t len, line, pos;
    char* name;
    char* value;
    
    // try to open file
    file = fopen(ini, "r");
    if(!file)
        errorf(ini, 0, 0, "%s", strerror(errno));
    
    // start with first line
    line = 0;
    
    // read file line by line
    while(fgets(buf, sizeof(buf), file))
    {
        // next line
        line += 1;
        
        // get length of line
        len = strlen(buf);
        
        // make sure a whole line was read
        if(len+1 == sizeof(buf))
            errorf(ini, line, "line too long (max. %zu characters)", sizeof(buf)-1);
        
        // now get length of line until end-of-line token
        len = strcspn(buf, EOL);
        
        // terminate string at EOL
        buf[len] = '\0';
        
        // skip initial whitespace
        pos = strspn(buf, WS);
        
        // whole line is whitespace
        if(pos == len)
            continue;
        
        // name of option starts here
        name = buf + pos;
        
        // find position of assignment char
        pos += strcspn(buf + pos, ASSIGN);
        
        // assignment character was not found
        if(pos == len)
            errorf(ini, line, "line does not assign anything");
        
        // split buffer here
        buf[pos] = '\0';
        
        // trim whitespace from end of name
        rtrim(name);
        
        // advance position past assignment
        pos += 1;
        
        // skip whitespace at beginning of value
        pos += strspn(buf + pos, WS);
        
        // value starts here
        value = buf + pos;
        
        // trim whitespace from end of value
        rtrim(value);
        
        // try to read option
        int err = read_option(name, value, options);
        
        // check for parse errors
        if(err != OPTION_OK)
            errorf(ini, line, "%s", options_error(options));
    }
    
    // check abort condition for errors
    if(ferror(file))
        errorf(ini, line, 0, "%s", strerror(errno));
}
