#!/usr/bin/env python
# Build the documentation.

from __future__ import print_function
import errno, os, shutil, sys, tempfile
from subprocess import check_call, check_output, CalledProcessError, Popen, PIPE
from distutils.version import LooseVersion

versions = ['1.0.0', '1.1.0', '2.0.0', '3.0.2', '4.0.0', '4.1.0', '5.0.0', '5.1.0', '5.2.0', '5.2.1', '5.3.0', '6.0.0', '6.1.0', '6.1.1', '6.1.2', '6.2.0', '6.2.1', '7.0.0', '7.0.1', '7.0.2', '7.0.3', '7.1.0', '7.1.1', '7.1.2']

def pip_install(package, commit=None, **kwargs):
  "Install package using pip."
  if commit:
    package = 'git+https://github.com/{0}.git@{1}'.format(package, commit)
  print('Installing {0}'.format(package))
  check_call(['pip', 'install', package])

def create_build_env(dirname='virtualenv'):
  # Create virtualenv.
  if not os.path.exists(dirname):
    check_call(['virtualenv', dirname])
  import sysconfig
  scripts_dir = os.path.basename(sysconfig.get_path('scripts'))
  activate_this_file = os.path.join(dirname, scripts_dir, 'activate_this.py')
  with open(activate_this_file) as f:
    exec(f.read(), dict(__file__=activate_this_file))
  # Import get_distribution after activating virtualenv to get info about
  # the correct packages.
  from pkg_resources import get_distribution, DistributionNotFound
  # Upgrade pip because installation of sphinx with pip 1.1 available on Travis
  # is broken (see #207) and it doesn't support the show command.
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
  # Install Sphinx and Breathe. Require the exact version of Sphinx which is
  # compatible with Breathe.
  pip_install('sphinx-doc/sphinx', '12b83372ac9316e8cbe86e7fed889296a4cc29ee')
  pip_install('michaeljones/breathe',
              '129222318f7c8f865d2631e7da7b033567e7f56a')

def build_docs(version='dev', **kwargs):
  doc_dir = kwargs.get('doc_dir', os.path.dirname(os.path.realpath(__file__)))
  work_dir = kwargs.get('work_dir', '.')
  include_dir = kwargs.get(
      'include_dir', os.path.join(os.path.dirname(doc_dir), 'include', 'fmt'))
  # Build docs.
  cmd = ['doxygen', '-']
  p = Popen(cmd, stdin=PIPE)
  doxyxml_dir = os.path.join(work_dir, 'doxyxml')
  p.communicate(input=r'''
      PROJECT_NAME      = fmt
      GENERATE_LATEX    = NO
      GENERATE_MAN      = NO
      GENERATE_RTF      = NO
      CASE_SENSE_NAMES  = NO
      INPUT             = {0}/chrono.h {0}/color.h {0}/core.h {0}/compile.h \
                          {0}/format.h {0}/os.h {0}/ostream.h {0}/printf.h
      QUIET             = YES
      JAVADOC_AUTOBRIEF = YES
      AUTOLINK_SUPPORT  = NO
      GENERATE_HTML     = NO
      GENERATE_XML      = YES
      XML_OUTPUT        = {1}
      ALIASES           = "rst=\verbatim embed:rst"
      ALIASES          += "endrst=\endverbatim"
      MACRO_EXPANSION   = YES
      PREDEFINED        = _WIN32=1 \
                          FMT_USE_VARIADIC_TEMPLATES=1 \
                          FMT_USE_RVALUE_REFERENCES=1 \
                          FMT_USE_USER_DEFINED_LITERALS=1 \
                          FMT_USE_ALIAS_TEMPLATES=1 \
                          FMT_API= \
                          "FMT_BEGIN_NAMESPACE=namespace fmt {{" \
                          "FMT_END_NAMESPACE=}}" \
                          "FMT_STRING_ALIAS=1" \
                          "FMT_ENABLE_IF(B)="
      EXCLUDE_SYMBOLS   = fmt::formatter fmt::printf_formatter fmt::arg_join \
                          fmt::basic_format_arg::handle
    '''.format(include_dir, doxyxml_dir).encode('UTF-8'))
  if p.returncode != 0:
    raise CalledProcessError(p.returncode, cmd)
  html_dir = os.path.join(work_dir, 'html')
  main_versions = reversed(versions[-3:])
  check_call(['sphinx-build',
              '-Dbreathe_projects.format=' + os.path.abspath(doxyxml_dir),
              '-Dversion=' + version, '-Drelease=' + version,
              '-Aversion=' + version, '-Aversions=' + ','.join(main_versions),
              '-b', 'html', doc_dir, html_dir])
  try:
    check_call(['lessc', '--clean-css',
                '--include-path=' + os.path.join(doc_dir, 'bootstrap'),
                os.path.join(doc_dir, 'fmt.less'),
                os.path.join(html_dir, '_static', 'fmt.css')])
  except OSError as e:
    if e.errno != errno.ENOENT:
      raise
    print('lessc not found; make sure that Less (http://lesscss.org/) ' +
          'is installed')
    sys.exit(1)
  return html_dir

if __name__ == '__main__':
  create_build_env()
  build_docs(sys.argv[1])
