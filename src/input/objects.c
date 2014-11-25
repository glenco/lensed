#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "../input.h"
#include "objects.h"
#include "../log.h"

// buffer for last error message
static char ERROR_MSG[1024] = {0};

int add_object(input* inp, const char* name, const char* type)
{
    object* obj;
    
    // make sure object does not yet exist
    if(find_object(inp, name))
    {
        snprintf(ERROR_MSG, sizeof(ERROR_MSG)-1, "duplicate object name \"%s\"", name);
        return 1;
    }
    
    // make sure that a type is given
    if(!type || !*type)
    {
        snprintf(ERROR_MSG, sizeof(ERROR_MSG)-1, "missing type for object \"%s\"", name);
        return 1;
    }
    
    // realloc space for one more object
    inp->nobjs += 1;
    inp->objs = realloc(inp->objs, inp->nobjs*sizeof(object));
    if(!inp->objs)
        error("could not add object \"%s\": %s", name, strerror(errno));
    
    // realloc was successful, get new object
    obj = &inp->objs[inp->nobjs-1];
    
    // allocate space and copy name and type into object
    obj->name = malloc(strlen(name) + 1);
    obj->type = malloc(strlen(type) + 1);
    if(!obj->name || !obj->type)
        error("could not allocate space for object \"%s\": %s", name, strerror(errno));
    strcpy((char*)obj->name, name);
    strcpy((char*)obj->type, type);
    
    // success
    return 0;
}

object* find_object(const input* inp, const char* name)
{
    // position of found object
    size_t pos;
    
    // look for name in each object
    for(pos = 0; pos < inp->nobjs; ++pos)
        if(strcmp(name, inp->objs[pos].name) == 0)
            break;
    
    // return object if found, or else NULL
    if(pos < inp->nobjs)
        return &inp->objs[pos];
    return NULL;
}

void free_object(object* obj)
{
    free((char*)obj->name);
    free((char*)obj->type);
}

const char* objects_error()
{
    return ERROR_MSG;
}
