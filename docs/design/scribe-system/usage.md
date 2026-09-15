# How GPlates and pyGPlates Use Scribe

*Part of the [Scribe system design docs](README.md).*

## 1. Sessions and project files

### What is transcribed

`TranscribeSession::save()` / `load()` (`src/presentation/TranscribeSession.h:62`, `:83`;
implementation `TranscribeSession.cc`) serialize **application session state, not feature data**:

- Feature-collection **filenames only** (via `TranscribeUtils::save_file_paths`, which compacts
  shared path prefixes); on load the files are re-read from disk. Project files therefore
  *reference* external data files rather than embedding them (unlike pickling, below).
- The reconstruct graph: default reconstruction-tree layer, layer visual ordering, per-layer input
  connections.
- Per-layer app-logic parameters via save/load visitor pairs (reconstruction/reconstruct params,
  scalar coverage, topology network, velocity, raster band, co-registration config, …).
- GUI/visual state: colour-palette parameters, draw styles, render settings, graticules, symbols,
  etc.

### Project files (`ProjectSession.cc`)

A `.gproj` project file is a **binary archive containing two consecutive transcriptions**:

1. **Metadata** — `save_session` (`ProjectSession.cc:168`) uses a dedicated `Scribe`
   (`scribe_metadata`) to save the save-time, the loaded feature-collection file paths, *all*
   transcribed file paths, and the original project filename (used to detect that a project file
   has moved so data files in the same relative location can be found).
2. **Data** — a second `Scribe` (`scribe_data`) wrapped in a
   `ScopedTranscribeContextGuard<TranscribeUtils::FilePath>` (so every transcribed file path is
   also collected for the metadata), running `TranscribeSession::save()`.

Both scribes are checked with `is_transcription_complete()` before
`BinaryArchiveWriter::create(...)` writes the two transcriptions (`ProjectSession.cc:246`–`:252`).
Because archives support consecutive transcriptions, `create_restore_session` can read *only* the
metadata transcription — showing the file list and detecting moved projects without loading
session state.

### Internal (auto-saved) sessions (`InternalSession.cc`)

Same two-transcription pattern, but the binary archives are stored as `QByteArray`s in user
preferences rather than a file. In addition, a **deprecated GPlates 1.5 session format** is
maintained for backwards compatibility: `TranscribeSession::save(scribe_data, scribe_data_1_5)`
fills a parallel scribe whose transcription is written as a **text archive** into a string
(`InternalSession.cc:355`) — the only production use of the text archive format.

### Error handling

- Hard errors (`GPlatesScribe::Exceptions::BaseException`) abort the whole save/load and surface
  in the GUI (`src/gui/FileIOFeedback.cc`).
- Per-piece incompatibility is tolerated: loaders check each `transcribe()` result and skip
  incompatible pieces (a layer connection, a palette) rather than failing the session.
- If required state is incompatible, `TranscribeSession::load` throws
  `TranscribeSession::UnsupportedVersion` (`TranscribeSession.h:96`) — carrying the
  transcribe-incompatible call stack from
  `Scribe::get_transcribe_incompatible_call_stack()` for diagnostics.
- `ProjectSession::has_session_state_changed()` re-runs `TranscribeSession::save` into a fresh
  transcription and compares it (`Transcription::operator==`) with the last-saved one to detect
  unsaved changes.

## 2. pyGPlates pickling

### The bridge (`src/api/PythonPickle.h` / `.cc`)

Pickling a pygplates object is a **binary Scribe transcription of the underlying C++ object**,
wrapped in Python `bytes`. A class opts in with one line in its boost::python wrapper:

```cpp
// PyRotationModel.cc
.def(GPlatesApi::PythonPickle::PickleDefVisitor<GPlatesApi::RotationModel::non_null_ptr_type>())
// PyFeatureCollection.cc
.def(GPlatesApi::PythonPickle::PickleDefVisitor<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type>())
```

`PickleDefVisitor<ObjectHolderType>` (`PythonPickle.h:280`) registers two things
(`visit`, `PythonPickle.h:303`):

