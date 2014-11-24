#define OBJECT_SIZE 64

typedef struct __attribute__ ((packed)) { char _[OBJECT_SIZE]; } object;

// object types
enum
{
    LENS   = 'L',
    SOURCE = 'S'
};

// macro to specify type of object
#define OBJECT(x) constant int object_##x
