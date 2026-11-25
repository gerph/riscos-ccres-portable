#!/usr/bin/env python3
"""
Generate the per-object field reference of the CCres manual from the
converter's own description of its text format.

`ccres --describe resource|template <file>` writes a skeleton of the text
format in which each field is followed by '#@' comment lines giving its type,
its limit field and its flag or enumeration table. This module parses that
description, merges the blocks it contains, and renders one PRM-in-XML
subsubsection per block into a marked region of the manual.

Generated content is refreshed on every run; text written by an author is
kept. Within a region, an author owns:

* the intro of each block: everything between its `<subsubsection>` and the
  `<!-- ccres:generated -->` marker;
* the notes of each block: everything between `<!-- ccres:notes -->` and the
  `</subsubsection>`;
* the body of each `<definition>` and `<value>`, unless it is empty or begins
  with `<!-- generated -->`, which marks text the generator wrote.

A region is delimited by `<!-- ccres-region: NAME -->` and
`<!-- ccres-region-end: NAME -->`. Bodies may contain markup, but not nested
`<definition>` or `<value>` elements.
"""

import argparse
import collections
import difflib
import os
import re
import subprocess
import sys
import tempfile


# Marker for text the generator wrote, which it may replace
GENERATED_MARK = '<!-- generated -->'

# Markers dividing a block's subsubsection into its parts
GENERATED_START = '<!-- ccres:generated -->'
NOTES_START = '<!-- ccres:notes -->'

# Region markers
REGION_START = '<!-- ccres-region: {} -->'
REGION_END = '<!-- ccres-region-end: {} -->'

# Blocks with more distinct kinds of nested block than this have their nested
# blocks summarised in their example, rather than shown
EXAMPLE_NESTED_KINDS_MAX = 3

# Groups of blocks whose leading fields in common are documented once.
# Each entry is (title, key, member test); the test is given a block.
COMMON_GROUPS = [
    ('Common object fields', 'common-object',
     lambda block: block.parent is None and block.name.endswith('_object')),
    ('Common gadget fields', 'common-gadget',
     lambda block: block.parent == 'window_object' and block.name != 'keyboardshortcut_object'),
]

# Prefix of a variant condition meaning the opposite condition
NOT_PREFIX = 'not-'

# Title of the subsubsection holding tables used by more than one block
COMMON_VALUES_TITLE = 'Common values'
COMMON_VALUES_KEY = 'common-values'

# Descriptions of the field types written by describe_field() in _Object.c
TYPE_DESCRIPTIONS = {
    'message': 'message',
    'string': 'string',
    'template-string': 'indirected string',
    'flags': 'flags',
    'icon-flags': 'icon flags',
    'enum': 'enumeration',
    'component': 'component id',
    'os-colour': 'OS colour',
    'wimp-colour': 'Wimp colour',
    'box': 'box: xmin,ymin,xmax,ymax',
    'coord': 'coordinate: x,y',
    'action': 'action number',
    'expression': 'expression',
    'bits': 'number',
    'int': 'integer',
    'short': '16-bit integer',
    'byte': '8-bit integer',
    'sprite-area': 'sprite area',
    'fixed-string': 'string',
    'esg': 'ESG number, 0 to 31',
}

