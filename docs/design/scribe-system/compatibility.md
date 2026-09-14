# Errors and Backward/Forward Compatibility

*Part of the [Scribe system design docs](README.md). This chapter absorbs most of the retired
`src/scribe/DesignRationale.txt`.*

## Two failure channels

The system deliberately separates two kinds of failure:

1. **Exceptions** (`ScribeExceptions.h`) signal programmer errors, misuse of the scribe API, or a
   corrupt/wrong archive. They are thrown to (a) avoid generating an erroneous archive and (b) let
   the programmer fix their usage. They should **not** be caught *during* transcribing — a thrown
   exception means the transcribe state is indeterminate and the whole operation (e.g. the project
   load) must be aborted. Callers catch `Exceptions::BaseException` *around* the whole save/load
   (e.g. `src/gui/FileIOFeedback.cc`).
2. **Return codes** (`TranscribeResult`, checked via the must-check `Bool` returned by
   `transcribe()`) signal that an object in an otherwise valid transcription could not be read.
   This is the *recoverable*, per-object channel used for version compatibility: the caller can
   supply a default value and continue, or propagate the failure up. If it propagates to the root,
   the load fails as a whole (sessions then throw `TranscribeSession::UnsupportedVersion`).

## `TranscribeResult` (`TranscribeResult.h`)

- **`TRANSCRIBE_SUCCESS`**.
- **`TRANSCRIBE_INCOMPATIBLE`** — the object's tag name/version was not found in the parent's
  scope, or a primitive had the wrong type (e.g. transcription holds a string where an integer was
  requested). Non-primitive object *types* are mostly not recorded in the transcription, so
  incompatibility detection rests on tag matching.
- **`TRANSCRIBE_UNKNOWN_TYPE`** — an export-registered class name, enum value, or variant/any
  member type in the transcription is unknown to this version of GPlates. Only detectable where
  type names are actually written: polymorphic (owning) pointers, enums, variants/`boost::any`.

Why two failure codes? Forward compatibility. When a future version adds a new *data member*, the
old version never looks up the new tag and is unaffected. But when a future version adds a new
*derived type* into a heterogeneous sequence (e.g. `std::vector<boost::shared_ptr<Base>>`),
sequence elements have no per-element tag the old version could ignore. Distinguishing
`TRANSCRIBE_UNKNOWN_TYPE` lets sequence loaders drop just the unknown elements and keep the rest,
instead of failing the whole sequence.

## How forward compatibility works: ignore what you don't ask for

Because the loaded `Transcription` is random-access
(see [transcription-and-archives.md](transcription-and-archives.md)), an old version reading a
newer archive simply *never queries* objects it doesn't know about — they are skipped without ever
being noticed. There is no need to version whole archives when data is added; unused objects (and
whole subtrees) are inert.

Consequently, explicit versioning is only needed when a *change breaks* an existing tag's meaning.
The levers, from finest to coarsest:

| Lever | Where | When to use |
|---|---|---|
| **Object tag name/version** | per `transcribe()` call (`ScribeObjectTag.h`) | Change a member's representation: bump the tag version (or use a new name). Old versions fail to find the new tag → `TRANSCRIBE_INCOMPATIBLE` → their fallback path; new versions can probe for the old tag (`is_in_transcription()`) to load legacy archives. |
| **Export class-id name strings** | `ScribeExportRegistration.h` | Never rename once released (`ScribeExportRegistration.h:128`); keep the old string even if the C++ class is renamed. Unknown names → `TRANSCRIBE_UNKNOWN_TYPE`. |
| **Enum value names** | `TranscribeEnumProtocol.h` | Enums are transcribed as registered strings, so reordering/renumbering C++ enum values is safe; renaming is not. Unknown names → `TRANSCRIBE_UNKNOWN_TYPE`. |
| **Scribe version** | `Scribe::CURRENT_SCRIBE_VERSION` (`Scribe.h:1514`, currently 0) | Reserved for changes to the scribe library's own conventions. |
| **Archive format versions** | `ScribeArchiveCommon.h:69`–`:87` (all currently 0) | Reserved for byte-level encoding changes, independent of object-network changes. |

