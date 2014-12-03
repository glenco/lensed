lensed
======

A tool to reconstruct lenses and sources from strong lensing observations.


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
