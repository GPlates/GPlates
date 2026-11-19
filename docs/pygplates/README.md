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

Then build the `docs-pygplates` target — the `docs-pygplates` project in Visual Studio, or:

```bash
cmake --build <build-dir> --config Release --target docs-pygplates
```

The result is `<build-dir>/docs/pygplates/html/index.html`.

Warnings are errors (`-W`), so the build fails on anything Sphinx complains about; it also builds
in parallel (`-j auto`) on Linux and macOS — Sphinx ignores `-j` on Windows and builds serially.

> **Editing `conf.py.in` does not always trigger a rebuild.** Sphinx invalidates its cached
> doctrees when *config values* change, not when conf.py *code* changes — so editing a handler
> such as `process_docstring` leaves the previously built HTML in place. Delete
> `<build-dir>/docs/pygplates/_doctrees` (or run Sphinx by hand with `-E`) when verifying such a
> change.

> **Build from scratch for anything you publish.** An incremental rebuild produces HTML identical
> to a from-scratch build except for `searchindex.js`, where it silently drops *all* index entries
> (`indexentries` goes from 854 to 0, while `terms`, `docnames` and the rest are unchanged) —
> reproducible across repeated incremental runs. Search still works on page text, but index-entry
> matches are lost. Delete `<build-dir>/docs/pygplates/_doctrees` (and `generated/`) first, or
> pass `-E`.

## Publishing

The documentation is published by GitHub Pages from the `gh-pages` branch, under
<https://gplates.github.io/GPlates/pygplates/>:

| Path | What | Published by |
|---|---|---|
| `dev/` | `main` | `build-test-pygplates.yml`, after each push to `main` that it builds (it skips a push that changes only Markdown files) and that passes its tests on all three platforms |
| `<X.Y>/` | the newest release in the X.Y series | `publish-docs-pygplates.yml`, when the GitHub release of a final `PyGPlates-X.Y.Z` tag is published |
| `stable` | a link to the newest release series | the same, when it publishes a newer series than the one `stable` points at |
| `1.0/` | pyGPlates 1.0 | a copy, made once (see below) |

The site's root redirects to <https://www.gplates.org/docs/>, the documentation hub, rather than
being a second landing page. Only CI writes to `gh-pages`, always through `publish_docs.py`,
which replaces one of these directories and nothing else (its docstring describes the branch, and
why publishers retry rather than queue). It refuses an incremental build, a candidate or
development build in a release directory, and an older release over a newer one.

To publish again by hand, run the workflow from the Actions tab: `build-test-pygplates.yml` on
`main` for `dev`, or `publish-docs-pygplates.yml` with the release's tag and *publish* ticked.
With *publish* unticked, `publish-docs-pygplates.yml` only builds, and keeps the HTML as a run
artifact: that is how to try it on a release candidate, which is never published.

A pull request runs the `dev` build without publishing it, so a docstring that makes Sphinx warn
fails the pull request.

### Why it is published this way

- **Releases are documented from the wheel on PyPI**, with the pages from the release's tag. The
  documentation then describes the artifact users install (a build tree could differ in build
  flags, Python version or optional dependencies), needs no compiler, and checks that the
  published wheel imports. Not from the publishing run's own wheel artifacts, which would not
  check what PyPI serves. `dev` comes from CI's build tree instead, since there are no development
  wheels.
- **The release documentation has a workflow of its own**, started by the GitHub release (which is
  created after the PyPI publish), rather than being a job in `build-wheels.yml`. A documentation
  failure then cannot turn a successful publishing run red, and the documentation can be re-run
  on its own. The workflow file's comments list its traps.
- **GitHub Pages, not Read the Docs.** Read the Docs builds in its own container, under time limits
  too short to compile pyGPlates, so it could document released wheels but never `dev`.
- **A `gh-pages` branch**, not a deployment from a workflow artifact: an artifact deployment
  replaces the whole site, and the versions have to accumulate. Every clone downloads the branch.
  A whole tree (about 420 files, 19 MB) adds about 6 MiB packed. A rebuild of the same version is
  byte-identical, so publishing a release again adds almost nothing, but every `dev` publish
  rewrites all the HTML pages, since each carries the exact version in its title, and adds about
  0.1 MiB. The history can be squashed if it grows too large (the ruleset protecting the branch
  allows force pushes; `docs/design/versioning/README.md`, section 13).
