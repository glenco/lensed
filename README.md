lensed
======

Reconstruct gravitational lenses and lensed sources from strong lensing observations.


Installation
------------

### Requirements

Lensed needs the following libraries in order to compile:

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


Running
-------

### Usage

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


### Batch mode

It is possible to run lensed in batch mode to reconstruct many images. The
relevant options are `--batch-header` and `--batch`. The former command will
generate a descriptive header file for the output of the batch mode.

    $ lensed --batch-header config.ini > results.txt

This would result in a results.txt file with the following content:

    summary                       mean                        ...
    log-ev    log-lh    chi2/n    x_L    y_L    r_L    f_L    ...

As can be seen from the header, the batch mode output contains the summary
of the reconstruction and blocks for the mean, sigma, maximum likelihood (ML)
and maximum a-posteriori (MAP) values for each parameter. The parameter labels
are taken from the configuration.

To run lensed in batch mode, use the `--batch` option in a script or loop, pass
the image (and other options) for the individual reconstruction, and append
output to the results file.

    $ for i in `seq 1 10`; do
        lensed --batch config.ini --image=image_$i.fits >> results.txt
        done

Note that the above command uses `>>` to append to the results file.


Configuration
-------------


### Options

Options are given either at the beginning of the configuration file, or in a group
called `[options]`.

The following options are known to Lensed:

```
  --gpu=<bool>      Enable computations on GPU [default: true].
  --output=<bool>   Output results [default: true].
  --root=<string>   Root element for all output paths.
  --image=<path>    Input image, FITS file in counts/sec.
  --gain=<gain>     Conversion factor to counts.
  --offset=<real>   Subtracted flat-field offset [default: 0].
  --weight=<path>   Weight map in 1/(counts/sec)^2 [default: none].
  --mask=<path>     Input mask, FITS file [default: none].
  --psf=<path>      Point-spread function, FITS file [default: none].
  --nlive=<int>     Number of live points [default: 300].
  --ins=<bool>      Use importance nested sampling [default: true].
  --mmodal=<bool>   Mode separation (if ins = false) [default: true].
  --ceff=<bool>     Constant efficiency mode [default: true].
  --acc=<real>      Target acceptance rate [default: 0.05].
  --tol=<real>      Tolerance in log-evidence [default: 0.1].
  --shf=<real>      Shrinking factor [default: 0.8].
  --maxmodes=<int>  Maximum number of expected modes [default: 100].
  --updint=<int>    Update interval for output [default: 1000].
  --seed=<int>      Random number seed for sampling [default: -1].
  --resume=<bool>   Resume from last checkpoint [default: false].
  --maxiter=<int>   Maximum number of iterations [default: 0].
```

The list of all options can be shown using `lensed --help`.


### Objects

Objects are the individual components that create the physical model for the
reconstruction. Each object corresponds to a definition in the `kernel` folder
that lists its parameters and physical properties.

The model used for reconstruction is created by listing one or more objects in
the `[objects]` section of the configuration file, together with a unique name
for identification.

For example, the configuration

```ini
[objects]
lens   = sis
source = sersic
```

describes a model that contains a SIS lens called `lens` and a Sérsic source
called `source`.

**Important:** The order of the objects in the configuration file determines the
physical layout of the system. If a source is meant to be placed before/behind a
lens, it must appear in the list of objects above/below that lens.


### Priors

Priors for parameters are specified in the `[priors]` section of a configuration
file. They are given in the format

```ini
[priors]
obj.param = <prior> <arg0> <arg1> ...
```

An exception is the pseudo-prior that fixes the value of a parameter, which is
given simply as `<value>` without any name.

The following priors are known:

-   `<value>` -- pseudo-prior that fixes the parameter to the given `<value>`
    (i.e. a delta function)
-   `unif <a> <b>` -- uniform prior on the interval [*a*, *b*]
-   `norm <m> <s>` -- normal prior with mean *m* and standard deviation *s*

Examples:

```ini
[priors]

; parameter "x" of object "lens" is fixed to be 100
lens.x = 100

; uniform probability for parameter "x" of object "lens" to be in [80, 120]
lens.y = unif 80 120
```


### Labels

It is possible to attach labels to parameters for post-processing e.g. with
getdist. These labels are given in the `[labels]` section of a configuration
file. The format is

```ini
[labels]
obj.param = <label>
```

Examples:

```ini
[labels]

; label parameter "x" of object "lens"
lens.x = x_L
```


Versions
--------

The lensed project uses a form of [semantic versioning](http://semver.org).

Given a version number MAJOR.MINOR.PATCH, lensed increments the

1.  MAJOR version for changes that render input files incompatible,
2.  MINOR version for changes that leave input files compatible but might
    lead to changes in results, and
3.  PATCH version for bugfix changes that do not alter results, unless the
    results are affected by the bugs fixed.
