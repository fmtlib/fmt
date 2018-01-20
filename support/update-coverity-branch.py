#!/usr/bin/env python
# Update the coverity branch from the master branch.
# It is not done automatically because Coverity Scan limits
# the number of submissions per day.

from __future__ import print_function
import shutil, tempfile
from subprocess import check_output, STDOUT

class Git:
    def __init__(self, dir):
        self.dir = dir

    def __call__(self, *args):
        output = check_output(['git'] + list(args), cwd=self.dir, stderr=STDOUT)
        print(output)
        return output

dir = tempfile.mkdtemp()
try:
    git = Git(dir)
    git('clone', '-b', 'coverity', 'git@github.com:fmtlib/fmt.git', dir)
    output = git('merge', '-X', 'theirs', '--no-commit', 'origin/master')
    if 'Fast-forward' not in output:
        git('reset', 'HEAD', '.travis.yml')
        git('checkout', '--', '.travis.yml')
        git('commit', '-m', 'Update coverity branch')
    git('push')
finally:
    shutil.rmtree(dir)
