# PR B review 4: the feature level, and GPlates

Reviewed 2026-11-08 (Fable), at `c3f9804df` (the whole of PR B, `public/main...HEAD`, seen as one):
`src/app-logic/PartitionFeatureGeometries.{h,cc}` (the shared visitor `PartitionFeatureGeometryProperties`:
which properties are partitioned, `visit_gpml_piecewise_aggregation`, `get_geometry_property` and its clones,
`Range`, `partition_geometry`, `partition_geometries`, `partition_range`, `interpolate_ring_values`,
`partition_feature`), `src/app-logic/PartitionFeatureUtils.{h,cc}` (`GeometrySize`, `GeometrySizeMetric`,
`add_partitioned_geometries_to_feature`, `add_unpartitioned_geometry_to_feature`,
`find_partition_containing_most_geometry`, `reverse_reconstruct`, `PartitionedFeatureManager`),
`src/app-logic/GenericPartitionFeatureTask.cc`, `src/app-logic/AssignPlateIds.{h,cc}` (the catch,
`get_features_not_partitioned`, `get_feature_description`), `src/qt-widgets/AssignReconstructionPlateIdsDialog.cc`
(by reading: it cannot run headless), `src/cli/CliAssignPlateIdsCommand.cc`, the PR B entries of
`CHANGELOG-GPlates.md` and `src/unit-test/PartitionFeatureUtilsTest.cc`; against
`docs/design/architecture/foundation/spherical-geometry.md` sections 5.4 (provenance, scalar coverages, most
overlapping plate, time-dependent geometry), 5.5 (the widths) and 5.6, with the eight step reviews (6, 7 and 8
touched this area) and PR B reviews 1 to 3 for what is known. Where the lower layers were needed (the
multi-point and whole-geometry provenance in `PolygonPartitioner.cc`'s `GeometryPartitioner`, the cascade in
`GeometryCookieCutter::partition`, `ScalarCoverageFeatureProperties::match_domains`, the Python side of
`partition_features`) they were read for what this layer relies on, not re-reviewed. Line numbers are at
`c3f9804df`.

