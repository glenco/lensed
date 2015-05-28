Creating objects
================

From the start, Lensed was intended to be easily extended with custom objects.
This allows for the targeted reconstruction of individual observations with
made-to-measure objects.

New objects for lenses and sources can be added to Lensed by simply creating a
new file with the model name and extension `.cl` in the `objects` folder. There
is no need to recompile Lensed, and the new objects are available immediately.

The file has to contain a number of required elements. Consider the following
sample `objects/sis.cl` that implements a singular isothermal sphere lens.

```c
// file: objects/sis.cl

type = LENS;

params
{
    { "x" }, // x position
    { "y" }, // y position
    { "r" }  // Einstein radius
};

data
{
    float2 x; // lens position
    float r;  // Einstein radius
};

static float2 deflection(constant data* this, float2 x)
{
    return this->r*normalize(x - this->x);
}

static void set(global data* this, float x, float y, float r)
{
    // lens position
    this->x = (float2)(x, y);
    
    // Einstein radius
    this->r = r;
}

```

These four blocks are the fundamental components for a lens. A source is very
similar, except that it must contain a surface brightness function

```c
static float brightness(constant data* this, float2 x)
{
    return ...;
}
```

instead of the `deflection` function required for a lens.


Object type
-----------

```c
// the type of object: LENS, SOURCE
type = LENS;
```

The first definition for a new object should be its type. There are two basic
choices, `LENS` and `SOURCE`, that determine the role of the object. A `LENS`
must contain a `deflection` function, and a source must contain a `brightness`
function (see below).


Parameters
----------

```c
// the object parameters
params
{
    { "x" }, // x position
    { "y" }, // y position
    { "r" }  // Einstein radius
};
```


Object data
-----------

```c
// the object data
// the same rules as for a regular struct apply
data
{
    float2 x; // lens position
    float r;  // Einstein radius
};
```


Deflection (lens)
-----------------

```c
// a deflection function for a lens
// the first argument is the object data
// the second argument is the image plane position
// return value is the source plane position
static float2 deflection(constant data* this, float2 x)
{
    // deflection for SIS
    return this->r*normalize(x - this->x);   
};
```


Brightness (source)
-------------------

```c
// surface brightness function for a source
// the first argument is the object data
// the second argument is the source plane position
// return value is the surface brightness
static float brightness(constant data* this, float2 x)
{
    // Gaussian profile for centered and rotated coordinate system
    float2 y = mv22(this->t, x - this->x);
    return this->norm*exp(-0.5f*dot(y, y)/this->s2);
}
```


Parameter setter
----------------

```c
// update object data for new parameters
// first argument is the object data
// further arguments are float values of the parameters
// the order is the same as in the definition
static void set(global data* this, float x, float y, float r)
{
    // lens position
    this->x = (float2)(x, y);
    
    // Einstein radius
    this->r = r;
};
```
