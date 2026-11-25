#!/usr/bin/env python3
"""
Unit tests for formatdocs: parsing the converter's description, merging
blocks, and updating the manual without losing an author's text.

Run with: python3 -m unittest discover -s tools -p 'test_*.py'
"""

import collections
import unittest

import formatdocs


# A small description in the form `ccres --describe` writes: two objects
# sharing a header field and a table, one with a limit field, and a nested
# block with two forms of one field.
DESCRIBE = '''RESF:1.01

first_object {
  header_flags:
  #@ values 1 kind=flags
  #@ value 1 0x1 HEADER_ONE
  #@ field header_flags: type=flags bits=32 values=1
  colour:
  #@ values 2 kind=enum
  #@ value 2 0x0 COLOUR_WHITE
  #@ value 2 0x7 COLOUR_BLACK
  #@ field colour: type=wimp-colour values=2
  title:""
  title_limit:*
  #@ field title: type=message limit=title_limit:
  item {
    #@ variant text-item when=not-ITEM_IS_SPRITE
    text:""
    #@ field text: type=message
  }
  item {
    #@ variant sprite-item when=ITEM_IS_SPRITE
    text:""
    #@ field text: type=string
    sprite_area:&1
    #@ field sprite_area: type=sprite-area
  }
}

second_object {
  header_flags:
  #@ field header_flags: type=flags bits=32 values=1
  shade:
  #@ field shade: type=wimp-colour values=2
  name:"x"
  #@ field name: type=fixed-string size=11 terminated
}
'''

# A manual with an empty region for the description above
EMPTY_DOC = '''<riscos-prm>
<p>Before.</p>
<!-- ccres-region: resource -->
<!-- ccres-region-end: resource -->
<p>After.</p>
</riscos-prm>
'''


def generate(doc, describe=DESCRIBE):
    """
    Update a manual's resource region from a description.
    """
    sections = formatdocs.build_sections(formatdocs.parse_describe(describe))
    return formatdocs.update_document(doc, collections.OrderedDict([('resource', sections)]))


def section(doc, title):
    """
    Give the text of one generated subsubsection of a manual.
    """
    start = doc.index('<subsubsection title="{}">'.format(title))
    return doc[start:doc.index('</subsubsection>', start)]


class TestParse(unittest.TestCase):

    def test_blocks_fields_and_tables(self):
        description = formatdocs.parse_describe(DESCRIBE)
        self.assertEqual(description.header, 'RESF:1.01')
        self.assertEqual([instance.name for instance in description.instances], ['first_object', 'second_object'])
        first = description.instances[0]
        self.assertEqual([field.name for field in first.fields], ['header_flags:', 'colour:', 'title:'])
        self.assertEqual(first.fields[2].attrs, {'limit': 'title_limit:'})
        self.assertEqual([child.name for child in first.children], ['item', 'item'])
        self.assertEqual(description.tables[2].values, [(0, 'COLOUR_WHITE'), (7, 'COLOUR_BLACK')])

    def test_variant_applies_to_following_fields(self):
        description = formatdocs.parse_describe(DESCRIBE)
        sprite = description.instances[0].children[1]
        self.assertEqual([field.variant for field in sprite.fields],
                         [('sprite-item', 'ITEM_IS_SPRITE')] * 2)

    def test_terminated_word(self):
        description = formatdocs.parse_describe(DESCRIBE)
        name = description.instances[1].fields[2]
        self.assertEqual(name.attrs, {'size': '11'})
        self.assertEqual(name.words, ['terminated'])

    def test_errors_are_reported(self):
        with self.assertRaisesRegex(formatdocs.DescribeError, 'ends inside block'):
            formatdocs.parse_describe('RESF:1.01\nfirst_object {\n')
        with self.assertRaisesRegex(formatdocs.DescribeError, 'unbalanced'):
            formatdocs.parse_describe('RESF:1.01\n}\n')
        with self.assertRaisesRegex(formatdocs.DescribeError, 'unknown annotation'):
            formatdocs.parse_describe('RESF:1.01\nfirst_object {\n#@ bogus\n}\n')
        with self.assertRaisesRegex(formatdocs.DescribeError, 'unknown table'):
            formatdocs.parse_describe('RESF:1.01\n#@ value 9 0x1 X\n')
        with self.assertRaisesRegex(formatdocs.DescribeError, 'empty'):
            formatdocs.parse_describe('')


class TestMerge(unittest.TestCase):

    def setUp(self):
        self.blocks = formatdocs.merge_blocks(formatdocs.parse_describe(DESCRIBE))

    def test_limit_field_follows_its_string(self):
        names = self.blocks['first_object'].field_names()
        self.assertEqual(names, ['header_flags:', 'colour:', 'title:', 'title_limit:'])
        self.assertEqual(self.blocks['first_object'].fields['title_limit:'].limit_of, 'title:')

    def test_field_in_a_condition_and_its_opposite_is_not_conditional(self):
        item = self.blocks['item']
        self.assertTrue(item.fields['text:'].always)
        self.assertEqual(list(item.fields['text:'].types), ['message', 'string'])

    def test_field_in_one_form_only_is_conditional(self):
        self.assertFalse(self.blocks['item'].fields['sprite_area:'].always)

    def test_common_fields_need_the_same_table(self):
        description = formatdocs.parse_describe(DESCRIBE)
        members, common = formatdocs.common_fields(self.blocks, description.tables,
                                                   lambda block: block.parent is None)
        self.assertEqual([block.name for block in members], ['first_object', 'second_object'])
        # colour: and shade: differ in name, so only the header is common
        self.assertEqual(common, ['header_flags:'])


