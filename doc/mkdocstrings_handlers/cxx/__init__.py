# A basic mkdocstrings handler for C++.
# Copyright (c) 2012 - present, Victor Zverovich

import os
import xml.etree.ElementTree as et
from mkdocstrings.handlers.base import BaseHandler
from typing import Any, Mapping, Optional
from subprocess import CalledProcessError, PIPE, Popen, STDOUT

class Definition:
  pass

class CxxHandler(BaseHandler):
  def __init__(self, **kwargs: Any) -> None:
    super().__init__(handler='cxx', **kwargs)

    # Run doxygen.
    cmd = ['doxygen', '-']
    doc_dir = os.path.dirname(os.path.dirname(os.path.dirname(__file__)))
    include_dir = os.path.join(os.path.dirname(doc_dir), 'include', 'fmt')
    doxyxml_dir = 'doxyxml'
    p = Popen(cmd, stdin=PIPE, stdout=PIPE, stderr=STDOUT)
    out, _ = p.communicate(input=r'''
        PROJECT_NAME      = fmt
        GENERATE_LATEX    = NO
        GENERATE_MAN      = NO
        GENERATE_RTF      = NO
        CASE_SENSE_NAMES  = NO
        INPUT             = {0}/args.h {0}/base.h {0}/chrono.h {0}/color.h \
                            {0}/core.h {0}/compile.h {0}/format.h {0}/os.h \
                            {0}/ostream.h {0}/printf.h {0}/ranges.h {0}/xchar.h
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
                            __linux__=1 \
                            FMT_ENABLE_IF(...)= \
                            FMT_USE_VARIADIC_TEMPLATES=1 \
                            FMT_USE_RVALUE_REFERENCES=1 \
                            FMT_USE_USER_DEFINED_LITERALS=1 \
                            FMT_USE_ALIAS_TEMPLATES=1 \
                            FMT_USE_NONTYPE_TEMPLATE_ARGS=1 \
                            FMT_API= \
                            "FMT_BEGIN_NAMESPACE=namespace fmt {{" \
                            "FMT_END_NAMESPACE=}}" \
                            "FMT_STRING_ALIAS=1" \
                            "FMT_VARIADIC(...)=" \
                            "FMT_VARIADIC_W(...)=" \
                            "FMT_DOC=1"
        EXCLUDE_SYMBOLS   = fmt::formatter fmt::printf_formatter fmt::arg_join \
                            fmt::basic_format_arg::handle
        '''.format(include_dir, doxyxml_dir).encode('utf-8'))
    if p.returncode != 0:
        raise CalledProcessError(p.returncode, cmd)

    # Load XML.
    with open(os.path.join(doxyxml_dir, 'namespacefmt.xml')) as f:
      self._doxyxml = et.parse(f)

  def collect(self, identifier: str,
              config: Mapping[str, Any]) -> list[Definition]:
    paren = identifier.find('(')
    args = None
    if paren > 0:
      identifier, args = identifier[:paren], identifier[paren:]
      
    nodes = self._doxyxml.findall(
      f"./compounddef/sectiondef/memberdef/name[.='{identifier}']/..")
    defs = []
    for n in nodes:
      node_args = n.find('argsstring').text
      if args != None and args != node_args:
        continue
      d = Definition()
      d.type = n.find('type').text
      d.name = identifier
      d.args = node_args
      desc = n.find('detaileddescription/para/verbatim')
      d.desc = desc.text if desc != None else ''
      defs.append(d)
    return defs

  def render(self, defs: list[Definition], config: dict) -> str:
    text = ''
    for d in defs:
      text += '<pre><code>'
      text += d.type + ' ' + d.name + d.args
      text += '</code></pre>\n'
      text += d.desc
    return text

def get_handler(theme: str, custom_templates: Optional[str] = None,
                **config: Any) -> CxxHandler:
  '''Return an instance of `CxxHandler`.

  Arguments:
    theme: The theme to use when rendering contents.
    custom_templates: Directory containing custom templates.
    **config: Configuration passed to the handler.
  '''
  return CxxHandler(theme=theme, custom_templates=custom_templates)
