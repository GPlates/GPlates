# The Fast Path ("Raw Lane"): Design and Implementation Plan

*Part of the [Scribe system design docs](README.md). Status: **approved plan, not yet
implemented** (2026-07-14). This is the work of the `feature/pickle-fast-path` branch; see
[performance.md](performance.md) for the motivation, cost inventory and prior work.*

## Summary

A **scoped streamed mode inside the existing `Scribe`** — the "raw lane". A new `RAW` option bit
on a transcribe call makes that object's entire subtree stream *positionally* (order-defined)
into a single new `RAW_STREAM` blob object in the `Transcription`, written by the binary archive
as one bulk `writeRawData` run. Inside the lane there are no object ids, no tags, no
`ObjectInfo`, no tracking maps and no composite encoding — the machinery identified in
[performance.md](performance.md) as the per-object constant factor.

The same ~200 existing client `transcribe()` handlers drive **both** lanes unchanged: `Scribe`
has no virtuals, so a `bool d_is_raw` member checked at 8 internal branch points re-routes
everything (the delegate protocol, `Scribe::transcribe_delegate_object`, is the existing
precedent — it already streams into the current slot with no id/tag/tracking). The lane is
switched on for pygplates pickling and off for sessions/projects and the future binary `.gpml`
alternative (which will combine general-path structure with raw bulk leaves).

Expected gain for `RotationModel(rotations.rot)` pickling: **~10× on save, ~7–11× on load**
(from ~157/166 ms; see the accounting below).

## Why this design

- One `GpmlTimeSample` today costs ~12–16 tracked object ids, ~3 polymorphic class-name *string
  objects*, and ~30–40 scribe calls; `rotations.rot` ≈ 2,331 samples plus a ~500-feature skeleton
  ⇒ ~40–55k transcription objects and ~100k+ scribe calls per pickle. Nothing on the path is
  bulk — even geometry writes three tracked `Real` objects per point.
- The parked tracking rework (`backup/pickle-perf-rewritten`) proved bookkeeping-only changes cap
  out at ~1.3×; the order-of-magnitude goal requires not creating the per-object records at all.
- Whole-subtree raw for pickles (rather than raw only at bulk leaves) removes ~95% of the
  machinery instead of ~70% — and `RotationModel`/`FeatureCollection` have little truly
  homogeneous bulk, so per-element machinery removal, not columnar layout, is where the win is.
- Structure-of-arrays / per-column tags were considered and deferred: general path at structural
  levels + raw sections at lower levels already emulates "one tag per column" for the formats
  that need tag compatibility (volume format, binary `.gpml`), and the raw blob layout is
  scribe-owned so a columnar layout can be introduced later behind a codec version bump without
  touching client code.

## Decisions (agreed 2026-07-14)

1. **Raw lane as a switchable mode, same handlers** — no duplicate serialization code per type.
2. **Whole-subtree raw for pickles**: `PythonPickle`'s `Transcribe<Holder>` hook saves the holder
   with `RAW`, so the entire pickle payload is one blob.
3. **Order-defined, version-gated raw sections** (no per-field tag skipping inside a section).
   Three versioning layers:
   - **Codec version** (scribe-owned): varint at every blob head; covers value encodings, pointer
     markers, backref scheme. Layout evolution (e.g. columnar) happens behind this, invisible to
     clients.
   - **Section version** (client-owned): written at the section front by the entry-point client —
     `PythonPickle` owns it for pickles. The coarse gate for unanticipated layout changes.
   - **Opt-in per-type version ints**: a handler expecting evolution may transcribe its own
     version int at a location of its choosing (works identically in both lanes; ~1 byte per
     instance). Boost::serialization-style *automatic* per-type versions are a documented later
     upgrade behind a codec bump — not implemented now.
4. **Compatibility guarantees**: new pygplates always loads old pickles — on load the boundary
   object dispatches on its transcription kind (`COMPOSITE` → general path, `RAW_STREAM` → raw
   lane). Old pygplates reading a new (raw) pickle fails with a clean `UnsupportedVersion`, not
   garbage. Pickles are transient today (multiprocessing), but dumping them to files is supported
   under these guarantees.

## Design

### New `Transcription` object kind

