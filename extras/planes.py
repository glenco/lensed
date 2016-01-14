#!/usr/bin/env python

import sys
import astropy.cosmology as co

# get redshifts from command arguments
redshifts = [float(x) for x in sys.argv[1:]]

# make a standard cosmology
cosmo = co.FlatLambdaCDM(H0=70, Om0=0.3)

# comoving angular diameter distances
D2  = 0 # = D_{n-2}
D1  = 1 # = D_{n-1}
D   = 0 # = D_n

# plane scale factors
f = []

# go through redshifts
for z in redshifts:

    # compute comoving angular diameter distance
    D2, D1, D = D1, D, (1 + z)*cosmo.angular_diameter_distance(z).value

    # append factor
    f.append((D - D1)/D*D2/(D1 - D2))

# print values
print('\t'.join(["%.4g" % x for x in f[2:]]))
