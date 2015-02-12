#!/usr/bin/env python
# Build the project on AppVeyor.

from subprocess import check_call
import os

env = os.environ
build = env['BUILD']
cmake_command = ['cmake', '-DFMT_EXTRA_TESTS=ON', '-DCMAKE_BUILD_TYPE=' + env['CONFIG']]
if build == 'mingw':
  # Remove path to Git bin directory from $PATH because it breaks MinGW config.
  env['PATH'] = env['PATH'].replace(r'C:\Program Files (x86)\Git\bin', '')
  cmake_command.append('-GMinGW Makefiles')
check_call(cmake_command, env=env)
