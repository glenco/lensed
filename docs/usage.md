Usage
=====

The usual invocation of Lensed is with a [configuration](configuration.md) file
which contains program options and the model that is being reconstructed. The
reconstruction is started using

```sh
$ lensed config.ini
```

where `config.ini` is the name of the configuration file.

It is possible to split the configuration into different files, for example to
modularise options and individual aspects of the model. The following command
would run Lensed with the configuration taken from three different files:

```sh
$ lensed options.ini lens.ini sources.ini
```

Repeated options are overwritten by the later configuration files.

Finally, it is possible to give any of the options (but not the model) directly
on the command line. For example, in order to change the number of live points
for a quick look at results, one could invoke Lensed in the following way:

```sh
$ lensed config.ini --nlive=50
```

As before, the order of the given configuration files and options is important.
