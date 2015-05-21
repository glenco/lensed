Troubleshooting
===============

Contents:

[TOC]

Problems running Lensed
-----------------------

### The program complains that shared libraries cannot be loaded.

*Note: When building Lensed, this can be fixed at the compiler level, see
[here](#the-linker-complains-that-shared-libraries-cannot-be-loaded).*

This error occurs when the dynamic library loader cannot find the libraries
which where linked into Lensed at compile time. This is usually the case when
a pre-compiled binary is used without all [dependencies](dependencies/) present
and resolvable in the system, or when some of the libraries were moved after
the code has been compiled.

To diagnose which libraries cannot be found, use (on Linux)

```sh
$ ldd bin/lensed
	libcfitsio.so.2 => /usr/local/lib/libcfitsio.so.2
	libmultinest.so.3.8 => /usr/local/lib/libmultinest.so.3.8
	libOpenCL.so.1 => /usr/lib64/libOpenCL.so.1
	libm.so.6 => /lib64/libm.so.6
	libc.so.6 => /lib64/libc.so.6
	/lib64/ld-linux-x86-64.so.2
```

or (on Mac OS X)

```sh
$ otool -L bin/lensed
bin/lensed:
	/usr/local/lib/libcfitsio.2.dylib
	/usr/local/lib/libmultinest.3.8.dylib
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

Please see the [section on configuring](building/#configuration) the Lensed
build system.


### The linker complains that shared libraries cannot be loaded.

*Note: This is different from required libraries that cannot be found.*

This issue usually occurs when some of the successfully linked shared libraries
depend dynamically on further shared libraries which cannot be resolved.

After locating the unresolved files, e.g. `/opt/lib/libLibrary.so`, the search
path for dynamic linking can be augmented as follows:

```sh
$ make EXTRA_LIBS="-Wl,-rpath,/opt/lib" # plus eventual other EXTRA_LIBS flags
```

This instructs the linker to look into `/opt/lib` when trying to resolve shared
libraries.
