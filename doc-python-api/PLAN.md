# Plan: doc-python-api modernization

> Status: **planned, not yet executed** (2026-08-02, branch `feature/pygplates_pyi`).
>
> Companion to `pygplates/stub/PLAN.md` "Round 2" (which covers the stub generator,
> docstring restyles and `[*staticmethod*]` marker removal). This plan covers the
> Sphinx build itself: the `conf.py.in` docstring handler's rendering upgrade,
> config/CMake cleanup and modern-Sphinx adoption, and rewriting `README` as Markdown.
>
> Suggested Claude model per step is listed in each step heading (and summarized in
> the table at the end).

## Context

The docs now build with conda Sphinx 9.1.0 + sphinx_rtd_theme 3.1.0, invoked as
`python -m sphinx` under the same interpreter pyGPlates is built against (see
`env.docs.yml` and CMakeLists.txt). Much of the configuration and all of the README
date from the Sphinx 1.x era. Investigation findings (2026-08-02):

- **Multiple `__init__` docstrings**: Sphinx ≥ 3.1's `autodoc_docstring_signature`
  does consume multiple signature lines, but only when *contiguous from line 0* and
  sharing *one* body (confirmed from `_extract_signatures_from_docstrings` in the
  Sphinx 9 source). pyGPlates deliberately gives each overload its own
  Parameters/Raises/examples block, so stacked signatures would collapse that — the
  `process_docstring` handler in conf.py.in (added 98f098060) remains the right
  mechanism. Sphinx has no native "overload with its own body" rendering
  (`sphinx_toolbox.more_autodoc.overloads` only handles `typing.overload` in Python
  source, not C-extension docstrings).
- **Staticmethods**: modern Sphinx auto-detects Boost.Python's real `staticmethod`
  descriptors and already renders the italic *static* prefix on the published pages —
  the manual `[*staticmethod*]` body markers are duplicated noise and are being
  deleted (see stub PLAN.md Round 2, Step R5).
- The build is currently **warning-free** (verified on a forced full rebuild), which
  makes `-W` safe to adopt.
- Dead config: `mathjax_path` pins `cdn.mathjax.org` (retired 2017; Sphinx 9 defaults
  to MathJax v4); `templates_path` is unused; `needs_sphinx = '1.8'`;
  `SPHINX_THEME`/`SPHINX_THEME_DIR` in CMakeLists.txt are set but never used.
- User decisions (2026-08-02): upgrade extra-overload rendering to `py:method`
  directives; adopt **all four** build extras (`-W`, intersphinx, `-j auto`,
  signature wrapping).

## Step D1 — `process_docstring`: render extra overloads as `py:method` directives   [model: Opus 5]

In conf.py.in's `process_docstring`, replace the inline-literal rendering of each
extra signature block —

```python
output.extend(['``%s``' % signature, ''])
```

— with a real directive, the block body becoming its indented content:

```rst
.. py:method:: __init__(rotation_model, [reconstruction_tree_cache_size=2], [default_anchor_plate_id])
   :no-index:

   <block body, indented as directive content>
```

- `:no-index:` avoids duplicate index/xref entries for the member name.
- Per-overload field lists (`:param:`/`:type:`/`:rtype:`) then parse natively inside
  the directive, and the signature gets real Python-domain styling (bold name, italic
  params) matching the first overload.
- Add the `:staticmethod:` option when `inspect.getattr_static(parent, member)` is a
  `staticmethod` (the handler receives `obj`; resolve the parent class from `name`).
  Check the overloaded static converters on the `LocalCartesian` page.
- Keep the existing block-splitting, per-block dedent, flush-directive protection and
  verbatim-duplicate dropping exactly as they are — only the emission changes.

## Step D2 — conf.py.in / CMakeLists.txt cleanup and extras   [model: Sonnet 5]

`conf.py.in`:

- `needs_sphinx = '7.1'` (floor for `maximum_signature_line_length`; sphinx_rtd_theme
  3.x already requires ≥ 6.0).
- Delete the dead `mathjax_path` override (falls back to Sphinx's bundled MathJax v4
  default).
- Delete unused `templates_path` and the pre-Sphinx-1.3 autosummary-templates comment
  block; drop stale "renamed in 1.8"-style comments.
- Add `sphinx.ext.intersphinx` to `extensions` with
  `intersphinx_mapping = {'python': ('https://docs.python.org/3', None)}` — lets
  `os.PathLike`, `str`, `int` etc. become live cross-references (existing raw-URL
  links keep working unchanged; migrating them is optional follow-up).
