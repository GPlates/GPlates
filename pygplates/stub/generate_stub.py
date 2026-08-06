#!/usr/bin/env python
"""
Generate the pygplates type stub ('__init__.pyi') by introspecting the built pygplates module.

pygplates is a Boost.Python extension, so there is no type information available to IDEs and
type checkers. However every function/method/property docstring follows a hand-written
convention:

    Line 1 (column 0): a bare call signature with NO types, optional params in [...]:
        get_geometry([property_query], [property_return=PropertyReturn.exactly_one])

    Body (indented 2 spaces): standard Sphinx ReST fields carrying the real type info:
        :type property_query: PropertyName, or callable (accepting single Property argument)
        :rtype: GeometryOnSphere, or None

Boost.Python concatenates the docstrings of sibling '.def()' overloads, so a docstring can
contain multiple column-0 signature lines - these become '@overload' definitions.

This script imports the *built* pygplates module (which gives us real staticmethod
descriptors, properties, enums, bases and the injected pure-Python members for free), parses
the docstring convention, and emits a PEP 561 stub with the full docstrings embedded
(signature lines stripped) so that IDE hover documentation keeps working - IDEs like Pylance
never import the module, so the stub is their only source of documentation.

Any ':type:'/':rtype:' text the ReST type-expression parser cannot understand becomes 'Any'
and is reported (deduplicated) on stderr - that report is the incremental
docstring-improvement worklist, not an error. MANUAL_OVERRIDES (below) supplies complete
replacement signatures for hopeless cases.

Usage:
    generate_stub.py --module-dir <dir-of-built-pyd> [--output <path>] [--check <committed-stub>]

    --module-dir  directory containing the built pygplates module ('pygplates.pyd' / '.so')
    --output      write the generated stub to this path
    --check       regenerate in memory and compare against the committed stub (newline
                  differences are ignored); on mismatch print a capped unified diff and the
                  exact regeneration command, and exit with status 1. Combinable with
                  --output (the 'pygplates-stub-test' ctest uses both so the regenerated
                  stub can be inspected in the build directory).

The generated stub is deterministic (no timestamps, sorted names, fixed formatting) - the
committed copy in this directory should be byte-identical to a fresh regeneration on any
platform; the 'pygplates-stub-test' ctest enforces this.

Note: 'python -m mypy.stubtest pygplates' (against an installed pygplates) is a useful
advisory check of the stub but is expected to be noisy - Boost.Python function objects are
not inspectable, so stubtest reports every def as 'is not a function' style noise.

This source file must stay Python 3.8 compatible (it runs under GPLATES_PYTHON_EXECUTABLE);
the *emitted* stub freely uses modern stub-only syntax ('X | None', 'list[X]') which is
legal in a '.pyi' regardless of the runtime Python version.
"""

import argparse
import ast
import difflib
import inspect
import io
import os
import platform
import re
import sys


# ---------------------------------------------------------------------------
# Configuration.
# ---------------------------------------------------------------------------

# Modules that pygplates API objects legitimately report via '__module__'.
#
# 'pygplates.pygplates' when 'pygplates' is imported as a package - which is how it is both built and
# installed, and hence how this generator normally sees it (the shared library lives inside the package;
# see 'cmake/modules/Install.cmake').
#
# 'pygplates' when the shared library is imported directly, without the package around it. That happens
# in the documentation build (deliberately - see 'doc-python-api/conf.py.in') and in GPlates' embedded
# interpreter (which registers 'pygplates' via 'PyImport_AppendInittab').
_PYGPLATES_MODULES = ('pygplates', 'pygplates.pygplates')

# Module-level names to always exclude from the stub:
# Python 2 compatibility helpers exec'd into the module dict (they get
# '__module__' == 'pygplates' so the foreign-module filter does not catch them).
_SKIP_MODULE_NAMES = frozenset(['iteritems', 'itervalues', 'listitems', 'listvalues'])

# Class attributes that are implementation machinery, not API.
_SKIP_CLASS_ATTRS = frozenset([
    '__doc__', '__module__', '__qualname__', '__name__', '__dict__', '__weakref__',
    '__slots__', '__instance_size__', '__safe_for_unpickling__', '__getinitargs__',
    '__reduce__',
])

# A column-0 docstring signature line: 'name(args)'.
_SIG_LINE_RE = re.compile(r'^(\w+)\((.*)\)\s*$')

# Sphinx info fields (one per line in the pygplates docstrings).
_TYPE_FIELD_RE = re.compile(r'^\s*:type\s+([\w*]+):\s*(.*?)\s*$')
_RTYPE_FIELD_RE = re.compile(r'^\s*:rtype:\s*(.*?)\s*$')
_PROPERTY_TYPE_FIELD_RE = re.compile(r'^\s*:type:\s*(.*?)\s*$')

# Methods of *nested* classes document their parameters as a bullet list rather than with
# ':param:'/':type:' fields, because Sphinx does not render the fields there (see the note
# at src/api/PyTopologicalModel.cc, 'DeactivatePoints.deactivate'):
#   * **prev_point** (:class:`PointOnSphere`): the previous position of the point
#   * **Return type**: bool
_BULLET_TYPE_FIELD_RE = re.compile(r'^\s*\*\s+\*\*(\w+)\*\*\s+\((.*?)\):\s')
_BULLET_RTYPE_FIELD_RE = re.compile(r'^\s*\*\s+\*\*Return type\*\*:\s*(.*?)\s*$')

# A docstring body that is nothing but a delegation to another method, eg
# 'Same as :meth:`get_resolved_boundary`.' - such bodies carry no ':rtype:' of their own,
# so the delegate's type fields are inherited (see 'StubGenerator._alias_type_fields').
_ALIAS_RE = re.compile(r'^(?:Same as|See|Equivalent to)\s+:meth:`~?([\w.]+)`\s*\.?$')

# Prose that implies a return value. A docstring with no ':rtype:' is normally a C++ 'void'
# function (the convention omits the field), but if the prose says otherwise the field was
# probably just forgotten - report it rather than silently annotating '-> None'.
_RETURNS_PROSE_RE = re.compile(r'^(?:Returns?|Queries|Same as|See|Equivalent to)\b')

# Fallback signatures for dunders that have no docstring (mostly generated by
# boost/python/operators.hpp). Values are (parameters-after-self, return-type).
# Arithmetic operators return 'Any' in v1 - refine via MANUAL_OVERRIDES later.
_DUNDER_FALLBACK_SIGNATURES = {
    '__eq__': ('other: object', 'bool'),
    '__ne__': ('other: object', 'bool'),
    '__lt__': ('other: Any', 'bool'),
    '__le__': ('other: Any', 'bool'),
    '__gt__': ('other: Any', 'bool'),
    '__ge__': ('other: Any', 'bool'),
    '__add__': ('other: Any', 'Any'),
    '__radd__': ('other: Any', 'Any'),
    '__iadd__': ('other: Any', 'Any'),
    '__sub__': ('other: Any', 'Any'),
    '__rsub__': ('other: Any', 'Any'),
    '__isub__': ('other: Any', 'Any'),
    '__mul__': ('other: Any', 'Any'),
    '__rmul__': ('other: Any', 'Any'),
    '__imul__': ('other: Any', 'Any'),
    '__truediv__': ('other: Any', 'Any'),
    '__rtruediv__': ('other: Any', 'Any'),
    '__itruediv__': ('other: Any', 'Any'),
    '__pow__': ('other: Any', 'Any'),
    '__rpow__': ('other: Any', 'Any'),
    '__xor__': ('other: Any', 'Any'),
    '__rxor__': ('other: Any', 'Any'),
    '__neg__': ('', 'Any'),
    '__pos__': ('', 'Any'),
    '__abs__': ('', 'Any'),
    '__invert__': ('', 'Any'),
    '__hash__': ('', 'int'),
    '__str__': ('', 'str'),
    '__repr__': ('', 'str'),
    '__bool__': ('', 'bool'),
    '__len__': ('', 'int'),
    '__iter__': ('', 'Any'),
    '__contains__': ('item: Any', 'bool'),
    '__getitem__': ('key: Any', 'Any'),
    '__setitem__': ('key: Any, value: Any', 'None'),
    '__delitem__': ('key: Any', 'None'),
    '__call__': ('*args: Any, **kwargs: Any', 'Any'),
    '__copy__': ('', 'Any'),
    '__deepcopy__': ('memo: Any', 'Any'),
    '__getstate__': ('', 'Any'),
    '__setstate__': ('state: Any', 'None'),
}

# Complete replacement signatures for cases the docstring parser cannot express.
# Key: fully qualified name (eg, 'RotationModel.get_rotation').
# Value: dict mapping overload index (0-based, 0 for a non-overloaded def) to the full
# 'def name(...) -> T' line (without trailing ':'); the parsed docstring is still embedded.
# For a member whose docstring has no signature line at all, multiple indices emit
# '@overload' definitions.
MANUAL_OVERRIDES = {
    # An alias of 'create_normalised' - its docstring is just 'See create_normalised'.
    'Vector3D.create_normalized': {
        0: 'def create_normalized(xyz: Sequence[tuple[float, float, float]] | Vector3D)'
           ' -> Vector3D',
        1: 'def create_normalized(x: float, y: float, z: float) -> Vector3D',
    },
}