# Default meanings for each type, used until an author writes one
TYPE_MEANINGS = {
    'message': 'Text, held in the message table so that it can be translated.',
    'string': 'Text.',
    'template-string': 'Text, held in an indirected buffer.',
    'flags': 'Zero or more of the flags below, separated by <userinput>|</userinput>. A value with no name may be given in hexadecimal, as <userinput>0x</userinput><em>value</em>.',
    'icon-flags': 'Zero or more Wimp icon flags, separated by <userinput>|</userinput>, and at most one button type.',
    'enum': 'One of the names below.',
    'component': 'A component id, as a number or one of the names below.',
    'os-colour': 'An OS colour, as one of the names below.',
    'wimp-colour': 'A Wimp colour, as one of the names below.',
    'box': 'Four numbers or expressions, separated by commas.',
    'coord': 'Two numbers or expressions, separated by commas.',
    'action': 'Toolbox event number. Left blank, no event is raised.',
    'expression': 'A number or an expression.',
    'bits': 'A number.',
    'int': 'A number.',
    'short': 'A number.',
    'byte': 'A number.',
    'sprite-area': 'Sprite area: <userinput>&amp;1</userinput> for the Wimp sprite area.',
    'fixed-string': 'Text, stored in the object itself.',
    'esg': 'Exclusive selection group number.',
}


class DescribeError(Exception):
    """
    The description from the converter could not be understood.
    """


class MergeError(Exception):
    """
    The manual could not be updated without losing an author's text.
    """


class Field(object):
    """
    One field of one block instance, as described by the converter.
    """

    def __init__(self, name, ftype, attrs, words, variant):
        self.name = name
        self.type = ftype
        self.attrs = attrs
        self.words = words
        self.variant = variant


class Instance(object):
    """
    One block in the description, with its fields, raw lines and children.
    """

    def __init__(self, name, parent):
        self.name = name
        self.parent = parent
        self.fields = []
        self.lines = []
        self.children = []


class Table(object):
    """
    A flag or enumeration table: a kind and a list of (value, name) pairs.
    """

    def __init__(self, number, kind):
        self.number = number
        self.kind = kind
        self.values = []

    def signature(self):
        """
        Identify the table by its contents, so that copies are merged.
        """
        return (self.kind, tuple(self.values))


class Description(object):
    """
    The parsed output of `ccres --describe`.
    """

    def __init__(self, header):
        self.header = header
        self.instances = []
        self.tables = {}


def parse_attrs(words):
    """
    Split 'key=value' words from the rest, giving a dictionary and a list.
    """
    attrs = {}
    rest = []
    for word in words:
        if '=' in word:
            key, value = word.split('=', 1)
            attrs[key] = value
        else:
            rest.append(word)
    return attrs, rest


def parse_describe(text):
    """
    Parse the text written by `ccres --describe` into a Description.
    """
    lines = text.splitlines()
    if not lines:
        raise DescribeError('The description is empty')
    description = Description(lines[0])
    stack = []
    variant = None
    block_re = re.compile(r'^\s*([A-Za-z_][A-Za-z0-9_]*)\s*\{\s*$')

    for number, line in enumerate(lines[1:], start=2):
        stripped = line.strip()
        if stripped.startswith('#@'):
            words = stripped[2:].split()
            if not words:
                raise DescribeError('Line {}: empty annotation'.format(number))
            kind = words[0]
            if kind == 'values':
                attrs, rest = parse_attrs(words[1:])
                table = Table(int(rest[0]), attrs.get('kind', ''))
                description.tables[table.number] = table
            elif kind == 'value':
                table = description.tables.get(int(words[1]))
                if table is None:
                    raise DescribeError('Line {}: value for an unknown table'.format(number))
                table.values.append((int(words[2], 16), words[3]))
            elif kind == 'field':
                if not stack:
                    raise DescribeError('Line {}: field outside a block'.format(number))
                attrs, rest = parse_attrs(words[2:])
                stack[-1].fields.append(Field(words[1], attrs.pop('type', ''), attrs, rest, variant))
            elif kind == 'variant':
                attrs, rest = parse_attrs(words[1:])
                variant = (rest[0], attrs.get('when', ''))
            else:
                raise DescribeError('Line {}: unknown annotation {!r}'.format(number, kind))
            continue

        match = block_re.match(line)
        if match:
            instance = Instance(match.group(1), stack[-1].name if stack else None)
            if stack:
                stack[-1].children.append(instance)
                stack[-1].lines.append(('child', instance))
            else:
                description.instances.append(instance)
            stack.append(instance)
            variant = None
        elif stripped == '}':
            if not stack:
                raise DescribeError('Line {}: unbalanced closing brace'.format(number))
            stack.pop()
            variant = None
        elif stripped and stack:
            stack[-1].lines.append(('line', line.strip()))
    if stack:
        raise DescribeError('The description ends inside block {!r}'.format(stack[-1].name))
    return description