- Set `maximum_signature_line_length` (start ~90; judge by the `RotationModel` page —
  long signatures wrap one-parameter-per-line).

`CMakeLists.txt`:

- Add `-W` (warnings are errors — locks in the current warning-free state) and
  `-j auto` (parallel build) to the `python -m sphinx` command. Note: each parallel
  worker re-imports pygplates.pyd (Windows spawn), covered by the existing
  `os.add_dll_directory()` bootstrap in conf.py.in — verify on first build.
- Delete the unused `SPHINX_THEME` / `SPHINX_THEME_DIR` block (lines 30-36).

Deferred (not in this round): `nitpicky` + `nitpick_ignore_regex` — high value for
catching unresolvable `:class:`/`:type:` references, but needs a triage pass of its
initial warning burst first.

## Step D3 — `README` → `README.md`   [model: Sonnet 5]

`git rm doc-python-api/README`; write `doc-python-api/README.md` covering:

- **Building**: create the conda build env (`env.Windows.yml` / `env.macOS.yml` /
  `env.Linux.yml`) then overlay `env.docs.yml`; the same-interpreter requirement
  (Sphinx imports the built `pygplates` module in-process, so it must run under
  `GPLATES_PYTHON_EXECUTABLE` — CMake reports "Looking for sphinx..." accordingly);
  build the `doc-python-api` target (Visual Studio project on Windows, `make`/`ninja`
  elsewhere); output lands at `<build>/doc-python-api/html/index.html`.
- **Docstring conventions that matter**: first line is a bare signature
  `name(arg, [opt=Default])` with the body indented two spaces; extra overloads as
  additional signature-led blocks; empty `__init__()` signature for classes that
  cannot be instantiated directly; types in `:type x:` / `:rtype:` fields (these also
  feed `pygplates/stub/generate_stub.py` — see `pygplates/stub/PLAN.md`); pointer to
  `process_docstring` in conf.py.in for how multi-overload docstrings are rendered.
- **Why autosummary with `:toctree:`** (one page per class + summary tables) and the
  `autodoc_default_options` rationale — carried over from the old README, trimmed.
- **Dropped**: the Sphinx 1.x-era sections ("Use Sphinx >= 1.8", "Using templates
  with autosummary is problematic", `sphinx-build`-on-PATH instructions, dead install
  URL).
- **Re-test before writing it down**: the old advice to delete the `generated/`
  directory after adding/renaming members (`autosummary_generate_overwrite` defaults
  to True since Sphinx 2.0, so stale stubs should regenerate; stale *removed* members
  may still need a manual delete). Document whatever the test shows.

## Step D4 — Build and verify   [model: Sonnet 5]

1. Reconfigure CMake (conf.py.in and CMakeLists.txt changed), then build the
   `doc-python-api` target — must succeed with `-W -j auto` (zero warnings).
2. Inspect pages:
   - `pygplates.RotationModel.html` — three constructor forms, the 2nd and 3rd as
     styled `py:method` signatures; long signatures wrapped.
   - `pygplates.FeatureId.html` — single italic *static* label, no `[staticmethod]`
     body text (after stub PLAN.md Round 2 Step R5 lands).
   - `pygplates.LocalCartesian.html` — the overloaded static converters still render
     all three Parameters/Returns sections, each now under a styled signature.
   - Any intersphinx-resolved reference links to docs.python.org.
3. Diff a rebuilt run against a clean `-E` rebuild for determinism of the generated
   autosummary tree (sanity, not byte-exactness).

## Commit

One commit: conf.py.in + CMakeLists.txt + README.md (+ deleted README). (The stub-side
work has its own commit split — see `pygplates/stub/PLAN.md` Round 2.)

## Suggested Claude model per step (summary)

| Step | Work | Suggested model | Why |
|---|---|---|---|
| D1 | `py:method` rendering upgrade | **Opus 5** | Sphinx/docutils subtleties (directive content indentation, `:no-index:`, staticmethod detection) with iterative HTML verification; the surrounding handler logic already exists. |
| D2 | Config/CMake cleanup + extras | Sonnet 5 | Well-specified additions/deletions listed above. |
| D3 | README.md rewrite | Sonnet 5 | Prose from a detailed outline; one empirical re-test. |
| D4 | Build + verify | Sonnet 5 | Mechanical build plus page eyeballing against listed criteria. |

Steps D2–D4 can run in one session by one model. If running the whole plan in a single
session, use Opus 5 since D1 dominates.
