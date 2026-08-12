# Copyright (c) 2026 Matt Borgerson
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import unittest
import tempfile
import os
import sys
import yaml

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from gen_config import ConfigNode, Config


class TestConfigNode(unittest.TestCase):
    def test_shorthand_types(self):
        node_bool = ConfigNode('b', 'bool')
        self.assertEqual(node_bool.type, 'boolean')
        self.assertFalse(node_bool.value)

        node_int = ConfigNode('i', 'int')
        self.assertEqual(node_int.type, 'integer')
        self.assertEqual(node_int.value, 0)

        node_float = ConfigNode('f', 'float')
        self.assertEqual(node_float.type, 'number')
        self.assertEqual(node_float.value, 0.0)

        node_str = ConfigNode('s', 'str')
        self.assertEqual(node_str.type, 'string')
        self.assertEqual(node_str.value, '')

    def test_invalid_type(self):
        with self.assertRaises(AssertionError):
            ConfigNode('bad', {'type': 'unknown_type'})

    def test_enum_default(self):
        node = ConfigNode('e', {'type': 'enum', 'values': ['opt1', 'opt2', 'opt3']})
        self.assertEqual(node.value, 'opt1')

        node_with_def = ConfigNode('e', {'type': 'enum', 'values': ['opt1', 'opt2'], 'default': 'opt2'})
        self.assertEqual(node_with_def.value, 'opt2')

    def test_table_node(self):
        node = ConfigNode('settings', {
            'type': 'table',
            'port': {'type': 'int', 'default': 8080},
            'enabled': {'type': 'bool', 'default': True}
        })
        self.assertEqual(node.type, 'table')
        self.assertEqual(len(node.children), 2)
        child_names = [c.name for c in node.children]
        self.assertIn('port', child_names)
        self.assertIn('enabled', child_names)

    def test_array_node(self):
        node = ConfigNode('items', {
            'type': 'array',
            'items': 'string'
        })
        self.assertEqual(node.type, 'array')
        self.assertIsNotNone(node.array_item_type)
        self.assertEqual(node.array_item_type.type, 'string')


class TestConfigGeneration(unittest.TestCase):
    def setUp(self):
        self.sample_yaml = """
server:
  host:
    type: string
    default: "localhost"
  port:
    type: int
    default: 8080
  debug:
    type: bool
    default: false
  mode:
    type: enum
    values: ['dev', 'prod', 'staging']
    default: 'dev'
  tags:
    type: array
    items: string
"""

    def test_code_generation(self):
        with tempfile.NamedTemporaryFile('w+', delete=False, suffix='.yml') as f:
            f.write(self.sample_yaml)
            f.flush()
            tmp_path = f.name

        try:
            config = Config(tmp_path, 'config')
            c_code = config.gen_c_file()
            self.assertIn('enum CONFIG_SERVER_MODE_ {', c_code)
            self.assertIn('CONFIG_SERVER_MODE_DEV = 0,', c_code)
            self.assertIn('CONFIG_SERVER_MODE_PROD = 1,', c_code)
            self.assertIn('struct config {', c_code)
            self.assertIn('const char *host;', c_code)
            self.assertIn('int port;', c_code)
            self.assertIn('bool debug;', c_code)
            self.assertIn('const char **tags;', c_code)
            self.assertIn('unsigned int tags_count;', c_code)

            cnode_code = config.gen_cnode_def_file()
            self.assertIn('CNode config_tree =', cnode_code)
            self.assertIn('ctab("config", {', cnode_code)
            self.assertIn('cstring(', cnode_code)
            self.assertIn('cinteger(', cnode_code)
            self.assertIn('cbool(', cnode_code)
            self.assertIn('cenum(', cnode_code)
            self.assertIn('carray(', cnode_code)
        finally:
            os.unlink(tmp_path)


if __name__ == '__main__':
    unittest.main()
