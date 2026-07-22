# Plan: Generated .pyi type stubs for pygplates

> Status: **planned, not yet executed** (2026-07-23, branch `feature/pygplates_pyi`).
> This file can later be reworked into a design doc for the stub generator.
>
> Suggested Claude model per step is listed in each step heading (and summarized in the
> table at the end). The parser/generator work is the hard part; the CMake/test wiring
> is mechanical.

## Context

pygplates is a Boost.Python extension, so IDEs/type checkers (e.g. Pylance when editing
GPlately) see no type information. Auto-generated signatures are disabled
(`docstring_options(true, false, false)` in src/api/PyGPlatesModule.cc:375) — the only
machine-readable API description is the hand-written docstring convention: first line is
a bare signature `name(arg, [opt=Default])`, with real type info in Sphinx `:type x:` /
`:rtype:` ReST fields. A naive prototype (`boost_pyi_gen.py` + `pygplates.pyi` at repo
root, both untracked) proved the introspection approach but produced `Any`-heavy and
partly invalid output.

**Confirmed approach**: docstring introspection of the *built* module is the right
approach (there is no pybind11-style stubgen for boost-python; mypy's stubgen can't read
these ReST fields; parsing C++ source would be far more fragile than importing the module
boost-python already assembled — which also gives us real staticmethod descriptors,
properties, enums, bases, and the injected pure-Python members for free).

