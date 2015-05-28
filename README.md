Lensed
======

Reconstruct gravitational lenses and lensed sources from strong lensing
observations.

Information regarding Lensed can be found on the [website] and [project] on
GitHub. The main [documentation] contains detailed instructions on how to set
up, run, configure and extend the code.

This project aims to be a community effort, and we are happy for any and all
contributions. The easist part to play is to open [issues] whenever something
does not work, and create [pull requests] whenever you have fixed something
that was broken. Contributions in the form of new models are most welcome if
they are generally useful.


Setup
-----

There are a number of dependencies for Lensed (see docs/dependencies.md). Once
they are satisfied and installed into the system, it should be possible to run
or compile Lensed.

If you are building Lensed from source, is typically enough to run

    $ make

in the root of the source directory, and the code should start compiling.


Usage
-----

The most common way to invoke Lensed is to create a configuration file (see
docs/configuration.md) which contains both the program options and the model
that is being reconstructed. In this case, the reconstruction is started using

    $ lensed config.ini

where `config.ini` is the name of the parameter file.

It is possible to split the configuration into different configuration files,
in order to e.g. modularise options and individual aspects of the model. For
example, the following command would run Lensed with the configuration taken
from three different files:

    $ lensed options.ini lens.ini sources.ini

Repeated options are overwritten by the later configuration files.

Finally, it is possible to give all of the options of a configuration file
directly on the command line. For example, in order to change the number of
live points for a quick look at results, one could invoke Lensed in the
following way:

    $ lensed config.ini --nlive=50

As before, the order of the given configuration files and options is important.

For more information on how to run Lensed, see the [documentation].


Versions
--------

The Lensed project uses a form of [semantic versioning](http://semver.org).

Given a version number MAJOR.MINOR.PATCH, Lensed increments the

1.  MAJOR version for changes that render input files incompatible,
2.  MINOR version for changes that leave input files compatible but might
    lead to changes in results, and
3.  PATCH version for bugfix changes that do not alter results, unless the
    results are affected by the bugs fixed.

By comparing the output of `lensed --version` before installing a different
version of the code, the user can anticipate whether updates could possibly
break existing models or alter the results of a reconstruction.


[website]: http://glenco.github.io/lensed/
[project]: https://github.com/glenco/lensed
[documentation]: http://lensed.readthedocs.org
[issues]: https://github.com/glenco/lensed/issues
[pull requests]: https://github.com/glenco/lensed/pulls
