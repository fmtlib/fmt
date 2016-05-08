#!/usr/bin/env python

import os, shutil, sys
from subprocess import check_call

class Git:
  def __init__(self, dir):
    self.dir = dir

  def call(self, method, args, **kwargs):
    return check_call(['git', method] + list(args), **kwargs)

  def clone(self, *args):
    return self.call('clone', list(args) + [self.dir])

  def checkout(self, *args):
    return self.call('checkout', args, cwd=self.dir)

# Create build environment.
fmt_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(fmt_dir, 'doc'))
import build
build.create_build_env()

git = Git('fmt')
git.clone('git@github.com:fmtlib/fmt.git')

versions = ['1.0.0']
for version in versions:
  git.checkout(version)
  target_doc_dir = os.path.join(git.dir, 'doc')
  # Remove the old theme.
  for entry in os.listdir(target_doc_dir):
    path = os.path.join(target_doc_dir, entry)
    if os.path.isdir(path):
      shutil.rmtree(path)
  # Copy the new theme.
  for entry in ['_static', '_templates', 'basic-bootstrap', 'bootstrap']:
    src = os.path.join(fmt_dir, 'doc', entry)
    dst = os.path.join(target_doc_dir, entry)
    shutil.copytree(src, dst)
    build.build_docs(version, target_doc_dir)
    # TODO: copy docs to website
