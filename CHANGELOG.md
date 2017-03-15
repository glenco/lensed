Changelog
=========

v1.3.1 (2017-03-15)
-------------------

  * fix OpenCL compile errors due to INFINITY macro

v1.3.0 (2017-03-14)
-------------------

  * fix OpenCL compatibility issues when building
  * new `bscale` option for config file
  * support for even PSF size
  * weight, xweight, gain options take file or number
  * Sersic approximations for full parameter range
  * fix crash when no gain option is given
  * output p-value map
  * use CFITSIO to check whether file is FITS
  * use region files for mask
  * default values of zero for sky gradient
  * fixed existing EPL tests and added isothermal test
  * write paramnames and ranges at start of reconstruction
  * 7-sigma bounds instead of INF for normal prior
  * simplified Sersic source coefficients

v1.2.0 (2016-02-12)
-------------------

  * FITS extension names and weight map output
  * check that weights are positive
  * boolean options can be set as flags
  * XPA support and basic DS9 integration

v1.1.1 (2016-01-12)
-------------------

  * bugfix: use parameter map to get correct wraparound flag
  * fixed bug when last object was a lens
  * fixed EPL scale

v1.1.0 (2015-10-10)
-------------------

  * parameters can have default values
  * pseudo-priors are derived parameters
  * quadrature rule selection
  * suppress file output from Fortran in quiet or batch mode
  * image plane priors for position
  * object data in local memory
  * show maxloglike in progress
  * support for extra weight map
  * elliptical power law profile lens
  * show progress in MultiNest feedback
  * use cl_float instead of cl_char for object memory

v1.0.2 (2015-06-26)
-------------------

  * catch keyboard interrupts and print results
  * minor output tweak for help display and device list
  * simple profiling for OpenCL functions
  * parameter bounds
  * parameter types
  * add Makefile option for debug builds
  * parameter wrap specification with prior
  * added parameter file and documentations for analysis with GetDist

v1.0.1 (2015-06-05)
-------------------

  * improved handling of binary dependencies
  * documentation for releases
  * show feedback from MultiNest

v1.0.0 (2015-05-28)
-------------------

  * changed spelling of Lensed
  * documentation for Lensed
  * platform support
  * smarter object definitions
  * MIT license added
  * fix invalid memory being freed by options
  * fix infinite deflections
  * fixed bug when no default config was present
  * compatibility with older CFITSIO
  * added OpenCL to list of requirements
  * support for default config file
  * OpenCL device selection
  * New example with simultaneous optimization of lens mass, lens light and
    background source
  * Added extras/pl_density2d.pro for simple visualisation using IDL.
  * tests for existing objects
  * Inverting the directions of the psf convolution
  * fix FWHM estimation of background counts
  * normal distribution prior
  * Individual work sizes for kernels.
  * Fix the blank log files for MultiNest.
  * Limit block size by OpenCL device capabilities.
  * Flag to list computation devices.
  * Pixel coordinate system.
  * Added Elliptical Power-Law Potential models, eplp.cl and eplp_plus_shear.cl
  * Optimised and unified object code.
  * Added sis_plus_shear and sie_plus_shear lens models. Removed shear model.
  * Define kernel constants also in object code.
  * Added some helpful links and PR info.
  * Fix line-too-long if there's no final newline in ini.
  * Fixes incorrect allocation of PSF buffer.
  * Added point mass lens.
  * Split `efr` option into separate `acc` and `shf` options.
  * Functions for and output of prior bounds.
  * New lens models: sis, nsis, nsie
  * Adding new source models.
  * Fix ambiguous `mad24` overload in kernel. Clean up Makefile.

v0.4.0 (2015-01-12)
-------------------

  * Replace PCC by GK integration rule.
  * Parameterisation of models.
  * Read per-pixel effective gain from FITS file.
  * Make sure no second lensing plane is created.

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
