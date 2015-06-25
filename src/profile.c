#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "opencl.h"
#include "profile.h"
#include "log.h"

profile* profile_create(const char* name)
{
    profile* prof = malloc(sizeof(profile));
    if(!prof)
        errori(NULL);
    
    prof->name    = name;
    prof->queue   = 0;
    prof->submit  = 0;
    prof->execute = 0;
    
    return prof;
}

void profile_free(profile* prof)
{
    free(prof);
}

cl_event* profile_event()
{
    cl_event* e = malloc(sizeof(cl_event));
    if(!e)
        errori(NULL);
    return e;
}

void profile_read(profile* prof, cl_event* event)
{
    cl_ulong queue, submit, start, end;
    
    clGetEventProfilingInfo(*event, CL_PROFILING_COMMAND_QUEUED, sizeof(cl_ulong), &queue, NULL);
    clGetEventProfilingInfo(*event, CL_PROFILING_COMMAND_SUBMIT, sizeof(cl_ulong), &submit, NULL);
    clGetEventProfilingInfo(*event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
    clGetEventProfilingInfo(*event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);
    
    prof->queue   += submit - queue;
    prof->submit  += start - submit;
    prof->execute += end - start;
    
    clReleaseEvent(*event);
    free(event);
}

void profile_print(int profc, profile* profv[])
{
    unsigned long long total;
    
    // get total ticks across all profiles
    total = 0;
    for(int i = 0; i < profc; ++i)
        total += profv[i]->execute;
    
    // table header
    info(LOG_BOLD "  %-12s  " LOG_DARK "%10s  %10s  %10s" LOG_RESET LOG_BOLD "  %10s  %10s" LOG_RESET,
         "function", "queue", "submit", "execute", "per cent", "time");
    info("  ------------------------------------------------------------------------");
     
    // output table
    for(int i = 0; i < profc; ++i)
    {
        unsigned long long queue   = profv[i]->queue/1000000;
        unsigned long long submit  = profv[i]->submit/1000000;
        unsigned long long execute = profv[i]->execute/1000000;
        
        double percent = 100.0*profv[i]->execute/total;
        
        double time = 1e-9*profv[i]->execute;
        double sec  = fmod(time, 60);
        int    min  = floor(time/60) + 0.5;
        
        info("  %-12s  " LOG_DARK "%10llu  %10llu  %10llu" LOG_RESET "  %10.2f  %3d:%06.3f",
             profv[i]->name, queue, submit, execute, percent, min, sec);
    }
}
