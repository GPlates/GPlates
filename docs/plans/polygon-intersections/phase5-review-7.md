# Phase 5 review 7: step 13, pyGPlates' side of polygon partitioning

Reviewed 2026-11-06 (Fable), the diff `354405908..877646cca` (code commits `27cbfea1f`, step 13a, and
`295cc3270`, step 13b; plan commits `328017a1a`, `db3f05133`, `877646cca`): `src/api/PyExceptions.{h,cc}`
(`PartitionError`), `src/api/PyGeometriesOnSphere.cc` and `src/api/PyPlatePartitioner.cc` (docstrings,
`_partition_feature_geometries`), `src/qt-resources/python/api/PlatePartitioning.py`,
`src/app-logic/PartitionFeatureGeometries.{h,cc}` (moved out of `PartitionFeatureUtils.{h,cc}`),
`src/app-logic/CMakeLists.txt`, the stub, both changelogs, `docs/pygplates/pygplates_reference.rst` and
the tests in `pygplates/test/test_maths/test_geometries_on_sphere.py` and
`pygplates/test/test_app_logic/test.py`, against `phase4b-design.md` sections 7.3, 8.4, 8.6, 9.2 and 11.5
and PLAN.md's 2026-11-06 entries for 13a and 13b. Line numbers are at `877646cca`. Evidence is marked
*run* (six probe scripts against the Release module, listed in section 2, and three mutations, each
rebuilt and run against `pygplates-test`; all since removed by `git checkout` and a touch, the module
rebuilt and the suite green afterwards, pyGPlates 7 of 7 and GPlates 191 of 191, and the docs built from scratch with no warnings;
`git status` shows only this file) or *reading*.

Baseline: pyGPlates 7 of 7 (the closure, linkage and stub tests included), GPlates 191 of 191.

Two behaviour regressions in `partition_features` were found, both reachable with ordinary inputs
(1.1 with any coverage whose pieces under one property name have the same number of points, 1.2 with a
time-dependent geometry property); one lost test name (1.4); the rest is limits of the
exterior-ring-only form, test gaps and minor observations.

## 1. Findings, most severe first

### 1.1 `partition_features` now raises `AmbiguousGeometryCoverageError` when two pieces of a coverage, under one property name, have the same number of points (run)

`_plate_partitioning_set_geometry` (`PlatePartitioning.py:21-41`) sets a property name's pieces in one
`set_geometry(geometries, name)` call (`:41`). With coverages, `set_geometries` (`PyFeature.cc:1116-1190`)
refuses two coverage geometries of the same point count under one name, because it would later read the
ranges back by count (`AmbiguousGeometryCoverageException`, `:1183-1186`), and that exception is a
`PreconditionViolationError`, not the `InformationModelError` the caller catches
(`:57`), so it escapes `partition_features` and aborts the whole call, every feature included. The old
code set plain geometries, which have no such check.

