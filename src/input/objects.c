#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "../input.h"
#include "objects.h"
#include "../log.h"

void add_object(input* inp, const char* name, const char* type)
{
    object* obj;
    
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
    
    // no parameters
    obj->npars = 0;
    obj->pars = NULL;
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
    for(size_t i = 0; i < obj->npars; ++i)
        free_param(&obj->pars[i]);
    free(obj->pars);
}

param* get_param(object* obj, const char* name)
{
    param* par;
    
    // position of found param
    size_t pos;
    
    // look for name in each param
    for(pos = 0; pos < obj->npars; ++pos)
        if(strcmp(name, obj->pars[pos].name) == 0)
            break;
    
    // return param if found
    if(pos < obj->npars)
        return &obj->pars[pos];
    
    // not found, make new
    obj->npars += 1;
    obj->pars = realloc(obj->pars, obj->npars*sizeof(param));
    if(!obj->pars)
        error("could not add parameter \"%s.%s\": %s", obj->name, name, strerror(errno));
    
    // realloc was successful, get new object
    par = &obj->pars[obj->npars-1];
    
    // allocate space and copy name into param
    par->name = malloc(strlen(name) + 1);
    if(!par->name)
        error("could not allocate space for parameter \"%s.%s\": %s", obj->name, name, strerror(errno));
    strcpy((char*)par->name, name);
    
    // no label is set
    par->label = NULL;
    
    // no prior is set
    par->prior = NULL;
    
    // done, return the new param
    return par;
}

void free_param(param* par)
{
    free((char*)par->name);
    free((char*)par->label);
    free((char*)par->prior);
}

void set_param_label(param* par, const char* label)
{
    // allocate space for string
    par->label = realloc((char*)par->label, strlen(label) + 1);
    if(!par->label)
        error("could not set parameter label \"%s\": %s", label, strerror(errno));
    
    // copy label to param
    strcpy((char*)par->label, label);
}

void set_param_prior(param* par, const char* prior)
{
    // allocate space for string
    par->prior = realloc((char*)par->prior, strlen(prior) + 1);
    if(!par->prior)
        error("could not set parameter prior \"%s\": %s", prior, strerror(errno));
    
    // copy prior to param
    strcpy((char*)par->prior, prior);
}
