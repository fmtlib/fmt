#!/usr/bin/env python
# Update the coverity branch from the master branch.
# It is not done automatically because Coverity Scan limits
# the number of submissions per day.

from __future__ import print_function
import shutil, tempfile
from subprocess import check_call

class Git:
  def __init__(self, dir):
    self.dir = dir

  def __call__(self, *args):
    check_call(['git'] + list(args), cwd=self.dir)

dir = tempfile.mkdtemp()
try:
  git = Git(dir)
  git('clone', '-b', 'coverity', 'git@github.com:cppformat/cppformat.git', dir)
  git('merge', '-X', 'theirs', '--no-commit', 'origin/master')
  git('reset', 'HEAD', '.travis.yml')
  git('checkout', '--', '.travis.yml')
  git('commit', '-m', 'Update coverity branch')
  git('push')
finally:
  shutil.rmtree(dir)
