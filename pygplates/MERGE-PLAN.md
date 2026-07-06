# Plan: Merge the `pygplates` branch into `gplates` (one develop branch, dual product)

> Companion documents (same directory):
> - `MERGE-FINDINGS-1-model.md` — src/model divergence + `feature/pygplates-model-revisions` perspective
> - `MERGE-FINDINGS-2-property-values-downstream.md` — property-values pattern + app-side divergence
> - `MERGE-FINDINGS-3-build-layout.md` — build system, tree layout, packaging, src/api
> - `MERGE-EXECUTION-DETAILS.md` — empirical `git merge-tree` conflict list, hunk-level resolutions, risk register

> **2026-07-07 landing-strategy update (supersedes the "land on `gplates`" plan below):**
> `gplates` and `pygplates` stay **separate** long-lived develop branches — they are not being
> unified into one. The goal of this work is instead to reconcile the model/build divergence so
> future `pygplates` ⇄ `gplates` merges are trivial (a plain merge, no cherry-picking). Landing is
> now via a **pull request from the staging branch into `pygplates`** (not a fast-forward of
> `gplates`); `gplates` can merge `pygplates` whenever convenient afterwards. The merge commit was
> flattened into a regular single-parent commit so the PR carries no second-parent history back to
> `gplates`. The excluded WIP commit (a4e8f1391) now lives on branch `pygplates-wip` (not a tag),
> and `pygplates` itself was reset back to its pre-divergence tip (3adc36990). See the updated
> Step 3 below and `MERGE-EXECUTION-DETAILS.md` §2/§5 for the exact mechanics.

## Context

GPlates development runs on two long-lived develop branches: `gplates` (desktop app, `GPLATES_BUILD_GPLATES=true`) and `pygplates` (Python module, `=false`), with a merge-one-way / cherry-pick-the-other treadmill between them. The pygplates branch reworked the internal model (`src/model/`, `src/property-values/`) into a bubble-up revisioning system so features/properties are individually accessible from Python. Goal: merge `pygplates` into `gplates` so a single branch builds both products and the treadmill ends.

