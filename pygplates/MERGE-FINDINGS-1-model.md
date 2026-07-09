# Divergence of `src/model/` between `gplates` and `pygplates` branches

> Exploration report (agent 1 of 3) for the pygplates→gplates branch unification. See `MERGE-PLAN.md`.

**Bottom line up front:** The pygplates branch replaced GPlates' old "clone-on-write" model with a full **bubble-up revisioning system** (`Revisionable` / `Revision` / `RevisionContext` / `ModelTransaction` / `BubbleUpRevisionHandler` / `RevisionedReference` / `RevisionedVector`). Crucially, **the pygplates branch already ported the entire desktop app to the new model API** — both branches carry the full app tree (`gui`, `qt-widgets`, `app-logic`, `file-io`, …), and pygplates already uses `get_property_name()` in 56 app files, has zero remaining `TopLevelPropertyRef` uses, etc. So this is not "make gplates app compile against a foreign model" from scratch; pygplates *is* GPlates-app + revisioning + Python API. The **weak-reference/callback publisher subsystem is byte-identical** between the two branches, which is what preserves app behavior. The realistic merge question is reconciling two develop lines, and the model layer is best unified (not `#ifdef`-walled), because the pygplates model is a strict superset in capability with the app already adapted.

Scale for context: whole-tree divergence is **~400 files, +84k/−11k** (path-filtered; 482 files counting docs/packaging); `src/property-values` alone is **102 files, +13.9k/−3.8k** (every PropertyValue subclass was rewritten for revisioning). `src/model` is the 36-file, +3.75k/−973 core described below.

---

## (A) Per-major-file summary

**New files (all additive — the revisioning engine):**
- `Revision.h` — base class holding mutable/revisionable state; carries `boost::optional<RevisionContext &> d_context` (the bubble-up parent link) and a `clone_revision()` virtual.
- `Revisionable.h/.cc` — new abstract base for all revisionable model entities; holds a single `mutable Revision::non_null_ptr_to_const_type d_current_revision`, exposes `clone()`, `get_model()`, `create_bubble_up_revision()`, protected `get_current_revision<RevisionType>()`. `PropertyValue` and `TopLevelProperty` now derive from it.
- `RevisionContext.h` — interface with `bubble_up(ModelTransaction&, revisionable)` and `get_model()`; parents (TopLevelPropertyInline, feature handles, RevisionedVector) implement it.
- `ModelTransaction.h/.cc` — collects `RevisionTransaction` records and `commit()`s them, swapping current revisions atomically.
- `BubbleUpRevisionHandler.h/.cc` — RAII helper used inside every mutating setter; builds the bubble-up revision chain to the feature store and commits (also drives model-event signalling under the notification guard).
- `RevisionedReference.h/.cc` — smart handle to a nested revisionable that manages parent/child context links and ref-counting into revisions.
- `RevisionedVector.h` (+1174 lines) — revisioned `std::vector`-like container of revisionable children, itself a `RevisionContext`; this is the workhorse for property lists / feature-collection contents.
- `TranscribeRevisionedVector.h`, `ScribeExportModel.h` — Scribe (serialization) support, new.

**Rewritten files (semantic change):**
- `PropertyValue.h/.cc` — was `ReferenceCount` + instance-id + `deep_clone_as_prop_val()` + `operator==` with `directly_modifiable_fields_equal()`. Now derives from `Revisionable`, drops the instance-id machinery and the `DEFINE_FUNCTION_DEEP_CLONE_AS_PROP_VAL` macro, adds nested `Revision` class. `deep_clone_as_prop_val()` → `clone()` (via `clone_impl()`).
- `TopLevelProperty.h/.cc` — same conversion; **accessor renames** `property_name()`→`get_property_name()`, `xml_attributes()`→`get_xml_attributes()`; `set_xml_attributes` now goes through a revision; `clone()`/`deep_clone()` pair collapsed into one `clone()`; `operator==` replaced by `equality(const Revisionable&)`.
- `TopLevelPropertyInline.h/.cc` (+530/+279) — now also `public RevisionContext`; the container became a revisioned vector; introduces a proxy `Reference<PropertyValueQualifiedType>` and custom bidirectional `Iterator` (works across revisions). Notably **assignment through an iterator (`*iter = new_ptr`) is deliberately deleted**.
- `FeatureHandle.h/.cc` — `set(iter, new_child)` signature changed from `non_null_ptr_to_const_type` → `non_null_ptr_type`; removed the `new_child_equals_existing`/`deep_clone()` clone-on-set logic (now stores the child directly); added `transcribe()` / `transcribe_construct_data()`.
- `FeatureCollectionHandle.h/.cc` — `tags()` inlined; added `transcribe()`.
- `BasicHandle.cc` — `actual_add` no longer `deep_clone()`s the incoming TopLevelProperty ("we can't allow direct modification" comment deleted); stores directly.
- `FeatureVisitor.h/.cc` — the `FeatureHandle`/`const FeatureHandle` template specializations of `visit_feature_property` (which existed to clone properties for non-const visits) were **removed and unified** into one template that just calls `accept_visitor`. The helper class **`FeatureVisitorThatGuaranteesNotToModify` was deleted** (no longer needed — revisioning makes non-const visiting cheap and safe).
- `HandleTraits.h` / `RevisionAwareIterator.h/.cc` — FeatureHandle iterator `iterator_value_type` changed from `TopLevelPropertyRef` to `PointerTraits<TopLevelProperty>::non_null_ptr_type`; the `RevisionAwareIterator<FeatureHandle>::current_element()` specialization removed.
- `ModelUtils.h/.cc` — `create_gml_time_sample` now returns `GpmlTimeSample::non_null_ptr_type` instead of a `GpmlTimeSample` value (consequence: property values are now non-copyable revisionables).

