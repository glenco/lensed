Troubleshooting
===============

[TOC]

Problems running Lensed
-----------------------

### The program cannot find the MultiNest library.

When MultiNest was not installed to a default system path such as `/usr/lib`,
it is possible that the linker cannot find the shared library `libmultinest.so`
(Linux) or `libmultinest.dylib` (Mac OS X).

The easiest fix is to link the shared library into the `bin/` folder of Lensed,
side by side with the program executable.

```sh
$ # in Lensed's root directory
$ cd bin
$ ls
lensed
$ ln -s /path/to/libmultinest.so # .dylib on Mac OS X
```

After this, the program should be able to load the shared library.


### The program crashes when loading the MultiNest library.

This error usually occurs when the program tries to load a version of MultiNest
that is not binary-compatible with the one Lensed was linked against.

For binary releases, please take a careful look at the version requirements for
the MultiNest library. For maintainability, releases are not linked against a
particular shared library version of MultiNest (i.e. `libmultinest.so.3.9` on
Linux or `libmultinest.3.9.dylib` on Mac OS X), but rather against the generic
`libmultinest.so` or `libmultinest.dylib` library. This is because MultiNest's
shared library version increases by default with every new version, even though
the libraries might be binary-compatible. To prevent either having to link the
binaries against many versions of MultiNest, or forcing users to recompile the
MultiNest library even though it is binary-compatible, the program links with
the version-less shared library name.


### The program complains that other shared libraries cannot be loaded.

*
Note: When building Lensed, this can be fixed at the compiler level, see
[here](#the-linker-complains-that-shared-libraries-cannot-be-loaded).
*

This error occurs when the dynamic library loader cannot find shared libraries
on which parts of Lensed (for example MultiNest) depends. These libraries are
usually the Fortran runtime, LAPACK or BLAS, or system libraries.

To diagnose which libraries cannot be found, use (on Linux)

```sh
$ ldd bin/lensed
	libmultinest.so => /usr/local/lib/libmultinest.so
	libOpenCL.so.1 => /usr/lib/fglrx/libOpenCL.so.1
	libz.so.1 => /lib/x86_64-linux-gnu/libz.so.1
	libm.so.6 => /lib/x86_64-linux-gnu/libm.so.6
	libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6
	libgfortran.so.3 => /usr/lib/x86_64-linux-gnu/libgfortran.so.3
	libdl.so.2 => /lib/x86_64-linux-gnu/libdl.so.2
	/lib64/ld-linux-x86-64.so.2
```

or (on Mac OS X)

```sh
$ otool -L bin/lensed 
bin/lensed:
	@rpath/libmultinest.dylib
	/System/Library/Frameworks/OpenCL.framework/Versions/A/OpenCL
	/usr/lib/libSystem.B.dylib
```

If any libraries result as not found, it is necessary to make their location
explicitly known. This can be done using the respective environment variables
`LD_LIBRARY_PATH` (on Linux) or `DYLD_LIBRARY_PATH` (on Mac OS X).

For example, in order to make known the location of a shared library located at
`$HOME/lib`, one could call Lensed as (Linux)

```sh
$ LD_LIBRARY_PATH=$HOME/lib:$LD_LIBRARY_PATH bin/lensed
```

or (Mac OS X)

```sh
$ DYLD_LIBRARY_PATH=$HOME/lib:$DYLD_LIBRARY_PATH bin/lensed
```

After finding the correct setting, it is possible to create a small script that
automatically calls Lensed with this environment variable set. The following
snippet should be saved as `bin/lensed.sh`:

```sh
#!/bin/bash

export LD_LIBRARY_PATH=... # your settings here

$(dirname $0)/lensed $@
```

Make sure to edit the line containing `LD_LIBRARY_PATH` appropriately for your
system, changing the name to `DYLD_LIBRARY_PATH` on Mac OS X. Then, make the
script executable using

```sh
$ chmod +x bin/lensed.sh
```

and call the wrapper script `bin/lensed.sh` whenever you would normally call
Lensed directly.


Problems building Lensed
------------------------

### Some of Lensed's dependencies cannot be resolved.

Please see the [section on configuring](building/#build-configuration) Lensed's
build system.


### The linker complains that shared libraries cannot be loaded.

*
Note: This is different from required libraries that cannot be found.
*

This issue usually occurs when some of the successfully linked shared libraries
depend dynamically on further shared libraries which cannot be resolved.

After locating the unresolved files, e.g. `/opt/lib/libLibrary.so`, the search
path for dynamic linking can be augmented as follows:

```sh
$ make EXTRA_LIBS="-Wl,-rpath,/opt/lib" # plus eventual other EXTRA_LIBS flags
```

This instructs the linker to look into `/opt/lib` when trying to resolve shared
libraries.
