# Core Architecture: the `Scribe` class and the transcribe protocol

*Part of the [Scribe system design docs](README.md).*

## The `Scribe` class

`GPlatesScribe::Scribe` (`src/scribe/Scribe.h:165`) is the single entry point clients use to
transcribe an object graph. It is non-copyable and directional:

- `Scribe()` (`Scribe.cc:40`) constructs a **saving** scribe with an empty `Transcription`.
- `Scribe(transcription)` (`Scribe.cc:51`) constructs a **loading** scribe from a transcription
  (typically read from an archive); it throws `Exceptions::TranscriptionIncomplete` if the
  transcription is not complete.
- `is_saving()` / `is_loading()` (`Scribe.h:190`, `:200`) report the direction; client
  `transcribe()` handlers branch on these when save and load genuinely differ.

Responsibilities:

- Drive the object network: recursively invoke client `transcribe()` handlers down to primitives.
- Maintain **object tracking** (address ↔ object id), **class registration**
  (`std::type_info` ↔ class id), pointer/reference resolution and **relocation**
  (see [tracking-and-pointers.md](tracking-and-pointers.md)).
- Delegate the *representation* to a `Transcription` via an embedded `TranscriptionScribeContext`
  (`d_transcription_context`, `Scribe.h:1545`; see
  [transcription-and-archives.md](transcription-and-archives.md)).
- Own a `VoidCastRegistry` (`d_void_cast_registry`, `Scribe.h:1553`) for multiple-inheritance
  pointer fix-ups.

## Public API

All calls take a `TRANSCRIBE_SOURCE` first argument — a macro (`Scribe.h:136`) expanding to a
`GPlatesUtils::CallStack::Trace(__FILE__, __LINE__)`. Every transcribe call pushes this trace onto
a call-stack tracker so diagnostics can print *where in the source* a problem object was
transcribed.

| Method | Anchor | Purpose |
|---|---|---|
| `transcribe(source, object, tag, options = 0)` | `Scribe.h:371` | The primary call, symmetric save/load. Returns a `Bool` that is `false` only on *load* failure (always `true` on save). |
| `transcribe_base<BaseType>(source, derived, base_tag)` / `transcribe_base<BaseType, DerivedType>(source)` | `Scribe.h:504`, `:599` | Transcribe a base-class sub-object from within a derived class's handler; also registers the derived→base link in the void-cast registry. |
| `save(source, object, tag, options)` | `Scribe.h:641` | Save-only form, used inside `transcribe_construct_data()` and in handlers where save/load are asymmetric. |
| `load<T>(source, tag, options)` → `LoadRef<T>` | `Scribe.h:712` | Load-only counterpart. The returned `LoadRef<T>` must be checked with `is_valid()`; the loaded object lives in scribe-owned storage until the client copies it out and (if tracked) calls `relocated()`. |
| `save_reference` / `load_reference<T>` | `Scribe.h:767`, `:828` | For C++ *references*. References are not objects: they cannot be re-bound on load, so they may only reference already-transcribed tracked objects. |
| `relocated(source, relocated_object, transcribed_object)` | `Scribe.h:921`, `:931` (LoadRef overload) | Load-path only: tells Scribe a loaded object has moved to its final address so tracking and pointers follow it. |
| `get_transcribe_result()` | `Scribe.h:860` | The `TranscribeResult` explaining the most recent load failure. |
| `has_been_transcribed(object)` | `Scribe.h:966` | Whether a (tracked) object has been transcribed. |
| `is_in_transcription(tag)` | `Scribe.h:979` | Whether a tag exists in the current scope — lets loaders probe for optional/legacy data. |
| `push_transcribe_context<T>` / `get_transcribe_context<T>` / `pop_transcribe_context<T>` | `Scribe.h:1006`, `:1078`, `:1086` | Per-type context stack (see below); RAII wrapper `ScopedTranscribeContextGuard` at `Scribe.h:1093`. |
| `get_transcribe_incompatible_call_stack()` | `Scribe.h:1132` | Call stack captured when incompatibility was first detected (used for the `UnsupportedVersion` diagnostics in sessions). |
| `is_transcription_complete(emit_warnings)` | `Scribe.h:1179`, impl `Scribe.cc:76` | Checks no transcribed object was left uninitialised (e.g. a pointer whose pointed-to object was never transcribed) and that the `Transcription` itself is complete. Callers assert this before writing an archive. |
| `get_transcription()` | `Scribe.h:1190` | The transcription, to hand to an `ArchiveWriter`. |

### The `Bool` return type