**Deleted files:**
- `TopLevelPropertyRef.h/.cc` — the old clone-on-write proxy returned when dereferencing a non-const feature iterator. Its whole reason for existence is subsumed by revisioning.

---

## (B) Classification of change types

**(a) Purely additive — harmless to app code:** all 9 new engine files (`Revision*`, `Revisionable*`, `ModelTransaction*`, `BubbleUpRevisionHandler*`, `RevisionedReference*`, `RevisionedVector.h`), the Scribe `transcribe*` methods on `FeatureHandle`/`FeatureCollectionHandle` (gplates has **no** transcribe on these — confirmed absent on the gplates branch), and `ScribeExportModel.h` / `TranscribeRevisionedVector.h`. Inlining of `FeatureCollectionHandle::tags()` is behavior-neutral.

**(b) Signature / semantic changes that touch app code:**
- Accessor renames `TopLevelProperty::property_name()`→`get_property_name()` and `xml_attributes()`→`get_xml_attributes()`. In *gplates* these are used in ~51 and ~11 files respectively outside `src/model`; in *pygplates* they are already ported (56 files use `get_property_name()`; only 6 residual `.property_name()` calls, which are unrelated methods on other classes).
- `FeatureHandle::set(iterator, non_null_ptr_type)` (was `non_null_ptr_to_const_type`).
- FeatureHandle iterator dereference type `TopLevelPropertyRef` → `non_null_ptr_type` (`HandleTraits.h`), and the removal of `*iter = x` assignment via `TopLevelPropertyInline::Reference::operator=` being `= deleted`.
- `ModelUtils::create_gml_time_sample` return-type change (value → `non_null_ptr_type`).
- `deep_clone_as_prop_val()` / `TopLevelProperty::deep_clone()` → unified `clone()`. gplates uses `deep_clone` in 85 files and `deep_clone_as_prop_val` in 10; pygplates has already reduced app `deep_clone` usage to 7 files.

**(c) Removals of gplates functionality:**
- `FeatureVisitorThatGuaranteesNotToModify` (used in 5 gplates files) — deleted; the performance hack it represented is obsolete.
- `TopLevelPropertyRef` (7 gplates files) — deleted.
- `PropertyValue` instance-id / `directly_modifiable_fields_equal` / `update_instance_id` — deleted; equality is now structural via `equality()`.

**Estimate of conditional-compilation need:** **Low.** Because pygplates already carries a fully-ported app against this model, almost nothing needs `#ifdef GPLATES_BUILD_GPLATES` at the model layer — the model should be *unified*, and the app code should be taken from the pygplates side. Conditionals, if any, would be confined to (i) Python-only extras (e.g. `ScribeExportPyGPlates`, pickling entry points) and (ii) any spot where the gplates develop line added app features after the branches diverged that still call the *old* API — those are app-side reconciliations, not model `#ifdef`s. The revisioning engine itself is inert when unused, so it does not need to be walled off.

---

## (C) Verdict on gplates-app compatibility

The gplates app **can compile and behave identically** against the pygplates model, and in fact the pygplates branch is the existence proof — it builds the same `gplates_main.cc` / `gui` / `qt-widgets` tree against the revisioned model. Behavioral identity rests on two facts confirmed here:

1. **Undo/redo is not built on model clone-on-write.** GPlates' undo/redo is `QUndoCommand`-based (`gui/*UndoCommand`, canvas-tool workflows). The model's own change tracking is the **weak-reference / callback publisher subsystem**, and `WeakReference.h`, `WeakReferenceCallback.h`, `WeakObserver.h`, `WeakObserverPublisher.h`, `WeakObserverVisitor.*`, `NotificationGuard.*`, `ChangesetHandle.*` are **identical between the two branches** (not in the 36-file diff at all). So model-modification events that the app's undo and view-update logic depend on still fire; `BubbleUpRevisionHandler::commit()` routes through the same `notify_listeners_of_modification` path (visible in `FeatureHandle::set`).
2. The one true semantic shift is that **non-const feature-property visitation no longer silently clones**. Old GPlates cloned a property when you dereferenced a non-const feature iterator (`TopLevelPropertyRef`) or ran a non-const `FeatureVisitor`; code then had to write the property back. Under revisioning, mutation is captured by the revision/bubble-up machinery instead. The pygplates app has already been adjusted for this (0 `TopLevelPropertyRef`, no `FeatureVisitorThatGuaranteesNotToModify`), so as long as the merge takes the pygplates app-side edits, behavior matches.

