# A basic mkdocstrings handler for {fmt}.
# Copyright (c) 2012 - present, Victor Zverovich
# https://github.com/fmtlib/fmt/blob/master/LICENSE

# pyright: strict

import os
import xml.etree.ElementTree as ET
from pathlib import Path
from subprocess import PIPE, STDOUT, CalledProcessError, Popen

from markupsafe import Markup
from mkdocstrings import BaseHandler
from typing_extensions import TYPE_CHECKING, Any, ClassVar, final, override

if TYPE_CHECKING:
    from collections.abc import Mapping, MutableMapping

    from mkdocs.config.defaults import MkDocsConfig
    from mkdocstrings import CollectorItem, HandlerOptions


@final
class Definition:
    """A definition extracted by Doxygen."""

    def __init__(
        self,
        name: str,
        kind: "str | None" = None,
        node: "ET.Element | None" = None,
        is_member: bool = False,
    ):
        self.name = name
        self.kind: "str | None" = None
        if kind is not None:
            self.kind = kind
        elif node is not None:
            self.kind = node.get("kind")
        self.desc: "list[ET.Element[str]] | None" = None
        self.id: "str | None" = name if not is_member else None
        self.members: "list[Definition] | None" = None
        self.params: "list[Definition] | None" = None
        self.template_params: "list[Definition] | None" = None
        self.trailing_return_type: "str | None" = None
        self.type: "str | None" = None


# A map from Doxygen to HTML tags.
tag_map = {
    "bold": "b",
    "emphasis": "em",
    "computeroutput": "code",
    "para": "p",
    "itemizedlist": "ul",
    "listitem": "li",
}

# A map from Doxygen tags to text.
tag_text_map = {"codeline": "", "highlight": "", "sp": " "}


def escape_html(s: str) -> str:
    return s.replace("<", "&lt;")


# Converts a node from doxygen to HTML format.
def convert_node(
    node: ET.Element, tag: str, attrs: "Mapping[str, str] | None" = None
) -> str:
    if attrs is None:
        attrs = {}

    out: str = "<" + tag
    for key, value in attrs.items():
        out += " " + key + '="' + value + '"'
    out += ">"
    if node.text:
        out += escape_html(node.text)
    out += doxyxml2html(list(node))
    out += "</" + tag + ">"
    if node.tail:
        out += node.tail
    return out


def doxyxml2html(nodes: "list[ET.Element]"):
    out = ""
    for n in nodes:
        tag = tag_map.get(n.tag)
        if tag:
            out += convert_node(n, tag)
            continue
        if n.tag == "programlisting" or n.tag == "verbatim":
            out += "<pre>"
            out += convert_node(n, "code", {"class": "language-cpp"})
            out += "</pre>"
            continue
        if n.tag == "ulink":
            out += convert_node(n, "a", {"href": n.attrib["url"]})
            continue
        out += tag_text_map[n.tag]
    return out


def convert_template_params(node: ET.Element) -> "list[Definition] | None":
    template_param_list = node.find("templateparamlist")
    if template_param_list is None:
        return None
    params: "list[Definition]" = []
    for param_node in template_param_list.findall("param"):
        name = param_node.find("declname")
        if name is not None:
            name = name.text
        if name is None:
            name = ""
        param = Definition(name, "param")
        param_type = param_node.find("type")
        if param_type is not None:
            param.type = param_type.text
        params.append(param)
    return params


def get_description(node: ET.Element) -> list[ET.Element]:
    return node.findall("briefdescription/para") + node.findall(
        "detaileddescription/para"
    )


def normalize_type(type_: str) -> str:
    type_ = type_.replace("< ", "<").replace(" >", ">")
    return type_.replace(" &", "&").replace(" *", "*")


def convert_type(type_: "ET.Element | None") -> "str | None":
    if type_ is None:
        return None
    result = type_.text if type_.text else ""
    for ref in type_:
        if ref.text is None:
            raise ValueError
        result += ref.text
        if ref.tail:
            result += ref.tail
    if type_.tail is None:
        raise ValueError
    result += type_.tail.strip()
    return normalize_type(result)


def convert_params(func: ET.Element) -> list[Definition]:
    params: "list[Definition]" = []
    for p in func.findall("param"):
        declname = p.find("declname")
        if declname is None or declname.text is None:
            raise ValueError
        d = Definition(declname.text, "param")
        d.type = convert_type(p.find("type"))
        params.append(d)
    return params


def convert_return_type(d: Definition, node: ET.Element) -> None:
    d.trailing_return_type = None
    if d.type == "auto" or d.type == "constexpr auto":
        argsstring = node.find("argsstring")
        if argsstring is None or argsstring.text is None:
            raise ValueError
        parts = argsstring.text.split(" -> ")
        if len(parts) > 1:
            d.trailing_return_type = normalize_type(parts[1])


