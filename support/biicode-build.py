#!/usr/bin/env python
# Build the project with Biicode.

import bootstrap, glob, os, shutil
from download import Downloader
from subprocess import check_call

os_name = os.environ['TRAVIS_OS_NAME']
if os_name == 'linux':
  # Install newer version of CMake.
  bootstrap.install_cmake(
    'cmake-3.1.1-Linux-i386.tar.gz', check_installed=False, download_dir=None)
  with Downloader().download('http://www.biicode.com/downloads/latest/ubuntu64') as f:
    check_call(['sudo', 'dpkg', '-i', f])
elif os_name == 'osx':
  with Downloader().download('http://www.biicode.com/downloads/latest/macos') as f:
    check_call(['sudo', 'installer', '-pkg', f, '-target', '/'])

project_dir = 'biicode_project'
check_call(['bii', 'init', project_dir])
cppformat_dir = os.path.join(project_dir, 'blocks/vitaut/cppformat')
shutil.copytree('.', cppformat_dir,
                ignore=shutil.ignore_patterns('biicode_project'))
for f in glob.glob('support/biicode/*'):
  shutil.copy(f, cppformat_dir)
check_call(['bii', 'cpp:build'], cwd=project_dir)
