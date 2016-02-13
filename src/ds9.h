#pragma once

// connect to a DS9 instance via XPA
void* ds9_connect(char* tmpl);

// disconnect from a DS9 instance
void ds9_disconnect(void* ds9);

// get template name from DS9 connection
const char* ds9_template(void* ds9);

// get frame number from DS9 connection
int ds9_frame(void* ds9);

// show FITS cube in DS9
void ds9_mecube(void* ds9, void* fits, size_t len);