def render_param(param: Definition) -> str:
    if param.type is None:
        raise ValueError
    return param.type + (f"&nbsp;{param.name}" if len(param.name) > 0 else "")


def render_decl(d: Definition) -> str:
    text = ""
    if d.id is not None:
        text += f'<a id="{d.id}">\n'
    text += '<pre><code class="language-cpp decl">'

    text += "<div>"
    if d.template_params is not None:
        text += "template &lt;"
        text += ", ".join([render_param(p) for p in d.template_params])
        text += "&gt;\n"
    text += "</div>"

    text += "<div>"
    end = ";"
    if d.kind is None:
        raise ValueError
    if d.kind == "function" or d.kind == "variable":
        if d.type is None:
            raise ValueError
        text += d.type + " " if len(d.type) > 0 else ""
    elif d.kind == "typedef":
        text += "using "
    elif d.kind == "define":
        end = ""
    else:
        text += d.kind + " "
    text += d.name

    if d.params is not None:
        params = ", ".join([
            (p.type + " " if p.type else "") + p.name for p in d.params
        ])
        text += "(" + escape_html(params) + ")"
        if d.trailing_return_type:
            text += " -&NoBreak;>&nbsp;" + escape_html(d.trailing_return_type)
    elif d.kind == "typedef":
        if d.type is None:
            raise ValueError
        text += " = " + escape_html(d.type)

    text += end
    text += "</div>"
    text += "</code></pre>\n"
    if d.id is not None:
        text += "</a>\n"
    return text


