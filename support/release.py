#!/usr/bin/env python3

"""Make a release.

Usage:
  release.py [<branch>]

For the release command $FMT_TOKEN should contain a GitHub personal access token
obtained from https://github.com/settings/tokens.
"""

from __future__ import print_function
import datetime, docopt, errno, fileinput, json, os
import re, shutil, sys
from subprocess import check_call
import urllib.request


class Git:
    def __init__(self, dir):
        self.dir = dir

    def call(self, method, args, **kwargs):
        return check_call(['git', method] + list(args), **kwargs)

    def add(self, *args):
        return self.call('add', args, cwd=self.dir)

    def checkout(self, *args):
        return self.call('checkout', args, cwd=self.dir)

    def clean(self, *args):
        return self.call('clean', args, cwd=self.dir)

    def clone(self, *args):
        return self.call('clone', list(args) + [self.dir])

    def commit(self, *args):
        return self.call('commit', args, cwd=self.dir)

    def pull(self, *args):
        return self.call('pull', args, cwd=self.dir)

    def push(self, *args):
        return self.call('push', args, cwd=self.dir)

    def reset(self, *args):
        return self.call('reset', args, cwd=self.dir)

    def update(self, *args):
        clone = not os.path.exists(self.dir)
        if clone:
            self.clone(*args)
        return clone


def clean_checkout(repo, branch):
    repo.clean('-f', '-d')
    repo.reset('--hard')
    repo.checkout(branch)


class Runner:
    def __init__(self, cwd, env=None):
        self.cwd = cwd
        self.env = env

    def __call__(self, *args, **kwargs):
        kwargs['cwd'] = kwargs.get('cwd', self.cwd)
        if self.env is not None:
            kwargs['env'] = kwargs.get('env', self.env)
        check_call(args, **kwargs)


def create_build_env():
    """Create a build environment."""
    class Env:
        pass
    env = Env()
    env.fmt_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    env.build_dir = 'build'
    env.fmt_repo = Git(os.path.join(env.build_dir, 'fmt'))
    return env


def create_doc_env(env, fmt_repo):
    """Create a virtualenv with the pinned documentation dependencies and
    return an environment dict with it prepended to PATH. This ensures the
    docs are built with the exact mkdocs/mkdocstrings versions required by the
    custom handler, regardless of what is installed system-wide."""
    venv_dir = os.path.join(env.build_dir, 'venv')
    shutil.rmtree(venv_dir, ignore_errors=True)
    check_call([sys.executable, '-m', 'venv', venv_dir])
    venv_bin = os.path.join(venv_dir, 'bin')
    pip = os.path.join(venv_bin, 'pip')
    check_call([pip, 'install', '--quiet', '--upgrade', 'pip'])
    requirements = os.path.join(
        fmt_repo.dir, 'support', 'doc-requirements.txt')
    check_call(
        [pip, 'install', '--quiet', '--require-hashes', '-r', requirements])
    doc_env = os.environ.copy()
    doc_env['PATH'] = venv_bin + os.pathsep + doc_env.get('PATH', '')
    return doc_env


if __name__ == '__main__':
    args = docopt.docopt(__doc__)
    env = create_build_env()
    fmt_repo = env.fmt_repo

    branch = args.get('<branch>')
    if branch is None:
        branch = 'main'
    if not fmt_repo.update('-b', branch, 'git@github.com:fmtlib/fmt'):
        clean_checkout(fmt_repo, branch)

    # Update the date in the changelog and extract the version and the first
    # section content.
    changelog = 'ChangeLog.md'
    changelog_path = os.path.join(fmt_repo.dir, changelog)
    is_first_section = True
    first_section = []
    for i, line in enumerate(fileinput.input(changelog_path, inplace=True)):
        if i == 0:
            version = re.match(r'# (.*) - TBD', line).group(1)
            line = '# {} - {}\n'.format(
                version, datetime.date.today().isoformat())
        elif not is_first_section:
            pass
        elif line.startswith('#'):
            is_first_section = False
        else:
            first_section.append(line)
        sys.stdout.write(line)
    if first_section[0] == '\n':
        first_section.pop(0)

    ns_version = None
    base_h_path = os.path.join(fmt_repo.dir, 'include', 'fmt', 'base.h')
    for line in fileinput.input(base_h_path):
        m = re.match(r'\s*inline namespace v(.*) .*', line)
        if m:
            ns_version = m.group(1)
            break
    major_version = version.split('.')[0]
    if not ns_version or ns_version != major_version:
        raise Exception(f'Version mismatch {ns_version} != {major_version}')

    # Workaround GitHub-flavored Markdown treating newlines as <br>.
    changes = ''
    code_block = False
    stripped = False
    for line in first_section:
        if re.match(r'^\s*```', line):
            code_block = not code_block
            changes += line
            stripped = False
            continue
        if code_block:
            changes += line
            continue
        if line == '\n' or re.match(r'^\s*\|.*', line):
            if stripped:
                changes += '\n'
                stripped = False
            changes += line
            continue
        if stripped:
            line = ' ' + line.lstrip()
        changes += line.rstrip()
        stripped = True

    fmt_repo.checkout('-B', 'release')
    fmt_repo.add(changelog)
    fmt_repo.commit('-m', 'Update version')

    # Build the docs locally in a virtualenv with the pinned doc dependencies;
    # the source zip is now built and attached to the release in CI by
    # .github/workflows/release.yml, which also generates a SLSA provenance
    # attestation for it. The venv is prepended to PATH so that CMake's
    # find_program(MKDOCS mkdocs) and the ./mkdocs deploy step below both pick
    # up the correct mkdocs/mkdocstrings versions.
    doc_env = create_doc_env(env, fmt_repo)
    run = Runner(fmt_repo.dir, env=doc_env)
    run('cmake', '.')
    run('make', 'doc')

    # Create a draft release on GitHub. The release workflow triggers on
    # `release: created`, builds the source zip from `target_commitish`, and
    # attaches the zip plus *.intoto.jsonl provenance to this draft. After
    # reviewing the draft, the maintainer clicks Publish to finalize.
    fmt_repo.push('origin', 'release')
    auth_headers = {'Authorization': 'token ' + os.getenv('FMT_TOKEN')}
    req = urllib.request.Request(
        'https://api.github.com/repos/fmtlib/fmt/releases',
        data=json.dumps({'tag_name': version,
                         'target_commitish': 'release',
                         'body': changes, 'draft': True}).encode('utf-8'),
        headers=auth_headers, method='POST')
    with urllib.request.urlopen(req) as response:
        if response.status != 201:
            raise Exception(f'Failed to create a release ' +
                            '{response.status} {response.reason}')

    short_version = '.'.join(version.split('.')[:-1])
    check_call(['./mkdocs', 'deploy', short_version], env=doc_env)
