# AGENTS.md

Guidance for AI coding agents working in this repository.

## One repo, two products

GPlates (Qt desktop app) and pyGPlates (Boost.Python extension module) are built from the
same sources, selected by the CMake option `GPLATES_BUILD_GPLATES`. It is declared in
`cmake/modules/Version.cmake` (not `ConfigDefault.cmake`) and **defaults to `TRUE`**, so a
pyGPlates build that omits `-DGPLATES_BUILD_GPLATES=FALSE` silently builds GPlates instead.

Either product can be built from either develop branch. Default to the product matching the
checked-out branch — `pygplates` branch to `build-pygplates/`, `gplates` branch to
`build-gplates/` — but building the other product from the same worktree is normal and fine.

Versions live in `cmake/modules/Version.cmake`: `GPLATES_SEMANTIC_VERSION` and
`PYGPLATES_PEP440_VERSION`.

## Build

Dependencies come from conda-forge; the environment files are per-platform and all create an
environment named `gplates`.

```
conda env create -n gplates -f env.Windows.yml   # or env.macOS.yml / env.Linux.yml
conda activate gplates
```

```
cmake -S . -B build-pygplates -G Ninja -DCMAKE_BUILD_TYPE=Release -DGPLATES_BUILD_GPLATES=FALSE
cmake -S . -B build-gplates   -G Ninja -DCMAKE_BUILD_TYPE=Release -DGPLATES_BUILD_GPLATES=TRUE
cmake --build <build-dir>
```

- Do **not** pass `-DCMAKE_PREFIX_PATH` or `-DBoost_ROOT`. The root `CMakeLists.txt` detects
  `CONDA_PREFIX` itself. Configure output must include `Detected active conda environment`; if
  it does not, the environment was not activated and the build will find the wrong dependencies.
- The conda environment must use OpenBLAS, never MKL (`libblas=*=*openblas`). MKL cannot be
  bundled into a standalone GPlates, and numpy then fails to import.
- **Windows: plain PowerShell cannot configure a Ninja build.** `conda activate` runs the
  `vs2022_win-64` vcvars script in a nested `cmd`, so the compiler environment never propagates
  back and CMake reports "No CMAKE_C_COMPILER could be found". Use **Anaconda Prompt (cmd)** or
  **Developer PowerShell for VS 2022**. CI does the same via `shell: cmd /C call {0}`.
- `GPLATES_USE_PRECOMPILED_HEADERS` must stay `FALSE` in CI — sccache cannot cache PCH
  translation units.
- `.clangd` is **generated** on every configure from the committed `.clangd.in`, pointing at the
  most recently configured in-source build tree. Edit `.clangd.in`, never `.clangd`. Generation
  is skipped for out-of-tree builds (`pip install .`, `conda build`) and for VS/Xcode generators.

Full instructions: @BUILD-Windows.md, @BUILD-macOS.md, @BUILD-Linux.md

## Test

CTest is the only top-level test runner, and **`-C Release` is mandatory**:

```
ctest --test-dir build-pygplates -C Release --output-on-failure
ctest --test-dir build-gplates   -C Release --output-on-failure
```

Without `-C Release`, CTest filters out every test and **reports success having run nothing** —
the tests are registered `CONFIGURATIONS Release MinSizeRel` because `GPlatesGlobal::Assert`
aborts in Debug rather than throwing.

The GPlates unit-test binary is `EXCLUDE_FROM_ALL`, so build it explicitly:

```
cmake --build build-gplates --target gplates gplates-unit-test
```

Use GoogleTest for all new C++ tests; do not mix frameworks. Conventions (headless,
working-directory independent, `GPLATES_UNIT_TEST_DATA_DIR`, `QTemporaryDir`, and leaving
`git status` clean) are in @doc-cpp/design/testing/README.md.

## Python API docstrings and the `.pyi` stub

Docstrings are hand-written in **two** locations — `src/api/*.cc` (C++) and
`src/qt-resources/python/api/*.py` (Python) — and both are compiled into the module. A given
entity's docstring lives in exactly one of them, so edit it where the entity is defined; but
check both when searching, and **any convention or style sweep must cover both**.

`pygplates/stub/generate_stub.py` parses the `:type:`/`:rtype:` fields to produce
`__init__.pyi`, so docstring wording is load-bearing: a change can stale the stub and fail
`pygplates-stub-test`. After changing the Python API surface or its docstrings, regenerate and
verify the stub rather than leaving it stale.

```
python pygplates/stub/generate_stub.py --module-dir <dir-containing-built-pygplates> --check <committed-stub>
```

`*.pyi` is pinned to LF in `.gitattributes` because `pygplates-stub-test` compares bytes.

The docstring conventions are strict and are the highest-value style document in the repo:
@doc-python-api/README.md

## Docs