- **This repository, not the website's.** Publishing into `GPlates/new-gplates-website`, which
  served the documentation until 1.1, would have kept the `www.gplates.org/docs/pygplates/` URLs
  that papers cite, with nothing to redirect. But it needs a credential for another repository (a
  personal access token or a GitHub App), where publishing here uses the workflow's own token; it
  commits generated HTML to a repository of over 100 MB and rebuilds the whole website for each
  update; and `dev` would still have needed a second host. A repository of its own
  (`GPlates/pygplates-docs`, say) would have dropped the `GPlates/` segment from the URL, at the
  cost of a new repository and the same credential.
- **`gplates.github.io/GPlates/pygplates/`, with no `docs.gplates.org`.** `www.gplates.org/docs/`
  is the documentation hub, and already links out to `gplates.github.io` for GPlately and rgplates.
  A second root would compete with it, and could never replace it: a custom domain belongs to one
  repository's Pages site, and GPlately's and rgplates' are others. The `pygplates/` segment is
  needed because this repository also builds GPlates, whose user manual is to be published beside
  it as `user-manual/`, and without it `GPlates/1.1/` would read as GPlates 1.1. A custom domain
  can still be added later without breaking a link, since GitHub redirects the `github.io` URLs
  to it.
- **`dev` and `stable`, but no `latest`.** On Read the Docs `latest` means the development branch,
  which readers easily take for the newest release. And once two names for one tree are published,
  neither can be withdrawn without breaking links. A `latest` link to `dev` can be added at any
  time.
- **No version switcher.** Read the Docs' version menu was the one thing given up by not using it,
  and is not a reason to choose a host. Each version is reached by its URL (`pygplates/` itself
  redirects to `stable/`).

### The 1.0 documentation

`1.0/` is a copy of the tree the website served at `www.gplates.org/docs/pygplates/` (from
`GPlates/new-gplates-website`), not a rebuild: the `PyGPlates-1.0.0` tag predates `build_docs.py`.
Its search index has no index entries, the mark of an incremental build; it is kept as 1.0 users
have been reading it. Once the website's copy is deleted it is the only live
one, which is why `gh-pages` is protected from deletion.

On Windows, copy such a tree from a checkout with `core.autocrlf` off. With it on (the default
there), the checkout converts every HTML file to CRLF line endings, and a `diff -r` against that
same checkout still passes. Compare git tree hashes instead (`git rev-parse <commit>:<path>` on
both sides).

### The old URLs

Until 1.1 the documentation was served unversioned at `www.gplates.org/docs/pygplates/`, and those
URLs are in papers and in other projects' pages, in four shapes: the directory itself, top-level
pages (often with a fragment), `generated/<name>.html` and `sample-code/<name>.html`. The website
is to redirect each to the same path below `stable/`. It is itself a GitHub Pages site, which
serves static files only, so it cannot answer with an HTTP redirect, only redirect in the browser
with a script. So:

- The path of a page below `stable/` is a published address. Renaming a page breaks old links
  unless the page it replaces stays, as a redirect.
- Anything that does not run JavaScript (intersphinx, a link checker) is not redirected. An
  intersphinx mapping must name `https://gplates.github.io/GPlates/pygplates/stable/` itself.

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

