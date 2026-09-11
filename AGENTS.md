# AGENTS.md

Guidance for AI coding agents working in this repository.

## One repo, two products

GPlates (Qt desktop app) and pyGPlates (Boost.Python extension module) are built from the
same sources, selected by the CMake option `GPLATES_BUILD_GPLATES`. It is declared in
`cmake/modules/Version.cmake` (not `ConfigDefault.cmake`) and **defaults to `TRUE`**, so a
pyGPlates build that omits `-DGPLATES_BUILD_GPLATES=FALSE` silently builds GPlates instead.

There is one development branch, `gplates`, and both products are developed on it. Building
either one from the same worktree is normal; keep them in separate build trees
(`build-pygplates/`, `build-gplates/`) so neither reconfigure invalidates the other.

Versions are **derived, not written**. `cmake/modules/VersionRelease.cmake` holds the release
each line is heading towards (`GPLATES_RELEASE_VERSION`, `PYGPLATES_RELEASE_VERSION`), and
`cmake/modules/VersionFromGit.cmake` adds a development number counted from the first-parent
distance to the nearest release tag — giving `GPLATES_SEMANTIC_VERSION` (eg, `2.6.0-47`) and
`PYGPLATES_PEP440_VERSION` (eg, `1.1.0.dev46`) in `cmake/modules/Version.cmake`. Do not add a
literal version back: a hand-incremented development number has to anticipate the order in
which branches *merge*, which is why the old one repeatedly collided and drifted.

To see what a checkout resolves to, without configuring a build:

```
cmake -P cmake/modules/VersionFromGit.cmake pygplates
cmake -P cmake/modules/VersionFromGit.cmake gplates
```

Counting needs the full history, so a shallow clone is refused rather than allowed to produce
a plausible but wrong number. A build with no repository at all (a source archive, or a wheel
built inside the Linux container) takes the versions from `-D` defines, environment variables
of the same names, or the generated `cmake/modules/VersionRecorded.cmake` that ships in the
sdist; `VersionFromGit.cmake` documents the whole resolution order.

**Both versions are always resolved, whichever product is being built** — `src/global/Version.cc`
is compiled into pyGPlates as well, and the GPlates version string reaches user data (exported
shapefiles carry it). So supplying only `PYGPLATES_PEP440_VERSION` to a git-less pyGPlates build
is not enough.

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
- The environment files install **Qt6**. A Qt5 build needs a *second* environment, hand-edited
  as the comment at the top of each `env.*.yml` describes (`qt-main<6` and `qwt<=6.2.0`;
  Windows may optionally use `vs2019_win-64`). Agents: **do not create or modify conda
  environments unprompted** — if the environment a task needs is missing, say so and ask.
  Which environments exist, and what they are called, is the developer's choice.
- `.clangd` is **generated** on every configure from the committed `.clangd.in`, pointing at the
  most recently configured in-source build tree. Edit `.clangd.in`, never `.clangd`. Generation
  is skipped for out-of-tree builds (`pip install .`, `conda build`) and for VS/Xcode generators.

Full instructions: `BUILD-Windows.md`, `BUILD-macOS.md`, `BUILD-Linux.md`.

## Test

CTest is the only top-level test runner. **Always configure Release and always pass
`-C Release`:**

```
ctest --test-dir build-pygplates -C Release --output-on-failure
ctest --test-dir build-gplates   -C Release --output-on-failure
```

A Debug test run is unsupported either way, because `GPlatesGlobal::Assert` calls `std::abort()`
in Debug instead of throwing, which kills every test that exercises an error path. But the two
products enforce that differently, and the difference decides how you fix an empty run:

- **pyGPlates** tests are registered `CONFIGURATIONS Release MinSizeRel`. Omitting `-C Release`
  matches nothing and CTest still **exits 0**, so the run looks like a pass.
- **GPlates** tests are registered by `gtest_discover_tests()`, which cannot attach
  `CONFIGURATIONS`, so they are instead skipped at *configure* time by
  `if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")` (`src/CMakeLists.txt`). A Debug build tree contains
  no registered tests at all and **no `-C` value will reveal any** — reconfigure as Release. On a
  single-config Release tree a bare `ctest` does work; `-C Release` matters for the multi-config
  generators (Visual Studio, Xcode).

So: zero tests from `build-pygplates` usually means a missing `-C Release`; zero tests from
`build-gplates` usually means the tree was configured Debug.

The GPlates unit-test binary is `EXCLUDE_FROM_ALL`, so build it explicitly:

```
cmake --build build-gplates --target gplates gplates-unit-test
```

Use GoogleTest for all new C++ tests; do not mix frameworks. Conventions (headless,
working-directory independent, `GPLATES_UNIT_TEST_DATA_DIR`, `QTemporaryDir`, and leaving
`git status` clean) are in `doc-cpp/design/testing/README.md`.

## The pyGPlates module boundary

