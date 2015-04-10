#!/usr/bin/env python
# Build the documentation.

from __future__ import print_function
import os, shutil, tempfile
from subprocess import check_call, Popen, PIPE

def pip_install(package, commit=None):
  "Install package using pip."
  if commit:
    package = 'git+git://github.com/{0}.git@{1}'.format(package, commit)
  check_call(['pip', 'install', '-q', package])

def build_docs(workdir, travis):
  # Create virtualenv.
  virtualenv_dir = os.path.join(workdir, 'virtualenv')
  check_call(['virtualenv', virtualenv_dir])
  activate_this_file = os.path.join(virtualenv_dir, 'bin', 'activate_this.py')
  execfile(activate_this_file, dict(__file__=activate_this_file))
  # Install Sphinx, Breathe and the Bootstrap theme.
  pip_install('sphinx==1.2.2')
  pip_install('michaeljones/breathe', '18bd461b4e29dde0adf5df4b3da7e5473e2c2983')
  pip_install('cppformat/sphinx-bootstrap-theme', '534ba82ef51c17e0f13e8e57b9fa133fa1926c80')
  # Build docs.
  doc_dir = os.path.dirname(__file__)
  p = Popen(['doxygen', '-'], stdin=PIPE)
  p.communicate(input=r'''
      PROJECT_NAME      = C++ Format
      GENERATE_LATEX    = NO
      GENERATE_MAN      = NO
      GENERATE_RTF      = NO
      CASE_SENSE_NAMES  = NO
      INPUT             = {0}/format.h
      QUIET             = YES
      JAVADOC_AUTOBRIEF = YES
      AUTOLINK_SUPPORT  = NO
      GENERATE_HTML     = NO
      GENERATE_XML      = YES
      XML_OUTPUT        = doxyxml
      ALIASES           = "rst=\verbatim embed:rst"
      ALIASES          += "endrst=\endverbatim"
      PREDEFINED        = _WIN32=1 \
                          FMT_USE_VARIADIC_TEMPLATES=1 \
                          FMT_USE_RVALUE_REFERENCES=1
      EXCLUDE_SYMBOLS   = fmt::internal::* StringValue write_str
    '''.format(os.path.dirname(doc_dir)))
  if p.returncode != 0:
    return p.returncode
  check_call(['sphinx-build',
              '-D', "breathe_projects.format=" + os.path.join(os.getcwd(), "doxyxml"),
              '-b', 'html', doc_dir, 'html'])
  return 0

returncode = 1
travis = 'TRAVIS' in os.environ
workdir = tempfile.mkdtemp()
try:
  returncode = build_docs(workdir, travis=travis)
finally:
  # Don't remove workdir on Travis because the VM is discarded anyway.
  if not travis:
    shutil.rmtree(workdir)
exit(returncode)
