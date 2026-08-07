# Generating the pyGPlates Python API documentation

This directory holds the Sphinx sources for the pyGPlates Python API reference. Sphinx builds
the HTML from the docstrings compiled into the `pygplates` module — it does not read the C++
sources, it *imports the built module* and reads `__doc__`.

## Building

Sphinx must run in the **same Python interpreter that pyGPlates was built against**, because
`autodoc`/`autosummary` import the `pygplates` extension module in-process. So the build target
runs Sphinx as `python -m sphinx` using exactly `GPLATES_PYTHON_EXECUTABLE`, rather than looking
for a `sphinx-build` on the `PATH` (one found there generally belongs to some other Python, and
`pygplates` then fails to import with an unhelpful "no module named pygplates").

Create the platform build environment, then overlay the documentation dependencies:

```bash
conda env create -n gplates -f env.Windows.yml   # or env.macOS.yml / env.Linux.yml
conda env update -n gplates -f env.docs.yml      # sphinx + sphinx_rtd_theme
conda activate gplates
```

See `BUILD-Windows.md` / `BUILD-macOS.md` / `BUILD-Linux.md` for the full build setup. At CMake
configure time you should see:

```
-- Looking for sphinx... - found <version> in <python>
```

If instead it reports `NOT found in <python>`, Sphinx is missing from *that* interpreter and no
documentation target is created.

Then build the `doc-python-api` target — the `doc-python-api` project in Visual Studio, or:

```bash
cmake --build <build-dir> --config Release --target doc-python-api
```

The result is `<build-dir>/doc-python-api/html/index.html`.

Warnings are errors (`-W`), so the build fails on anything Sphinx complains about; it also builds
in parallel (`-j auto`).

> **Editing `conf.py.in` does not always trigger a rebuild.** Sphinx invalidates its cached
> doctrees when *config values* change, not when conf.py *code* changes — so editing a handler
> such as `process_docstring` leaves the previously built HTML in place. Delete
> `<build-dir>/doc-python-api/_doctrees` (or run Sphinx by hand with `-E`) when verifying such a
> change.

> **Build from scratch for anything you publish.** An incremental rebuild produces HTML identical
> to a from-scratch build except for `searchindex.js`, where it silently drops *all* index entries
> (`indexentries` goes from 854 to 0, while `terms`, `docnames` and the rest are unchanged) —
> reproducible across repeated incremental runs. Search still works on page text, but index-entry
> matches are lost. Delete `<build-dir>/doc-python-api/_doctrees` (and `generated/`) first, or
> pass `-E`.

## Docstring conventions

The docstrings are hand-written in the C++ sources under `src/api/` and in the Python-side API
files under `src/qt-resources/python/api/`. **Both** are compiled into the module, so a
convention or style sweep has to cover both.

### Signature and body

Autodoc cannot introspect a Boost.Python function, so the **first line of the docstring is the
signature** and the body is indented two spaces (`autodoc_docstring_signature` is on):

```
wrap(geometry, [tessellate_degrees])
  Wrap a geometry to the range ...

  :param geometry: the geometry to wrap
  :type geometry: GeometryOnSphere
```

Optional arguments use the bracket notation `[name=Default]`. This is not valid Python syntax, so
Sphinx renders it with `_pseudo_parse_arglist()`; that is fine, but it is also why
**`maximum_signature_line_length` must not be set** — the multi-line signature writer only knows
how to wrap real parameter nodes and emits the `[`/`]` markers with unbalanced HTML around them.

Every documented method needs such a signature, including `__init__`. A class that cannot be
constructed directly still needs an empty one:

```
__init__()
```

Without it, autosummary reports `error while formatting arguments for ...: <Boost.Python.function
object> is not a Python function`.

### Overloads

Boost.Python concatenates the docstrings of overloaded `def()`s into a single `__doc__`, and
pyGPlates deliberately gives each overload its own Parameters/Raises/example block. Extra
overloads are therefore written as further signature-led blocks in the same docstring.

Sphinx cannot render that natively, so `process_docstring` in `conf.py.in` splits the docstring on
those signature lines, dedents each block, and re-emits every extra one as a `py:method` /
`py:function` directive (with `:no-index:`, and `:staticmethod:` where applicable) whose content
is that block's body. Read that function before changing docstring layout — in particular it also
protects a `.. versionadded::` / `.. versionchanged::` written flush at column zero.

### Type fields

`:type x:` / `:rtype:` fields feed **two** consumers: Sphinx auto-links them, and
`pygplates/stub/generate_stub.py` parses them to produce the type stub
`pygplates/stub/__init__.pyi` (see `pygplates/stub/PLAN.md`). One style serves both.

1. **`:type x:` and `:rtype:` contain only a type expression, 100% markup-free** — no
   `:class:`/`:meth:` roles, no ``` ``literals`` ```, no `*emphasis*`. Sphinx auto-links every
   identifier in a type field, but a *single* piece of inline markup anywhere in the field
   disables auto-linking for the whole field.
2. **Hybrid syntax**: natural prose where unambiguous — `list of GeometryOnSphere`,
   `FiniteRotation, or None` — and Python bracket syntax whenever tuples, dicts or nesting
   appear: `tuple[GeometryOnSphere, dict]`, `dict[FeatureId, list[Feature]]`.