The signature lines also make the message of a `pygplates.ArgumentError`, raised when no
overload accepts the arguments of a call: `src/qt-resources/python/api/ArgumentErrors.py` lists the
function's documented signatures, each followed by its parameters' `:type:` fields, in place of
Boost.Python's C++ signatures. It does so only if each documented parameter has the same name in
the C++ (its `bp::arg`), which a keyword argument needs anyway; `pygplates-test` fails for a
function where one doesn't. A function without a signature line gets the first paragraph of its
docstring instead (so a pickle-only constructor's "You cannot directly instantiate this class from
Python." is what a caller sees).

Do not mark a static method in the body (the old `[*staticmethod*]` marker). Boost.Python's
`.staticmethod()` installs a real `staticmethod` descriptor, which Sphinx detects and renders as
the italic *static* prefix itself (and which `process_docstring` reads for its `:staticmethod:`
option) — a marker only duplicates it.

**Never use single backquotes.** `default_role` is unset in `conf.py.in`, so `` `dict` `` renders
as an italic title reference, not code. Use double backquotes for a literal, or a role.

### Overloads

Boost.Python concatenates the docstrings of overloaded `def()`s into a single `__doc__`, and
pyGPlates deliberately gives each overload its own Parameters/Raises/example block. Extra
overloads are therefore written as further signature-led blocks in the same docstring.

Sphinx cannot render that natively, so `process_docstring` in `conf.py.in` splits the docstring on
those signature lines, dedents each block, and re-emits every extra one as a `py:method` /
`py:function` directive (with `:no-index:`, and `:staticmethod:` where applicable) whose content
is that block's body. Read that function before changing docstring layout — in particular it also
protects a `.. versionadded::` / `.. versionchanged::` written flush at column zero.

### Class attributes

A class attribute that Boost.Python defines with `def_readonly()` on a value that isn't a data
member (`FeatureType.gpml_coastline`, `Vector3D.x_axis`, `Earth.mean_radius_in_kms` and the like)
can't have a docstring: Boost.Python drops it, and Python can't add one. Its documentation lives in
`conf.py.in` instead:

- A feature type's or property name's text is generated from its value: its GPGIM name, linked to
  that name's entry in the GPGIM documentation. A property name of an enumeration or geometry
  property is also in `_ENUMERATION_PROPERTY_NAMES` or `_GEOMETRY_PROPERTY_NAMES`, and its text
  says so.
- Any other's text is one sentence in `_CLASS_ATTRIBUTE_DOCSTRINGS`. An attribute added without
  one fails the docs build, and so does an entry for an attribute that no longer exists, a text
  that doesn't name its value (where the value has a qualified name, eg `gpml:CrustalThickness`),
  and an attribute of a nested class (documented on its outer class's page, which the template
  doesn't reach).

The class template (`_templates/autosummary/class.rst`) writes the attributes' summary table and
an entry for each. A class docstring refers to them ("listed under *Attributes* below", or by an
`:attr:` role) rather than listing them again.

### Type fields

`:type x:` / `:rtype:` fields feed **two** consumers: Sphinx auto-links them, and
`pygplates/stub/generate_stub.py` parses them to produce the type stub
`pygplates/stub/__init__.pyi` (see `pygplates/stub/README.md`). One style serves both. (The
`ArgumentError` message also shows the `:type x:` fields, as written; see *Signature and body*.)

1. **`:type x:` and `:rtype:` contain only a type expression, 100% markup-free** — no
   `:class:`/`:meth:` roles, no ``` ``literals`` ```, no `*emphasis*`. Sphinx auto-links every
   identifier in a type field, but a *single* piece of inline markup anywhere in the field
   disables auto-linking for the whole field. The one intentional exception: the list-like
   docstrings templated in `src/api/PyRevisionedVector.h` interpolate the element type as a
   `` :class:`X` `` role (the stub generator accepts roles for them — see
   `pygplates/stub/README.md`).
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
8. Other conventions: a parameter taking an enumeration names the enumeration type
   (`:type property_return: PropertyReturn`), not a list of its values; "returns the *default*
   argument" cases are written `type(default)`; numpy arrays are `numpy.ndarray`.
9. **`iterable of X` or `sequence of X` follows the binding.** Write `iterable of X` when it reads
   the argument with `PythonExtractUtils::extract_iterable` (one pass, so any iterable works: a
   generator, `zip(...)`, a numpy array), and `sequence of X` when it needs a real sequence
   (`check_sequence`, which raises `TypeError` for an iterator, or Python `len()` and indexing).
   One exception: a field taking a `dict` or a sequence of (key, value) pairs keeps `sequence of
   (K, V) tuples` even where any iterable works. With `Iterable` in that union, mypy rejects a
   `dict` literal, since a `dict` is an iterable of its keys. (The generator makes an argument's
   `dict` a `Mapping`, which is covariant, so a `dict` that pyGPlates returns, such as
   `dict[ScalarType, list[float]]`, can be passed back where `dict[ScalarType, iterable of float]`
   is documented. A returned `dict` stays a `dict`.)
10. **A point given by its coordinates** is `sequence (latitude,longitude) in degrees or (x,y,z)`,
    which the generator makes `Sequence[float]`: the converters take any non-iterator sequence
    (a list, a tuple) of the right length, which the stub can't express. A vector is `sequence
    (such as list or tuple) of float`, or `sequence (x,y,z)` as the element of a sequence (where
    `or sequence of float` would trip the generator's bare-`or` lint). Any group of coordinates
    (`sequence (magnitude, azimuth, inclination)`) is a sequence, even where the binding would
    also take an iterator. A value the binding *returns* as a tuple stays a tuple
    (`tuple (float, float)`).

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

This is deliberately *not* NumPy-style (napoleon). Napoleon converts to these same info fields
internally, so the HTML would be essentially unchanged, while its
one-Parameters-section-per-docstring assumption conflicts with the per-overload signature-led
blocks above, and both the stub generator's parser and hundreds of docstrings would need
rewriting. The rules above map one-to-one onto NumPy-style `name : type` lines should that ever
be wanted.

Possible future work: `nitpicky` with `nitpick_ignore_regex` would catch unresolvable `:class:`
and type-field references at build time, but its initial burst of warnings needs a triage pass
first.

## What goes in a docstring and what goes in the Primer

A docstring documents one member: what it does, its parameters, return value and exceptions, and
a short example. Anything a reader needs *before* they can use the member - a derivation, an
algorithm, a sign or ordering convention, or a workflow that spans several classes - belongs in
the Primer (a page in `primer/`), and the docstring points at it:

```
  .. seealso:: :ref:`pygplates_primer_equivalent_stage_rotation` in the *Primer* documentation.
```

`PyResolveTopologyParameters.cc` and `PyTopologicalModel.cc` follow this pattern. The reason is duplication: a convention explained inside one method's docstring
gets pasted into the next method that needs it, and the copies then drift. Conversely, a
docstring should not merely say "see the Primer" - it must still state the member's contract.

Enumerations (`bp::enum_`) take their docstring from the constructor's second argument; the
docstring is the only place their values are described (a table of `Value` / `Description`), so
a parameter that accepts an enumeration names it in its `:type:` field (which links to the
enumeration's page), and its `:param:` text mentions only the values that particular function
restricts or defaults to. Exceptions take their docstring from
`export_exception()` in `PyExceptions.cc`, except `ArgumentError` (Boost.Python's class, which
`src/qt-resources/python/api/ArgumentErrors.py` names and documents). Both are listed in the
*Enumerations* and *Exceptions* sections of `pygplates_reference.rst`; the enumerations use the custom autosummary template
`_templates/autosummary/enum.rst`, because the default class template would list every method
that a Boost.Python enum inherits from `int`.

Pure-Python API files (`src/qt-resources/python/api/*.py`) are exec'd into the `pygplates`
module namespace, so every module-level name they define becomes a public attribute of
`pygplates`. Alias imports with a leading underscore (`import itertools as _itertools`).

## Sample code

Every page under `sample-code/` gets its code from a script beside it, `sample-code/<page>.py` (one
per "Sample code" block, so a page with several sub-examples has `<page>_<slug>.py` files). The
page includes the script through the `sample-code` directive defined in `conf.py.in`: the whole
file for the listing, and named fragments for the "Details" walkthrough:

```
.. sample-code:: pygplates_plate_rotation_hierarchy.py

.. sample-code:: pygplates_plate_rotation_hierarchy.py
   :fragment: load-rotations
```

Fragments are delimited in the script by comment markers, which the directive strips from every
listing (and it dedents a fragment shown on its own):

```
# [fragment: load-rotations]
rotation_model = pygplates.RotationModel('rotations.rot')
# [end: load-rotations]
```

Fragments may nest, since the ends are named. A fragment the script does not contain is a Sphinx
warning, which `-W` turns into a failed build - so renaming a marker cannot leave a page silently
showing nothing. So is a marker that appears twice, or a fragment without its end. Never paste code
into a page's `::` block: the code exists once, in the script. A `::` block may still show a shortened
excerpt, marked with `...` (a long list of coordinates cut down, or a branch reduced to its shape), as
long as every line it does show matches the script.

The directive is not `literalinclude` with `:start-after:`/`:end-before:` because the whole-script
listing must lose its markers too, which `literalinclude` can only do by hard-coded line numbers.
It is defined in `conf.py.in` rather than in an extension module of its own because
`build_docs.py` builds from a scratch copy of the sources it knows about, and `conf.py.in` is
already one of them.

### Page template

Each page - or each sub-example, on a page with several under a `.. contents::` - has, in order:

- **Data files**: every file the script reads, what it must contain for the script to work (which
  properties, which plate IDs) and where to get such data, normally the GPlates sample data. A
  script that creates its features from scratch has no Data files section.
- **Sample code**: the whole script.
- **Details**: the walkthrough, quoting the script by fragment.
- **Output**, only where the page has one: what the script printed on real geodata, kept as
  captured. It is never the fixtures' output.
- **See also**: a bullet each for Primer, Reference and Sample code. Link only to Primer sections
  that exist: pages on features, geometries, reconstruction, partitioning and velocities have no
  Primer bullet until those sections are written.

The samples use the model classes (`ReconstructSnapshot`, `PlatePartitioner`, `TopologicalModel`,
...) rather than `reconstruct()` and `partition_into_plates()`. The one page that calls
`partition_into_plates()` - the import page, which partitions once - does so on purpose and says
why.

**Never rename a page.** `sample-code/<page>.rst` is the page's URL, so renaming it breaks every
existing link to it. The index (`pygplates_sample_code.rst`) can be regrouped freely; the pages stay
where they are.

### Testing

The scripts are tested. `pygplates-sample-code-test` (`pygplates/test/sample_code_test.py`) runs
each one in a temporary directory seeded with `pygplates/test/fixtures/`, where the data files the
samples name (`rotations.rot`, `coastlines.gpml`, `static_polygons.gpml`, ...) are small stand-ins:
a few dozen real features cut from the GPlates sample data by
`pygplates/test/fixtures/generate_sample_fixtures.py`, or synthetic geometry. The test asserts that a
script runs; a page's "Output" block stays as captured from real geodata, so it need not match what
the fixtures produce. When a new sample names a data file the fixtures lack, add it there (and to
the generator if it is cut from real data) rather than special-casing the test.

The test lays two more directories over the shared fixtures. `fixtures/sample-code/` holds files for
every sample that must differ from the unit tests' files of the same name: its `topologies.gpml` has
a ridge that diverges and a trench that converges at every time the samples visit, where the unit
tests' one has only unclassified sections on plates that do not move. `fixtures/sample-code/<script>/`
holds files for one script, whose page reproduces its own input in full
(`create_topological_features` shows its `features.gpml`).

To run a sample by hand, run it in a copy of the layered fixtures, not in `fixtures/` itself: the
scripts write their output files into the working directory, and pyGPlates writes a
`<name>.gmt.gplates.xml` sidecar beside every `.gmt` file it reads. Neither belongs in the
repository.

After changing a script or a page, build the docs from scratch (`-W` catches a fragment the script
no longer has) and run the test.

The test only checks that each script exits normally, so a script that reads nothing still passes.
pyGPlates loads no features, without an error, from a GPML file with anything before its XML
declaration, even a blank line: a fixture with one made the topology-creation sample build empty
topologies for as long as it was there. When adding a fixture, check that the script actually finds
something in it.

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
- `'inherited-members': False` — inherited members are deliberately *not* documented on a derived
  class's page. That avoids confusing entries such as `GeometryOnSphere.get_points()` documented
  on `PointOnSphere`. They still appear in the class's summary listing, as references through the
  derived class (`pygplates.PointOnSphere.distance`) that nothing defines, so
  `resolve_inherited_member` in `conf.py.in` (a `missing-reference` handler) resolves them through
  the class's MRO and links each one to the base class's entry (`pygplates.GeometryOnSphere.distance`).
  A role in a docstring that names a member through a derived class resolves the same way.
- `'show-inheritance': True` — show base classes.

Two autosummary templates, in `_templates/autosummary/`, change the defaults: `enum.rst` (selected
by the Enumerations block of `pygplates_reference.rst`) leaves out the members an enum inherits
from `int`, and `class.rst` adds the class attributes (see *Class attributes* above).

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