- `RAW_STREAM` added to `ObjectType` **before `UNUSED`** (`Transcription.h:234`; `UNUSED` must
  stay last — it is the hole marker and default). Storage `std::vector<std::vector<char>>`
  (keeps `Transcription` Qt-free) + `add_raw_stream` / `get_raw_stream` /
  `get_num_raw_stream_objects` mirroring the string methods (`Transcription.cc:272`).
- `operator==` gets a bytewise case (the switch at `Transcription.cc:681` has `default: Abort`).
  Note this is bit-exact while `FLOAT`/`DOUBLE` compare with tolerance — irrelevant for pickling;
  flag if sessions ever adopt raw (change-detection would get stricter).
- `is_complete()` needs no change (leaf kind, no child references).
- Refactor `add_string` to expose `get_or_create_unique_string_index(const std::string &)`: the
  raw lane interns strings and class names into the **existing unique-string pool** and stores
  varint indices. Archive ordering already works — the binary writer emits the pool before
  objects, the reader populates it before parsing objects.

### Archive layer

- `RAW_STREAM_CODE = 6` in `ScribeArchiveCommon.h:98`; binary writer/reader case = varint length
  + `QDataStream::writeRawData`/`readRawData` (`ScribeBinaryArchiveWriter.cc:167–231`,
  `Reader.cc:136–194`).
- **Conditional format version bump**: write `BINARY_ARCHIVE_FORMAT_VERSION = 1` only when the
  transcription contains a raw stream, so non-raw archives (sessions, projects) keep writing
  version 0 and stay readable by old builds. Old readers hitting version 1 fail cleanly with
  `UnsupportedVersion` (`ScribeBinaryArchiveReader.cc:78`) — without the bump they would hit an
  unknown type code and raise a confusing `ArchiveStreamError`.
- Text/XML archives: base64-encode the blob (deprecated / unit-test-only formats; low effort).

### Raw mode inside `Scribe`

- `RAW = 1 << 3` in `ScribeOptions.h` (bits 3+ are free). RAII guard + a `RawContext` member on
  `Scribe`: {codec version, save buffer, load cursor, shared-object dedup maps, export-registry
  caches}.
- **The boundary object transcribes normally** (object id, tag, `pre_transcribe` /
  `post_transcribe` — preserving `is_transcription_complete` and stack invariants). Only the
  *stream* step is replaced: the subtree streams into the blob, then a new
  `TranscriptionScribeContext::transcribe_raw_stream(...)` binds it to the current object id
  exactly like the `std::string` primitive case (`TranscriptionScribeContext.cc:367`; category
  stays `PRIMITIVE`, no new `ObjectCategory` needed).
- **Branch points** (a single predictable `if (d_is_raw)` each), validated against the code:
  1. `transcribe_object(ObjectType &, tag, options)` (`Scribe.h:3018`) → `stream_object` directly.
  2. `transcribe_object(ObjectType *&, ...)` (`Scribe.h:3182`) → raw owned-pointer path if an
     owner option is set; non-owning raw pointers → throw.
  3. Both `transcribe_construct_object` overloads (`Scribe.h:3096`, `:3126`) →
     `stream_construct_object` (gives `load<T>()` / `transcribe_construct_data` for free).
  4. `stream(..., StreamPrimitiveTag)` (`Scribe.h:4566`) → raw codec. **Essential**: the delegate
     protocol calls `stream_object` directly, bypassing the other funnels.
  5. `transcribe_smart_pointer_object` (`Scribe.h:3727`).
  6. `transcribe_base_object` (`Scribe.h:3760`) — **void-cast registration still runs** (needed
     for backref `up_cast` on load); id/list bookkeeping skipped; base sub-object streamed.
  7. `relocated` (`Scribe.h:2736`) → silent no-op, placed *before* the tracked-map lookup that
     throws `RelocatedUntrackedObject`. This alone makes the sequence, mapping and
     `boost::optional` protocols correct **unchanged** in raw mode (their per-element `TRACK` +
     `relocated()` calls become harmless).
  8. `save_object_reference` / `load_object_reference` (`Scribe.h:3854`, `:3913`) → throw.
     References and `is_in_transcription` are verified unused in the pickle graph (only session
     code uses them: `TranscribeSession.cc`, `BuiltinColourPaletteType.cc`).
- `LoadRef` destruction is already raw-safe: `Scribe::untrack_object` (`Scribe.h:4079`) silently
  returns when the address isn't tracked — no throw, no double-free.
