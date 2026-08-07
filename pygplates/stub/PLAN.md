# Plan: Generated .pyi type stubs for pygplates

> Status: **Round 1 executed** (2026-07-26, branch `feature/pygplates_pyi`) and
> **Round 2 executed** (2026-08-05) — all steps below are done and committed. See the
> [Round 2 section](#round-2-2026-08-02--overload-reorder-type-grammar-extensions-staticmethod-removal)
> at the end of this file.
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

## Notes on the two Sphinx quirks (RESOLVED 2026-08-02 — see Round 2 below)

- `[*staticmethod*]` marker: **confirmed redundant** — boost-python's `.staticmethod()`
  creates *real* `staticmethod` descriptors (`PyStaticMethod_New` in class.cpp), modern
  Sphinx auto-detects them (`sphinx.util.inspect.isstaticmethod` scans the class
  `__dict__` via `__mro__`) and the published docs already render the italic *static*
  prefix **plus** the now-duplicated manual marker. Round 2 deletes all 56 markers.
- Multiple `__init__`: **not fixable via Sphinx's stacked-signature support** — since
  Sphinx 3.1 `autodoc_docstring_signature` does consume multiple signature lines, but
  only when they are *contiguous from line 0* and share *one* body (confirmed from
  `_extract_signatures_from_docstrings` in Sphinx 9 source), which would collapse
  pyGPlates' per-overload Parameters/Raises blocks into one. The `process_docstring`
  handler in doc-python-api/conf.py.in remains the right mechanism; its rendering of the
  extra signatures is being upgraded to real `.. py:method::` directives — see
  `doc-python-api/PLAN.md`. In the stub this was never a problem: overloads are emitted
  as `@overload` defs.

## Risks

- Docstrings are fully hand-written and interpolated defaults are compile-time
  constants → cross-platform regeneration should be byte-identical; any drift surfaces
  as a freshness-test diff (easy to diagnose).
- `pygplates.pygplates.<symbol>` stays untyped — **decided, not a risk**. The private
  submodule is deliberate and load-bearing (Boost.Python pickles classes by reference, so
  its name is embedded in every pickle already written by a released pyGPlates; `dill`
  additionally resolves it by `getattr` on the parent package; and it names every
  exception in a traceback) — full rationale in `cmake/modules/Install.cmake`. Adding a
  second `pygplates/pygplates.pyi` would duplicate all ~1750 members for a surface no user
  should touch, and double the freshness-test burden. It is now covered at runtime by
  `PackageCase` in `pygplates/test/test.py`, which the `pygplates-test` ctest can only
  exercise because the build tree is itself the installed package layout.
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

---

## Round 2 (2026-08-02, revised 2026-08-05) — overload reorder, type-grammar extensions, type-field style sweep, `[*staticmethod*]` removal

> Status: **all steps executed.** R1 + R2: commit 4e681e263; **R3-R6 redesigned
> 2026-08-05** after the docstring type-field style decisions (see below); R3 + R4
> executed 2026-08-05 (R3: e48751a46 plus follow-ups e9ad947b1/47e5fc675 discovered by
> dry-running the sweep; R4: f6ec0c79b mechanical + 33e30aa80 hand restyles); R5:
> 1b30bd0ad; R6: rebuilt/regenerated/pruned/verified 2026-08-05 (the commit containing
> this status update). Only the user-side VS Code re-check (R6 item 6) remains.
> Prompted by VS Code verification of the installed stub (Round 1's end-to-end check):
> Pylance showed only two `RotationModel.__init__` overloads, and the remaining
> "Unparsed type expressions" worklist (20 entries) was reviewed for grammar/docstring
> fixes. The companion `doc-python-api/PLAN.md` covers the Sphinx/docs side **and holds
> the canonical docstring type-field style guideline** that Steps R3/R4 implement.

### Findings driving this round

- **`RotationModel.__init__(rotation_model)` is documented and parses** — the generator
  *deliberately drops* any overload fully subsumed by an earlier broader one
  (generate_stub.py:1475-1485; the comment names this exact case), because mypy flags a
  *later*-subsumed overload as "will never be matched". Emitting the narrower overload
  **before** its subsumer is mypy-legal and makes Pylance show all three.
- Sphinx's officially blessed union syntax for info-field types is exactly the house
  style already (`A or B`, auto-linked; `|` also works) — there is no "better ReST way"
  to express multiple types. So the worklist is cleared by a mix of small grammar
  extensions (where the prose is natural English) and docstring restyles (where it
  isn't).
- **Silent precedence hazard**: `list of A or list of B` parses as `list[A | list[B]]`
  with *no* report entry (`_parse_of_target`'s greedy or-loop, generate_stub.py:630-634).
  Verified live. Only 2 docstrings still use the bare form (both already on the worklist
  for other reasons); the codebase otherwise converged on the safe `, or` separator
  (see the comment at src/api/PyDateLineWrapper.cc:396-399).
- Of the 20 unparsed entries, 11 live in `src/api/*.cc` and 9 in the injected
  pure-Python `src/qt-resources/python/api/*.py`.

### Style decisions (2026-08-05) — reshape Steps R3-R6

Investigating `:returns:`/`:rtype:` conventions surfaced a decisive Sphinx 9 mechanic
(verified in the installed conda Sphinx source): a `:type x:`/`:rtype:` body that is
**pure plain text** gets every identifier auto-cross-referenced (the python domain
splits on `[ ] ( ) ,` / ` or ` / ` of ` / `|`), but a single piece of inline markup
anywhere in the field — a `:class:` role, a ``literal``, an *emphasis* — disables ALL
auto-linking for that field. `:returns:` is the opposite: pure prose, never
auto-linked, so explicit `:class:` roles are the only way to link there.

A corpus survey found 468 `:rtype:` fields (415 `.cc` + 53 `.py`): ~400 simple and
fine, ~60 problem sites in three overlapping groups — 23 underspecified containers
(``` ``list`` or ``dict`` ``` etc., the source of `list[Unknown]` Pylance hovers), 36
conditional rtypes (`if *arg* is True`, `depending on`, the 18-site `type(*default*)`
family), and the 10 worklist survivors.

User decisions (full guideline in `doc-python-api/PLAN.md`):

1. **Full type-field sweep** — ALL `:rtype:`/`:type:` fields become markup-free
   (roles/literals/emphasis dropped *inside those fields only*; prose keeps its roles).
2. **Hybrid syntax** — prose where unambiguous (`list of GeometryOnSphere`,
   `FiniteRotation, or None`); Python brackets whenever tuples/dicts/nesting appear
   (`tuple[GeometryOnSphere, dict]`, `dict[K, list[V]]`).
3. **`, or` top-level separator**; `|` allowed inside brackets.
4. Conditional clauses move out of `:rtype:` into `:returns:` (which keeps `:class:`
   roles); `:rtype:` becomes the flat union of all possibilities.
5. NumPy-style/napoleon conversion considered and rejected.

Verification invariant for the sweep: members whose fields already parsed must keep
**byte-identical** stub annotations; only previously-unparsed/underspecified sites may
change (improve).

### Step R1 — Generator: reorder subsumed overloads   [model: Fable 5]   ✔ DONE (4e681e263)

In the dedup loop (generate_stub.py:1466-1486), when
`_signature_subsumes(entry[0], signature)`:

- **identical** signatures → keep today's merge-and-drop (the const/non-const
  `visit_*` dedup);
- **strictly narrower** → `rendered.insert(<index of subsumer>, [signature, doc_text])`
  instead of `continue`, so every documented overload is emitted, narrower-first.

Edge to verify: the umbrella prose from the `__init__(...)` marker attaches to
`rendered[0]` (lines 1488-1490). In RotationModel the subsumer sits at index 1 so the
prose stays on the features-overload; check behaviour is sensible for all 11
multi-`__init__` classes if an insert ever lands at index 0.

Expected stub outcome: `RotationModel` gains
`def __init__(self, rotation_model: RotationModel) -> None` as a third `@overload`,
emitted *before* the adapt-overload; `mypy` stays clean.

### Step R2 — Generator: type-grammar extensions   [model: Fable 5]   ✔ DONE (4e681e263)

All in `TypeExpressionParser`:

- **`N-tuple of A and B` → `tuple[A, B]`**: replace the `\bN-tuple of\b` →
  `tuple of <count-word>` text rewrite (lines 309-310) with direct parsing in
  `_parse_tuple`; keep `tuple of two X` → `tuple[X, X]`. Fix the flagged hazard first:
  the current rewrite would make `2-tuple of A and B` silently mean `tuple[A, A]`.
  (Fixes the three `Feature.get_*geometr*` returns, with the markup fix in Step R4.)
- **`named-tuple 'X'`**: tokenizer gains a quoted-name token; resolve `X` against the
  module (`Crossover` is a real runtime class in the stub). Chosen over restyling to
  `` :class:`Crossover` `` because Crossover has no page in the HTML docs (only
  sample-code mentions) — the xref wouldn't resolve. (Fixes `find_crossovers` and
  `synchronise_crossovers(crossover_filter)` with no docstring change.)
- **`where`-gloss substitution**: strip a top-level `where <name> is/can be <expr>`
  clause (parenthesised `(where ...)` or trailing), parse `<expr>` as a union, and bind
  `<name>` so the word-atom / tuple-element lookup resolves it. Covers
  `PolygonOnSphere.__init__(interior_rings)` (nested:
  `sequence of rings (where ring is any sequence of ...)`),
  `TopologicalModel.reconstruct_geometry(geometry)`, and both
  `NetRotationModel/NetRotationSnapshot.__init__(point_distribution)` — there the
  placeholder appears *inside* a tuple form (`tuple (point, float) where *point* is ...`),
  so bind before parsing the main text.
- **numpy**: in `parse()`, special-case text starting `2D numpy array` →
  `numpy.ndarray`; track a `uses_numpy` flag → emit `import numpy` in the stub header
  when used. (User decision: yes — numpy is an optional runtime dep, but practically
  every IDE user has it; fixes `GeometryOnSphere.to_lat_lon_array`/`to_xyz_array`.)
- **`:meth:`-only suffix groups are commentary**: a parenthetical group whose only
  markup is `:meth:` (no `:class:`/`:exc:`) is skipped, like the existing no-markup
  skip. Fixes `Feature.create_reconstructable_feature/create_tectonic_section(geometry)`
  (the `(or a coverage ... - :meth:`set_geometry`)` tail) — consistent with the
  `set_geometry` precedent whose own `:type geometry:` already parses by skipping its
  no-markup `(... - see below)` group (coverage forms likewise omitted from its type).
- **Ambiguity lint**: in `_parse_of_target`'s greedy or-loop, when a bare `or` is
  followed by a container keyword (`list`/`sequence`/`tuple`/`dict`), add a
  `self.warnings` entry (parse result unchanged) — turns the silent `list[A | list[B]]`
  mis-parse into a visible report.

Outcome: worklist 20 → 10 (the survivors are the Step R4 restyle sites and the
`partition_*` returns); mypy clean; deterministic. Note: several R2 features
(where-glosses, `N-tuple of A and B`, named-tuple, `2D numpy array`) are expected to
become unused once the Step R4 sweep restyles the docstrings that need them — they get
pruned in Step R6.

### Step R3 — Generator: grammar round 3 (bracket syntax + identifier resolution)   [model: Fable 5]   ✔ DONE (e48751a46, e9ad947b1, 47e5fc675)

Prepares the parser for the restyled corpus (must land before Step R4's regeneration):

- **Bracket syntax**: `list[X]`, `tuple[A, B]`, `dict[K, V]`, `X | Y`, arbitrarily
  nested, mixing with prose forms (`list of tuple[A, B]`).
- **Bare/dotted identifier resolution**: a plain word or dotted name in a type field
  resolves against the introspected module — classes (incl. nested, e.g.
  `NetworkTriangulation.Triangle`), and enum *values* like `PropertyReturn.exactly_one`
  resolve to their owning enum class (mirroring the existing emphasis-form handling,
  since the sweep de-italicizes them). Unknown identifiers still error → worklist.
- Keep the ambiguity lint; keep legacy role/literal parsing during the transition
  (pruned in Step R6 once the corpus no longer exercises it).
- `PlatePartitioner.partition_features` / `partition_into_plates`
  (src/qt-resources/python/api/PlatePartitioning.py:143/567, currently
  `:rtype: depends on *partition_return* (see table below)`): with bracket syntax,
  first try writing the honest flat union directly in the restyled `:rtype:` (read the
  table); fall back to a `MANUAL_OVERRIDES` entry only if the union is unwieldy.

### Step R4 — Docstring type-field sweep (corpus-wide)   [model: Opus 5 (script), Fable 5 (hand restyles)]   ✔ DONE (f6ec0c79b, 33e30aa80)

Applies the style guideline (see `doc-python-api/PLAN.md`) to ALL `:rtype:`/`:type:`
fields in `src/api/*.cc` + `src/qt-resources/python/api/*.py`. Compiled-in → rebuild
pygplates afterwards. Two parts, two commits:

1. **Mechanical conversion script** (scratchpad one-off, not committed; review the full
   diff by hand). Transforms ONLY `:type:`/`:rtype:` field text, multi-line aware
   (fields wrap across C++ string-literal continuation lines):
   `` :class:`X` `` → `X`; alt-text `` :class:`words<X>` `` → `X`;
   ``` ``X`` ``` → `X`; `*X*` → `X` (e.g. `*PropertyReturn.exactly_one*` enum-value
   lists keep the listed values, italics removed). Everything else byte-identical.
   The ~245 role-only rtypes and most `:type:` fields convert this way.
2. **Hand restyles** (~60 complex sites from the survey): flat-union `:rtype:` per the
   guideline, conditions moved to `:returns:` (adding a `:returns:` where the real type
   info currently lives only there). Key sites:
   `calculate_plate_boundary_statistics` (PyTopologicalSnapshot.cc:3012-3014,
   `` ``list`` or ``dict`` ``), `get_scalar_values` (PyTopologicalModel.cc:1843-1847),
   the 9 `` ``list`` or ``None`` `` ReconstructedGeometryTimeSpan accessors
   (PyTopologicalModel.cc:1694-1941), the 5 bare-`tuple` LocalCartesian returns
   (+ `list of tuple`, PyLocalCartesian.cc:789-887), the PyFeature.cc
   get/set_geometry family (:5238, :5393, :5544, :5577 — incl. the 3 single-backquote
   `` `dict` `` mis-markups), `type(*default*)` → `type(default)` (18 sites),
   ReconstructionGeometries.py:38/189/319 (`N-tuple appending a str` → explicit
   bracket tuples), Crossovers.py:542-543, `2D numpy array ...` →
   `numpy.ndarray` (GeometriesOnSphere.py:123/201), and the malformed
   PyGreatCircleArc.cc:445 (`list :class:`points<PointOnSphere>``).
   Also: `:returns:` goes before `:rtype:` (3 reversed sites), `:return:` →
   `:returns:` (3 sites).

### Step R5 — Delete the `[*staticmethod*]` markers   [model: Haiku 4.5]   ✔ DONE (1b30bd0ad)

- Remove all **56** `[*staticmethod*] ` markers plus the recurring explanatory C++
  comment above each ("Documenting 'staticmethod' here since Sphinx cannot
  introspect...") across 16 src/api/*.cc files (PyQualifiedXmlNames 12,
  PyFiniteRotation 8, PyFeature 7, PyVector3D 5, PyNetRotation 4, PyLocalCartesian 4,
  and 2 or fewer in each of 10 more). **Do not touch the `.staticmethod("...")` calls.**
- Then remove `_STATICMETHOD_MARKER_RE` and its stripping logic from generate_stub.py
  (lines 86-89 and use) — the staticmethod signal is already
  `inspect.getattr_static` (1318-1319), unchanged.
- HTML result: the auto-detected *static* prefix remains; the duplicated body marker
  disappears.

### Step R6 — Rebuild, regenerate, prune, verify   [model: Sonnet 5 (prune: Fable 5)]   ✔ DONE

> Outcome (2026-08-05): items 1-2 verified via an annotation-only (ast-based) diff
> against the stale committed stub — all 45 changed member signatures were either
> prior-`Any` worklist fallbacks landing or entries on the deliberate-changes list in
> item 2; the one "new" member (`RotationModel.__init__(self, rotation_model)`) is R1's
> overload-reorder fix regenerated for the first time. Unparsed worklist 0, missing
> fields unchanged (33), no new lint warnings.
>
> Item 3 (prune) was driven by corpus evidence — a hook on
> `TypeExpressionParser.parse()` recorded all 309 unique type-field texts the built
> module actually feeds the parser. **Pruned** (0 corpus hits each): the `2D numpy
> array` special-case, named-tuple normalization, the `N-tuple of` count-word rewrite
> and the `N-tuple of A and B` distinct-elements arm (the homogeneous `tuple of three
> X` form and the `2-tuple (A, B)` arity-prefix rewrite stay — 1 live site each), the
> double-backquote literal token path (`_LITERAL_TYPES`), the `*emphasis*` token path
> (`_resolve_emphasis`), the `:meth:`-commentary/markup group classification in
> `_parse_suffix` (`_is_type_markup` deleted — every parenthesised suffix group is now
> commentary, per the guideline that alternatives/conditions no longer live in type
> fields), and the dead legacy-role sub-branches (`list :class:` missing-`of`,
> class-object role arm, role-atom `of`/`containing` tails, one-of-the-values emphasis
> arm). **Kept, contrary to the plan's expectation**: the where-gloss machinery (3 live
> sites, e.g. `sequence of rings (where ring is ...)`) and the `:class:` role token
> path — `src/api/PyRevisionedVector.h` builds templated docstrings with
> `:class:`-role element types, and the PyTopologicalModel.cc nested-class bullet
> fields use roles by design; both were outside the R4 sweep's `*.cc`/`*.py` type-field
> scope. Ambiguity lint kept. Post-prune regeneration is byte-identical to the
> pre-prune stub with an identical stderr report, so the prune is a proven no-op on
> output; items 4-5 re-verified (spot-checks, mypy clean, determinism,
> `pygplates-stub-test` passes).
>
> Item 6 (user's VS Code re-check) verified 2026-08-07 — Round 2 is complete.

1. Rebuild: `cmake --build build-pygplates-vs --config Release --target pygplates`.
2. Regenerate the stub. **Invariant check**: members whose fields already parsed keep
   byte-identical annotations; the underspecified/conditional sites now emit full
   generics (e.g. `calculate_plate_boundary_statistics ->
   list[PlateBoundaryStatistic] | dict[ResolvedTopologicalSharedSubSegment, list[PlateBoundaryStatistic]]`).
   stderr expectations: **unparsed** worklist → 0 (or lists only deliberate
   leftovers); **missing-fields** unchanged (Crossover/CrossoverTypeFunction still
   deliberately out of scope); no new lint warnings.
   **Deliberate annotation changes from the R4 hand restyles** (expected diffs beyond
   the previously-unparsed sites - everything else must be byte-identical):
   - `TopologicalSnapshot.get_point_velocities/get_point_strain_rates` and
     `ReconstructSnapshot.get_point_velocities`: element types gain `| None` (points
     outside all topologies/polygons yield `None` elements), e.g.
     `list[Vector3D | None] | tuple[list[Vector3D | None], list[TopologyPointLocation]]`.
   - `TopologicalSnapshot.reconstruct_points -> list[PointOnSphere | None]` (was
     `list[PointOnSphere | None]` too - unchanged - but now via the flat style);
     `ReconstructSnapshot.get_point_locations -> list[ReconstructedFeatureGeometry | None]`.
   - `TopologicalSnapshot.get_resolved_topologies`,
     `ReconstructSnapshot.get_reconstructed_features/get_reconstructed_geometries`:
     bare `list` → full generics.
   - `find_crossovers(crossover_filter)` / `synchronise_crossovers(crossover_filter)`:
     callables gain the `[Crossover]` parameter list;
     `synchronise_crossovers(crossover_results)` → `list[tuple[Crossover, int]] | None`;
     `synchronise_crossovers(crossover_type_function)` gains `| None`.
   - `Feature.get_geometry/get_geometries/get_all_geometries`: coverage `dict` is now
     `dict[ScalarType, list[float]]` (also updates the spot-check in item 4);
     `Feature.get_shapefile_attributes -> dict[str, int | float | str] | Any | None`.
   - `GpmlIrregularSampling.get_enabled_time_samples -> list[GpmlTimeSample]`;
     `GpmlIrregularSampling.get_time_samples_bounding_time` unchanged but restyled;
     `NetRotationSnapshot.get_net_rotation` full generics; LocalCartesian
     magnitude/azimuth/inclination returns → `tuple[float, float, float]` (and
     `list[...]` for the sequence overload);
     `ReconstructedGeometryTimeSpan` accessors → `list[<element>] | None`;
     `get_scalar_values -> list[float] | dict[ScalarType, list[float]] | None`;
     `partition_features`/`partition_into_plates` → the three-way union (their old
     `Any` came from the worklist, so this lands via the unparsed→parsed path).
3. **Prune now-dead grammar** (less complexity was an explicit goal): where-gloss
   machinery, `N-tuple of A and B`, named-tuple normalization, `2D numpy array`
   special-case, `:meth:`-commentary heuristic, and the role/literal token paths —
   delete each only after verifying the swept corpus no longer exercises it (no
   worklist regression); keep the ambiguity lint.
4. Spot-checks: `RotationModel` has 3 `__init__` overloads (narrow one before the
   adapt-overload); `to_lat_lon_array -> numpy.ndarray` (+ `import numpy` in header);
   `Feature.get_all_geometries -> list[GeometryOnSphere] | list[tuple[GeometryOnSphere, dict[ScalarType, list[float]]]]`;
   `find_crossovers -> list[Crossover]`.
5. `mypy pygplates/stub/__init__.pyi` → clean (no "will never be matched"); regenerate
   twice → byte-identical; `ctest -C Release -R pygplates-stub-test` passes.
6. User re-checks in VS Code: signature help (Ctrl+Shift+Space) on
   `pygplates.RotationModel(` cycles all three constructor forms (bare hover always
   shows only the *first* overload's doc — Pylance behaviour, not a stub defect); hover
   on `calculate_plate_boundary_statistics` shows the full generic union.

### Round 2 commit split

1. Generator: overload reorder + grammar extensions + numpy + ambiguity lint
   (Steps R1-R2). ✔ 4e681e263
2. Generator: bracket syntax + identifier resolution (Step R3). ✔ e48751a46
   (+ follow-ups from dry-running the sweep: e9ad947b1 markup-free callable
   parameters, 47e5fc675 word-identifier suffixes / class-object / one-of-the-values /
   any-combination bare-word forms).
3. Docstring sweep, mechanical conversion (Step R4 part 1). ✔ f6ec0c79b
4. Docstring sweep, complex-site hand restyles (Step R4 part 2). ✔ 33e30aa80
5. `[*staticmethod*]` marker removal + `_STATICMETHOD_MARKER_RE` drop (Step R5).
   ✔ 1b30bd0ad
6. Regenerated `pygplates/stub/__init__.pyi` + grammar pruning + this PLAN.md status
   update (Step R6). ✔ (the commit carrying this line)

### Round 2 model summary

| Step | Work | Suggested model | Why |
|---|---|---|---|
| R1 | Overload reorder ✔ | **Fable 5** | Typing semantics (mypy overload-ordering rules) and prose-attachment edge cases; same session as R2. |
| R2 | Grammar extensions ✔ | **Fable 5** | The hard part — five parser features with known hazards (the `N-tuple` rewrite trap, nested `where` glosses). |
| R3 | Bracket grammar + identifier resolution | **Fable 5** | New parser surface interacting with every existing rule; the `partition_*` union judgement call. |
| R4 | Type-field sweep | **Opus 5** (script) + **Fable 5** (hand restyles) | The script must edit wrapped C++ string literals without touching prose; the ~60 hand sites need per-site semantic reading and `:returns:` rewrites. |
| R5 | Marker deletion | Haiku 4.5 | Mechanical removal of a fixed marker + comment pattern across 16 files. |
| R6 | Rebuild/regenerate/prune/verify | Sonnet 5 (prune: **Fable 5**) | Mechanical execution and reporting, but deleting grammar safely needs the parser author's judgement. |