The pygplates module compiles only the **include closure of the pyGPlates API** — not the
whole tree. The layering, the rules for new files (which directory kind defaults to
GPlates-only, `.h`/`.cc` pairing for AUTOMOC, no `QMessageBox` in shared code) and the
enforcement are described in `doc-cpp/design/architecture/README.md`. Two pyGPlates CTests
enforce the boundary: `pygplates-source-closure-test` (the source list must equal the
closure computed by `cmake/pygplates_source_closure.py`, which also drift-checks the
committed dependency matrix) and `pygplates-linkage-test` (`cmake/check_linkage.py` - the
built module must have no direct dependency on GPlates' GUI/rendering libraries). When
either fails after adding a file or an `#include`, the failure message says which CMake list
to fix — do that rather than weakening the tracer.

**CI builds both products on every push**, which is what makes the boundary enforceable: the
two CTests above are pyGPlates tests, so only a pyGPlates build can run them. Until the develop
branches were unified each workflow ran on its own branch and built only its own product, and a
change to the shared sources or the CMake source lists could break the other product undetected
— discovered at the next sync merge, where it looked like the merge's fault. Both defects PR #70
fixed had been hiding in exactly that gap.

What CI still does **not** cover is Qt5. Build GPlates under Qt5 locally before pushing a change
that could plausibly be Qt-version-sensitive (any `QT_VERSION` conditional, `qt-widgets`, or a Qt
include whose header moved between Qt5 and Qt6). That needs a second conda environment (see
*Build* above); if it is not set up, say what you could not verify rather than skipping it
silently.

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
# Rewrite the committed stub, and report whether it had drifted:
python pygplates/stub/generate_stub.py --module-dir <dir-containing-built-pygplates>     --output pygplates/stub/__init__.pyi --check pygplates/stub/__init__.pyi
```

`--check` only compares and exits non-zero; `--output` is what actually rewrites the stub.

`*.pyi` is pinned to LF in `.gitattributes` because `pygplates-stub-test` compares bytes.

The docstring conventions are strict and are the highest-value style document in the repo:
`doc-python-api/README.md`

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
- **Pointer and reference tokens bind to the name**: `Type *name` and `Type &name`, never
  `Type* name` or `Type& name`. This one wins over the file-matching rule below: a few older
  files bind them to the type, and new code in them still binds to the name.
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

This is **no longer** gitflow — it is the trunk-plus-release-series model, described in `README.md`
and argued for in `doc-cpp/design/versioning/README.md`.

- **`gplates`** is the single development branch and the repository's default. Both products are
  developed on it; there is no per-product branch. (It will be renamed `main` in a later change.)
- **release series** branches — `release/gplates-<X.Y>`, `release/pygplates-<X.Y>` — are
  permanent, cut from `gplates` when the first release in the series is prepared. **Release tags
  live only here**, never on `gplates`: candidates, the release, and each later patch release are
  successive commits on the one branch, so the tip is always the newest X.Y.z. There is no
  `hotfix/` concept and no permanent 'production' branch.
- **short-lived** branches: `feature/<name>` and `fix/<name>` off `gplates` (a fix rather than a
  feature, but otherwise identical), and patch branches off a release series branch.

**Base pull requests on `gplates`, never on a release series branch.** CI enforces this: both
`build-test-gplates.yml` and `build-test-pygplates.yml` run only on `gplates`. Building **both**
products on every push is deliberate — it is what closes the coverage gap that two develop
branches used to leave open (see *The pyGPlates module boundary*).

Pull requests are merged with a merge commit (`Merge pull request #N from …`), so a branch's
commits become the permanent record. **Whether to tidy a branch before merging is a judgement
about that branch, not a convention** — there is no squash policy. Ask what a reader hitting the
commit in
`git log` or `git bisect` a year from now gets from it:

- **Squash** commits that exist only because of iteration — "fix the CI", "try again", a typo
  fixed three commits later, a build fixed in the next commit. They carry no information and
  they make `bisect` land on broken trees. The cibuildwheel PRs each landed as one or two
  commits for exactly this reason; the CI thrash behind them was worth nothing to anyone.
- **Keep** commits that record a distinct decision — including a revert whose message says why
  the change turned out to be unnecessary. That is history, not churn.

When in doubt, keep. Rewriting a branch that has already been pushed is a decision for the
author, not something to do in passing: force-pushing detaches any review comments, and other
people may have fetched it.

The GitHub remote is `https://github.com/GPlates/GPlates.git`, usually named `origin`. Some
checkouts give it another name and have no `origin` at all, so **name the remote explicitly** in
push and fetch commands rather than assuming. There is an active downstream fork tracking the
`gplates` branch, so changes merged there warrant extra care.

## Releases (pyGPlates wheels)

Set `PYGPLATES_RELEASE_VERSION` in `cmake/modules/VersionRelease.cmake` to the release version,
commit, then tag **exactly** `PyGPlates-<version>` on the release series branch
`release/pygplates-<X.Y>` (release tags belong only there — see *Branches and pull requests*); the workflow fails in its first
minute on a mismatch or a `.dev` version. Standing on the tag, the derived version *is* the
release target (no development number), which is what makes the two agree. Afterwards set the
target to the next release, or the following commit resolves to a version sorting below the one
just released — a hard error rather than a bad package. The resolver also refuses a target that
sorts below the nearest release or skips a version, so the next target has to be the next patch,
minor or major (or a candidate of one); `cmake -P cmake/modules/VersionFromGitTest.cmake` runs
those rules as tests.
Publishing uses PyPI Trusted Publishing (OIDC, no tokens) and pauses for manual approval on the
`pypi` deployment environment. **Renaming `.github/workflows/build-wheels.yml` silently breaks
publishing** — the trusted-publisher registration binds to the filename. Adding a Python version
requires updating both `[tool.cibuildwheel].build` in `pyproject.toml` and `PYTHON_VERSIONS` in
`pygplates/wheel/versions.sh`, or the sdist job fails. Details: `pygplates/wheel/README.md`.

## Working agreements

- Propose a plan and get agreement before multi-file refactors or changes to CMake or CI.
- `pyproject.toml` is heavily commented and is the authoritative reference for the
  scikit-build-core and cibuildwheel configuration; read those comments before changing it. Note
  that its per-platform `config-settings` tables *override* rather than merge with the base table.
- The `pygplates.pygplates` private submodule must not be flattened: `dill` resolves dotted names
  via `getattr` on the parent package, unlike stdlib `pickle`.
