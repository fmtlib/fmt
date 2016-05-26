#!/usr/bin/env python

import os, re, shutil, sys, tempfile
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

  def clean(self, *args):
    return self.call('clean', args, cwd=self.dir)

  def reset(self, *args):
    return self.call('reset', args, cwd=self.dir)

# Create build environment.
fmt_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(fmt_dir, 'doc'))

work_dir = tempfile.mkdtemp('fmt')
try:
  import build
  # The virtualenv is stored in ~/.cache/fmt/virtualenv and reused to speed
  # up builds.
  build.create_build_env(os.path.join(
    os.path.expanduser('~'), '.cache', 'fmt', 'virtualenv'))

  fmt_repo = Git(os.path.join(work_dir, 'fmt'))
  fmt_repo.clone('git@github.com:fmtlib/fmt')
  doc_repo = Git('fmtlib.github.io')
  doc_repo.clone('git@github.com:fmtlib/fmtlib.github.io')

  for version in ['1.0.0']:#, '1.1.0', '2.0.0', '3.0.0']:
    fmt_repo.clean('-f', '-d')
    fmt_repo.reset('--hard')
    fmt_repo.checkout(version)
    target_doc_dir = os.path.join(fmt_repo.dir, 'doc')
    # Remove the old theme.
    for entry in os.listdir(target_doc_dir):
      path = os.path.join(target_doc_dir, entry)
      if os.path.isdir(path):
        shutil.rmtree(path)
    # Copy the new theme.
    for entry in ['_static', '_templates', 'basic-bootstrap', 'bootstrap',
                  'conf.py', 'fmt.less']:
      src = os.path.join(fmt_dir, 'doc', entry)
      dst = os.path.join(target_doc_dir, entry)
      copy = shutil.copytree if os.path.isdir(src) else shutil.copyfile
      copy(src, dst)
    # Rename index to contents.
    contents = os.path.join(target_doc_dir, 'contents.rst')
    if not os.path.exists(contents):
      os.rename(os.path.join(target_doc_dir, 'index.rst'), contents)
    # Fix issues in reference.rst.
    reference = os.path.join(target_doc_dir, 'reference.rst')
    if os.path.exists(reference):
      with open(reference) as f:
        data = f.read()
      data = data.replace('std::ostream &', 'std::ostream&')
      data = re.sub('doxygenfunction.. (bin|oct|hexu|hex)',
                    r'doxygenfunction:: \1(int)', data)
      with open(reference, 'w') as f:
        f.write(data)
    # Build the docs.
    build.build_docs(version, doc_dir=target_doc_dir, include_dir=fmt_repo.dir,
                     work_dir=work_dir)
    # Create symlinks for older versions.
    for link, target in {'index': 'contents', 'api': 'reference'}.items():
      os.symlink(target + '.html', os.path.join('html', link) + '.html')
    # Copy docs to the website.
    version_doc_dir = os.path.join(doc_repo.dir, version)
    shutil.rmtree(version_doc_dir)
    shutil.move('html', version_doc_dir)
    # TODO: update links in navbar
    # TODO: remove doc repo
except:
  shutil.rmtree(work_dir)