Concrete failing input (*run*, probe A): the polygon `box(-5, 5, 0, 20)` with scalars `1, 2, 3, 4`, one
plate `box(-20, 20, 8, 12)` across its middle. The two outside pieces each have four points (two of the
original's vertices and two cut points), so the unpartitioned feature's `set_geometry` raises
`AmbiguousGeometryCoverageError: more than one coverage *geometry* named 'gpml:unclassifiedGeometry'
with same number of points`. Without the coverage (probe A2) the same input gives one partitioned
feature and one unpartitioned feature with two polygons, as before. A polyline is the same (probe J: the
line `(0, 0)-(0, 20)` with two scalars, cut by the same plate, two outside pieces of two points each). The
case is common: any plate boundary that crosses a feature twice, leaving two outside pieces, and a
symmetric cut gives equal counts; two *inside* pieces in one plate (a plate with a hole, or a feature
that enters a plate twice) likewise. The `InformationModelError` fallback (`:57-63`), which would give
one feature per piece, is never reached, because the ambiguity check runs before the property is set.

Fix: catch `AmbiguousGeometryCoverageError` where `InformationModelError` is caught and take the same
one-feature-per-piece path (the feature's other properties are already in `feature_without_geometry`);
or split the pieces into features whenever two coverages' counts collide. Writing them two-step, as the
exterior-ring-only branch does, would set them but lose both on read-back (1.3). Note GPlates' Assign
Plate IDs writes such pieces as separate properties with no check
(`PartitionFeatureUtils.cc`'s `add_partitioned_geometries_to_feature`) and so reads back no coverage for
them: the same loss as 1.3, silently.

### 1.2 A time-dependent geometry property is no longer partitioned, and is dropped from every result (run)

`PartitionFeatureGeometryProperties` (`PartitionFeatureGeometries.cc:67-145`) visits
`GpmlConstantValue` and the five geometry types and inherits `FeatureVisitor`'s empty
`visit_gpml_piecewise_aggregation` (`model/FeatureVisitor.h:552-554`). The old Python used
`property_value.get_geometry()`, which is `GeometryUtils::get_geometry_from_property_value` at time 0
(`GeometryUtils.cc:864-907`, `GeometryUtils.h:332-334`) and takes the time window containing 0 Ma. So a
`GpmlPiecewiseAggregation` geometry property is now partitioned into nothing, and the removal loop
(`PlatePartitioning.py:341-349`), which still tests `property_value.get_geometry()`, removes it from the
clone: no piece feature carries it. A feature whose only geometry is time-dependent is neither
partitioned nor unpartitioned and vanishes from the output.

*Run*, probe B: a feature with `gpml:unclassifiedGeometry` = `GpmlPiecewiseAggregation([GpmlTimeWindow(
GmlPolygon(box(-5, 5, 0, 20)), 100, 0)])`, one plate `box(-20, 20, 8, 30)`:
`property.get_value().get_geometry()` finds the polygon, `_partition_feature_geometries(feature)`
returns `[]`, and `partition_features` returns no partitioned and no unpartitioned feature. Before 13b
the feature was cut into two. The `most_overlapping_plate` branch (`:375`) still uses
`feature.get_all_geometries()`, which does see it, so the two methods now disagree on what geometry a
feature has. The coverage visitor does handle piecewise aggregations
(`ScalarCoverageFeatureProperties.cc:270-293`, at the cookie cutter's time), so such a geometry's
coverage is found and its geometry is not partitioned.

Rare in practice (GPlates writes geometries as constant values), and the same visitor has always
skipped them in Assign Plate IDs, so this is pyGPlates catching GPlates' gap rather than a new one;
but it is silent. Fix: in the visitor, `visit_gpml_piecewise_aggregation` taking the window that
contains `geometry_cookie_cutter.get_reconstruction_time()`, as the coverage visitor does; that also
changes Assign Plate IDs (for the better, by reading). Either way, add the input to the tests.

### 1.3 Pieces of an exterior-ring-only coverage lose the coverage on read-back when, under one property name, their exterior rings have the same number of points (run)

The two-step write (`PlatePartitioning.py:36-39`) sets the geometries and adds one `GmlDataBlock` per
piece. Reading them back, `ScalarCoverageFeatureProperties::match_domains` (`:385-500`) finds, for the
first domain, two ranges of the right size, erases both from the candidates and creates no coverage
(`qWarning "Ambiguous ... coverage range"`, `:453-464`), so `get_geometry(coverage_return=...)` returns
nothing for that feature. Probe A3 (*run*): the polygon `box(-5, 5, 0, 20)` with holes at longitudes 2-4
and 16-18 and four exterior-ring scalars, cut by the plate `box(-20, 20, 8, 12)`: the unpartitioned
feature has two 8-point polygons and two 4-scalar ranges and reads back no coverage. Probe A4 shows the
same with *different* point counts: one piece of 4 points (no hole) and one of 8 (the hole), both with
4-scalar ranges, since the range is sized by the exterior ring; the first pass (all points) matches the
4-point piece to both ranges and discards them. The design's "a hole left whole gets none: inherent in
that form" covers the hole's values, not this: here the whole coverage of every piece under the name is
unreadable. GPlates writes and reads these the same way, so it is a limit of the form rather than of
13b, but "keeps that form, as GPlates' Assign Plate IDs writes them" (design 8.4, the changelog, the
commit message) reads as if the result were usable. Say so, or give each such piece its own feature
(the one-feature-per-piece path of 1.1 would do both).

### 1.4 `test_partition_point` lost its `def` (reading)

`295cc3270` replaced the line `def test_partition_point(self):` (at `354405908`, `test.py:2146`) with
`def test_partition_features_coverage(self):` (`:2175`), so the old test's body (`:2239-2259`,
the resolved-topologies `partition_point` checks) now runs as the tail of the coverage test. It still
runs, but under the wrong name, after the coverage assertions, and a failure in either part is
reported as the other's. Restore the `def` before `:2239`.

### 1.5 Pieces of mixed kind under one property name raise `TypeError` (run)

`get_geometry_property` (`PartitionFeatureGeometries.cc:464-472`) explicitly allows "some domains will
have no associated range while other domains (with the same domain property name) will have associated
ranges", and the Python groups pieces by property name (`:331-339`), so a feature with two
`gpml:unclassifiedGeometry` properties, one with a coverage, puts a plain geometry and a
`(geometry, scalars)` tuple in one list. `set_geometry` accepts a sequence of geometries or a sequence of
coverages, not a mix (`PyFeature.cc:205-364`): probe C (*run*) raises `TypeError: Expected a
GeometryOnSphere, or a sequence of GeometryOnSphere, or a coverage, or a sequence of coverages ...`.
In the exterior-ring-only branch, `for geometry, scalars in geometries` (`:37-38`) would fail on the
plain geometry instead (reading). The old code set every piece plain. A range whose count does not match
its domain gives the same mix (the domain is partitioned without a range, `:428-431`). Rare input (two
same-name geometry properties and a coverage on one); the one-feature-per-piece path of 1.1 covers it
too.

### 1.6 Minor observations (reading unless marked)

- `respect_feature_time_period` is `false` (`PyPlatePartitioner.cc:428`), matching the old Python, which
  never looked at the feature's valid time (probe E, *run*: a feature valid 100-50 Ma is partitioned at
  0 Ma, as before). With `true` the feature would silently vanish, as 1.2's input does (mutation M2,
  section 2); nothing pins the `false`. `if (!partitioned)` (`:454-457`) is then unreachable; harmless.
- Removing range properties by *name* (`PlatePartitioning.py:341-349`) also removes a `GmlDataBlock` of
  that name that is not this geometry's coverage (probe F2, *run*: a 3-value block beside a matched
  4-value coverage is gone from every piece). But `set_geometry` without a coverage already removed every
  property of the range name (`PyFeature.cc:1263-1277`, "so that the geometries are not interpreted as
  coverage domains"), so the old code lost such an orphan too (probe F2's second case): not a regression.
  Mutation M1 (section 2) shows the loop is redundant for a feature with one geometry property name; it
  matters when a feature has two names and a result feature gets pieces of only one, where the other
  name's original range would otherwise survive, orphaned. Keep it.
- `feature.add(range_property_name, GmlDataBlock(scalars))` (`:39`) verifies the information model by
  default, so a feature whose type does not allow the range property (one added with
  `VerifyInformationModel.no`, as the test's own second input is, `test.py:2221-2224`) raises
  `InformationModelError` there; the caller's `except` (`:57`) retries one piece per feature and raises
  again, uncaught. `set_geometry` with a coverage behaves the same for such a feature
  (`PyFeature.cc:1145-1154`, then the property set with verification), so the two branches agree; the old
  code, writing no range, did not raise. Only reachable for a coverage its feature type disallows.
- The move is verbatim: every non-blank line of `PartitionFeatureUtils.{h,cc}` at `354405908` is in the
  union of the four files now, and the only lines added are the new header's guard, comment and
  `#include "PartitionFeatureGeometries.h"` (a whitespace-trimmed sorted-line comparison, *run*).
  `PartitionFeatureUtils.cc` keeps the moved code's includes, none of which it now uses (grep:
  `ScalarCoverageFeatureProperties.h`, `ReconstructionFeatureProperties.h`, `maths/GeometryDistance.h`,
  `maths/Vector3D.h`, `boost/utility/in_place_factory.hpp`, `<numeric>`, `model/FeatureVisitor.h`,
  `global/AssertionFailureException.h`, the five `property-values/Gml*.h` and `GpmlConstantValue.h`, and
  the `maths/AngularExtent.h` review 6 noted). Harmless, worth a sweep. `PartitionFeatureUtils.h:39`
  includes the new header, so Assign Plate IDs and the tasks compile unchanged (GPlates 191 of 191,
  *run*).
- Include order: `PyExceptions.cc:54` puts `maths/PolygonOverlay.h` before `MultiPointOnSphere.h`;
  `PyExceptions.h:84` declares `PartitionError` between `MathematicalError` and
  `OpenFileForReadingError` (the `.cc:262` list has it in order); `PyPlatePartitioner.cc:51-54` has two
  `property-values` groups separated by a blank line, `GmlDataBlockCoordinateList.h` before
  `GeoTimeInstant.h`.
- Style otherwise: hard tabs, Allman braces, broken signatures. Every new C++ line over 100 columns has a
  neighbour within 30 lines that allows it (`PyPlatePartitioner.cc:413-464`, up to 111 beside 111; the
  over-limit lines in `PartitionFeatureGeometries.{h,cc}` are the moved code's). Python: `:58` (133) and
  `:62` (143) exceed 130 in a file whose lines reach 174; the file's own style.
- The `partition_features` docstring (`:118-268`) says nothing about coverages being carried; only the
  code comments (`:318-323`) and the changelog do. A user cannot learn it from the API docs. One sentence
  and a `.. versionchanged:: 1.1` on `partition_features` (and `partition_into_plates`) would do; the
  private helper rightly has no docstring.
- `PartitionError`'s message is usable from Python (probe H, *run*): `The overlay's classes are
  inconsistent after 20 repairs: 2 vertices have no consistent arrangement and 0 pieces are unresolved.
  The first is at latitude -21.309, longitude -91.9874, with records LS(2,12) SL(4,10) SS(4,12).` The
  record kinds are overlay jargon, but the message is for a report to a developer.

## 2. Test gaps

*Run*: three mutations, each rebuilt and run against `pygplates-test` (the module checked to carry the
mutation: `partition_features.__code__.co_names` loses `values` under M1; probe 6 under M2).

| Mutation | Committed suite | Why |
| --- | --- | --- |
| M1 the range properties not removed from the clone (`PlatePartitioning.py:346-347` reduced to the geometry test) | passes | `set_geometry` replaces or removes the range name itself (1.6); only a feature with two geometry property names tells the difference, and no test has one |
| M4 the measure `(arc_length, area, points)` (`:403`) | passes | a polygon contributes only area and a polyline only arc length (`:394-399`), so the order decides only a feature with both kinds of geometry; `test_most_overlapping_plate_by_area` is all polygons (its two measures are `(0, area, 0)`) |
| M2 `respect_feature_time_period` true (`PyPlatePartitioner.cc:428`) | passes | no test feature has a valid time excluding the partitioner's; under M2 probe 6's feature (valid 100-50 Ma) vanishes from both outputs |

Not run as mutations (by reading, the committed coverage test catches them): dropping the coverage from
a piece (`len(scalars) == len(points)` fails), reversing a piece's scalars (the original-vertex values
fail), skipping the two-step write (`set_geometry` raises `ValueError` on the exterior-ring-only range),
registering `PartitionError` under another base (`issubclass` fails), and the measure's area for polygons
(`test_most_overlapping_plate_by_area` fails).

Probe scripts (*run*, all since deleted): A/A2/A3/A4 (1.1, 1.3), B (1.2), C (1.5), D (two scalar types
through `partition_features`: both carried, `ScalarType` keys round-tripped), E (valid time), F/F2
(no geometry: the feature is in neither output, as before; orphan ranges), G (`GmlOrientableCurve`
under `gpml:centerLineOf`: partitioned and set back as `GmlOrientableCurve`), H (`PartitionError`
message, and raised from `partition_geometry` with and without lists, `partition_features` in both
methods, and `partition_into_plates`), J (1.1 with a polyline and with `gpml:Coastline`), K
(`partitioned_groups_and_unpartitioned` with a coverage at 10 Ma with a rotation: the piece is
reverse-reconstructed and keeps its coverage; `valid_time_period` copied), L (a multi-point and a
polyline with coverages: each point its own value; the polyline's cut point interpolated).

Inputs no committed test covers; each worth a test, the first three because they fail today:

1. **Two pieces of one coverage under one property name with the same point count** (1.1): probe A's
   input; expect two features (or one with two readable coverages), never an exception.
2. **A `GpmlPiecewiseAggregation` geometry property** (1.2): probe B's input; expect it partitioned as
   `get_all_geometries()` sees it.
3. **Two same-name geometry properties, one with a coverage** (1.5).
4. **Pieces of an exterior-ring-only coverage under one name** (1.3): probe A3 or A4; decide what is
   expected and pin it, since today's "kept form" reads back as no coverage.
5. **A feature outside its valid time at the partitioner's time** (M2): still partitioned.
6. **A feature with two geometry property names**, one wholly inside a plate and one wholly outside
   (M1): the unpartitioned feature carries no orphan range of the first.
7. **A feature with a polygon and a polyline** under `most_overlapping_plate`, where arc length and
   area disagree (M4).
8. **The `InformationModelError` fallback with coverages**: a single-occurrence geometry property cut
   into pieces of different counts (the fallback's clone-per-piece with a range each). Not probed: no
   feature type was found whose geometry property both allows a coverage and forbids repeats before
   the ambiguity check fires (probe J).
9. **`PartitionError` through `partition_features` and `partition_into_plates`** (probe H): the
   committed `test_partition_error` covers `PolygonOnSphere.partition` only; design 9.2 names the
   others.
10. **The count check's `AssertionError`** (`PlatePartitioning.py:33`): unreachable from committed inputs
    (it needs a range the C++ sized wrongly); by reading it fires only on a programming error, which is
    what it says.
11. **A polyline or multi-point with a coverage through `partition_features`** (probe L): fine today,
    unpinned.

## 3. Checked and found correct

- **The helper's contract** (`PyPlatePartitioner.cc:404-490`). `partition_feature` reads the feature and
  clones properties, modifying nothing (`PartitionFeatureGeometries.cc:772-801`, the visitor's
  `get_geometry_property` `:392-475`); the Python then clones the feature itself (`:342`). Per property
  name the 4-tuple is `(domain name, range name or None, [(ReconstructionGeometry, [piece])],
  [piece])`, a piece a geometry or `(geometry, {ScalarType: [float]})`; the keys are
  `GPlatesPropertyValues::ValueObjectType` converted to `ScalarType` (probe D: `ScalarType` keys,
  both tuple elements carried, each piece's values the originals at original vertices). Two properties
  of one name merge into one map entry (`PartitionFeatureGeometries.h:155`, `:447-456`) with their
  partitions appended, which the Python's grouping by name (`:331-339`) treats the same as before.
- **Point order and pairing in the common case.** A piece's range comes out in `get_geometry_points`
  order (`partition_range`, `:573-609`), which is `set_geometry`'s and `get_geometry`'s order; with one
  piece per feature, or pieces of different counts, `match_domains` pairs by count and both committed
  cases (`test.py:2189-2211`, `:2215-2237`) read back the right values. The exterior-ring-only trim
  (`:581-595`) sizes the range by the exterior ring, which the second `match_domains` pass reads as
  `exterior_ring_only` (`ScalarCoverageFeatureProperties.cc:234-257`), so a piece keeping its hole keeps
  the form (`test.py:2225-2237`, and probe A4's 8-point piece when alone).
- **The two-step write's count check** (`:26-34`): a full count passes as a coverage, an exterior-ring
  count on a polygon takes the two-step path, anything else is an `AssertionError`; `any(...)` (`:36`)
  sends a property's pieces together, so a piece without a hole (full count) and one with (exterior
  count) are set as geometries with one range each, which is the only way `set_geometry` lets them
  coexist (probe A4 sets them; the read-back is 1.3).
- **`partition_features` otherwise unchanged**: `properties_to_copy` (`:356-359`), `partition_return`
  groups (`:363-364`, probe K), reverse reconstruction after setting (`:426-431`; probe K: the coverage
  survives the rotation, the range count unchanged), an unpartitioned feature (probe A2), a feature with
  no geometry (in neither output, as at `354405908`: both versions produce no feature for it), the
  `InformationModelError` fallback (`:57-63`, unchanged in shape, now through
  `_plate_partitioning_set_geometry` with one-element slices). `most_overlapping_plate` untouched but
  for the measure.
- **`PartitionError`** (`PyExceptions.cc:482-494`): registered after `MathematicalError` with it as
  base, from `InconsistentOverlayException`, which is the only exception the overlay throws for an
  inconsistent cut (`PolygonOverlay.cc:2460`, `:3767`, `:3857`); `MathematicalError` is right (the class
  derives from `MathematicalException`, `PolygonOverlay.h:546-547`). Not caught anywhere on the way:
  `PolygonPartitioner.cc`, `GeometryCookieCutter.cc`, `PyGeometriesOnSphere.cc` and
  `PyPlatePartitioner.cc` have no handler for it (grep; the two `catch (const std::invalid_argument &)`
  in `PyGeometriesOnSphere.cc` are elsewhere); the one catch is `AssignPlateIds.cc:300`, GPlates'. Probe
  H: raised from every entry point the docstrings name, with and without lists, and from
  `most_overlapping_plate` (through `partition_geometry`, `:375`). The reference lists it under
  `MathematicalError` (`pygplates_reference.rst:532`), alphabetically.
- **Docstrings against the code.** Holes taking part, whole returns for a touch, a polyline along the
  outline inside, identical polygons inside: each is pinned by `test_partition_polygons_with_holes` and
  `test_partition_touching` (`test_geometries_on_sphere.py:1790-1838`). "the pieces outside one plate are
  cut by the next" is the cookie cutter's cascade (reviews 5 and 6). The measure's order is as the
  enum's docstring says (`PyPlatePartitioner.cc:428`, `PlatePartitioning.py:394-403`). Conventions:
  signature-first lines untouched; `:raises PartitionError:` after `:rtype:`, as the file's other
  `:raises:` are (`:670`, `:741`); no single backquotes; `.. versionchanged::` blocks indented two
  spaces inside the body; type fields unchanged. The stub matches (`pygplates-stub-test` passes at
  `877646cca`).
- **Changelogs.** Every item of design 8.6 is present in both files except `partition_geometry()`
  accepting coverages, which section 14 question 5 withdrew. Wording is user-facing. GPlates' coverage
  bullet says "on every ring"; pyGPlates' adds the exterior-ring-only sentence (see 1.3 for what it
  implies).
- **The closure.** `pygplates-source-closure-test` and `pygplates-linkage-test` pass at `877646cca`
  with the committed dependency matrix; `PartitionFeatureGeometries.h` includes only `GeometryCookieCutter.h`,
  `ReconstructionGeometry.h`, maths, model and `GmlDataBlockCoordinateList.h`, all already in the
  closure; its `.cc` adds `ReconstructionFeatureProperties.h` and `ScalarCoverageFeatureProperties.h`,
  both already there. `src/app-logic/CMakeLists.txt:98-99` lists both files.
- **Verbatim move**: 1.6.

## 4. Design statements to amend

- **8.4 as built**: the helper "returns, per geometry property, its name" is per property *name*
  (two properties of one name merge); and "sets the pieces back" needs the limits of 1.1 and 1.5 (one
  `set_geometry` per name), or the fix. "A piece of an exterior-ring-only coverage keeps that form ...
  as Assign Plate IDs writes them": add that two such pieces under one name with equal exterior-ring
  counts read back as no coverage, in both products (1.3). Add that time-dependent geometry properties
  are not seen by the shared visitor (1.2), in both products, where the old pyGPlates path saw them.
- **9.2**, pyGPlates: now settled; say the name `PartitionError`, base `MathematicalError`, and that
  `most_overlapping_plate` raises too (through `partition_geometry`).
- **11.5**: "`partition_features` preserving a coverage" is done; add the inputs of section 2 (items 1
  to 4 at least) and note that `partition_geometry` with a coverage was withdrawn.
- **PLAN.md step 13b**, "by reading, losing the coverage or the two-step write fails the new test": true;
  add that the three mutations run here (section 2) all survive, each by an equivalence the test's
  single-property, all-polygon inputs allow, and that `test_partition_point` lost its `def` (1.4).
