#pragma once

typedef struct
{
    // profile name
    const char* name;
    
    // number of ticks between queue and submit of event
    unsigned long long queue;
    
    // number of ticks between submit and start of event
    unsigned long long submit;
    
    // number of ticks between start and end of event
    unsigned long long execute;
} profile;

// create a new profile with the given name, which is not copied
profile* profile_create(const char* name);

// delete a profile
void profile_free(profile*);

// create an event for profiling
cl_event* profile_event();

// read profile data from event, freeing the event
void profile_read(profile* prof, cl_event* event);

// print table for profile
void profile_print(int profc, profile* profv[]);