class BlockField(object):
    """
    A field of a merged block: its types and the variants it appears in.
    """

    def __init__(self, name):
        self.name = name
        self.types = collections.OrderedDict()
        self.attrs = {}
        self.words = []
        self.variants = set()
        self.always = False
        self.limit_of = None


class Block(object):
    """
    All the instances of one kind of block, merged.
    """

    def __init__(self, name, parent):
        self.name = name
        self.parent = parent
        self.instances = []
        self.fields = collections.OrderedDict()
        self.variants = collections.OrderedDict()

    def field_names(self):
        """
        Give the names of the fields, in order.
        """
        return list(self.fields.keys())


def merge_blocks(description):
    """
    Merge the instances of each kind of block, in the order first seen.
    """
    blocks = collections.OrderedDict()

    def visit(instance):
        block = blocks.get(instance.name)
        if block is None:
            block = Block(instance.name, instance.parent)
            blocks[instance.name] = block
        block.instances.append(instance)
        for field in instance.fields:
            merged = block.fields.get(field.name)
            if merged is None:
                merged = BlockField(field.name)
                block.fields[field.name] = merged
            when = field.variant[1] if field.variant else None
            merged.types.setdefault(field.type, set()).add(when)
            merged.attrs.update(field.attrs)
            merged.words = field.words
            if field.variant:
                merged.variants.add(field.variant)
                block.variants[field.variant] = True
            else:
                merged.always = True
            # A limit field is documented after the field it belongs to
            limit = field.attrs.get('limit')
            if limit and limit not in block.fields:
                limit_field = BlockField(limit)
                limit_field.limit_of = field.name
                block.fields[limit] = limit_field
            if limit:
                limit_field = block.fields[limit]
                limit_field.types.setdefault('limit', set()).add(when)
                if field.variant:
                    limit_field.variants.add(field.variant)
                else:
                    limit_field.always = True
        for child in instance.children:
            visit(child)

    for instance in description.instances:
        visit(instance)

    # A field found under both a condition and its opposite ('X' and
    # 'not-X') is found whatever the condition, so is not conditional
    for block in blocks.values():
        for field in block.fields.values():
            whens = set(when for _, when in field.variants)
            if not field.always and any(NOT_PREFIX + when in whens for when in whens):
                field.always = True
    return blocks


def field_signature(field, tables):
    """
    Identify a field by its name, types and the contents of its tables, so
    that fields of the same name with different flags are told apart.
    """
    values = tuple(tables[int(field.attrs[key])].signature() if int(field.attrs[key]) in tables else None
                   for key in ('values', 'buttons') if key in field.attrs)
    return (field.name, tuple(field.types), values)


def common_fields(blocks, tables, test):
    """
    Find the members of a group and the leading fields they all share, with
    the same types and values.
    """
    members = [block for block in blocks.values() if test(block)]
    if len(members) < 2:
        return members, []
    common = []
    first = list(members[0].fields.values())
    for index, field in enumerate(first):
        signature = field_signature(field, tables)
        if all(len(block.fields) > index and field_signature(list(block.fields.values())[index], tables) == signature
               for block in members):
            common.append(field.name)
        else:
            break
    return members, common


def escape(text):
    """
    Escape text for an XML attribute or element.
    """
    return text.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;').replace('"', '&quot;')


