Coordinate system
=================

When dealing with positions, lensed uses the pixel coordinate system as it is
read from the input FITS image.

A brief summary of the coordinate system:

-   The centre of each pixel is at an integer coordinate.
-   The centre of the bottom-left pixel has coordinates `(1, 1)`.
-   The left and bottom borders of the image are at `x = 0.5` and `y = 0.5`, 
    respectively. Similarly, the right and top borders of the image are at 
    `x = IMAGE_WIDTH + 0.5` and `y = IMAGE_HEIGHT + 0.5`, respectively.

These are the same coordinates that one would get, for example, by looking at
the FITS file in [SAOImage DS9].

[SAOImage DS9]: http://ds9.si.edu/site/Home.html