Evidence is marked *run* or *reading*. *Run*: seven temporary probe tests appended to
`src/unit-test/PartitionFeatureUtilsTest.cc` (a polygon of area below `GreatCircleArc`'s zero length and a
zero-length polyline under "most overlapping plate"; a multi-point, a polyline and an exterior-ring-only coverage
through a cascade of two plates; a feature with two same-named geometry properties, one with a coverage, a
two-window time-dependent geometry and a name, by both methods through `AssignPlateIds`; a single-window domain
with a constant range and with a two-window range; a range whose count does not match its domain; the
inconsistent polygon as the second of two geometry properties, by both methods; an original vertex lying on
the plate's outline), each run through `AssignPlateIds` or `partition_feature` and inspected by a dump of every
output feature's properties and of what `ScalarCoverageFeatureProperties::get_coverages` reads back from it;
and six mutations of `PartitionFeatureUtils.cc` and `PartitionFeatureGeometries.cc`, each applied to a freshly
restored file, rebuilt and run against `PartitionFeatureUtils*` (11 committed tests, plus the probes). All
removed afterwards by `git checkout` and a touch of the three files; `gplates` and the unit-test target rebuilt;
`PartitionFeatureUtils*:PolygonPartitioner*` green (29 tests) and the full suite green (`ctest`: 203 of 203,
`PolygonOverlayPrototypeTest.File` and `PolygonOverlayRealDataTest.Coastlines` skipped by design); `git status`
clean but for this file. Probe sources and the mutation script were kept in the session's scratchpad.

Baseline (*run*, before any change): `gplates-unit-test --gtest_filter=PartitionFeatureUtils*:PolygonPartitioner*`
29 tests, 29 passed.

No feature loses a property, a geometry, a coverage value or its plate ID, and none gains a duplicate, in any
case tried: the layer does what the page says. What was found is one disagreement between the products on a
degenerate input (1.1), one pre-existing hazard that the new polygon pieces make easier to hit (1.2), one gap in
the time-dependent rule's statement (1.3), stale comments, and the unpinned rule of the partitioned feature
manager (section 2). The fixes of reviews 6 (the ring walk), 7 (the single-window geometry) and 8 (the range
clone) hold, and each is pinned by a committed test (*run*, mutations M9, M3, M8 in section 2).

## 1. Findings, most severe first

### 1.1 "Most overlapping plate" never chooses a plate whose pieces all measure zero, so a polygon with edges under 1e-6 radians, or a zero-length polyline, inside a plate keeps its old plate ID; pyGPlates and the old GPlates give it the plate (run)

`find_partition_containing_most_geometry` (`PartitionFeatureUtils.cc:477-541`) starts the running maximum at
`GeometrySizeMetric()` (`(0, 0, 0)`, `:496`) and takes a partition only when its metric is strictly greater
(`:532`), with `real_t`'s comparisons (`operator<`, `:898-913`: `d_area != rhs.d_area` is "differ by more than
`EPSILON`", `1e-12`). A polygon whose every edge is under `GreatCircleArc`'s zero-length threshold
(`asin(sqrt(EPSILON))`, `1e-6` radians, about 6 metres; `GreatCircleArc.cc:326-346`) has `get_area() == 0`
exactly (`SphericalArea`'s fan drops zero-length edges, section 5.5), and a polyline of coincident points has
arc length 0; the metric is then `(0, 0, 0)`, not greater than the start, and `max_partition` stays none.

*Run*, probe `ProbeTinyGeometryMostOverlapping`: a box of `1e-5` degrees a side (`get_area()` prints `0`) wholly
inside the plate `box(-10, 10, 0, 10)`, and the polyline `(1, 5)-(1, 5)`: `partition_feature` reports one inside
partition and nothing outside, `find_partition_containing_most_geometry` returns none, and after
`AssignPlateIds` with `ASSIGN_FEATURE_TO_MOST_OVERLAPPING_PLATE` the feature still has plate ID 5. The Python
branch of `partition_features` (`PlatePartitioning.py:425-436`) starts its maximum at `(0.0, 0.0, -1)` exactly so
that a zero measure still wins (its comment says why), so pyGPlates assigns the plate. The old GPlates compared
arc length (`public/main`'s `operator<`, `d_using_arc_distance`), which for the tiny box is `7e-7 > 1e-12`, so it
assigned the plate too; for the zero-length polyline it did not.

Degenerate input (polygons under about 6 metres across, polylines of one position), so minor; recorded because
the two products now answer differently and the page says the measure is the same. Fix: take the first
partition that has any inside piece and replace it only on a strictly greater metric (`if (!max_partition ||
partition_size_metric > max_partition_size_metric)`), which is what the Python's `-1` start does; or start
the maximum below zero. The probe is the test.

### 1.2 A range property that matched no coverage is carried into every piece feature, and is read back as the coverage of any piece whose point count happens to equal its (run; pre-existing mechanism)

`get_geometry_property` (`PartitionFeatureGeometries.cc:479-522`) reports as partitioned (`d_partitioned_properties`)
the domain and, when `get_coverages` matched one, its range. A range that `ScalarCoverageFeatureProperties::match_domains`
could not match (its count equals no domain's, or two same-named domains have its count, which it warns about and
leaves) is neither reported nor removed: `GenericPartitionFeatureTask::partition_feature` removes only the
reported properties (`:68-71`), then clones the feature (`PartitionedFeatureManager`, `:756`) for every piece
feature, so the orphan range appears, unchanged, in every output feature beside pieces of new point counts.
`match_domains` matches by count, so whichever piece has the orphan's count takes it as its coverage.

*Run*, probe `ProbeOrphanRange`: a triangle with plate ID 5 and a `gpml:unclassifiedGeometryCoverage` of four
values (which `get_coverages` does not match: `coverages read back: 0` before), cut by the plate
`box(0, 40, -20, 20)`. After `PARTITION_FEATURE`: the inside piece (plate 100, 3 points) and the outside piece
(plate 5, 4 points) each carry the orphan range, and the outside feature reads back `1 coverage [4pts, 4vals]`:
the four values now colour a polygon they never described. The ambiguous form is the same (*run*,
`ProbeMixedFeature`'s first version: two 4-point same-named polygons and one 4-value range; the range rides into
every piece feature; here each got two 4-point pieces so it stayed ambiguous, but one 4-point piece alone would
have taken it).

Pre-existing: the old visitor reported the same properties and the clones carried orphans the same way; and the
old polyline pieces had varying counts too. Recorded because the page (5.4) says the two products "cannot
disagree" on which properties are removed, and they do here: pyGPlates' `set_geometry` without a coverage removes
every property of the range name (review 7, 1.6), so its piece features carry no orphan. Fix, if wanted: when a
domain is partitioned, also report (and so remove) the range properties of its range name that matched no
coverage, with a `qWarning` naming the feature, since the data was already unreadable as a coverage; or leave
them on one feature only. Either way, the probe is the test.

### 1.3 A single-window domain with a range of more windows: the domain is written constant beside a still time-dependent range (most overlapping), or the range's other windows are dropped (partition) (run)

`get_geometry_property` (`:495-512`) writes the range clone constant only when the range itself is a
`GpmlPiecewiseAggregation` of exactly one window. With a single-window domain and a two-window range (one window
containing the time), the clone is the original two-window range. *Run*, `ProbeSingleWindowDomainRangeForms`,
form 1: after "most overlapping plate" the feature has `unclassifiedGeometry[CV]` and
`unclassifiedGeometryCoverage[TD]`; after `PARTITION_FEATURE` each piece has a constant range built from the
current window's values, and the other window's values are gone (the range property was reported as
partitioned, `:517`, and removed). The same drop happens, pre-existing, for a *constant* domain with a
multi-window range: pieces take the current window's values. All consistent with "its pieces, and a
single-window coverage's range, are written back constant" (5.4), which just does not say what happens to a
range with more windows than its domain; a geometry that exists only in one window cannot have had values
outside it, so dropping them loses nothing meaningful, but it is silent. Rare (GPlates writes coverages as
constants). Say so on the page, or write such a range back constant too (its current window's values) for
consistency with the pieces. Form 0 (constant range) is as the page says (*run*: constant beside constant, read
back as one coverage), and the committed `AssignPlateIdsTimeDependentGeometry` pins the single-window range
(*run*, mutation M8 fails it).

### 1.4 Minor (reading unless marked)

- **Stale comments contradict the measure.** `PartitionFeatureUtils.h:283-284` (`find_partition_containing_most_geometry`:
  "based on arc distance of the partitioned geometries if they are line geometries (polyline, polygon) or number
  of points") and `:401-403` (`GeometrySizeMetric`: "for polylines/polygons this is arc distance") describe the
  old measure; `:412-415` and `:430-434` beside them describe the new one.
- **The guarantee the catch relies on is not in the task interface.** `AssignPlateIds.cc:299-305` says "a task
  partitions every geometry of the feature before it modifies the feature, so the exception leaves it untouched";
  that is true of `GenericPartitionFeatureTask` (`:48-71`: `partition_feature` runs to completion before the
  removal loop; *run*, `ProbeExceptionSecondProperty`: with the inconsistent polygon as the second of two
  geometry properties, by both methods, the feature keeps its property count, plate ID 5 and its coverage, and is
  reported once) and vacuous for `VgpPartitionFeatureTask` (`partition_point` cannot throw), but
  `PartitionFeatureTask.h:86-99` does not state it, so a future task could break the contract silently. One
  sentence there.
- **Zero-measure tie-break and `real_t`.** Two plates whose areas differ by under `1e-12` steradians (40 square
  metres) are equal to `operator<` and fall through to arc length and points; the first in the cookie cutter's
  order then keeps the maximum. Fine, but 5.4's "lexicographically with `real_t`'s comparisons" is the only
  place that says it (review 6 noted the non-transitivity; nothing to change).
- **A feature with polylines in plate A and a polygon in plate B goes to B whatever the lengths**, since area
  is compared first (*run*, committed `SizeMetricByArea`'s second case; mutation M6 shows the suite pins it).
  The page and both changelogs state the order, so this is the decision, not a bug; noted because the old code
  gave A (arc length, the polygon's included), and a user with mixed-type features will see the change.
- **The dialog's plural.** `tr("%n feature(s) could not be partitioned ...", nullptr, n)`
  (`AssignReconstructionPlateIdsDialog.cc:551-555`): with `%n` the "(s)" is for the untranslated form, which Qt
  shows as "1 feature(s)". Cosmetic.
- **Changelog units.** "where its outline runs within about 1e-6 radians of the plate's" (`CHANGELOG-GPlates.md`,
  the last Assign Plate IDs bullet): the other bullets speak in metres ("about 9 metres", "about 90 m"); "within
  about 10 metres" matches them.
- **The time-dependent geometry left unchanged is copied into every piece feature** (*run*, `ProbeMixedFeature`:
  the two-window `unclassifiedGeometry[TD]` is on all four piece features under `PARTITION_FEATURE`, and on
  both under most overlapping), each with a different plate ID, so it is then drawn once per piece feature. This
  is what "stays unchanged on every feature returned" (5.4) and "left unchanged, as before" (the changelog) mean,
  and the old code did the same; a user reading the changelog may not realise "unchanged" means "duplicated".
  One clause.
- Style: hard tabs, Allman braces, broken signatures throughout the new code; the GPL header on the new files;
  include groups in order. `PartitionFeatureUtils.cc`'s includes were swept (review 7's list is gone).

## 2. Test gaps

*Run*: six mutations, each on a freshly restored file, rebuilt and run against `PartitionFeatureUtils*` (the 11
committed tests with the seven probes present; a probe failing alone is noted).

| Mutation | Committed suite | Note |
| --- | --- | --- |
| M1 `PartitionedFeatureManager::get_feature_for_partition`: the range rule removed (`:805-806` made `if (true)`, so any feature holding the name is reused) | **passes** (18 of 18 with probes) | two coverage pieces then share a feature, where `match_domains` reads neither back; no committed test has two coverage pieces in one plate, or a coverage piece beside a same-named plain piece |
| M2 `add_partitioned_geometries_to_feature`: `geometry_domain_has_associated_range` always false (`:237-238`) | caught (`AssignPlateIdsTimeDependentGeometry`, `num_coverages`) | |
| M3 `visit_gpml_piecewise_aggregation`: the window's `contains(time)` check dropped (`:169-171`) | caught (`AssignPlateIdsTimeDependentGeometry`) | review 7's 1.2 fix is pinned |
| M6 `GeometrySizeMetric::operator<`: area not compared (`:904-907`) | caught (`SizeMetricByArea`, `AssignPlateIdsTimeDependentGeometry`) | |
| M8 `get_geometry_property`: the single-window range cloned as the original (`:509-512`) | caught (`AssignPlateIdsTimeDependentGeometry`) | review 8's 1.4 fix is pinned |
| M9 `interpolate_ring_values`: the polyline end rule run for closed rings too (`:829`) | caught (`RunAcrossRingStart`) | review 6's 1.1 fix is pinned |

Inputs no committed test covers, each run here as a probe (all since removed):

1. **The manager's rule** (M1): a feature with two same-named geometry properties, one with a coverage, cut by
   two plates (`ProbeMixedFeature`): expect each coverage piece in a feature of its own, the plain pieces of
   one plate together, and `get_coverages` reading back exactly one coverage per coverage-piece feature. Today
   the probe only prints this; it should assert it.
2. **Zero-measure most overlapping** (1.1): the tiny box and the one-position polyline; decide the answer
   (pyGPlates' is the plate) and pin it.
3. **An orphan range** (1.2): the triangle with a four-value range; pin whatever is decided.
4. **A single-window domain with a multi-window range** (1.3), by both methods.
5. **Coverages through a cascade for the other types** (*run*, `ProbeCascadeOtherTypes`, all right): a
   multi-point of five points and a three-vertex polyline across two plates, every value equal to its point's
   longitude; and an exterior-ring-only coverage whose hole is opened by the *second* plate (the hole's vertices,
   whose composed locations are on ring 1, get no value and are interpolated along the run: `8 8 3.16 1.58 -1.58
   -3.16 -8 -8`). The committed `OtherGeometryTypes` and `ExteriorRingOnlyCoverage` use one plate.
6. **An original vertex on the plate's outline** (*run*, `ProbeVertexOnClipOutline`): the plate's left edge
   along longitude 0 through `P`'s vertex `(10, 0)`; the piece point there has exactly `10`, the points on
   `P`'s meridian edges their latitudes, and the plate's corner inside `P` its arc-length interpolation
   (`0.95`). The committed tests have crossings and cut points but no touch at a vertex.
7. **The exception in a feature's second geometry property, by both methods** (*run*,
   `ProbeExceptionSecondProperty`): untouched and reported once. The committed
   `AssignPlateIdsSkipsInconsistentFeature` has one property and one method.
8. **`get_feature_description` with a name**: the committed test checks only that a nameless feature's
   description is not empty; the `name (id)` form is unpinned.
9. **The dialog's warning, and on cancel; the command line's `qWarning`**: not testable headless; by reading
   (section 3).

## 3. Checked and found correct

- **Which properties are partitioned** (`PartitionFeatureGeometries.cc:143-179`): a constant geometry; a
  `GpmlPiecewiseAggregation` of exactly one window whose `valid_time()->contains(time)` (inclusive at both ends,
  `GmlTimePeriod.h:210-215`, as the coverage visitor's test is); anything else, and a `GpmlIrregularSampling`
  (the base visitor's no-op), left alone and not reported. The committed `AssignPlateIdsTimeDependentGeometry`
  covers one window in and out of time, two windows, and the coverage; M3 confirms the pinning.
- **The clones** (`:461-512`): a single-window domain's clone is a constant property through
  `create_top_level_property` with `check_property_value_type = false` (the GPGIM's wrapper, `[CV]` in the
  dumps), falling back to a bare `TopLevelPropertyInline` for an unknown name; the range's likewise when it is
  a one-window aggregation (`:506-512`; review 8's 1.4), else the original (1.3). The window that was found is
  the one containing the time, since `ScalarCoverageFeatureProperties` visits only those.
- **Every geometry is partitioned before the feature is touched**: `partition_feature` (`:847-877`) only
  visits; the task removes and adds afterwards (`GenericPartitionFeatureTask.cc:48-100`); *run*,
  `ProbeExceptionSecondProperty`, both methods. The catch (`AssignPlateIds.cc:290-306`) is per feature and per
  task, records the feature once, and `return`s; the `NotificationGuard`s unwind. `partition_geometries`
  (plural) is not on this path.
- **`partition_range`** (`:640-746`): the piece's points are its exterior ring's alone under an
  exterior-ring-only coverage (`:652-671`) and all of them otherwise, in `get_geometry_points` order, which is
  the provenance's; the assertion `provenance->size() >= points.size()` allows the truncation. Step 1 reads a
  vertex's scalars at `ring_offsets[ring] + index` and a segment's by interpolation with the closing segment
  wrapping (`get_segment_value`, `:296-324`); a location on a hole of the original under an exterior-ring-only
  coverage gets none (`ring >= ring_sizes.size()`), so step 2 interpolates it (*run*, probe 5, and the committed
  `ExteriorRingOnlyCoverage`). Step 2 (`interpolate_ring_values`, `:754-840`) walks once round from the first
  valued point and ends on it, so every point is set; the polyline end rule is guarded on `!closed` (M9).
  Step 3's nearest outline: `minimum_distance`'s segment index is global across the rings (the committed
  `NearestOutlineOnHole` would fail otherwise), the ratio is the projection's angle fraction, clamped. Per-tuple
  scalars written per element with the element's type and attributes (`:728-745`).
- **Points, multi-points and polylines through the cascade** (*run*, probe 5): `GeometryPartitioner` gives a
  split multi-point's pieces their points' own tables (`PolygonPartitioner.cc:313-367`, `input_point_provenance`),
  and a whole geometry its input table or the identity (`add_whole`, `:516-535`), so a point's value is its own
  at every step; a polyline's cut points interpolate along the equator exactly.
- **Original vertices are exact**: a vertex of the original keeps its scalars whether it is a plain vertex, a
  vertex on the plate's outline (a touch record; *run*, probe 6) or a vertex of a hole now on an exterior ring
  (the committed `CoverageAcrossCut`'s `100..103`).
- **The partitioned feature manager** (`PartitionFeatureUtils.cc:750-868`): a piece with a range always gets a
  feature in which its name is absent; a plain piece never joins a feature where its name has a range; plain
  pieces of one plate share a feature (*run*, `ProbeMixedFeature`: `0: [CV](4pts) + Coverage[CV]`,
  `1: [CV](4pts) + Coverage[CV]`, `2: [CV](4pts)`, `3: [CV](5pts)`, every coverage read back; `gml:name` on each;
  the original feature claimed first, the clones made after the removal loop so no piece feature carries an
  original geometry). Under most overlapping the same rule splits a feature whose two same-named geometries
  include one coverage into two features of the same plate (*run*): the old behaviour, and the ambiguity it
  avoids is real (1.2).
- **Most overlapping** accumulates per partition across properties through a map, replaces the maximum only on
  a strictly greater metric (so an earlier plate keeps a tie), and area, then arc length, then points (M6;
  `SizeMetricByArea`). `get_area()` is `abs(get_signed_area())`, holes subtracted for the overlay's orientation.
  The tie-break and the zero start are 1.1 and 1.4.
- **Reverse reconstruction of pieces**: `get_feature_for_partition` runs before `reverse_reconstruct`
  (`:244-256`), so the piece feature already has the plate ID the assigner gave it; the "outside" feature keeps
  the original's. The range is per point and untouched by it. The unpartitioned (most overlapping) path sets the
  reverse-reconstructed geometry back into the clone (`:449-472`) and adds the range clone beside it.
- **The dialog** (`AssignReconstructionPlateIdsDialog.cc:290-318`, `:537-568`): the warning is shown whenever
  the list is non-empty, before `completed` is returned, so also after a cancel (review 6's note is fixed);
  the cancel check is between features (`:519-528`), and a feature is modified only after its partitioning
  succeeded, so no feature is half-done; `apply()` keeps the dialog open on `false`. Up to ten features by
  `name (id)` or id, the rest counted. **The command line** writes one `qWarning` per feature after all
  collections (`CliAssignPlateIdsCommand.cc:392-398`); the one `AssignPlateIds` object accumulates across them.
- **The changelog's PR B bullets** (`CHANGELOG-GPlates.md:45-53`) are accurate against the code: polygons into
  polygons with holes; holes of plates and features taking part; touching not splitting (a polyline along a
  shared boundary to the first plate, review 3's probe); coverages on every ring, the closing segment, the
  nearest outline; the `10 h` sliver filter; the single-window geometry and its single-window range; the
  measure's order; the skipped features listed at the end. Wording in 1.4.
- **The page's 5.6 statements** on Assign Plate IDs hold: the catch, `get_features_not_partitioned`, the
  untouched feature, the dialog's one warning also on cancel, the command line's `qWarning`.

## 4. Page and changelog statements to amend

- **5.4 "Most overlapping plate"**: add that a plate whose pieces all measure zero (a polygon of edges under
  `1e-6` radians, a polyline of one position) is never chosen by GPlates while pyGPlates chooses it (1.1), or
  fix 1.1 and say both choose it.
- **5.4 "Time-dependent geometry"**: "a single-window coverage's range too" holds for a range of one window; a
  range with more windows than its single-window domain stays time-dependent under most overlapping and is
  reduced to its current window's values in the pieces (1.3). State it, or make the range constant too.
- **5.4 "pyGPlates' `partition_features` removes exactly the properties it reports as partitioned, so the two
  products cannot disagree"**: true of the reported properties; they disagree on an unmatched range of a
  partitioned domain's range name, which GPlates carries into every piece feature and pyGPlates' `set_geometry`
  removes (1.2).
- **5.6**: the "task partitions every geometry before it modifies the feature" guarantee is
  `GenericPartitionFeatureTask`'s, not the interface's (1.4); worth a clause, and a sentence in
  `PartitionFeatureTask.h`.
- **`CHANGELOG-GPlates.md`**: "within about 1e-6 radians" to metres; "Other time-dependent geometries are left
  unchanged, as before" could add "on every feature the cut produces" (1.4).
- **`PartitionFeatureUtils.h:283-284` and `:401-403`**: the arc-length comments (1.4).