```
conda env update -n gplates -f env.docs.yml
cmake --build <build-dir> --config Release --target doc-python-api
```

Sphinx must run in the same interpreter pyGPlates was built against (autodoc imports the module
in-process), which is why the target invokes `${GPLATES_PYTHON_EXECUTABLE} -m sphinx` rather than
a `sphinx-build` on `PATH`. Sphinx runs with `-W`, so warnings are errors.

**Build docs from scratch for anything published.** An incremental rebuild silently drops *all*
index entries from `searchindex.js` while leaving the HTML identical. Delete
`<build-dir>/doc-python-api/_doctrees` and `generated/`, or pass `-E`. Removing or renaming a
class also requires deleting its stale `generated/*.rst` or the build fails.

## Code style

There is no `.clang-format` or `.clang-tidy` in this repo, but the existing C++ is consistent and
new code is expected to match it. Measured over `src/`:

- **Hard tabs** for indentation, never spaces (~97% of files).
- **Allman braces throughout** — the opening brace goes on its own line for namespaces, classes and
  functions *and* for `if` / `for` / `while` bodies (~92%). Do not use K&R style.
- **Wrap at 100 columns.** (Much of the existing tree wraps nearer 80; 100 is the current target,
  and ~97% of existing lines already fit it. Don't rewrap old code to suit it.)
- **Pointer and reference tokens bind to the name**: `Type *name`, never `Type* name`.
- **Data members are prefixed `d_`** — `d_feature_ref`, `d_is_active`.
- **Function and method signatures always break across lines**, however short they are. The return
  type goes on its own line, the name and `(` on the next, and **every** parameter on its own line
  indented **two tabs** relative to the name (~93% of multi-parameter signatures). The closing `)`
  stays on the last parameter's line, carrying any trailing qualifiers. A leading `virtual` or
  `static` goes on its own line too, above the return type:

  ```
  boost::optional<PointOnSphere>
  is_close_to(
  		const PointOnSphere &test_point,
  		const AngularExtent &closeness_angular_extent_threshold,
  		real_t &closeness) const;

  static
  non_null_ptr_type
  save_session(
  		QString project_filename);
  ```

  When a single parameter is itself too long to fit, wrap it however reads best — there is no fixed
  rule for that case.

- Every file opens with the **GPL-2 header block** carrying the `$Id$` / `$Revision$` / `$Date$`
  keywords; copy the block from a neighbouring file.
- Include guards are `GPLATES_<DIR>_<FILE>_H`.
- Group `#include`s by source subdirectory, separated by blank lines.

Where this guidance and a specific file disagree, match the file you are editing.

## Branches and pull requests

The branching model is a gitflow variant, described in @README.md.

- **develop** branches: `gplates` (the repository's default branch) and `pygplates`. These are
  kept closely in sync — GPlates-related work is done on `gplates` and pyGPlates-related work on
  `pygplates`, but they are otherwise near-identical.
- **main** branches: `release-gplates` and `release-pygplates` track release history.
- Short-lived branches: `feature/<name>`, `release/{gplates,pygplates}-<version>`,
  `hotfix/{gplates,pygplates}-<version>`.

**Base pull requests on the develop branch you are working from — `pygplates` or `gplates` —
never on a `release-*` branch.** CI enforces this: `build-test-pygplates.yml` only runs on
`pygplates` and `build-test-gplates.yml` only on `gplates`.

The GitHub remote is `https://github.com/GPlates/GPlates.git`. This checkout names it `public`
rather than the usual `origin`, so **always name the remote explicitly** in push and fetch
commands. There is an active downstream fork tracking the `gplates` branch, so changes merged
there warrant extra care.

## Releases (pyGPlates wheels)

Set `PYGPLATES_PEP440_VERSION` in `cmake/modules/Version.cmake`, commit, then tag **exactly**
`PyGPlates-<version>`; the workflow fails in its first minute on a mismatch or a `.dev` version.
Publishing uses PyPI Trusted Publishing (OIDC, no tokens) and pauses for manual approval on the
`pypi` deployment environment. **Renaming `.github/workflows/build-wheels.yml` silently breaks
publishing** — the trusted-publisher registration binds to the filename. Adding a Python version
requires updating both `[tool.cibuildwheel].build` in `pyproject.toml` and `PYTHON_VERSIONS` in
`pygplates/wheel/versions.sh`, or the sdist job fails. Details: @pygplates/wheel/README.md

## Working agreements

- Propose a plan and get agreement before multi-file refactors or changes to CMake or CI.
- `pyproject.toml` is heavily commented and is the authoritative reference for the
  scikit-build-core and cibuildwheel configuration; read those comments before changing it. Note
  that its per-platform `config-settings` tables *override* rather than merge with the base table.
- The `pygplates.pygplates` private submodule must not be flattened: `dill` resolves dotted names
  via `getattr` on the parent package, unlike stdlib `pickle`.
