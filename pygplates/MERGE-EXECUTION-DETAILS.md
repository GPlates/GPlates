# Execution details: merge `pygplates` into `gplates` (empirical conflict list, hunk-level resolutions, risks)

> Design report (Plan agent) for the pygplates→gplates branch unification. See `MERGE-PLAN.md` for the approved plan; this file carries the full hunk-level detail behind it.
>
> Note: decisions confirmed after this report was written — (1) embedded console adopts the full pygplates API (drop `Python.cc`/`PyFunctions.cc`); (2) **exclude** the WIP commit a4e8f1391 (merge from staging point 3adc36990); (3) ~~land on `gplates` by **fast-forward** (§2 option a)~~.
>
> **Superseded 2026-07-07:** `gplates` and `pygplates` remain **separate** develop branches (not
> unified into one). The excluded WIP commit now lives on branch `pygplates-wip` (not a
> `pygplates-final` tag); `pygplates` itself was reset back to 3adc36990 (its pre-divergence tip);
> the two-parent merge commit was flattened into a single-parent commit; and landing is via a PR
> from the staging branch into `pygplates` (approved/merged on the `public` GitHub remote) instead
> of a fast-forward of `gplates`. See §2 step 3 and §5 step 11 (both updated below).

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

**Pre-step decision — the WIP commit:** staging branch `feature/pygplates-merge-into-gplates` (3adc36990) is **one commit behind** the `pygplates` tip — a4e8f1391 "WIP: Fix for pygplates.GpmlTopologicalSection.create for a polygon" (touches `src/api/PyFeature.h`, `PyPropertyValues.cc`, +2). **DECISION: exclude** — merge from 3adc36990; the WIP is preserved on branch `pygplates-wip` (a4e8f1391) rather than a tag, and `pygplates` itself was reset back to 3adc36990 so it no longer contains the WIP commit. The conflict list above is identical either way (the WIP doesn't touch any conflicted file).

**Merge steps:**
1. On `feature/pygplates-merge-into-gplates`: `git merge gplates` → resolve the 12 units per §3 (plus the silent `option()` flip and §4 deletions). Commit the merge (call it **M**; parents: pygplates-lineage first, gplates second).
2. Validate both build variants and run tests on staging (§5). Push staging for CI/review.
3. **Land on `pygplates` (superseded 2026-07-07; supersedes options a/b below):** `gplates` and
   `pygplates` are kept as separate long-lived develop branches — the goal is reconciling the
   model/build divergence, not merging the branches into one. Flatten merge commit **M** into a
   regular single-parent commit on the pygplates lineage: `git commit-tree M^{tree} -p 69a58bbc0
   -m "..."` (69a58bbc0 = the pygplates-side parent of M), producing a new commit with the exact
   same tree as M but no second parent pointing at `gplates`. Then `git rebase --onto <new-commit>
   M feature/pygplates-merge-into-gplates` to replay the follow-on fix commits on top of it
   (trivial — identical tree, so no conflicts). Finally, open a PR from
   `feature/pygplates-merge-into-gplates` into `pygplates` on the `public` remote
   (github.com/GPlates/GPlates), for review/approval and merge in GitHub. This keeps
   `pygplates`'s history free of a merge commit pointing back at `gplates`, so a later `git
   checkout gplates && git merge pygplates` (whenever convenient) needs no cherry-picking, and
   either branch can still build either product via `GPLATES_BUILD_GPLATES`.
   - ~~(a) Simple: `git checkout gplates && git merge feature/pygplates-merge-into-gplates` —
     fast-forwards gplates to M.~~ Not chosen (would retire `pygplates` as a separate branch).
   - ~~(b) Parent-swapped merge commit onto `gplates`.~~ Not chosen, same reason.
4. **`pygplates` branch handling (superseded 2026-07-07):** rather than retiring `pygplates` via
   `git tag pygplates-final a4e8f1391` + branch deletion, its excluded WIP tip (a4e8f1391) was
   moved to branch `pygplates-wip`, and `pygplates` itself was reset back to 3adc36990 (its
   pre-divergence commit) so the PR in step 3 merges into it with a clean, linear history.
5. **Release branches:** `release-gplates` / `release-pygplates` are historical release lines — leave untouched; future releases of *both* products continue to be cut from their respective develop branches (`gplates` / `pygplates`).
6. **Feature branches in flight (updated 2026-07-07 for the two-branches-kept strategy):**
   - `feature/pygplates-model-revisions` (17 commits atop c3f42c065, pure pygplates lineage): merges into `pygplates` normally once this PR lands — its merge-base is fully contained in the updated pygplates. No action needed now; optionally merge updated pygplates into it early to surface conflicts.
   - `feature/diligent-migration` (contains gplates tip already; merge-base with pygplates is the old dc735bca4): sync happens in two hops now — first `pygplates` receives this PR, then `gplates` merges `pygplates` (whenever convenient), and only then does `feature/diligent-migration` merge the updated `gplates`. That merge-base is still the old gplates tip, so it receives the whole pygplates-side reconciliation + resolutions as one diff; conflicts only where diligent-migration itself touched pygplates-refactored files. Its `CLAUDE.md` describes the old cherry-pick-only cross-branch constraint, which relaxes (both branches can build both products) but doesn't disappear (branches are still separate) — update it as part of this sync, not before. Do the sync **soon after** `gplates` picks up `pygplates` while the resolution rationale is fresh; enable `git config rerere.enabled true` before step 1 so recorded resolutions replay.

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

## 5. Sequencing & verification plan (start → PR into `pygplates`, both develop branches kept)

1. `git config rerere.enabled true` (project-local). WIP commit a4e8f1391 excluded (decision); stay at 3adc36990.
2. On staging: `git merge gplates` (expect exactly the 14-path conflict set above; any delta means a branch moved — re-run merge-tree).
3. Resolve per §3/§4, including the two non-conflict manual fixes: `option(GPLATES_BUILD_GPLATES ... true)` and the 3-line `get_type()/get_value()` rename.
4. Pre-commit greps on the resolved tree: no conflict markers; no `->deep_clone()` outside `src/gui/DrawStyle*` and the deprecated HOWTO doc; `option(GPLATES_BUILD_GPLATES` says `true`; no `src/api/Python.cc`/`PyFunctions.cc`/`system-fixes/boost/cstdint.hpp`; python.qrc has 8 union entries. Commit merge M.
5. **Build variant A (GPlates app), Windows, `build/` dir:** `cmake -B build -S . -DGPLATES_BUILD_GPLATES=TRUE` then build `gplates` (and `gplates-unit-test`). First-ever compile of the gplates executable targets against the pygplates tree — expect the bulk of any fallout here (though the whole app tree already compiles into the pygplates module today, so linking `gplates.exe` is the new part: `gplates_main.cc`, static lib split).
6. **Build variant B (pyGPlates module):** separate build dir (e.g. `build-pygplates/`), `-DGPLATES_BUILD_GPLATES=FALSE`; build the module; run `ctest` (pygplates test suite, enabled for this config) — this is the regression baseline proving the merge didn't perturb pygplates.
7. GPlates app smoke tests (manual, Windows): launch; load & save a GPML file; load/save a `.gproj` project/session; edit feature properties + **undo/redo** (QUndoCommand path over the bubble-up model — the highest-risk runtime behavior); Hellinger tool opens and runs (script now sourced from the Qt resource); reconstruct/animate; export resolved topologies (exercises 47c6ccaab); small-circle tool (9e5327bf7); subduction teeth render on topological sections AND boundaries (exercises the one real code conflict); Python console: new API `pygplates.reconstruct(...)` runs, `pygplates.OldFeature`/`OldFeatureCollection` exist, main-window/instance/style bindings respond.
8. Unit tests: run `gplates-unit-test` (covers the min/max fix 90a66391a etc.).
9. Packaging checks: configure-only run of CPack targets for variant A; verify conda recipe (`pygplates/conda/`) and wheel scripts reference nothing branch-specific (boost pins already verified OK); on a Mac (later, see risks) exercise the merged `InstallSharedLibraryDependencies.cmake` codesign path for both variants.
10. Docs commit(s) on staging: `README.md` (removed the obsolete note against compiling GPlates
    from a pyGPlates branch or vice versa — done), `BUILD.*`/`DEPS.*` (already unified wording from
    the Step 1 resolution — verified, no changes needed); `git grep -i 'pygplates branch'` (clean
    outside historical MERGE-*.md docs). CLAUDE.md files outside this tree describing the old
    cherry-pick-only cross-branch constraint are updated later, during the `feature/diligent-migration`
    sync (step 11), since the constraint relaxes rather than disappears (`gplates`/`pygplates` are
    still separate branches).
11. CI green on staging → flatten merge commit M into a single-parent commit and rebase the
    follow-on fixes onto it (§2 step 3, updated) → open a PR from
    `feature/pygplates-merge-into-gplates` into `pygplates` on the `public` remote → once
    approved/merged, `pygplates` carries the reconciliation (WIP preserved on `pygplates-wip`,
    `pygplates` itself reset to 3adc36990 beforehand — §2 step 4) → `gplates` merges `pygplates`
    whenever convenient (plain merge, no cherry-picking needed going forward) → sync
    `feature/diligent-migration` promptly once `gplates` has picked this up; leave
    `feature/pygplates-model-revisions` to merge into `pygplates` normally, any time after the PR lands.

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

## 7. Post-merge regression found in Step 2 smoke testing: dead `*iter = clone` model commits

**Symptom (reported):** moving a vertex of a file-loaded feature, then undoing, did not flag unsaved
changes (no red row in Manage Feature Collections). Root cause is broader: the edit never reached the
model at all (the moved overlay came from `GeometryBuilder`, not the model).

**Root cause:** the old gplates model's `HandleTraits<FeatureHandle>::iterator_value_type` was the
`TopLevelPropertyRef` proxy, whose `operator=` committed via `FeatureHandle::set()` (notify → unsaved
changes; instance-id equality skip — this is what `PropertyValue::update_instance_id()` fed). The
pygplates model removed the proxy: dereferencing `FeatureHandle::iterator` returns a **temporary**
`non_null_ptr_type`, so `*iter = new_property` compiles but is a silent no-op. The pygplates branch
knew this for its own code (see the disabled-`__setitem__` comment in `src/api/PyFeatureCollection.cc`
referencing the deferred `pygplates-model-revisions` branch) but the desktop-app call sites were never
audited because the app never ran on that branch. This is risk "auto-merged files hide
compiles-but-wrong mixes" materialising — via the model-API semantics change, not the 22 gplates-only
commits.

**Fixed call sites** (replaced with `FeatureHandle::set()` — e.g. `iter.handle_weak_ref()->set(iter, x)`):
- `src/view-operations/FocusedFeatureGeometryManipulator.cc` (2 sites — vertex move/insert/delete/split commit path for focused features)
- `src/qt-widgets/EditWidgetGroupBox.cc` `commit_property_to_model()` (all edit-property dialogs; commits a clone since the widget retains its working copy — new `set()` stores the pointer directly, unlike old `set()` which deep-cloned internally)
- `src/gui/FeaturePropertyTableModel.cc` `setData()` (dormant — table currently non-editable; commit moved inside conversion-success branch), plus deleted the dead dry-run assignment in `refresh_data()` (a read path — must NOT commit, because new `set()` has no equality skip and would dirty files on mere viewing)
- `src/qt-widgets/MetadataDialog.cc` `save_fc_meta()` / `save_mprs_meta()` (added `is_still_valid()` guards matching the old proxy's silent-skip)
- `src/file-io/GpmlUpgradeReaderUtils.cc` crustal-thinning-factor upgrade (**also affects pygplates**: loading pre-GPlates-1.6.338 files silently kept unconverted values)
- `src/qt-widgets/CreateFeatureDialog.cc` `reverse_reconstruct_geometry_property()` (also removed a stray duplicated `clone()` statement that predates the merge on both branches)
- `src/qt-widgets/TotalReconstructionSequencesDialog.cc` `update_current_sequence()` (3 sites — TRS
  dialog → Edit sequence → Apply) and `set_seq_disabled()` (1 site — enable/disable a sequence).
  **Found later**, while investigating issue #38 (see §9): these are spelled `**opt_iter = x` /
  `(**opt_iter) = x`, dereferencing a `boost::optional<FeatureHandle::iterator>` returned **by value**
  from `TRSUtils::TRSFinder`, which the original audit's `*iter = ` grep did not match. Same bug class,
  same fix (`feature_ref->set(iter, x)` with `is_still_valid()` guards). Independent of §9's fix:
  `TRSFinder` is read-only (it only records iterators) and the mutation happens *after* the visit.

**Not fixed / known residual:**
- `BubbleUpRevisionHandler::commit()` has `// TODO: Emit model events.` — **in-place** property-value
  modifications (bubble-up path, e.g. embedded-console scripts calling `pv.set_value(...)` on a
  model-attached property value) do not notify handle listeners, so no view refresh / unsaved-changes
  flag. Same gap existed for in-place edits on the old branch (old GUI always used clone + commit).
  Console `feature.add/remove/set` paths DO notify. Full fix is the deferred
  `feature/pygplates-model-revisions` work.
  **Update:** this gap turned out to be live for the desktop app too, via the non-const `FeatureVisitor`
  mutation path — see §9 for the symptom (issue #38) and the interim fix. The TODO itself remains
  unimplementable until `FeatureHandle` is a `RevisionContext`.
- New `FeatureHandle::set()` lacks the old instance-id equality skip, so a commit of an unchanged
  clone now still dirties the file (rare; e.g. a no-op geometry-builder update).
- Disabled (`#if 0`) code still containing the dead pattern, left as-is: `SplitFeatureUndoCommand.cc`,
  `PyFeature.cc`, `PyFeatureCollection.cc`, `FeaturePropertyTableModel.cc` `assign_new_property_value`.

**Verified-clean commit paths:** `PartitionFeatureUtils.cc` and active `SplitFeatureUndoCommand.cc`
code use `feature->add()`; `ModelUtils.cc` already uses `feature->set()`; `RevisionedVector` iterators
have a working assignment proxy (`OgrUtils.cc` etc. are fine).

## 8. Property/feature cloning semantics: `gplates` branch vs pygplates-merged branch

Raised by the user as a hypothesis (referencing `PropertyValue::update_instance_id()` and the
comment on `feature_handle_clone()` in `src/api/PyFeature.cc`) that needed verifying before treating
the merge as otherwise stable. Confirmed: real, and one instance is currently live/reachable.

**Old `gplates` branch semantics** (`git show gplates:<path>`):
- `PropertyValue`: only `deep_clone_as_prop_val()` (always recursive). Equality is instance-id based:
  `operator==` returns `d_instance_id == other.d_instance_id && directly_modifiable_fields_equal(...)`
  — "is this an unmodified clone of that", not structural equality. `update_instance_id()` breaks the
  link when a subclass mutates a value in place.
- `TopLevelProperty`: has *both* `clone()` (shallow — shares the contained `PropertyValue` instances;
  header warns "probably not what you want... until bubble-up is fully operational") and `deep_clone()`
  (recursively deep-clones contained property values).
- `FeatureHandle::clone()`: shallow (`FeatureRevision::clone()` shallow-copies the `d_children` pointer
  vector). Justified in the header: *"property objects in the model are immutable; if a property were
  to be changed... the clone would point to the old property, while this feature would point to the
  new... Hence, there is no need for a deep clone method."*
- `FeatureHandle::set()`: checks `new_child_equals_existing()` (the instance-id `operator==` above)
  and, only if different, stores `new_child->deep_clone()` — **always installs an independent,
  freshly-cloned object**, never the caller's own pointer.
- Net effect: the *only* way a property could change was via `set()`/`add()`/`remove()`, and `set()`
  always deep-cloned before storing. There was no in-place-mutation path yet ("bubble-up... not yet
  fully operational" — the old branch's own words). That is precisely what made the shallow
  `FeatureHandle::clone()` provably safe.

**Current (merged) branch semantics:**
- `PropertyValue`/`TopLevelProperty`: `clone()` (via `Revisionable::clone()` → `clone_impl()`) is now
  itself a genuine **deep**, recursive clone (`TopLevelPropertyInline`'s deep-clone constructor calls
  `clone_impl()` on every contained `PropertyValue`). `deep_clone()` no longer exists — `clone()`
  absorbed its behaviour. Instance-id equality is gone, replaced by a real structural
  `Revisionable::operator==()`/`equality()`.
- `FeatureHandle::clone()`: **still shallow**, unmodified from the old branch — `FeatureHandle` was
  never migrated onto `Revisionable`/bubble-up (it still uses `BasicRevision`/`FeatureRevision`). Same
  doc comment, same shallow pointer-vector copy.
- `FeatureHandle::set()`/`add()`/`remove()`: store the incoming pointer **directly** — no clone, no
  equality check. This is the safety net the old branch had and the merged branch lost.
- **Bubble-up** (`BubbleUpRevisionHandler`, used by every in-place `PropertyValue` setter, e.g.
  `GpmlPlateId::set_value()`): mutates the *same* `TopLevelProperty`/`PropertyValue` C++ object's
  `d_current_revision` pointer in place — object identity preserved, no new object allocated — and
  does not notify any owning `FeatureHandle` (same gap as §7's residual item). This in-place path is
  new capability the old branch never had.
- `src/api/PyFeature.cc`/`PyFeatureCollection.cc` already route around this: both explicitly avoid
  `FeatureHandle::clone()`/`FeatureCollectionHandle::clone()`, instead manually looping over properties
  and calling the (now genuinely deep) `TopLevelProperty::clone()` on each, with the comment: *"We
  don't use FeatureHandle::clone() because it currently does a shallow copy instead of a deep copy...
  Once FeatureHandle has been updated to use the same revisioning system as TopLevelProperty and
  PropertyValue then just delegate directly to FeatureHandle::clone()."* — the pygplates authors knew
  about this gap and deliberately worked around it for the Python API, but never touched the desktop
  GUI code that has the same problem (matching §7's pattern: pygplates fixed its own call sites,
  gplates-only call sites were never audited).

**Two distinct risk classes:**
- **Risk A — `set()`/`add()` no longer clone/guard:** dormant today. An audit of every
  `FeatureHandle::set()`/`add()` call site in `src/` (~100+, including all 8 sites fixed in §7) found
  every one currently passes a freshly-created or freshly-`.clone()`'d property — so no active bug from
  this angle, but it is no longer *structurally* enforced (previously enforced once, centrally, inside
  `set()`; now depends on every call site remembering to clone).
- **Risk B — bubble-up in-place mutation + shallow `FeatureHandle::clone()` (confirmed live):**
  `src/view-operations/CloneOperation.cc:111`, `clone_focused_feature()` — the GUI "Clone Feature"
  action — calls the shallow `feature_ref->clone()` directly. This file predates the merge and was
  never given the `PyFeature.cc`-style deep-clone-loop workaround. After cloning, the original feature
  and its GUI-created sibling share every `TopLevelProperty`/`PropertyValue` object. Any subsequent
  in-place mutation of a shared property value that bypasses `FeatureHandle::set()` (e.g. the embedded
  pygplates console calling a property value's own setter directly on a value fetched by reference)
  would silently mutate **both** features — with no model notification and no unsaved-changes flag on
  either, since bubble-up doesn't touch `FeatureHandle` at all.

**Options considered:**
1. *Recommended minimal fix — applied:* rewrite `CloneOperation::clone_focused_feature()` to build the
   cloned feature via a per-property deep-clone loop mirroring `PyFeature.cc`'s `feature_handle_clone()`.
2. *Optional hardening — not applied:* reinstate a deep-clone-before-store inside
   `FeatureHandle::set()`/`add()` (no equality check needed — `clone()` is genuinely deep now, so this
   is just the old safety net, not the old optimisation). Would close Risk A structurally, but does not
   touch Risk B since bubble-up bypasses `set()` entirely. Skipped: the user judged this unnecessary
   because the full fix (option 3) will land soon enough, and every current `set()`/`add()` call site is
   already disciplined (see Risk A above).
3. *Full fix (deferred, matches the plan's existing expectation):* migrate `FeatureHandle`/
   `FeatureCollectionHandle` onto `Revisionable`/`BubbleUpRevisionHandler`, per the not-yet-completed
   `feature/pygplates-model-revisions` branch — the merge plan already anticipated this work would be
   needed at the Feature/FeatureCollection level; this finding confirms why. Out of scope here.

**Fix applied:** `src/view-operations/CloneOperation.cc`, `clone_focused_feature()` — replaced the single
shallow `feature_ref->clone()` call with a `FeatureHandle::create()` + per-property
`new_feature_ptr->add(feature_property->clone())` loop, deep-cloning each property (now that
`TopLevelProperty::clone()` is itself deep) instead of sharing property objects with the original
feature. This closes the one confirmed live instance of Risk B. The new feature still gets a fresh
`FeatureId` (via `FeatureHandle::create()`'s default argument), matching the semantics
`FeatureHandle::clone()` used to provide.

This is deliberately a **temporary, un-refactored fix** — the loop is duplicated inline in
`CloneOperation.cc` rather than factored into a shared `GPlatesModel::ModelUtils` helper, with a `FIXME`
comment (mirroring the one on `GPlatesApi::feature_handle_clone()` in `src/api/PyFeature.cc`) pointing at
the real fix: once `FeatureHandle` is migrated onto `Revisionable`/bubble-up (`feature/pygplates-model-revisions`),
both this call site and `PyFeature.cc`'s can go back to calling `FeatureHandle::clone()` directly, and the
workaround can be deleted. No shared helper was introduced since it would just be more code to delete later.

**Conclusion:** Risk B's one confirmed live call site is now fixed. Risk A remains dormant and
un-hardened (option 2 skipped, per above) — every current `set()`/`add()` call site is disciplined today,
and the full model migration (option 3) is expected to close this properly rather than layering a second
temporary workaround on top of `set()`/`add()`.

## 9. Post-merge regression (GPlates issue #38): non-const `FeatureVisitor` edits never reach the feature

**Symptom (reported, GPlates 2.6.0-dev):** with the **Modify Reconstruction Pole** tool the plate follows
the mouse while dragging, but **Apply** has no visible effect — the plate snaps back to its original
position, and "Save All Changes" writes a byte-identical `.rot` file.

**Root cause:** two consequences of the merge combine.

1. *The write-back disappeared.* Pre-merge, `src/model/FeatureVisitor.cc` specialised the non-const
   visitor to *clone → visit → commit*:
   ```cpp
   TopLevelProperty::non_null_ptr_type prop_clone = (*iter)->deep_clone();
   prop_clone->accept_visitor(*this);
   *iter = prop_clone;    // TopLevelPropertyRef proxy -> FeatureHandle::set() -> notifies the model
   ```
   The merge deleted that file (its `FeatureVisitorThatGuaranteesNotToModify` opt-out no longer exists
   either), leaving only the generic template in `src/model/FeatureVisitor.h`:
   `(*feature_iterator)->accept_visitor(*this);` — which mutates the live property in place.
2. *Bubble-up does not reach the feature.* `GpmlFiniteRotation::set_finite_rotation()` commits via
   `BubbleUpRevisionHandler`, and the chain terminates at `TopLevelPropertyInline` because
   `FeatureHandle`/`BasicHandle` are **not** `RevisionContext`s — i.e. §7's residual `// TODO: Emit model
   events.` (`src/model/BubbleUpRevisionHandler.cc`). That TODO cannot be implemented today: `d_model` is
   always `boost::none` for an in-feature property value, so the handler has no feature to notify.

So the pole data **is** updated in memory, but `BasicHandle::notify_listeners_of_modification()` never
runs — and that one function does both jobs:

- `set_unsaved_changes()` on the `FeatureCollectionHandle`. Without it, "Save All Changes" **skips the
  file entirely** (`src/gui/FileIOFeedback.cc`), hence the unchanged `.rot`. (The per-row Save button
  passes `only_unsaved_changes = false` and *would* write the new poles, since
  `PlatesRotationFormatWriter` reads live property values — a useful confirmation that the model data
  itself was correct all along.)
- fires `publisher_modified` → `ApplicationState::reconstruct()` and `ReconstructGraph::modified_input_file()`
  → `ReconstructionLayerProxy::invalidate()`, the only thing that discards the cached reconstruction
  trees. Without it the view redraws from stale trees while
  `ModifyReconstructionPoleWidget::reset_adjustment()` clears the drag orientation — so the plate
  visibly snaps back.

`AdjustmentApplicator::apply_adjustment()` (`src/qt-widgets/ApplyReconstructionPoleAdjustmentDialog.cc`)
already wraps the visit in a `NotificationGuard` and releases it *expecting* a reconstruction; there was
simply nothing pending to flush.

**Interim fix applied** (two hunks, both marked `INTERIM (GPlates issue #38)`):
- `src/model/TopLevelProperty.h` — new `accept_visitor_and_detect_modification(FeatureVisitor &)`
  returning whether the visit created a new revision. The comparison lives inside `TopLevelProperty` so
  `Revision` stays out of the public API (`Revisionable::get_current_revision()` is protected). Note the
  `GPlatesModel::Revision` qualification — unqualified `Revision` would find the nested
  `TopLevelProperty::Revision`.
- `src/model/FeatureVisitor.h` — an `inline template<>` specialisation of
  `FeatureVisitorBase<FeatureHandle>::visit_feature_property()` that, **only when the visit actually
  created a new revision**, re-sets the property via `feature_ref->set(iter, property)` to notify model
  listeners. A top-level property acquires a new revision iff something nested in it was modified, since
  it is currently the root of the bubble-up chain — so this is an exact change detector, not a heuristic.

Deliberately *not* a restoration of the pre-merge behaviour: there is **no deep clone** of every visited
property, so the read-only visitors on the hot reconstruction path pay only a pointer copy and compare.
`set()` with the same pointer is safe — `BasicRevision::set` is a copy-and-swap on the intrusive pointer
(no reallocation, no size change, no revision swap), and `RevisionAwareIterator` holds only
`{weak_ref, index}` and re-reads the revision on each dereference, so the enclosing
`visit_feature_properties()` loop iterator stays valid.

**Blast radius audited.** Of the 23 classes deriving from non-const `GPlatesModel::FeatureVisitor`,
exactly three mutate property values:

| Visitor | Verdict |
|---|---|
| `TotalReconstructionSequenceRotationInserter` | the bug — *wants* the notification, and already runs under a `NotificationGuard` |
| `MakeFilePathsAbsoluteVisitor` (`src/file-io/GpmlReader.cc`) | runs on a **detached** collection during load (no model, no observers); `FeatureCollectionFileIO` clears the flag after a clean load — no regression |
| `GeometryRotator` (`src/feature-visitors/GeometryRotator.h`) | **zero callers** — dead code |

The other 20 (all the `ReconstructMethod*`, `TopologyGeometryResolver`, `TopologyNetworkResolver`,
`GeometryCookieCutter`, `TopologyInternalUtils`, `*GeometryPopulator`, `TRSUtils`, `PartitionFeatureUtils`,
`EditWidgetChooser`, `PaleomagUtils`, `api/PyPropertyValueVisitor`) are non-const only to obtain non-const
references; none calls a setter or a `RevisionedVector` mutator, so none can trip the detector
(`PartitionFeatureUtils.cc` even carries a TODO saying exactly this). Nothing overrides
`visit_feature_property`, so the specialisation cannot be bypassed. pyGPlates is unaffected:
`FeatureVisitorWrap` exposes only property-value visiting and never goes through `visit_feature_property`.

**Removal criterion:** delete **both** hunks once `FeatureHandle` is a `RevisionContext` — Stage 4 of
`feature/pygplates-model-revisions` (`src/model/FeatureBase.h`) — and the model emits these events itself
via the now-implementable `BubbleUpRevisionHandler::commit()` TODO. That branch deliberately leaves
`visit_feature_property()` as the plain in-place call, which is the correct end state; this interim
specialisation exists only to bridge the gap.

**Also fixed alongside** (drive-by, pre-existing on both branches — not caused by the merge):
`TotalReconstructionSequenceRotationInserter::update_finite_rotation()` built its `old_pole` from
`gpml_finite_rotation.get_finite_rotation()` *after* calling `set_finite_rotation()`, so
`old_pole == new_pole`. Benign today (only the `.grot` proxy consumes it, and
`PlatesRotationFileProxy::update_pole` matches on moving-plate-id + time), but it would break `.grot`
editing the moment that match becomes value-sensitive. Now captures the original rotation first.

**Explicitly out of scope** (recorded, not attempted): making the handles `RevisionContext`s;
implementing the `BubbleUpRevisionHandler::commit()` TODO; making Python property-value edits notify.
