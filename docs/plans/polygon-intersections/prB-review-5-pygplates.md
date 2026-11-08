# PR B review 5: pyGPlates

Reviewed 2026-11-08 (Fable), at `9e089dad1` (the whole of PR B, `public/main...HEAD`, seen as one):
`src/api/PyGeometriesOnSphere.cc` (`polygon_on_sphere_partition` and the `partition` docstring),
`src/api/PyPlatePartitioner.cc` (both `__init__` overloads, `get_sliver_min_width`, `partition_geometry`,
`partition_point`, `_partition_feature_geometries`, the enum docstrings), `src/qt-resources/python/api/PlatePartitioning.py`
(`_plate_partitioning_set_geometry`, `_plate_partitioning_set_geometries`, `partition_features`, `partition_into_plates`),
`src/api/PyExceptions.cc` (`PartitionError`), `docs/pygplates/pygplates_reference.rst`, the committed stub
`pygplates/stub/__init__.pyi`, the PR B entries of `CHANGELOG-pyGPlates.md`, the `test_partition*` and
`test_most_overlapping*` tests of `pygplates/test/test_maths/test_geometries_on_sphere.py` and
`pygplates/test/test_app_logic/test.py`, and the sample code that partitions (`docs/pygplates/sample-code/*assign_plate_ids*`);
against `docs/design/architecture/foundation/spherical-geometry.md` sections 5.1, 5.4, 5.5, 5.6 and 5.7, with the step
reviews 5, 7 and 8 and PR B reviews 1 to 4 for what is known. Where the C++ below the binding was needed
(`PartitionFeatureUtils::partition_feature` and the visitor's `get_geometry_property`, `GeometryUtils`'
`get_geometry_from_property_value`, `PyFeature.cc`'s `set_geometries`, `RevisionAwareIterator`) it was read for what the
binding relies on, not re-reviewed. Line numbers are at `9e089dad1`.

Evidence is marked *run* or *reading*. *Run*: the baseline suite; the stub check
(`generate_stub.py --check`, exit 0); the docs built from scratch (`_doctrees` and `generated/` deleted first;
`-W`; clean, `generated/pygplates.PartitionError.html` present and referenced from the three pages); two probe
scripts against the built module (a constant geometry beside a same-named two-window time-dependent geometry,
with and without coverages; an unmatched range; a feature with a removed property, for the index alignment; the
inconsistent overlay of `src/unit-test/data/polygon-overlay/inconsistent.txt` as a feature and as a plate, through
all four entry points; a grouped return with a split multi-point coverage; a two-window geometry under
`most_overlapping_plate` at 100 Ma; the sliver argument positionally, `NaN`, and the piece types of a point, a
multi-point and a polyline; a coverage polygon beside a plain same-named polygon, two scalar types; an
exterior-ring-only coverage beside a plain polygon; a cut at 10 Ma under a real rotation with a kept time-dependent
property; a polyline and a point coverage; the result-only and list forms on a sliver case at three widths); and
four mutations of `PlatePartitioning.py` and `PyPlatePartitioner.cc`, two at a time, each pair rebuilt and run
against `pygplates-test` (504 tests). All removed afterwards by `git checkout` and a touch of both files; the module
rebuilt; the full suite green (`ctest`: 7 of 7); `git status` clean but for this file. The probe scripts were kept
in the session's scratchpad.

Baseline (*run*, before any change): `ctest --test-dir build-pygplates -C Release`: 7 of 7 passed
(`pygplates-test` 504 tests).

The binding does what the page says in every case tried but one: `set_geometry` replaces every property of the
name it sets, so a same-named property that the shared visitor deliberately left unpartitioned is lost from every
piece feature (1.1); that is also the mechanism of review 4's 1.2 (1.3). The fixes of reviews 5 (the docstrings,
the result-only path), 7 (the ambiguous counts, the dropped time-dependent property, mixed pieces, the lost `def`,
the exterior-ring-only form) and 8 (the great-circle ring, `float, or None`) hold (*run*, section 3), and the
four decisions mutated are each pinned by a committed test (section 2).

## 1. Findings, most severe first

### 1.1 A geometry property, or a coverage range, that was deliberately not partitioned is dropped from every piece feature when a partitioned property has the same name (run)

