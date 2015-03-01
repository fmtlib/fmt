#!/usr/bin/env python
# Build the project on AppVeyor.

import os
from download import Downloader
from subprocess import check_call

build = os.environ['BUILD']
cmake_command = ['cmake', '-DFMT_EXTRA_TESTS=ON', '-DCMAKE_BUILD_TYPE=' + os.environ['CONFIG']]
build_command = ['msbuild', '/m:4', '/p:Config=' + os.environ['CONFIG'], 'FORMAT.sln']
test_command = ['msbuild', 'RUN_TESTS.vcxproj']
if build == 'mingw':
  # Install MinGW.
  mingw_url = 'http://ufpr.dl.sourceforge.net/project/mingw-w64/' + \
    'Toolchains%20targetting%20Win64/Personal%20Builds/mingw-builds/' + \
    '4.9.2/threads-win32/seh/x86_64-4.9.2-release-win32-seh-rt_v3-rev1.7z'
  with Downloader().download(mingw_url) as f:
    check_call(['7z', 'x', '-oC:\\', f])
    
  # Remove path to Git bin directory from $PATH because it breaks MinGW config.
  path = os.environ['PATH'].replace(r'C:\Program Files (x86)\Git\bin', '')

  os.environ['PATH'] = r'C:\Program Files (x86)\MSBUILD\12.0\bin\;' + path + r';C:\mingw64\bin'
  cmake_command.append('-GMinGW Makefiles')
  build_command = ['mingw32-make', '-j4']
  test_command = ['mingw32-make', 'test']

check_call(cmake_command)
check_call(build_command)
check_call(test_command)