class TestRender(unittest.TestCase):

    def setUp(self):
        self.doc = generate(EMPTY_DOC)

    def test_outside_the_region_is_kept(self):
        self.assertTrue(self.doc.startswith('<riscos-prm>\n<p>Before.</p>\n'))
        self.assertTrue(self.doc.endswith('<p>After.</p>\n</riscos-prm>\n'))

    def test_one_section_per_block_and_group(self):
        titles = [line.split('"')[1] for line in self.doc.splitlines() if line.startswith('<subsubsection')]
        self.assertEqual(titles, ['Common object fields', 'first_object', 'item', 'second_object', 'Common values'])

    def test_common_fields_are_documented_once(self):
        self.assertIn('name="header_flags:"', section(self.doc, 'Common object fields'))
        self.assertNotIn('name="header_flags:"', section(self.doc, 'first_object'))
        self.assertIn('It begins with the Common object fields.', section(self.doc, 'second_object'))

    def test_types_and_conditions(self):
        item = section(self.doc, 'item')
        self.assertIn('extra="message when without ITEM_IS_SPRITE; string when ITEM_IS_SPRITE"', item)
        self.assertIn('extra="sprite area; only in the sprite-item form (ITEM_IS_SPRITE)"', item)
        self.assertIn('name="name:" extra="string of at most 11 characters"', section(self.doc, 'second_object'))
        self.assertIn('truncated, with a warning', section(self.doc, 'second_object'))

    def test_shared_table_goes_in_common_values(self):
        self.assertIn('extra="Wimp colour; values in Common values"', section(self.doc, 'first_object'))
        common = section(self.doc, 'Common values')
        self.assertIn('<value number="7" name="COLOUR_BLACK">', common)
        self.assertIn('<value number="&amp;1" name="HEADER_ONE">', section(self.doc, 'Common object fields'))

    def test_example_shows_every_field(self):
        first = section(self.doc, 'first_object')
        self.assertIn('first_object {<br/>', first)
        self.assertIn('  title_limit:*<br/>', first)
        # Only the first of each kind of nested block is shown
        self.assertEqual(first.count('item {<br/>'), 1)

    def test_regenerating_changes_nothing(self):
        self.assertEqual(generate(self.doc), self.doc)


class TestAuthorText(unittest.TestCase):

    def setUp(self):
        self.doc = generate(EMPTY_DOC)

    def write(self, old, new):
        """
        Make an author's change to the generated manual.
        """
        self.assertEqual(self.doc.count(old), 1, old)
        self.doc = self.doc.replace(old, new)

    def test_field_body_is_kept_and_attributes_refreshed(self):
        self.write('<definition name="title:" extra="message"><!-- generated -->Text, held in the message table so that it can be translated.</definition>',
                   '<definition name="title:" extra="stale">The <emphasis>title</emphasis> of the window.</definition>')
        updated = generate(self.doc)
        self.assertIn('<definition name="title:" extra="message">The <emphasis>title</emphasis> of the window.</definition>', updated)
        self.assertEqual(generate(updated), updated)

    def test_generated_body_is_replaced(self):
        self.write('<definition name="title:" extra="message"><!-- generated -->Text, held in the message table so that it can be translated.</definition>',
                   '<definition name="title:" extra="message"><!-- generated -->Out of date default.</definition>')
        updated = generate(self.doc)
        self.assertNotIn('Out of date default.', updated)
        self.assertIn('<definition name="title:" extra="message"><!-- generated -->Text, held in the message table', updated)

    def test_intro_and_notes_are_kept(self):
        self.write('<p>\n<!-- generated -->The fields of an <userinput>item</userinput> block, which is nested in a <userinput>first_object</userinput> block.\n</p>\n',
                   '<p>\nAn item of the first object.\n</p>\n<p>\nSecond paragraph.\n</p>\n')
        start = self.doc.index('<subsubsection title="item">')
        notes = self.doc.index('<!-- ccres:notes -->\n', start) + len('<!-- ccres:notes -->\n')
        self.doc = self.doc[:notes] + '<p>\nA note after the tables.\n</p>\n' + self.doc[notes:]
        updated = section(generate(self.doc), 'item')
        self.assertIn('An item of the first object.', updated)
        self.assertIn('Second paragraph.', updated)
        self.assertNotIn('The fields of an', updated)
        self.assertIn('A note after the tables.', updated)

    def test_value_body_is_kept(self):
        self.write('<value number="7" name="COLOUR_BLACK"></value>', '<value number="7" name="COLOUR_BLACK">Black.</value>')
        self.assertIn('<value number="7" name="COLOUR_BLACK">Black.</value>', generate(self.doc))

    def test_text_for_a_removed_field_is_reported(self):
        self.write('<definition name="name:" extra="string of at most 11 characters"><!-- generated -->',
                   '<definition name="name:" extra="string of at most 11 characters">Written by hand. ')
        without = DESCRIBE.replace('  name:"x"\n  #@ field name: type=fixed-string size=11 terminated\n', '')
        with self.assertRaisesRegex(formatdocs.MergeError, 'second_object name:'):
            generate(self.doc, without)

    def test_missing_region_is_reported(self):
        with self.assertRaisesRegex(formatdocs.MergeError, 'no region'):
            generate('<riscos-prm/>\n')


if __name__ == '__main__':
    unittest.main()
