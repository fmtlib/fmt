#!/usr/bin/env python2
# Build the documentation.

from __future__ import print_function
import os, shutil, tempfile
from subprocess import check_call, check_output, CalledProcessError, Popen, PIPE

def pip_install(package, commit=None, **kwargs):
  "Install package using pip."
  if commit:
    check_version = kwargs.get('check_version', '')
    cmd = ['pip', 'show', package.split('/')[1]]
    p = Popen(cmd, stdout=PIPE, stderr=PIPE)
    stdout, stderr = p.communicate()
    if stdout and check_version in stdout:
      print('{} already installed'.format(package))
      return
    if p.returncode != 0:
      # Old versions of pip such as the one installed on Travis don't support
      # the show command - continue installation in this case.
      # Otherwise throw CalledProcessError.
      if p.returncode > 1 and 'No command by the name pip show' not in stderr:
        raise CalledProcessError(p.returncode, cmd)
    else:
      print('Uninstalling {}'.format(package))
      check_call('pip', 'uninstall', '-y', package)
    package = 'git+git://github.com/{0}.git@{1}'.format(package, commit)
  print('Installing {}'.format(package))
  check_call(['pip', 'install', '--upgrade', package])

def build_docs():
  # Create virtualenv.
  doc_dir = os.path.dirname(os.path.realpath(__file__))
  virtualenv_dir = 'virtualenv'
  check_call(['virtualenv', virtualenv_dir])
  activate_this_file = os.path.join(virtualenv_dir, 'bin', 'activate_this.py')
  execfile(activate_this_file, dict(__file__=activate_this_file))
  # Install Sphinx and Breathe.
  pip_install('sphinx-doc/sphinx',
              '4d2c17e043d9e8197fa5cd0db34212af3bb17069',
              check_version='1.4a0.dev-20151013')
  pip_install('michaeljones/breathe',
              '511b0887293e7c6b12310bb61b3659068f48f0f4')
  print(check_output(['pip', '--version']))
  print(check_output(['sphinx-build', '--version']))
  print('PATH:', os.environ['PATH'])
  print(check_output(['which', 'sphinx-build']))
  print(check_output(['cat', '/home/travis/build/cppformat/cppformat/virtualenv/bin/sphinx-build']))
  import sphinx
  print(sphinx.__version__)
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
                          FMT_USE_RVALUE_REFERENCES=1 \
                          FMT_USE_USER_DEFINED_LITERALS=1
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