def table_owners(blocks, description, skipped):
    """
    Find which block and field first uses each table, and whether more than
    one block does. Copies of the same table are treated as one.
    """
    owners = collections.OrderedDict()
    users = collections.defaultdict(set)
    canonical = {}
    for number, table in description.tables.items():
        canonical[number] = canonical.get(number) or next(
            other for other, candidate in description.tables.items() if candidate.signature() == table.signature())
    for block in blocks.values():
        for field in block.fields.values():
            for key in ('values', 'buttons'):
                if key in field.attrs:
                    number = canonical.get(int(field.attrs[key]))
                    if number is None:
                        continue
                    group = skipped.get((block.name, field.name))
                    owner = group or block.name
                    users[number].add(owner)
                    owners.setdefault(number, (owner, field.name))
    return owners, users, canonical


def value_number(kind, value):
    """
    Format a table value: hexadecimal for flags, signed decimal for names.
    """
    if kind == 'flags' or value >= 0x10000:
        if value >= 0x80000000 and kind != 'flags':
            return str(value - 0x100000000)
        return '&amp;{:X}'.format(value)
    return str(value)


def field_extra(field, users, canonical):
    """
    Describe a field's type, as plain text, for the 'extra' column of its
    definition.
    """
    if field.limit_of:
        parts = ['number, or *']
    else:
        parts = []
        for ftype, whens in field.types.items():
            text = TYPE_DESCRIPTIONS.get(ftype, ftype)
            if ftype == 'flags' and field.attrs.get('bits') == '8':
                text = '8-bit flags'
            if ftype == 'fixed-string' and 'size' in field.attrs:
                text = 'string of at most {} characters'.format(field.attrs['size'])
            if len(field.types) > 1:
                conditions = sorted(when for when in whens if when)
                if conditions:
                    text = '{} when {}'.format(text, ', '.join(describe_condition(c) for c in conditions))
            parts.append(text)
        for key in ('values', 'buttons'):
            if key in field.attrs:
                number = canonical.get(int(field.attrs[key]))
                if number is not None and len(users[number]) > 1:
                    parts.append('values in {}'.format(COMMON_VALUES_TITLE))
    if not field.always and field.variants:
        forms = sorted('the {} form ({})'.format(name, describe_condition(when)) for name, when in field.variants)
        parts.append('only in {}'.format(' or '.join(forms)))
    return '; '.join(parts)


def describe_condition(when):
    """
    Describe a variant's condition: 'not-X' reads as 'without X'.
    """
    if when.startswith(NOT_PREFIX):
        return 'without {}'.format(when[len(NOT_PREFIX):])
    return when


def article(name):
    """
    Give the indefinite article for a name.
    """
    return 'an' if name[:1].lower() in 'aeiou' else 'a'


def field_meaning(field):
    """
    Give the default meaning of a field, until an author writes one.
    """
    if field.limit_of:
        return 'Size of the buffer for <userinput>{}</userinput>. <userinput>*</userinput> makes it the length of the text given.'.format(escape(field.limit_of))
    ftype = next(iter(field.types))
    meaning = TYPE_MEANINGS.get(ftype, '')
    if ftype == 'fixed-string' and 'terminated' in field.words:
        meaning = 'Text, stored in the object itself. Longer text is truncated, with a warning.'
    return meaning


def render_example(instance):
    """
    Render a skeleton block as an example, summarising nested blocks when
    there are many kinds of them.
    """
    kinds = []
    for child in instance.children:
        if child.name not in kinds:
            kinds.append(child.name)
    lines = ['{} {{'.format(instance.name)]
    shown = set()
    summarised = False
    for kind, item in instance.lines:
        if kind == 'line':
            lines.append('  ' + item)
        elif len(kinds) <= EXAMPLE_NESTED_KINDS_MAX:
            if item.name not in shown:
                shown.add(item.name)
                lines.extend('  ' + line for line in render_example(item))
        elif not summarised:
            summarised = True
            lines.append('  # nested blocks, each described in its own section')
    lines.append('}')
    return lines