- an `__init__` overload via `boost::python::make_constructor(&Impl::init<ObjectHolderType>)` —
  the unpickling constructor, and
- `def_pickle(Impl::PickleSuite<ObjectHolderType>())` (`PythonPickle.h:191`), whose
  - `getinitargs()` pickles the C++ object to bytes (the payload), and
  - `getstate()`/`setstate()`/`getstate_manages_dict()` additionally pickle the Python object's
    `__dict__`, so attributes added from Python survive a round trip.

The pipeline (`Impl::pickle`, `PythonPickle.h:136`):

```
save:  Scribe scribe; Transcribe<Holder>::pickle(scribe, holder)      // scribe.save(holder, "object")
       → scribe.get_transcription() → transcription_to_bytes()        // BinaryArchiveWriter → QByteArray
load:  bytes_to_transcription() → Scribe scribe(transcription)
       → Transcribe<Holder>::unpickle(scribe)                         // scribe.load<Holder>("object")
```

`transcription_to_bytes`/`bytes_to_transcription` (`PythonPickle.cc`) run a
`BinaryArchiveWriter`/`Reader` over a `QDataStream` on a `QBuffer`; the `QByteArray` crosses into
Python as a small `PickleBytes` wrapper class. If unpickling fails (`LoadRef` invalid), it throws
`Exceptions::UnsupportedVersion` — the pickling and unpickling pyGPlates versions differ
(`PythonPickle.h:84`).

The default `Transcribe<ObjectHolderType>` (`PythonPickle.h:57`) simply saves/loads the holder
(owning smart pointer), which recursively transcribes the pointee. It is **specializable** per
holder type — the sanctioned hook for giving a type a pickle-only serialization that differs from
its session serialization.

### What pickling a `RotationModel` transcribes

`GPlatesApi::RotationModel` (`src/api/PyRotationModel.cc`) implements
`transcribe_construct_data` (`:424`) / `transcribe` (`:479`), both delegating to
`save_construct_data` (`:530`) / `load_construct_data` (`:562`):

- For each feature-collection file: the **entire feature collection by value**
  (`scribe.save(..., feature_collection, files_tag[i]("feature_collection"))`, `:550`) plus its
  absolute **filename** as metadata (`:551`). The filename is *not* used to reload data — the
  features are embedded.
- Three scalars: `reconstruction_tree_cache_size`,
  `extend_total_reconstruction_poles_to_distant_past`, `default_anchor_plate_id` (`:555`–`:557`).
- On load: files and weak-refs are rebuilt and a fresh cached reconstruction-tree creator is
  constructed (`:457`) — reconstruction trees are **recomputed, not serialized**.

So a `RotationModel` pickle scales with the total rotation-feature content, and the cost is the
recursive feature-collection walk below.

### What pickling a `FeatureCollection` transcribes

The chain is a deep per-object recursive transcription — **no GPML/XML string and no file-io
shortcut is involved anywhere**:

```
FeatureCollectionHandle::transcribe            src/model/FeatureCollectionHandle.cc:64
  d_tags, then std::vector<FeatureHandle::non_null_ptr_type> "features"
  └─ FeatureHandle                             src/model/FeatureHandle.cc:231 (construct: feature_type, feature_id), :261
       std::vector<TopLevelProperty::non_null_ptr_type> "properties"
       └─ TopLevelPropertyInline               src/model/TopLevelPropertyInline.cc:156 / :213
            property_name, std::vector<PropertyValue::non_null_ptr_type> "property_values",
            xml_attributes; transcribe_base<TopLevelProperty>() (:285)
            └─ PropertyValue (polymorphic — via export registry)
                 ~70 concrete types in src/property-values/ each with their own transcribe():
                 GpmlPlateId, GpmlFiniteRotation, GpmlIrregularSampling, GpmlConstantValue,
                 GmlTimePeriod, GmlLineString, GmlMultiPoint, XsString, Enumeration, ...
                 └─ leaves in src/maths/: PolylineOnSphere, MultiPointOnSphere, PointOnSphere,
                    GreatCircleArc, UnitQuaternion3D, FiniteRotation, Real, ...
```

