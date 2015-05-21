#!/usr/bin/env python
# Build the project on Travis CI.

from __future__ import print_function
import errno, os, re, shutil, sys, tempfile
from subprocess import call, check_call, Popen, PIPE, STDOUT

def rmtree_if_exists(dir):
  try:
    shutil.rmtree(dir)
  except OSError as e:
    if e.errno == errno.ENOENT:
      pass

build = os.environ['BUILD']
if build == 'Doc':
  travis = 'TRAVIS' in os.environ
  # Install dependencies.
  if travis:
    with open('/etc/apt/sources.list.d/nodesource.list', 'a') as f:
      f.write('deb http://deb.nodesource.com/node_0.10 precise main\n')
    check_call(['sudo', 'apt-get', 'update'])
    check_call(['sudo', 'apt-get', 'install', 'python-virtualenv', 'doxygen', 'nodejs'])
    check_call(['npm', 'install', '-g', 'less'])
  cppformat_dir = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
  sys.path.insert(0, os.path.join(cppformat_dir, 'doc'))
  import build
  html_dir = build.build_docs()
  # Clone the cppformat.github.io repo.
  repo = 'cppformat.github.io'
  rmtree_if_exists(repo)
  git_url = 'https://github.com/' if travis else 'git@github.com:'
  check_call(['git', 'clone', git_url + 'cppformat/{}.git'.format(repo)])
  # Copy docs to the repo.
  target_dir = os.path.join(repo, 'dev')
  rmtree_if_exists(target_dir)
  shutil.copytree(html_dir, target_dir, ignore=shutil.ignore_patterns('.*'))
  if travis:
    check_call(['git', 'config', '--global', 'user.name', 'amplbot'])
    check_call(['git', 'config', '--global', 'user.email', 'viz@ampl.com'])
  # Push docs to GitHub pages.
  check_call(['git', 'add', '--all'], cwd=repo)
  if call(['git', 'diff-index', '--quiet', 'HEAD'], cwd=repo):
    check_call(['git', 'commit', '-m', 'Update documentation'], cwd=repo)
    cmd = 'git push'
    if travis:
      cmd += ' https://$KEY@github.com/cppformat/cppformat.github.io.git master'
    p = Popen(cmd, shell=True, stdout=PIPE, stderr=STDOUT, cwd=repo)
    # Remove URL from output because it may contain a token.
    print(re.sub(r'https:.*\.git', '<url>', p.communicate()[0]))
    if p.returncode != 0:
      raise CalledProcessError(p.returncode, cmd)
  exit(0)

check_call(['git', 'submodule', 'update', '--init'])
check_call(['cmake', '-DCMAKE_BUILD_TYPE=' + build, '-DFMT_PEDANTIC=ON', '.'])
check_call(['make', '-j4'])
env = os.environ.copy()
env['CTEST_OUTPUT_ON_FAILURE'] = '1'
if call(['make', 'test'], env=env):
  with open('Testing/Temporary/LastTest.log', 'r') as f:
    print(f.read())
