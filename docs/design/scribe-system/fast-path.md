# The Fast Path ("Raw Lane")

*Part of the [Scribe system design docs](README.md). Implemented on `feature/pickle-fast-path`,
phases 0–6 (2026-07-14 to 2026-07-16); see [performance.md](performance.md) for the motivation,
the per-object cost inventory it addresses, and the measured results.*

## Summary

A **scoped streamed mode inside the existing `Scribe`** — the "raw lane". A new `RAW` option bit
(`ScribeOptions.h:80`) on a transcribe call makes that object's entire subtree stream
*positionally* (order-defined) into a single `RAW_STREAM` blob object in the `Transcription`
(`Transcription.h:242`), written by the binary archive as one bulk `writeRawData` run
(`ScribeBinaryArchiveWriter.cc:428`). Inside the lane there are no object ids, no tags, no
`ObjectInfo`, no tracking maps and no composite encoding — the machinery
[performance.md](performance.md) identifies as the per-object constant factor.

The same ~200 existing client `transcribe()` handlers drive **both** lanes unchanged: `Scribe` has
no virtuals, so a `bool d_is_raw` member (`Scribe.h:1771`) checked at internal branch points
re-routes everything (the delegate protocol, `Scribe::transcribe_delegate_object`, was the
existing precedent — it already streams into the current slot with no id/tag/tracking). The lane
is switched on for pyGPlates pickling (`PythonPickle.h:101`) and stays off for sessions/projects
and any future binary `.gpml` alternative that mixes general-path structure with raw bulk leaves.