3. **Top-level union separator is `, or`** (house English); `|` is used inside brackets, eg
   `list[PointOnSphere | None]`.
4. **No conditions in type fields.** Clauses like "(if *return_x* is ``True``)" or "depending
   on ..." belong in `:returns:` (or the `:param:` description), where roles and emphasis are
   welcome. The `:rtype:` is the flat union of every possibility — type checkers cannot use the
   conditions anyway.
5. **Field order**: `:returns:` (optional, prose) directly before `:rtype:`; spell it `:returns:`,
   not `:return:`.
6. **Use the real Python type names, singular** — `str` (not `string`/`strings`), `int` (not
   `integer`/`integers`), `bool`, `float`. Only these link: each identifier is resolved as a
   `py:class` cross-reference, and `string` is not a Python type (the stdlib `string` *module* is
   unrelated, and a `py:class` reference never matches a module). Plurals never link either, so
   write `list of str`, not `list of strings`.
7. **Never separate a union with `/`.** Sphinx splits type fields on `[ ] ( ) , |`, ` or `, ` of `
   and `...` — but not on `/`, so `string/os.PathLike` stays one unresolvable token and neither
   half links. Write `str, or os.PathLike`. The sole exception is a union used as a prose
   container's element (`sequence of str/os.PathLike`), where `, or` would re-bind it to the top
   level and change the meaning.
8. Other conventions: enum-value lists keep the listed values without italics
   (`PropertyReturn.exactly_one, PropertyReturn.first_matching`); "returns the *default*
   argument" cases are written `type(default)`; numpy arrays are `numpy.ndarray`.

`intersphinx` is enabled for the Python and numpy documentation, so `str`, `int`, `os.PathLike`
and `numpy.ndarray` resolve to their upstream pages. Note `None` never links — it is documented
as data, not a class.

Example:

```rst
:returns: list of :class:`PlateBoundaryStatistic` ..., or (if *return_shared_sub_segment_dict*
  is ``True``) a ``dict`` mapping each :class:`ResolvedTopologicalSharedSubSegment` to a list
  of :class:`PlateBoundaryStatistic`
:rtype: list[PlateBoundaryStatistic], or dict[ResolvedTopologicalSharedSubSegment, list[PlateBoundaryStatistic]]
```

## Math markup

The narrative pages (the primer and the sample-code walkthroughs) use `:math:` roles and `.. math::`
directives, rendered in the browser by MathJax via `sphinx.ext.mathjax`. Do not set `mathjax_path` —
the extension's default already tracks a current MathJax release. The override this project carried
until 2026-08 pointed at `cdn.mathjax.org`, retired in 2017; it still answers, but only with a shim
that redirects to MathJax 2.7.1, so the docs silently ran an eight-year-old renderer off a service
that may stop answering at any time.

**Escape underscores inside `\text{...}`.** `_` is a math-mode-only character in LaTeX. MathJax 2
tolerated it and rendered it literally, so this went unnoticed for years; MathJax 3 and later reject
it, and `\text{geometry_final}` renders as the error `'_' allowed only in math mode`. Write
`\text{geometry\_final}`, which is correct under every renderer. Underscores *outside* `\text{}` are
ordinary subscripts and need no escaping (`t_{from}`, `P_{A}`).

Because MathJax runs in the browser, a clean Sphinx build proves nothing about the math — the build
only checks that the reST parses. Open the affected pages after changing any formula.

## Why autosummary with `:toctree:`

`pygplates_reference.rst` lists the API under `autosummary` directives with the `:toctree:
generated` option, so Sphinx writes a stub `.rst` file per class/function into `generated/` in the
build tree (hence `autosummary_generate = True`). Each class therefore gets its own HTML page —
lighter to load than one enormous page — and each page opens with a summary table of its members,
which `automodule` does not provide.

Autosummary emits a bare `.. autoclass::` into each stub, with no `:members:`, so members would go
undocumented. `autodoc_default_options` supplies the defaults instead:

- `'members': True` — document the members.
- `'inherited-members': False` — inherited members are deliberately *not* documented. They still
  appear in the class's summary listing, just without a link, which is a reasonable hint to go
  read the base class. It also avoids confusing entries such as `GeometryOnSphere.get_points()`
  showing up on `PointOnSphere`.
- `'show-inheritance': True` — show base classes.

Autosummary *templates* are not used; the defaults plus `autodoc_default_options` give good enough
output.

### Adding, removing or renaming classes and functions

The old advice was to delete the `generated/` directory whenever anything was added, removed or
renamed. Re-tested against the current Sphinx (2026-08):

- **Adding or changing members: nothing to do.** `autosummary_generate_overwrite` has defaulted to
  true since Sphinx 2.0, and a stub deliberately edited to be stale was overwritten on the next
  build.
- **Removing or renaming a class/function: delete its stale `generated/*.rst`.** Autosummary never
  deletes stubs it no longer generates. Because warnings are errors, this now *fails the build*
  rather than silently publishing a stale page — you get `autodoc: failed to import 'X'` plus
  `document isn't included in any toctree`. Deleting the file (or the whole `generated/`
  directory) fixes it.
