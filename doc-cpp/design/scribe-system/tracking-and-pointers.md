# Object Tracking, Pointer Fix-ups and Polymorphism

*Part of the [Scribe system design docs](README.md).*

## Why tracking exists

Transcribing a lone value needs no bookkeeping. Tracking exists for the graph cases:

- **Pointers/references on save**: a transcribed pointer must record *which* object it points to,
  which requires knowing the object ids of already/later transcribed objects by address.
- **Pointers on load**: multiple pointers to the same object must be re-linked to the single
  loaded instance — including pointers transcribed *before* the pointed-to object.
- **Relocation**: objects are often loaded into temporary storage (constructor parameters,
  `LoadRef` storage, container elements) and then moved to their final address; tracked state must
  follow the move so pointers resolve to the final location.
- **Shared ownership**: several `boost::shared_ptr`s to one object must share one control block
  after load.

Tracking is **opt-in** per call (`GPlatesScribe::TRACK`); untracked objects pay only transient
bookkeeping (they are mapped during their own transcribe so their children can relocate into them,
then unmapped in `post_transcribe` — `Scribe.cc:308`). Pointed-to objects of owning pointers are
always tracked.

## Data structures (all private to `Scribe`, `src/scribe/Scribe.h`)

### `ObjectInfo` (`Scribe.h:1328`) — one per transcribed object

Note that *pointers are themselves objects* (you can have pointers to pointers), so every
transcribed pointer also gets an `ObjectInfo`. Key fields:

| Field | Purpose |
|---|---|
| `object_id` | Index into `d_object_infos`. |
| `class_id` (optional) | Index into `d_class_infos`. |
| `object_address` (optional) | Current address; reset when untracked. |
| `is_object_pre_initialised` / `is_object_post_initialised` | Lifecycle flags: submitted for transcribing / fully transcribed-and-valid. A transcribed non-owning pointer whose pointee hasn't been transcribed yet is pre- but not post-initialised. |
| `uninitialised_transcribe_call_stack` (optional) | `TRANSCRIBE_SOURCE` stack captured for never-initialised pointers, printed by `is_transcription_complete()`. |
| `is_load_object_bound_to_a_reference_or_untracked_pointer` | Load-path flag: once a reference (can't re-bind) or untracked pointer (can't update) is bound to this object, relocating it becomes an error. |
| `pointers_referencing_object` | Linked list of pointer-object ids referencing this object — resolved or waiting. |
| `object_referenced_by_pointer` (optional) | If this object *is* a pointer: the pointee's id. |
| `parent_object` (optional), `child_objects`, `sub_objects`, `base_class_sub_objects` | The transcribe nesting structure. `child_objects` = everything transcribed while this object was being transcribed; `sub_objects` = the subset physically inside this object's memory (data members, bases — these move together on relocation); `base_class_sub_objects` = the base-class subset. |

The four lists are `GPlatesUtils::SmartNodeLinkedList<object_id_type>` with nodes allocated from a
`boost::object_pool` (`d_object_ids_list_node_pool`, `Scribe.h:1559`).

`untrack()` (`Scribe.h:1345`) resets everything *except* the pointer/reference bookkeeping, which
cannot be regained once dropped.

### `ClassInfo` (`Scribe.h:1207`) — one per registered type

Holds the per-type `transcribe_context_stack` (see
[architecture.md](architecture.md)), `object_size`, `object_type_info`, `dereference_type_info`
(for pointer types: what the pointer statically dereferences to), an optional type-erased
`relocated_handler`, and an optional type-erased `transcribe_owning_pointer` (knows how to
construct/transcribe an object of this type through an owning pointer; absent for abstract
classes).

### The maps

- `d_tracked_object_address_to_id_map` (`Scribe.h:1568`) — `std::map` keyed by
  `InternalUtils::ObjectAddress` = `{void *address, const std::type_info *type}`
  (`ScribeInternalUtils.h`). The key includes the *type* because an object and its first data
  member share an address; the comparator (`SortObjectAddressPredicate`) orders by address then
  `std::type_info::before()`. For polymorphic types the registered address is that of the *full
  dynamic object* (obtained via `dynamic_cast<void *>`), which makes multiple-inheritance offsets
  come out right. Const is stripped from types before tracking so `const int *` and `int *` unify
  (rationale at `Scribe.h:1598`).
- `d_class_type_to_id_map` (`Scribe.h:1571`) — `std::map<const std::type_info *, class_id>` using
  `type_info::before()` ordering.
- `d_object_infos` / `d_class_infos` — id-indexed vectors of pool-allocated infos.

## Pointer transcription and resolution

A transcribed pointer writes two things under reserved tags (`Scribe.cc:36`):

- `POINTS_TO_OBJECT_TAG` (`"scribe_points_to_object"`) — the pointee's object id
  (`NULL_POINTER_OBJECT_ID = 0` for NULL pointers).
- `POINTS_TO_CLASS_TAG` (`"scribe_points_to_class"`) — for owning pointers to polymorphic types:
  the pointee's export-registered class name, via `transcribe_class_name` (`Scribe.cc:397`). On
  load an unknown class name yields `TRANSCRIBE_UNKNOWN_TYPE` (`Scribe.cc:453`) — the
  forward-compatibility hook that lets sequences drop elements of types this version doesn't know.

