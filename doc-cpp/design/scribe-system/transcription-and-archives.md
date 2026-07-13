# The `Transcription` IR and the Archive Formats

*Part of the [Scribe system design docs](README.md).*

## `Transcription`: the intermediate representation

`GPlatesScribe::Transcription` (`src/scribe/Transcription.h:58`, reference-counted via
`non_null_ptr_type`) holds the transcribed state of the whole object network in a **random-access**
form. This sits between the `Scribe` (which reads/writes it via `TranscriptionScribeContext`) and
the archives (which read/write it directly, sequentially). Random access is the key property:
loading code looks objects up by tag, and anything it never asks for is simply skipped — the basis
of forward compatibility.

### Object model

- Every object has an **`object_id_type`** (`unsigned int`, `Transcription.h:72`).
- An object is one of the **`ObjectType`s** (`Transcription.h:234`): `SIGNED_INTEGER`,
  `UNSIGNED_INTEGER`, `FLOAT`, `DOUBLE`, `STRING`, `COMPOSITE` — or `UNUSED` for id holes
  (ids that exist in the id space but were never populated, e.g. skipped data).
- `d_object_locations` (`Transcription.h:551`) maps object id → `{ObjectType, index}`, where the
  index points into one of the typed storage vectors:
  - `d_signed_integer_objects` / `d_unsigned_integer_objects` — 32-bit (`boost::int32_t` /
    `uint32_t`, `Transcription.h:250`); wider integers are transcribed as multiple primitives by
    the scribe layer.
  - `d_float_objects` / `d_double_objects`.
  - Strings are **interned**: `d_unique_string_objects` holds each distinct string once,
    `d_string_objects` maps a string *object* to an index into the unique pool, and
    `d_string_object_index_map` (a `std::map<std::string, unsigned>`) provides save-path dedup
    (`Transcription.h:560`–`:562`).
- **Object tag names are also interned**: `d_object_tag_names` + `d_object_tag_name_id_map`
  (`Transcription.h:547`) map tag-name strings to integer `object_tag_name_id_type`s. An
  **object key** (`object_key_type`, `Transcription.h:93`) is the pair
  `(tag_name_id, tag_version)`.

### `CompositeObject`

A composite object (`Transcription.h:101`) maps object keys → one or more child object ids
(multiple children per key model sequence items). Its storage is deliberately compact: everything
lives in a single packed `std::vector<unsigned int> d_encoding` (`Transcription.h:227`) — for each
key, three header ints (`tag_id`, `tag_version`, `num_children`; offsets defined at
`Transcription.h:214`–`:219`) followed by the child object ids. Key lookup (`find_key`,
`Transcription.h:193`) is a linear scan over this array, which is fine for typical objects with a
handful of keys (but see [performance.md](performance.md)).

Composites are allocated from a `boost::object_pool` (`d_composite_object_pool`,
`Transcription.h:534`) so that growing the `d_composite_objects` vector doesn't copy the inner
encoding vectors.

Children can be set out of order, leaving *holes* that must be filled before the transcription is
complete (`set_child` documentation at `Transcription.h:157`) — this supports transcribing
sequence elements at arbitrary indices.

### Completeness and comparison

- `is_complete(null_pointer_object_id, emit_warnings)` (`Transcription.h:484`) verifies every
  referenced object id actually exists. The loading `Scribe` constructor asserts this; the save
  path checks it inside `Scribe::is_transcription_complete()`.
- `operator==` (`Transcription.h:501`) compares two transcriptions structurally. Two
  transcriptions only compare equal if produced by the same code path in the same order — which is
  exactly the use case: `ProjectSession::has_session_state_changed()` re-transcribes the current
  session state and compares against the last-saved transcription to detect unsaved changes.

## `TranscriptionScribeContext`: the Scribe⇄Transcription adapter

`TranscriptionScribeContext` (`src/scribe/TranscriptionScribeContext.h:52`) is how `Scribe`
reads/writes a `Transcription` without knowing its storage details. It holds the transcription,
the save/load direction, the next save object id, and a stack of "currently being transcribed"
objects so that tag lookups are always relative to the enclosing (parent) object:

- Reserved ids (`TranscriptionScribeContext.h:67`, `:74`):
  `NULL_POINTER_OBJECT_ID = 0` (what a transcribed NULL pointer points to) and
  `ROOT_OBJECT_ID = 1` (a pseudo-object that parents all root-level transcribe calls).
