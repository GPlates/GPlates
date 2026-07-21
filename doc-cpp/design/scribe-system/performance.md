# Performance: Costs, Prior Work, Future Directions

*Part of the [Scribe system design docs](README.md).*

## Motivation

Pickling large pygplates objects is slow — a `pygplates.RotationModel` or
`pygplates.FeatureCollection` holding a full rotation model / feature collection takes long enough
to matter in workflows that pickle them repeatedly (e.g. multiprocessing in GPlately; see
[GPlates/gplately#366](https://github.com/GPlates/gplately/issues/366)). Pickling is a full
recursive Scribe transcription of every feature, property, property value and geometry point
(see [usage.md](usage.md)) — millions of small, individually tagged, individually tracked objects.

An alternative proposed in that issue — remembering the *filenames* used to create the object and
re-loading from them on unpickle — was rejected: it only works for objects created from files
(not from in-memory feature collections or modified models), ties the pickle to filesystem state
that may have changed or not exist on the unpickling machine, and changes semantics (unpickling
would no longer reproduce the pickled state, but whatever is currently in the files). The chosen
direction is to make transcription itself fast, with no knowledge of how the object was created.

The same speed matters beyond pickling: planned scribe-based file formats — a time-dependent 3D
volume visualisation format, and a binary alternative to the `.gpml` GML/XML format (smaller
files, faster loading) — want the scribe's flexibility/compatibility for structure while dumping
bulk data at raw speed.

## Where the per-object cost is (current design)

> The cost inventory below (and the other doc chapters) describe the scribe system as it exists on
> this branch (`feature/pickle-fast-path`); the section after it describes the parked
> tracking-rework branch that changed some of this internal bookkeeping.

The design optimizes for flexibility and backward/forward compatibility; its per-object constant
factor is large. For every transcribed object (and every *pointer*, which is also an object):

- **`ObjectInfo` construction** (`Scribe.h:1328`): a heavy struct — four
  `SmartNodeLinkedList`s, several `boost::optional`s — pool-allocated per object and appended to
  `d_object_infos`. List nodes come from another pool per link.
- **Address→id map operations**: `d_tracked_object_address_to_id_map` (`Scribe.h:1568`) is a
  red-black tree keyed by `{address, type_info}` whose comparator calls
  `std::type_info::before()`. Even *untracked* objects (the default) are inserted during their own
  transcribe and erased in `post_transcribe` (`Scribe.cc:274`, `:318`) so their children can be
  relocated into them — the map churn happens regardless of `TRACK`.
- **Class-id lookup**: `d_class_type_to_id_map` (`Scribe.h:1571`), another `type_info`-keyed
  `std::map`, consulted per object; polymorphic objects additionally pay `dynamic_cast<void *>` +
  `typeid(*obj)` to find the full-object address.
- **Tag handling**: every `transcribe()` builds an `ObjectTag` (a vector of sections holding
  `std::string` names); the fluent builders (`tag[i]("name")`) copy the section vector per step.
  Each tag name is interned through `Transcription`'s `std::map<std::string, id>` per lookup.
- **Composite key scan**: `CompositeObject::find_key` (`Transcription.h:193`) linearly scans the
  packed encoding array per child access — cheap for small objects, but paid per member per
  object.
- **Owning-pointer machinery per element**: model containers hold `non_null_intrusive_ptr`
  elements, so each element adds a pointer object, the smart-pointer protocol, an export-registry
  lookup (string-keyed map) writing/reading the class-name string, and mandatory tracking of the
  pointee.
- **Call-stack tracking**: each transcribe call pushes/pops a `TRANSCRIBE_SOURCE` trace
  (`Scribe.h:136`); `post_transcribe` snapshots the whole stack for uninitialised pointers.
- **Virtual dispatch** through the type-erased helpers (`TranscribeOwningPointer`, `Relocated`,
  void-cast links) on the polymorphic paths.
- **Two-pass output**: the save path materialises the entire `Transcription` in memory before the
  archive writer serialises it (and load reads the whole transcription before any object is
  constructed).

### What is already optimized

- Pool allocators for `ObjectInfo`, `ClassInfo`, list nodes and `CompositeObject`.
- Interned string and tag-name pools; composites packed into a single `unsigned int` array.
- Binary archive varint + zig-zag encoding and contiguous-id object grouping
  (see [transcription-and-archives.md](transcription-and-archives.md)).
- Tracking is opt-in (`TRACK`), so most objects avoid a *persistent* map entry — though not the
  transient one noted above (removing that is what the parked rework branch below did).

## Prior work: the parked tracking rework (`backup/pickle-perf-rewritten`)

A first attempt at speeding up transcription reworked the tracking bookkeeping itself, on the
branch `feature/improve-pickle-performance` (based, like this branch, on `feature/conda-deps`).
It bought ~1.3× on save / ~1.2× on load (measured below) — real, but far from the
order-of-magnitude goal, and with one unresolved pathological case — so on 2026-07-13 it was
parked: renamed to `backup/pickle-perf-rewritten` (local branch), with this branch
(`feature/pickle-fast-path`) started fresh from `feature/conda-deps` to pursue the fast path
instead. It is unmerged; the remote `private/feature/improve-pickle-performance` still holds its
superseded pre-rewrite history.

### What it contains

The branch was originally 6 scribe commits (2025); on 2026-07-13 its history was rewritten to keep
only the changes judged worth building on (see "What was dropped" below). Its final shape is four
scribe commits plus a fix:

1. **Tracking bookkeeping made strictly opt-in** (`23b6e5ec6`): the tracked-address map is only
   maintained for objects whose tracking was actually requested via the per-call `TRACK` option
   (the pre-branch design inserted *every* load-path object into the map during its transcribe and
   erased it afterwards if untracked); the `TranscriptionScribeContext` id path was split into
   inlined `save_object_id()` / `load_object_id()` fast paths. Untracked *saves* no longer
   deduplicate by address — each occurrence gets a fresh object id.
2. **Object references instead of object ids internally** (`f560a1701`): the internal graph
   passes/stores `ObjectInfo &` directly instead of integer ids that were repeatedly resolved
   through maps.
3. **Intrusive lists** (`f560a1701`): `ObjectInfo` carries `boost::intrusive::list` base hooks
   (`auto_unlink`), replacing the `SmartNodeLinkedList` + node-pool design — zero allocation per
   link, O(1) removal, auto-unlink on destruction.
4. **Retained pointer lists for re-tries** (`c9f19a821`): an object's referencing-pointers list is
   kept when the object is discarded, so a failed object load can be re-tried and still resolve
   its pointers. Also relaxes rules around untracked pointers/references to *untracked* objects.
5. **`remove_all_const_t<>` metafunction** (`10d1cc737`, a cherry-pick of the original
   `6a65b5ef3`) replacing a large block of boost-preprocessor-generated const-stripping overloads
   (mainly compile-time/maintainability). *This one is functionality-neutral and has been
   cherry-picked onto `feature/pickle-fast-path` as `6b73c3518` (re-targeted at this branch's
   internal names: `has_object_been_transcribed`, `get_or_create_class_id`, no `options` on
   `transcribe_base_object`), so it is no longer unique to the rework branch.*
6. **Untrack children of a finished untracked object** (`d8ebaa580`, new in the rewrite): restores,
   in the new data structures, cleanup that commit 1 had removed — see next subsection.

### The bug the rewrite surfaced (and its fix)

Commit `23b6e5ec6` removed the pre-branch behaviour where finishing an *untracked* object
recursively untracked everything transcribed inside it. Children are tracked while their parent is
being transcribed so they can be relocated (eg, sequence items relocated into an untracked
sequence) — but once an untracked temporary (eg, `GmlFile`'s stack-local range-parameters
composite during load-construction) is destroyed and its memory reused, the stale tracked-map
entries confused later objects at the same address: saves threw
`AlreadyTranscribedObject` ("already been saved at the same memory address", 11 pickle tests) and
loads could resolve pointers through freed memory (a segfault in one pickle test).

These failures existed at every intermediate commit of the original branch but were *masked* by
the later type-scoped-tracking commit (`66bb26dc5`), which disabled tracking entirely (nothing
called `enable_tracking()`), trading the save-path failures for load-path
`RelocatedUntrackedObject` failures (32 tests) — the state the branch was found in.

The fix (`d8ebaa580`, `Scribe::untrack_object_and_children`): when an untracked object finishes
`post_transcribe`, its subtree is recursively removed from the tracked-address map, unlinked from
the referencing-pointers list of any pointee (so relocation of the pointee never writes through a
freed pointer address), detached from its parent's child list (so relocating an enclosing tracked
object does not recurse into it), and stripped of all evidence of having been transcribed — so a
later occurrence of the same object id (an object saved through multiple tracked references)
transcribes a fresh second copy, the documented pre-branch behaviour. Additionally, relocating an
*untracked* object to the *same* address is now a silent no-op (nothing moved, nothing to update)
— hit by shared-object-id values in `GmlFile`'s nested attribute maps.

All 439 tests in `pygplates/test/test.py` pass at that branch's tip.

### What was dropped (available in `backup/pickle-perf-full-history`)

- **Type-scoped tracking** (`66bb26dc5`): `enable_tracking<T>()` / `ScopedTrackingGuard<T>`,
  reference-counted per `ClassInfo`. Conceptually attractive (track the items of one container
  without paying for the type everywhere), but it landed half-finished: the per-call `TRACK` flag
  became dead, no call sites enabled tracking, and the transcribe protocols still relocated every
  loaded item — breaking every pickle load. It also masked the save-path bug above. A per-type
  tracking model can be revisited as part of the fast-path design.
- **`LoadRef` deleter change** (`255274149`): stopped untracking/discarding a tracked-but-never-
  relocated `LoadRef<>`, which only made sense under the (dropped) everything-untracked model —
  without it a tracked, un-relocated `LoadRef` would delete its object while leaving a dangling
  map entry.
- **`801266e56`** (relocation of untracked objects = unconditional no-op): the fix that made the
  *old* branch tip pass all 439 tests; superseded by `d8ebaa580`'s narrower same-address-only
  no-op, since with per-call `TRACK` working again a relocation of a *moved* untracked object is
  once again a genuine client error worth diagnosing.

### Measured impact (2026-07-13)

Method: Release build (MSVC), Python 3.14.6, per-call `pickle.dumps`/`pickle.loads` timing
(`time.perf_counter`, 15–500 repeats after warm-up, median reported), same machine back-to-back.
Fixtures: `pygplates/test/fixtures/rotations.rot` (169 KB source, 1,588,009-byte pickle — a
`RotationModel` pickles its underlying feature collections) and `topologies.gpml` (99 KB source,
41,900-byte pickle). "Pre-branch" = the branch point `268580d1f` (`feature/conda-deps` tip — the
code on *this* branch); "old branch tip" = the original 6 commits + `801266e56` (tracking
effectively disabled everywhere); "rework" = the rewritten `d8ebaa580`
(`backup/pickle-perf-rewritten`).

| Benchmark | pre-branch | old branch tip | rework |
|---|---|---|---|
| `RotationModel(rotations.rot)` dumps | 156.9 ms | 120.6 ms | 121.8 ms |
| `RotationModel(rotations.rot)` loads | 166.1 ms | 120.9 ms | 138.0 ms |
| `FeatureCollection(topologies.gpml)` dumps | 4.01 ms | 3.04 ms | 3.31 ms |
| `FeatureCollection(topologies.gpml)` loads | 4.19 ms | 3.01 ms | 3.60 ms |

Takeaways:

- The rework is worth **~1.3× on save** (matching the old tip — on the save path pickling tracks
  almost nothing, so skipping the map entirely vs disabling tracking is nearly equivalent) and
  **~1.2× on load** (vs the old tip's ~1.4× — protocols still `TRACK` + relocate each container
  item on load, so per-item map insert/erase remains; the old tip skipped tracking wholesale, at
  the cost of correctness).
- Pickled size is unchanged everywhere.
- The remaining ~120–140 ms per `RotationModel` round-trip is the general per-object machinery
  (id allocation, tag navigation, composite writes, pointer/registry work per tiny object) that
  tracking changes cannot touch. An order-of-magnitude improvement (the gplately-scale goal) must
  come from the fast path below — which also bounds how much more effort per-object tracking
  optimisation deserves.

### Known issue: pathological load time in one test (confirmed regression, undiagnosed)

`test_model.test.FeatureCase.test_pickle_property_values_not_in_pygplates` passes but takes a
pathologically long — and *wildly variable* — time at the rework branch's tip. Measured
2026-07-13 (same Release build as the table above):

- The entire cost is in `pickle.loads` of a **17,821-byte** pickle: ~13.5 s per call when run in
  isolation (read of the source `.gpml`, `pickle.dumps`, and the result comparison are all
  ≤ 5 ms). For scale, `topologies.gpml`'s 41,900-byte pickle loads in 3.6 ms — this is ~4 orders
  of magnitude off.
- The time is stable *within* a process (13.0–14.5 s over 6 consecutive calls) but varies hugely
  *across* processes: two back-to-back full-suite runs measured this one test at 4.7 s and 26.4 s
  (every other test was stable to within a few percent; the suite is otherwise ~6 s). That
  per-process character points at heap-address-dependent behaviour (ASLR / allocation layout) in
  the address-keyed tracking machinery.
- The fixture (`feature_with_properties_not_in_pygplates.gpml`) is exactly the shared-object-id
  stress case: `UninterpretedPropertyValue` holding nested `XmlElementNode` trees with shared
  alias maps, plus `GmlFile`'s nested attribute maps. Prime suspects, undiagnosed: the
  fresh-copy re-transcription of an already-untracked shared object id (each occurrence re-loads
  the whole subtree from scratch — restored pre-branch semantics, but the tip's save path no
  longer deduplicates untracked saves, so tip-written pickles contain more duplicate subtrees),
  possibly amplified by `c9f19a821`'s object-load re-try machinery into super-linear re-loading
  of nested subtrees.
- **Confirmed to be a rework regression, not pre-existing** (measured 2026-07-13 on a build of
  *this* branch — `cb3bcd61d`, i.e. the branch-point code): the same `pickle.loads` takes
  **1.7 ms** in isolation (vs ~13.5 s at the rework tip — ~8000×), and two full-suite runs were
  both ~6.8 s with this test not even in the slowest 15 (< ~75 ms). The mechanism itself is left
  undiagnosed because the rework branch is parked in favour of the fast-path work here; diagnose
  before ever merging that branch.

### Companion branches

`feature/improve-pickle-performance-wip` holds one throwaway profiling commit (`ab2408a67`, "to be
reverted when continuing this work"; its largest experiments touch `GmlDataBlock.cc` and
`PythonPickle.h/.cc`) based on the *old* history. `feature/test-improve-pickle-performance` (also
old history) ports the changes into the GPlates scribe unit-test harness — note
`src/unit-test/TranscribeTest.cc` on gplates trunk still references pre-rework API
(`has_been_transcribed`, renamed `has_tracked_been_transcribed` by the rework) and needs updating
if those scribe changes ever reach a GPlates build. `backup/pickle-perf-full-history` (local)
preserves the original 8-commit history that the 2026-07-13 rewrite replaced;
`backup/pickle-perf-rewritten` (local, formerly `feature/improve-pickle-performance`) is the
parked rework itself.

## The fast path ("raw lane"): implemented

*This was the future-facing proposal in this section; it is now implemented, on this branch
(`feature/pickle-fast-path`), across phases 0–6 (2026-07-14 to 2026-07-16). See
[fast-path.md](fast-path.md) for the full design: the `RAW` option, the `RAW_STREAM` transcription
kind, the raw codec and its versioning layers, shared-owner backrefs, bulk-array/geometry
adoption, and the constraints anyone touching this code must respect.*

The working idea was a **fast path through the scribe system** for situations that don't need the
general machinery, while leaving the general path intact for complex cases — realised as a
switchable mode inside the existing `Scribe` (the "raw lane") rather than a parallel serialization
scheme:

- Much of the transcribed data is bulk and homogeneous (e.g. millions of rotation samples, point
  arrays, time windows) with no need for per-element tracking, per-element tags, or polymorphism.
  Such data is now dumped/read as contiguous raw data rather than as one composite object per
  element (see "Bulk arrays and geometry" in [fast-path.md](fast-path.md)).
- The general path stays authoritative for structure, headers, heterogeneous/polymorphic parts,
  and anything needing pointer fix-ups — preserving the compatibility story
  ([compatibility.md](compatibility.md)); the raw lane is switched on only where it's been
  deliberately adopted (currently pyGPlates pickling), never for sessions/projects.
- Pointer fix-ups turned out to still matter inside the raw lane itself (shared/aliased owning
  pointers, e.g. `XmlElementNode`'s shared alias maps) even though object *tracking* in the general-
  path sense (address-keyed, for external references) is unused on the pickle graph and stays
  unavailable in raw mode — see the backref scheme in [fast-path.md](fast-path.md).

Intended consumers, in order: pyGPlates pickling (`RotationModel`, `FeatureCollection`, …, **done**),
the planned time-dependent 3D volume visualisation format, and a binary `.gpml` alternative — the
latter two (not yet started) would combine the general path (flexibility, backward/forward
compatibility) with the fast path (raw-data speed) in one archive.

Benchmark harness: `RotationModelTestCase.test_pickle`
(`pygplates/test/test_app_logic/test.py:2586`) pickles a `RotationModel` loaded from a real
rotation file and was the primary target for profiling and regression timing throughout.

### Phase-by-phase results

Same method as the table above (Release build, MSVC, median of repeats after warm-up,
`RotationModel(rotations.rot)` / `FeatureCollection(topologies.gpml)` fixtures):

| Stage | dumps | loads | pickle size | Notes |
|---|---|---|---|---|
| Phase 0 (baseline, this branch's start point) | 151 ms | 159–165 ms | 1,588,009 bytes | Confirms no drift from the pre-branch numbers above |
| Phase 4 (raw lane live for pickling) | ~26 ms | ~41 ms | 345 KB | Three bug classes surfaced by real object graphs, all fixed in this phase (see below) |
| Phase 5 (hot-path call-site trims) | ~20 ms | ~35 ms | 345 KB | `const char *` tag overloads + raw sequence/mapping protocol branches |
| Phase 6 (bulk-array API + geometry) | ~23 ms | ~37 ms | 345 KB | Within noise of phase 5 for *this* fixture — `rotations.rot` is heterogeneous property-value structs, not homogeneous geometry, so it was never phase 6's target |

Overall: **~7.5× on save, ~4.7× on load** for `RotationModel(rotations.rot)` (from phase 0), short
of the ~10×/~7–11× target because the residual cost is per-object owning-pointer machinery
(~9,000 heterogeneous property-value pointers: marker byte + class-name cache + heap construct +
backref) that a bulk API cannot touch — there is no homogeneous array to bulk-transfer in this
fixture. `topologies.gpml` (99 KB source): ~0.74–0.86 ms dumps / ~1.35–1.43 ms loads throughout
phases 4–6 (unchanged, since it's small enough that per-call overhead already dominated less).
The pathological fixture (`feature_with_properties_not_in_pygplates.gpml`, see above): the backref
pre-order fix (phase 4) brought its `pickle.loads` from the rework's ~13.5 s down to ~0.4–0.7 ms,
holding steady through phases 5–6 — the actual fix for the regression this section documents.

For **homogeneous point geometry** — phase 6's actual target — the win is much larger: an ad-hoc
50,000-point synthetic `Polyline`/`MultiPoint` benchmark measured ~1.2–2.3 ms dumps/loads at that
scale (vs. tens of milliseconds on the old per-point path), with zero round-trip coordinate error.
Large, homogeneous `FeatureCollection`s (dense geometry, not rotation-sample metadata) are expected
to see gains closer to the original ~10× target than `RotationModel` does.

### What each phase surfaced

Rolling the raw lane out in stages, rather than all at once, caught real bugs the tag-keyed general
path had been silently tolerating:

- **Phase 4** (enabling the lane for real pickle graphs) surfaced three bug classes, all specific
  to a *positional* stream having no tags to paper over mismatches: (1) a handler saving through
  one integral type and loading through another (harmless with tags; fatal positionally) — fixed
  with the self-describing integer codec in [fast-path.md](fast-path.md); (2) save/load call order
  mismatches in two handlers (harmless with tags; fatal positionally) — fixed by reordering the
  loads to match the saves; (3) shared/nested owning-pointer backref indices computed in a
  different order on save vs load — fixed by reserving a placeholder backref slot before loading a
  shared object's contents (pre-order), matching the save-side registration order. Bug (3) is also
  the actual fix for this section's pathological-load regression.
- **Phase 5** profiling ruled out per-item heap allocation on load as a significant cost (an
  in-place-construction experiment measured <0.5 ms gain and was reverted) — the dominant residual
  turned out to be `ObjectTag` construction at call sites, not object construction itself.
- **Phase 6** confirmed the opposite of a plausible hypothesis: `RotationModel`'s load being slower
  than its save is *not* mainly the load-only cached-reconstruction-tree build (measured at ~1 ms,
  3–4% of the load-side gap) — the ~13 ms load>save asymmetry is intrinsic to heap-constructing
  ~9,000 property values and wiring their owning/backref pointers on load, versus merely walking an
  already-built model on save. This is exactly the per-object cost a bulk API cannot address for a
  heterogeneous fixture.