### Container interchangeability

All sequence containers (`std::vector`, `std::list`, `std::set`, `QList`, `QSet`, …) share one
transcribe protocol (`TranscribeSequenceProtocol.h`), so the container type can change between
versions without breaking archives. Caveat from the original design notes: loading a saved
`std::vector` with duplicates into a `std::set` silently collapses the duplicates — usually fine,
but a problem if a transcribed pointer referenced one of the dropped duplicates.

## Exception hierarchy overview (`ScribeExceptions.h`)

All derive from `GPlatesScribe::Exceptions::BaseException`. Grouped by cause:

- **Archive/stream**: `InvalidArchiveSignature`, `UnsupportedVersion` (archive format or scribe
  version newer than this build), `ArchiveStreamError`, XML-specific parse/element errors.
- **API misuse** (`ScribeUserError` family): `ScribeTranscribeResultNotChecked` (a `Bool` was
  dropped unchecked), `InvalidTranscribeOptions`, `ConstructNotAllowed`,
  `TranscribedReferenceInsteadOfObject`, `TranscribedUntrackedPointerBeforeReferencedObject`,
  `TranscribedReferenceBeforeReferencedObject`, `AlreadyTranscribedObject(WithoutOwningPointer)`,
  `UntrackingObjectWithReferences`, the `Relocated*` family (e.g.
  `RelocatedObjectBoundToAReferenceOrUntrackedPointer`, `RelocatedUntrackedObject`).
- **Consistency**: `TranscriptionIncomplete` (asserted before writing and when constructing a
  loading scribe), `TranscriptionIncompatible`.
- **Registries**: `UnregisteredClassType` (saving an unregistered polymorphic type — a hard error,
  unlike the load-path `TRANSCRIBE_UNKNOWN_TYPE`), `UnregisteredCast`, `AmbiguousCast`,
  `UnregisteredEnumValue`, `ExportRegisteredMultipleClassTypesWithSameClassName` (and its inverse),
  `UnregisteredQVariantMetaType`.

## Why not `boost::serialization`

Scribe is deliberately very close to `boost::serialization` in concept; it was written in-house
for three reasons (preserved from the original design rationale):

1. **Version coupling.** boost archives embed the *boost serialization library* version. GPlates
   only requires "boost ≥ some minimum", so the same GPlates release built against different boost
   versions (different Linux distros, different developers' macOS bundles) would produce mutually
   unreadable archives. With Scribe, the serialization version maps one-to-one to the GPlates
   version regardless of the boost used to compile.
2. **Backward-compatibility bugs.** boost::serialization had long-standing binary-archive
   compatibility bugs (boost ticket #4660, still recurring around boost 1.51 when the decision was
   made), plus reported text-archive issues.
3. **External context for load-construction.** boost's free-function `load_construct_data()` makes
   it awkward to pass in objects that were *not* serialized (partial serialization) — e.g.
   constructing `class X { X(const Y &); }` when `Y` is application state. Scribe solves this
   with the specializable `TranscribeContext<T>` pushed onto a per-type stack
   (see [architecture.md](architecture.md)).

## Status of the old `DesignRationale.txt` TODOs

The retired document framed several mechanisms as TODO/aspirational; all are long since
implemented: `TRANSCRIBE_UNKNOWN_TYPE` handling (`Scribe.cc:453`), the void-cast registry
(`ScribeVoidCastRegistry.h`), and the `TranscribeContext` push/pop stack (`Scribe.h:1006`).
Its remaining substance — the two failure channels, the derived-object-id rationale for the
void-cast registry, container interchangeability, and the boost comparison — is captured above and
in [tracking-and-pointers.md](tracking-and-pointers.md).