**Why this is now feasible (exploration findings):**
- `pygplates` already contains nearly all of `gplates`' history — only **22 commits** (post-2.5 fixes: GDAL 3.13, subduction teeth, Hellinger-as-Qt-resource, boost 1.69/1.89, macOS codesign, etc.) are gplates-only; pygplates is 1415 commits ahead.
- The **entire desktop app tree already compiles into the pygplates module** (all source dirs feed `${SOURCE_TARGET}` unconditionally), fully ported to the revisioned model (`get_*` renames, `clone()`, `RevisionedVector` iterators). No stale app code.
- **Undo/redo and view updates are safe**: GPlates undo/redo is `QUndoCommand`-based; the model-change notification layer (`WeakReference*`, `WeakObserver*`, `NotificationGuard`, `ChangesetHandle`) is byte-identical between branches; `BubbleUpRevisionHandler::commit()` fires the same notification path. GPML I/O is visitor-based and unaffected; `FeatureHandle::transcribe()` etc. are additive.
- **`src/api/` is pre-unified on the pygplates side**: gplates' thin wrappers already exist there renamed as `PyOldFeature.*`/`PyOldFeatureCollection.*` (`OldFeature` class), and `PyGPlatesModule.cc` already has `#ifdef GPLATES_PYTHON_EMBEDDING` sections registering the embedded-console exports (console reader/writer, `export_instance`, `export_main_window`, `export_style`, `export_coregistration_layer_proxy` — all verified present and compiled). Only `src/api/Python.cc` (superseded module init) and `src/api/PyFunctions.cc` (old-signature `reconstruct`/`reverse_reconstruct`) are gplates-unique.
- Top-level `CMakeLists.txt` is byte-identical and already dual-target; `doc/`→`doc-cpp/` rename already on both branches; `pygplates/` subtree, `doc-python-api/`, `pyproject.toml`, `FindSphinx.cmake` are purely additive.
- Empirical `git merge-tree --write-tree gplates pygplates`: **14 conflicted paths (12 resolution units)**; everything else auto-merges correctly (audited — incl. Hellinger relocation and `src/api/CMakeLists.txt` where pygplates' list wins automatically).

**User decisions (confirmed):**
- Embedded console adopts the **full pygplates API** (PyGPlatesModule.cc serves both builds — in gplates it's essentially only used by the Python console). Delete `Python.cc`/`PyFunctions.cc`; old thin classes remain as `OldFeature`/`OldFeatureCollection`; `reconstruct`/`reverse_reconstruct` get the new-API signatures (document in release note). Bonus: console users can explicitly load GPML via the full API.
- **Exclude** the pygplates-tip WIP commit a4e8f1391 (`GpmlTopologicalSection.create` polygon fix): merge from the current staging point (3adc36990); tag the pygplates tip `pygplates-final` so the WIP stays recoverable.
- ~~Land on `gplates` by **fast-forward** to the merge commit (first-parent history follows the pygplates lineage — accepted).~~ **Superseded 2026-07-07** — see the landing-strategy update above: lands via PR into `pygplates` instead, keeping both branches.

## Step 0 — Preparation

- `git config rerere.enabled true` (project-local; resolutions replay later for `feature/diligent-migration` sync).
- Confirm branches haven't moved since analysis (`git merge-tree --write-tree gplates 3adc36990` should show the same 14 conflicts; any delta → re-audit).
- Work on `feature/pygplates-merge-into-gplates` at 3adc36990 (current position — do NOT fast-forward to pygplates tip).

## Step 1 — The merge

On the staging branch: `git merge gplates` (expect exactly the conflict set below).

### Conflict resolution table

| Path | Resolution |
|---|---|
| `cmake/modules/Version.cmake` | Header comment → new unified wording ("this branch builds both; default true"). `GPLATES_SEMANTIC_VERSION 2.6.0-4` (gplates). `PYGPLATES_PEP440_VERSION 1.1.0.dev5` (pygplates). |
| `src/CMakeLists.txt` | Both hunks gplates: `_GPLATES_MIN_BOOST_VERSION 1.69`; drop `system` from Boost COMPONENTS. (conda/wheel boost pins are ≥1.69 — verified.) |
| `src/ScribeExportPyGPlates.cc` | Both hunks pygplates: keep `model/ScribeExportModel.h` + `property-values/ScribeExportPropertyValues.h` includes and `SCRIBE_EXPORT_MODEL`/`SCRIBE_EXPORT_PROPERTY_VALUES` macro lines (pickling). |
| `src/presentation/ReconstructionGeometryRenderer.cc` | Take **gplates** side (delete pygplates' orphaned `get_subduction_polarity` member fn — gplates 81a9ce709 moved it to a file-local helper; the `.h` auto-merges to gplates so the member wouldn't compile). Then port 3 old-API lines inside the surviving helper (~merged lines 217/222/226): `->type()`→`->get_type()`, `->value()`→`->get_value()` (×2). **This is the only real code fix in the merge.** |
| `cmake/modules/InstallSharedLibraryDependencies.cmake` | Hunk 1 pygplates (`CODE "set(GPLATES_PYTHON_STDLIB_INSTALL_PREFIX ...)"` injection); hunks 2–3 gplates (generalized all-frameworks macOS codesign loop). Afterwards verify every `GPLATES_PYTHON_STDLIB_INSTALL_PREFIX` use has a matching `set` injection in the same `install(CODE)` scope. |
| `cmake/modules/ConfigDefault.cmake` | All 4 hunks gplates (copyright 2026). |
| `BUILD.Linux` / `BUILD.OSX` / `BUILD.Windows` | Replace the "originated from X branch" paragraph with unified wording: builds both, `-DGPLATES_BUILD_GPLATES=TRUE/FALSE` (default TRUE). |
| `src/qt-resources/python.qrc` | **Union**: keep gplates' `python/scripts/hellinger/hellinger.py` line AND pygplates' 7 `python/api/*.py` lines (all 8 files verified in the merged tree). |
| `src/api/PyFunctions.cc` | modify/delete → **`git rm`** (old-API `deep_clone()`; superseded by `export_reconstruct()` / `PyReconstruct.cc`'s `reverse_reconstruct`, which PyGPlatesModule.cc exports unconditionally). |
| rename/rename triple (`src/system-fixes/boost/cstdint.hpp` ↔ `src/app-logic/VelocityUnits.h` / `src/api/PyInformationModel.cc`) | Spurious rename pairing of two independent deletions. `git checkout gplates -- src/app-logic/VelocityUnits.h`; take the pygplates side of `src/api/PyInformationModel.cc`; ensure `cstdint.hpp` stays deleted. |

### Non-conflict manual fixes (silent — easy to miss)

1. **`cmake/modules/Version.cmake` line ~25: `option(GPLATES_BUILD_GPLATES ... false)` auto-merges to `false`** (base was `true`, only pygplates changed it). Flip to `true`. Nothing will flag this.
2. Pre-commit gate greps on the resolved tree: no conflict markers; `option(GPLATES_BUILD_GPLATES ... true)`; no `src/api/Python.cc` / `PyFunctions.cc` / `system-fixes/boost/cstdint.hpp`; python.qrc has the 8-entry union; no `->deep_clone()` outside `src/gui/DrawStyle*` (that one is a GUI-class method, not model API).

Commit the merge (**M**, parents: staging first, gplates second) — later flattened into a
single-parent commit before landing (see the updated Step 3 below).

## Step 2 — Verification (both variants, Windows, in-tree gitignored build dirs)

1. **GPlates app**: `cmake -B build -S . -G "Visual Studio 17 2022" -A x64` (default `GPLATES_BUILD_GPLATES=TRUE`); build `gplates` + `gplates-unit-test`. This is the first-ever compile of the gplates *executable targets* against the pygplates tree (the library sources all compile today; the new part is `gplates_main.cc`/static-lib link) — expect any fallout here; fix forward on staging.
2. **pyGPlates module**: `cmake -B build-pygplates -S . -DGPLATES_BUILD_GPLATES=FALSE`; build; run `ctest --output-on-failure` (the `pygplates/test/` suite is the regression baseline proving pygplates is unperturbed).
3. **App smoke tests** (manual): launch; load & save GPML; load/save `.gproj` project; edit feature properties + **undo/redo** (highest-risk runtime path over the revisioned model); Hellinger tool (script from Qt resource); export resolved topologies (47c6ccaab); small-circle tool (9e5327bf7); subduction teeth on topological sections and boundaries (exercises the one real code conflict); Python console: full API works (`pygplates.reconstruct(...)`, explicit GPML load), `OldFeature`/`OldFeatureCollection` present, draw-style/colouring scripts still register.
4. `.gproj`/GPML **cross-version round-trip**: files saved by released GPlates 2.5/2.6-dev load in merged build and vice versa (transcribe additions are additive; smoke-test anyway).
5. Configure-only packaging check for both variants; conda recipe / wheel scripts reference nothing branch-specific (verified, re-check).

## Step 3 — Docs & landing (landing steps superseded 2026-07-07 — see update note at top)

1. Docs commit on staging: `README.md` (removed the now-obsolete note against cross-compiling
   GPlates/pyGPlates from the "wrong" branch — `gplates` and `pygplates` remain separate branches,
   but both can now build either product via `GPLATES_BUILD_GPLATES`), `BUILD.*`/`DEPS.*` (already
   unified wording from the Step 1 conflict resolution — verified, no further changes needed);
   `git grep -i 'pygplates branch'` to catch stragglers (none outside historical MERGE-*.md docs).
   Release/porting note (embedded-console `reconstruct`/`reverse_reconstruct` signature change;
   `Feature` in console is now the full-API class, old one = `OldFeature`) is documented in
   `MERGE-EXECUTION-DETAILS.md` §4 for extraction into `CHANGELOG` at the next release cut (this
   repo's `CHANGELOG` is only updated per release, not per merge). Any CLAUDE.md kept outside this
   tree describing the old cross-branch cherry-pick-only constraint (e.g. on
   `feature/diligent-migration`) should be updated when that branch is synced (step 4 below) —
   not yet done, since that constraint is now relaxed rather than removed (see update note).
2. ~~Land: `git checkout gplates && git merge feature/pygplates-merge-into-gplates` (fast-forwards
   to M).~~ **Superseded:** flatten merge commit M into a single-parent commit on the pygplates
   lineage (`git commit-tree M^{tree} -p <pygplates-side-parent> -m "..."`, then `git rebase
   --onto <new-commit> M feature/pygplates-merge-into-gplates` to replay the follow-on fix
   commits), then open a PR from `feature/pygplates-merge-into-gplates` into `pygplates` on the
   `public` remote (github.com/GPlates/GPlates) for review/approval and merge in GitHub.
3. ~~Retire: `git tag pygplates-final a4e8f1391` (preserves the excluded WIP too); delete
   `pygplates` local/remote per team agreement; delete staging branch.~~ **Superseded:**
   `pygplates` is kept, not retired. The excluded WIP commit (a4e8f1391) lives on branch
   `pygplates-wip` instead of a tag; `pygplates` itself was reset back to its pre-divergence tip
   (3adc36990) so the PR above merges into it cleanly. `release-gplates`/`release-pygplates`
   untouched — future releases of both products still cut from their respective branches.
4. Sync `feature/diligent-migration` (merge updated `gplates` into it, once `gplates` has in turn
   merged `pygplates`) **promptly** while rerere cache + rationale are fresh; conflicts only where
   that branch touched pygplates-refactored files. `feature/pygplates-model-revisions` merges
   normally later (its merge-base is inside the pygplates lineage).

## Optional perspective: `feature/pygplates-model-revisions` (not part of this merge)

Extends bubble-up revisioning from property values to features/feature collections: CRTP `src/model/FeatureBase.h` (+1036 lines), multi-parent `RevisionedReference`, ~17 commits (+1707/−472 over 16 model files). Design looks worked out (multi-parent contexts, iterator edge cases addressed); the dominant cost is **~14 months of drift** (last synced 2025-04-01) requiring a re-merge against the current model, then call-site churn and undo/redo + round-trip validation. Order of magnitude: a focused multi-week effort — deferred, and *easier* after this unification (one branch to track instead of two). Details in `MERGE-FINDINGS-1-model.md` §E.

## Risks

| Risk | Mitigation |
|---|---|
| Silent `GPLATES_BUILD_GPLATES=false` default survives the merge | Explicit fix + grep gate (Step 1) |
| gplates.exe target link fallout (first compile of exe targets on this tree) | Whole app tree already compiles into the module today; fix forward on staging before landing |
| Embedded-console API change surprises console-script users | Release/porting note; superset API otherwise |
| App runtime regressions from revisioned model (undo/redo, edit dialogs) | Notification layer byte-identical; targeted smoke tests (Step 2.3) |
| macOS codesign merge untestable on Windows | Kept gplates' proven framework-signing shipped in 2.6.0-4; static-check variable plumbing; schedule one macOS package build of each variant before next release |
| `feature/diligent-migration` re-conflicts | rerere enabled; sync immediately after landing |

**Critical files:** `cmake/modules/Version.cmake`, `src/CMakeLists.txt`, `src/presentation/ReconstructionGeometryRenderer.cc`, `cmake/modules/InstallSharedLibraryDependencies.cmake`, `src/qt-resources/python.qrc`, `src/ScribeExportPyGPlates.cc`, `src/api/PyGPlatesModule.cc` (reference — no edit expected).

## Suggested Claude model per step

| Step | Model | Rationale |
|---|---|---|
| Step 0 (prep, merge-tree re-check) | Sonnet 5 (or Haiku 4.5) | Mechanical, fully scripted above. |
| Step 1 (merge + conflict resolution + audit gates) | **Fable 5** (or Opus 4.8) | The one-shot critical step. Resolutions are prescribed, but judgment is needed if branches moved, if merge-tree output differs, and for the `ReconstructionGeometryRenderer` reconciliation + silent-default audit. |
| Step 2 (build both variants, fix compile/link fallout, run ctest) | Opus 4.8 (fast mode is handy for long build-fix loops) | Iterative build-and-fix; escalate to Fable 5 only if a deep architectural link problem appears. |
| Step 2.3–2.4 (app smoke tests) | Human (you) + any model to assist | GUI interaction; a model can drive builds/logs but the visual/undo checks are manual. |
| Step 3 (docs, landing via PR into `pygplates`) | Sonnet 5 | Mechanical text edits and prescribed git commands; landing is a reviewed PR, not a fast-forward. |
