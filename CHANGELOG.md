Changelog
=========

v0.3.0 (2014-12-10)
-------------------

  * Output of residuals and relative errors.
  * Convolution with PSF.
  * Separate kernels for rendering and likelihood.
  * Automatic freeing of options.
  * Paths are read relative to input files.
  * Better log file handling.
  * Warn if background is zero but no offset was given.
  * Warn user if a sky should be used.
  * Foreground objects, sky.
  * Delta function prior.
  * Disentangled options and parser.
  * Output numerical constants.
  * Simplified data handling with optional weight map.
  * Linear kernel execution.
  * Output kernel device information.
  * Output kernel notifications to stderr.
  * Kernel works in two dimensions.
  * Added info on how to contribute.
  * Example added.

v0.2.0 (2014-11-30)
-------------------

  * Some fixes for Makefile when releasing versions.
  * Release tool for updating version number.
  * Don't wait for user interaction in batch mode. Closes #51.
  * Fixed memory leak when redefining a prior.
  * Batch output mode.
  * Parameters have `id` field in the form "x.y".
  * Moved internal `errno`-related errors to `errori` function.
  * Write a getdist-compatible `<root>.paramnames` file.
  * Dumper outputs images and saves results.
  * Organised parameter space creation.
  * Options are required depending on other options.
  * Correct sign for Sérsic magnitude.
  * Parse numerical constants such as `pi`.
  * Various fixes, mostly memory leaks.
  * Priors added.
  * Input parsing factored into its own module.
  * GPU option added.
  * Minor output fixes.
  * More simplifications of kernel code for objects.
  * Output generated kernel code.
  * Simpler check for duplicate objects in kernel creation.
  * Dynamic size of object data.
  * Dynamically generate kernel code to set parameters.
  * Dynamically generate kernel code to compute surface brightness.
  * Get object meta-data from kernels.
  * Simplified object definition in kernel code.
  * Read parameter definitions (priors and labels) from ini file.
  * Read list of objects from ini file.
  * Fancy status output while doing make.
  * Organised code and build artefacts into folders.
  * Improved ini file reading with group parsing.
  * Refactored input handling with custom ini file parser.
  * Add .PHONY targets to Makefile.
  * Output changes, program logo.
  * To do list migrated to github issues.
  * Changed MultiNest default parameters.
  * Optimised GPU performance.
  * Measure run time.
  * Notifications from OpenCL already include build log.
  * Generic OpenCL header inclusion.
  * Add notification callback to OpenCL context.
  * Rename MultiNest's tolerance parameter to `tol`.
  * Ignore the generated `config.h` file.
  * Renamed `config` to `options`. Create `config.h` file that contains
    compile-time settings.
  * Working OpenCL version.
  * All program data contained in `lensed` struct. MultiNest options added.
    Default values of optional options shown. Not casting `void*`.
  * Versioning.
  * Working reconstruction.
  * First version.