def extended_example(lines):
    """
    Wrap example lines as a PRM-in-XML extended-example.
    """
    body = ''.join('{}<br/>\n'.format(escape(line)) for line in lines)
    return '<p>\n<extended-example type="format">\n{}</extended-example>\n</p>\n'.format(body)


class Section(object):
    """
    One generated subsubsection: its key, title, default intro and the
    generated content, with keyed bodies to be filled from the manual.
    """

    def __init__(self, key, title, intro):
        self.key = key
        self.title = title
        self.intro = intro
        self.parts = []

    def add_text(self, text):
        """
        Add generated markup.
        """
        self.parts.append(('text', text))

    def add_body(self, key, default):
        """
        Add a body which an author may replace.
        """
        self.parts.append(('body', (key, default)))


def build_sections(description):
    """
    Build the generated sections for a region from a description.
    """
    blocks = merge_blocks(description)
    sections = []

    # Leading fields shared by a group are documented once, for the group
    skipped = {}
    groups = []
    for title, key, test in COMMON_GROUPS:
        members, common = common_fields(blocks, description.tables, test)
        if common:
            groups.append((title, key, members, common))
            for member in members:
                for name in common:
                    skipped[(member.name, name)] = key
                    limit = member.fields[name].attrs.get('limit')
                    if limit:
                        skipped[(member.name, limit)] = key

    owners, users, canonical = table_owners(blocks, description, skipped)

    def field_table(section, owner_key, block, names):
        section.add_text('<p>\n<definition-table head-name="Field" head-extra="Type" head-value="Meaning">\n')
        for name in names:
            field = block.fields[name]
            section.add_text(' <definition name="{}" extra="{}">'.format(
                escape(name), escape(field_extra(field, users, canonical))))
            section.add_body(('field', owner_key, name), field_meaning(field))
            section.add_text('</definition>\n')
        section.add_text(' </definition-table>\n</p>\n')

    def value_tables(section, owner_key, shared):
        for number, (owner, field_name) in owners.items():
            if (len(users[number]) > 1) != shared or (not shared and owner != owner_key):
                continue
            table = description.tables[number]
            if not table.values:
                continue
            section.add_text('<p>\nValues for <userinput>{}</userinput>{}\n</p>\n'.format(
                escape(field_name), '' if not shared else ' (and other fields)'))
            section.add_text('<p>\n<value-table head-number="Value" head-name="Name" head-value="Meaning">\n')
            table_key = '{}/{}'.format(owner, field_name)
            for value, name in table.values:
                section.add_text(' <value number="{}" name="{}">'.format(value_number(table.kind, value), escape(name)))
                section.add_body(('value', table_key, name), '')
                section.add_text('</value>\n')
            section.add_text(' </value-table>\n</p>\n')

    for title, key, members, common in groups:
        section = Section(key, title, '<p>\n{}These fields begin every block of the following kinds: {}.\n</p>\n'.format(
            GENERATED_MARK, ', '.join('<userinput>{}</userinput>'.format(escape(m.name)) for m in members)))
        # The common fields include any limit fields, which follow their strings
        field_table(section, key, members[0], common)
        value_tables(section, key, False)
        sections.append(section)

    for block in blocks.values():
        section = Section(block.name, block.name, '<p>\n{}The fields of {} <userinput>{}</userinput> block{}.\n</p>\n'.format(
            GENERATED_MARK, article(block.name), escape(block.name),
            '' if block.parent is None else ', which is nested in {} <userinput>{}</userinput> block'.format(article(block.parent), escape(block.parent))))
        names = [name for name in block.field_names() if (block.name, name) not in skipped]
        groups_used = sorted(set(skipped[(block.name, name)] for name in block.field_names() if (block.name, name) in skipped))
        if groups_used:
            section.add_text('<p>\nIt begins with the {}.\n</p>\n'.format(
                ' and '.join(title for title, key, _, _ in groups if key in groups_used)))
        if names:
            field_table(section, block.name, block, names)
        value_tables(section, block.name, False)
        section.add_text('<p>\nExample, with every field at its zero value:\n</p>\n')
        section.add_text(extended_example(render_example(block.instances[0])))
        sections.append(section)

    shared = [number for number in owners if len(users[number]) > 1]
    if shared:
        section = Section(COMMON_VALUES_KEY, COMMON_VALUES_TITLE, '<p>\n{}Values accepted by fields of more than one kind of block.\n</p>\n'.format(GENERATED_MARK))
        value_tables(section, None, True)
        sections.append(section)
    return sections