# Word spellings of primitive types found in the ':type:' fields.
_WORD_TYPES = {
    'int': 'int', 'integer': 'int', 'integers': 'int', 'ints': 'int', 'long': 'int',
    'float': 'float', 'floats': 'float', 'double': 'float', 'doubles': 'float',
    'str': 'str', 'string': 'str', 'strings': 'str',
    'bool': 'bool', 'boolean': 'bool', 'booleans': 'bool', 'bools': 'bool',
    'none': 'None',
    # An explicit 'accepts anything' spelling, for runtime type predicates such as
    # 'FeaturesFunctionArgument.contains_features' - deliberate, unlike a missing ':type:'.
    'any': 'Any',
    # Coordinate names used in the tuple forms '(latitude,longitude)' and '(x,y,z)'.
    'latitude': 'float', 'longitude': 'float', 'x': 'float', 'y': 'float', 'z': 'float',
    # '(key, value) tuples' - untyped in that spelling.
    'key': 'Any', 'value': 'Any',
}

# Words that spell out a tuple arity ('tuple of three Property').
_COUNT_WORDS = {'two': 2, 'three': 3, 'four': 4, 'five': 5}


# ---------------------------------------------------------------------------
# ReST type-expression parser (recursive descent over a token stream).
# ---------------------------------------------------------------------------

class TypeParseError(Exception):
    pass


_TOKEN_RE = re.compile(
    r":(?P<role_name>[\w:.]+):`(?P<role_text>[^`]+)`"
    r"|(?P<word>[A-Za-z_][\w.]*)"
    r"|(?P<punct>[()\[\],/|])"
    r"|(?P<ellipsis>\.\.\.)"
    r"|(?P<period>\.)"
    r"|(?P<ws>\s+)"
    r"|(?P<other>\S)"
)


def _tokenize(text):
    tokens = []
    for match in _TOKEN_RE.finditer(text):
        # (Cannot use 'match.lastgroup' for the role alternative - it names the *last*
        # participating group, 'role_text'.)
        if match.group('role_name') is not None:
            tokens.append(('role', (match.group('role_name'), match.group('role_text'))))
            continue
        kind = match.lastgroup
        value = match.group(kind)
        if kind == 'ws':
            continue
        if kind == 'word':
            # A sentence period glued onto a word ('None.') - split it off.
            stripped = value.rstrip('.')
            tokens.append(('word', stripped))
            if stripped != value:
                tokens.append(('period', '.'))
            continue
        tokens.append((kind, value))
    return tokens


class _TokenStream(object):
    def __init__(self, tokens):
        self.tokens = tokens
        self.pos = 0

    def peek(self, offset=0):
        index = self.pos + offset
        return self.tokens[index] if index < len(self.tokens) else None

    def next(self):
        token = self.peek()
        if token is None:
            raise TypeParseError('unexpected end of type expression')
        self.pos += 1
        return token

    def at_end(self):
        return self.pos >= len(self.tokens)

    def peek_word(self, offset=0):
        token = self.peek(offset)
        if token and token[0] == 'word':
            return token[1].lower()
        return None

    def accept_word(self, *words):
        if self.peek_word() in words:
            return self.next()[1]
        return None

    def expect(self, kind, value=None):
        token = self.next()
        if token[0] != kind or (value is not None and token[1] != value):
            raise TypeParseError('expected %s %r, got %r' % (kind, value, token))
        return token


