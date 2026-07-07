# Branch-unification map: everything OUTSIDE the core C++ model divergence

> Exploration report (agent 3 of 3) for the pygplates→gplates branch unification. See `MERGE-PLAN.md`.

**Key upfront finding:** The build system is *already* substantially prepared for unification. The top-level `CMakeLists.txt` is **byte-identical** between `gplates` and `pygplates`, and both already branch on `GPLATES_BUILD_GPLATES` to build either target. The real reconciliation work is concentrated in a handful of files where the *same path* holds *different content* per branch (notably `src/api/CMakeLists.txt`, `src/qt-resources/python.qrc`, `cmake/modules/Version.cmake`), plus pygplates-only top-level trees.

---

## 1. Build system

**Diff scope** (`git diff gplates pygplates -- CMakeLists.txt src/CMakeLists.txt cmake/`): only 7 files differ, 64 insertions / 75 deletions total. Top-level `CMakeLists.txt` diff is **empty**.

**How each branch uses `GPLATES_BUILD_GPLATES`:** Defined once in `cmake/modules/Version.cmake:19` as an `option(...)`. Default differs only in that line: `true` on gplates, `false` on pygplates. Everything downstream keys off it identically on both branches:

- Top-level `CMakeLists.txt` (lines 26, 62): sets `project(GPlates ...)` vs `project(PyGPlates ...)`, and in the `else()` (pygplates) branch calls `enable_testing()` + appends `--output-on-failure` to `CMAKE_CTEST_ARGUMENTS` (lines 37–40). After `add_subdirectory(src)`, it branches: gplates → `add_subdirectory(doc-cpp)`; pygplates → `add_subdirectory(pygplates)` + `add_subdirectory(doc-python-api)` (lines 62–78).
- `src/CMakeLists.txt` (line 333): `if (GPLATES_BUILD_GPLATES)` sets `SOURCE_TARGET=gplates-lib`, `BUILD_TARGET=gplates`, creates the `gplates-lib` STATIC lib + `gplates`, `gplates-no-gui`, `gplates-unit-test` executables. `else()` (line ~384) sets `SOURCE_TARGET=pygplates`, `BUILD_TARGET=pygplates`, and creates the Python extension via `Python3_add_library(pygplates MODULE ScribeExportPyGPlates.cc)`.

**Source directory selection into the pygplates target:** There is **no exclusion list**. `src/CMakeLists.txt` (lines ~427–450) defines one `source_sub_directories` list — `api app-logic canvas-tools cli data-mining feature-visitors file-io global gui maths model opengl presentation property-values qt-resources qt-widgets scribe unit-test utils view-operations` — and `foreach(sub_dir ...) add_subdirectory(${sub_dir})`. The **same** list is traversed for both targets. Each sub-directory's `CMakeLists.txt` calls `target_sources_util(${SOURCE_TARGET} PRIVATE ${srcs})`, so sources land in whichever target `SOURCE_TARGET` points to. Divergence in *what gets compiled* therefore lives inside each directory's `set(srcs ...)` list, not in a top-level include/exclude. (Notably `gui`, `qt-widgets`, `canvas-tools` etc. are compiled into `pygplates` too — pygplates is a full link of the model, hence `set_target_properties(pygplates PROPERTIES POSITION_INDEPENDENT_CODE ON)`.)

**Per-directory sources mechanism:** an inline `set(srcs …)` list inside each directory's `CMakeLists.txt`, fed to the helper `target_sources_util(target …)` defined at `cmake/modules/Utils.cmake:8`. (Maintenance script: `cmake/add_sources.py`, referenced in the header comment of every directory `CMakeLists.txt`.) There is no separate `cmake_sources.txt` file on these branches.

**Is `src/api` compiled on the gplates branch?** Yes. `api` is in `source_sub_directories` on both branches. On gplates it compiles the thin embedded-console API (see §7).