Measured gain for `RotationModel(rotations.rot)` pickling: **~7.5× on save, ~4.7× on load**
(151→20 ms, 159→35 ms; see [performance.md](performance.md) for the full progression and why the
~10×/~7–11× target wasn't fully reached for this particular, structurally heterogeneous fixture).
For homogeneous point geometry — the phase 6 bulk-array target — the gain is much larger (see
below).

## Why this design

- One `GpmlTimeSample` costs ~12–16 tracked object ids, ~3 polymorphic class-name *string
  objects*, and ~30–40 scribe calls on the general path; `rotations.rot` ≈ 2,331 samples plus a
  ~500-feature skeleton ⇒ ~40–55k transcription objects and ~100k+ scribe calls per pickle.
  Nothing on that path is bulk — even geometry writes three tracked `Real` objects per point.
- The parked tracking rework (`backup/pickle-perf-rewritten`) proved bookkeeping-only changes cap
  out at ~1.3× — the order-of-magnitude goal requires not creating the per-object records at all.
- Whole-subtree raw for pickles (rather than raw only at bulk leaves) removes ~95% of the
  machinery instead of ~70%, and `RotationModel`/`FeatureCollection` have little truly homogeneous
  bulk data, so per-element machinery removal — not columnar layout — is where the win comes from.
- Structure-of-arrays / per-column tags were considered and deferred: general path at structural
  levels + raw sections at lower levels already emulates "one tag per column" for formats that need
  tag compatibility (a future volume format, binary `.gpml`), and the raw blob layout is
  scribe-owned, so a columnar layout can be introduced later behind a codec version bump without
  touching client code.

## Decisions

1. **Raw lane as a switchable mode, same handlers** — no duplicate serialization code per type.
2. **Whole-subtree raw for pickles**: `PythonPickle`'s `Transcribe<Holder>` hook saves the holder
   with `RAW` (`PythonPickle.h:101`), so the entire pickle payload is one blob.
3. **Order-defined, version-gated raw sections** (no per-field tag skipping inside a section).
   Three versioning layers:
   - **Codec version** (scribe-owned): `Scribe::CURRENT_RAW_STREAM_CODEC_VERSION` (`Scribe.h:1573`),
     a varint at every blob head (`stream_raw_boundary`, `Scribe.h:5683`); covers value encodings,
     pointer markers, the backref scheme. Layout evolution (e.g. columnar) happens behind this,
     invisible to clients. A future codec version is rejected on load with
     `Exceptions::UnsupportedRawStreamVersion` (`ScribeExceptions.h:380`).
   - **Section version** (client-owned): written at the section front by the entry-point client —
     `PythonPickle` owns `CURRENT_PICKLE_SECTION_VERSION` (`PythonPickle.h:59`, tag
     `"pickle_section_version"`), gated on load by `is_in_transcription` so pre-raw pickles (which
     lack it) still load via the `COMPOSITE` fallback. The coarse gate for unanticipated layout
     changes.
   - **Opt-in per-type version ints**: a handler expecting evolution may transcribe its own version
     int at a location of its choosing (works identically in both lanes; ~1 byte per instance).
     Boost::serialization-style *automatic* per-type versions remain a documented future upgrade
     behind a codec bump — not implemented.
4. **Compatibility guarantee**: new pyGPlates always loads old pickles — on load, the boundary
   object dispatches on its transcription kind (`COMPOSITE` → general path, `RAW_STREAM` → raw
   lane; see `stream_raw_boundary`, `Scribe.h:2914`). Old pyGPlates reading a new (raw) pickle
   fails with a clean `UnsupportedVersion` (`PythonPickle.h:84` converts any load failure), not
   garbage. Pickles are transient today (multiprocessing), but dumping them to files is supported
   under these guarantees.

## Design

### The `RAW_STREAM` transcription kind

- `RAW_STREAM` (`Transcription.h:242`), added to `ObjectType` before `UNUSED` (which must stay
  last — it is the hole marker and default). Storage is `std::vector<std::vector<char>>` (keeps
  `Transcription` Qt-free) with `add_raw_stream` / `get_raw_stream` / `get_num_raw_stream_objects`
  (`Transcription.h:369`, `:359`, `:380`) mirroring the string methods.
- `operator==` (`Transcription.cc:754`) gets a bytewise case. This is bit-exact, while `FLOAT`/
  `DOUBLE` compare with tolerance elsewhere — irrelevant for pickling; would need attention if
  sessions ever adopted raw (change-detection would get stricter).
- `add_string`/`get_or_create_unique_string_index` (`Transcription.h:351`) is shared machinery: the
  raw lane interns strings and pointee class names into the *existing* unique-string pool and
  stores varint indices. Archive ordering already handles this correctly — the binary writer emits
  the pool before objects, the reader populates it before parsing objects.

### Archive layer

- `RAW_STREAM_CODE = 6` (`ScribeArchiveCommon.h:112`); the binary writer/reader case is a varint
  length + `QDataStream::writeRawData`/`readRawData`
  (`ScribeBinaryArchiveWriter.cc:245`, `ScribeBinaryArchiveReader.cc:187`).
- **Conditional format-version bump**: `BINARY_ARCHIVE_FORMAT_VERSION_RAW_STREAM = 1`
  (`ScribeArchiveCommon.h:91`) is written instead of the normal version 0 only when the
  transcription actually contains a raw stream (`ScribeBinaryArchiveWriter.cc:88`), so non-raw
  archives (sessions, projects) keep writing version 0 and stay readable by old builds. Old
  readers hitting version 1 fail cleanly with `UnsupportedVersion` instead of an unknown-type-code
  `ArchiveStreamError`.
- Text/XML archives base64-encode the blob (deprecated / unit-test-only formats; low effort, not
  perf-critical).

### Raw mode inside `Scribe`

- `RAW = 1 << 3` (`ScribeOptions.h:80`, chosen so it composes with the existing option bits).
  `Scribe::RawStreamScope` (`Scribe.h:1697`) is an RAII guard: on construction it asserts raw scopes
  don't nest (`!d_is_raw`; the `RAW` option is ignored on transcribe calls made *inside* an
  already-open raw subtree — everything there already streams into the enclosing stream), resets
  `d_raw_context` (`Scribe::RawContext`, `Scribe.h:1603`) and sets `d_is_raw = true`; the destructor
  always restores `d_is_raw = false` and releases the context, even if an exception unwinds through
  the subtree.
