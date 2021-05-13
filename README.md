# Relexec

![GitHub Workflow Status](https://img.shields.io/github/workflow/status/twosigma/relexec/Run%20tests)

`relexec` is a helper program to enable "relative shebangs".

Relative shebangs are not natively supported on POSIX systems. Paths specified
as relative in shebangs are resolved relative to the current working directory,
behaviour which is almost useless.

To work around this, a shebang can be specified with the `relexec` program,
installed in a well-known path. `relexec` constructs a path relative to the
script and execs it.

This allows scripts to be defined that run with interpreters installed with the
script. These can then run from anywhere in the filesystem, or moved to another
location without breaking (as long as the relative path between the script and
interpreter does not change).

Unlike other solutions to this problem, it is easy to programmatically rewrite
shebangs to use relexec (or rewrite them back).


## Usage

The intended usage is in the shebang line of a script:

```
#!/usr/lib/relexec python3.9
<Python 3.9 code>
```

which would re-exec the file with an executable named `python3.9` in the same
directory. Specifically executing `/path/to/file arg1 arg2` would run run
relexec with this argv:

```
/usr/lib/relexec python3.9 /path/to/file arg1 arg2
```
which it would transform to

```
/path/to/python3.9 /path/to/file arg1 arg2
```

and `exec()` the result.


## Installation

We provide C, Rust, and for reference, Python versions of `relexec`.

We specify that the `relexec` binary, if installed, must be installed to
`/usr/lib/relexec` so that this path can be hard coded in shebangs.

`/usr/lib` is the appropriate place for internal binaries according to the
[Filesystem Hierarchy Standard 3.0](https://refspecs.linuxfoundation.org/FHS_3.0/fhs/ch04s06.html),
as `/usr/libexec` does not exist on all systems.