- `allocate_save_object_id()` (`:115`) hands out ids on the save path.
- `transcribe_object_id(object_id, object_tag)` (`:137`) writes/reads a child object id under a
  tag in the current parent's composite — the load-path failure point that produces
  `TRANSCRIBE_INCOMPATIBLE`.
- `push_transcribed_object(object_id)` / `pop_transcribed_object()` (`:154`) maintain the parent
  scope.
- `transcribe(...)` overloads for `std::string`, `bool`, all character/integer types, and
  `float`/`double`/`long double` (`:168`–`:241`) store/fetch primitive values for the current
  object id. Everything satisfying `boost::is_arithmetic` is routed here directly by `Scribe`,
  bypassing the general dispatch.

## Archives

Abstract interfaces:

- `ArchiveWriter` (`ScribeArchiveWriter.h`): `write_transcription(const Transcription &)` +
  `close()`.
- `ArchiveReader` (`ScribeArchiveReader.h`): `read_transcription()` →
  `Transcription::non_null_ptr_type` + `close()`.

**Multiple transcriptions can be written to / read from one archive consecutively.** Project files
use exactly this: a metadata transcription followed by the session-data transcription, so the
metadata can be peeked without loading the whole project (see [usage.md](usage.md)).

Shared constants live in `ArchiveCommon` (`ScribeArchiveCommon.h`): per-format signature strings,
per-format version numbers, and integer type codes for the object kinds
(`SIGNED_INTEGER_CODE = 0` … `COMPOSITE_CODE = 5`, `ScribeArchiveCommon.h:93`).

### Binary archive (the production format)

`ScribeBinaryArchiveWriter` / `ScribeBinaryArchiveReader`. Used by project files, internal
sessions, and pyGPlates pickling — all over a `QDataStream` fixed at version
`QDataStream::Qt_4_4`, little-endian (`ScribeArchiveCommon.h:129`, `:136`).

- **Header**: the signature `"GPlatesScribeBinaryArchive"` (`ScribeArchiveCommon.h:53`), written
  char-by-char so a wrong file type is detected before any varint decoding, then the binary
  archive format version (currently 0) and the scribe version.
- **Integers are varint-encoded** (protobuf-style, 7 bits per byte with a continuation bit —
  `BinaryArchiveWriter::write(quint32)`, `ScribeBinaryArchiveWriter.cc:296`), and signed integers
  are zig-zag mapped to unsigned first (`write(qint32)`, `:264`) so small magnitudes stay small.
- **Per transcription**: the tag-name pool, the unique-string pool, then the objects grouped into
  contiguous runs of used object ids (so ids need not be written per object), each object as a
  type code + value; composites as key/child-id lists; terminated by a zero-count group.

### Text archive

`ScribeTextArchiveWriter/Reader`, signature `"GPlatesScribeTextArchive"`. Human-readable tokens
over `std::ostream`/`istream`, with special tokens for `inf`/`-inf`/`nan`
(`ScribeArchiveCommon.h:228`). **Production use**: only the deprecated GPlates 1.5 session format —
`InternalSession.cc` writes/reads the 1.5-compatible session state as a text archive stored in
user preferences (`src/presentation/InternalSession.cc:139`, `:355`).

### XML archive

`ScribeXmlArchiveWriter/Reader`, signature `"GPlatesScribeXmlArchive"`. Root element
`scribe_serialization` carrying signature/format-version/scribe-version attributes; groups for tag
names, strings and objects; per-object elements `signed`/`unsigned`/`float`/`double`/`string`/
`composite` with `oid` attributes (element names at `ScribeArchiveCommon.h:103`–`:117`).
**Not used in production** — only exercised by `src/unit-test/TranscribeTest.cc`.

### Versioning layers

Three independent version numbers travel with the data (see also
[compatibility.md](compatibility.md)):

1. **Archive format version** per format (`TEXT/BINARY/XML_ARCHIVE_FORMAT_VERSION`, all currently
   0, `ScribeArchiveCommon.h:69`–`:87`) — covers the byte-level encoding only.
2. **Scribe version** (`Scribe::CURRENT_SCRIBE_VERSION = 0`, `Scribe.h:1514`) — covers the
   scribe library's own transcription conventions.
3. **Object tag versions** (per tag, in the transcription itself) — the fine-grained
   compatibility lever for client data.
