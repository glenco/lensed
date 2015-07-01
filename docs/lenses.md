Lenses
======

SIS
---

The `sis` lens is a singular isothermal sphere with deflection[^1]
\\[
    \alpha_x = r_E \, \frac{x}{r} \;,
\\]
\\[
    \alpha_y = r_E \, \frac{y}{r} \;,
\\]
where $r_E$ is the Einstein radius, and $r$ is the distance to the position of
the lens.


SIE
---

The `sie` lens is a singular isothermal ellipsoid with deflection[^1]
\\[
    \alpha_x = r_E \, \frac{\sqrt{q}}{\sqrt{1 - q^2}} \, \text{arctan} \left( \frac{x \, \sqrt{1 - q^2}}{\sqrt{q^2 x^2 + y^2}} \right) \;,
\\]
\\[
    \alpha_y = r_E \, \frac{\sqrt{q}}{\sqrt{1 - q^2}} \, \text{arctanh} \left( \frac{y \, \sqrt{1 - q^2}}{\sqrt{q^2 x^2 + y^2}} \right)
\\]

### Notes

When the axis ratio `q` is fixed to unity, the lens becomes a
[singular isothermal sphere](#sis), but the implemented deflection diverges.
Use the `sis` lens in this case.


NSIS
----

The `nsis` lens is a non-singular isothermal sphere with deflection[^1]
\\[
    \alpha_x = r_E \, \frac{x}{r + s} \;,
\\]
\\[
    \alpha_y = r_E \, \frac{y}{r + s}
\\]

### Notes

When the core radius `s` is fixed to zero, the lens becomes a
[singular isothermal sphere](#sis). Use the `sis` lens in this case.


NSIE
----

The `nsie` lens is a non-singular isothermal ellipsoid with deflection[^1]
\\[
    \alpha_x = r_E \, \frac{\sqrt{q}}{\sqrt{1 - q^2}} \, \text{arctan} \left( \frac{x \, \sqrt{1 - q^2}}{\sqrt{q^2 x^2 + y^2} + s} \right) \;,
\\]
\\[
    \alpha_y = r_E \, \frac{\sqrt{q}}{\sqrt{1 - q^2}} \, \text{arctanh} \left( \frac{y \, \sqrt{1 - q^2}}{\sqrt{q^2 x^2 + y^2} + q^2 s} \right)
\\]

### Notes

When the axis ratio `q` is fixed to unity, the lens becomes a
[non-singular isothermal sphere](#nsis), but the implemented deflection
diverges. Use the `nsis` lens in this case.

When the core radius `s` is fixed to zero, the lens becomes a
[singular isothermal ellipsoid](#sie). Use the `sie` lens in this case.


EPL
---

The `epl` lens follows an elliptical power law profile [^2]

\\[
    \kappa(R) = \frac{2-t}{2} \left(\frac{b}{R}\right)^t
\\]

where $R$ is the elliptical radius $R = \sqrt{q^2 x^2 + y^2}$, $b$ is the scale
length, and $t$ is the slope of the power law.

### Notes

When the axis ratio $q$ is fixed to unity, the lens becomes a regular power law
lens.

When the slope $t$ is fixed to unity, the lens becomes a
[singular isothermal ellipsoid](#sie). Use the `sie` lens in this case.

When the slope $t$ is fixed to 2, the lens becomes a point mass. Use the
`point_mass` lens in this case.


[^1]: P. Schneider, C. S. Kochanek, and J. Wambsganss, Gravitational Lensing:
      Strong, Weak and Micro (Springer, 2006).
[^2]: N. Tessore & R. B. Metcalf, A&A (2015).
