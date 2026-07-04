# Execution details: merge `pygplates` into `gplates` (empirical conflict list, hunk-level resolutions, risks)

> Design report (Plan agent) for the pygplates→gplates branch unification. See `MERGE-PLAN.md` for the approved plan; this file carries the full hunk-level detail behind it.
>
> Note: decisions confirmed after this report was written — (1) embedded console adopts the full pygplates API (drop `Python.cc`/`PyFunctions.cc`); (2) **exclude** the WIP commit a4e8f1391 (merge from staging point 3adc36990, tag pygplates tip `pygplates-final`); (3) land on `gplates` by **fast-forward** (§2 option a).

## 1. Empirical conflict list (from `git merge-tree --write-tree gplates pygplates`, git 2.43.0.windows.1)

The merge produces result tree `47c873f8d493006bfb690918dd275c085f97e836` with **14 conflicted paths** (12 real resolution units — three paths belong to one spurious rename/rename):

| # | Path | Conflict type | Hunks | Resolution (details in §3) |
|---|------|--------------|-------|-----------------------------|
| 1 | `BUILD.Linux` | content | 1 | Rewrite the "originated from X branch" note → unified wording |
| 2 | `BUILD.OSX` | content | 1 | Same |
| 3 | `BUILD.Windows` | content | 1 | Same |
| 4 | `cmake/modules/ConfigDefault.cmake` | content | 4 | Take **gplates** (all 4 hunks are copyright year 2026 vs 2025) |
| 5 | `cmake/modules/InstallSharedLibraryDependencies.cmake` | add/add | 3 | Union: hunk 1 pygplates, hunks 2–3 gplates (see §3) |
| 6 | `cmake/modules/Version.cmake` | content | 3 | Mixed + **one silent non-conflict fix** (see §3, critical) |
| 7 | `src/CMakeLists.txt` | content | 2 | Take **gplates** both hunks (Boost 1.69 min; drop `system` component) |
| 8 | `src/ScribeExportPyGPlates.cc` | content | 2 | Take **pygplates** both hunks (model + property-values scribe exports) |
| 9 | `src/api/PyFunctions.cc` | modify/delete (deleted in pygplates, modified in gplates) | — | **Delete** (see §4) |
| 10 | `src/presentation/ReconstructionGeometryRenderer.cc` | content | 1 | Take **gplates** side (drop orphaned pygplates member fn) **+ 3-line API rename** (see §3) |
| 11 | `src/qt-resources/python.qrc` | content | 1 | **Union** (gplates' hellinger.py line + pygplates' 7 `python/api/*.py` lines) |
| 12–14 | `src/system-fixes/boost/cstdint.hpp` → `src/app-logic/VelocityUnits.h` (gplates) / `src/api/PyInformationModel.cc` (pygplates) | rename/rename | — | Spurious rename detection. Keep both new files, keep `cstdint.hpp` deleted (see §3) |

**Everything else auto-merges** (~30 files content-merged, incl. `Install.cmake`, `Package.cmake`, GreatCircleArc, OgrReader, topology/app-logic files), and the auto-merge was audited:

- `src/api/Python.cc`: gplates never modified it since base; pygplates deleted it → **auto-deletes**. Correct (superseded by `PyGPlatesModule.cc`).
- `src/api/CMakeLists.txt`: gplates never modified it since base → pygplates' ~110-file list **wins automatically** (it already excludes Python.cc/PyFunctions.cc and includes all embedding files: ConsoleReader/Writer, PyApplication, PyViewportWindow, PyCoregistrationLayerProxy, PyOldFeature*). No conditional-sources work needed.
- Hellinger: merge result has NO top-level `scripts/hellinger*.py`, HAS `src/qt-resources/python/scripts/hellinger/hellinger.py`, and merged `Install.cmake`'s script loop is empty — auto-resolved exactly as intended.
- Old-model-API residue scan of the entire merge result: only two spots — the 3 lines in ReconstructionGeometryRenderer.cc (§3) and `PyFunctions.cc:272` `->deep_clone()` (file being deleted). `DrawStyleAdapters::deep_clone` hits are the GUI style class's own method, present and compiling on pygplates today — not model API.