class TypeExpressionParser(object):
    """
    Parses the ':type:'/':rtype:' ReST text convention into a stub annotation.

    The corpus follows the markup-free type-field style (the docstring type-field style
    guideline in doc-python-api/PLAN.md): bare and dotted identifiers resolved against
    the module ('FiniteRotation', 'NetworkTriangulation.Triangle', enum values like
    'PropertyReturn.exactly_one' standing for their enumeration type), primitive
    spellings ('integer', 'string', 'none', 'double', plurals), Python bracket generics
    'list[X]', 'tuple[A, B]', 'tuple[X, ...]', 'dict[K, V]' (arbitrarily nested, mixing
    with the prose forms), prose containers 'list of X', 'sequence (eg, list or tuple)
    of X', 'tuple of three X', unions with 'or' / ', or' / '. Or' / '|', tuple forms
    'the tuple (A, B)', 'tuple (A, B [, C])', 'list of (A, B) tuples', '(x,y,z) tuple',
    'string/os.PathLike', 'callable (accepting single X argument)', and placeholder
    glosses ('sequence of rings (where ring is ...)', 'tuple (point, float) where
    point is ...').

    ':class:`X`' roles are also accepted: the docstrings templated in
    src/api/PyRevisionedVector.h and the bullet-list fields of nested-class methods
    (see '_BULLET_TYPE_FIELD_RE') still spell types that way.
    """

    def __init__(self, generator):
        self.generator = generator
        # Bindings of 'where <name> is <expr>' gloss placeholders, live for one 'parse()'.
        self._bindings = {}
        # Non-fatal observations made while parsing (drained by '_parse_type_text').
        self.parse_warnings = []

    def parse(self, text):
        """Return the annotation for 'text', or raise TypeParseError."""
        self._bindings = {}
        self.parse_warnings = []
        return self._parse_text(text)

    def _parse_text(self, text):
        # Normalize phrases that carry no type information.
        text = text.replace('read-only ', '')      # 'a read-only sequence of X'
        text = text.replace(', in degrees', '')    # 'tuple (latitude,longitude), in degrees'
        text = text.replace(' in degrees', '')     # '(latitude,longitude) tuple in degrees'
        # 'N-tuple' arity prefix: '2-tuple (A, B)' -> 'tuple (A, B)'.
        text = re.sub(r'\b\d+-tuple\b', 'tuple', text)
        # 'where <name> is <expr>' glosses bind a placeholder word used in the main text
        # ('sequence of tuple (point, float) where *point* is ...').
        text, glosses = self._extract_where_glosses(text)
        for gloss_name, gloss_text in glosses:
            self._bindings[gloss_name] = self._parse_text(gloss_text)
        stream = _TokenStream(_tokenize(text))
        if stream.at_end():
            raise TypeParseError('empty type expression')
        return self._parse_union(stream)

    def _extract_where_glosses(self, text):
        """
        Split off 'where <name> is/are/can be <expr>' gloss clauses.

        Returns (text with the clauses removed, [(name, expr)]). The clause is either
        parenthesised ('sequence of rings (where ring is any sequence of ...)') - found
        by a balanced-paren scan since the expr may itself contain tuple forms - or
        trails the whole expression ('tuple (point, float) where *point* is ...').
        """
        glosses = []
        while True:
            match = re.search(r'\(\s*where\b', text)
            if match is None:
                break
            start = match.start()
            depth = 0
            end = start
            for end in range(start, len(text)):
                if text[end] == '(':
                    depth += 1
                elif text[end] == ')':
                    depth -= 1
                    if depth == 0:
                        break
            if depth != 0:
                break  # unbalanced - leave for the tokenizer/parser to reject
            clause = self._match_where_gloss(text[start + 1:end])
            if clause is None:
                break
            glosses.append(clause)
            text = text[:start] + text[end + 1:]
        trailing = re.search(r'\bwhere\b', text)
        if trailing is not None:
            clause = self._match_where_gloss(text[trailing.start():])
            if clause is not None:
                glosses.append(clause)
                text = text[:trailing.start()]
        return text, glosses

    @staticmethod
    def _match_where_gloss(clause):
        """('name', 'expr') from a clause starting at its 'where', or None."""
        match = re.match(r'where\s+(?:an?\s+)?\*?(\w+)\*?\s+(?:is|are|can\s+be)\s+(.+)$',
                         clause, re.DOTALL)
        if match is None:
            return None
        return match.group(1).lower(), match.group(2)

    def _parse_union(self, stream):
        # 'items' doubles as the referent for a later 'them' / 'any combination of those
        # four types' ('sequence of any combination of them' unions the alternatives
        # already listed).
        items = []
        items.append(self._parse_disjunct(stream, items))
        while True:
            if not self._consume_union_separator(stream):
                break
            if stream.at_end():
                break
            items.append(self._parse_disjunct(stream, items))
        if not stream.at_end():
            raise TypeParseError('trailing tokens at %r' % (stream.peek(),))
        return self._make_union(items)

    def _consume_union_separator(self, stream):
        # Union separators: ',' and/or 'or' and '|' (also '. Or' - a new sentence
        # continuing the union). A bare trailing '.' (end of sentence) is consumed
        # harmlessly, as is a ' - ...' commentary tail ('float - defaults to zero'). An
        # 'if'/'depending' clause only qualifies *when* each alternative occurs, without
        # changing the union - consumed up to the next top-level comma.
        found = False
        while True:
            token = stream.peek()
            if token is None:
                return found
            kind, value = token
            if kind == 'period':
                stream.next()
                continue
            if kind == 'punct' and value in (',', '|'):
                stream.next()
                found = True
                continue
            if kind == 'word' and value.lower() == 'or':
                stream.next()
                found = True
                continue
            if kind == 'other' and value == '-':
                while not stream.at_end():  # trailing commentary
                    stream.next()
                return found
            if kind == 'word' and value.lower() in ('if', 'depending'):
                self._consume_condition_clause(stream)
                continue
            return found

    def _consume_condition_clause(self, stream):
        stream.next()  # 'if' / 'depending'
        while not stream.at_end():
            kind, value = stream.peek()
            if kind == 'punct' and value == ',':
                return  # the separator loop handles the comma
            if kind == 'punct' and value == '(':
                group = self._collect_paren_group(stream)
                if group and group[0] == ('word', 'or'):
                    # An '(or ...)' group inside the clause is a further alternative -
                    # consuming it would silently narrow the type.
                    raise TypeParseError('condition clause hides alternatives')
                continue
            stream.next()

    def _skip_articles(self, stream):
        while stream.peek_word() in ('a', 'an', 'the', 'single'):
            stream.next()

    def _peek_structural(self, stream, offset=0):
        # A word token in lowercase.
        token = stream.peek(offset)
        if token is not None and token[0] == 'word':
            return token[1].lower()
        return None

    def _parse_disjunct(self, stream, prior=None):
        annotation = self._parse_disjunct_core(stream, prior)
        return self._parse_suffix(stream, annotation)

    def _parse_disjunct_core(self, stream, prior):
        self._skip_articles(stream)
        structural = self._peek_structural(stream)

        if structural == 'any':
            if self._peek_structural(stream, 1) == 'combination':
                return self._parse_any_combination(stream, prior)
            if self._peek_structural(stream, 1) in ('sequence', 'sequences', 'list',
                                                    'lists', 'tuple', 'tuples', 'dict'):
                stream.next()  # 'any sequence of X' is just a sequence of X
                self._skip_articles(stream)
                structural = self._peek_structural(stream)

        if structural in ('sequence', 'sequences'):
            return self._parse_sequence(stream, prior)
        if structural in ('list', 'lists'):
            return self._parse_list(stream, prior)
        if structural in ('tuple', 'tuples'):
            return self._parse_tuple(stream, prior)
        if structural == 'dict':
            return self._parse_dict(stream, prior)
        if structural == 'callable':
            return self._parse_callable(stream)
        if structural == 'them' and prior:
            stream.next()  # 'or a sequence of them'
            return self._make_union(list(prior))
        if structural == 'type' and self._peek_punct(stream, '(', 1):
            # 'type(*default*)' - the type of whatever the caller supplied.
            stream.next()
            self._collect_paren_group(stream)
            self.generator.uses_any = True
            return 'Any'
        if structural == 'one' and self._peek_structural(stream, 1) == 'of':
            return self._parse_one_of_the_values(stream)
        if structural == 'bitwise' and self._peek_structural(stream, 1) == 'combination':
            # 'a bitwise combination of any of ``pygplates.ResolveTopologyType.boundary``
            # or ...' - OR-ing enum members produces a plain int.
            stream.next()
            stream.next()
            while stream.peek_word() in ('of', 'any'):
                stream.next()
            return self._make_union([self._parse_of_target(stream, prior), 'int'])
        if structural == 'class' and self._peek_structural(stream, 1) == 'object':
            return self._parse_class_object(stream)
        if self._peek_punct(stream, '('):
            # A tuple form written before the word 'tuple': '(x,y,z) tuple'.
            annotation = self._parse_tuple_form(stream, prior)
            stream.accept_word('tuples', 'tuple')
            return annotation

        # A plain atom, possibly '/'-joined ('string/``os.PathLike``').
        items = [self._parse_atom(stream)]
        while self._peek_punct(stream, '/'):
            stream.next()
            items.append(self._parse_atom(stream))
        return self._make_union(items)

    def _parse_suffix(self, stream, annotation):
        # A parenthesised group after a type is commentary ('(see table below)',
        # '(or a coverage or a sequence of coverages - see below)') - the markup-free
        # style guideline keeps alternatives and conditions out of the type fields
        # (they belong in ':returns:'/prose, with the type a flat union), so the group
        # never carries type information.
        while self._peek_punct(stream, '('):
            self._collect_paren_group(stream)
        return annotation

    def _parse_any_combination(self, stream, prior):
        stream.next()  # 'any'
        stream.next()  # 'combination'
        if stream.peek_word() == 'of':
            stream.next()
        token = stream.peek()
        if token is not None and token[0] == 'word' and \
                token[1].lower() in ('them', 'those', 'these'):
            # 'any combination of those four types' / 'of them' - the union of the
            # alternatives already listed (if known, else 'Any').
            while stream.peek() is not None and stream.peek()[0] in ('word', 'period'):
                stream.next()
            if prior:
                return self._make_union(list(prior))
            self.generator.uses_any = True
            return 'Any'
        # 'any combination of :class:`PropertyName` and the *PartitionProperty*
        # enumeration values' - an explicit union.
        items = [self._parse_disjunct(stream, prior)]
        while stream.peek_word() in ('and', 'or'):
            stream.next()
            items.append(self._parse_disjunct(stream, prior))
        return self._make_union(items)

    def _parse_one_of_the_values(self, stream):
        # 'One of the values in the *SortPartitioningPlates* table above'.
        stream.next()  # 'one'
        stream.next()  # 'of'
        while stream.peek_word() in ('the', 'values', 'in'):
            stream.next()
        token = stream.next()
        if token[0] != 'word':
            raise TypeParseError("expected an identifier after 'one of the values'")
        annotation = self._resolve_identifier(token[1])
        if annotation is None:
            raise TypeParseError("unknown identifier %r after 'one of the values'"
                                 % token[1])
        while stream.peek_word() in ('the', 'table', 'above', 'below'):
            stream.next()
        return annotation

    def _parse_class_object(self, stream):
        # 'a class object of the property type (derived from :class:`PropertyValue`)'
        # -> 'type[PropertyValue]'. A trailing 'except ...' narrows the set of classes
        # but not the annotation.
        stream.next()  # 'class'
        stream.next()  # 'object'
        annotation = 'type'
        while True:
            token = stream.peek()
            if token is None:
                break
            kind, value = token
            if kind == 'punct' and value == '(':
                # '(derived from PropertyValue)'.
                for group_kind, group_value in self._collect_paren_group(stream):
                    if group_kind == 'word':
                        resolved = self._resolve_identifier(group_value)
                        if resolved is not None:
                            annotation = 'type[%s]' % resolved
                            break
                continue
            if kind == 'word':
                if value.lower() == 'except':
                    while not stream.at_end():
                        stream.next()
                    break
                stream.next()
                continue
            break
        return annotation

    def _parse_sequence(self, stream, prior):
        stream.next()  # 'sequence'
        # Skip qualifiers: '(eg, ``list`` or ``tuple``)', 'such as a ``list`` or a
        # ``tuple``'.
        while True:
            if self._peek_punct(stream, '('):
                self._collect_paren_group(stream)
                continue
            if self._peek_structural(stream) in ('such', 'as', 'a', 'an', 'or', 'eg',
                                                 'list', 'tuple'):
                stream.next()
                continue
            break
        self.generator.uses_sequence = True
        if stream.peek_word() == 'of':
            stream.next()
            return 'Sequence[%s]' % self._parse_of_target(stream, prior)
        # 'Any sequence such as a ``list`` or a ``tuple``' - element type unspecified.
        self.generator.uses_any = True
        return 'Sequence[Any]'

    def _parse_list(self, stream, prior):
        stream.next()  # 'list'
        if self._peek_punct(stream, '['):
            arguments = self._parse_bracket_arguments(stream, prior)
            if len(arguments) != 1:
                raise TypeParseError('list[...] takes one element type')
            return 'list[%s]' % arguments[0]
        if stream.peek_word() == 'of':
            stream.next()
            return 'list[%s]' % self._parse_of_target(stream, prior)
        return 'list'

    def _parse_tuple(self, stream, prior):
        stream.next()  # 'tuple'
        if self._peek_punct(stream, '['):
            # 'tuple[A, B]' / homogeneous 'tuple[X, ...]'.
            arguments = self._parse_bracket_arguments(stream, prior)
            if not arguments:
                raise TypeParseError('tuple[...] takes at least one element type')
            return 'tuple[%s]' % ', '.join(arguments)
        if self._peek_punct(stream, '('):
            return self._parse_tuple_form(stream, prior)
        if stream.peek_word() == 'of':
            stream.next()
            if self._peek_punct(stream, '('):
                return self._parse_tuple_form(stream, prior)  # 'a tuple of (A, B)'
            count = _COUNT_WORDS.get(stream.peek_word())
            if count is not None:
                # Homogeneous: 'tuple of three X [or Y]' - a bare 'or' extends the
                # element type, as in '_parse_of_target'.
                stream.next()
                items = [self._parse_disjunct(stream, prior)]
                while stream.peek_word() == 'or':
                    self._warn_if_ambiguous_or(stream)
                    stream.next()
                    if stream.at_end():
                        break
                    items.append(self._parse_disjunct(stream, prior))
                element = self._make_union(items)
                return 'tuple[%s]' % ', '.join([element] * count)
            return 'tuple[%s, ...]' % self._parse_of_target(stream, prior)
        return 'tuple'

    def _parse_dict(self, stream, prior):
        stream.next()  # 'dict'
        if self._peek_punct(stream, '['):
            arguments = self._parse_bracket_arguments(stream, prior)
            if len(arguments) != 2:
                raise TypeParseError('dict[...] takes key and value types')
            return 'dict[%s, %s]' % (arguments[0], arguments[1])
        if stream.peek_word() == 'of':
            # 'a ``dict`` of scalar values' - a contents gloss in plain words, with no
            # machine-readable key/value types. Consume the words (stopping at anything
            # structural: a separator, condition, markup or punctuation).
            stream.next()
            while stream.peek() is not None and stream.peek()[0] == 'word' and \
                    stream.peek_word() not in ('or', 'and', 'if', 'depending', 'to'):
                stream.next()
            return 'dict'
        if stream.peek_word() != 'mapping':
            return 'dict'
        stream.next()
        if stream.peek_word() == 'each':
            stream.next()
        key_annotation = self._parse_mapping_item(stream, prior)
        if stream.peek_word() != 'to':
            raise TypeParseError("expected 'to' in dict mapping")
        stream.next()
        value_annotation = self._parse_mapping_item(stream, prior)
        return 'dict[%s, %s]' % (key_annotation, value_annotation)

    def _parse_mapping_item(self, stream, prior):
        # '``dict`` mapping each key (string) to a value (integer, float or string)' /
        # '``dict`` mapping each :class:`ScalarType` to a sequence of float'.
        self._skip_articles(stream)
        if stream.peek_word() in ('key', 'value') and self._peek_punct(stream, '(', 1):
            stream.next()
            return self._parse_union(_TokenStream(self._collect_paren_group(stream)))
        return self._parse_disjunct(stream, prior)

    def _parse_of_target(self, stream, prior=None):
        # 'list of (A, B) tuples'.
        if self._peek_punct(stream, '('):
            annotation = self._parse_tuple_form(stream, prior)
            stream.accept_word('tuples', 'tuple')
            return annotation
        # 'of' binds a bare 'or' union tightly ('sequence of float or
        # :class:`GeoTimeInstant`' is a sequence of the union); a comma returns to the
        # enclosing union ('list of :class:`PointOnSphere`, or tuple (...)').
        items = []
        items.append(self._parse_disjunct(stream, prior))
        while stream.peek_word() == 'or':
            self._warn_if_ambiguous_or(stream)
            stream.next()
            if stream.at_end():
                break
            items.append(self._parse_disjunct(stream, prior))
        return self._make_union(items)

    def _warn_if_ambiguous_or(self, stream):
        # A bare 'or' followed by another container ('list of A or list of B') binds to
        # the element - 'list[A | list[B]]' - which is almost never what the docstring
        # means. It still parses that way (some docstrings do mean it), but gets
        # reported so a ', or' (top-level alternative) fix can be considered.
        offset = 1
        while stream.peek_word(offset) in ('a', 'an', 'the'):
            offset += 1
        follower = stream.peek_word(offset)
        if follower in ('list', 'lists', 'sequence', 'sequences') or \
                (follower in ('tuple', 'tuples', 'dict') and
                 stream.peek_word(offset + 1) in ('of', 'mapping')):
            self.parse_warnings.append(
                "bare 'or %s' binds to the 'of' element - write ', or' if a top-level "
                'alternative was intended' % follower)

    def _parse_bracket_arguments(self, stream, prior=None):
        # The Python generic form after a container word - 'list[X]', 'tuple[A, B]',
        # 'tuple[X, ...]', 'dict[K, V]' - with ',' separating the arguments and '|' (or
        # 'or') uniting alternatives within one argument. Nesting recurses naturally
        # ('dict[K, list[V]]'). Returns the argument annotations in order.
        stream.expect('punct', '[')
        arguments = []
        while True:
            token = stream.peek()
            if token is None:
                raise TypeParseError('unterminated bracket form')
            kind, value = token
            if kind == 'punct' and value == ']':
                stream.next()
                return arguments
            if kind == 'punct' and value == ',':
                stream.next()
                continue
            if kind == 'ellipsis':
                stream.next()
                arguments.append('...')
                continue
            arguments.append(self._parse_bracket_argument(stream, prior))

    def _parse_bracket_argument(self, stream, prior):
        items = [self._parse_disjunct(stream, prior)]
        while self._peek_punct(stream, '|') or stream.peek_word() == 'or':
            stream.next()
            items.append(self._parse_disjunct(stream, prior))
        return self._make_union(items)

    def _parse_tuple_form(self, stream, prior=None):
        # '(A, B)' or '(A, B [, C])' (optional trailing elements, possibly nested) - the
        # optional-element form expands to a union of the possible tuple lengths.
        stream.expect('punct', '(')
        variants = self._parse_tuple_elements(stream, prior)
        stream.expect('punct', ')')
        annotations = ['tuple[%s]' % ', '.join(variant) for variant in variants]
        return self._make_union(annotations)

    def _parse_tuple_elements(self, stream, prior=None):
        required = []
        optional_variants = None
        while True:
            token = stream.peek()
            if token is None:
                raise TypeParseError('unterminated tuple form')
            kind, value = token
            if kind == 'punct' and value in (')', ']'):
                break
            if kind == 'punct' and value == ',':
                stream.next()
                continue
            if kind == 'punct' and value == '[':
                stream.next()
                optional_variants = self._parse_tuple_elements(stream, prior)
                stream.expect('punct', ']')
                break
            if kind == 'word' and value.lower() == 'or' and required:
                # An element that is itself a union: '(A, B or sequence of B)'.
                stream.next()
                required[-1] = self._make_union(
                    [required[-1], self._parse_disjunct(stream, prior)])
                continue
            required.append(self._parse_disjunct(stream, prior))
        variants = [required]
        if optional_variants is not None:
            for variant in optional_variants:
                variants.append(required + variant)
        return variants

    def _parse_callable(self, stream):
        stream.next()  # 'callable'
        self.generator.uses_callable = True
        parameters = []
        if self._peek_punct(stream, '('):
            # Eg, '(accepting single :class:`Property` argument)' - or, markup-free,
            # '(accepting single Property argument)' - the parameter types are the
            # class roles and resolvable identifiers mentioned inside the parentheses,
            # in order (the surrounding English words do not resolve).
            for kind, value in self._collect_paren_group(stream):
                if kind == 'role':
                    parameters.append(self._parse_role((kind, value)))
                elif kind == 'word':
                    annotation = self._resolve_identifier(value)
                    if annotation is not None:
                        parameters.append(annotation)
        elif stream.peek_word() == 'accepting':
            # The unparenthesised form: 'a callable accepting a single ... argument'.
            self._consume_condition_clause(stream)
        if parameters:
            return 'Callable[[%s], Any]' % ', '.join(parameters)
        return 'Callable[..., Any]'

    def _parse_atom(self, stream):
        token = stream.next()
        kind, value = token

        if kind == 'role':
            return self._parse_role(token)

        if kind == 'word':
            lowered = value.lower()
            # A placeholder bound by a 'where <name> is ...' gloss ('sequence of rings
            # (where ring is ...)') - checked first since a gloss is the most specific
            # statement of intent; the singular covers a pluralised use.
            binding = self._bindings.get(lowered)
            if binding is None and lowered.endswith('s'):
                binding = self._bindings.get(lowered[:-1])
            if binding is not None:
                return binding
            if lowered in _WORD_TYPES:
                annotation = _WORD_TYPES[lowered]
                if annotation == 'Any':
                    self.generator.uses_any = True
                return annotation
            if value == 'None':
                return 'None'
            if value.startswith('os.PathLike'):
                self.generator.uses_os = True
                return 'os.PathLike'
            if value == 'numpy.ndarray':
                self.generator.uses_numpy = True
                return 'numpy.ndarray'
            # A markup-free pygplates identifier: a class (including nested,
            # 'NetworkTriangulation.Triangle') or an enum value
            # ('PropertyReturn.exactly_one') standing for its enumeration type.
            annotation = self._resolve_identifier(value)
            if annotation is not None:
                # The same suffixes the role/emphasis forms take: a container-of
                # ('GpmlIrregularSampling of GpmlFiniteRotation') or 'containing'
                # contents clause (both describe the contents, not the type), and an
                # 'enumeration value' gloss.
                if stream.peek_word() == 'of':
                    stream.next()
                    self._parse_of_target(stream)
                elif stream.peek_word() == 'containing':
                    self._consume_condition_clause(stream)
                elif stream.peek_word() in ('enumeration', 'enumerated'):
                    stream.next()
                    if stream.peek_word() in ('value', 'values'):
                        stream.next()
                return annotation
            raise TypeParseError('unknown word %r' % value)

        raise TypeParseError('unexpected token %r' % (token,))

    def _resolve_identifier(self, text):
        """
        The annotation for a (possibly dotted) pygplates identifier: a class resolves to
        its qualified name, an enum *value* to its enumeration type's. None if 'text'
        does not resolve (or resolves to something foreign to pygplates).
        """
        text = text.strip()
        if text.startswith('pygplates.'):
            text = text[len('pygplates.'):]
        resolved = self.generator.resolve_dotted(text)
        if isinstance(resolved, type) and self.generator._is_pygplates_module(
                getattr(resolved, '__module__', '')):
            return resolved.__qualname__
        if resolved is not None and isinstance(resolved, int) and \
                self.generator.is_enum_class(type(resolved)):
            return type(resolved).__qualname__
        return None

    def _parse_role(self, token):
        role_name, role_text = token[1]
        # Only class-like roles describe types (':meth:'/':attr:' in a type field is prose).
        if role_name.split(':')[-1] not in ('class', 'exc'):
            raise TypeParseError('unexpected role :%s:' % role_name)
        text = role_text.strip()
        if '<' in text and text.endswith('>'):
            text = text[text.rindex('<') + 1:-1].strip()
        text = text.lstrip('~')
        if text.startswith('pygplates.'):
            text = text[len('pygplates.'):]
        resolved = self.generator.resolve_dotted(text)
        if isinstance(resolved, type):
            return resolved.__qualname__
        if text in ('int', 'float', 'str', 'bool', 'bytes', 'list', 'tuple', 'dict'):
            return text
        raise TypeParseError('unresolvable class reference %r' % role_text)

    def _peek_punct(self, stream, value, offset=0):
        token = stream.peek(offset)
        return token is not None and token[0] == 'punct' and token[1] == value

    def _collect_paren_group(self, stream):
        # Consume a balanced '(...)' group (the stream must be at the '(') and return the
        # tokens inside it.
        stream.expect('punct', '(')
        group = []
        depth = 1
        while True:
            token = stream.next()
            kind, value = token
            if kind == 'punct' and value == '(':
                depth += 1
            elif kind == 'punct' and value == ')':
                depth -= 1
                if depth == 0:
                    return group
            group.append(token)

    def _make_union(self, items):
        # Deduplicate preserving order; 'None' conventionally goes last.
        unique = []
        for item in items:
            if item not in unique:
                unique.append(item)
        if 'None' in unique and len(unique) > 1:
            unique.remove('None')
            unique.append('None')
        return ' | '.join(unique)


