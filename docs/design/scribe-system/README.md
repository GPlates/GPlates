# The Scribe System

*Design documentation for the GPlates/pyGPlates serialization framework in `src/scribe/`.*

*This document set supersedes the old (incomplete) `src/scribe/DesignRationale.txt`, whose content
has been folded into these chapters (mostly [compatibility.md](compatibility.md) and
[tracking-and-pointers.md](tracking-and-pointers.md)).*

## What Scribe is

Scribe is GPlates' custom object-serialization framework, living in the `GPlatesScribe` namespace.
Its word for serializing/deserializing is **transcribing**: a `GPlatesScribe::Scribe` object is
constructed in either *saving* or *loading* mode, and the same client `transcribe()` code runs in
both directions (branching on `scribe.is_saving()` where the two directions genuinely differ).

It was written instead of adopting `boost::serialization` for reasons that are still relevant —
principally that the archive version must track the GPlates version one-to-one regardless of which
boost version GPlates was compiled with (see [compatibility.md](compatibility.md) for the full
rationale).

## The two-stage pipeline

Everything in the system flows through the same two stages:

```
                transcribe                       archive read/write
  C++ object graph  ⇄  Transcription (in-memory IR)  ⇄  byte stream (binary / text / XML)
        stage 1: Scribe + client            stage 2: ArchiveWriter /
        transcribe() handlers               ArchiveReader
```

1. **Object graph ⇄ `Transcription`.** A `Scribe` drives `transcribe()`/`save()`/`load()` calls
   through the object network. Saving builds an in-memory `GPlatesScribe::Transcription`; loading
   consumes one. Types opt in by implementing a `transcribe()` method (befriending
   `GPlatesScribe::Access`), plus a static `transcribe_construct_data()` if they lack a default
   constructor. The `Transcription` is a *random-access* structure — this is what makes
   backward/forward compatibility work, because data the current version never asks for is simply
   skipped.
2. **`Transcription` ⇄ bytes.** An `ArchiveWriter`/`ArchiveReader` serializes the `Transcription`
   to/from a stream. Three formats exist (binary, text, XML); all production paths use the
   **binary** format except the deprecated GPlates 1.5 session format, which uses text.

## Chapters

| Chapter | Contents |
|---|---|
| [architecture.md](architecture.md) | The `Scribe` class, its public API, client customization points (`transcribe()`, `transcribe_construct_data()`, `relocated()`), `ObjectTag`, the save/load-construct machinery, `TranscribeContext`, and a walk-through of a transcribe call |
| [transcription-and-archives.md](transcription-and-archives.md) | The `Transcription` intermediate representation (object IDs, string/tag pools, `CompositeObject` encoding), `TranscriptionScribeContext`, and the three archive formats |
| [tracking-and-pointers.md](tracking-and-pointers.md) | Object tracking, pointer fix-ups, relocation, polymorphism via the export registry, the void-cast registry, smart-pointer support |
| [compatibility.md](compatibility.md) | Error-handling philosophy (exceptions vs `TranscribeResult`), backward/forward compatibility mechanisms, versioning levers, why not `boost::serialization` |
| [usage.md](usage.md) | How sessions/project files and pyGPlates pickling use the system, the deep transcribe chain for `RotationModel`/`FeatureCollection`, test coverage, and an inventory of all users |
| [performance.md](performance.md) | Where the per-object costs are, what is already optimized, the parked tracking-rework branch (`backup/pickle-perf-rewritten`) with its measured impact, and the fast path's phase-by-phase history and final benchmarks |
| [fast-path.md](fast-path.md) | The fast path ("raw lane"): a scoped streamed mode inside `Scribe`, the `RAW_STREAM` transcription kind, the raw codec and its versioning layers, shared-owner backrefs, bulk-array/geometry adoption, and the constraints (no references/probing inside raw sections, evolution discipline) anyone touching this code must respect |

## Who uses it

- **GPlates sessions and project files** — `src/presentation/TranscribeSession.cc` (common logic),
  `ProjectSession.cc` (`.gproj` files), `InternalSession.cc` (auto-saved sessions).
- **pyGPlates pickling** — `src/api/PythonPickle.h/.cc` bridges Python's pickle protocol to Scribe;
  every picklable pygplates class registers a `PickleDefVisitor`.
- **Transcribe-aware types across the codebase** — roughly 200 files outside `src/scribe/` include
  scribe headers: the model layer (`FeatureCollectionHandle`, `FeatureHandle`,
  `TopLevelPropertyInline`, …), ~70 property-value types, the maths geometry/rotation types, GUI
  state (colours, symbols, palettes), app-logic layer parameters, and view-operations render
  parameters.

See [usage.md](usage.md) for the full inventory.

## Quick mental model

- Every transcribed object (including every *pointer*, which is itself an object) gets an integer
  **object id** in the `Transcription`.
- Composite objects map **object tags** (interned name + version, relative to the parent object's
  scope) to child object ids; primitives (32-bit integers, float, double, interned strings) are
  leaves.
- **Tracking** (opt-in via the `GPlatesScribe::TRACK` option) records an object's address so that
  pointers/references to it can be resolved on load and can follow it if it is **relocated**
  (moved to its final address after being loaded into a temporary).
- Polymorphic objects owned through base-class pointers are reconstructed via the **export
  registry** (stable class-name strings written into the transcription).
- A failed load is reported through a **`TranscribeResult`** return code (recoverable — supply a
  default or skip), while programmer errors and corrupt archives throw **exceptions** (abort the
  whole transcribe).
