Dependencies
============

Lensed has a number of compile-time dependencies, namely

-   [CFITSIO 3](#cfitsio) ,
-   [MultiNest >= 3.8](#multinest) ,
-   [OpenCL](#opencl) .

If these dependencies are installed in a system-wide, default-accessible path,
Lensed should be able to find them without any further intervention.

For XPA support and integration with SAOImage DS9, there is a further
dependency on

-   [XPA](#xpa) .

For region file support, there is a further dependency on the

-   [regions library](#regions) .

XPA and region file support must be enabled explicitly in the
[build options](building.md).


CFITSIO
-------

The [CFITSIO](http://heasarc.gsfc.nasa.gov/docs/software/fitsio/) library is
the standard for reading and writing FITS files.

Binary releases are statically linked against a recent version of CFITSIO and
therefore do not depend on it. When building from source, it is necessary to
have both the CFITSIO library and the header files available.


### Packages

Because CFITSIO is a mature library, pre-compiled binary versions are often
found in the repositories of package management systems. If it is possible to
install new packages into the system, this is the easiest way to get CFITSIO.


#### Ubuntu Linux

For running a binary release, it should suffice to install the CFITSIO runtime:

```sh
$ sudo apt-get install libcfitsio3
```

When building Lensed from source, the CFITSIO development package must be used:

```sh
$ sudo apt-get install libcfitsio3-dev
```


#### Mac OS X (Homebrew)

The CFITSIO library can be found in the default Homebrew repository.

```sh
$ brew install cfitsio
```


### Sources

In case it is not possible to install CFITSIO from a package, it can be built
from source. This is straightforward:

```sh
$ curl ftp://heasarc.gsfc.nasa.gov/software/fitsio/c/cfitsio3370.tar.gz | tar -xz
$ cd cfitsio
$ ./configure  # --prefix=... and other options as necessary
$ make
```

If possible, CFITSIO should be installed into the system, using

```sh
$ sudo make install
```

or similar, depending on the platform at hand.


MultiNest
---------

The [MultiNest](http://ccpforge.cse.rl.ac.uk/gf/project/multinest/) library is
a modern sampler for posterior distributions and Bayesian inference problems.
It can be downloaded from the website after (free) registration. There are two
different versions of the library available,

-   a version using the [CMake](http://www.cmake.org) build system, and
-   a standard Makefile version.

Downloading and installing the CMake version is highly recommended, as it will
automatically detect the build configuration and offers a way to install the
library into the system.

MultiNest has a number of requirements of its own, among those are

-   a modern Fortran compiler, compatible with the 2003 standard, and
-   an implementation of the LAPACK library, which might need BLAS as well.

It is assumed that these requirements are met, as they depend heavily on the
individual system. As a guideline for workstation machines running Linux, a
combination of gfortran and OpenBLAS might be a good start. On Mac OS X, the
shortest route to a working setup is installing gfortran via Homebrew, and the
system already provides LAPACK in the Accelerate framework.


### MultiNest with CMake

After downloading and extracting the `MultiNest_v3.9_CMake.tar.gz` archive,
the building and installation should be straightforward:


```sh
$ cd MultiNest_v3.9_CMake/multinest
$ cd build
$ cmake .. -G "Unix Makefiles"
$ make
```

If possible, it is a good idea to install MultiNest into the system, so that it
can be picked up easily by the build system. This can be achieved with

```sh
$ sudo make install
```

or a similar command, depending on the platform.


### MultiNest with make

If CMake is not available, it is possible to build the Makefile version of
MultiNest. After extracting the `MultiNest_v3.9.tar.gz` archive, it might be
necessary to edit the contained Makefile. The default settings are for a system
using `gfortran` and `-llapack`, respectively. Alternative settings for Intel's
`ifort` compiler are also present, but commented out. Once the Makefile is set
up correctly, the library can be built using

```sh
$ make
```

as usual.

Please note that the Makefile version of MultiNest does not provide facilities
for system-wide installation. The name of the library differs as well, using
the name `libnest3` instead of `libmultinest`. Finally, the header file for C
is not directly exposed, but contained within the folder of the C example. All
of these things have to be kept in mind when preparing Lensed for building.


OpenCL
------

Lensed was designed from the start for heterogeneous computing environments,
using [OpenCL](https://www.khronos.org/opencl/) to communicate with both CPU
and GPU devices through a unified programming platform.

A OpenCL runtime library for the compute devices must present in the system.
Such a library usually comes with the driver of an OpenCL-enabled device. When
building Lensed from source, it is further necessary to have the OpenCL headers
installed.


### Packages

#### Ubuntu Linux

For the OpenCL runtime library, it is necessary to install the device drivers
for the CPUs/GPUs present in the system. Please refer to the Ubuntu manual for
information.

The OpenCL headers necessary for compiling Lensed can be found in package
`opencl-headers`.

```sh
$ sudo apt-get install opencl-headers
```

#### Linux (general)

The device vendors usually provide Linux drivers and SDKs for their platforms.
Some useful links are

-   [Intel OpenCL Technology](https://software.intel.com/en-us/intel-opencl) ,
-   [AMD APP SDK](http://developer.amd.com/tools-and-sdks/opencl-zone/amd-accelerated-parallel-processing-app-sdk/) ,
-   [Nvidia OpenCL](https://developer.nvidia.com/opencl) .

#### Mac OS X

Mac OS X ships with the libraries and headers required to build OpenCL programs
by default.


XPA
---

For XPA support and DS9 integration, the

- [XPA library](https://github.com/ericmandel/xpa)

is required.


### Sources

The XPA library and command line tools can be installed quickly from the GitHub
repository:

```sh
$ curl -L https://github.com/ericmandel/xpa/archive/master.tar.gz | tar xz
$ cd xpa-master
$ ./configure  # --prefix=... and other options as necessary
$ make
```

For use with Lensed, it is not necessary to have X11 or Tcl integration.

```sh
$ ./configure --without-x --without-tcl  # --prefix=...
$ make
```

If possible, the XPA libraries and command line tools should be installed into
the system, using

```sh
$ sudo make install
```

or similar, depending on the platform at hand.


### Packages

#### Mac OS X (Homebrew)

Under Mac OS X, the library can be installed using Homebrew from the official 
X11 repository. Please note that this currently requires an installation of
XQuartz, even though building from source does not (see above).

```sh
$ brew install homebrew/x11/xpa
```


Regions
-------

For region file support, the

- [regions library](https://github.com/ericmandel/regions)

is required.


### Sources

The regions library and command line tools can be installed quickly from the
GitHub repository:

```sh
$ curl -L https://github.com/ericmandel/regions/archive/master.tar.gz | tar xz
$ cd regions-master
$ ./configure  # --prefix=... and other options as necessary
$ make
```

If possible, the regions library and command line tools should be installed into
the system, using

```sh
$ sudo make install
```

or similar, depending on the platform at hand.