# ---------------------------------------------------------------------------
# Docstring signature parsing.
# ---------------------------------------------------------------------------

class Parameter(object):
    def __init__(self, name, kind='positional', optional=False, default_text=None):
        self.name = name
        self.kind = kind  # 'positional', 'varargs' or 'kwargs'
        self.optional = optional
        self.default_text = default_text


class OverloadBlock(object):
    def __init__(self, args_text, body_lines):
        self.args_text = args_text
        self.body_lines = body_lines

    def is_prose(self):
        # The generic overload marker ('__init__(...)') introduces prose shared by all
        # overloads, not a real signature.
        return self.args_text.strip() == '...'


def _split_docstring_blocks(name, doc):
    """
    Split a (possibly overloaded) docstring into a preamble plus signature blocks.

    Boost.Python concatenates sibling '.def()' docstrings, each starting with a column-0
    'name(args)' line. Body lines (indented 2 spaces by convention) belong to the preceding
    signature; anything before the first signature line is preamble prose.
    """
    preamble = []
    blocks = []
    for index, line in enumerate(doc.split('\n')):
        match = _SIG_LINE_RE.match(line)
        if match and match.group(1) == name:
            blocks.append(OverloadBlock(match.group(2), []))
        elif not blocks and line.strip() == name and \
                all(not previous.strip() for previous in preamble):
            # Docstring quirk: a leading signature missing its parentheses
            # (eg, 'NetworkTriangulation.get_triangles').
            blocks.append(OverloadBlock('', []))
        elif blocks:
            blocks[-1].body_lines.append(line)
        else:
            preamble.append(line)
    return preamble, blocks