`partition_features` removes from the clone exactly the properties the visitor reported (`PlatePartitioning.py:377-382`),
and the page (5.4) and the docstring (`:165-168`) promise that any other time-dependent geometry "stays unchanged
on every feature returned". But the pieces are then written with `Feature.set_geometry` (`:37`, `:41`, `:73`,
`:75`, `:77`), which *sets* the property name: `set_geometries` (`PyFeature.cc:1258`, `feature_handle_set_property`)
replaces every property of the geometry name, and with a coverage every property of the range name too; without a
coverage it removes every property of the range name (`:1274`). So whatever shares a name with a partitioned
property and was not partitioned, which is exactly the two-window `GpmlPiecewiseAggregation`, the single window not
containing the time, and the `GpmlIrregularSampling` the visitor leaves alone, is silently removed from every feature
`split_into_plates` returns.

*Run*, probe A: a feature with a constant `gpml:unclassifiedGeometry` polygon across plates 1 and 2 and a second
`gpml:unclassifiedGeometry` that is a two-window `GpmlPiecewiseAggregation`. The two piece features each have the
constant piece, the name, the plate ID and `gpml:geometryImportTime`; the two-window property is on neither. Probe A2,
with a constant coverage (domain and `gpml:unclassifiedGeometryCoverage` range) beside a two-window domain *and* a
two-window range of the same names: both time-dependent properties are gone from both pieces. Under
`most_overlapping_plate` (probe A, which clones the feature whole) the two-window property survives. GPlates'
Assign Plate IDs copies it into every piece feature (review 4, 1.4's last point), so the products disagree, and the
pyGPlates loss is permanent data loss from a user's file. The committed `test_partition_features_time_dependent_geometry`
puts the time-dependent property under `gpml:outlineOf` and the constant one under `gpml:unclassifiedGeometry`, so it
cannot see this.

Rare input (GPlates writes geometries as constants, and this needs two same-named geometry properties with one
time-dependent), but it is the one case the time-dependent rule exists for, it is documented as kept, and the loss is
silent. Fix: in `partition_features`, after `feature_without_geometry` is built, also remove from it every remaining
property whose name is a partitioned domain or range name (they are the unpartitioned same-named ones), keep their
clones, and `add` them back to every feature `_plate_partitioning_set_geometries` returns, after its `set_geometry`
calls; or write the pieces with `Feature.add` of the geometry property values instead of `set_geometry` (which
would lose `set_geometry`'s coverage checks). Probe A and A2 are the tests.

### 1.2 `most_overlapping_plate` measures a time-dependent geometry by its present-day window at any reconstruction time, and the docstring's time-dependent rule reads as applying to it (run; pre-existing)

The `most_overlapping_plate` branch (`PlatePartitioning.py:405-407`) partitions `feature.get_all_geometries()`,
whose docstring says "present day": `GeometryUtils`' visitor takes the time window containing 0 Ma
(`GeometryUtils.cc:888-905`), not the partitioner's reconstruction time. *Run*, probe F: a feature whose only geometry
is a two-window property (window 50-0 Ma in plate 1, window 200-50 Ma in plate 2), partitioned at 100 Ma:
`split_into_plates` returns it unpartitioned (the rule); `most_overlapping_plate` assigns plate 1, the plate of the
present-day window, while the geometry at 100 Ma lies in plate 2. The old code did the same, and GPlates uses the
visitor at the reconstruction time, so this is a pre-existing pyGPlates-only discrepancy; recorded because the new
docstring paragraph ("A time-dependent geometry ... is partitioned only if it has a single time window that contains
the reconstruction time ... Any other time-dependent geometry is not partitioned", `:165-168` and `:552-555`) is not
scoped to `split_into_plates`, where the changelog's bullet is. Fix: scope the paragraph, or have the branch
partition the geometries `_partition_feature_geometries` reports (which are at the reconstruction time, and apply the
same single-window rule), which would also make the two methods agree on what geometry a feature has.

### 1.3 Review 4's 1.2, the pyGPlates half: an unmatched range is removed from every piece feature, where GPlates carries it into each (run; pre-existing)

Confirmed. *Run*, probe B: a triangle with a four-value `gpml:unclassifiedGeometryCoverage` (which `get_coverages`
cannot match), cut by two plates: the visitor reports the domain only; `set_geometry` of the plain pieces removes the
range by name (`PyFeature.cc:1274`); neither piece feature has it, and neither reads back a coverage. GPlates keeps it
on both pieces and reads it back as the coverage of whichever piece has four points (review 4). So 5.4's "the two
products cannot disagree" on which properties are removed does not hold for this input (nor for 1.1); pyGPlates'
answer (drop data that no longer describes any geometry) is the safer of the two, but it is silent. Same mechanism as
1.1, so the same fix decides it: a range of a partitioned domain's range name that matched no coverage is either
kept on every piece (GPlates) or dropped (pyGPlates), and the page should say which.

### 1.4 Minor (reading unless marked)

- **`NaN` is accepted as a width** (*run*, probe G): `get_sliver_min_width` (`PyPlatePartitioner.cc:68-83`) tests
  `< 0` and `== 0`, both false for `NaN`, and returns it; `is_sliver`'s `width < NaN` (`PolygonOverlay.cc:4049`) is
  false, so the filter is silently off. `polygon_on_sphere_partition` (`PyGeometriesOnSphere.cc:3654-3661`) takes
  `NaN > 0` as false and turns it off too. Consistent, but a `ValueError` beside the negative check is one line.
- **The order of the returned features** follows `partitioned_geometry_properties`, a `std::map` by property name,
  so a feature with several geometry property names now yields its piece features in name order rather than the
  feature's property order (the old loop walked the feature). Nothing documents an order; noted for a user comparing
  outputs.
- **Property order within a piece feature** (*run*, probe C): `set_geometry` appends, so a piece's geometry follows
  `gpml:reconstructionPlateId` where the original had it before. As the old code; cosmetic.
- **`is_exterior_ring_only`'s `AssertionError`** (`PlatePartitioning.py:33-34`) can only be reached by a C++ piece
  whose scalar count matches neither its points nor its exterior ring, which `partition_range` cannot produce; fine
  as an internal check, but it would surface to a user as an `AssertionError` with no `pygplates` type.
- **`partition_geometry` without lists stops at the first claiming plate**, so an inconsistent cut by a later plate
  does not raise there (review 3's 1.1, known); the docstring's `:raises PartitionError:` is unconditional.
- **Changelog**: "previously the coverage was left behind, no longer matching its pieces" is not what the old code
  did: `set_geometry` of plain pieces has removed every property of the range name since 1.0.0
  (`PyFeature.cc` at `PyGPlates-1.0.0`, eight such calls), so the coverage was *dropped* from every piece feature.
  "previously the coverage was lost" is the accurate clause.
- **Stub**: `anchor_plate_id: int = ...` in both `__init__` overloads against `int | None = None` in
  `partition_into_plates` (review 8's 1.5, pre-existing; not PR B's).
- Style: hard tabs, Allman braces and broken signatures in the new C++; the lambda `to_python` and the range-for
  loops match the file's C++11 use elsewhere; the Python matches the file (four-space indentation, the module's
  private-function comment). The `_partition_feature_geometries` comment (`PyPlatePartitioner.cc:445-458`) describes
  the tuple exactly as built.

## 2. Test gaps

*Run*: four mutations, applied two at a time to freshly restored files, rebuilt and run against `pygplates-test`
(504 tests); each failure names its mutation.

| Mutation | Committed suite | Note |
| --- | --- | --- |
| M1 `partition_features`: the removal loop skipped (`PlatePartitioning.py:381`), so the original geometries stay in every clone | caught (`test_partition_features_removes_partitioned_properties`) | only a *different* property name shows it, since `set_geometry` replaces a same-named one anyway (1.1) |
| M2 `_plate_partitioning_set_geometries`: all coverage pieces of a name set on one feature (`:60-61`) | caught (`test_partition_features_coverage_pieces`, `AmbiguousGeometryCoverageError`) | review 7's 1.1 fix is pinned |
| M3 `_partition_feature_geometries`: `respect_feature_time_period` true (`PyPlatePartitioner.cc:470`) | caught (`test_partition_features_other_cases`) | review 7's 1.6 note is now pinned |
| M4 `get_sliver_min_width`: zero returns the default width (`:79-82`) | caught (`test_partition_sliver_width_threshold`) | |

Inputs no committed test covers, each run here as a probe (all since removed):

1. **A same-named unpartitioned property** (1.1): the constant polygon beside a two-window geometry of the same
   name, and the coverage form (probe A, A2). Expect both time-dependent properties on every piece feature.
2. **`most_overlapping_plate` with a time-dependent geometry at a non-zero time** (1.2, probe F); pin whichever
   window is decided.
3. **An unmatched range** (1.3, probe B): pin the decision.
4. **`PartitionError` from `partition_features`, `partition_into_plates` and `partition_geometry`** (*run*, probe D,
   all right, by both methods, and `partition_geometry` without lists on a single plate): only
   `PolygonOnSphere.partition` is tested (`test_partition_error`). A plate feature built from `inconsistent.txt`'s
   `Q` and a feature from its `P` is the input.
5. **`partitioned_groups_and_unpartitioned` with coverage pieces** (*run*, probe E, a split multi-point coverage:
   each point keeps its own scalar; one group per plate).
6. **Reverse reconstruction of coverage pieces at a non-zero time under a real rotation** (*run*, probe K, plate 1
   rotated 15 degrees at 10 Ma: the plate-1 piece is rotated back, its five scalars ride with it, the outside piece
   and the kept `gpml:outlineOf` windows are untouched). The committed tests use `RotationModel([])`.
7. **Two scalar types on one coverage** (*run*, probe I: both carried, both the piece's length).
8. **A coverage polygon beside a plain same-named polygon** (*run*, probe I: per plate, a feature with the coverage
   piece and its range, and one with the plain piece; the coverage reads back): review 7's 1.5 is fixed but unpinned.
9. **A polyline and a point coverage** (*run*, probe L: the polyline's cut point interpolates to 10 at the equator;
   the point keeps its scalar).
10. **`NaN`** (1.4).

## 3. Checked and found correct

- **`PolygonOnSphere.partition`** (`PyGeometriesOnSphere.cc:3643-3740`): `None` and an omitted argument give `h`
  (`get_on_segment_start_threshold_sine()`, `sin(acos(1 - 1e-12))`, `1.4142e-6`, as the docstring's literal); zero
  gives `boost::none` to `PolygonPartitioner::create` (filter off); negative raises `ValueError` before any work.
  The result-only path and the list path agree on a sliver case at three widths (*run*, probe M; review 5's 1.1 is
  fixed), pieces are `PolygonOnSphere`s, a multi-point's parts are two `MultiPointOnSphere`s, a polyline's are
  polylines, a point is appended itself (*run*, probe H), a non-list argument is a `TypeError`, and the lists are
  not cleared (appended after the partition, so an exception leaves them as they were).
- **`PartitionError`** (`PyExceptions.cc:482-494`): registered for `PolygonOverlay::InconsistentOverlayException`
  with `MathematicalError` as base (*run*, `issubclass`); declared in `PyExceptions.h`; listed in
  `pygplates_reference.rst` under the mathematical errors; in the stub. It propagates from all four documented
  entry points, by both partition methods, and from `partition_geometry` with and without lists on one plate
  (*run*, probe D), with the overlay's message. Nothing is modified when it is raised: the Python builds its
  features only after `_partition_feature_geometries` returns, and the C++ appends to the Python lists only after
  the cascade.
- **Both `__init__` overloads** pass `sliver_min_width` to the cookie cutter's constructors in the right slot
  (`PyPlatePartitioner.cc:173-178`, `:283-288`; the types force it: an enum then an `optional<double>`), with
  `get_default_sliver_min_width()` (`10 h`) for `None`; the width is checked before the time and plate checks, so a
  bad width raises first with either overload. The sixth positional argument of the features overload is reached by
  `partition_into_plates` (`PlatePartitioning.py:738-740`) and the fifth of the plates overload works positionally
  (*run*, probe G). Sorting and `anchor_plate_id` are unchanged from `public/main`.
- **`_partition_feature_geometries`** (`:461-549`): `respect_feature_time_period` is `false` (M3 pins it; the page's
  "it never did" holds). The indices are counted over `feature->begin()..end()`, the same `RevisionAwareIterator`
  (skipping null slots) that Python's `__iter__` uses (`bp::iterator<FeatureHandle>`, `PyFeature.cc:4133`), and
  `FeatureHandle::clone` copies the revision, so `enumerate(feature_without_geometry)` lines up with them after a
  property has been removed from the original (*run*, probe C: the polyline, the polygon and the plate ID are each
  where they should be). The ranges are reported through the same visitor as the domains, so the removal covers
  both. A piece's scalars are built per `GmlDataBlockCoordinateList` element keyed by `ScalarType`, every element
  carried (probe I). `partition.reconstruction_geometry` and `piece.geometry_domain` go through the registered
  `non_null_ptr_to_const_type` converters, and the plate objects hash by C++ identity, so pieces of several
  properties group under one plate key (probe E, the committed tests).
- **`partition_features`, `split_into_plates`**: a feature with nothing partitioned is cloned whole into the
  unpartitioned list (the fixed G8-1, `test_partition_features_no_geometry`); every coverage piece gets its own
  clone and plain pieces of a plate share one (`_plate_partitioning_set_geometries`; M2); a property name with only
  coverage pieces makes no plain feature; the `InformationModelError` fallback is unchanged for plain pieces; the
  exterior-ring-only form is written as geometry plus an added `GmlDataBlock` and reads back (*run*, probe J; the
  committed test); the plain and coverage pieces of one name are no longer mixed in one `set_geometry` call (review
  7's 1.5; probe I). Properties are copied per piece feature, grouping by plate works for several features per
  plate, and the reverse reconstruction runs once over all partitioned features; a kept time-dependent property is
  not rotated by it (`ReconstructMethodByPlateId`'s visitor handles `GpmlConstantValue` only; *run*, probe K).
- **`most_overlapping_plate`**: area, then arc length, then points as a Python tuple; the `(0.0, 0.0, -1)` start
  takes a zero-measure plate (review 4's 1.1 is the GPlates side); `test_most_overlapping_plate_by_area` and
  `test_partition_features_other_cases` pin the order.
- **Docstrings against behaviour**: the `partition` docstring's defaults (`1.4142e-6`, about 9 metres; ten times
  that for `PlatePartitioner`, `1.4142e-5`, about 90 metres: `10 h` at 6371 km), the touching rule, the polyline
  along the outline inside, the `versionchanged`; `partition_geometry`'s cascade sentence; the `PartitionMethod`
  table's measure with its "before 1.1" note; `partition_into_plates`' equivalent-code block carries the new
  argument. Conventions: `:type sliver_width_threshold_radians: float, or None` in all three places (review 8's 1.5
  fixed), markup-free type fields, `:raises:` after `:rtype:`, double backquotes, `versionchanged` blocks; the stub
  check is clean and the stub carries `float | None` for the three.
- **Changelog**: the four-function bullet (polygons, holes, touching, `PartitionError`), the sliver bullet (the two
  defaults, zero), the time-dependent bullet scoped to `split_into_plates`, the measure bullet and the no-geometry
  fix are accurate; the coverage bullet's parenthetical is 1.4.
- **Sample code**: `pygplates_import_geometries_and_assign_plate_ids_*` partitions points and polylines with
  `partition_into_plates`, and `pygplates_calculate_velocities_in_dynamic_plates_assign_plate_ids` points with
  `partition_features`; neither cuts a polygon or uses a coverage, so PR B changes none of their output
  (`pygplates-sample-code-test` passes). The import page's text ("one or more of the static polygons") still holds.
- **The page**: 5.1 (same result with or without lists), 5.5 (the three widths, `None`/zero/negative), 5.6 (the
  four raise `PartitionError`), 5.7 (the private helper, one coverage per feature) hold against the code; 5.4's
  "cannot disagree" is 1.1 and 1.3.

## 4. Page, docstring and changelog statements to amend

- **5.4 "Time-dependent geometry"**: "stays unchanged on every feature returned" and "pyGPlates' `partition_features`
  removes exactly the properties it reports as partitioned, so the two products cannot disagree" fail when the kept
  property shares its name with a partitioned one (1.1), and for an unmatched range (1.3). Fix 1.1 and keep the
  sentences, or state the exception.
- **`partition_features` and `partition_into_plates` docstrings** (`PlatePartitioning.py:165-168`, `:552-555`):
  scope the time-dependent paragraph to `PartitionMethod.split_into_plates`, and say what `most_overlapping_plate`
  measures (the present-day geometry, 1.2), unless 1.2 is fixed.
- **`CHANGELOG-pyGPlates.md:43`**: "previously the coverage was left behind, no longer matching its pieces" to
  "previously the coverage was lost" (1.4).
- **`PartitionError` docstring** ("Nothing is partitioned when it is raised") and `partition_geometry`'s
  `:raises PartitionError:` are right for what runs; review 3's 1.1 (the no-list cascade stops early) is the only
  caveat, and belongs on the page, not here.
- **`get_sliver_min_width` and `polygon_on_sphere_partition`**: refuse `NaN` with the negative width's
  `ValueError`, or leave it and say nothing (1.4).