`transcribe()` and friends return `GPlatesScribe::Bool` (`ScribeBool.h`), not plain `bool`. `Bool`
sets a "checked" flag when tested; if a `Bool` is destroyed unchecked it throws
`Exceptions::ScribeTranscribeResultNotChecked`. This forces load-path callers to handle
incompatibility rather than silently ignoring it. (Release builds relax this.)

### Options (`ScribeOptions.h`)

- `TRACK` (`ScribeOptions.h:43`) — objects are **not tracked by default**; pass this to enable
  tracking (needed if pointers/references to the object are transcribed, or if the object will be
  relocated). Pointed-to objects of owning pointers are always tracked.
- `EXCLUSIVE_OWNER` / `SHARED_OWNER` (`:47`, `:51`) — pointer-only options declaring that the
  pointer owns its pointee, which makes Scribe transcribe (and on load, heap-allocate) the
  pointed-to object through the pointer. Without an owner option only the pointer *link* is
  transcribed and the client must transcribe the pointee itself.

## Client customization points (`Transcribe.h`)

A type becomes transcribable by providing, in order of preference:

1. **Intrusive member** `TranscribeResult transcribe(Scribe &, bool transcribed_construct_data)`
   (usually private, with `friend class GPlatesScribe::Access;`), and optionally a static
   `transcribe_construct_data(Scribe &, ConstructObject<T> &)` when the type has no default
   constructor, plus an optional static/member `relocated()` hook for types with self-references.
2. **Non-member overloads** of `GPlatesScribe::transcribe(Scribe &, T &, bool)` (and
   `transcribe_construct_data`, `relocated`) — used for third-party types
   (`TranscribeStd.h`, `TranscribeBoost.h`, `TranscribeQt.h`).

Dispatch goes through `GPlatesScribe::Access` (`ScribeAccess.h`), the one friend class clients
declare. It calls the intrusive members if present (detected via SFINAE metafunctions like
`HasStaticMemberTranscribeConstructData`) and falls back to the non-member overloads
(`TranscribeImpl.h`). Enums and pointers are statically rejected from the generic path — enums use
the registered-name protocol in `TranscribeEnumProtocol.h`, and pointers have dedicated handling
inside `Scribe`.

The `transcribed_construct_data` parameter passed to `transcribe()` tells the handler whether the
constructor-argument data was already transcribed by `transcribe_construct_data()` (i.e. the object
was created by Scribe via an owning pointer or `load<T>()`), so the handler can skip those members.

### Protocol helpers

Generic container/wrapper support is layered on the same primitives:

- `TranscribeSequenceProtocol.h` — `std::vector`, `std::list`, `QList`, … (all share one protocol,
  so containers can be swapped between versions).
- `TranscribeMappingProtocol.h` — map-like containers.
- `TranscribeArray.h` — native C arrays.
- `TranscribeSmartPointerProtocol.h` — shared logic for smart pointers.
- `TranscribeDelegateProtocol.h` — transcribe one type via another's representation (e.g. wrapper
  types that should look identical to their wrapped value in the transcription).
- `TranscribeNonNullIntrusivePtr.h`, `TranscribeBoost.h`, `TranscribeQt.h`, `TranscribeStd.h` —
  per-library specializations.

## `ObjectTag` (`ScribeObjectTag.h`)

A tag identifies a child object *within the scope of its parent object only* — tags are not global
names. An `ObjectTag` is a list of sections: a named tag section (name + integer **tag version**),
an array-index section, or an array-size section. Fluent builders compose paths:

```cpp
ObjectTag files_tag("files");
scribe.save(TRANSCRIBE_SOURCE, num_files, files_tag.sequence_size());
scribe.save(TRANSCRIBE_SOURCE, fc, files_tag[i]("feature_collection"));  // files[i]/feature_collection
```

A `std::string`/`const char *` converts implicitly to a single-section tag. The sequence/mapping
protocols use fixed section names (`"item"`, `"size"`, …) so a hand-indexed tag and a transcribed
`std::vector` interoperate. The **tag version** is a compatibility lever: bumping it (or renaming
the tag) makes older code fail to find the object and take its incompatibility path (see
[compatibility.md](compatibility.md)).

## Save/load-construct: objects without default constructors

- `ConstructObject<T>` (`ScribeConstructObject.h`) wraps possibly-unconstructed storage;
  `construct_object(args...)` placement-news the object (via `Access::construct_object`, which is
  why `GPlatesScribe::Access` must be a friend to reach private constructors).
- `SaveConstructObject` / `LoadConstructObjectOnStack` / `LoadConstructObjectOnHeap`
  (`ScribeSaveLoadConstructObject.h`) are the concrete save-side wrapper and the stack/heap
  load-side storage variants (heap used when an owning pointer loads its pointee).
