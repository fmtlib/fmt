# A basic mkdocstrings handler for {fmt}.
# Copyright (c) 2012 - present, Victor Zverovich
# https://github.com/fmtlib/fmt/blob/master/LICENSE

# pyright: strict

import colorsys
import os
import re
from html import escape
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
        self.enum_values: "list[tuple[str, str, list[ET.Element[str]]]] | None" = None
        self.color_values: "list[tuple[str, str, list[ET.Element[str]]]] | None" = None
        self.qualifiers = ""
        self.default = ""


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
        name = declname.text if declname is not None and declname.text else ""
        d = Definition(name, "param")
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


def display_type(type_: str, color_reference: bool) -> str:
    if color_reference:
        if type_.startswith("detail::styled_arg<"):
            type_ = "unspecified-type"
        type_ = type_.replace("detail::color_type", "unspecified-type")
    text = escape_html(type_)
    return text.replace("unspecified-type", "<em>unspecified-type</em>")


def render_decl(d: Definition, color_reference: bool = False) -> str:
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
        text += display_type(d.type, color_reference) + " " if len(d.type) > 0 else ""
    elif d.kind == "enum" and color_reference:
        text += "enum class "
    elif d.kind == "typedef":
        text += "using "
    elif d.kind == "define":
        end = ""
    else:
        text += d.kind + " "
    text += d.name

    if d.params is not None:
        params = ", ".join([
            (p.type + " " if p.type else "") + p.name
            + (" = " + p.default if color_reference and p.default else "")
            for p in d.params
        ])
        text += "(" + display_type(params, color_reference) + ")"
        if color_reference:
            text += d.qualifiers
        if d.trailing_return_type:
            text += " -&NoBreak;>&nbsp;" + display_type(d.trailing_return_type, color_reference)
    elif d.kind == "enum" and color_reference:
        if d.type:
            text += " : " + escape_html(d.type)
        if d.enum_values is not None:
            text += " {\n"
            for name, initializer, _ in d.enum_values:
                text += "  " + name
                if initializer:
                    text += " " + escape_html(initializer)
                text += ",\n"
            text += "}"
    elif d.kind == "namespace" and color_reference:
        text += " { /* named RGB constants */ }"
        end = ""
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


def swatch_luminance(packed: int) -> float:
    rgb = tuple(((packed >> shift) & 255) / 255 for shift in (16, 8, 0))
    linear = tuple(c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4
                   for c in rgb)
    return sum(c * w for c, w in zip(linear, (0.2126, 0.7152, 0.0722)))


def swatch_foreground(luminance: float) -> str:
    return "#000000" if (luminance + 0.05) / 0.05 >= 1.05 / (luminance + 0.05) else "#FFFFFF"


def render_cube_palette() -> str:
    """Render red rows, green columns, and six three-character blue fields."""
    levels = (0, 95, 135, 175, 215, 255)
    text = '<div class="cube-palette-scroll"><table class="cube-palette">'
    text += '<thead><tr><th></th><th colspan="6">Green index →</th></tr>'
    text += '<tr><th scope="col">Red ↓</th>'
    text += ''.join(f'<th scope="col">{green}</th>' for green in range(6))
    text += '</tr></thead><tbody>'
    for red in range(6):
        text += f'<tr><th scope="row">{red}</th>'
        for green in range(6):
            text += '<td>'
            for blue in range(6):
                packed = (levels[red] << 16) | (levels[green] << 8) | levels[blue]
                background = f'#{packed:06X}'
                label = (f'cube_color{{{red}, {green}, {blue}}}: '
                         f'rgb{{{levels[red]}, {levels[green]}, {levels[blue]}}} '
                         f'({background}); palette index {16 + 36 * red + 6 * green + blue}')
                text += (f'<span class="cube-index" '
                         f'style="background-color:{background};color:{swatch_foreground(swatch_luminance(packed))}" '
                         f'title="{label}" aria-label="{label}"> {blue} </span>')
            text += '</td>'
        text += '</tr>'
    return text + '</tbody></table></div>'


