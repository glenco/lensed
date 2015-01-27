Working on lensed
=================

This document describes the procedure for contributing to the development of
lensed.

Using the repository
--------------------

There is **one rule** for using the repository: **do not push to master**.
Instead, push to a branch under your initials, e.g. `nt/my-branch` if you have
write access (or create a fork if you do not, as usual). Then, when you want to
merge your completed changes into the master branch, open a pull request (PR)
so that others are aware of your changes and can comment on them. This is the
usual [GitHub flow][flow].

**The master branch must always remain usable.**


Issues
------

You can open issues for anything that should be discussed, not just for bugs.
Issues can be tagged with labels according to the type of discussion you want
to initiate:

-   *bug* -- something in the code does not work
-   *code* -- regards the organisation of the code itself
-   *doc* -- about documentation
-   *idea* -- ideas without concrete implementations
-   *question* -- general questions about the code
-   *todo* -- things that need to be implemented

There are additional labels that can be used to describe the severity of the
changes to the existing code:

-   *major* -- requires a major release
-   *minor* -- requires a minor release
-   *patch* -- requires a patch release

Issues can be closed and given one of the following labels as reason:

-   *duplicate* -- an existing issue contains the same discussion
-   *invalid* -- the issue is not appropriate
-   *wontfix* -- while the issue is acknowledged as such, it will not change


Pull Requests
-------------

Since all code changes should come through Pull Requests, please take care to
make them as informative and well-written as possible. Keep in mind that PRs
are a part of the project's history, and that users will look at past PRs when
trying to solve their problems. The GitHub team has written a blog post on
[writing Pull Requests][prs], and it is generally a good guideline.

Once a PR is opened, someone else from the lensed team should look it over, and
comment or merge as appropriate.

[flow]: https://guides.github.com/introduction/flow/index.html
[prs]: https://github.com/blog/1943-how-to-write-the-perfect-pull-request
