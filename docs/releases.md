Get Lensed
==========

The easiest way to get started with Lensed is by downloading a release version
from the [releases page](https://github.com/glenco/lensed/releases), where you
can find current and previous iterations of the code that have been assigned a
fixed version number. New versions are released often and usually do not miss
features from the development branch.

Releases are the recommended way to run Lensed for production use. People who
intend to contribute to the development of Lensed should refer to the relevant
section on [building Lensed from source](building.md).

Binary distributions are available for Linux and Mac OS X. When using a binary,
it is still possible to extend Lensed with new lens and source objects, as this
does not require recompiling the code.


Dependencies
------------

Binary releases of Lensed have the following dependencies:

-   [MultiNest](dependencies.md#multinest)
-   [OpenCL drivers](dependencies.md#opencl)

Source code releases have the following additional dependencies:

-   [CFITSIO](dependencies.md#cfitsio)
-   OpenCL header files

Detailed descriptions of the requirements can be found in the relevant sections
of the [dependencies page](dependencies.md). A number of common issues related
to dependencies are resolved on the [troubleshooting page](troubleshooting.md).


Installation
------------

Lensed is self-contained in its directory. The layout of the distribution is
important, as Lensed's binary located at `bin/lensed` will look for files in
the `kernel` and `objects` folders relative to its own location. Therefore, the
Lensed directory should be moved in its entirety, and individual files should
not be redistributed into various folders such as `/usr/local/bin/` or similar.

A possibility for a system-wide installation is to move the entire directory of
Lensed to a folder such as `/usr/local/lensed` or `/opt/lensed`, and create a
symbolic link to the binary somewhere on the PATH, e.g. `/usr/local/bin/lensed`
or `/opt/bin/lensed`, as this does not interfere with the internal lookup.


Source releases
---------------

The source code for release versions is available together with the binaries.
On "special needs" systems such as clusters, it is often necessary to compile
Lensed for the particular requirements at hand. This is detailed on a [separate
page](building.md).