Every element of every vector is an owning `non_null_intrusive_ptr`, so each property value goes
through the smart-pointer protocol, the export registry (class-name string per pointee), and
object tracking of the pointee. This walk — millions of small tagged objects for a large rotation
model or feature collection — is where pickling time goes (see
[performance.md](performance.md)).

Polymorphic property-value types are export-registered via `SCRIBE_EXPORT_REGISTRATION` in
`src/ScribeExportPyGPlates.cc:41` (combining the maths / model / property-values / external lists).

### Test coverage

`pickle.dumps`/`loads` round-trips are covered by ~49 `test_pickle` methods across the pygplates
test suite:

- `pygplates/test/test_app_logic/test.py` — `RotationModelTestCase.test_pickle` (`:2586`; also
  covers a delegating rotation model, a non-zero default anchor plate, and a Python-side
  attribute via `__dict__`), plus `ReconstructModel`, `ReconstructSnapshot`, `NetRotationModel`,
  `StrainRate`, `TopologicalModel`, `TopologicalSnapshot`.
- `pygplates/test/test_model/test_property_values.py` — one per property-value type (28).
- `pygplates/test/test_model/test.py` — features, properties, and feature lists.
- `pygplates/test/test_maths/` — geometries, rotations, etc.

`RotationModelTestCase.test_pickle` is the natural benchmark harness for the planned performance
work: it pickles a `RotationModel` loaded from a real rotation file.

## 3. Inventory of all Scribe users

From `#include "scribe/..."` outside `src/scribe/` (~200 files):

| Area | Files / purpose |
|---|---|
| **Export registration** | `src/ScribeExportPyGPlates.cc` (pygplates master list); `ScribeExportMaths.h`, `ScribeExportModel.h`, `ScribeExportPropertyValues.h` in their subdirectories. |
| **api/** | `PythonPickle.h/.cc`; ~30 `Py*.cc` files adding `PickleDefVisitor` and/or `transcribe()` for wrapped types (`PyRotationModel`, `PyFeatureCollection`, `PyTopologicalModel`, `PyGeometriesOnSphere`, `PyRevisionedVector.h`, …); `APIVersion.h/.cc`. |
| **model/** | `FeatureCollectionHandle.cc`, `FeatureHandle.cc`, `TopLevelPropertyInline.cc`, `RevisionedVector.h`, `RevisionId`, `XmlNode`, `GpgimVersion`, and generic helpers `TranscribeRevisionedVector.h`, `TranscribeQualifiedXmlName.h`, `TranscribeStringContentTypeGenerator.h`, `TranscribeIdTypeGenerator.h`. |
| **property-values/** | ~70 files — every transcribable `Gpml*`, `Gml*`, `Xs*` type, `Enumeration`, `GeoTimeInstant`, `GmlDataBlock`, topological types, … |
| **maths/** | `PointOnSphere`, `MultiPointOnSphere`, `PolylineOnSphere`, `PolygonOnSphere`, `GreatCircleArc`, `UnitVector3D`, `UnitQuaternion3D`, `Vector3D`, `FiniteRotation`, `LatLonPoint`, `Real`. |
| **app-logic/** | Layer/reconstruction parameter types (`TopologyNetworkParams`, `VelocityDeltaTime`, `NetRotationUtils`, `DeformationStrain*`). |
| **presentation/** | `TranscribeSession`, `ProjectSession`, `InternalSession`, `SessionManagement` (exception handling), visual-layer params. |
| **gui/** | Transcribed visual state: `Colour`, `Symbol`, `GraticuleSettings`, `BuiltinColourPalettes`; `FileIOFeedback.cc` (exception handling only). |
| **view-operations/** | `ScalarField3DRenderParameters`. |
| **utils/** | `UnicodeString` (transcribe for `TextContent`); `CallStackTracker.h` underpins `TRANSCRIBE_SOURCE`. |
| **unit-test/** | `TranscribeTest.cc` — the scribe test suite; sole user of the XML archive format. |