**Approved decisions**:
1. **Committed stub + freshness ctest** (no build-time generation). Deterministic; works
   for cross-compiled conda-forge builds (`cross-python` in pygplates/conda/meta.yaml —
   the built .pyd can't be imported there) and arbitrary sdist builds. A ctest
   regenerates from the built .pyd and fails if the committed stub is stale.
2. **Installed layout: `pygplates/__init__.pyi` + `pygplates/py.typed`** (PEP 561).
   pygplates installs as a *package* (`__init__.py` doing `from .pygplates import *` +
   `pygplates.pyd`), so a bare `pygplates.pyi` would only stub the private submodule —
   checkers resolving `import pygplates` need the package's `__init__.pyi`. `py.typed`
   is required for mypy to honor inline stubs.
3. **Full docstrings embedded** in the stub (signature lines stripped). Pylance never
   imports modules — hover docs come only from the stub; omitting them would regress
   hover vs Pylance's current auto-scraping. Repo-bloat concern addressed: git packs
   deltas, so an API tweak adds ~KB, not another copy of the ~1 MB file; deterministic
   sorted output keeps diffs minimal.

Key build facts (verified):
- `wheel.packages = []` in pyproject.toml → CMake `install()` is the **only** route into
  the wheel; both pip and conda go through scikit-build-core → one `install(FILES)`
  covers both.
- Install rules: cmake/modules/Install.cmake pygplates branch (lines 162–234);
  `PYGPLATES_PYTHON_PACKAGE_DIR` already carries SKBUILD vs non-SKBUILD prefix logic
  (lines 175–186); `__init__.py` installed at line 232.
- Proven import pattern to reuse: pygplates/test/CMakeLists.txt runs
  `${GPLATES_PYTHON_EXECUTABLE}` with `PYTHONPATH=$<TARGET_FILE_DIR:pygplates>`;
  doc-python-api/conf.py.in:29–37 has the Windows Py3.8+ `os.add_dll_directory()`
  bootstrap (PATH entries, reversed, skipped under conda) the generator must replicate.

## Deliverables

| Artifact | Path |
|---|---|
| Generator script (new) | `pygplates/stub/generate_stub.py` |
| Committed stub (generated) | `pygplates/stub/__init__.pyi` |
| PEP 561 marker (new, empty) | `pygplates/stub/py.typed` |
| Freshness ctest (new) | `pygplates/stub/CMakeLists.txt` |
| Hook up subdir (edit) | `pygplates/CMakeLists.txt` (add `add_subdirectory(stub)`) |
| Install rule (edit) | `cmake/modules/Install.cmake` (~line 232) |
| Line endings (edit) | `.gitattributes` (add `*.pyi text eol=lf`) |
| Delete prototypes | root `boost_pyi_gen.py`, `pygplates.pyi` (untracked) |

Location rationale: `pygplates/` subdir is already the pyGPlates-only packaging/test tree
(only traversed when `GPLATES_BUILD_GPLATES=FALSE`); `stub/` sits beside `test/`, and
committing the stub *as* `__init__.pyi` avoids any rename at install.

## Step 1 — Generator script (`pygplates/stub/generate_stub.py`)   [model: Fable 5]

Single file, stdlib-only, Python 3.8-compatible source (runs under
`GPLATES_PYTHON_EXECUTABLE`). Emitted stub uses modern stub-only syntax (`X | None`,
`list[X]` — legal in .pyi regardless of runtime version).

**CLI**: `generate_stub.py --module-dir <dir-of-built-pyd> [--output <path>] [--check <committed-stub>]`
- Before `import pygplates`: `sys.path.insert(0, module_dir)`; on Windows Py≥3.8 and not
  conda, `os.add_dll_directory()` for every existing PATH entry (reversed) — mirrors
  conf.py.in:29–37.
- `--check`: regenerate in memory, compare against committed file (universal-newline
  decoded, so CRLF-immune), print capped unified diff + the exact regeneration command
  on mismatch, exit 1. Combinable with `--output` (test also writes the regenerated stub
  to the build dir for inspection).
- Self-gate: `ast.parse()` the generated text before writing/comparing.
- Write with `newline='\n'`, UTF-8; no timestamps; fixed "DO NOT EDIT — regenerate
  with …" header comment.

**Module walker / filtering**:
- Skip modules (`math`), objects whose `__module__` is set and not in
  `{'pygplates', 'pygplates.pygplates'}` (leaked `namedtuple`, `partial`), plus explicit
  skip-set `{'iteritems','itervalues','listitems','listvalues'}` (Py2-compat helpers
  exec'd into the module dict get `__module__ == 'pygplates'`).
- Emit module docstring, `__version__: str`,
  `def _post_import(package_dir: str) -> None: ...` (referenced by the generated
  `__init__.py`).
- Ordering: plain `sorted()` names at module and class level (forward refs are always
  legal in stubs, so alphabetical is safe). Deterministic imports block (only what's
  used).

**Docstring signature parser**:
- Split `__doc__` into overload blocks on column-0 lines matching `^name\((.*)\)\s*$`
  (boost-python concatenates sibling `.def()` docstrings — e.g. the four
  `RotationModel.__init__` overloads, src/api/PyRotationModel.cc:663–851). A block with
  literal `(...)` args (the generic `__init__(...)` marker) is prose — prepend its text
  to the first real overload's docstring. ≥2 real blocks → `@overload` defs.
- Param tokenizer: top-level comma split respecting `[]`; `x` required, `[x]` → `= ...`,
  `[x=Default]` → literal default when `ast.literal_eval` succeeds or it's a resolvable
  pygplates enum member (`PropertyReturn.exactly_one`), else `= ...`; handle
  `*args`/`**kwargs` incl. `[**output_parameters]` (`reconstruct`/`resolve_topologies`
  raw_functions) with **no** default (fixes prototype's invalid
  `**output_parameters: Any = ...`).
- Types from `:type <name>:` within that overload's block; return from `:rtype:`;
  missing → `Any`. `self` prepended for instance methods; `__init__ -> None`.

**ReST type-expression parser** — recursive-descent over a tokenizer (not regexes).
Vocabulary (from full survey of src/api): `` :class:`X` `` (incl. `` :class:`display<X>` ``
→ target); primitives + variant spellings (`integer`, `string`, `none`, `double`);
`list of X`/`list of floats`; ``sequence (eg, ``list`` or ``tuple``) of X`` →
`Sequence[X]`; unions with `or` / `, or None`; tuple forms `the tuple (float, float)`,
`tuple (A, B [, C])` → `tuple[A,B] | tuple[A,B,C]`, `list of (A, B) tuples`; enum
emphasis `*PropertyReturn.exactly_one*, … or *PropertyReturn.all*` → enum class name;
`` string/``os.PathLike`` `` → `str | os.PathLike`;
`` callable (accepting single :class:`Property` argument) `` → `Callable[[Property], Any]`.
Anything unparseable (e.g. `type(*default*)`, trailing "depending on…" prose) → `Any`
**plus** an entry in a deduplicated stderr report (`qualified.name: raw text`) — the
incremental docstring-improvement worklist. A small `MANUAL_OVERRIDES` dict (keyed by
qualified name, optional overload index) supplies complete replacement signatures for
hopeless cases.

**Emission per kind**:
- Functions/methods with docstring: body = the docstring alone (valid stub form);
  otherwise one-line `: ...`. Docstring embedding: drop signature line(s), strip the
  `[*staticmethod*] ` marker (56 occurrences), dedent 2-space body, rstrip lines;
  deterministic raw-string/escaping rule for backslashes and `"""`.
- Staticmethods: authoritative signal is
  `isinstance(inspect.getattr_static(cls, name), staticmethod)` (boost-python
  `.staticmethod()` creates real descriptors) → `@staticmethod`, no `self`.
- Properties (`add_property`): no signature line; type from the bare `:type:` field
  (e.g. PyNetworkTriangulation.cc:371) → `@property` (+ `@name.setter` when `fset`
  present).
- Enums (19 `bp::enum_` int-derived types, e.g. `PropertyReturn` PyFeature.cc:3875):
  `class PropertyReturn(int):` with members as `ClassVar[PropertyReturn]` ordered by
  integer value, plus `names`/`values` ClassVar dicts.
- Exceptions/bases: real `__bases__` filtered to pygplates classes + builtins (drop
  `object`, `Boost.Python.instance`), e.g. `class AbortError(RuntimeError): ...`.
- Operators from operators.hpp (no docstrings): fixed dunder table
  (`__eq__(self, other: object) -> bool`, arithmetic → `Any`), refined later via
  `MANUAL_OVERRIDES`; `__hash__: ClassVar[None]` when `cls.__hash__ is None`. Protocol
  methods (`__iter__`, `__len__`, `__getitem__`…) mostly have docstrings — parse
  normally, fall back to table. Undocumented pickle methods →
  `__getstate__(self) -> Any` / `__setstate__(self, state: Any) -> None`.
- Pure-Python injected members (7 files exec'd from Qt resources, source
  src/qt-resources/python/api/*.py): real Python functions — use `inspect.signature()`
  for names/defaults, rename param 0 (`property` etc.) to `self` for instance methods,
  take types from the same docstring convention; real annotations (if ever added) win.
- No `__all__` (the stub defines every public name; default stub export rules suffice).

## Step 2 — Generate, iterate, commit the stub   [model: Fable 5 (or Opus 4.8)]

Build pygplates (Release), run generator with `--module-dir` = the built target dir,
review the unparsed-type report, extend grammar / add `MANUAL_OVERRIDES` until the
remainder is genuinely free-text (don't chase zero — `Any` + report is the escape
hatch). Judgment-heavy: each mapping decision affects the public typing surface.
Eyeball: `RotationModel.__init__` overloads, `reconstruct` (`**output_parameters`), one
enum, one property, one exception. Commit generator + `py.typed` + `.gitattributes`
first, generated stub as its own commit (review sanity).

## Step 3 — CMake install rule   [model: Sonnet 5]

`cmake/modules/Install.cmake`, pygplates branch only, immediately after the
`__init__.py` install (line 232) — `PYGPLATES_PYTHON_PACKAGE_DIR` already handles
SKBUILD/non-SKBUILD, and this branch never runs for the GPlates app build:

```cmake
    # Install the type stub ('__init__.pyi') and PEP 561 marker ('py.typed') for pygplates.
    # The stub is generated by 'pygplates/stub/generate_stub.py' and committed to source control
    # (the 'pygplates-stub-test' ctest checks it stays up-to-date with the built pygplates module).
    install(FILES
            "${PROJECT_SOURCE_DIR}/pygplates/stub/__init__.pyi"
            "${PROJECT_SOURCE_DIR}/pygplates/stub/py.typed"
        DESTINATION ${PYGPLATES_PYTHON_PACKAGE_DIR})
```

`py.typed` is a committed empty file (not `file(WRITE)`) so the whole typing payload
lives in one directory.

## Step 4 — Freshness ctest   [model: Sonnet 5]

`pygplates/CMakeLists.txt`: add `add_subdirectory(stub)` after `add_subdirectory(test)`.

New `pygplates/stub/CMakeLists.txt` (mirrors pygplates/test/CMakeLists.txt;
`GPLATES_PYTHON_EXECUTABLE` is exported to top-level scope by src/CMakeLists.txt:216):

```cmake
add_test(
    NAME pygplates-stub-test
    COMMAND ${GPLATES_PYTHON_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/generate_stub.py"
        --module-dir "$<TARGET_FILE_DIR:pygplates>"
        --check "${CMAKE_CURRENT_SOURCE_DIR}/__init__.pyi"
        --output "${CMAKE_CURRENT_BINARY_DIR}/__init__.pyi"
    CONFIGURATIONS Release MinSizeRel)
```

- Pass/fail by exit code only — deliberately **no**
  `FAIL_REGULAR_EXPRESSION "FAIL|Fail|fail"` (the printed docstring diff could contain
  "fail").
- No `ENVIRONMENT PYTHONPATH` needed (`--module-dir` + internal DLL bootstrap cover it).
- Cross-compiled conda builds don't run ctest → naturally skipped; the committed stub
  still installs.

## Step 5 — Cleanup   [model: Haiku 4.5 (trivial)]

Delete root `boost_pyi_gen.py` and `pygplates.pyi` (untracked prototypes, superseded).

**Deferred, needs explicit signoff (API-visible)**: tidy src/qt-resources/python/api/*.py
to `del` leaked module-level names (`namedtuple`, `partial`, `math`, `iteritems`, …) the
way Property.py already `del`s its functions — this *removes* accidental runtime
attributes like `pygplates.namedtuple`. Until then the generator's filter keeps them out
of the stub only.

## Verification   [model: Sonnet 5; step 4 is a manual IDE check]

1. Configure/build pygplates (Release),
   `ctest -C Release -R pygplates-stub-test --output-on-failure` → passes. Hand-edit one
   stub line, rerun → fails with diff + regeneration command; revert.
2. `mypy pygplates/stub/__init__.pyi` (or pyright) → clean; fix generator if not.
   (ast.parse gate is built-in.)
3. Optional, advisory: `python -m mypy.stubtest pygplates` against an installed wheel
   (expect boost-python noise; document in the generator docstring).
4. End-to-end: `pip wheel .` (or `pip install .` into a fresh env), confirm the wheel
   contains `pygplates/__init__.pyi` + `pygplates/py.typed`; open GPlately in VSCode
   using that env — check `pygplates.RotationModel(` shows typed overloads, hover on
   `get_rotation` shows full docs, `PropertyReturn.first` resolves.
5. Determinism: regenerate twice (ideally on a second platform) → byte-identical.

## Notes on the two Sphinx quirks (out of scope, for later)

- `[*staticmethod*]` marker: boost-python's `.staticmethod()` creates *real*
  `staticmethod` descriptors, which modern Sphinx autodoc can detect and label — worth
  an experiment to drop the manual markers later. The stub generator relies on the
  descriptor, not the marker (and strips the marker from embedded docs).
- Multiple `__init__`: `autodoc_docstring_signature = True` (already set in
  doc-python-api/conf.py.in:222) supports *multiple* signature lines at the docstring
  start in Sphinx ≥ 3.1 and renders them all. In the stub this is represented properly
  as `@overload` — the standard typing mechanism, so Pylance shows each constructor form
  separately (better than the current Sphinx rendering).

## Risks

- Docstrings are fully hand-written and interpolated defaults are compile-time
  constants → cross-platform regeneration should be byte-identical; any drift surfaces
  as a freshness-test diff (easy to diagnose).
- `pygplates.pygplates.<symbol>` (kept for pickling, Install.cmake:206–212) stays
  untyped — private surface, acceptable.
- Debug-only developers never run the freshness test (Release/MinSizeRel configurations,
  matching `pygplates-test`) — a Release ctest run (locally or CI) is what enforces
  freshness.
- Operator return types (`__add__ -> Any`) are the weakest part of v1 — refine via
  `MANUAL_OVERRIDES` or C++ docstrings later.

## Suggested Claude model per step (summary)

| Step | Work | Suggested model | Why |
|---|---|---|---|
| 1 | Generator script (parsers, emission) | **Fable 5** | The hard part: overload splitting, ReST type grammar, many edge cases — highest-capability model pays for itself here. |
| 2 | Generate/iterate/commit stub | **Fable 5** (Opus 4.8 acceptable) | Judgment calls on type mappings and `MANUAL_OVERRIDES`; shapes the public typing surface. |
| 3 | CMake install rule | Sonnet 5 | Small, well-specified edit with exact snippet above. |
| 4 | Freshness ctest wiring | Sonnet 5 | Mirrors an existing pattern; snippet above. |
| 5 | Prototype cleanup | Haiku 4.5 | Trivial file deletions. |
| Verify | Build/ctest/mypy/wheel checks | Sonnet 5 | Mechanical execution and reporting; the VSCode/GPlately hover check is a manual human step. |

Steps 3–5 can be done in one session by one model; they're listed separately only for
clarity. If running the whole plan in a single session, use Fable 5 (or Opus 4.8) since
Step 1–2 dominate.
