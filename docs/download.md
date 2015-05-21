Get Lensed
==========

There are a number of ways to get Lensed, depending on the intended use case
and capabilities of the system.


Releases
--------

*Note: binary releases are not yet available*

The [releases page](https://github.com/glenco/lensed/releases) contains current
and previous versions of Lensed that have been assigned a fixed version number.

Release versions are provided in both source and binary form. The binaries are
built on the following platforms:

-   Linux: *Ubuntu 12.04 LTS*
-   Mac OS X: *Mac OS X Yosemite*

Please note that the binary versions are linked dynamically against Lensed's
[dependencies](dependencies/), which must still be satisfied.

This method of distribution is intended primarily for users of Lensed who do
not wish to modify the source code. It is still possible to extend Lensed with
new lens and source objects. For people intending to develop Lensed, please see
the next section.


Using Git
---------

Lensed is developed using the [Git version control system](https://git-scm.com)
and [hosted on GitHub](https://github.com/glenco/lensed). Use Git to get the
most recent development version of the code, especially if you plan on taking
part in extending the code or fixing bugs.

With Git installed on your system, use

```sh
$ git clone https://github.com/glenco/lensed.git
```

to clone the most recent version of the code into a subfolder of the current
directory.

There is a lot of information available about Git. For a first introduction,
please [refer to the documentation](https://git-scm.com/doc), especially the
first two chapters on [getting started](https://git-scm.com/book/en/v2/Getting-Started-About-Version-Control)
and the [Git basics](https://git-scm.com/book/en/v2/Git-Basics-Getting-a-Git-Repository).
A quick introduction can also be found in the [Git tutorial](http://git-scm.com/docs/gittutorial).


From an archive
---------------

On systems where no Git client is available, it is alternatively possible to
get the latest development version of the sources in the form of an archive.
A link is provided at the [project page](https://github.com/glenco/lensed).

When working in a terminal environment, it is possible to download and extract
Lensed using a combination of cURL and tar with the following command:

```sh
$ curl -L https://github.com/glenco/lensed/archive/master.tar.gz | tar -xz
```

Alternatively, if `curl` is not available, you can try to use `wget` instead:

```sh
$ wget https://github.com/glenco/lensed/archive/master.tar.gz
$ tar -xzf master.tar.gz
```

Finally, if `tar` is not available you can replace the `.tar.gz` extension by
`.zip`, and try the `unzip` command.

Please note that using this method to get Lensed, it will not be possible to
commit your changes or bug fixes back into the code base. Use of this method is
therefore discouraged for normal development work, and only intended for the
distribution of the code to "difficult" environments.
