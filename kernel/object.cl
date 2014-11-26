// object types
enum
{
    LENS   = 'L',
    SOURCE = 'S'
};

// macro to specify type of object
#define OBJECT(x) constant int object_##x
