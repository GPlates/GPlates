# Divergence report: property-values and downstream app code (`gplates` → `pygplates`)

> Exploration report (agent 2 of 3) for the pygplates→gplates branch unification. See `MERGE-PLAN.md`.

## (A) Property-values change pattern

**It is ONE conceptual pattern applied ~100 times, with complexity tiers.** Every property value is converted from the old "clone/deep_clone + plain data members" model to the pygplates **revisioned property-value** model. The transform is:

| Old (gplates) | New (pygplates) |
|---|---|
| plain member `d_value` | nested `struct Revision : public PropertyValue::Revision` holding the data |
| `set_value(x){ d_value=x; update_instance_id(); }` (inline) | out-of-line `set_value` using `GPlatesModel::BubbleUpRevisionHandler` + `get_revision<Revision>()` + `commit()` |
| getter `value()` returns `const T&` from `d_value` | getter **renamed** `get_value()`, returns `get_current_revision<Revision>().value` |
| `clone()` = `new T(*this)`; separate `deep_clone()`; `DEFINE_FUNCTION_DEEP_CLONE_AS_PROP_VAL()` | `clone()` delegates to virtual `clone_impl(context)`; `deep_clone`/macro **removed** |
| private copy-ctor + deleted `operator=` | clone-ctor takes `boost::optional<GPlatesModel::RevisionContext&> context_` |
| `static const StructuralType STRUCTURAL_TYPE` local to `get_structural_type()` | promoted to a **public static class member** `T::STRUCTURAL_TYPE` (defined in `.cc`) |
| (none) | added Scribe serialization: `transcribe_construct_data()` + `transcribe()` + `friend GPlatesScribe::Access` |
| (none) | `Revision` requires `clone_revision()` + `equality()` overrides |

**Concrete before/after — `src/property-values/XsString.h`** (the whole point of the file changes):
- `value()` → `get_value()` (returns `get_current_revision<Revision>().value`)
- ctor now `XsString(const TextContent&) : PropertyValue(Revision::non_null_ptr_type(new Revision(tc)))`
- adds nested `struct Revision : public PropertyValue::Revision` with `value`, `clone_revision`, `equality`
- adds `clone_impl(context)`, `STRUCTURAL_TYPE` static, and Scribe `transcribe*`
- `GpmlPlateId.cc` is the identical shape: `set_value` via `BubbleUpRevisionHandler`, `print_to` via `get_current_revision<Revision>().value`, plus `transcribe*`.

**Complexity tiers (this is the only "heterogeneity"):**
- **Leaf value types** (`XsString`, `XsInteger`, `XsDouble`, `XsBoolean`, `GpmlPlateId`, `GmlLineString`, `GmlPoint`): simple — single scalar/geometry in `Revision`. Getter renamed `x()` → `get_x()`.
- **Container types** (`GpmlPiecewiseAggregation`, `GpmlIrregularSampling`, `GpmlKeyValueDictionary`, topological types): additionally inherit `public GPlatesModel::RevisionContext`; their child collections change from `std::vector<GpmlTimeWindow>` to **`GPlatesModel::RevisionedVector<GpmlTimeWindow>`**, constructed through a `GPlatesModel::ModelTransaction` and held via `RevisionedReference`. This is where the ~14k insertions concentrate.
- Signatures **did change**: getters gained `get_` prefix (`value()`→`get_value()`, `polyline()`→`get_polyline()`, `value_type()`→`get_value_type()`, `time_position()`→`get_time_position()`); container accessors now return `RevisionedVector<...>&` instead of `std::vector<...>&`; some `create()` overloads now take `non_null_ptr_type` elements + iterator-range templates. So **downstream callers had to be edited** — they are not source-compatible.

New shared file on pygplates side: `src/property-values/ScribeExportPropertyValues.h` (+248) registering all PVs for serialization.

## (B) Per-directory verdict

| Dir | #files | Nature of change | pygplates-branch compile risk |
|---|---|---|---|
| `property-values/` | 102 | The revisioning refactor itself (source of the new API) | n/a (this is the model) |
| `app-logic/` | 36 | Mechanical getter renames (`get_value`, `get_time_position`, `get_polyline`…) in reconstruct-method/util files | none — already adapted |
| `file-io/` | 37 | Mechanical getter renames **+** `std::vector<GpmlTimeWindow>::const_iterator` → `RevisionedVector<...>::const_iterator` | none — adapted |
| `qt-widgets/` | 42 | Mechanical — edit widgets (`EditStringWidget`, `EditPlateIdWidget`, …) updated to `get_value()` etc. | none — adapted |
| `feature-visitors/` | 15 | Mechanical — `GeometryFinder`, `PropertyValueFinder` updated (`get_polyline`, `get_multipoint`, `get_property_name`) | none — adapted |
| `gui/` | 10 | Mechanical getter renames (small, net −8) | none |
| `data-mining/` | 6 | Mechanical **+ deletion**: `load_file`/`load_files` helpers removed from `DataMiningUtils.h/.cc` (net −72) | none |
| `maths/` | 4 | **Genuine behavioral change** (see D), not API adaptation | none |
| `view-operations/` | 3 | Mechanical (`FocusedFeatureGeometryManipulator`, `SplitFeatureUndoCommand`) | none |
| `utils/` | 2 | Mechanical (`GetPropertyAsPythonObjVisitor.h`) | none |
| `presentation/` | 2 | **Refactor** of `ReconstructionGeometryRenderer` (see D) | none |
| `cli/` | 1 | 1 deletion, cosmetic | none |
| `qt-resources/` | 9 | **Genuine feature divergence** (see D) | n/a (resources) |

