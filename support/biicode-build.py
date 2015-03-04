#!/usr/bin/env python
# Build the project with Biicode.

import glob, os, shutil
from subprocess import check_call

project_dir = 'biicode_project'
check_call(['bii', 'init', project_dir])
cppformat_dir = os.path.join(project_dir, 'blocks/vitaut/cppformat')
shutil.copytree('.', cppformat_dir, ignore=shutil.ignore_patterns(project_dir))
for f in glob.glob('support/biicode/*'):
  shutil.copy(f, cppformat_dir)
check_call(['bii', 'cpp:build'], cwd=project_dir)
