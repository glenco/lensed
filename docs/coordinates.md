Coordinate system
=================

When dealing with positions, Lensed uses the pixel coordinate system as read
from the input FITS image.

By default, the coordinate system is positioned and oriented as follows.

-   The centre of each pixel is at an integer coordinate.
-   The centre of the bottom-left pixel has coordinates `(1, 1)`.
-   The left and bottom borders of the image are at `x = 0.5` and `y = 0.5`, 
    respectively. Similarly, the right and top borders of the image are at 
    `x = IMAGE_WIDTH + 0.5` and `y = IMAGE_HEIGHT + 0.5`, respectively.

These are the same coordinates that one would get, for example, by looking at
the FITS file in [SAOImage DS9](http://ds9.si.edu/).

In case the FITS file was cropped using the extended filename syntax (e.g.
`image = observation.fits[400:500,600:700]`), the coordinate system is adapted
accordingly.
