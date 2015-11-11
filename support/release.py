#!/usr/bin/env python
# Release script

from __future__ import print_function
import datetime, fileinput, re, sys
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
        check_call(args, cwd=self.cwd, **kwargs)

run = Runner()
run('git', 'clone', 'git@github.com:cppformat/cppformat.git')

changelog = 'ChangeLog.rst'
changelog_path = 'cppformat/' + changelog
changes, version = core.publish_file(source_path=changelog_path, writer=MDWriter())
for line in fileinput.input('cppformat/CMakeLists.txt', inplace=True):
    prefix = 'set(CPPFORMAT_VERSION '
    if line.startswith(prefix):
        line = prefix + version + ')\n'
    sys.stdout.write(line)

title_len = 0
for line in fileinput.input(changelog_path, inplace=True):
    if line.startswith(version + ' - TBD'):
        line = version + ' - ' + datetime.date.today().isoformat()
        title_len = len(line)
        line += '\n'
    elif title_len:
        line = '-' * title_len + '\n'
        title_len = 0
    sys.stdout.write(line)

run.cwd = 'cppformat'
run('git', 'add', changelog, 'CMakeLists.txt')
run('git', 'commit', '-m', 'Update version')
run('cmake', '.')
run('make', 'doc', 'package_source')

# TODO: create a release on GitHub
