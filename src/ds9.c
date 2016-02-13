// provide POSIX standard in strict C99 mode
#define _XOPEN_SOURCE 600

#include <stdlib.h>

#ifdef LENSED_XPA

#include <sys/select.h>
#include "xpa.h"

#include "ds9.h"
#include "log.h"

struct ds9_conn
{
    void* handle;
    char* tmpl;
    int frame;
};

void* ds9_connect(char* tmpl)
{
    // XPA connection handle
    void* handle;
    
    // XPA response
    char* buf = NULL;
    size_t len = 0;
    
    // create persistent XPA handle
    handle = XPAOpen(NULL);
    if(!handle)
        return NULL;
    
    // new DS9 connection object
    struct ds9_conn* ds9 = malloc(sizeof(struct ds9_conn));
    if(!ds9)
        errori(NULL);
    
    // store handle and template
    ds9->handle = handle;
    ds9->tmpl = tmpl;
    
    // create frame for images
    XPASet(handle, tmpl, "frame new", NULL, NULL, 0, NULL, NULL, 1);
    
    // get newly created frame
    XPAGet(handle, tmpl, "frame", NULL, &buf, &len, NULL, NULL, 1);
    if(buf)
        ds9->frame = atoi(buf);
    else
        ds9->frame = 999;
    
    // free response buffer
    free(buf);
    
    // return DS9 connection
    return ds9;
}

void ds9_disconnect(void* p)
{
    struct ds9_conn* ds9 = p;
    XPAClose(ds9->handle);
    free(ds9);
}

const char* ds9_template(void* p)
{
    struct ds9_conn* ds9 = p;
    return ds9->tmpl;
}

int ds9_frame(void* p)
{
    struct ds9_conn* ds9 = p;
    return ds9->frame;
}

void ds9_mecube(void* p, void* fits, size_t len)
{
    struct ds9_conn* ds9 = p;
    
    // buffer for frame number
    char frame[64];
    
    // the XPA mode
    char* mode = "ack=false,verify=false,doxpa=false";
    
    // create DS9 frame paramlist
    snprintf(frame, 64, "frame %d", ds9->frame);
    
    // set the image array
    XPASet(ds9->handle, ds9->tmpl, frame, mode, NULL, 0, NULL, NULL, 1);
    XPASet(ds9->handle, ds9->tmpl, "mecube", mode, fits, len, NULL, NULL, 1);
}

#else

void* ds9_connect(char* tmpl)
{
    return NULL;
}

void ds9_disconnect(void* p)
{
}

const char* ds9_template(void* p)
{
    return NULL;
}

int ds9_frame(void* p)
{
    return 0;
}

void ds9_mecube(void* p, void* fits, size_t len)
{
}

#endif
