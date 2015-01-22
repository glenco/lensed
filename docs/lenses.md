Lens models
===========

SIS
---

The `sis` lens is a singular isothermal sphere with deflection[^1]
$$
    \alpha_x = r_E \, \frac{x}{r} \;,
$$
$$
    \alpha_y = r_E \, \frac{y}{r} \;,
$$
where $r_E$ is the Einstein radius, and $r$ is the distance to the position of
the lens.


SIE
---

The `sie` lens is a singular isothermal ellipsoid with deflection[^1]
$$
    \alpha_x = r_E \, \frac{\sqrt{q}}{\sqrt{1 - q^2}} \,
        \text{arc tan}\! \left( \frac{x \, \sqrt{1 - q^2}}{\omega} \right) \;,
$$
$$
    \alpha_y = r_E \, \frac{\sqrt{q}}{\sqrt{1 - q^2}} \,
        \text{arc tanh}\! \left( \frac{y \, \sqrt{1 - q^2}}{\omega} \right)
$$

### Notes

When the axis ratio `q` is fixed to unity, the lens becomes a
[singular isothermal sphere](#sis), but the implemented deflection diverges.
Use the `sis` lens in this case.


NSIS
----

The `nsis` lens is a non-singular isothermal sphere with deflection[^1]
$$
    \alpha_x = r_E \, \frac{x}{r + s} \;,
$$
$$
    \alpha_y = r_E \, \frac{y}{r + s}
$$

### Notes

When the core radius `s` is fixed to zero, the lens becomes a
[singular isothermal sphere](#sis). Use the `sis` lens in this case.


NSIE
----

The `nsie` lens is a non-singular isothermal ellipsoid with deflection[^1]
$$
    \alpha_x = r_E \, \frac{\sqrt{q}}{\sqrt{1 - q^2}} \,
        \text{arc tan}\! \left( \frac{x \, \sqrt{1 - q^2}}{\omega + s} \right) \;,
$$
$$
    \alpha_y = r_E \, \frac{\sqrt{q}}{\sqrt{1 - q^2}} \,
        \text{arc tanh}\! \left( \frac{y \, \sqrt{1 - q^2}}{\omega + q^2 s} \right)
$$

### Notes

When the axis ratio `q` is fixed to unity, the lens becomes a
[non-singular isothermal sphere](#nsis), but the implemented deflection
diverges. Use the `nsis` lens in this case.

When the core radius `s` is fixed to zero, the lens becomes a
[singular isothermal ellipsoid](#sie). Use the `sie` lens in this case.

[^1]: P. Schneider, C. S. Kochanek, and J. Wambsganss, Gravitational Lensing:
      Strong, Weak and Micro (Springer, 2006).