- `Bool` return semantics unchanged. Raw load failures **throw** (unknown class name, cursor
  overrun, version mismatch) — the cursor cannot resync, and this matches the existing pickle
  contract (`PythonPickle.h:84` already converts any load failure to `UnsupportedVersion`).

### Raw codec (little-endian only; `static_assert` on big-endian targets)

- Arithmetic: fixed-width little-endian memcpy append; `bool`: 1 byte; counts/sizes/indices:
  varint.
- 64-bit integers written full-width — a deliberate, documented divergence (the general path
  throws on `long long` outside 32-bit range, `TranscriptionScribeContext.cc:938`).
- `std::string` / `QString`: varint unique-string-pool index (keeps interning for the repeated
  `QualifiedXmlName` triples).
- Owned pointers: marker byte {`NULL`, `INLINE`, `SHARED_FIRST`, `SHARED_BACKREF` + varint
  index}; polymorphic pointee class name = pool string index — replacing today's full transcribed
  string object per pointee.

### Shared-owner dedup (backrefs)

Preserves aliasing (shared `XmlElementNode` alias maps, `GmlFile` attribute maps) and prevents
the exponential re-write blowup that produced the parked rework's pathological 13-second load
(see [performance.md](performance.md)):

- Thread an optional `use_count_hint` through `transcribe_smart_pointer_protocol`
  (`TranscribeSmartPointerProtocol.h:50`) — the refcount is *not* reachable at `Scribe` level
  because callers strip smart pointers to raw `T*` before the protocol call. The
  `boost::shared_ptr` overload passes `use_count()` (`TranscribeBoost.h:400`); intrusive-pointer
  overloads supply `GPlatesUtils::ReferenceCount::get_reference_count()` via a SFINAE hook
  (`TranscribeNonNullIntrusivePtr.h:80`); unknown → always-dedup (conservative default).
- hint == 1 (the common case) skips the dedup map entirely. hint > 1 → save-side address→index
  map entry; repeats write `SHARED_BACKREF` + index. Load keeps an encounter-ordered vector of
  {dynamic address, `type_info`} (deterministic — load replays save order) and resolves backrefs
  through `d_void_cast_registry.up_cast`. The existing `d_shared_ptr_map` control-block sharing
  works unchanged in raw mode (it is keyed by dynamic address, not object id).
- New virtual `TranscribeOwningPointer::load_object_raw(Scribe &) → void *`
  (`ScribeInternalUtils.h:449`, `ScribeInternalUtilsImpl.h:100`) — the existing `load_object`
  returns the heap address only through `ObjectInfo`, which doesn't exist in raw mode.

### Residual hot-path costs to trim (measure first)

- **`ObjectTag` construction at call sites** (~20–30 ms/pickle — material against a ~16 ms
  budget): the entry signatures take `const ObjectTag &`, so the implicit `const char *`
  conversion (heap alloc) happens caller-side *before* `Scribe` sees the mode flag. Fix: add
  non-template `const char *` overloads of `transcribe`/`save`/`load` that check `d_is_raw`
  before constructing the `ObjectTag` (overload resolution beats the user conversion — zero
  call-site changes), plus raw branches in the sequence/mapping protocols that skip the
  per-element `ObjectTag()[n]` builders.
- **ExportRegistry lookups** (~1–2 ms; `std::map` keyed by `type_info` with `before()` ordering):
  per-`Scribe` cache `type_info * → {ExportClassType *, pool string index}` on save; lazy
  `vector<const ExportClassType *>` indexed by pool id on load.
- **Per-item heap allocation on load** (`LoadConstructObjectOnHeap` per sequence item,
  ~100–200 ns — the dominant residual load cost): in-place construction in the protocols' raw
  branches (phase 5).

## Implementation phases

Each phase compiles, passes the full test suite, and is a natural commit boundary. The model
column is guidance for executing phases with Claude Code — heavier models where the work is
subtle template/lifetime surgery in `Scribe.h`, lighter where the work is well-specified and
mechanical.

