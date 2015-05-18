Lensed
======

Reconstruct gravitational lenses and lensed sources from strong lensing observations.


Installation
------------

### Requirements

Lensed needs the following libraries in order to compile:

-   OpenCL 1.2
-   [CFITSIO] 3.370
-   [MultiNest] 3.9

Please make sure that the libraries are accessible for the compiler.

On Mac OS X, the libraries can be installed quickly using [homebrew]. While CFITSIO can
be found in the main repository, you will need to run

    $ brew tap ntessore/nt

before you can install MultiNest in this way.

[CFITSIO]: http://heasarc.gsfc.nasa.gov/docs/software/fitsio/
[MultiNest]: http://ccpforge.cse.rl.ac.uk/gf/project/multinest/
[homebrew]: http://brew.sh/


### Compiling

Lensed is a Makefile project. This means that it is typically enough to run

    $ make

in the root of the source directory, and the code should start compiling.

If you need to run with a different compiler or using special flags for compilation or
linking, you can pass these to `make` in the usual way.


Usage
-----

The most common way to invoke Lensed is to create a configuration file (see below)
which contains both the program options and the model that is being reconstructed.
In this case, the reconstruction is started using

    $ lensed config.ini

where `config.ini` is the name of the parameter file.

It is possible to split the configuration into different configuration files, in order
to e.g. modularise options and individual aspects of the model. For example, the
following command would run Lensed with the configuration taken from three different
files:

    $ lensed options.ini lens.ini sources.ini

Repeated options are overwritten by the later configuration files.

Finally, it is possible to give all of the options of a configuration file directly on
the command line. For example, in order to change the number of live points for a quick
look at results, one could invoke Lensed in the following way:

    $ lensed config.ini --nlive=50

As before, the order of the given configuration files and options is important.

For more information on how to run Lensed, see the [wiki].


Documentation
-------------

The central hub for information regarding Lensed are the [website] and [project]
on GitHub. There you will find a [wiki] that contains information on how to run,
configure, optimise, and extend Lensed.

This project is a community effort, and we are happy for any and all contributions.
The easist part to play is to open [issues] whenever something does not work, and
create [pull requests] whenever you have fixed something that was broken.
Contributions in the form of new models are most welcome if they are generally useful.

[website]: http://glenco.github.io/lensed/
[project]: https://github.com/glenco/lensed/
[wiki]: https://github.com/glenco/lensed/wiki
[issues]: https://github.com/glenco/lensed/issues
[pull requests]: https://github.com/glenco/lensed/pulls


Versions
--------

The lensed project uses a form of [semantic versioning](http://semver.org).

Given a version number MAJOR.MINOR.PATCH, lensed increments the

1.  MAJOR version for changes that render input files incompatible,
2.  MINOR version for changes that leave input files compatible but might
    lead to changes in results, and
3.  PATCH version for bugfix changes that do not alter results, unless the
    results are affected by the bugs fixed.