def _parse_signature_parameters(args_text):
    # Square brackets mark optional parameters and can wrap a whole parameter
    # ('[x=Default]') or open mid-list ('index(x[,i[,j]])' in the Boost.Python
    # indexing-suite docstrings) - a parameter is optional if it *starts* inside brackets.
    # Parentheses (inside default values) protect commas from splitting.
    tokens = []  # (token text, started inside brackets)
    current = ''
    current_optional = False
    bracket_depth = 0
    paren_depth = 0

    def flush():
        if current.strip():
            tokens.append((current.strip(), current_optional))

    for char in args_text:
        if paren_depth == 0:
            if char == '[':
                bracket_depth += 1
                continue
            if char == ']':
                bracket_depth = max(0, bracket_depth - 1)
                continue
            if char == ',':
                flush()
                current = ''
                current_optional = False
                continue
        if char == '(':
            paren_depth += 1
        elif char == ')':
            paren_depth = max(0, paren_depth - 1)
        if not current.strip() and not char.isspace():
            current_optional = bracket_depth > 0
        current += char
    flush()

    parameters = []
    for token, optional in tokens:
        kind = 'positional'
        if token.startswith('**'):
            kind = 'kwargs'
            token = token[2:]
        elif token.startswith('*'):
            kind = 'varargs'
            token = token[1:]
        default_text = None
        if '=' in token:
            token, default_text = token.split('=', 1)
            token = token.strip()
            default_text = default_text.strip()
        if token:
            parameters.append(Parameter(token, kind, optional, default_text))
    return parameters


def _signature_subsumes(broad, narrow):
    """
    True if calls matching stub signature 'narrow' always also match 'broad': narrow's
    parameters are a leading subset of broad's (same names and annotations), broad's
    extra parameters all have defaults, and the return types match.
    """
    try:
        broad_args = ast.parse(broad + ': ...').body[0]
        narrow_args = ast.parse(narrow + ': ...').body[0]
    except SyntaxError:
        return False

    def annotation_key(node):
        return None if node is None else ast.dump(node)

    if annotation_key(broad_args.returns) != annotation_key(narrow_args.returns):
        return False
    broad_args, narrow_args = broad_args.args, narrow_args.args
    for arguments in (broad_args, narrow_args):
        if arguments.vararg or arguments.kwarg or arguments.kwonlyargs:
            return False
    if len(narrow_args.args) > len(broad_args.args):
        return False
    for narrow_arg, broad_arg in zip(narrow_args.args, broad_args.args):
        if narrow_arg.arg != broad_arg.arg:
            return False
        if annotation_key(narrow_arg.annotation) != annotation_key(broad_arg.annotation):
            return False
    # Broad's parameters beyond narrow's must all be defaulted.
    first_defaulted = len(broad_args.args) - len(broad_args.defaults)
    return len(narrow_args.args) >= first_defaulted


def _extract_type_fields(body_lines):
    """Return ({param name: raw type text}, raw rtype text or None) from a block body."""
    type_map = {}
    rtype = None
    for line in body_lines:
        match = _TYPE_FIELD_RE.match(line) or _BULLET_TYPE_FIELD_RE.match(line)
        if match:
            type_map.setdefault(match.group(1), match.group(2))
            continue
        match = _RTYPE_FIELD_RE.match(line) or _BULLET_RTYPE_FIELD_RE.match(line)
        if match and rtype is None:
            rtype = match.group(1)
    return type_map, rtype


# ---------------------------------------------------------------------------
# Docstring text processing (for embedding in the stub).
# ---------------------------------------------------------------------------

def _dedent_convention(lines):
    """
    Remove the conventional 2-space body indent (lines the docstring convention places at
    column 0 - eg, overload preamble prose - are left untouched, preserving any deeper
    relative indentation of code examples and ReST directives).
    """
    return [line[2:] if line.startswith('  ') else line for line in lines]


def _clean_docstring_text(lines, dedent=True):
    if dedent:
        lines = _dedent_convention(lines)
    lines = [line.rstrip() for line in lines]
    # Strip leading/trailing blank lines.
    while lines and not lines[0]:
        lines.pop(0)
    while lines and not lines[-1]:
        lines.pop()
    return '\n'.join(lines)


def _format_docstring(text, indent):
    """
    Return the source lines of a docstring literal (deterministic raw-string/escaping rule:
    plain if possible, raw if backslashes require it, fully escaped as a last resort).
    """
    if not text:
        return []
    prefix = ''
    if '"""' not in text and not text.endswith('"') and not text.endswith('\\'):
        if '\\' in text:
            prefix = 'r'
    else:
        text = text.replace('\\', '\\\\').replace('"""', '\\"\\"\\"')
        if text.endswith('"'):
            text = text[:-1] + '\\"'
    lines = text.split('\n')
    source = [indent + prefix + '"""' + lines[0]]
    for line in lines[1:]:
        source.append((indent + line) if line else '')
    source.append(indent + '"""')
    return source


# ---------------------------------------------------------------------------
# Stub generation.
# ---------------------------------------------------------------------------

_HEADER = '''\
# DO NOT EDIT - this file is generated from the built pygplates module.
# Regenerate with:
#   python pygplates/stub/generate_stub.py --module-dir <dir-of-built-pygplates-module> --output pygplates/stub/__init__.pyi
# (the 'pygplates-stub-test' ctest checks this file stays up-to-date).
'''


