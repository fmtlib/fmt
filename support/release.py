#!/usr/bin/env python

"""Create a release.

Usage:
  release.py [<branch>]
"""

from __future__ import print_function
import datetime, docopt, fileinput, json, os, re, requests, shutil, sys, tempfile
from docutils import nodes, writers, core
from subprocess import check_call

class MDWriter(writers.Writer):
    """GitHub-flavored markdown writer"""

    supported = ('md',)
    """Formats this writer supports."""

    def translate(self):
        translator = Translator(self.document)
        self.document.walkabout(translator)
        self.output = (translator.output, translator.version)


def is_github_ref(node):
    return re.match('https://github.com/.*/(issues|pull)/.*', node['refuri'])


class Translator(nodes.NodeVisitor):
    def __init__(self, document):
        nodes.NodeVisitor.__init__(self, document)
        self.output = ''
        self.indent = 0
        self.preserve_newlines = False

    def write(self, text):
        self.output += text.replace('\n', '\n' + ' ' * self.indent)

    def visit_document(self, node):
        pass

    def depart_document(self, node):
        pass

    def visit_section(self, node):
        pass

    def depart_section(self, node):
        # Skip all sections except the first one.
        raise nodes.StopTraversal

    def visit_title(self, node):
        self.version = re.match(r'(\d+\.\d+\.\d+).*', node.children[0]).group(1)
        raise nodes.SkipChildren

    def depart_title(self, node):
        pass

    def visit_Text(self, node):
        if not self.preserve_newlines:
            node = node.replace('\n', ' ')
        self.write(node)

    def depart_Text(self, node):
        pass

    def visit_bullet_list(self, node):
        pass

    def depart_bullet_list(self, node):
        pass

    def visit_list_item(self, node):
        self.write('* ')
        self.indent += 2

    def depart_list_item(self, node):
        self.indent -= 2
        self.write('\n\n')

    def visit_paragraph(self, node):
        pass

    def depart_paragraph(self, node):
        pass

    def visit_reference(self, node):
        if not is_github_ref(node):
            self.write('[')

    def depart_reference(self, node):
        if not is_github_ref(node):
            self.write('](' + node['refuri'] + ')')

    def visit_target(self, node):
        pass

    def depart_target(self, node):
        pass

    def visit_literal(self, node):
        self.write('`')

    def depart_literal(self, node):
        self.write('`')

    def visit_literal_block(self, node):
        self.write('\n\n```')
        if 'c++' in node['classes']:
            self.write('c++')
        self.write('\n')
        self.preserve_newlines = True

    def depart_literal_block(self, node):
        self.write('\n```\n')
        self.preserve_newlines = False

    def visit_inline(self, node):
        pass

    def depart_inline(self, node):
        pass


class Runner:
    def __init__(self):
        self.cwd = '.'

    def __call__(self, *args, **kwargs):
        kwargs['cwd'] = kwargs.get('cwd', self.cwd)
        check_call(args, **kwargs)

if __name__ == '__main__':
    args = docopt.docopt(__doc__)
    workdir = tempfile.mkdtemp()
    try:
        run = Runner()
        fmt_dir = os.path.join(workdir, 'fmt')
        branch = args.get('<branch>')
        if branch is None:
	  branch = 'master'
        run('git', 'clone', '-b', branch, 'git@github.com:fmtlib/fmt.git', fmt_dir)

        # Convert changelog from RST to GitHub-flavored Markdown and get the version.
        changelog = 'ChangeLog.rst'
        changelog_path = os.path.join(fmt_dir, changelog)
        changes, version = core.publish_file(source_path=changelog_path, writer=MDWriter())
        cmakelists = 'CMakeLists.txt'
        for line in fileinput.input(os.path.join(fmt_dir, cmakelists), inplace=True):
            prefix = 'set(FMT_VERSION '
            if line.startswith(prefix):
                line = prefix + version + ')\n'
            sys.stdout.write(line)

        # Update the version in the changelog.
        title_len = 0
        for line in fileinput.input(changelog_path, inplace=True):
            if line.decode('utf-8').startswith(version + ' - TBD'):
                line = version + ' - ' + datetime.date.today().isoformat()
                title_len = len(line)
                line += '\n'
            elif title_len:
                line = '-' * title_len + '\n'
                title_len = 0
            sys.stdout.write(line)
        run.cwd = fmt_dir
        run('git', 'checkout', '-b', 'release')
        run('git', 'add', changelog, cmakelists)
        run('git', 'commit', '-m', 'Update version')

        # Build the docs and package.
        run('cmake', '.')
        run('make', 'doc', 'package_source')
        site_dir = os.path.join(workdir, 'fmtlib.github.io')
        run('git', 'clone', 'git@github.com:fmtlib/fmtlib.github.io.git', site_dir)
        doc_dir = os.path.join(site_dir, version)
        shutil.copytree(os.path.join(fmt_dir, 'doc', 'html'), doc_dir,
                        ignore=shutil.ignore_patterns('.doctrees', '.buildinfo'))
        run.cwd = site_dir
        run('git', 'add', doc_dir)
        run('git', 'commit', '-m', 'Update docs')

        # Create a release on GitHub.
        run('git', 'push', 'origin', 'release', cwd=fmt_dir)
        r = requests.post('https://api.github.com/repos/fmtlib/fmt/releases',
                          params={'access_token': os.getenv('FMT_TOKEN')},
                          data=json.dumps({'tag_name': version, 'target_commitish': 'release',
					   'body': changes, 'draft': True}))
        if r.status_code != 201:
            raise Exception('Failed to create a release ' + str(r))
    finally:
        shutil.rmtree(workdir)
