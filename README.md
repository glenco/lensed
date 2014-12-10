lensed
======

Reconstruct gravitational lenses and lensed sources from strong lensing observations.


Configuration
-------------

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
-   `unif <a> <b>` -- uniform prior on the interval [a, b]

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


Batch mode
----------

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


Versions
--------

The lensed project uses a form of [semantic versioning](http://semver.org).

Given a version number MAJOR.MINOR.PATCH, lensed increments the

1.  MAJOR version for changes that render input files incompatible,
2.  MINOR version for changes that leave input files compatible but might
    lead to changes in results, and
3.  PATCH version for bugfix changes that do not alter results, unless the
    results are affected by the bugs fixed.