- The client's static `transcribe_construct_data(Scribe &, ConstructObject<T> &object)`:
  - on **save**: `scribe.save(...)` each constructor parameter (read from `object.get_object()`);
  - on **load**: `scribe.load<>(...)` each parameter, call `object.construct_object(params...)`,
    then `scribe.relocated(...)` each tracked parameter into its data-member location inside the
    new object.
- If `T` is default-constructible no hook is needed; the default implementation
  (`TranscribeImpl.h`) default-constructs and lets `transcribe()` fill in the state.

A representative example is `FeatureHandle::transcribe_construct_data`
(`src/model/FeatureHandle.cc:231`): it saves/loads `feature_type` and `feature_id`, then constructs
the feature from them; the main `transcribe()` (`FeatureHandle.cc:261`) handles the properties and
re-loads type/id itself when `transcribed_construct_data` is false (i.e. when the object was *not*
created by Scribe).

## `TranscribeContext<T>`: external context for loading

`TranscribeContext<ObjectType>` (`TranscribeContext.h`) is an empty template the client
*specializes* to carry information that is **not in the archive** but is needed to construct an
object on load — typically a reference to some untranscribed object (application state, a model,
a file registry). Instances are pushed per-type onto a stack inside `Scribe` (each class's
`ClassInfo` holds a `transcribe_context_stack`) via `push_transcribe_context<T>` or the RAII
`ScopedTranscribeContextGuard<T>`, and retrieved inside `transcribe_construct_data()` /
`transcribe()` via `get_transcribe_context<T>()`.

This is Scribe's answer to the `boost::serialization` difficulty of passing external context into
`load_construct_data` — one of the original reasons for writing Scribe (see
[compatibility.md](compatibility.md)).

Example: `ProjectSession::save_session` (`src/presentation/ProjectSession.cc:168`) pushes a
`TranscribeContext<TranscribeUtils::FilePath>` so that every file path transcribed anywhere in the
session state is also recorded for the project-file metadata.

> Naming note: `TranscribeContext<T>` (client-facing, per-type context) is unrelated to
> `TranscriptionScribeContext` (internal Scribe⇄Transcription adapter, see
> [transcription-and-archives.md](transcription-and-archives.md)).

## Anatomy of a transcribe call

For `scribe.transcribe(TRANSCRIBE_SOURCE, obj, "tag", options)`:

1. **Entry** (`Scribe.h`, templated): a `CallStackTracker` pushes the `TRANSCRIBE_SOURCE` trace.
   A compile-time/runtime check asserts `typeid(ObjectType) == typeid(obj)` to catch transcribing
   a derived object through a base *reference* (slicing hazard). All `const` is stripped via
   const-cast delegates so tracking sees one canonical type (rationale documented at
   `Scribe.h:1598`).
2. **Object id** — `Scribe::transcribe_object_id` (`Scribe.cc:336`): on save, allocates (or finds,
   if tracked and already known) the object id and writes it under the tag into the parent
   composite; on load, looks the tag up in the parent's composite — failure here returns
   `TRANSCRIBE_INCOMPATIBLE`.
3. **Pre-transcribe** — `Scribe::pre_transcribe` (`Scribe.cc:182`): initialises the object's
   `ObjectInfo` (address, class id), throws `AlreadyTranscribedObject` if the same tracked address
   is transcribed twice, maps the address→id (on load this happens here; note that even *untracked*
   objects are temporarily mapped while being transcribed so their children can be relocated into
   them), and pushes the object onto the transcribed-object stack (making subsequent tags relative
   to it).
4. **Stream** — the object's category dispatches:
   - *Arithmetic primitives* stream directly into `TranscriptionScribeContext::transcribe(int&)`
     etc. (`TranscriptionScribeContext.h:168`–`:241`), which store the value in the
     `Transcription` under the current object id. `std::string` is a special-cased primitive.
   - *Everything else* routes through the free `GPlatesScribe::transcribe()` →
     `Access::transcribe()` → the client handler, which recursively transcribes members
     (back to step 1 for each member).
   - *Pointers* transcribe the pointee's object id (+ class name if owning/polymorphic) — see
     [tracking-and-pointers.md](tracking-and-pointers.md).
5. **Post-transcribe** — `Scribe::post_transcribe` (`Scribe.cc:283`): marks the object initialised
   (or records the call stack if it is a not-yet-resolvable pointer), then either **untracks** it
   (the default — removes the address→id map entry; also used to discard objects that failed to
   load) or, if `TRACK` was requested, resolves all pointers waiting on this object
   (`resolve_pointers_referencing_object`, `Scribe.cc:1491`). Finally pops the object stack.

On the save side the result is a fully populated `Transcription`; callers then assert
`is_transcription_complete()` and hand `get_transcription()` to an archive writer.
