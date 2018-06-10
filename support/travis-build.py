#!/usr/bin/env python
# Build the project on Travis CI.

from __future__ import print_function
import errno, os, shutil, subprocess, sys, urllib
from subprocess import call, check_call, Popen, PIPE, STDOUT

def rmtree_if_exists(dir):
    try:
        shutil.rmtree(dir)
    except OSError as e:
        if e.errno == errno.ENOENT:
            pass

def makedirs_if_not_exist(dir):
    try:
        os.makedirs(dir)
    except OSError as e:
        if e.errno != errno.EEXIST:
            raise

def install_dependencies():
    branch = os.environ['TRAVIS_BRANCH']
    if branch != 'master':
        print('Branch: ' + branch)
        exit(0) # Ignore non-master branches
    check_call('curl -s https://deb.nodesource.com/gpgkey/nodesource.gpg.key ' +
               '| sudo apt-key add -', shell=True)
    check_call('echo "deb https://deb.nodesource.com/node_0.10 precise main" ' +
               '| sudo tee /etc/apt/sources.list.d/nodesource.list', shell=True)
    check_call(['sudo', 'apt-get', 'update'])
    check_call(['sudo', 'apt-get', 'install', 'python-virtualenv', 'nodejs'])
    check_call(['sudo', 'npm', 'install', '-g', 'less@2.6.1', 'less-plugin-clean-css'])
    deb_file = 'doxygen_1.8.6-2_amd64.deb'
    urllib.urlretrieve('http://mirrors.kernel.org/ubuntu/pool/main/d/doxygen/' +
                       deb_file, deb_file)
    check_call(['sudo', 'dpkg', '-i', deb_file])

fmt_dir = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))

build = os.environ['BUILD']
if build == 'Doc':
    travis = 'TRAVIS' in os.environ
    if travis:
        install_dependencies()
    sys.path.insert(0, os.path.join(fmt_dir, 'doc'))
    import build
    build.create_build_env()
    html_dir = build.build_docs()
    repo = 'fmtlib.github.io'
    if travis and 'KEY' not in os.environ:
        # Don't update the repo if building on Travis from an account that
        # doesn't have push access.
        print('Skipping update of ' + repo)
        exit(0)
    # Clone the fmtlib.github.io repo.
    rmtree_if_exists(repo)
    git_url = 'https://github.com/' if travis else 'git@github.com:'
    check_call(['git', 'clone', git_url + 'fmtlib/{}.git'.format(repo)])
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
            cmd += ' https://$KEY@github.com/fmtlib/fmtlib.github.io.git master'
        p = Popen(cmd, shell=True, stdout=PIPE, stderr=STDOUT, cwd=repo)
        # Print the output without the key.
        print(p.communicate()[0].replace(os.environ['KEY'], '$KEY'))
        if p.returncode != 0:
            raise subprocess.CalledProcessError(p.returncode, cmd)
    exit(0)

standard = os.environ['STANDARD']
install_dir    = os.path.join(fmt_dir, "_install")
build_dir      = os.path.join(fmt_dir, "_build")
test_build_dir = os.path.join(fmt_dir, "_build_test")

# Configure library.
makedirs_if_not_exist(build_dir)
cmake_flags = [
    '-DCMAKE_INSTALL_PREFIX=' + install_dir, '-DCMAKE_BUILD_TYPE=' + build,
    '-DCMAKE_CXX_STANDARD=' + standard
]
check_call(['cmake', '-DFMT_DOC=OFF', '-DFMT_PEDANTIC=ON', '-DFMT_WERROR=ON', fmt_dir] +
           cmake_flags, cwd=build_dir)

# Build library.
check_call(['make', '-j4'], cwd=build_dir)

# Test library.
env = os.environ.copy()
env['CTEST_OUTPUT_ON_FAILURE'] = '1'
if call(['make', 'test'], env=env, cwd=build_dir):
    with open(os.path.join(build_dir, 'Testing', 'Temporary', 'LastTest.log'), 'r') as f:
        print(f.read())
    sys.exit(-1)

# Install library.
check_call(['make', 'install'], cwd=build_dir)

# Test installation.
makedirs_if_not_exist(test_build_dir)
check_call(['cmake', os.path.join(fmt_dir, "test", "find-package-test")] +
            cmake_flags, cwd=test_build_dir)
check_call(['make', '-j4'], cwd=test_build_dir)