| Phase | Work | Files | Claude model |
|---|---|---|---|
| **0 — Baseline** | Benchmark `RotationModelTestCase.test_pickle` (`pygplates/test/test_app_logic/test.py:2586`) per performance.md's method (Release, median of repeats after warm-up); record times + pickle sizes; also `FeatureCollection(topologies.gpml)` and the pathological fixture load. | (none — measurement only) | Sonnet 5 |
| **1 — Transcription kind + archives** | `RAW_STREAM` kind, storage + accessors, `operator==` case, string-pool refactor; `RAW_STREAM_CODE`; binary writer/reader case with conditional format-version bump; text/XML base64 cases. | `Transcription.h/.cc`, `ScribeArchiveCommon.h`, `ScribeBinaryArchiveWriter/Reader.cc`, `ScribeTextArchive*.cc`, `ScribeXmlArchive*.cc` | Sonnet 5 |
| **2 — Raw lane core** | `RAW` option; `RawContext` + RAII guard; boundary enter/exit with `COMPOSITE` fallback on load; branch points 1–4 & 7–8; codec; strings via pool; new exception(s). | `ScribeOptions.h`, `Scribe.h/.cc`, `TranscriptionScribeContext.h/.cc`, `ScribeExceptions.h` | **Fable 5** (or Opus 4.8) — the subtle heart: templated entry paths, delegate interactions, exception safety of the guard |
| **3 — Pointers** | Branch points 5–6; marker bytes; class names as pool ids + registry caches; `load_object_raw`; `use_count_hint` threading + SFINAE refcount hook; backref save/load maps. | `Scribe.h/.cc`, `ScribeInternalUtils.h/Impl.h`, `TranscribeSmartPointerProtocol.h`, `TranscribeNonNullIntrusivePtr.h`, `TranscribeBoost.h` | **Fable 5** (or Opus 4.8) — aliasing/lifetime correctness is where the parked rework failed |
| **4 — Pickle adoption** | `Transcribe<Holder>::pickle/unpickle` pass `RAW` + write/check the pickle section version; full 439-test suite incl. all ~49 `test_pickle`; benchmark vs phase 0. | `PythonPickle.h` | Opus 4.8 — integration + debugging of whatever surfaces |
| **5 — Hot-path trims** | `const char *` tag overloads; protocol raw branches (skip `ObjectTag()[n]`, skip per-element `relocated`); in-place item construction on load; ExportRegistry caches if not done in 3; re-benchmark after each. | `Scribe.h`, `TranscribeSequenceProtocol.h`, `TranscribeMappingProtocol.h` | Opus 4.8 — profiling-driven; C++ overload-resolution subtleties |
| **6 — Bulk-array API** | `transcribe_raw_array(source, T *, count)` for trivially-copyable arithmetic (general-path fallback = element loop); geometry adoption (`vector<PointOnSphere>` = 3N doubles in Release — `PolylineOnSphere.cc:323`, `MultiPointOnSphere.cc:230`); groundwork for the volume format / binary `.gpml`. | `Scribe.h`, `TranscribeArray.h`, geometry `.cc` files | Sonnet 5 |
| **Docs** | Update [performance.md](performance.md) (future directions → implemented) and turn this plan into a `fast-path.md` design chapter (codec, versioning layers, constraints: no references/probing inside raw sections, evolution discipline). | `doc-cpp/design/scribe-system/` | Sonnet 5 |

## Verification

- **Unit tests** (`src/unit-test/TranscribeTest.cc`, built in this repo): raw round-trips of
  primitives / 64-bit integers / string interning / `boost::optional` none+some / sequences and
  maps / owned polymorphic pointers / intrusive dedup (refcount > 1) / `shared_ptr` aliasing
  (XmlElementNode-style shared maps) / NULL pointers / nested-`RAW` option / codec- and
  section-version rejection / old-format (`COMPOSITE`) fallback / errors for references and
  non-owning pointers in raw mode.
- **Full pygplates suite**: all 439 tests, including the ~49 `test_pickle` cases.
  `test_pickle_property_values_not_in_pygplates` must round-trip **and stay ~ms** — it is the
  parked rework's pathological shared-alias-map case; the backref scheme is the designed fix.
- **Cross-version**: a fixture of pre-raw pickle bytes must load via the fallback; a raw pickle
  presented to a format-version-0 reader must raise a clean `UnsupportedVersion`.
- **Benchmarks** (performance.md method, Release build): targets ~10× dumps (≲16 ms) and ~7–11×
  loads (≲25 ms) for `RotationModel(rotations.rot)`; record results in
  [performance.md](performance.md).
- **GPlates-side safety**: sessions/projects never pass `RAW`; confirm a non-raw binary archive
  still writes format version 0 byte-identically.