**Surprises vs the expected list:**
1. **SILENT MISMATCH (not a conflict): `option(GPLATES_BUILD_GPLATES ... false)`** survives at merged `Version.cmake:25`. The merge-base already had `true`, gplates left it unchanged, pygplates changed it to `false` — so git "correctly" takes pygplates' change, which is wrong for the goal. Must be manually flipped to `true` during resolution; nothing will flag it.
2. The rename/rename on `cstdint.hpp` (both branches independently deleted it: gplates in a3108dce7, pygplates in dd2126a6a; git's similarity matcher paired the deletions with unrelated added files). Trivial: `VelocityUnits.h` is byte-identical on both branches; `PyInformationModel.cc` exists only on pygplates (41 new lines).
3. `src/app-logic/VelocityUnits.h` / `src/api/PyInformationModel.cc` appear in the conflict list only as satellites of #2.
4. ReconstructionGeometryRenderer resolution direction is the opposite of naive expectation — see §3.

**Cherry-pick commits:** `a3108dce7` ("Cherry picked 55f378f..4781c6 from pygplates into gplates") is a *huge* content copy (BUILD/DEPS docs, Install/Package/Version/ConfigDefault cmake, InstallSharedLibraryDependencies.cmake creation, ScribeExportPyGPlates.cc, PyFunctions.cc edits, .gitignore…). It is the **cause of most conflicts** above: it duplicated pygplates content into gplates, and where pygplates subsequently evolved (versions, codesign, scribe exports), 3-way merge sees two-sided edits. It also *helps*: because the copies are near-identical, those conflicts collapse to 1–4 small hunks each instead of whole-file conflicts. `c4680ebe5` (Qwt 6.3 fix, 2 lines in `src/qt-widgets/Kinematic*`) causes **no conflict** — pygplates has the original commit, both sides ended identical.

## 2. Git strategy

**Pre-step decision — the WIP commit:** staging branch `feature/pygplates-merge-into-gplates` (3adc36990) is **one commit behind** the `pygplates` tip — a4e8f1391 "WIP: Fix for pygplates.GpmlTopologicalSection.create for a polygon" (touches `src/api/PyFeature.h`, `PyPropertyValues.cc`, +2). **DECISION: exclude** — merge from 3adc36990; `git tag pygplates-final a4e8f1391` preserves the WIP. The conflict list above is identical either way (the WIP doesn't touch any conflicted file).

**Merge steps:**
1. On `feature/pygplates-merge-into-gplates`: `git merge gplates` → resolve the 12 units per §3 (plus the silent `option()` flip and §4 deletions). Commit the merge (call it **M**; parents: pygplates-lineage first, gplates second).
2. Validate both build variants and run tests on staging (§5). Push staging for CI/review.
3. Land on `gplates`. Two options:
   - **(a) Simple (CHOSEN):** `git checkout gplates && git merge feature/pygplates-merge-into-gplates` — this **fast-forwards** gplates to M (gplates is M's ancestor). Consequence: `git log --first-parent gplates` now follows the pygplates lineage. Given pygplates is 1,415 commits of real development containing nearly all of gplates' history, this is acceptable and simplest.
   - (b) If first-parent = gplates lineage mattered: mint a parent-swapped merge without redoing resolution: `git commit-tree M^{tree} -p gplates -p <pygplates-tip> -m "..."` then fast-forward gplates to the new commit. Same tree, different parent order. (Not chosen.)
4. **Retire `pygplates`:** `git tag pygplates-final a4e8f1391`, then delete the local branch and (with team agreement) the remote `private/pygplates`. Same treatment for the staging branch once merged.
5. **Release branches:** `release-gplates` / `release-pygplates` are historical release lines — leave untouched; future releases of *both* products cut from unified `gplates`.
6. **Feature branches in flight:**
   - `feature/pygplates-model-revisions` (17 commits atop c3f42c065, pure pygplates lineage): merges into unified `gplates` normally afterward — its merge-base is fully contained in the new gplates. No action needed now; optionally merge new gplates into it early to surface conflicts.
   - `feature/diligent-migration` (contains gplates tip already; merge-base with pygplates is the old dc735bca4): after unification, merging new `gplates` into it uses merge-base = old gplates tip, so it receives the whole pygplates side + the resolutions as one diff. Conflicts only where diligent-migration itself touched pygplates-refactored files. Do this sync **soon after** unification while the resolution rationale is fresh; enable `git config rerere.enabled true` before step 1 so recorded resolutions replay.

## 3. Per-conflict resolutions (concrete)

| File | Action |
|------|--------|
| `cmake/modules/Version.cmake` | Hunk 1 (header comment): replace both sides with new text: this branch builds both products, `GPLATES_BUILD_GPLATES` defaults to true. Hunk 2: `set(GPLATES_SEMANTIC_VERSION 2.6.0-4)` (gplates). Hunk 3: `set(PYGPLATES_PEP440_VERSION 1.1.0.dev5)` (pygplates). **Then flip line ~25 `option(GPLATES_BUILD_GPLATES ... false)` → `true`** — outside any conflict marker; easy to miss. |
| `src/CMakeLists.txt` | Both hunks gplates: `_GPLATES_MIN_BOOST_VERSION 1.69` (+ its 1.89 comment) and `COMPONENTS program_options thread ${GPLATES_BOOST_PYTHON_COMPONENT_NAME}` (no `system`). Packaging safe: conda `libboost-devel` unpinned, macOS wheels boost 1.76, manylinux 1.84 — all ≥ 1.69. |
| `src/ScribeExportPyGPlates.cc` | Both hunks pygplates: keep `#include "model/ScribeExportModel.h"` / `"property-values/ScribeExportPropertyValues.h"` and the `SCRIBE_EXPORT_MODEL \` / `SCRIBE_EXPORT_PROPERTY_VALUES \` macro lines (needed for pickling; harmless for the gplates app). |
| `src/presentation/ReconstructionGeometryRenderer.cc` | Take **gplates side of the hunk (i.e., delete pygplates' orphaned member-function definition of `get_subduction_polarity`)**. Rationale: gplates commit 81a9ce709 refactored it from a class member into a file-local anonymous-namespace helper called generically from `create_rendered_geometry()` (teeth on sections, not only boundaries), and removed the member declaration from the `.h` — the `.h` auto-merges to the gplates side, so keeping pygplates' member definition would not compile. pygplates' old call site in the shared-sub-segment path auto-merged away (verified: absent from result tree). **Then apply the model-API rename inside the surviving helper (merged lines ~217/222/226):** `->type()` → `->get_type()`, `->value()` → `->get_value()` (2 call sites for value). This is the only real code conflict, and this rename is the entire manual code fix in the merge. |
| `cmake/modules/InstallSharedLibraryDependencies.cmake` | Hunk 1: **pygplates** (`CODE "set(GPLATES_PYTHON_STDLIB_INSTALL_PREFIX [[...]])"` injection — the install code needs the variable). Hunk 2: **gplates** (comment "…and then codesign the dependency"). Hunk 3: **gplates** (the generalized all-frameworks codesign loop; it already contains the note that Python stdlib installs only for the gplates target, so it's variant-aware). After resolving, grep the merged file to confirm every use of `GPLATES_PYTHON_STDLIB_INSTALL_PREFIX` has a matching `CODE "set(...)"` injection in the same `install(CODE)` scope. |
| `cmake/modules/ConfigDefault.cmake` | All 4 hunks gplates (2026 copyright years). |
| `BUILD.Linux/OSX/Windows` | Each has one hunk: the "Note: This source code originated from a "X" branch … should only be used to compile X" paragraph. Replace with unified wording: source builds both GPlates and pyGPlates, selected by `-DGPLATES_BUILD_GPLATES=TRUE/FALSE` (default TRUE). |
| `src/qt-resources/python.qrc` | Union: keep `<file>python/scripts/hellinger/hellinger.py</file>` **and** the 7 `<file>python/api/*.py</file>` lines. All 8 underlying files verified present in the merge result tree. |
| `src/api/PyFunctions.cc` | `git rm` (see §4). |
| rename/rename triple | Keep `src/app-logic/VelocityUnits.h` (identical both branches) and `src/api/PyInformationModel.cc` (pygplates version — the only one that exists); ensure `src/system-fixes/boost/cstdint.hpp` stays deleted. In practice: `git checkout <pygplates-side> -- src/api/PyInformationModel.cc`, `git checkout gplates -- src/app-logic/VelocityUnits.h`, `git rm` cstdint.hpp if staged. |

Suggested resolution order: cmake files first (Version → src/CMakeLists → ConfigDefault → InstallSharedLibraryDependencies) so a configure smoke test runs early, then api deletions, then ReconstructionGeometryRenderer, then docs/qrc.

## 4. src/api resolution

- **Drop `src/api/Python.cc`** — happens automatically (auto-delete). Its `BOOST_PYTHON_MODULE(pygplates)` init is superseded by `PyGPlatesModule.cc`, whose `#ifdef GPLATES_PYTHON_EMBEDDING` block (lines ~149–164) registers `export_console_reader/writer`, `export_instance`, `export_main_window`, `export_style`, `export_coregistration_layer_proxy` — **all six verified defined on the pygplates branch** (`ConsoleReader.cc:109`, `ConsoleWriter.cc:121`, `PyApplication.cc:267`, `PyViewportWindow.cc:229`, `PyColour.cc:162`, `PyCoregistrationLayerProxy.cc:132`) and all present in pygplates' `src/api/CMakeLists.txt` srcs list. `export_colour` is exported unconditionally (line ~222). `export_co_registration` is commented out on both branches — no loss.
- **Delete `src/api/PyFunctions.cc`** (resolve the modify/delete as delete). Reasons: (1) pygplates' api CMakeLists (which wins automatically) doesn't compile it; (2) it contains old-API `->deep_clone()` (line 272) that won't compile against the new model; (3) its exports `reconstruct`/`reverse_reconstruct` would collide with the new-API ones that `PyGPlatesModule.cc` exports **unconditionally** (`export_reconstruct()` line ~208; `bp::def("reverse_reconstruct", ...)` in `PyReconstruct.cc:769`) — the embedded console gets full new-API replacements. Do **not** port the old signatures under different names; accept the change with a release note.
- **Embedded-console API surface changes to document (release note / porting note for console users):**
  1. `pygplates.reconstruct(recon_files, rot_files, time, anchor_plate_id, export_file_name)` (old, file-list based, export name last) → new API `reconstruct(...)` signature (output argument third); positional-arg scripts break. Same for `reverse_reconstruct` (old had `output_file_basename_suffix`/`output_file_format` params that no longer exist).
  2. `pygplates.Feature` / `pygplates.FeatureCollection` in the console are now the **full new-API classes**; the old thin wrappers are still available but renamed `OldFeature` / `OldFeatureCollection` (verified: `PyOldFeature.cc:39`, `PyOldFeatureCollection.cc:49`; both exported unconditionally at `PyGPlatesModule.cc:198–199`). Console gains the entire pygplates API — a large net win (incl. explicit GPML loading from console Python code).
- No conditional-source CMake work is needed: every api file already compiles into both target shapes on the pygplates branch today (embedding files included), with variant differences handled by `GPLATES_PYTHON_EMBEDDING` ifdefs.

## 5. Sequencing & verification plan (start → single develop branch)

1. `git config rerere.enabled true` (project-local). WIP commit a4e8f1391 excluded (decision); stay at 3adc36990.
2. On staging: `git merge gplates` (expect exactly the 14-path conflict set above; any delta means a branch moved — re-run merge-tree).
3. Resolve per §3/§4, including the two non-conflict manual fixes: `option(GPLATES_BUILD_GPLATES ... true)` and the 3-line `get_type()/get_value()` rename.
4. Pre-commit greps on the resolved tree: no conflict markers; no `->deep_clone()` outside `src/gui/DrawStyle*` and the deprecated HOWTO doc; `option(GPLATES_BUILD_GPLATES` says `true`; no `src/api/Python.cc`/`PyFunctions.cc`/`system-fixes/boost/cstdint.hpp`; python.qrc has 8 union entries. Commit merge M.
5. **Build variant A (GPlates app), Windows, `build/` dir:** `cmake -B build -S . -DGPLATES_BUILD_GPLATES=TRUE` then build `gplates` (and `gplates-unit-test`). First-ever compile of the gplates executable targets against the pygplates tree — expect the bulk of any fallout here (though the whole app tree already compiles into the pygplates module today, so linking `gplates.exe` is the new part: `gplates_main.cc`, static lib split).
6. **Build variant B (pyGPlates module):** separate build dir (e.g. `build-pygplates/`), `-DGPLATES_BUILD_GPLATES=FALSE`; build the module; run `ctest` (pygplates test suite, enabled for this config) — this is the regression baseline proving the merge didn't perturb pygplates.
7. GPlates app smoke tests (manual, Windows): launch; load & save a GPML file; load/save a `.gproj` project/session; edit feature properties + **undo/redo** (QUndoCommand path over the bubble-up model — the highest-risk runtime behavior); Hellinger tool opens and runs (script now sourced from the Qt resource); reconstruct/animate; export resolved topologies (exercises 47c6ccaab); small-circle tool (9e5327bf7); subduction teeth render on topological sections AND boundaries (exercises the one real code conflict); Python console: new API `pygplates.reconstruct(...)` runs, `pygplates.OldFeature`/`OldFeatureCollection` exist, main-window/instance/style bindings respond.
8. Unit tests: run `gplates-unit-test` (covers the min/max fix 90a66391a etc.).
9. Packaging checks: configure-only run of CPack targets for variant A; verify conda recipe (`pygplates/conda/`) and wheel scripts reference nothing branch-specific (boost pins already verified OK); on a Mac (later, see risks) exercise the merged `InstallSharedLibraryDependencies.cmake` codesign path for both variants.
10. Docs commit(s) on staging: `README.md`, `BUILD.*` (already unified in resolution — re-read end-to-end), `DEPS.*` if they mention branch choice; remove "which branch to use" language everywhere (`git grep -i 'pygplates branch'`). Update any CLAUDE.md kept outside this tree (cross-branch flow constraint obsolete).
11. CI green on staging → land on `gplates` per §2 step 3 → tag `pygplates-final`, delete `pygplates` + staging branches (local and remote per team agreement) → announce; sync `feature/diligent-migration` promptly; leave `feature/pygplates-model-revisions` to merge normally later.

## 6. Risk register

| Risk | Likelihood/Impact | Mitigation |
|---|---|---|
| Silent wrong default `GPLATES_BUILD_GPLATES=false` (auto-merged, no conflict) | Certain if unaddressed / high (everyone building gplates gets pygplates) | Explicit step; post-merge grep gate in §5.4 |
| gplates.exe link/build failures in first variant-A build (target split never compiled against pygplates tree) | Medium / medium | Whole app tree already compiles in pygplates module target; budget a fix-forward day; keep fixes as follow-up commits on staging before landing |
| Embedded-console API break (reconstruct/reverse_reconstruct signatures; Feature→OldFeature) | Certain / low-medium (few console-script users) | Release note + porting table (§4); consider a console banner mentioning the new API |
| Runtime regressions in app from bubble-up model (undo/redo, edit dialogs) | Low (weak-ref layer byte-identical; QUndoCommand unaffected) / high | Focused smoke tests §5.7; `.gproj` and GPML round-trip both directions (save in merged build, load in released 2.5, and vice versa — GPML I/O unaffected, `transcribe()` additions are additive) |
| macOS codesign merge (`InstallSharedLibraryDependencies.cmake`) untestable on Windows | Certain / medium (notarization failures found late) | Resolution keeps gplates' proven framework-signing (shipped in 2.6.0-4 era) + adds only the stdlib-prefix injection; schedule one macOS package build of each variant before the next release; static-check the `CODE "set(...)"` variable plumbing |
| `feature/diligent-migration` re-conflicts on the same files when syncing | Medium / medium | rerere enabled before the merge; sync immediately after landing |
| Excluded WIP commit a4e8f1391 stranded when pygplates is retired | Certain / low | `pygplates-final` tag preserves it; finish later on the unified branch if wanted |
| Auto-merged files hide semantic (compiles-but-wrong) mixes of the 22 gplates-only commits with pygplates refactors | Low (audited: only 3 old-API lines injected, all in the known conflict file) / medium | §5.7 targets each of the 22 commits' features (teeth, small-circle, topologies export, palette epsilon via colour tests, GDAL/OGR load) |

### Critical files
- `cmake/modules/Version.cmake`
- `src/CMakeLists.txt`
- `src/presentation/ReconstructionGeometryRenderer.cc`
- `cmake/modules/InstallSharedLibraryDependencies.cmake`
- `src/api/PyGPlatesModule.cc` (reference — no edit expected)