class StubGenerator(object):
    def __init__(self, module):
        self.module = module
        self.type_parser = TypeExpressionParser(self)
        # Deduplicated report of ':type:'/':rtype:' text that fell back to 'Any'.
        self.unparsed_report = set()
        # Deduplicated report of ':type:'/':rtype:' fields that are absent altogether
        # (also 'Any'); the two are separate worklists - one needs grammar work, the
        # other needs a docstring field adding.
        self.missing_report = set()
        # Docstrings with no ':rtype:' (so annotated '-> None' as a C++ 'void' function)
        # whose prose suggests they do return something.
        self.suspicious_rtype_report = set()
        self.warnings = set()
        self.uses_any = False
        self.uses_callable = False
        self.uses_classvar = False
        self.uses_numpy = False
        self.uses_overload = False
        self.uses_sequence = False
        self.uses_os = False

    # -- Helpers -----------------------------------------------------------

    def resolve_dotted(self, dotted):
        obj = self.module
        for part in dotted.split('.'):
            if not part:
                return None
            try:
                obj = getattr(obj, part)
            except AttributeError:
                return None
        return obj

    def is_enum_class(self, obj):
        # Boost.Python 'bp::enum_' types derive from int and carry 'names'/'values' dicts.
        return (isinstance(obj, type) and issubclass(obj, int) and
                isinstance(getattr(obj, 'names', None), dict) and
                isinstance(getattr(obj, 'values', None), dict))

    def _parse_type_text(self, raw_text, qualified):
        if not raw_text:
            self.missing_report.add(qualified)
            self.uses_any = True
            return 'Any'
        try:
            annotation = self.type_parser.parse(raw_text)
        except TypeParseError:
            self.unparsed_report.add('%s: %s' % (qualified, raw_text))
            self.uses_any = True
            annotation = 'Any'
        for warning in self.type_parser.parse_warnings:
            self.warnings.add('%s: %s' % (qualified, warning))
        return annotation

    def _is_pygplates_module(self, module_name):
        return module_name in _PYGPLATES_MODULES

    # -- Delegating docstrings ('Same as :meth:`X`.') -----------------------

    @staticmethod
    def _alias_target(body_lines):
        """The method name a block delegates to, or None if it is not a delegation."""
        for line in body_lines:
            line = line.strip()
            if not line:
                continue  # eg, 'Version.get_prerelease_suffix' has a leading blank line
            match = _ALIAS_RE.match(line)
            return match.group(1) if match else None
        return None

    def _lookup_alias_target(self, owner, target):
        if '.' in target:
            return self.resolve_dotted(target)
        if owner is not None:
            member = getattr(owner, target, None)
            if member is not None:
                return member
        return getattr(self.module, target, None)

    def _alias_type_fields(self, owner, body_lines, visited=None):
        """
        The ':type x:'/':rtype:' fields a delegating block inherits from its delegate.

        Returns ({}, None) when the block is not a delegation (or the delegate cannot be
        resolved). Delegations can chain, so this recurses through a 'visited' set.
        """
        target = self._alias_target(body_lines)
        if target is None:
            return {}, None
        visited = set() if visited is None else visited
        if target in visited or len(visited) >= 4:
            return {}, None  # cycle, or an implausibly long chain
        visited.add(target)

        delegate = self._lookup_alias_target(owner, target)
        doc = getattr(delegate, '__doc__', None)
        if not doc:
            return {}, None
        _, blocks = _split_docstring_blocks(target.rsplit('.', 1)[-1], doc)
        blocks = [block for block in blocks if not block.is_prose()]
        if not blocks:
            return {}, None

        type_map, rtype = _extract_type_fields(blocks[0].body_lines)
        if rtype is None:
            nested_map, rtype = self._alias_type_fields(owner, blocks[0].body_lines,
                                                        visited)
            for key, value in nested_map.items():
                type_map.setdefault(key, value)
        return type_map, rtype

    def _return_annotation(self, name, rtype, qualified, body_lines):
        """
        The stub return annotation for a Boost.Python method.

        The docstring convention omits ':rtype:' for a C++ 'void' function, so an absent
        field means 'None' - which is what a type checker needs (unlike 'Any', which would
        silently accept 'x = feature.set_boolean(True)').
        """
        if name == '__init__':
            return 'None'
        if rtype:
            return self._parse_type_text(rtype, '%s -> ' % qualified)
        self._note_void_return(qualified, body_lines)
        return 'None'

    def _note_void_return(self, qualified, body_lines):
        lines = [line.strip() for line in body_lines]
        summary = next((line for line in lines if line), '')
        if _RETURNS_PROSE_RE.match(summary) or \
                any(line.startswith(':returns:') or '**Return type**' in line
                    for line in lines):
            self.suspicious_rtype_report.add('%s: %r' % (qualified, summary[:120]))

    def _note_annotation_usage(self, signature_text):
        # Keep the imports block in sync with annotations hand-written in
        # MANUAL_OVERRIDES.
        if 'Any' in signature_text:
            self.uses_any = True
        if 'Callable[' in signature_text:
            self.uses_callable = True
        if 'ClassVar[' in signature_text:
            self.uses_classvar = True
        if 'numpy.' in signature_text:
            self.uses_numpy = True
        if 'Sequence[' in signature_text:
            self.uses_sequence = True
        if 'os.PathLike' in signature_text:
            self.uses_os = True

    # -- Top level ---------------------------------------------------------

    def generate(self):
        body = []

        body.extend(_format_docstring(_clean_docstring_text(
            (self.module.__doc__ or '').split('\n'), dedent=False), ''))
        body.append('')
        body.append('IMPORTS_PLACEHOLDER')
        body.append('')
        body.append('__version__: str')
        body.append('')
        # Called by the generated package '__init__.py' to tell pygplates its location.
        body.append('def _post_import(package_dir: str) -> None: ...')

        for name in sorted(vars(self.module)):
            if name.startswith('_') or name in _SKIP_MODULE_NAMES:
                continue
            obj = vars(self.module)[name]
            if inspect.ismodule(obj):
                continue
            module_name = getattr(obj, '__module__', None)
            if module_name is not None and not self._is_pygplates_module(module_name):
                continue  # leaked import (eg, 'namedtuple', 'partial')
            body.append('')
            body.extend(self._emit_module_member(name, obj))

        text = '\n'.join(body) + '\n'
        return _HEADER + text.replace('IMPORTS_PLACEHOLDER', self._imports_block(), 1)

    def _imports_block(self):
        lines = []
        if self.uses_numpy:
            lines.append('import numpy')
        if self.uses_os:
            lines.append('import os')
        typing_names = []
        if self.uses_any:
            typing_names.append('Any')
        if self.uses_callable:
            typing_names.append('Callable')
        if self.uses_classvar:
            typing_names.append('ClassVar')
        if self.uses_sequence:
            typing_names.append('Sequence')
        if self.uses_overload:
            typing_names.append('overload')
        if typing_names:
            if lines:
                lines.append('')
            lines.append('from typing import %s' % ', '.join(typing_names))
        return '\n'.join(lines)

    def _emit_module_member(self, name, obj):
        if self.is_enum_class(obj):
            return self._emit_enum(name, obj)
        if isinstance(obj, type):
            return self._emit_class(name, obj, indent='')
        if inspect.isfunction(obj):
            return self._emit_python_function(name, obj, qualified=name, in_class=False,
                                             indent='')
        if callable(obj):
            return self._emit_boost_function(name, obj, qualified=name, in_class=False,
                                            is_static=False, indent='')
        # Module-level data.
        return ['%s: %s' % (name, self._annotation_for_value(obj))]

    def _annotation_for_value(self, value):
        value_type = type(value)
        if self._is_pygplates_module(getattr(value_type, '__module__', '')):
            return value_type.__qualname__
        if value_type.__module__ == 'builtins':
            return value_type.__name__
        self.uses_any = True
        return 'Any'

    # -- Classes -----------------------------------------------------------

    def _emit_class(self, name, cls, indent):
        member_indent = indent + '    '
        lines = ['%sclass %s%s:' % (indent, name, self._format_bases(cls))]

        doc_text = _clean_docstring_text((cls.__doc__ or '').split('\n'), dedent=False)
        body = _format_docstring(doc_text, member_indent)

        qualified_prefix = cls.__qualname__
        for member_name in sorted(vars(cls)):
            if member_name in _SKIP_CLASS_ATTRS:
                continue
            if member_name.startswith('_') and not member_name.startswith('__'):
                continue  # private helper (eg, '_get_value')
            member = vars(cls)[member_name]
            member_lines = self._emit_class_member(cls, member_name, member,
                                                   qualified_prefix, member_indent)
            if member_lines:
                if body:
                    body.append('')
                body.extend(member_lines)

        if not body:
            return [lines[0][:-1] + ': ...']
        return lines + body

    def _format_bases(self, cls):
        bases = []
        for base in cls.__bases__:
            base_module = getattr(base, '__module__', '')
            if self._is_pygplates_module(base_module):
                bases.append(base.__qualname__)
            elif base_module == 'builtins' and base is not object:
                bases.append(base.__name__)
            # Anything else ('Boost.Python.instance') is implementation detail.
        if bases:
            return '(%s)' % ', '.join(bases)
        return ''

    def _emit_class_member(self, cls, name, member, qualified_prefix, indent):
        qualified = '%s.%s' % (qualified_prefix, name)

        if name == '__hash__' and member is None:
            # Unhashable type (Boost.Python sets '__hash__ = None' when unhashable).
            # The ignore is the standard typeshed idiom - mypy objects to overriding
            # 'object.__hash__' with None.
            self.uses_classvar = True
            return ['%s__hash__: ClassVar[None]  # type: ignore[assignment]' % indent]
        if member is None:
            return []

        if self.is_enum_class(member):
            return self._emit_enum(name, member, indent)
        if isinstance(member, type):
            return self._emit_class(name, member, indent)
        if isinstance(member, property):
            # Boost.Python 'add_static_property' descriptors ('Earth.mean_radius_in_kms',
            # 'FeatureType.gpml_topological_network') read as plain class attributes.
            if type(member).__name__ == 'StaticProperty':
                return self._emit_static_property(cls, name, member, qualified, indent)
            return self._emit_property(name, member, qualified, indent)

        static_descriptor = inspect.getattr_static(cls, name)
        if isinstance(static_descriptor, staticmethod):
            function = static_descriptor.__func__
            if inspect.isfunction(function):
                return self._emit_python_function(name, function, qualified,
                                                 in_class=True, indent=indent,
                                                 is_static=True)
            return self._emit_boost_function(name, function, qualified, in_class=True,
                                            is_static=True, indent=indent, owner=cls)

        if inspect.isfunction(member):
            # Pure-Python member injected from src/qt-resources/python/api/*.py.
            return self._emit_python_function(name, member, qualified, in_class=True,
                                             indent=indent)
        if callable(member):
            return self._emit_boost_function(name, member, qualified, in_class=True,
                                            is_static=False, indent=indent, owner=cls)

        # Class data attribute (eg, 'Earth.equatorial_radius_in_kms', 'StrainRate.zero').
        self.uses_classvar = True
        return ['%s%s: ClassVar[%s]' % (indent, name, self._annotation_for_value(member))]

    # -- Enums -------------------------------------------------------------

    def _emit_enum(self, name, cls, indent=''):
        self.uses_classvar = True
        member_indent = indent + '    '
        lines = ['%sclass %s(int):' % (indent, name)]
        doc_text = _clean_docstring_text((cls.__doc__ or '').split('\n'), dedent=False)
        lines.extend(_format_docstring(doc_text, member_indent))
        qualname = cls.__qualname__
        # Members ordered by integer value (their C++ declaration order).
        for value, member_name in sorted((int(member), member_name)
                                         for member_name, member in cls.names.items()):
            lines.append('%s%s: ClassVar[%s]' % (member_indent, member_name, qualname))
        lines.append('%snames: ClassVar[dict[str, %s]]' % (member_indent, qualname))
        lines.append('%svalues: ClassVar[dict[int, %s]]' % (member_indent, qualname))
        return lines

    # -- Properties --------------------------------------------------------

    def _emit_static_property(self, cls, name, prop, qualified, indent):
        doc = prop.__doc__ or ''
        raw_type = None
        for line in doc.split('\n'):
            match = _PROPERTY_TYPE_FIELD_RE.match(line)
            if match:
                raw_type = match.group(1)
                break
        if raw_type is not None:
            annotation = self._parse_type_text(raw_type, qualified)
        else:
            # No ':type:' field - infer from the actual class-attribute value.
            annotation = self._annotation_for_value(getattr(cls, name))
        self.uses_classvar = True
        lines = ['%s%s: ClassVar[%s]' % (indent, name, annotation)]
        doc_text = _clean_docstring_text(doc.split('\n'))
        lines.extend(_format_docstring(doc_text, indent))  # attribute docstring
        return lines

    def _emit_property(self, name, prop, qualified, indent):
        doc = prop.__doc__ or ''
        raw_type = None
        for line in doc.split('\n'):
            match = _PROPERTY_TYPE_FIELD_RE.match(line)
            if match:
                raw_type = match.group(1)
                break
        annotation = self._parse_type_text(raw_type, qualified)

        lines = ['%s@property' % indent,
                 '%sdef %s(self) -> %s:' % (indent, name, annotation)]
        doc_text = _clean_docstring_text(doc.split('\n'))
        doc_lines = _format_docstring(doc_text, indent + '    ')
        if doc_lines:
            lines.extend(doc_lines)
        else:
            lines[-1] += ' ...'
        if prop.fset is not None:
            lines.append('%s@%s.setter' % (indent, name))
            lines.append('%sdef %s(self, value: %s) -> None: ...' % (indent, name,
                                                                     annotation))
        return lines

    # -- Boost.Python functions/methods ------------------------------------

    def _emit_boost_function(self, name, function, qualified, in_class, is_static, indent,
                             owner=None):
        doc = function.__doc__ or ''
        preamble, blocks = _split_docstring_blocks(name, doc)

        prose_lines = list(preamble)
        overloads = []
        for block in blocks:
            if block.is_prose():
                prose_lines.extend(block.body_lines)
            else:
                overloads.append(block)
        prose_text = _clean_docstring_text(prose_lines)

        overrides = MANUAL_OVERRIDES.get(qualified, {})

        if not overloads:
            if overrides:
                # Manual replacement signature(s) for a member whose docstring has no
                # signature line ('Vector3D.create_normalized').
                signatures = [overrides[index] for index in sorted(overrides)]
                for signature in signatures:
                    self._note_annotation_usage(signature)
                lines = []
                emit_overloads = len(signatures) > 1
                if emit_overloads:
                    self.uses_overload = True
                for signature in signatures:
                    if lines:
                        lines.append('')
                    lines.extend(self._emit_def(
                        signature, prose_text, is_static, indent,
                        ['@overload'] if emit_overloads else []))
                return lines
            # No parseable signature - fall back to the fixed dunder table, else be
            # permissive (and note it in the report if the member was documented).
            fallback = _DUNDER_FALLBACK_SIGNATURES.get(name)
            if fallback is not None:
                parameters, return_type = fallback
                if 'Any' in parameters or return_type == 'Any':
                    self.uses_any = True
                if in_class and not is_static:
                    parameters = ('self, ' + parameters) if parameters else 'self'
                signature = 'def %s(%s) -> %s' % (name, parameters, return_type)
            else:
                # (A documented '__init__' without a signature line is the deliberate
                # 'cannot directly instantiate this class' / pickle-only pattern.)
                if doc.strip() and name != '__init__':
                    self.warnings.add(
                        '%s: documented but has no parseable signature line' % qualified)
                self.uses_any = True
                parameters = '*args: Any, **kwargs: Any'
                if in_class and not is_static:
                    parameters = 'self, ' + parameters
                return_type = 'None' if name == '__init__' else 'Any'
                signature = 'def %s(%s) -> %s' % (name, parameters, return_type)
            return self._emit_def(signature, prose_text, is_static, indent,
                                  decorators=[])

        # Boost.Python registers some members once per C++ overload with identical
        # docstrings (eg, the const/non-const 'PropertyValueVisitor.visit_*' pairs) -
        # deduplicate, merging any differing documentation.
        rendered = []  # [signature, doc_text]
        for index, block in enumerate(overloads):
            signature = overrides.get(index)
            if signature is None:
                signature = self._build_signature(name, block, qualified, in_class,
                                                  is_static, owner)
            else:
                self._note_annotation_usage(signature)
            doc_text = _clean_docstring_text(block.body_lines)
            duplicate = next((entry for entry in rendered if entry[0] == signature), None)
            if duplicate is not None:
                if doc_text and doc_text not in duplicate[1]:
                    duplicate[1] += '\n\n' + doc_text
                continue
            # An overload fully subsumed by an earlier broader one (eg, the
            # 'RotationModel.__init__(rotation_model)' convenience overload) is emitted
            # *before* its subsumer: at runtime Boost.Python picks it by registration
            # priority, but mypy rejects a subsumed overload that comes after the
            # broader one ('will never be matched') while accepting narrower-first.
            subsumer = next((position for position, entry in enumerate(rendered)
                             if _signature_subsumes(entry[0], signature)), len(rendered))
            rendered.insert(subsumer, [signature, doc_text])

        if prose_text:
            rendered[0][1] = prose_text + \
                ('\n\n' + rendered[0][1] if rendered[0][1] else '')
        lines = []
        emit_overloads = len(rendered) > 1
        if emit_overloads:
            self.uses_overload = True
        for signature, doc_text in rendered:
            decorators = ['@overload'] if emit_overloads else []
            if lines:
                lines.append('')
            lines.extend(self._emit_def(signature, doc_text, is_static, indent,
                                        decorators))
        return lines

    def _build_signature(self, name, block, qualified, in_class, is_static, owner=None):
        parameters = _parse_signature_parameters(block.args_text)
        type_map, rtype = _extract_type_fields(block.body_lines)

        # A body that just delegates ('Same as :meth:`get_resolved_boundary`.') has no
        # fields of its own - inherit the delegate's. Fields written here always win.
        alias_types, alias_rtype = self._alias_type_fields(owner, block.body_lines)
        for key, value in alias_types.items():
            type_map.setdefault(key, value)
        if rtype is None:
            rtype = alias_rtype

        if not all(re.match(r'^\w+$', parameter.name) for parameter in parameters):
            # A malformed signature line (eg, a stray C++ type in a parameter name) - stay
            # permissive rather than emitting an invalid stub.
            self.warnings.add('%s: malformed signature line %r'
                              % (qualified, block.args_text))
            self.uses_any = True
            pieces = ['*args: Any', '**kwargs: Any']
            if in_class and not is_static:
                pieces.insert(0, 'self')
            return_type = self._return_annotation(name, rtype, qualified,
                                                  block.body_lines)
            return 'def %s(%s) -> %s' % (name, ', '.join(pieces), return_type)

        pieces = []
        if in_class and not is_static:
            pieces.append('self')
        seen_default = False
        for parameter in parameters:
            if parameter.kind == 'kwargs':
                # Eg, '[**output_parameters]' - '**kwargs' never takes a default.
                self.uses_any = True
                pieces.append('**%s: Any' % parameter.name)
                continue
            if parameter.kind == 'varargs':
                self.uses_any = True
                pieces.append('*%s: Any' % parameter.name)
                seen_default = False  # keyword-only params after '*args' need no default
                continue
            annotation = self._parse_type_text(type_map.get(parameter.name),
                                               '%s(%s)' % (qualified, parameter.name))
            default = self._render_default(parameter)
            if default is None and seen_default:
                # A required parameter after an optional one is not valid Python - keep
                # the stub valid and note it.
                default = '...'
                self.warnings.add('%s: required parameter %r follows an optional one'
                                  % (qualified, parameter.name))
            if default == 'None' and annotation != 'Any' and 'None' not in annotation:
                annotation += ' | None'  # PEP 484 prohibits implicit Optional
            piece = '%s: %s' % (parameter.name, annotation)
            if default is not None:
                piece += ' = %s' % default
                seen_default = True
            pieces.append(piece)

        return_type = self._return_annotation(name, rtype, qualified, block.body_lines)
        return 'def %s(%s) -> %s' % (name, ', '.join(pieces), return_type)

    def _render_default(self, parameter):
        if parameter.default_text is not None:
            text = parameter.default_text
            try:
                ast.literal_eval(text)
                return text
            except (ValueError, SyntaxError):
                pass
            # A resolvable pygplates name (eg, 'PropertyReturn.exactly_one'); a
            # 'pygplates.' prefix is dropped (the stub's own scope provides the name).
            if text.startswith('pygplates.'):
                text = text[len('pygplates.'):]
            if self.resolve_dotted(text) is not None:
                return text
            return '...'
        if parameter.optional:
            return '...'
        return None

    def _emit_def(self, signature, doc_text, is_static, indent, decorators):
        lines = []
        for decorator in decorators:
            lines.append(indent + decorator)
        if is_static:
            lines.append(indent + '@staticmethod')
        doc_lines = _format_docstring(doc_text, indent + '    ')
        if doc_lines:
            lines.append(indent + signature + ':')
            lines.extend(doc_lines)
        else:
            lines.append(indent + signature + ': ...')
        return lines

    # -- Pure-Python functions (injected from qt-resources) ----------------

    def _emit_python_function(self, name, function, qualified, in_class, indent,
                              is_static=False):
        # 'inspect.getdoc()' normalizes the body indentation to column 0.
        doc = inspect.getdoc(function) or ''
        preamble, blocks = _split_docstring_blocks(name, doc)

        doc_lines = list(preamble)
        type_map = {}
        rtype = None
        for block in blocks:
            block_types, block_rtype = _extract_type_fields(block.body_lines)
            for key, value in block_types.items():
                type_map.setdefault(key, value)
            if rtype is None:
                rtype = block_rtype
            doc_lines.extend(block.body_lines)
        doc_text = _clean_docstring_text(doc_lines, dedent=False)

        overrides = MANUAL_OVERRIDES.get(qualified, {})
        signature = overrides.get(0)
        if signature is not None:
            self._note_annotation_usage(signature)
        else:
            try:
                python_signature = inspect.signature(function)
            except (TypeError, ValueError):
                python_signature = None
            pieces = []
            if python_signature is not None:
                for index, parameter in enumerate(python_signature.parameters.values()):
                    if index == 0 and in_class and not is_static:
                        # The injected functions name their first parameter after the class
                        # (eg, 'property', 'feature') - it is really 'self'.
                        pieces.append('self')
                        continue
                    if parameter.kind == inspect.Parameter.VAR_POSITIONAL:
                        self.uses_any = True
                        pieces.append('*%s: Any' % parameter.name)
                        continue
                    if parameter.kind == inspect.Parameter.VAR_KEYWORD:
                        self.uses_any = True
                        pieces.append('**%s: Any' % parameter.name)
                        continue
                    # Real annotations (if ever added to the injected sources) win.
                    if parameter.annotation is not inspect.Parameter.empty:
                        annotation = getattr(parameter.annotation, '__qualname__',
                                             str(parameter.annotation))
                    else:
                        annotation = self._parse_type_text(
                            type_map.get(parameter.name),
                            '%s(%s)' % (qualified, parameter.name))
                    default = None
                    if parameter.default is not inspect.Parameter.empty:
                        default = self._render_python_default(parameter.default)
                        if default == 'None' and annotation != 'Any' and \
                                'None' not in annotation:
                            annotation += ' | None'  # no implicit Optional
                    piece = '%s: %s' % (parameter.name, annotation)
                    if default is not None:
                        piece += ' = %s' % default
                    pieces.append(piece)
            else:
                self.uses_any = True
                pieces = ['*args: Any', '**kwargs: Any']
                if in_class and not is_static:
                    pieces.insert(0, 'self')
            if name == '__init__':
                return_type = 'None'
            else:
                return_type = self._parse_type_text(rtype, '%s -> ' % qualified)
            signature = 'def %s(%s) -> %s' % (name, ', '.join(pieces), return_type)
        return self._emit_def(signature, doc_text, is_static, indent, decorators=[])

    def _render_python_default(self, default):
        # Enum members before primitives - a Boost.Python enum member *is* an int, and
        # its repr ('pygplates.PropertyReturn.exactly_one') is not valid in stub scope.
        default_type = type(default)
        if self._is_pygplates_module(getattr(default_type, '__module__', '')) and \
                self.is_enum_class(default_type):
            for member_name, member in sorted(default_type.names.items()):
                if int(member) == int(default):
                    return '%s.%s' % (default_type.__qualname__, member_name)
            return '...'
        if default is None or isinstance(default, (bool, int, float, str)):
            return repr(default)
        return '...'


