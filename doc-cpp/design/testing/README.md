# Testing GPlates

How GPlates and pyGPlates are tested today, the principles new tests should follow, and the
roadmap for the test tiers that don't exist yet. This is a strategy document, not a tutorial —
the "how to add a test" section is deliberately short because adding a test is deliberately easy.

## Status

What exists (as of the GoogleTest migration, 2026):

- **C++ unit tests** — GoogleTest, in `src/unit-test/`, built as the `gplates-unit-test`
  executable (GPlates build configs only, i.e. `GPLATES_BUILD_GPLATES=TRUE`). Every test case is
  registered individually with CTest via `gtest_discover_tests()`, so `ctest` runs them in
  parallel, `ctest -R <pattern>` targets them, and `ctest --rerun-failed` works at test-case
  granularity. CI runs them on Linux/macOS/Windows via
  `.github/workflows/build-test-gplates.yml` (the `gplates` branch).
- **pyGPlates tests** — Python `unittest` suite in `pygplates/test/`, registered with CTest as
  `pygplates-test` (plus `pygplates-stub-test` for the `.pyi` stub) in pyGPlates build configs
  (`GPLATES_BUILD_GPLATES=FALSE`). CI runs them via `.github/workflows/build-test-pygplates.yml`
  (the `pygplates` branch). These cover the pyGPlates API surface (app-logic, maths, model) and
  are documented separately.

The two suites are complementary, not redundant: the pyGPlates suite exercises the
reconstruction/model machinery through the public Python API; the C++ suite exercises what the
Python API cannot reach (eg, the Scribe transcription system, GUI-layer utilities like
mipmapping, low-level maths and utils).

## Principles

1. **CTest is the single top-level runner.** Every kind of test — C++ unit test, Python test,
   and the future end-to-end and rendering tests — is registered with CTest. One command
   (`ctest`) runs everything a build tree supports; labels (below) select subsets. Do not
   introduce a second top-level runner (eg, pytest wrapping ctest): CTest owns the build-tree
   environment, working directories, timeouts and parallelism, and other runners nest naturally
   *under* it (as `pygplates-test` already demonstrates).
2. **GoogleTest is the framework for all new C++ tests.** Chosen over Boost.Test (the previous
   framework) for CMake-native per-case discovery (`gtest_discover_tests()` ships in CMake
   itself), better failure output, and familiarity — both for contributors (GDAL, PROJ, FreeCAD
   and LLVM all use it) and for AI coding agents, which write a `TEST()` correctly far more
   reliably than Boost.Test's idioms. Don't mix frameworks within an executable.
3. **Tests must run headless.** The test main() defaults `QT_QPA_PLATFORM` to `offscreen`, and
   provides a `QApplication` — so QSettings-dependent code (`ApplicationState`,
   `UserPreferences`) and widget-level code are testable, but nothing may require a real display,
   a GPU, or user interaction. (Rendering tests will eventually need more than the offscreen
   platform provides — see the roadmap.)
4. **Tests must be working-directory independent and must not write into the source tree.**
   Test *input* data lives in `src/unit-test/data/` and is located via the
   `GPLATES_UNIT_TEST_DATA_DIR` compile definition (an absolute path baked in by CMake), so the
   test executable runs correctly from anywhere. Any *output* a test produces goes to a
   `QTemporaryDir`. A test run must leave `git status` clean.
5. **Test runs are Release (or MinSizeRel) only.** `GPlatesGlobal::Assert` calls `std::abort()`
   in Debug builds (`GPLATES_DEBUG`) rather than throwing, which kills any test that exercises an
   error path. The pyGPlates tests are registered with `CONFIGURATIONS Release MinSizeRel`; the
   C++ tests are not discovered in Debug build trees. Always run `ctest -C Release`.
6. **Failures must be diagnosable from CI logs alone.** Failure detail goes to stdout/stderr
   (never only to a log file), `--output-on-failure` is CTest's default here (via
   `CMAKE_CTEST_ARGUMENTS`), and the CI workflows upload `Testing/Temporary/` as an artifact on
   failure. Use `SCOPED_TRACE` to disambiguate checks that run in loops or shared helpers.

## Adding a C++ test