**`GPLATES_NO_PYTHON` / python-embedding options:** `GPLATES_NO_PYTHON` does **not exist** on either branch — Python is mandatory. Relevant options/vars (identical between branches unless noted):
- `GPLATES_PYTHON_3` — `option(... true)` at `cmake/modules/ConfigDefault.cmake:274`.
- `GPLATES_PYTHON_EMBEDDING` — a *compile definition* (not an option) added to `gplates-lib` only, at `src/CMakeLists.txt:958`, guarded by `if (GPLATES_BUILD_GPLATES)`. Distinguishes "Python embedded in the app" from "pygplates is the module."
- `include(TestPythonEmbedding)` runs only for gplates (`src/CMakeLists.txt` ~967).

**Real diff content in `src/CMakeLists.txt`** (only substantive build difference): Boost minimum version and components. pygplates uses `_GPLATES_MIN_BOOST_VERSION 1.55` and links `Boost::system` explicitly; gplates uses `1.69` (header-only `boost::system`, dropped the `system` component). Lines ~205, ~249, ~857. **This will conflict** — pick one Boost baseline (recommend gplates' 1.69).

---

## 2. Tree-layout differences (top level)

| Entry | gplates | pygplates | Note |
|---|---|---|---|
| `doc-cpp/` | yes | yes | **Already renamed on BOTH** — see callout |
| `doc-python-api/` | no | yes | Sphinx docs, pygplates-only |
| `pygplates/` | no | yes | packaging + tests subtree |
| `pyproject.toml` | no | yes | pygplates-only |
| all others (AUTHORS, BUILD.*, CHANGELOG, CMakeLists.txt, cmake, sample-data, scripts, src, COPYING, CREDITS, DEPS.*, README.md) | yes | yes | shared |

**CALLOUT — the `doc/` vs `doc-cpp/` rename is NOT a conflict.** Both branches already have `doc-cpp/` and neither has `doc/`. The rename happened long ago on shared history (commit `07cc001b7` "Moved doxygen build folder from 'doc' to 'doc-cpp'"). Nothing to reconcile.

**`pygplates/` subtree structure:**
- `pygplates/CMakeLists.txt` — just `add_subdirectory(test)`.
- `pygplates/conda/` — `meta.yaml`, `build.sh`, `bld.bat`, `conda_build_config.yaml`, `yum_requirements.txt`, `README.txt`.
- `pygplates/wheel/` — `build_macos_wheels.sh`, `build_manylinux_wheels.sh`, `build_windows_wheels.bat`, `manylinux.dockerfile`, `README.md`.
- `pygplates/test/` — `CMakeLists.txt`, `test.py`, `fixtures/`, `test_app_logic/`, `test_maths/`, `test_model/` (see §6).

**`doc-python-api/`** (pygplates-only): Sphinx project — `conf.py.in`, `index.rst`, `pygplates_getting_started.rst`, `pygplates_introduction.rst`, `pygplates_primer.rst`, `pygplates_reference.rst`, `pygplates_sample_code.rst`, `_static/`, `images/`, `sample-code/`, `CMakeLists.txt`, `README`. Its `CMakeLists.txt` does `find_package(Sphinx)`, configures `conf.py` via `@VAR@` substitution (uses `PYGPLATES_DOCS_COPYRIGHT_STRING` from ConfigDefault), and adds a `doc-python-api` custom target depending on the built `pygplates` module.

**`pyproject.toml`** — pygplates-only (drives pip/scikit-build packaging).

**`scripts/` diff:** pygplates has `scripts/hellinger.py` (1372 lines) and `scripts/hellinger_maths.py` (2257 lines); gplates has neither at top level. gplates instead ships hellinger as an embedded Qt resource at `src/qt-resources/python/scripts/hellinger/hellinger.py` (+ `hellinger_35.png`). Hellinger exists on *both* branches but in *different locations and roles* — the gplates arrangement is the newer one (its recent commits moved it); merge auto-resolves correctly because pygplates never touched these files since the merge-base.

**`BUILD.*` diff:** trivial — `BUILD.Linux/OSX/Windows` each differ by 2 lines (the "originated from X branch" note). `README.md` also differs.

---

## 3. `src/qt-resources/python/` and the `.qrc` (CONFLICT)

**pygplates** `src/qt-resources/python/api/` contains 7 pure-Python API augmentation files: `Crossovers.py`, `Feature.py`, `GeometriesOnSphere.py`, `PlatePartitioning.py`, `Property.py`, `PropertyValues.py`, `ReconstructionGeometries.py`. These extend the C++ boost-python classes with Python-side methods and are compiled into the module as Qt resources.

**gplates** has **no** `python/api/` directory. It has `src/qt-resources/python/scripts/hellinger/hellinger.py` and `hellinger_35.png` (recent gplates app feature).

**Shared:** both have `src/qt-resources/python/scripts/colouring/{ArbitraryColours,ColorByProperty,draw_style_demo}.py`.

**Embedding/install:** via `src/qt-resources/python.qrc` (Qt resource compiler), whose owning `src/qt-resources/CMakeLists.txt` is **identical** between branches. The **conflict is in `python.qrc` itself**:
- gplates lists `python/scripts/hellinger/hellinger.py`.
- pygplates lists the 7 `python/api/*.py` files.

Resolution: **union** — both the hellinger script and the `python/api/*.py` set; both file trees coexist.

---

## 4. Versioning / packaging

**`cmake/modules/Version.cmake` (CONFLICT on values, not structure):** Both branches carry the *same dual-version machinery* — both `GPLATES_SEMANTIC_VERSION` and `PYGPLATES_PEP440_VERSION` are defined on both. Differences:

| Variable | gplates | pygplates |
|---|---|---|
| `GPLATES_BUILD_GPLATES` default (line 19) | `true` | `false` |
| `GPLATES_SEMANTIC_VERSION` (line 50) | `2.6.0-4` | `2.5.0` |
| `PYGPLATES_PEP440_VERSION` (line 81) | `1.0.0` | `1.1.0.dev5` |
| Header comment (lines 15–17) | "THIS IS CURRENTLY THE GPLATES BRANCH…" | "THIS IS CURRENTLY THE PYGPLATES BRANCH…" |

Each branch bumped *its own* product's version and left the other stale. The merge must take the max/current of each: GPlates **2.6.0-4** and pygplates **1.1.0.dev5**, and deliberately set the option default (`true` for the unified gplates branch). The version-parsing logic (lines 144+) is identical.

**`cmake/modules/ConfigDefault.cmake` (trivial CONFLICT):** only copyright years differ — gplates `2003-2026`, pygplates `2003-2025`, across `GPLATES_PACKAGE_LICENSE`, `GPLATES_COPYRIGHT_STRING`, `GPLATES_HTML_COPYRIGHT_STRING`, `PYGPLATES_DOCS_COPYRIGHT_STRING`. Take 2026. Both already define the pygplates docs copyright.

**`cmake/modules/Config_h.cmake`:** **identical** (empty diff). Already handles both variants.

**`cmake/modules/Install.cmake` (CONFLICT, 1 line):** `if (GPLATES_BUILD_GPLATES)` python-script install loop — gplates has `foreach (_script )` (empty), pygplates has `foreach (_script hellinger.py hellinger_maths.py)`. Take gplates (tied to the hellinger relocation). (In the actual merge-tree run this auto-resolved to gplates — verify.)

**`cmake/modules/Package.cmake`:** pygplates only deletes a block of macOS notarization *comments* (9 lines, no functional change). Non-conflicting.

**`cmake/modules/InstallSharedLibraryDependencies.cmake` (real functional CONFLICT, macOS):** The two branches diverge in how they codesign Python `.so` files inside the app bundle on APPLE. pygplates introduces `GPLATES_PYTHON_STDLIB_INSTALL_PREFIX` and, guarded by `if (GPLATES_BUILD_GPLATES)`, globs `.../${GPLATES_PYTHON_STDLIB_INSTALL_PREFIX}/*.so`. gplates instead walks *all* installed `.framework` dirs, collects `_installed_frameworks`, recursively codesigns `*.so` and then the frameworks themselves (fixes "a sealed resource is missing or invalid"). Two different implementations of the same codesigning step (~50 lines) — manual merge: keep gplates' broader framework-signing, preserve pygplates' stdlib-prefix variable plumbing.

**`cmake/modules/FindSphinx.cmake`:** **new file on pygplates only** (24 lines) — required by `doc-python-api/`. Purely additive.

**CI (`.github/`):** **Neither branch has `.github/`.** No GitHub Actions to reconcile.

---

## 5. `ScribeExport*` variant registration files

The four files exist with **identical names on both branches**: `src/ScribeExportGPlates.cc`, `src/ScribeExportGPlatesDemoNoGui.cc`, `src/ScribeExportGPlatesUnitTest.cc`, `src/ScribeExportPyGPlates.cc`.

**CMake selection:** hard-wired per target in `src/CMakeLists.txt`:
- `add_executable(gplates gplates_main.cc ScribeExportGPlates.cc)` (line ~357)
- `add_executable(gplates-no-gui … ScribeExportGPlatesDemoNoGui.cc)` (line ~363)
- `add_executable(gplates-unit-test … ScribeExportGPlatesUnitTest.cc)` (line ~372)
- `Python3_add_library(pygplates MODULE ScribeExportPyGPlates.cc)` (line ~395)

**Diffs:** `ScribeExportGPlates.cc`, `…DemoNoGui.cc`, `…UnitTest.cc` are **identical** between branches. Only **`ScribeExportPyGPlates.cc` differs** (CONFLICT): pygplates adds `#include "model/ScribeExportModel.h"` and `#include "property-values/ScribeExportPropertyValues.h"`, and expands the `SCRIBE_EXPORT_PYGPLATES` macro with `SCRIBE_EXPORT_MODEL` and `SCRIBE_EXPORT_PROPERTY_VALUES` (the module serializes model/property-value types for pickling). Take pygplates.

---

## 6. Tests

**pygplates branch:** `pygplates/test/` — a ctest-wired Python suite. `pygplates/test/CMakeLists.txt` registers `add_test(NAME pygplates-test COMMAND ${GPLATES_PYTHON_EXECUTABLE} test.py CONFIGURATIONS Release MinSizeRel)`, sets `ENVIRONMENT "PYTHONPATH=$<TARGET_FILE_DIR:pygplates>"` and `FAIL_REGULAR_EXPRESSION "FAIL|Fail|fail"`. Suite: `test.py` driver + subdirs `test_app_logic/`, `test_maths/`, `test_model/`, plus `fixtures/` (12 `.py` files).

**gplates branch equivalent:** none of this Python suite exists. gplates uses the **C++ Boost unit-test executable** `gplates-unit-test` populated from `src/unit-test/` (identical directory on both branches; driven by `src/unit-test/add_tests.py`). There is a stray `sample-data/unit-test-data/test_feature.py` on gplates but no pygplates-style suite.

**ctest wiring per branch:** `enable_testing()` is called **only in the pygplates (`else()`) branch** of the top-level `CMakeLists.txt` (line ~38). On a gplates build, `ctest`/`RUN_TESTS` is not available; `gplates-unit-test` is a standalone `EXCLUDE_FROM_ALL` executable. Post-merge decision: leave as-is initially (lowest risk); optionally enable testing for both configs later.

---

## 7. Python embedding & `src/api` (the biggest structural area — largely pre-solved)

**Does the gplates app embed Python?** Yes. `gplates-lib` gets `target_compile_definitions(gplates-lib PUBLIC GPLATES_PYTHON_EMBEDDING)` (`src/CMakeLists.txt:958`), links `Python3::Python` (full embedding library, line ~939), and runs `include(TestPythonEmbedding)`. This drives the in-app Python console. By contrast `pygplates` is a `MODULE` that must **not** link the Python library (only `Python3::Module`) because symbols come from the host interpreter (explicit comments at `src/CMakeLists.txt:941-949`).

**`src/api/` — same directory, different contents:**
- **gplates** `src/api/` = ~35 files: the embedded-console/runner layer — `AbstractConsole.h`, `Console{Reader,Writer}.*`, `PythonRunner.*`, `PythonExecution{Monitor,Thread}.*`, `PythonInterpreter{Locker,Unlocker}.*`, `PythonUtils.*`, `PyApplication.cc`, `PyViewportWindow.cc`, `PyColour.cc`, `PyFeature.*`, `PyFeatureCollection.*`, `PyCoregistrationLayerProxy.*`, `PyTopologyTools.cc`, `CoReg.cc`, `Sleeper.*`, plus **two files unique to gplates: `Python.cc` and `PyFunctions.cc`**.
- **pygplates** `src/api/` = ~110 files: the entire boost-python module surface — `APIVersion.*`, `PyGPlatesModule.cc`, `PyRotationModel.*`, `PyReconstruct*.*`, `PyTopologicalModel/Snapshot.*`, `PyResolveTopologies.*`, `PyFiniteRotation.cc`, `PyGeometriesOnSphere.*`, `PyPropertyValues.*`, `PythonPickle.*`, `PyExceptions.*`, `PyOldFeature*.*`, `PyNetRotation.*`, `PyStrain.cc`, `PythonConverterUtils.h`, `PythonExtractUtils.*`, `PythonVariableFunctionArguments.*`, etc. It does **not** contain gplates's `Python.cc` or `PyFunctions.cc`.

**Follow-up verification (main session) that de-fanged this conflict:**
- pygplates **already renamed** gplates' thin wrappers: `PyOldFeature.cc:39` registers `class_<GPlatesApi::OldFeature>("OldFeature", no_init)`, `PyOldFeatureCollection.cc:49` similarly; the console/embedding files reference them.
- `PyGPlatesModule.cc` already contains `#ifdef GPLATES_PYTHON_EMBEDDING` blocks (lines ~149–164) registering `export_console_reader/writer`, `export_instance`, `export_main_window`, `export_style`, `export_coregistration_layer_proxy`, and `#if !defined(GPLATES_PYTHON_EMBEDDING)` blocks for module-only glue (`_post_import`). `export_old_feature()`/`export_old_feature_collection()` are called unconditionally (lines ~198–199). I.e. **the pygplates module init was already written to serve both builds**.
- gplates' `Python.cc` is an 80-line `BOOST_PYTHON_MODULE(pygplates)` init that registers a subset of what `PyGPlatesModule.cc` registers — fully superseded.
- gplates' `PyFunctions.cc` (304 lines) exports old-signature free functions `reconstruct`/`reverse_reconstruct`; contains one old-API call (`->deep_clone()` line 272). The new API exports functions with the same names unconditionally (`export_reconstruct()`, `PyReconstruct.cc:769`), so this file is deleted in the merge (decision confirmed).
- The app's shared colouring/draw-style scripts (`ColorByProperty.py`, `draw_style_demo.py`) reference only `pygplates.Application`, `pygplates.Colour`, `pygplates.PaletteKey` and call methods on objects passed in — unaffected by the `Feature`→`OldFeature` rename.
- `src/api/CMakeLists.txt`: gplates lists 35 files; pygplates lists ~110 (already excluding Python.cc/PyFunctions.cc, already including all embedding files). Since gplates never modified this file since the merge-base, **pygplates' list wins automatically in the merge** — no conditional-sources work needed.
