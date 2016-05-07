#!/usr/bin/env python
# Build the documentation.

from __future__ import print_function
import errno, os, shutil, sys, tempfile
from subprocess import check_call, check_output, CalledProcessError, Popen, PIPE
from distutils.version import LooseVersion

def pip_install(package, commit=None, **kwargs):
  "Install package using pip."
  if commit:
    check_version = kwargs.get('check_version', '')
    #output = check_output(['pip', 'show', package.split('/')[1]])
    #if check_version in output:
    #  print('{} already installed'.format(package))
    #  return
    package = 'git+git://github.com/{0}.git@{1}'.format(package, commit)
  print('Installing {}'.format(package))
  check_call(['pip', 'install', '--upgrade', package])

def build_docs(version='dev'):
  # Create virtualenv.
  doc_dir = os.path.dirname(os.path.realpath(__file__))
  virtualenv_dir = 'virtualenv'
  check_call(['virtualenv', virtualenv_dir])
  import sysconfig
  scripts_dir = os.path.basename(sysconfig.get_path('scripts'))
  activate_this_file = os.path.join(virtualenv_dir, scripts_dir,
                                    'activate_this.py')
  with open(activate_this_file) as f:
    exec(f.read(), dict(__file__=activate_this_file))
  # Upgrade pip because installation of sphinx with pip 1.1 available on Travis
  # is broken (see #207) and it doesn't support the show command.
  from pkg_resources import get_distribution, DistributionNotFound
  pip_version = get_distribution('pip').version
  if LooseVersion(pip_version) < LooseVersion('1.5.4'):
    print("Updating pip")
    check_call(['pip', 'install', '--upgrade', 'pip'])
  # Upgrade distribute because installation of sphinx with distribute 0.6.24
  # available on Travis is broken (see #207).
  try:
    distribute_version = get_distribution('distribute').version
    if LooseVersion(distribute_version) <= LooseVersion('0.6.24'):
      print("Updating distribute")
      check_call(['pip', 'install', '--upgrade', 'distribute'])
  except DistributionNotFound:
    pass
  # Install Sphinx and Breathe.
  pip_install('fmtlib/sphinx',
              '12dde8afdb0a7bb5576e2656692c3478c69d8cc3',
              check_version='1.4a0.dev-20151013')
  pip_install('michaeljones/breathe',
              '1c9d7f80378a92cffa755084823a78bb38ee4acc')
  # Build docs.
  cmd = ['doxygen', '-']
  p = Popen(cmd, stdin=PIPE)
  p.communicate(input=r'''
      PROJECT_NAME      = fmt
      GENERATE_LATEX    = NO
      GENERATE_MAN      = NO
      GENERATE_RTF      = NO
      CASE_SENSE_NAMES  = NO
      INPUT             = {0}/format.h {0}/ostream.h
      QUIET             = YES
      JAVADOC_AUTOBRIEF = YES
      AUTOLINK_SUPPORT  = NO
      GENERATE_HTML     = NO
      GENERATE_XML      = YES
      XML_OUTPUT        = doxyxml
      ALIASES           = "rst=\verbatim embed:rst"
      ALIASES          += "endrst=\endverbatim"
      MACRO_EXPANSION   = YES
      PREDEFINED        = _WIN32=1 \
                          FMT_USE_VARIADIC_TEMPLATES=1 \
                          FMT_USE_RVALUE_REFERENCES=1 \
                          FMT_USE_USER_DEFINED_LITERALS=1 \
                          FMT_API=
      EXCLUDE_SYMBOLS   = fmt::internal::* StringValue write_str
    '''.format(os.path.join(os.path.dirname(doc_dir), 'fmt')).encode('UTF-8'))
  if p.returncode != 0:
    raise CalledProcessError(p.returncode, cmd)
  check_call(['sphinx-build',
              '-Dbreathe_projects.format=' + os.path.join(os.getcwd(), 'doxyxml'),
              '-Dversion=' + version, '-Drelease=' + version, '-Aversion=' + version,
              '-b', 'html', doc_dir, 'html'])
  try:
    check_call(['lessc', '--clean-css',
                '--include-path=' + os.path.join(doc_dir, 'bootstrap'),
                os.path.join(doc_dir, 'fmt.less'),
                'html/_static/fmt.css'])
  except OSError as e:
    if e.errno != errno.ENOENT:
      raise
    print('lessc not found; make sure that Less (http://lesscss.org/) is installed')
    sys.exit(1)
  return 'html'

if __name__ == '__main__':
  build_docs(sys.argv[1])
