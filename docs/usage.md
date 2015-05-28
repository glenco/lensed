Usage
=====

The most common way to invoke Lensed is to create a configuration file (see
docs/configuration.md) which contains both the program options and the model
that is being reconstructed. In this case, the reconstruction is started using

```sh
$ lensed config.ini
```

where `config.ini` is the name of the parameter file.

It is possible to split the configuration into different configuration files,
in order to e.g. modularise options and individual aspects of the model. For
example, the following command would run Lensed with the configuration taken
from three different files:

```sh
$ lensed options.ini lens.ini sources.ini
```

Repeated options are overwritten by the later configuration files.

Finally, it is possible to give all of the options of a configuration file
directly on the command line. For example, in order to change the number of
live points for a quick look at results, one could invoke Lensed in the
following way:

```sh
$ lensed config.ini --nlive=50
```

As before, the order of the given configuration files and options is important.