- **The boundary object transcribes normally** (object id, tag, `pre_transcribe`/`post_transcribe`
  — preserving `is_transcription_complete` and stack invariants). Only the *stream* step differs:
  `stream_raw_boundary` (`Scribe.h:2914`) streams the subtree into a byte buffer (save) or decodes
  it (load, dispatching on the boundary object's transcription kind — `RAW_STREAM` uses the raw
  lane, anything else falls back to the general path for old-format compatibility), then binds the
  buffer to the current object id via `TranscriptionScribeContext::add_raw_stream`, exactly like the
  `std::string` primitive case (same `PRIMITIVE` category, no new `ObjectCategory`).
- **Branch points** — a single predictable `if (d_is_raw)` each — route the same handlers into the
  raw codec instead of the general path: `transcribe_object`/`transcribe_construct_object`
  (`Scribe.h:3478`, `:3745`, `:3847`), `stream(..., StreamPrimitiveTag)` (essential: the delegate
  protocol calls `stream_object` directly, bypassing the other funnels), `transcribe_smart_pointer_object`
  and `transcribe_base_object` (void-cast registration still runs in raw mode — needed for backref
  `up_cast` on load — only id/list bookkeeping is skipped), and `relocated` (`Scribe.h:4828`,
  `:4882`), which is a silent no-op *before* the tracked-map lookup that would otherwise throw
  `RelocatedUntrackedObject`. This one branch alone makes the sequence, mapping and
  `boost::optional` protocols correct **unchanged** in raw mode — their per-element `TRACK` +
  `relocated()` calls become harmless (and, per the phase-5 note below, are now skipped outright as
  a further optimisation, since they're dead work in the raw lane). Object references and
  non-owning pointers throw `InvalidRawTranscribeOperation` (`ScribeExceptions.h:464`) — verified
  unused on the pickle graph (only session code uses them).
- `LoadRef` destruction is raw-safe unconditionally: `Scribe::untrack_object` silently returns when
  an address isn't tracked — no throw, no double-free.
- Raw load failures **throw** rather than returning a soft `TranscribeResult` (unknown class name,
  cursor overrun, version mismatch) — a raw stream is positional, so decoding cannot re-synchronise
  after a failure. This matches the existing pickle contract: `PythonPickle.h:84` already converts
  any load failure (soft or thrown) to `UnsupportedVersion`.

### The raw codec

Values are encoded positionally — no per-object ids, tags or type slots — via
`transcribe_raw(...)` overloads (`Scribe.h:3023`) mirroring `TranscriptionScribeContext`'s general
primitive set:

- **Integers** use a canonical, type-independent encoding (`transcribe_raw_integer`,
  `Scribe.h:2994`): a sign-discriminator byte followed by a (zig-zag, if signed) varint
  (`write_raw_signed_integer`/`write_raw_unsigned_integer`/`read_raw_integer` returning
  `RawInteger{is_signed, ...}`, `Scribe.h:2956`–`2980`). Decoding uses the *save-side* signedness
  (recorded in the stream) and `numeric_cast`s to the load-side type — mirroring the general path's
  int↔unsigned tolerance, so a value saved through one integral type (e.g. `int`) can be loaded
  through another (e.g. `unsigned long`) without the caller having to keep them in sync. 64-bit
  integers are written full-width — the general path instead throws on `long long` outside 32-bit
  range, a deliberate, documented divergence.
- **`bool`**: one byte. **Floating point** (`transcribe_raw_fixed_width`, `Scribe.h:3000`): fixed-
  width little-endian memcpy, `float`/`double` each through their own overload — deliberately
  *not* self-describing like the integer codec (see the "Float/double stayed fixed-width" note
  below): a value **must** be saved and loaded through the same type in the raw lane.
- **Strings**: a varint index into the transcription's existing unique-string pool
  (`get_or_create_unique_string_index`) — keeps interning for repeated names (e.g.
  `QualifiedXmlName` triples).
- **Bulk arithmetic arrays**: `Scribe::transcribe_raw_array(source, T *, count, tag = ObjectTag())`
  (`Scribe.h:395`) — a single `write_raw_bytes`/`read_raw_bytes` memcpy of `count` contiguous,
  already-known-length values (`Scribe.h:3195`), for when per-element `transcribe_raw` calls would
  themselves become the bottleneck (see "Bulk arrays and geometry" below). Same same-type-on-both-
  sides constraint as the scalar float/double overloads; `count` is not itself written/read (both
  sides already know it). `BOOST_STATIC_ASSERT(boost::is_arithmetic<ArithmeticType>::value)` catches
  a call with a non-arithmetic element type at compile time.
- All of it is little-endian only (a `#error` guards big-endian targets in `Scribe.cc`).
- Raw stream decoding cannot fail softly and re-synchronise (unlike the general path, which can
  skip a malformed field) — any inconsistency throws `Exceptions::RawStreamError`
  (`ScribeExceptions.h:425`), including reading past the end of the stream or an invalid
  unique-string-pool index.

### Owning pointers and shared-owner dedup (backrefs)

Owned pointers stream their pointed-to object inline — there are no object ids inside a raw stream
to link a pointer to a separately transcribed object — via a marker byte, `RawPointerMarker`
(`Scribe.h:1584`): `RAW_POINTER_NULL`, `RAW_POINTER_INLINE` (sole owner, cannot be back-referenced),
`RAW_POINTER_SHARED` (first owner: streams the object and registers it), `RAW_POINTER_SHARED_BACKREF`
(+ varint index into the encounter order of `RAW_POINTER_SHARED` objects). Polymorphic pointee
class names are a varint unique-string-pool index, with a per-`Scribe` `RawContext` cache
(`save_export_class_types`/`load_export_class_types`, `Scribe.h:1665`, `:1674`) avoiding an
`ExportRegistry` lookup and string-pool search per pointed-to object.

This preserves aliasing (shared `XmlElementNode` alias maps, `GmlFile` attribute maps) and prevents
the exponential re-write blowup that produced the parked tracking rework's pathological 13-second
load (see [performance.md](performance.md)):

- An optional `use_count_hint` threads through `transcribe_smart_pointer_protocol`
  (`TranscribeSmartPointerProtocol.h:65`) — the refcount isn't reachable at `Scribe` level, since
  callers strip smart pointers to a raw `T *` before the protocol call. The `boost::shared_ptr`
  overload deliberately passes **no hint** (see below); intrusive-pointer overloads supply
  `get_reference_count()` via a SFINAE hook, `get_intrusive_use_count_hint`
  (`TranscribeSmartPointerProtocol.h:83`); unknown → always-dedup (the conservative default).
- `hint == 1` (the common case) skips the dedup map entirely and writes `RAW_POINTER_INLINE`.
  `hint > 1` uses `RAW_POINTER_SHARED`/`RAW_POINTER_SHARED_BACKREF` via a save-side
  address→index map (`RawContext::save_shared_object_indices`, `Scribe.h:1629`, keyed by *dynamic*
  address-and-type so an object and its first data member at the same address aren't conflated) and
  a load-side encounter-ordered vector (`RawContext::load_shared_objects`, `Scribe.h:1660`) —
  deterministic, since load replays save order — resolved through `d_void_cast_registry.up_cast`
  for backref up-casting. The existing `d_shared_ptr_map` control-block sharing works unchanged in
  raw mode (keyed by dynamic address, not object id).
- **Deliberate deviation from `boost::intrusive_ptr`: `boost::shared_ptr` passes *no* hint at all**
  (always conservative always-dedup), not `use_count()`. `use_count()` excludes `weak_ptr`s, so a
  `weak_ptr` to a count-1 `shared_ptr`, transcribed later in the same subtree, would see a stale
  hint of 1 and duplicate the object instead of aliasing it. Always-dedup instead relies on
  `d_shared_ptr_map`, which already handles shared/weak aliasing correctly.
- `reserve_raw_shared_object_on_load()`/`set_raw_shared_object_on_load(idx, ...)`
  (`Scribe.h:2334`, `:2338`) reserve a backref slot for a shared object *before* loading its
  contents (pre-order) and fill it in afterwards — matching save order, where a shared object is
  registered before its contents are streamed, so a nested child gets a *higher* backref index than
  its parent. This ordering mismatch (register-after vs register-before) was the source of one of
  the three bug classes phase 4 surfaced when the lane went live for real object graphs — see
  [performance.md](performance.md)'s phase-4 note; it's also the actual fix for the pathological
  shared-alias-map load blowup, since a stable pre-order backref index makes shared subtrees stream
  once instead of once per alias.
- New virtual `TranscribeOwningPointer::save_object_raw`/`load_object_raw`
  (`ScribeInternalUtils.h`; `load_object_raw` returns `void *`, or `NULL` on soft failure) — the
  existing `load_object` returns the heap address only through `ObjectInfo`, which doesn't exist in
  raw mode.

### Float/double stayed fixed-width (deliberate, not an oversight)

Unlike integers, `float`/`double` did **not** get the self-describing (discriminator-byte)
treatment. The general path *does* support the full int↔float↔double cross-matrix (a per-object
type tag + `numeric_cast`s on load, including Inf/NaN handling for narrowing double→float), but the
raw lane requires a value be saved and loaded through the *same* floating-point type. Rationale: no
handler in this codebase actually crosses float types on save vs load (unlike the real
`XsInteger`-style int/`unsigned long` case that motivated the integer codec); raw payloads are
heavily double-dominated, so a self-describing form costs storage and speed on the hot path for a
hypothetical case; and a future mismatch fails loudly — a 4-byte write read back as 8 bytes
desyncs the cursor and throws `RawStreamError` immediately — rather than silently. If a genuine
cross-type float handler ever appears, the fix is to bump `CURRENT_RAW_STREAM_CODEC_VERSION` and
extend the codec, the same escape hatch used for every other layout change.

### Bulk arrays and geometry

`transcribe_raw_array` (above) exists because per-element `transcribe_raw` calls are themselves a
measurable per-call cost once an array is large and homogeneous — the residual the phase-5 hot-path
trims (below) could not remove, since it's proportional to element count, not per-object overhead.
Adopted at:

- `PolylineOnSphere::transcribe` and `MultiPointOnSphere::transcribe` — flatten
  `vector<PointOnSphere>` to 3 doubles/point (`x/y/z().dval()` of the unit vector) and back.
- `PolygonOnSphere::transcribe` — the same flatten/unflatten, applied per-ring (exterior plus each
  interior ring).
- The `T (&)[N]` free-function protocol in `TranscribeArray.h`, for arithmetic-element fixed-size
  C arrays (including the arithmetic leaf of a multidimensional array) — dispatched at compile time
  (`boost::mpl::eval_if<boost::is_arithmetic<T>, ...>`) so `transcribe_raw_array<T>`'s
  `BOOST_STATIC_ASSERT` is never instantiated for a non-arithmetic element type.

In every adopter the general (non-raw) path is left byte-for-byte unchanged in an `else` branch —
these types are also transcribed by GPlates sessions/projects, which never pass `RAW`.

### Hot-path trims beyond the codec itself

Two further, profiling-driven changes (phase 5) cut per-call overhead that has nothing to do with
the codec above but dominated measured time once the codec itself was fast:

- **`const char *` overloads of `transcribe`/`save`/`load`**: nearly every call site passes a
  string-literal tag, which — because the entry signatures take `const ObjectTag &` — builds a
  heap-allocated `ObjectTag` (a `std::vector<Section>`) *caller-side*, before `Scribe` ever sees
  `d_is_raw`. Non-template `const char *` overloads check `d_is_raw` first and build nothing in raw
  mode; overload resolution picks them for a string literal (array-to-pointer decay beats the
  user-defined `ObjectTag` conversion) with zero call-site changes. This was the single largest
  contributor to the phase-4→phase-5 improvement.
- **Raw branches in the sequence and mapping protocols**, gated on the public
  `Scribe::is_transcribing_raw()` accessor: items stream positionally, so there are no per-element
  tags to build, and — per the `relocated()` no-op above — the load-side relocate step is dead work
  that can be skipped outright rather than merely made harmless.

In-place/on-stack item construction on load was investigated and **reverted**: it measured
negligible (<0.5 ms) gain, meaning per-item heap allocation was not the dominant residual load
cost — not worth the extra public API and lifetime surface for no measurable benefit.

## Constraints for anyone touching this code

- **No references or non-owning pointers inside a raw section.** Both require object tracking to
  resolve later, which the raw lane omits entirely; both throw
  `InvalidRawTranscribeOperation` immediately rather than silently mis-encoding.
- **No probing/skipping inside a raw section.** The general path's random-access `Transcription`
  lets a handler ask for a field that isn't there (skip forward/backward compatibility). A raw
  stream is a flat byte sequence with no addressing scheme — every `transcribe()` call inside a raw
  boundary must execute in the *exact same order* on load as it did on save, for every version of
  the code that will ever load that stream. This is why raw sections are versioned as an opaque
  unit (the codec/section/per-type version layers above) rather than field-by-field like the
  general path's tags.
- **Evolution discipline**: adding a field inside an existing raw section is a breaking change to
  that section's binary layout — bump the owning section's version (or the relevant per-type
  version int) and branch on it at load time, exactly as `PythonPickle`'s
  `pickle_section_version` does. Do not add or reorder `transcribe()` calls inside an existing raw
  boundary without a version bump; do that and old pickles silently decode garbage instead of
  failing cleanly.
- **Same-type-both-sides for floating point** (see above) and **do not add a cross-type float
  handler without bumping the codec version** first.
- **`transcribe_raw_array` element type must be arithmetic** (compile-time enforced) — routing a
  class-type array/container through it does not compile; use the general per-element path (or add
  a bespoke flatten/unflatten, as the geometry types do) instead.
- **The general (non-raw) path must stay byte-for-byte unchanged** wherever both lanes coexist in
  one `transcribe()` (every adopter above): GPlates sessions and project files use the same
  handlers and must never observe a difference.

## Verification

- **Unit tests** (`src/unit-test/TranscribeTest.cc`): raw round-trips of primitives, 64-bit
  integers, string interning, `boost::optional` none/some, sequences and maps, owned polymorphic
  pointers, intrusive dedup (refcount > 1), `shared_ptr` aliasing (`XmlElementNode`-style shared
  maps), NULL pointers, nested-`RAW` (ignored), fixed-size arrays (both directly via
  `transcribe_raw_array` and through `TranscribeArray.h`), codec- and section-version rejection,
  old-format (`COMPOSITE`) fallback, and errors for references/non-owning pointers in raw mode.
- **Full pyGPlates suite**: all 439 tests, including ~49 `test_pickle` cases.
  `test_pickle_property_values_not_in_pygplates` — the parked rework's pathological shared-alias-map
  case — round-trips in well under a millisecond, confirming the backref pre-order fix.
- **Cross-version**: a pre-raw pickle loads via the `COMPOSITE` fallback; a raw pickle presented to
  a format-version-0 reader raises a clean `UnsupportedVersion`.
- **GPlates-side safety**: sessions/projects never pass `RAW`; a non-raw binary archive still
  writes format version 0 byte-identically.

See [performance.md](performance.md) for the full phase-by-phase history, the bugs each phase
surfaced, and the final benchmark numbers.