# ---------------------------------------------------------------------------
# Main.
# ---------------------------------------------------------------------------

def _bootstrap_dll_directories():
    # On Windows with Python >= 3.8 (outside conda) extension-module dependency DLLs are no
    # longer found via PATH - register every existing PATH entry, reversed since
    # 'os.add_dll_directory()' appears to insert at the front of the DLL search order.
    # (Mirrors doc-python-api/conf.py.in; conda already ensures dependencies are found.)
    if platform.system() == 'Windows' and sys.version_info >= (3, 8) and \
            not os.environ.get('CONDA_PREFIX'):
        env_path = os.environ.get('PATH')
        if env_path:
            for dll_path in reversed(env_path.split(os.pathsep)):
                if dll_path and os.path.exists(dll_path):
                    os.add_dll_directory(os.path.abspath(dll_path))


def main():
    parser = argparse.ArgumentParser(
        description='Generate the pygplates type stub from the built pygplates module.')
    parser.add_argument('--module-dir', required=True,
                        help='directory containing the built pygplates module')
    parser.add_argument('--output', help='write the generated stub to this path')
    parser.add_argument('--check', metavar='COMMITTED_STUB',
                        help='compare against the committed stub; exit 1 if it is stale')
    args = parser.parse_args()

    sys.path.insert(0, args.module_dir)
    _bootstrap_dll_directories()
    import pygplates

    generator = StubGenerator(pygplates)
    stub_text = generator.generate()

    for line in sorted(generator.warnings):
        print('WARNING: %s' % line, file=sys.stderr)
    def report(header, entries):
        if entries:
            print(header, file=sys.stderr)
            for entry in sorted(entries):
                print('  %s' % entry, file=sys.stderr)

    report('Unparsed type expressions (emitted as Any) - the docstring-improvement '
           'worklist:', generator.unparsed_report)
    report("Missing ':type'/':rtype:' fields (emitted as Any) - the docstring-improvement "
           'worklist:', generator.missing_report)
    report("Suspicious ':rtype:' omissions (emitted as '-> None' - a C++ 'void' function "
           'omits the field, but this prose suggests a return value):',
           generator.suspicious_rtype_report)

    # Self-gate: never write or compare a syntactically invalid stub.
    try:
        ast.parse(stub_text, filename='__init__.pyi')
    except SyntaxError as error:
        print('ERROR: generated stub is not valid Python: %s' % error, file=sys.stderr)
        return 2

    if args.output:
        with io.open(args.output, 'w', encoding='utf-8', newline='\n') as output_file:
            output_file.write(stub_text)

    if args.check:
        # Universal-newline decoding makes the comparison immune to CRLF checkouts.
        with io.open(args.check, 'r', encoding='utf-8') as committed_file:
            committed_text = committed_file.read()
        if committed_text != stub_text:
            diff = list(difflib.unified_diff(
                committed_text.split('\n'), stub_text.split('\n'),
                fromfile='committed %s' % args.check, tofile='regenerated', lineterm=''))
            max_diff_lines = 200
            for line in diff[:max_diff_lines]:
                print(line)
            if len(diff) > max_diff_lines:
                print('... diff truncated (%d more lines)' % (len(diff) - max_diff_lines))
            print()
            print('The committed pygplates stub is out of date with the built pygplates '
                  'module.')
            print('Regenerate it with:')
            print('  %s %s --module-dir %s --output %s'
                  % (sys.executable, os.path.abspath(__file__), args.module_dir,
                     args.check))
            return 1

    return 0


if __name__ == '__main__':
    sys.exit(main())
