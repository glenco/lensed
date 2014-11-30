#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../src/version.h"

enum { MAJOR, MINOR, PATCH };

#define NL "\n"

void usage()
{
    fprintf(stderr, "usage: release (major | minor | patch)" NL);
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[])
{
    int rel = 0;
    
    // check that type of release was given
    if(argc < 2)
        usage();
    
    // get type of release
    if(strcmp(argv[1], "major") == 0)
        rel = MAJOR;
    else if(strcmp(argv[1], "minor") == 0)
        rel = MINOR;
    else if(strcmp(argv[1], "patch") == 0)
        rel = PATCH;
    else
        usage();
    
    // print header
    printf("#pragma once" NL);
    printf(NL);
    printf("#define VERSION_MAJOR %d" NL, rel == MAJOR ? VERSION_MAJOR + 1 : VERSION_MAJOR);
    printf("#define VERSION_MINOR %d" NL, rel < MINOR ? 0 : (rel == MINOR ? VERSION_MINOR + 1 : VERSION_MINOR));
    printf("#define VERSION_PATCH %d" NL, rel < PATCH ? 0 : VERSION_PATCH + 1);
    
    return EXIT_SUCCESS;
}