**Deferred resolution.** A non-owning pointer may be transcribed before its pointee. On load the
pointer object exists but is unresolved; its id sits in the pointee's
`pointers_referencing_object` list. When the pointee finishes transcribing (or is relocated),
`resolve_pointers_referencing_object` (`Scribe.cc:1491`) walks that list and
`resolve_pointer_reference_to_object` (`Scribe.cc:1520`) writes the pointee's address into each
pointer — after **up-casting** from the pointee's dynamic type to the pointer's static dereference
type through the void-cast registry (`Scribe.cc:1577`), which applies any multiple-inheritance
pointer offset. A missing cast path throws `Exceptions::UnregisteredCast`. Pointers that never
resolve are reported by `is_transcription_complete()` with the call stack of the transcribe site.

## Relocation

`Scribe::relocated(source, relocated_object, transcribed_object)` (load path only) tells Scribe an
object moved from where it was transcribed to its final home. `relocated_address`
(`Scribe.cc:466`) then:

1. Asserts the object isn't bound to a reference or untracked pointer
   (`RelocatedObjectBoundToAReferenceOrUntrackedPointer`, `Scribe.cc:501`) — those bindings can't
   be updated.
2. Recursively re-maps the object's `sub_objects` (data members and bases move with the parent by
   a fixed pointer delta); children *outside* the parent's memory (e.g. heap elements a
   `std::vector` points at) are reclassified as needed.
3. Re-resolves every pointer referencing the object (and its sub-objects).

Typical triggers: `LoadRef<T>` values copied into their final variable, constructor parameters
absorbed into the constructed object inside `transcribe_construct_data`, and container elements
moved into a container by the sequence protocol. Types whose members self-reference can hook the
move with a `relocated()` customization point (`Transcribe.h`).

## Polymorphism: the export registry

`ExportRegistry` (`ScribeExportRegistry.h`, a singleton) maps a **stable class-id name string** ↔
`std::type_info` ↔ an `ExportClassType` carrying the type-erased helpers
(`TranscribeOwningPointer` to construct/transcribe a derived object through a base pointer, and
`TranscribeAnyObject` for `boost::any`/variant support).

What must be export-registered (`ScribeExportRegistration.h` header comment):

1. every non-abstract class transcribed through a base-class (owning) pointer, and
2. every type stored in a transcribed `boost::variant` or `boost::any` (which is why fundamental
   types appear in `SCRIBE_EXPORT_EXTERNAL`).

Registration is declarative: each source subdirectory defines a `SCRIBE_EXPORT_<subdir>`
boost-preprocessor sequence of `(((Type), "IdName"))` entries, and each *program/library* combines
them and invokes `SCRIBE_EXPORT_REGISTRATION(...)` once. For pyGPlates that is
`src/ScribeExportPyGPlates.cc:41`, which combines the maths, model, property-values and external
lists. Registering the same name for two types (or vice versa) throws at startup; registering an
abstract class fails to compile.

The class-id name is written into the transcription for owned polymorphic pointees, so:

- **never rename an id string** once archives exist — even if the C++ class is renamed/moved,
  keep registering it under the old string (`ScribeExportRegistration.h:128`);
- an id string unknown to the loading version produces `TRANSCRIBE_UNKNOWN_TYPE`, not a hard
  error.

`Scribe` holds a reference to `Access::EXPORT_REGISTERED_CLASSES` (`Scribe.h:1587`) purely to
force the registration translation unit to be linked into the binary.

## The void-cast registry

`VoidCastRegistry` (`ScribeVoidCastRegistry.h`) is a runtime inheritance graph over
`std::type_info` nodes. Links are registered lazily whenever `transcribe_base()` runs
(derived→base, with `static_cast` links for non-polymorphic and `dynamic_cast` links for
polymorphic pairs, plus `boost::shared_ptr<void>` variants). `up_cast`/`down_cast` search the
graph for a path and apply the composed pointer adjustment; more than one path (a
non-virtual-diamond) throws `Exceptions::AmbiguousCast`.

**Design rationale** (from the original `DesignRationale.txt`): when transcribing a pointer to a
base-class sub-object, Scribe stores the object id of the *full derived object* and up-casts on
load, rather than storing an id for the base sub-object itself. This is more robust across
versions: if a later GPlates changes `B` to inherit from `A` and starts transcribing `A *`
pointers where it used to transcribe `B *`, both old and new pointers still reference the same
object id (that of the full `B` object), so the pointer transcription itself needs no
compatibility shim — only the object's own transcribe handler does.

## Smart-pointer support

- **Raw pointers** — tracked links as above; with `EXCLUSIVE_OWNER` the pointee is transcribed
  (and on load heap-allocated) through the pointer.
- **`boost::shared_ptr<T>`** (`TranscribeBoost.h`) — transcribes via the shared-owner smart-pointer
  protocol. On load, `Scribe` keeps `d_shared_ptr_map` keyed by pointee `ObjectAddress` so all
  `shared_ptr`s to one object share a single control block; the registry has dedicated
  `shared_ptr<void>` up/down-cast support for this.
- **`boost::weak_ptr<T>`** — delegates to the `shared_ptr` handling.
- **`boost::intrusive_ptr<T>`** / **GPlates `non_null_intrusive_ptr<T>`**
  (`TranscribeBoost.h`, `TranscribeNonNullIntrusivePtr.h`) — shared-owner semantics; the pointee's
  own reference count is the control block, so no sharing map is needed. `non_null_intrusive_ptr`
  is the holder type used throughout the GPlates model, and hence the workhorse of pickling.