**Where conditionals would plausibly be needed:** only around Python-lifecycle glue (pickling/transcribe entry points exported via `ScribeExportPyGPlates.cc` vs `ScribeExportGPlates.cc`) and any late gplates-only app features that must be re-ported from old→new accessor names. No structural `#ifdef` inside `Revisionable`/`Revision`/`PropertyValue`/`TopLevelProperty` is warranted.

---

## (D) Serialization / on-disk compatibility

- **GPML (`.gpml`) I/O is unaffected.** GPML reading/writing goes through the file-io feature *visitors* using property-value structural types, not through the model's Scribe path. Property-value external representation (StructuralType, visitor accept methods) is preserved; the revisioning is purely in-memory. No save-file format change from the model refactor.
- **Native project/session Scribe format:** the `transcribe()` / `transcribe_construct_data()` methods on `FeatureHandle` and `FeatureCollectionHandle`, plus `TranscribeRevisionedVector.h`, are **new** (gplates has none). gplates projects reference feature files rather than embedding features, so this is additive capability (used by pyGPlates pickling and any embedded transcription), not a change to an existing gplates format — low risk to existing `.gproj` compatibility, though it should be smoke-tested.
- **pyGPlates pickle compatibility** is a pygplates-internal concern; it is *introduced* by this branch (via the new transcribe methods and `ScribeExportModel.h`), so there is no pre-existing pickle format to break.

---

## (E) `feature/pygplates-model-revisions` branch — perspective

**What it does:** extends the bubble-up revisioning system *upward* from property values to **features and feature collections** (and the feature store root). The centerpiece is a large new header **`src/model/FeatureBase.h` (+1036 lines)** — a `FeatureBase<DerivedHandle, ChildType>` CRTP base that manages revisioned children, so `FeatureHandle` inherits `FeatureBase<FeatureHandle, TopLevelProperty>` **and** `RevisionContext`. Commit log highlights: multiple parent contexts per revision (`RevisionedReference` owns the parent pointer, enabling the same property to live in two features), visiting all parent contexts toward the model root when bubbling up, and relaxed iterator semantics (remove-then-reset, remove-a-removed-child no-ops). It reworks `FeatureHandle.h/.cc` heavily and touches `Revision*`, `RevisionedReference*`, `RevisionedVector.h`, `WeakReference.h`, `TopLevelProperty*`.

**How far along / how hard to finish:** It is a coherent, sizeable increment — **~17 path-filtered commits ahead of `pygplates`**, +1707/−472 across 16 model files, with the design clearly worked out (the CRTP `FeatureBase`, multi-parent support, iterator edge cases all addressed). The main risk is **staleness, not incompleteness**: its last integration with `pygplates` was a **2025-04-01 merge**, while `pygplates` has since advanced to **2026-06-16** — roughly 14 months of drift, so a non-trivial re-merge/rebase against current `pygplates` (especially `FeatureHandle`, `RevisionedVector`, `RevisionedReference`, `WeakReference`) is the first real cost. Functionally it looks close to feature-complete for property/feature/feature-collection revisioning, but finishing would require: (1) reconciling with the latest `pygplates` model, (2) re-porting app + Python-API call sites to the `FeatureBase` API and the multi-parent `RevisionedReference`, and (3) validation of undo/redo + save-file round-trips. Order-of-magnitude: a focused but multi-week effort, dominated by the drift re-merge and downstream call-site churn rather than by unsolved design questions.

---

**Key file references:** `src/model/Revisionable.h`, `Revision.h`, `RevisionContext.h`, `ModelTransaction.h`, `BubbleUpRevisionHandler.h`, `RevisionedReference.h`, `RevisionedVector.h`, `PropertyValue.h`, `TopLevelProperty.h`, `TopLevelPropertyInline.h` (proxy `Reference`/`Iterator`), `FeatureHandle.cc` (`set`, `transcribe`), `FeatureVisitor.h` (removed `FeatureVisitorThatGuaranteesNotToModify` + specializations), `HandleTraits.h` (iterator_value_type), deleted `TopLevelPropertyRef.h/.cc`; unchanged callback layer `WeakReference.h` / `WeakObserverPublisher.h` / `NotificationGuard.*` / `ChangesetHandle.*`; and on the secondary branch `src/model/FeatureBase.h`.