def render_color_swatches(
    values: "list[tuple[str, str, list[ET.Element[str]]]]", group: str
) -> str:
    """Render hue groups by lightness, or terminal RGB constants in palette order."""
    groups = ("red", "yellow", "green", "cyan", "blue", "magenta")
    if group not in groups + ("neutral", "terminal", "terminal-normal", "terminal-bright"):
        raise ValueError(f"Unknown color group: {group}")
    swatches: "list[tuple[float, float, str, str]]" = []
    for index, (name, initializer, _) in enumerate(values):
        packed = int(initializer.removeprefix("=").strip(), 0)
        rgb = tuple(((packed >> shift) & 255) / 255 for shift in (16, 8, 0))
        hue, saturation, _ = colorsys.rgb_to_hsv(*rgb)
        category = "neutral" if saturation == 0 else groups[int((hue * 6 + 0.5) % 6)]
        if name.startswith("term_"):
            category = "terminal-bright" if name.startswith("term_bright_") else "terminal-normal"
        if category != group and not (group == "terminal" and category.startswith("terminal-")):
            continue
        luminance = swatch_luminance(packed)
        foreground = swatch_foreground(luminance)
        background = f"#{packed:06X}"
        tile = (f'<li id="named-color-{escape(name)}" class="color-swatch" '
                f'style="background-color:{background};color:{foreground}" '
                f'title="{escape(name)}: {background}">{escape(name)}</li>')
        order = (0.0, float(index)) if group.startswith("terminal") else (
            (hue + 1 / 12) % 1, luminance)
        swatches.append((*order, name, tile))
    swatches.sort()
    return '<ul class="color-swatches">' + "".join(tile for _, _, _, tile in swatches) + '</ul>'


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
            "chrono.h",
            "color-ostream.h",
            "color.h",
            "compile.h",
            "core.h",
            "enum.h",
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
            doxyxml = ET.parse(os.path.join(self._doxyxml_dir, filename))
            if self._file_doxyxml is None:
                self._file_doxyxml = doxyxml
                continue
            root = self._file_doxyxml.getroot()
            for node in doxyxml.getroot():
                root.append(node)

    def collect_compound(self, identifier: str, cls: "list[ET.Element]",
                         color_reference: bool = False) -> Definition:
        """Collect a compound definition such as a struct."""
        refid = cls[0].get("refid")
        if refid is None:
            raise ValueError
        path = os.path.join(self._doxyxml_dir, refid + ".xml")
        xml = ET.parse(path)
        node = xml.find("compounddef")
        if node is None:
            raise ValueError
        d = Definition(identifier, node=node)
        d.template_params = convert_template_params(node)
        d.desc = get_description(node)
        d.members = []
        if d.kind == "namespace" and color_reference:
            d.color_values = []
            for m in node.findall('sectiondef[@kind="var"]/memberdef'):
                initializer = m.findtext("initializer", "")
                components = re.fullmatch(r"\{\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\}",
                                          initializer.strip())
                if components is None:
                    raise ValueError(f"Invalid named RGB initializer: {initializer}")
                red, green, blue = (int(c) for c in components.groups())
                packed = (red << 16) | (green << 8) | blue
                d.color_values.append((m.findtext("name", ""), f"= 0x{packed:06X}",
                                       get_description(m)))
            return d
        members = node.findall(
            'sectiondef[@kind="public-attrib"]/memberdef'
        ) + node.findall('sectiondef[@kind="public-func"]/memberdef')
        if color_reference:
            members += node.findall('sectiondef[@kind="friend"]/memberdef')
        for m in members:
            if m.get("prot") == "private":
                continue
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
            if kind == "friend":
                kind = "function"
            member = Definition(name if name else "", kind=kind, is_member=True)
            type_ = m.find("type")
            if type_ is None:
                raise ValueError
            type_text = type_.text
            member.type = type_text if type_text else ""
            if color_reference:
                member.type = normalize_type("".join(type_.itertext()))
                if m.get("explicit") == "yes":
                    member.type = "explicit " + member.type
            if kind == "function":
                member.params = convert_params(m)
                for param, source in zip(member.params, m.findall("param")):
                    default = source.find("defval")
                    if default is not None:
                        param.default = "".join(default.itertext())
                convert_return_type(member, m)
                if m.get("const") == "yes":
                    member.qualifiers += " const"
                if m.get("noexcept") == "yes":
                    member.qualifiers += " noexcept"
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
            doxyxml = ET.parse(os.path.join(self._doxyxml_dir, path))
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
            if options.get("color_reference"):
                d.id = "color-" + re.sub(r"[^a-z0-9]+", "-", identifier.lower()).strip("-")
                if node.get("noexcept") == "yes":
                    d.qualifiers = " noexcept"
                if d.kind == "enum":
                    placeholder = options.get("enum_value_placeholder")
                    d.enum_values = [
                        (v.findtext("name", ""),
                         "= " + str(placeholder)
                         if placeholder is not None and v.find("initializer") is not None else
                         ("".join(v.find("initializer").itertext())
                          if v.find("initializer") is not None else ""),
                         get_description(v))
                        for v in node.findall("enumvalue")
                    ]
                    if not options.get("show_enum_members", True):
                        d.enum_values = None
            return d

        cls = doxyxml.findall(f"compounddef/innerclass[.='{qual_name}']")
        if not cls and options.get("color_reference"):
            cls = doxyxml.findall(f"compounddef/innernamespace[.='{qual_name}']")
        if not cls:
            raise Exception(f"Cannot find {identifier}. Candidates: {candidates}")
        return self.collect_compound(identifier, cls, bool(options.get("color_reference")))

    @override
    def render(
        self,
        data: "CollectorItem",
        options: "HandlerOptions",
        *,
        locale: "str | None" = None,
    ) -> str:
        d = data
        swatch_group = options.get("color_swatch_group")
        if swatch_group is not None:
            if d.name == "cube_color" and swatch_group == "cube":
                return render_cube_palette()
            if d.name == "gray_color" and swatch_group == "gray":
                values: "list[tuple[str, str, list[ET.Element[str]]]]" = [
                    (f"gray_color{{{index}}}",
                     f"= 0x{(8 + 10 * index) * 0x010101:06X}", [])
                    for index in range(24)]
                return render_color_swatches(values, "neutral")
            if d.color_values is None:
                raise ValueError("Color swatches require named RGB constants")
            return render_color_swatches(d.color_values, str(swatch_group))
        if d.id is not None:
            _ = self.do_heading(Markup(), 0, id=d.id)
        if d.desc is None:
            raise ValueError
        text = '<div class="docblock">\n'
        color_reference = bool(options.get("color_reference"))
        text += render_decl(d, color_reference)
        text += '<div class="docblock-desc">\n'
        text += doxyxml2html(d.desc)
        if color_reference and d.enum_values:
            descriptions = [(name, desc) for name, _, desc in d.enum_values if desc]
            if descriptions:
                emphasis = d.name == "emphasis"
                text += "<table><thead><tr><th>Attribute</th><th>Description</th>"
                if emphasis:
                    text += "<th>SGR</th>"
                text += "</tr></thead><tbody>"
                for name, desc in descriptions:
                    text += "<tr><td><code>" + name + "</code></td><td>"
                    description = doxyxml2html(desc)
                    sgr = re.search(r" \(SGR (\d+)\)", description) if emphasis else None
                    if sgr:
                        description = description[:sgr.start()] + description[sgr.end():]
                    text += description + "</td>"
                    if emphasis:
                        text += "<td>" + (sgr.group(1) if sgr else "") + "</td>"
                    text += "</tr>"
                text += "</tbody></table>"
        if d.members is not None:
            for m in d.members:
                if m.name in options.get("exclude_members", []):
                    continue
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
