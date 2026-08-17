---
name: api-docstring
description: Conventions for writing or reviewing pyGPlates Python API docstrings (signature-first line, type fields, overloads, math markup). Use whenever adding or editing a docstring in src/api/*.cc or src/qt-resources/python/api/*.py, or reviewing one.
---

# pyGPlates API docstring conventions

Read @doc-python-api/README.md — it is the authoritative reference and this skill is a summary of
its "Docstring conventions" and "Math markup" sections. Read `process_docstring` in
`doc-python-api/conf.py.in` before changing docstring *layout* (as opposed to wording).

## Where docstrings live

Two locations, both compiled into the module:

- `src/api/*.cc` — the C++ (Boost.Python) side
- `src/qt-resources/python/api/*.py` — the Python side

A given entity's docstring lives in exactly one of them; edit it where the entity is defined.
Search both when looking for one, and **any convention or style sweep must cover both**.

## Two consumers, one style

The docstrings feed Sphinx autodoc *and* `pygplates/stub/generate_stub.py`, which parses the type
fields to produce `pygplates/stub/__init__.pyi`. Wording is therefore load-bearing: a "harmless"
rephrasing of a `:type:` field can change the generated stub and fail `pygplates-stub-test`.

## Signature and body

Autodoc cannot introspect a Boost.Python function, so the **first line of the docstring is the
call signature at column 0**, with no types, and the body is indented two spaces:

```
get_geometry([property_query], [property_return=PropertyReturn.exactly_one])
  Return the geometry ...

  :param property_query: ...
  :type property_query: PropertyName, or callable (accepting single Property argument)
  :rtype: GeometryOnSphere, or None
```

Optional arguments use `[name=Default]`. Every method needs a docstring, including `__init__()`.
Never set `maximum_signature_line_length` — it breaks this pseudo-arglist rendering.

## Overloads

Boost.Python concatenates the docstrings of overloaded `def()`s, and each overload gets its own
signature-led block with its own Parameters/Raises/example. `process_docstring` splits on the
column-0 signature lines and re-emits the extras as directives.

## Type fields (`:type x:` / `:rtype:`)

1. **100% markup-free.** No `:class:`/`:meth:` roles, no ``` ``literals`` ```, no `*emphasis*`.
   One piece of inline markup anywhere in the field kills auto-linking for the *entire* field.
2. **Hybrid syntax**: prose where unambiguous (`list of GeometryOnSphere`), Python brackets when
   tuples/dicts/nesting appear (`dict[FeatureId, list[Feature]]`).
3. **Top-level union separator is `, or`.** `|` is only used inside brackets
   (`list[PointOnSphere | None]`).
4. **Never `/` as a union separator** — Sphinx does not split on it, so neither half links. Write
   `str, or os.PathLike`. Sole exception: a union as a prose container's element
   (`sequence of str/os.PathLike`), where `, or` would rebind to the top level.
5. **No conditionals.** "(if *return_x* is ``True``)" belongs in `:returns:`, where roles and
   emphasis are welcome. `:rtype:` is the flat union of every possibility.
6. **Real Python type names, singular** — `str` not `string`/`strings`, `int` not `integer`,
   plus `bool`, `float`. Plurals and non-types never link; write `list of str`.
7. **Field order and spelling**: `:returns:` (prose, optional) directly before `:rtype:`. Spell
   it `:returns:`, never `:return:`.
8. Enum-value lists keep the values un-italicised; "returns the *default* argument" is written
   `type(default)`; numpy arrays are `numpy.ndarray`. `None` never links (it is data, not a class).

## Math

Escape underscores inside `\text{...}` (`\text{geometry\_final}`) — MathJax 3+ rejects a bare `_`
there. Underscores outside `\text{}` are ordinary subscripts and need no escaping. Do not set
`mathjax_path`. A clean Sphinx build proves nothing about math rendering; open the page.

## After editing

Rebuild the docs from scratch (`/docs-build`) — Sphinx runs with `-W`, so warnings are errors —
and re-run `pygplates-stub-test` via `ctest --test-dir build-pygplates -C Release`, since the
stub is generated from these fields.
