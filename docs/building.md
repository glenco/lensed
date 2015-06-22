Building from source
====================

Lensed is a standard Makefile project. On a system where all dependencies can
be resolved automatically, it should be enough to call

```sh
$ make
```

from the root folder of Lensed to build the program. If this fails, further
[configuration of the build system](#build-configuration) might be necessary.

After compilation has finished, a quick check of

```sh
$ bin/lensed --version
lensed X.Y.Z
```

should display the correct version number of Lensed.


Getting the sources
-------------------

### Releases

Sources are available for each release of Lensed. These can be downloaded from
the [releases page](https://github.com/glenco/lensed/releases). For details on
releases, see the [corresponding page](releases.md).


### Using Git

Lensed is developed using the [Git version control system](https://git-scm.com)
and [hosted on GitHub](https://github.com/glenco/lensed). Use Git to get the
most recent development version of the code, especially if you plan on taking
part in extending the code or fixing bugs.

With Git installed on your system, use

```sh
$ git clone https://github.com/glenco/lensed.git
```

to clone the most recent version of the code into a subfolder of the current
directory.

There is a lot of information available about Git. For a first introduction,
please [refer to the documentation](https://git-scm.com/doc), especially the
first two chapters on [getting started](https://git-scm.com/book/en/v2/Getting-Started-About-Version-Control)
and the [Git basics](https://git-scm.com/book/en/v2/Git-Basics-Getting-a-Git-Repository).
A quick introduction can also be found in the [Git tutorial](http://git-scm.com/docs/gittutorial).


### From an archive

On systems where no Git client is available, it is alternatively possible to
get the latest development version of the sources in the form of an archive.
A link is provided at the [project page](https://github.com/glenco/lensed).

When working in a terminal environment, it is possible to download and extract
Lensed using a combination of cURL and tar with the following command:

```sh
$ curl -L https://github.com/glenco/lensed/archive/master.tar.gz | tar -xz
```

Alternatively, if `curl` is not available, you can try to use `wget` instead:

```sh
$ wget https://github.com/glenco/lensed/archive/master.tar.gz
$ tar -xzf master.tar.gz
```

Finally, if `tar` is not available you can replace the `.tar.gz` extension by
`.zip`, and try the `unzip` command.

Please note that using this method to get Lensed, it will not be possible to
commit your changes or bug fixes back into the code base. Use of this method is
therefore discouraged for normal development work, and only intended for the
distribution of the code to "difficult" environments.


Dependencies
------------

Building from source requires a number of dependencies to be met, namely

-   MultiNest >= 3.8 ,
-   CFITSIO 3 ,
-   OpenCL headers and runtime library .

If these dependencies are installed in a system-wide, default-accessible path,
Lensed should be able to find them without any further intervention. For more
information, refer to the [dependencies page](dependencies.md).


Build configuration
-------------------

There are a number of configuration variables that can be passed to Lensed's
build system. They are given in the form of `NAME="value"` pairs to `make`,
e.g.

```sh
$ make EXTRA_LIBS="-L/opt/OpenBLAS/lib -lopenblas"
```

The following variables can be passed to Lensed:

| Name                    | Description                                       |
|-------------------------|---------------------------------------------------|
| `CFITSIO_DIR`           | path to a local CFITSIO build                     |
| `CFITSIO_INCLUDE_DIR`   | path to `fitsio.h`                                |
| `CFITSIO_LIB_DIR`       | path to `libcfitsio`                              |
| `CFITSIO_LIB`           | CFITSIO library (e.g. `-lcfitsio`)                |
| `MULTINEST_DIR`         | path to a local MultiNest CMake build             |
| `MULTINEST_INCLUDE_DIR` | path to `multinest.h`                             |
| `MULTINEST_LIB_DIR`     | path to the MultiNest library                     |
| `MULTINEST_LIB`         | MultiNest library (e.g. `-lmultinest`, `-lnest3`) |
| `OPENCL_DIR`            | path to the OpenCL implementation                 |
| `OPENCL_INCLUDE_DIR`    | path to the `CL/cl.h` header                      |
| `OPENCL_LIB_DIR`        | path to the OpenCL library                        |
| `OPENCL_LIB`            | OpenCL runtime library (e.g. `-lOpenCL`)          |
| `EXTRA_LIBS`            | additional libraries and linker flags             |
| `DEBUG`                 | build with debug symbols and no optimisation      |

The values of these variables are cached in a file `build/cache.mk` and do not
have to be repeated on subsequent calls to make. The contents of the cache can
be shown using `make show-cache`:

```sh
$ make show-cache
# cached settings for make
CFITSIO_INCLUDE_DIR = 
CFITSIO_LIB_DIR = 
CFITSIO_LIB = -lcfitsio
MULTINEST_INCLUDE_DIR = 
MULTINEST_LIB_DIR = 
MULTINEST_LIB = -lmultinest
OPENCL_INCLUDE_DIR = 
OPENCL_LIB_DIR = 
OPENCL_LIB = -framework OpenCL
EXTRA_LIBS = 
DEBUG = 
```

Please note that the `XYZ_INCLUDE_DIR` and `XYZ_LIB_DIR` variables override the
value of the corresponding `XYZ_DIR` variable, so either the former two or the
latter should be set.

The debug build can be enabled by defining the `DEBUG` symbol, for example by
calling `make DEBUG=1`. To disable the debug build, the `DEBUG` symbol has to
be undefined by calling `make DEBUG=` (i.e. with no value). Debug symbols and
optimisations are set for each individual object file at compile time, hence
it is necessary to perform a `make clean` in order to fully apply the setting.

The following sections contain further details on configuring the individual
components of Lensed.


### CFITSIO

If CFITSIO has been built from source, the path to the source folder can be
given to Lensed using the `CFITSIO_DIR` variable.

```sh
$ make CFITSIO_DIR="$HOME/cfitsio"
```

This sets both `CFITSIO_INCLUDE_DIR` and `CFITSIO_LIB_DIR` to `CFITSIO_DIR`, as
this is where `fitsio.h` and `libcfitsio` reside by default.

Alternatively, `CFITSIO_INCLUDE_DIR` and `CFITSIO_LIB_DIR` can be explicitly
specified.

```sh
$ make CFITSIO_INCLUDE_DIR="$HOME/headers" CFITSIO_LIB_DIR="$HOME/libraries"
```

The default CFITSIO library be linked is `-lcfitsio`. This can be overridden
using the `CFITSIO_LIB` flag, either giving `-l<name>` linker flag or the full
path to the library.

```sh
$ make CFITSIO_LIB="$HOME/libraries/my-cfitsio-lib.a"
```


### MultiNest

In case MultiNest was built using CMake, it is usually enough to pass the path
to the source folder to make using the `MULTINEST_DIR` variable.

```sh
$ make MULTINEST_DIR="$HOME/multinest"
```

This sets the variables `MULTINEST_INCLUDE_DIR` to `$MULTINEST_DIR/include` and
`MULTINEST_LIB_DIR` to `$MULTINEST_DIR/lib`, as these are the default paths to
`multinest.h` and `libmultinest`, respectively.

Alternatively, the variables `MULTINEST_INCLUDE_DIR` and `MULTINEST_LIB_DIR`
can be set explicitly to the paths containing the required files.

```sh
$ make MULTINEST_INCLUDE_DIR="$HOME/headers" MULTINEST_LIB_DIR="$HOME/libraries"
```

The name of the MultiNest library that will be linked is `MULTINEST_LIB`, which
defaults to `-lmultinest` as generated by the CMake version. This can be set to
a linker flag `-l<name>` or a full library path.

```sh
$ make MULTINEST_LIB="$HOME/multinest/my-multinest-lib.a"
```

**For the Makefile version** of MultiNest, the correct settings are

```sh
$ make MULTINEST_INCLUDE_DIR="/path/to/multinest/example_eggbox_C" \
       MULTINEST_LIB_DIR="/path/to/multinest" \
       MULTINEST_LIB="-lnest3"
```


### OpenCL

*
**Mac OS X**: Please note that Mac OS X comes with a framework containing the
OpenCL headers and library. It is not necessary to change the OpenCL settings.
*

In order to build Lensed, the compiler needs the OpenCL header `CL/cl.h` and
the OpenCL runtime library.

If these are provided by a vendor SDK, it can be enough to set the `OPENCL_DIR`
variable to the root of the SDK:

```sh
$ make OPENCL_DIR="/usr/local/cuda"
```

This sets the variables `OPENCL_INCLUDE_DIR` and `OPENCL_LIB_DIR` to default
values of `$OPENCL_DIR/include` and `$OPENCL_DIR/lib`, respectively.

Alternatively, it is possible to set `OPENCL_INCLUDE_DIR` and `OPENCL_LIB_DIR`
manually to the paths containing `CL/cl.h` (note the subfolder) and the OpenCL
library (usually `libOpenCL`), respectively.

```sh
$ make OPENCL_INCLUDE_DIR="$HOME/headers" OPENCL_LIB_DIR="$HOME/libraries"
```

The name of the OpenCL runtime library can be set by the `OPENCL_LIB` variable.
This can be a linker flag such as `-lOpenCL` or an explicit path to a library.

```sh
$ make OPENCL_LIB="$HOME/experimental-driver/libOpenCL.so"
```
