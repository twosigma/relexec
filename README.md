# Relexec

`relexec` provides "relative shebang" command to installed system-wide. This
allows scripts to be defined that run with interpreters in a related directory;
these can then run whereever the interpreter/script are found in the
filesystem.

The intended usage in a script would be in the shebang line:

```
#!/usr/libexec/relexec python3.6
<Python 3.6 code>
```

which would re-exec the file with an executable named `python3.6` in the same
directory. Specifically executing `/path/to/file arg1 arg2` would run run
relexec with this argv:

```
/usr/libexec/relexec python3.6 /path/to/file arg1 arg2
```
which it would transform to

```
/path/to/python3.6 /path/to/file arg1 arg2
```

and `exec()` the result.