Across all downstream app/GUI dirs the added calls are dominated by pure renames: **85× `.get_value(`, 28× `.get_time_position(`, 28× `.get_point(`, 28× `.get_multipoint(`, 22× `.get_value_type(`, 17× `.get_property_name(`, 16× `.get_polyline(`, 16× `.get_polygon(`, 6× `.get_geographic_description(`.** This is overwhelmingly mechanical adaptation, not behavioral divergence.

## (C) Is GPlates app code compiled on the pygplates branch, and against which model?

**Yes — the entire GPlates app/GUI tree is compiled on the pygplates branch, against the new revisioned model.** Evidence from `src/CMakeLists.txt` (identical structure on both branches):
- `source_sub_directories` on **both** branches lists all of `api app-logic canvas-tools cli data-mining feature-visitors file-io gui maths model opengl presentation property-values qt-resources qt-widgets scribe utils view-operations` and `add_subdirectory()` recurses into every one unconditionally (lines ~427–462).
- **No sub-directory `CMakeLists.txt` references `GPLATES_BUILD_GPLATES`** (grep returns zero). Each sub-dir ends with `target_sources_util(${SOURCE_TARGET} PRIVATE ${srcs})`, and `SOURCE_TARGET` is `pygplates` on the pygplates branch (line ~385) / `gplates-lib` on gplates (line ~337). `target_sources_util` (cmake/modules/Utils.cmake) does no filtering — it forwards everything to `target_sources`.
- Therefore `qt-widgets`, `gui`, `canvas-tools`, `presentation`, `view-operations` **all compile into the `pygplates` module**. `GPLATES_BUILD_GPLATES` only gates: the `gplates`/`gplates-no-gui`/`gplates-unit-test` executables vs the `pygplates` Python module, `/LARGEADDRESSAWARE`, the app icon/bundle, `Python::Python` vs `Python::Module` linkage, `GPLATES_PYTHON_EMBEDDING` define, and `TestPythonEmbedding`. It does **not** exclude any GUI source files.

**Consequence for the merge:** the app/GUI adaptations in (B) are not optional cosmetics — they are *required* for the pygplates module itself to compile, which is exactly why the pygplates branch already carries updated (not stale/broken) versions of the qt-widgets edit widgets and feature-visitors. **The qt-widgets edit widgets and feature-visitors on the pygplates branch are fully updated to the new property-value API** (e.g. `EditStringWidget.cc` `xs_string.get_value().get()`, `GeometryFinder.cc` `get_polyline()/get_multipoint()/get_point()/get_polygon()`).

## (D) Genuine divergence (NOT API adaptation) — items needing real merge attention

1. **`src/qt-resources/python/api/*` (pygplates Python API layer).** The pygplates branch **adds** `Feature.py` (+1392), `Crossovers.py` (+1032), `PlatePartitioning.py` (+847), `GeometriesOnSphere.py` (+463), `PropertyValues.py` (+457), `ReconstructionGeometries.py` (+435), `Property.py` (+107) under `python/api/`, and updates `python.qrc`. Pure pygplates functionality with no gplates counterpart.
2. **`src/qt-resources/python/scripts/hellinger/hellinger.py` (−3608 in the gplates→pygplates diff).** A desktop-only GUI tool script present on gplates but absent on pygplates — it comes from the 22 gplates-only commits (hellinger scripts moved into a Qt resource on gplates recently); pygplates instead still carries the old top-level `scripts/hellinger*.py`. The merge must preserve the gplates arrangement.
3. **`src/maths/` numerical/behavioral changes** (real, not renames):
   - `AngularExtent.h`: clamps `sqrt(1 - cos²)` — if `is_strictly_greater_than_one(d_cosine)` set `d_sine = 0.0` instead of calling `sqrt` (avoids exception on slightly-over-1 cosines).
   - `GeometryDistance.cc` / `GreatCircleArc.cc/.h`: adds computation/return of **closest positions on two great-circle arcs** (`closest_arc_positions` tuple plumbing). New capability, relied on by pygplates distance queries.
4. **`src/presentation/ReconstructionGeometryRenderer` refactor** (net −129/+110): free helper `get_subduction_polarity(...)` was moved into the renderer class as a member on pygplates, while gplates commit 81a9ce709 ("subduction teeth on topological sections, not only boundaries") restructured the same code differently — the one real manual merge conflict.
5. **`src/data-mining/DataMiningUtils` API removal**: `load_file`/`load_files` overloads removed on pygplates. Behavioral (API surface) change, not a rename.
6. **`src/gplates_main.cc`**: only a one-line change — a duplicate `Q_INIT_RESOURCE(python);` added next to the existing one. Cosmetic/likely accidental; harmless.

**Bottom line:** ~95% of the 167-file downstream diff and essentially all of the 102-file `property-values` diff is a single mechanical adaptation to the revisioned-property-value API (getter `get_` renames, `RevisionedVector` iterator types, `BubbleUpRevisionHandler` setters, Scribe transcribe). The GPlates GUI/app code *is* built on the pygplates branch (against the new model) and is already adapted, so the merge is low-risk for the property model. The genuinely divergent, human-review items are confined to: the pygplates Python `api/` additions, the hellinger relocation, the `maths/` numerical improvements, and the `presentation` renderer refactor.