Create (or extend) a file in `src/unit-test/` using `TEST()`/`TEST_F()`, add it to the `srcs`
list in `src/unit-test/CMakeLists.txt`, rebuild `gplates-unit-test` — discovery is automatic.
Test input data lives in `src/unit-test/data/` and is read via the
`GPLATES_UNIT_TEST_DATA_DIR` compile definition (eg,
`GPLATES_UNIT_TEST_DATA_DIR "/my_input.gpml"`). GoogleTest itself comes from the conda environment (the
`gtest` package in the root `env.*.yml` files); it is optional in CMake — without it the test
target simply doesn't exist — but CI builds the target explicitly, so it is mandatory in effect.

A note on scope: the historical unit-test suite went stale largely because no
`QCoreApplication` existed, making everything app-logic-adjacent untestable. That constraint is
gone (see `src/gplates_unit_test_main.cc`), so there is no longer a structural excuse for the
thinnest areas of coverage — file-io above all, then app-logic.

## Roadmap

In order of value per effort. Each tier should get CTest **labels** (`unit`, `python`, `app`,
`render`, `slow`) as it lands, so PR CI can run `-LE 'render|slow'` while nightly/manual runs
take everything.

1. **End-to-end file-in/file-out data tests** (the GDAL-autotest pattern, and the single
   highest-value future investment). A corpus of small input files (`.rot` rotation models plus
   feature collections); reconstruct at fixed times; export; compare exported *data* numerically
   against stored expected output with a tolerance. Deterministic across platforms, no GPU, no
   pixels, exact failure locations. Drive it two ways against the same corpus: through pyGPlates
   (extending `pygplates/test/`), and through a **headless script mode of the `gplates`
   executable** (to be added — the ParaView `pvbatch` pattern: run a script, exit with a
   meaningful code, registered with CTest). The second path is what turns "the app builds" into
   "the app works".
2. **QTest widget tests** for the highest-churn dialogs — in-process, offscreen, no OpenGL.
   Widget-level QTest (simulated Qt events, `QSignalSpy`) is the industry norm for Qt GUI
   testing; external GUI-driving robots are not (see "rejected" below).
3. **A small rendering smoke suite** (10–20 tests), Linux-only, software-rendered with a
   *pinned* Mesa (llvmpipe) version — software renderer output shifts between Mesa releases and
   silently invalidates baselines. Compare rendered output against baseline images with a
   per-test mismatch tolerance (QGIS's approach — per-pixel masks and/or multiple acceptable
   baselines — is the reference design); disable anti-aliasing under test; fixed framebuffer
   size; upload rendered+diff images as CI artifacts on failure. Pair it with a
   manually-triggered workflow that regenerates baselines and commits them to a PR branch
   (FreeCAD's pattern) so baseline updates are never a single-maintainer bottleneck. Never gate
   merges on macOS rendering — GitHub's macOS runners have documented software-GL defects.

## Rejected (deliberately)

- **CDash** — long-term dashboards are for nightly multi-machine fleets with operational
  staffing. CI logs + JUnit output + failure artifacts give the day-to-day value at ~zero cost.
- **Squish** (or other commercial GUI-automation tools) — per-seat licensing means contributors
  and CI can't run the suite freely; that alone disqualifies it.
- **Accessibility-driven external automation** (dogtail/pywinauto/FlaUI) — three
  platform-specific implementations, fragile against widget changes, and blind to the one thing
  we'd most want to verify (the GL globe/map canvas).
- **pytest as the outer runner** — inverts the CTest-owns-the-tree principle (see Principles).
  GDAL, ParaView and Slicer all land on CTest-outer.
- **Wholesale in-app record/replay GUI testing** (Kitware QtTesting) — proven in ParaView, but
  it's an in-app integration project and recorded scripts are brittle against layout change.
  Reconsider only after roadmap tiers 1–3 are paying for themselves.

## CI notes

- The two workflows are branch-scoped: `build-test-gplates.yml` on `gplates`,
  `build-test-pygplates.yml` on `pygplates`. The branches are normally at the same HEAD, so the
  per-branch split covers both products without doubling CI cost.
- sccache cache keys are namespaced per product (`sccache-gplates-*` vs the pygplates workflow's
  `sccache-<platform>-*`): `gplates` is the repository's *default* branch, so its caches are
  visible to every ref, and the two build configurations share no cache entries (every common
  translation unit differs in `-fPIC` and the `GPLATES_PYTHON_EMBEDDING` define). All caches
  share the repository's 10 GB Actions budget.
- A possible future change: pyGPlates may stop linking Qt Widgets (it is a non-graphical Python
  module). Nothing in the C++ test design depends on pyGPlates linking Qt GUI libraries — the
  `gplates-unit-test` executable exists only in GPlates build configs.
