Things left to do
=================

The following is a list of things that we still have to implement, grouped by
the complexity of the task.


Major things
------------

These require quite a lot of thought and care, and most likely profound changes
to the way the program works. Typically, such a change will result in a new
major version.

- [ ] Multiple lenses and sources.
- [ ] Multiple lensing planes and foregrounds.


Minor things
------------

These can be implemented with a moderate effort and without changing the
internal structure of the code. These changes will typically result in a new
minor version.

- [ ] Prior definition.
- [ ] Dynamic selection of lens and source types.
- [ ] MPI integration.
- [ ] DS9 mode with direct visualisation.
- [ ] Simplify definition of kernels.
- [ ] Double precision in kernel calculations.


Patch things
------------

Small changes that can be implemented quickly and do not affect other parts of
the code. An item from this list will typically only increase the patch level
of the program.

- [ ] Make computations work on both CPU and GPU.
- [ ] Guideline for default MultiNest options.
- [x] More config options.
- [x] Update dumper to work with MultiNest > 3.6.
- [ ] Simplify internal angle representation.
- [ ] Link MultiNest feedback to verbosity level.
- [ ] Cut rectangular region from FITS file.
- [ ] Configurable build options for OpenCL.
- [ ] Do not use external library for config-file parsing.
- [ ] Dumper output.

