#!/usr/bin/env python2
# Build the documentation.

from __future__ import print_function
import os, shutil, tempfile
from subprocess import check_call, CalledProcessError, Popen, PIPE

def pip_install(package, commit=None):
  "Install package using pip."
  if commit:
    cmd = ['pip', 'show', package.split('/')[1]]
    p = Popen(cmd, stdout=PIPE, stderr=PIPE)
    stdout, stderr = p.communicate()
    if stdout:
      return # Already installed
    elif p.returncode != 0:
      # Old versions of pip such as the one installed on Travis don't support
      # the show command - continue installation in this case.
      # Otherwise throw CalledProcessError.
      if p.returncode > 1 and 'No command by the name pip show' not in stderr:
        raise CalledProcessError(p.returncode, cmd)
    package = 'git+git://github.com/{0}.git@{1}'.format(package, commit)
  check_call(['pip', 'install', '-q', package])

def build_docs():
  # Create virtualenv.
  doc_dir = os.path.dirname(os.path.realpath(__file__))
  virtualenv_dir = 'virtualenv'
  check_call(['virtualenv', virtualenv_dir])
  activate_this_file = os.path.join(virtualenv_dir, 'bin', 'activate_this.py')
  execfile(activate_this_file, dict(__file__=activate_this_file))
  # Install Sphinx and Breathe.
  pip_install('sphinx==1.3.1')
  pip_install('michaeljones/breathe',
              '511b0887293e7c6b12310bb61b3659068f48f0f4')
  # Build docs.
  cmd = ['doxygen', '-']
  p = Popen(cmd, stdin=PIPE)
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
    raise CalledProcessError(p.returncode, cmd)
  check_call(['sphinx-build', '-D',
              'breathe_projects.format=' + os.path.join(os.getcwd(), 'doxyxml'),
              '-b', 'html', doc_dir, 'html'])
  check_call(['lessc', '--clean-css',
              '--include-path=' + os.path.join(doc_dir, 'bootstrap'),
              os.path.join(doc_dir, 'cppformat.less'),
              'html/_static/cppformat.css'])
  return 'html'

if __name__ == '__main__':
  build_docs()