def find_region(doc, name):
    """
    Find a region in the manual, giving the offsets of its contents.
    """
    start_marker = REGION_START.format(name)
    end_marker = REGION_END.format(name)
    start = doc.find(start_marker)
    end = doc.find(end_marker)
    if start < 0 or end < 0 or end < start:
        raise MergeError('The manual has no region {!r}: add {} and {} where it should go'.format(name, start_marker, end_marker))
    return start + len(start_marker), end


def is_generated(body):
    """
    Say whether a body may be replaced: empty, or written by the generator.
    """
    stripped = body.strip()
    return stripped == '' or stripped.startswith(GENERATED_MARK)


def read_region(text):
    """
    Read the author's text from an existing region: each section's intro and
    notes, and the bodies of its definitions and values.
    """
    authored = {}
    section_re = re.compile(r'<subsubsection title="([^"]*)">\n?(.*?)</subsubsection>', re.S)
    for match in section_re.finditer(text):
        content = match.group(2)
        key_match = re.search(r'<!-- ccres:key ([^ ]+) -->\n?', content)
        key = key_match.group(1) if key_match else unescape_name(match.group(1))
        if key_match:
            content = content.replace(key_match.group(0), '', 1)
        intro, _, rest = content.partition(GENERATED_START + '\n')
        generated, _, notes = rest.partition(NOTES_START + '\n')

        # The intro and notes belong to the author, unless generated
        if intro.strip() and GENERATED_MARK not in intro:
            authored[('intro', key)] = intro
        if notes.strip():
            authored[('notes', key)] = notes

        # Field bodies are keyed by the section and the field name
        for definition in re.finditer(r'<definition name="([^"]*)"[^>]*>(.*?)</definition>', generated, re.S):
            if not is_generated(definition.group(2)):
                authored[('field', key, unescape_name(definition.group(1)))] = definition.group(2)

        # Value bodies are keyed by their table, named before each table
        for table in re.finditer(r'<!-- ccres:table ([^ ]+) -->(.*?)</value-table>', generated, re.S):
            for value in re.finditer(r'<value number="[^"]*" name="([^"]*)">(.*?)</value>', table.group(2), re.S):
                if not is_generated(value.group(2)):
                    authored[('value', table.group(1), unescape_name(value.group(1)))] = value.group(2)
    return authored


def unescape_name(name):
    """
    Undo the escaping of a name attribute.
    """
    return name.replace('&quot;', '"').replace('&lt;', '<').replace('&gt;', '>').replace('&amp;', '&')


def render_region(sections, authored):
    """
    Render the sections of a region, keeping the author's text, and give the
    keys of authored text that has nowhere to go.
    """
    used = set()
    out = ['\n']
    for section in sections:
        out.append('<subsubsection title="{}">\n'.format(escape(section.title)))
        if section.key != section.title:
            out.append('<!-- ccres:key {} -->\n'.format(section.key))
        intro = authored.get(('intro', section.key))
        if intro is not None:
            used.add(('intro', section.key))
            out.append(intro)
        else:
            out.append(section.intro)
        out.append(GENERATED_START + '\n')
        for kind, item in section.parts:
            if kind == 'text':
                out.append(item)
                continue
            key, default = item
            full_key = (key[0],) + tuple(k if k is not None else '' for k in key[1:])
            if full_key in authored:
                used.add(full_key)
                out.append(authored[full_key])
            elif default:
                out.append(GENERATED_MARK + default)
        out.append(NOTES_START + '\n')
        notes = authored.get(('notes', section.key))
        if notes is not None:
            used.add(('notes', section.key))
            out.append(notes)
        out.append('</subsubsection>\n\n')
    orphans = sorted(key for key in authored if key not in used)
    return ''.join(out), orphans


