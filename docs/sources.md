Source models
=============

The sources in Lensed are two-dimensional surface brightness distributions
\\[
    I_S(x, y) = S(R(x, y)) \;,
\\]
where $S(r)$ is one of the one-dimensional profiles below, ellipticised by the
radius function
\\[
    R(x, y) = \sqrt{\tilde{x}^2 + q^2 \tilde{y}^2}
\\]
for the central coordinates
\\[
    \tilde{x} = (x - x_S) \cos(\theta) + (y - y_S) \sin(\theta) \;,
\\]
\\[
    \tilde{y} = (y - y_S) \cos(\theta) - (x - x_S) \sin(\theta) \;,
\\]
using source position $(x_S, y_S)$, axis ratio $q$ and position angle $\theta$.

Sources are usually normalised by their total luminosity $L_{\text{tot}}$,
which is given as the magnitude $m$ above the zero-point of the image, i.e.
\\[
    L_{\text{tot}} = 10^{-0.4 m} \;.
\\]
This means that the magnitude $m$ of a source with more than 1 count per second
must be less than $0$, and more negative values result in brighter sources.


Gaussian
--------

The `gauss` profile is a Gaussian blob given by
\\[
    S(r) = \frac{L_{\text{tot}}}{q \, \pi \sigma^2} \,
        \frac{1}{2} \, e^{ -\frac{1}{2} r^2/\sigma^2 } \;,
\\]
where $\sigma^2$ is the variance of the profile.

### Parameters

| Name      | Description                        | Range                  |
|-----------|------------------------------------|------------------------|
| `x`       | source position $x_S$              | image pixels           |
| `y`       | source position $y_S$              | image pixels           |
| `mag`     | total magnitude $m$                |                        |
| `sigma`   | standard deviation $\sigma$        | $\sigma > 0$           |
| `q`       | axis ratio $q$                     | $0 < q < 1$            |
| `pa`      | position angle $\theta$ in $\deg$  | $0 \leq \theta < 180$  |

Exponential
-----------

The `exp` profile is an exponential disk given by
\\[
    S(r) = \frac{L_{\text{tot}}}{q \, \pi R_s^2} \,
        \frac{1}{2} \, e^{ - r/R_s } \;,
\\]
where $R_s$ is the scale length of the profile.

### Parameters

| Name      | Description                        | Range                  |
|-----------|------------------------------------|------------------------|
| `x`       | source position $x_S$              | image pixels           |
| `y`       | source position $y_S$              | image pixels           |
| `mag`     | total magnitude $m$                |                        |
| `rs`      | scale length $R_s$                 | $R_s > 0$              |
| `q`       | axis ratio $q$                     | $0 < q < 1$            |
| `pa`      | position angle $\theta$ in $\deg$  | $0 \leq \theta < 180$  |


De Vaucouleurs
--------------

The `devauc` profile is De Vaucouleurs law, given by
\\[
    S(r) = \frac{L_{\text{tot}}}{q \, \pi R_{\text{eff}}^2} \,
        \frac{b^8}{8!} \,
        \exp\left( -b \, (r/R_{\text{eff}})^{1/4} \right) \;,
\\]
where $b = 7.6692494425008039044$ and $R_{\text{eff}}$ is the
effective radius containing half the total luminosity.

### Parameters

| Name      | Description                        | Range                  |
|-----------|------------------------------------|------------------------|
| `x`       | source position $x_S$              | image pixels           |
| `y`       | source position $y_S$              | image pixels           |
| `mag`     | total magnitude $m$                |                        |
| `r`       | effective radius $R_{\text{eff}}$  | $R_{\text{eff}} > 0$   |
| `q`       | axis ratio $q$                     | $0 < q < 1$            |
| `pa`      | position angle $\theta$ in $\deg$  | $0 \leq \theta < 180$  |


Sérsic
------

The `sersic` profile is an extension of De Vaucouleurs law to the general
Sérsic $1/n$ law given by[^1][^2]
\\[
    S(r) = \frac{L_{\text{tot}}}{q \, \pi R_{\text{eff}}^2} \,
        \frac{b_n^{2n}}{\Gamma(2n+1)} \,
        \exp\left( -b_n \, (r/R_{\text{eff}})^{1/n} \right) \;,
\\]
where $R_{\text{eff}}$ is the effective radius containing half the total
luminosity, and the coefficient $b_n$ is the solution of
\\[
    \frac{\Gamma(2n, b_n)}{\Gamma(2n)} = \frac{1}{2} \;,
\\]
which can be approximated by the minimax polynomial
\\[
    b_n \approx 1.9992n - 0.3271
\\]
in the range $0.5 < n < 8$.

The Sérsic profile with $n = 0.5$, $n = 1$ and $n = 4$ is equivalent to the
[Gaussian](#gaussian), [exponential](#exponential) and
[De Vaucouleurs](#de-vaucouleurs) profile, respectively, with a different
radius definition.

### Parameters

| Name      | Description                        | Range                  |
|-----------|------------------------------------|------------------------|
| `x`       | source position $x_S$              | image pixels           |
| `y`       | source position $y_S$              | image pixels           |
| `mag`     | total magnitude $m$                |                        |
| `r`       | effective radius $R_{\text{eff}}$  | $R_{\text{eff}} > 0$   |
| `n`       | Sérsic index $n$                   | $0.5 < n < 8$          |
| `q`       | axis ratio $q$                     | $0 < q < 1$            |
| `pa`      | position angle $\theta$ in $\deg$  | $0 \leq \theta < 180$  |


[^1]: J. L. Sérsic, (1968).
[^2]: A. W. Graham and S. P. Driver, Publ. Astron. Soc. Aust 22, 118 (2005).