@final
class CxxHandler(BaseHandler):
    name: ClassVar[str] = "cxx"

    domain: ClassVar[str] = "cxx"

    def __init__(
        self, config: "Mapping[str, Any]", base_dir: Path, **kwargs: Any
    ) -> None:
        super().__init__(**kwargs)

        self.config = config
        """The handler configuration."""
        self.base_dir = base_dir
        """The base directory of the project."""

        headers = [
            "args.h",
            "base.h",
            "chrono.h",
            "color.h",
            "compile.h",
            "format.h",
            "os.h",
            "ostream.h",
            "printf.h",
            "ranges.h",
            "std.h",
            "xchar.h",
        ]

        # Run doxygen.
        cmd = ["doxygen", "-"]
        support_dir = Path(__file__).parents[3]
        top_dir = os.path.dirname(support_dir)
        include_dir = os.path.join(top_dir, "include", "fmt")
        self._ns2doxyxml: "dict[str, ET.ElementTree[ET.Element[str]]]" = {}
        build_dir = os.path.join(top_dir, "build")
        os.makedirs(build_dir, exist_ok=True)
        self._doxyxml_dir = os.path.join(build_dir, "doxyxml")
        p = Popen(cmd, stdin=PIPE, stdout=PIPE, stderr=STDOUT)
        _, _ = p.communicate(
            input=r"""
            PROJECT_NAME     = fmt
            GENERATE_XML     = YES
            GENERATE_LATEX   = NO
            GENERATE_HTML    = NO
            INPUT            = {0}
            XML_OUTPUT       = {1}
            QUIET            = YES
            AUTOLINK_SUPPORT = NO
            MACRO_EXPANSION  = YES
            PREDEFINED       = _WIN32=1 \
                               __linux__=1 \
                               FMT_ENABLE_IF(...)= \
                               FMT_USE_USER_LITERALS=1 \
                               FMT_USE_ALIAS_TEMPLATES=1 \
                               FMT_USE_NONTYPE_TEMPLATE_ARGS=1 \
                               FMT_API= \
                               "FMT_BEGIN_NAMESPACE=namespace fmt {{" \
                               "FMT_END_NAMESPACE=}}" \
                               "FMT_DOC=1"
            """.format(
                " ".join([os.path.join(include_dir, h) for h in headers]),
                self._doxyxml_dir,
            ).encode("utf-8")
        )
        if p.returncode != 0:
            raise CalledProcessError(p.returncode, cmd)

        # Merge all file-level XMLs into one to simplify search.
        self._file_doxyxml: "ET.ElementTree[ET.Element[str]] | None" = None
        for h in headers:
            filename = h.replace(".h", "_8h.xml")
            with open(os.path.join(self._doxyxml_dir, filename)) as f:
                doxyxml = ET.parse(f)
                if self._file_doxyxml is None:
                    self._file_doxyxml = doxyxml
                    continue
                root = self._file_doxyxml.getroot()
                for node in doxyxml.getroot():
                    root.append(node)

    def collect_compound(self, identifier: str, cls: "list[ET.Element]") -> Definition:
        """Collect a compound definition such as a struct."""
        refid = cls[0].get("refid")
        if refid is None:
            raise ValueError
        path = os.path.join(self._doxyxml_dir, refid + ".xml")
        with open(path) as f:
            xml = ET.parse(f)
            node = xml.find("compounddef")
            if node is None:
                raise ValueError
            d = Definition(identifier, node=node)
            d.template_params = convert_template_params(node)
            d.desc = get_description(node)
            d.members = []
            for m in node.findall(
                'sectiondef[@kind="public-attrib"]/memberdef'
            ) + node.findall('sectiondef[@kind="public-func"]/memberdef'):
                name = m.find("name")
                if name is None or name.text is None:
                    raise ValueError
                name = name.text
                # Doxygen incorrectly classifies members of private unnamed unions as
                # public members of the containing class.
                if name.endswith("_"):
                    continue
                desc = get_description(m)
                if len(desc) == 0:
                    continue
                kind = m.get("kind")
                member = Definition(name if name else "", kind=kind, is_member=True)
                type_ = m.find("type")
                if type_ is None:
                    raise ValueError
                type_text = type_.text
                member.type = type_text if type_text else ""
                if kind == "function":
                    member.params = convert_params(m)
                    convert_return_type(member, m)
                member.template_params = None
                member.desc = desc
                d.members.append(member)
            return d

    @override
    def collect(self, identifier: str, options: "Mapping[str, Any]") -> Definition:
        qual_name = "fmt::" + identifier

        param_str = None
        paren = qual_name.find("(")
        if paren > 0:
            qual_name, param_str = qual_name[:paren], qual_name[paren + 1 : -1]

        colons = qual_name.rfind("::")
        namespace, name = qual_name[:colons], qual_name[colons + 2 :]

        # Load XML.
        doxyxml = self._ns2doxyxml.get(namespace)
        if doxyxml is None:
            path = f"namespace{namespace.replace('::', '_1_1')}.xml"
            with open(os.path.join(self._doxyxml_dir, path)) as f:
                doxyxml = ET.parse(f)
                self._ns2doxyxml[namespace] = doxyxml

        nodes = doxyxml.findall(f"compounddef/sectiondef/memberdef/name[.='{name}']/..")
        if len(nodes) == 0:
            if self._file_doxyxml is None:
                raise ValueError
            nodes = self._file_doxyxml.findall(
                f"compounddef/sectiondef/memberdef/name[.='{name}']/.."
            )
        candidates: "list[str]" = []
        for node in nodes:
            # Process a function or a typedef.
            params: "list[Definition] | None" = None
            d = Definition(name, node=node)
            if d.kind == "function":
                params = convert_params(node)
                params_type: "list[str]" = []
                for p in params:
                    if p.type is None:
                        raise ValueError
                    else:
                        params_type.append(p.type)
                node_param_str = ", ".join(params_type)
                if param_str and param_str != node_param_str:
                    candidates.append(f"{name}({node_param_str})")
                    continue
            elif d.kind == "define":
                params = []
                for p in node.findall("param"):
                    defname = p.find("defname")
                    if defname is None or defname.text is None:
                        raise ValueError
                    param = Definition(defname.text, kind="param")
                    param.type = None
                    params.append(param)
            d.type = convert_type(node.find("type"))
            d.template_params = convert_template_params(node)
            d.params = params
            convert_return_type(d, node)
            d.desc = get_description(node)
            return d

        cls = doxyxml.findall(f"compounddef/innerclass[.='{qual_name}']")
        if not cls:
            raise Exception(f"Cannot find {identifier}. Candidates: {candidates}")
        return self.collect_compound(identifier, cls)

    @override
    def render(
        self,
        data: "CollectorItem",
        options: "HandlerOptions",
        *,
        locale: "str | None" = None,
    ) -> str:
        d = data
        if d.id is not None:
            _ = self.do_heading(Markup(), 0, id=d.id)
        if d.desc is None:
            raise ValueError
        text = '<div class="docblock">\n'
        text += render_decl(d)
        text += '<div class="docblock-desc">\n'
        text += doxyxml2html(d.desc)
        if d.members is not None:
            for m in d.members:
                text += self.render(m, options, locale=locale)
        text += "</div>\n"
        text += "</div>\n"
        return text


def get_handler(
    handler_config: "MutableMapping[str, Any]", tool_config: "MkDocsConfig", **kwargs: Any
) -> CxxHandler:
    """Return an instance of `CxxHandler`.

    Arguments:
        handler_config: The handler configuration.
        tool_config: The tool (SSG) configuration.
    """
    base_dir = Path(tool_config.config_file_path or "./mkdocs.yml").parent
    return CxxHandler(config=handler_config, base_dir=base_dir, **kwargs)