def mark_tables(sections):
    """
    Record each value table's key before it, so that value bodies can be
    found again when the region is read back.
    """
    for section in sections:
        parts = []
        for kind, item in section.parts:
            if kind == 'text' and item.startswith('<p>\n<value-table'):
                parts.append(('table-mark', None))
            parts.append((kind, item))
        resolved = []
        for index, (kind, item) in enumerate(parts):
            if kind == 'table-mark':
                # The key of the first value body after the mark names the table
                for later_kind, later_item in parts[index + 1:]:
                    if later_kind == 'body' and later_item[0][0] == 'value':
                        resolved.append(('text', '<!-- ccres:table {} -->\n'.format(later_item[0][1])))
                        break
            else:
                resolved.append((kind, item))
        section.parts = resolved


def update_document(doc, regions):
    """
    Update the regions of a manual from their sections, giving the new text.
    `regions` maps region names to lists of sections.
    """
    problems = []
    for name, sections in regions.items():
        mark_tables(sections)
        start, end = find_region(doc, name)
        authored = read_region(doc[start:end])
        text, orphans = render_region(sections, authored)
        for orphan in orphans:
            problems.append('Region {!r}: the text written for {} no longer has a place; move or remove it'.format(name, ' '.join(orphan)))
        doc = doc[:start] + text + doc[end:]
    if problems:
        raise MergeError('\n'.join(problems))
    return doc


def describe(ccres, kind):
    """
    Run the converter to describe its text format for a kind of file.
    """
    with tempfile.TemporaryDirectory() as workdir:
        output = os.path.join(workdir, 'describe.txt')
        result = subprocess.run([ccres, '--describe', kind, output], capture_output=True, text=True)
        if result.returncode != 0 or result.stderr:
            raise DescribeError('{} --describe {} failed (status {}): {}'.format(ccres, kind, result.returncode, result.stderr.strip()))
        with open(output, encoding='latin-1') as handle:
            return handle.read()


def main(argv=None):
    """
    Update, or check, the generated regions of the manual.
    """
    parser = argparse.ArgumentParser(description='Update the generated field reference in the CCres manual.')
    parser.add_argument('--ccres', default='./ccres', help='converter to describe the format (default ./ccres)')
    parser.add_argument('--check', action='store_true', help='only check that the manual is up to date')
    parser.add_argument('document', help='PRM-in-XML manual to update')
    options = parser.parse_args(argv)

    try:
        regions = collections.OrderedDict()
        for kind in ('resource', 'template'):
            regions[kind] = build_sections(parse_describe(describe(options.ccres, kind)))
        with open(options.document, encoding='utf-8') as handle:
            doc = handle.read()
        updated = update_document(doc, regions)
    except (DescribeError, MergeError) as exc:
        sys.stderr.write('{}\n'.format(exc))
        return 1

    if options.check:
        if updated != doc:
            diff = difflib.unified_diff(doc.splitlines(True), updated.splitlines(True), options.document, options.document + ' (generated)', n=1)
            sys.stdout.writelines(list(diff)[:200])
            sys.stderr.write('{} is out of date: run tools/formatdocs.py {}\n'.format(options.document, options.document))
            return 1
        return 0
    if updated != doc:
        with open(options.document, 'w', encoding='utf-8') as handle:
            handle.write(updated)
    return 0


if __name__ == '__main__':
    sys.exit(main())
