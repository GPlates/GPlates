# The pyGPlates type stub

`__init__.pyi` is the PEP 561 type stub for the `pygplates` package. It is generated from the
*built* module by `generate_stub.py` and committed here alongside `py.typed`. This file records why
it is laid out as it is and how to keep it honest. The regeneration command is in `AGENTS.md`
("Python API docstrings and the `.pyi` stub"); the docstring conventions the generator parses are
in `docs/pygplates/README.md`.

## Why introspection, and why the stub is committed

pyGPlates is a Boost.Python extension, so IDEs and type checkers see no type information. The only
machine-readable description of the API is the hand-written docstring convention (a bare signature
on line one, real types in `:type x:`/`:rtype:` fields), and the generator reads it from the
imported module rather than from the C++ sources: there is no pybind11-style stubgen for
Boost.Python, mypy's stubgen cannot read the ReST fields, and parsing C++ string literals would be
far more fragile than reading what Boost.Python has already assembled - which also gives real
`staticmethod` descriptors, properties, enums, bases and the injected pure-Python members
(`src/qt-resources/python/api/*.py`) for free.

That import is impossible in a conda-forge cross-compile (`cross-python` in
`pygplates/conda/meta.yaml`: the built module is for another platform), and an arbitrary sdist
build cannot be relied on to import either. So the stub is committed, `cmake/modules/Install.cmake`
copies the committed copy into the package, and generation is deterministic (sorted names, no
timestamps, fixed formatting) so that `pygplates-stub-test` (`CMakeLists.txt` here) can regenerate
from the built module and fail on any difference. Cross-compiled builds never run CTest, so
they simply ship what is committed. Determinism also keeps the repository small: git stores the
file as deltas, so an API change costs kilobytes, not another copy of a megabyte.

## Layout

- `__init__.pyi` + `py.typed`, installed into the `pygplates` package directory. pyGPlates installs
  as a *package* (an `__init__.py` doing `from .pygplates import *` around the shared library), so a
  bare `pygplates.pyi` would stub only the private submodule; a checker resolving `import pygplates`
  needs the package's `__init__.pyi`, and mypy honours inline stubs only when `py.typed` is present.
- There is deliberately **no** second `pygplates/pygplates.pyi`. The private submodule is
  load-bearing (its name is embedded in every pickle ever written, `dill` resolves it by `getattr`
  on the parent, and it names every exception in a traceback - see `cmake/modules/Install.cmake`),
  but it is not a surface users should type against, and a second stub would duplicate every
  member and double the freshness burden. `PackageCase` in `pygplates/test/test.py` covers it at
  runtime instead.
- Full docstrings are embedded (signature lines stripped) because Pylance never imports a module:
  hover documentation comes only from the stub, and omitting them would regress hover against
  Pylance's own scraping of an unstubbed module.
- `*.pyi` is pinned to LF in `.gitattributes` because `--output` always writes LF, so on a CRLF
  checkout every regeneration would show the whole file as modified. (`--check` reads the committed
  stub with universal newlines, so it would not fail either way.)
- The stub must not depend on the Python or Boost version that generated it, or
  `pygplates-stub-test` fails for any developer whose versions differ from the committer's. So the
  generator skips the attributes Python 3.13 adds to every class statement (`__firstlineno__`,
  `__static_attributes__`), emits a `namedtuple` as a `typing.NamedTuple` rather than listing what
  `namedtuple` generates (`__replace__` is new in 3.13), and names nested classes by where it finds
  them in the module rather than by `__qualname__`, which Boost.Python 1.74 leaves unqualified.

## The maintenance loop

1. Build pyGPlates (Release) and regenerate (command in `AGENTS.md`). `generate_stub.py` is
   stdlib-only - it runs under `GPLATES_PYTHON_EXECUTABLE`; the *emitted* stub uses stub-only
   syntax (`X | None`, `list[X]`) freely.
2. Read stderr. `WARNING:` lint is printed first, then up to three deduplicated worklists; none of
   it is an error:
   - **Unparsed type expressions** (emitted as `Any`): the `:type:`/`:rtype:` text defeated
     `TypeExpressionParser`. The fix is normally a docstring restyle to the guideline in
     `docs/pygplates/README.md`; extend the grammar only for natural English that many docstrings
     share. `MANUAL_OVERRIDES` is the escape hatch for a signature the convention cannot express.
   - **Missing `:type:`/`:rtype:` fields** (emitted as `Any`): add the field. The
     `CrossoverTypeFunction` entries are deliberately left on this list. (The `Crossover` named
     tuple's fields are `Any` too, but it is not reported: it is emitted as a `typing.NamedTuple`.)
   - **Suspicious `:rtype:` omissions** (emitted as `-> None`): a C++ `void` function omits the
     field by convention, but this prose says it returns something - probably a forgotten field.
   - `WARNING:` lines are lint, chiefly the `list of A or list of B` ambiguity: a bare `or` binds
     to the `of` element, giving `list[A | list[B]]`. Write `, or` for a top-level alternative.
3. `mypy pygplates/stub/__init__.pyi` must be clean (the generator already refuses to write a stub
   that does not `ast.parse`). The usual failure is overload order: mypy rejects an overload that
   comes *after* one subsuming it ("will never be matched"), which is why the generator emits a
   subsumed overload before its subsumer even though Boost.Python registered it later.
4. `ctest --test-dir build-pygplates -C Release -R pygplates-stub-test`. Pass/fail is by exit code
   only - deliberately no `FAIL_REGULAR_EXPRESSION`, since the printed docstring diff can contain
   the word "fail".

`python -m mypy.stubtest pygplates` against an installed wheel is advisory at best: Boost.Python
function objects are not inspectable, so it reports every def as noise.

## Safe docstring sweeps

A style sweep over the two docstring corpora (`src/api/*.cc` and `src/qt-resources/python/api/*.py`)
changes the stub's embedded text, so the file always changes - but the *annotations* should not.
Check that with an AST diff: parse the committed stub and the regeneration, compare every
function, overload and property annotation by qualified name, and expect zero differences except at
the sites the sweep meant to improve (previously `Any`).

## Safe grammar pruning

The parser grew several forms while the corpus converged on its current style, and some are now
unused. To prune safely: hook `TypeExpressionParser.parse()` to record every unique type-field text
the built module feeds it, delete only grammar with zero hits, and require the regenerated stub
*and* the stderr report to be byte-identical afterwards. Two paths that look dead but are not: the
`:class:` role token path (`src/api/PyRevisionedVector.h` templates docstrings with role element
types, and the nested-class bullet fields in `src/api/PyTopologicalModel.cc` use roles by design)
and the `where <name> is ...` gloss machinery (`sequence of rings (where ring is ...)` is live).

## Release check (manual)

After building a wheel, confirm it contains `pygplates/__init__.pyi` and `pygplates/py.typed`, then
open a project that uses it in VS Code: `pygplates.RotationModel(` should offer three constructor
overloads in signature help (Ctrl+Shift+Space), `PropertyReturn.first` should resolve, and hover on
`get_rotation` should show the full docstring. Bare hover on an overloaded member shows only the
*first* overload's docstring - that is Pylance behaviour, not a stub defect.
