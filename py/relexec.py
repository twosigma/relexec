#!/usr/bin/python2.7 -S
"""Run a script using an interpreter relative to the script."""
import sys
import os.path
try:
    interp, targetfile = sys.argv[1:3]
except:
    sys.exit(
        'usage: {} <relative path to runner> <target script> ...'.format(
            sys.argv[0]
        )
    )

if ' ' in interp or '\t' in interp:
    sys.exit('relexec: interpreter path cannot contain whitespace')

targetdir = os.path.dirname(targetfile)
while os.path.islink(targetfile):
    targetfile = os.path.join(targetdir, os.readlink(targetfile))
    targetdir = os.path.dirname(targetfile)
target_interp = os.path.join(targetdir, interp)
os.execv(target_interp, [target_interp] + sys.argv[2:])
