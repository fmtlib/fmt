"""XML-loading regressions for the documentation handler.

Run with the documentation dependencies installed:
python -m unittest discover -s support/python/tests
"""

import builtins
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path
from unittest.mock import Mock, patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from mkdocstrings_handlers import cxx


def locale_open(file, mode="r", **kwargs):
    if "b" not in mode and "encoding" not in kwargs:
        kwargs["encoding"] = "cp1252"
    return builtins.open(file, mode, **kwargs)


class XmlEncodingTest(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)
        self.xml_dir = self.root / "build" / "doxyxml"
        self.xml_dir.mkdir(parents=True)
        self.description = "\u5341\u4e8c"
        self.handler = object.__new__(cxx.CxxHandler)
        self.handler._doxyxml_dir = str(self.xml_dir)
        self.handler._ns2doxyxml = {}
        self.handler._file_doxyxml = None
        opener = patch.object(cxx, "open", locale_open, create=True)
        opener.start()
        self.addCleanup(opener.stop)

    def write_xml(self, filename, body):
        xml = '<?xml version="1.0" encoding="UTF-8"?><doxygen>'
        xml += body + "</doxygen>"
        (self.xml_dir / filename).write_bytes(xml.encode("utf-8"))

    def compound(self):
        return (
            '<compounddef kind="struct"><briefdescription><para>'
            + self.description
            + "</para></briefdescription></compounddef>"
        )

    def test_header_xml(self):
        headers = (
            "args",
            "base",
            "chrono",
            "color",
            "compile",
            "enum",
            "format",
            "os",
            "ostream",
            "printf",
            "ranges",
            "std",
            "xchar",
        )
        for header in headers:
            self.write_xml(header + "_8h.xml", self.compound())
        module_path = self.root / "support/python/mkdocstrings_handlers/cxx/__init__.py"
        process = Mock(returncode=0)
        process.communicate.return_value = (b"", None)
        with (
            patch.object(cxx, "__file__", str(module_path)),
            patch.object(cxx.BaseHandler, "__init__", return_value=None),
            patch.object(cxx, "Popen", return_value=process),
        ):
            handler = cxx.CxxHandler({}, self.root)
        paragraphs = handler._file_doxyxml.findall("compounddef/briefdescription/para")
        self.assertEqual([p.text for p in paragraphs], [self.description] * len(headers))

    def test_compound_xml(self):
        self.write_xml("struct_example.xml", self.compound())
        reference = ET.Element("innerclass", refid="struct_example")
        result = self.handler.collect_compound("example", [reference])
        self.assertEqual(result.desc[0].text, self.description)

    def test_namespace_xml(self):
        self.write_xml(
            "namespacefmt.xml",
            '<compounddef><sectiondef><memberdef kind="variable">'
            "<type>int</type>\n<name>example</name><briefdescription><para>"
            + self.description
            + "</para></briefdescription></memberdef></sectiondef></compounddef>",
        )
        result = self.handler.collect("example", {})
        self.assertEqual(result.desc[0].text, self.description)


if __name__ == "__main__":
    unittest.main()
