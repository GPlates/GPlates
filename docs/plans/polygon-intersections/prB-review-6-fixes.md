# PR B review 6: the fixes of the five area reviews

Reviewed 2026-11-08 (Fable), at `b41aa8223`: the eight commits `6aee9fc85..b41aa8223` (seven code commits,
`3180ea66e` to `bb28c7824`, and the page and plan commit `b41aa8223`), each against the finding of
`prB-review-1-overlay-classification.md` to `prB-review-5-pygplates.md` it fixes, against the design as
built (`docs/design/architecture/foundation/spherical-geometry.md` section 5, as amended by `b41aa8223`)
and against `PLAN.md`'s Status entry of 2026-11-08 ("The full PR B review, by area"). Files read in full
around every hunk: `src/maths/PolygonOverlay.{h,cc}` (`build_stops_and_pieces`, `apply_loop_and_spike_rules`,
`find_common_pairs`, `find_self_touch_cuts`, `classify`, `find_reference_point`, `check_whole_result`,
`classify_polyline_pieces`, `get_common_chain`, `classify_anchorless_chain`, `extract`, `extract_polylines`),
`src/maths/PointInPolygon.{h,cc}` (`get_crossing_parity_excluding_segment`),
`src/app-logic/PartitionFeatureGeometries.cc`, `PartitionFeatureUtils.{h,cc}`, `GeometryCookieCutter.cc`
(the early return), `src/api/PyFeature.cc` (`set_geometries`, `feature_handle_set_property`),
`src/model/ModelUtils.cc` (`set_properties`), `src/api/PyPlatePartitioner.cc`
(`_partition_feature_geometries`), `src/api/PyReconstructionGeometries.cc` (hashing),
`src/app-logic/ScalarCoverageFeatureProperties.cc` (the time-window visit), `PlatePartitioning.py`, the
four test files and the two pyGPlates test files. Line numbers are at `b41aa8223`.

Evidence is marked *run* or *reading*. *Run*: ten mutations of the fixes, applied in three batches to freshly
restored files, rebuilt (`gplates-unit-test`, `--parallel 12`) and run against
`PolygonOverlay*:PolygonPartitioner*:PartitionFeatureUtils*`; two pyGPlates mutations of
`PlatePartitioning.py`, rebuilt and run through `pygplates-test`; one temporary probe test appended to
`PolygonOverlayTest.cc` (an annular and a C-shaped subject through `check_whole_result`); and six pyGPlates
probe scripts importing `build-pygplates/bin` (unpartitioned same-named properties in four forms, a
time-dependent coverage outside its window, most overlapping across two property names, `NaN`). All
removed afterwards by `git checkout` and a touch of each file; both products rebuilt; the GPlates suites
and the full pyGPlates `ctest` green (counts at the end of this header); `git status` clean but for this file.
Probe sources and mutation notes were kept in the session's scratchpad.

Baseline (*run*, before any change): `gplates-unit-test --gtest_filter=PolygonOverlay*:PolygonPartitioner*:PartitionFeatureUtils*`
99 tests, 97 passed, 2 skipped (`PolygonOverlayPrototypeTest.File`, `PolygonOverlayRealDataTest.Coastlines`,
by design). Final (*run*, after every restore): the same 99 tests, 97 passed, 2 skipped; pyGPlates `ctest`
7 of 7 (`pygplates-test`, `pygplates-stub-test` and the rest).

Mutations (*run*): M1 the clip's parity counted at the unprojected midpoint (`ClassifyAnchorlessNearCopy`
fails); M3 the spike rule restored for polylines and M4 the along test for an unpaired single arc removed
(`PolylineHairpin` fails, across and along); M5 the self-touch cuts keyed by vertex again (**nothing fails**);
M2b `classify()` not calling `check_whole_result` (**nothing fails**); M6 the first partition no longer
taken on a zero measure (`AssignPlateIdsFeatureCases` fails, the no-measure polygon); M7
`has_range_in_other_windows` ignored (`AssignPlateIdsTimeDependentGeometry` fails); M8 the orphan-range
removal off (`AssignPlateIdsFeatureCases` fails, the range matching no geometry); M9 the kept properties
not put back (`test_partition_features_single_time_window` fails at its first new assertion); M10 the most
overlapping branch back on `get_all_geometries()` (confirmed on the M9+M10 module by a probe of the
committed test's input: one feature partitioned where the test asserts none, so the test's second new
assertion fails; M9's failure stops the method before it).

Three of the fixes have a hole each, all in the same corner (fixes 2, 5 and 6; section 1); the other five
fixes do what the findings asked, and seven of the eight are pinned (section 3).

## 1. Findings, most severe first

### 1.1 The orphan-range removal (fix 6) removes the range of a time-dependent coverage that is kept: a kept domain loses its values (run)

`PartitionFeatureUtils::partition_feature`, `PartitionFeatureGeometries.cc:910-962`. After the visitor, every
property under a partitioned geometry name's range name that is not a matched coverage's range
(`geometry_coverages`) and not already reported is reported as partitioned, with a `qWarning`. The matched
coverages come from `ScalarCoverageFeatureProperties::get_coverages(..., reconstruction_time)`, whose visitor
takes from a `GpmlPiecewiseAggregation` only the window containing the time
(`ScalarCoverageFeatureProperties.cc:272-292`) and matches domains to ranges by point count. So two kinds of
range that belong to an *unpartitioned* time-dependent domain, which section 5.4 says "stays unchanged on
every feature returned", are "unmatched" and removed:

- a range with no value at the reconstruction time (its windows exclude it). *Run*, probe P3, both products'
  code path (`_partition_feature_geometries` calls the same `partition_feature`): a constant
  `gpml:unclassifiedGeometry` polygon across plates 1 and 2, a second `gpml:unclassifiedGeometry` in one
  window 50-10 Ma, and a `gpml:unclassifiedGeometryCoverage` in the same window, partitioned at 0 Ma. The
  reported indices are `[0, 2]`: the constant polygon and **the time-dependent range**. Every piece feature
  has the time-dependent domain (fix 5 puts it back) and **no range**: the coverage's values for 50-10 Ma
  are gone from the file, and its domain is left behind alone.
- a range whose window does contain the time but whose domain has the same point count as another domain of
  the name (`match_domains` warns and leaves both unmatched). *Run*, probe P1: the constant 4-point polygon
  beside a two-window 4-point domain and a two-window range (first window containing 0 Ma): indices
  `[0, 2]`, the two-window range removed, its domain kept. With a constant range on the constant polygon as
  well (P1b, review 5's A2) all three ranges are unmatched and reported, `[0, 2, 3]`, and the pieces come out
  with no coverage at all; before this fix GPlates copied both ranges into every piece and pyGPlates dropped
  them, so for GPlates the ambiguous constant coverage goes from "carried, ambiguously" to "deleted".

The first kind is the one that matters: the data is well formed, GPlates reads it, and the removal is the
only thing that touches it. It needs a same-named constant geometry partitioned beside it, so it is rare,
but it is exactly the configuration fix 5 was made for, and the loss is silent but for a `qWarning` that
says the range "matched no geometry", which is not what happened. The page's claim (5.4) that the
unpartitioned time-dependent geometry "stays unchanged" is false for its range, in both products.

**Fix.** Exclude from the removal any range property whose value is time-dependent (a
`GpmlPiecewiseAggregation` or `GpmlIrregularSampling` under `ModelUtils::get_property_value`), since the
visitor never partitions a time-dependent coverage that is not in a single window containing the time, and
the rule for those is to keep them; remove only a constant range that matched no domain, which is what the
finding (review 4, 1.2) described and what the `qWarning` says. The ambiguous constant case (P1b's constant
range) is then still removed, as decided. P3 is the regression test, in `PartitionFeatureUtilsTest.cc`
(`add_time_dependent_feature` with `coverage_windows = { { 50, 10 } }` beside a constant geometry) and in
`test_partition_features_single_time_window`.

### 1.2 Fix 5 puts back a kept range only beside a partitioned coverage: beside a partitioned plain geometry it is lost (run)

`_plate_partitioning_set_geometries`, `PlatePartitioning.py:90-100`: the plain-geometry path wraps
`feature.set_geometry(geometry_or_geometries, geometry_property_name)` with the keeper and gives it
`[geometry_property_name]` only. But `set_geometries` without coverages removes every property of the
*range* name too (`PyFeature.cc:1263-1277`, "so that the geometries are not interpreted as coverage
domains"), so a kept time-dependent range that shares the range name is removed and not put back. The
coverage path (`:49-59`) passes both names and is right.

*Run*, probes P6a and P6b, which differ only in whether the partitioned constant polygon has a coverage: a
constant `gpml:unclassifiedGeometry` 4-point polygon across plates 1 and 2, beside a two-window 3-point
triangle domain and its two-window 3-value range (so `get_coverages` matches the time-dependent coverage,
counts differ, and 1.1 does not apply; indices `[0]` or `[0, 1]` as expected). With a coverage on the
polygon, every piece feature has `unclassifiedGeometry[TD2] unclassifiedGeometryCoverage[TD2]` (kept, as
the fix intends). Without one, every piece feature has `unclassifiedGeometry[TD2]` and **no range**. Same
loss as review 5's 1.1, one branch over.

**Fix.** Pass the range name on the plain path too. `range_property_names` knows a name only where a
partitioned geometry of that name had a coverage, so have `_partition_feature_geometries` return the range
name of every geometry property (`get_range_property_name_from_domain`, which the C++ already calls for the
removal in 1.1) rather than only of those with a coverage; `_plate_partitioning_set_geometries` then uses it
for both paths. Nothing else reads `range_property_names`. P6b is the test.

### 1.3 The whole-answer hemisphere check (fix 2) is silently skipped for a subject whose first clear reference candidate is outside it: an annulus, a crescent, a thin needle (run)

`check_whole_result`, `PolygonOverlay.cc:2145-2154`. `find_reference_point` returns the *first* of ten
candidates clear of both outlines, the subject's bounding-circle centre first (`:2107-2134`); if that point
is not in the subject the check returns, with nothing counted. The remaining candidates are never tried.

*Run*, probe `ProbeCheckWholeResultAnnulus`: an annular subject (a square with a square hole, both centred)
inside a larger clip, `classify()` right (`RESULT_INSIDE`), then `check_whole_result(RESULT_OUTSIDE)`:
**no throw, `hemisphere_check_failures = 0`, `hemisphere_check_skipped = 0`**. A C-shaped subject (its
centre outside it) wholly outside a small clip, given `RESULT_INSIDE`: the same. The committed
`CheckWholeResult` uses two convex squares, where the centre is inside, so it cannot see this.

This matters more than its shapes suggest. The class the check was added for is a thin subject coinciding
with a near-copy (review 1, 1.1), and review 1 recorded that at `w = 1e-5` the subject's centre is within
`10 h` of the outline, the first clear candidate is its antipode, "outside both", and the extract check
passed vacuously; `check_whole_result` inherits exactly that: at the committed test's narrower width the
check is a no-op, and the test passes on the classification alone. So the one guard section 5.6 now
promises for a whole answer ("a point of it clear of both outlines must be on the side the answer says")
covers convex subjects fat enough for their centre to clear the margin, and nothing records when it does
not. Not a wrong answer by itself (the classes decide); a missing safety net, and a diagnostic that says
"checked" when it was not.

**Fix.** In `check_whole_result`, walk the candidates (the same list, in order) until one is both clear of
both outlines and inside the subject; count `hemisphere_check_skipped` when none is, so that the hunt and
the diagnostics see the gap. `find_reference_point` can take a predicate, or return the whole clear list.
Then the annulus and the C-shape throw on a wrong answer (their centres' antipodes and the axis points
are not inside them either, so the walk may still skip; the test should then assert the skip is counted).
Also a test gap (section 2): nothing pins that `classify()` calls the check at all (M2b survives).

### 1.4 Minor (reading unless marked)

- **Fix 1's fallback keeps the old failure mode.** When no arc of the chain has a clip partner with a
  direction (`PolygonOverlay.cc:3122-3151`), `clip_axis` is the subject's (`:3162-3164`), so the clip's
  parity is again counted at a point off the clip's own great circle (which does not exist for a segment
  under `1e-6` radians), beside the clip's neighbouring segments. Reached only when every clip segment along
  the chain is that short, so a degenerate clip; the comment says "failing that, any arc with a direction"
  without saying that the clip's test point is then the subject's. Say so there and on the page.
- **Fix 1's test point is the subject's midpoint.** `X` is the midpoint of the *subject* piece's stops
  (`:3165-3167`); the clip piece's stops at the same identity vertices can be displaced along the chain by
  the cluster's width (two subject vertices within `h` of one clip vertex make one vertex with two subject
  stops), so `X_Q` can still lie past the end of a clip segment no longer than that width. The failing
  configuration of review 1 needs a `2 h` clip segment; this needs a cluster at its end as well. Not
  constructed; a limit worth one sentence beside the projection.
- **The put-back loses a property's XML attributes and moves it to the end** (`PlatePartitioning.py:26-30`:
  `feature.add(name, property.get_time_dependent_value(), VerifyInformationModel.no)` builds a fresh
  property from the value alone). GPML top-level properties rarely carry attributes; order is undocumented.
  Cosmetic.
- **Changelog units.** `CHANGELOG-GPlates.md` now says "within about 9 m"; `CHANGELOG-pyGPlates.md:42` still
  says "within about 1e-6 radians" for the same `PartitionError` condition. One clause.
- **The `qWarning`'s wording** ("coverage range matching no geometry") is right for the constant orphan and
  wrong for 1.1's time-dependent range; with 1.1 fixed it is right again.

## 2. Test gaps

Mutations are listed in the header. Gaps:

- **`classify()` to `check_whole_result`** (*run*, M2b survives): `CheckWholeResult` calls
  `check_whole_result` directly with a wrong result, so removing the call from `classify()` fails nothing.
  No committed input gives a wrong whole answer with the fixes in, so pin the wiring instead: the
  `ClassifyAnchorlessNearCopy` subject at `w = 1e-4` with `throw_if_inconsistent = false` and the clip's
  class forced wrong is not reachable through `classify()` either. The cheapest pin is a counter: a
  diagnostic `whole_checks` incremented when `check_whole_result` tests a point inside the subject, asserted
  `1` after `classify()` in `CheckWholeResult` (and `0` in 1.3's annulus until 1.3 is fixed).
- **The self-touch cuts keyed by stop** (*run*, M5 survives): review 1's 1.2 said the input was not
  constructed, and it still is not; the key change is right by reading (section 3) and unpinned. Record it
  as such on the page rather than under "each pinned by a test".
- **The whole check on a holed or non-convex subject**: 1.3's probe (an annulus and a C-shape, each given
  the wrong answer; with 1.3 fixed, a throw or a counted skip).
- **A time-dependent coverage outside its window beside a partitioned constant geometry**: 1.1's P3, in both
  test suites.
- **A kept range beside a partitioned plain geometry**: 1.2's P6b, in `test_partition_features_single_time_window`
  next to the committed case (which has no range).
- **The `InformationModelError` retry path with a kept property** (reading only: `ModelUtils::set_properties`
  checks multiplicity before touching the feature, `ModelUtils.cc:778-795`, so the keeper's list is simply
  discarded and rebuilt on the retry; no 0..1 line or polygon property was found in the GPGIM to drive it
  from a test, and `gpml:centerLineOf` accepts two geometries (*run*)).
- **Most overlapping across two property names** (*run*, probe P4): two polygons under two names in plate 1
  and one larger than each in plate 2 give plate 1, and the plate keys are shared across names (2 distinct
  for 3 pieces), because `ReconstructionGeometry` hashes by C++ identity
  (`PyReconstructionGeometries.cc:523` and siblings). Right, and not pinned by a test; the committed case has
  one property.

## 3. Checked and found correct

Fix by fix, against the finding and the decision in `PLAN.md`:

1. **`classify_anchorless_chain`** (`3180ea66e`; review 1, 1.1). Each input's parity is counted to the chord
   midpoint projected onto its own segment's great circle (`:3189-3190`), that segment excluded; the
   projection lies within the segment (the stops lie within `h` of it and the piece is interior to it), so
   the arc from `R` meets the excluded great circle only there and `PointInPolygon.h`'s amended argument
   holds. The degeneracy guard uses the input's own axis and test point (`:3194-3196`). The reference points'
   sides are compared each against its own axis, the clip's sign aligned by `dot(subject_axis, clip_axis)`
   (`:3222-3227`), which is right whether the partner runs with or against the subject. The two-pass choice
   prefers an arc whose partner has a direction and falls back to any (`:3122-3151`; 1.4 for the fallback).
   `ClassifyAnchorlessNearCopy` does what its comment says (vertex moved by `a H` sideways, one inserted
   `2 H` along), asserts `RESULT_INSIDE`, one anchorless resolution and no repair, and fails without the
   projection (*run*, M1). `ClassifyLLRule`'s octagram is eight `LL` records with no midpoint test, as
   claimed (*run*, passes; it is review 1's gap, not a fix).
2. **`check_whole_result`** (same commit; review 1, 1.1's partitioner half). Called from `classify()` for
   `RESULT_INSIDE` and `RESULT_OUTSIDE` (`:2096-2102`), the only entry `PolygonPartitioner` uses
   (`PolygonPartitioner.cc:125`, `:205`), so every caller gets it; a polyline returns before it (`:2078-2094`)
   and `d_subject_polygon` is never null there. For a right answer it cannot be wrong: a point in the subject
   that is not in the clip when the answer is inside would make a clip ring cross or lie inside the subject,
   which is `RESULT_INTERSECTING`; a subject or clip larger than a hemisphere, or with holes, is read by
   exact parity (`is_point_in_polygon`, threshold off) on both sides, and a point in a subject hole is "not
   in the subject" and returns (1.3 is about what that return hides). Throw off counts
   `hemisphere_check_failures` and returns the result unchanged (`:2158-2167`). Cost: `find_reference_point`
   on every whole answer, ten `minimum_distance` calls at most (`:2124-2132`), which the plan's real-data
   timing (0.280 s against 0.277 s) bears out. `extract` now shares `find_reference_point` (`:4033`), its
   check unchanged. `CheckWholeResult` does what its comment says, with the throwing and the counting
   options.
3. **The polyline hairpin** (`5484193cd`; review 2, 1.1). The spike rule pairs nothing on a polyline ring
   (`num_pairs = 0`, `:953-955`); the loop rule still applies (a polyline's zero-length loop is within its
   identity group, as the page says). In `classify_polyline_pieces` the along test takes a non-internal
   subject single arc between two distinct vertices that a clip non-internal single arc also joins
   (`:2187-2212`): it cannot misfire, since two short great-circle arcs between the same two identity
   vertices are the same arc within the cluster width (a clip original vertex between them would be a record
   on the polyline, a polyline vertex between them `has_interior_vertex`); it catches the two-against-one
   hairpin the pairing leaves (`common_ambiguous`) and a self-touching clip's two arcs against one; and a
   zero-length twist piece it now marks along was already common where the clip has one arc there. Steps
   (3) and (4) skip `METHOD_ALONG` rather than `common` (`:2221`, `:2300`), so an along piece neither
   deduces nor is re-tested, under either `polyline_along_is_inside`. `extract_polylines` is unchanged and
   now never sees a hairpin as internal. `PolylineHairpin` does what its comment says (across: no spikes, no
   along, runs out 2 / in 3 / out 2, oracle clean; along: two along pieces) and fails without either half
   (*run*, M3 and M4, both traces).
4. **Self-touch cuts keyed by stop** (`bd7908f0e`; review 1, 1.2). The key is `(ring, segment, first record
   of the piece's start stop)` (`:1206-1211`), and the rebuild looks it up with `at_vertex_records[k].front()`
   for a vertex stop (`:693`), which is that stop's `d_stop_records[first_record]` by construction
   (`:676-692`), and with `r` for an inside-segment stop (`:727`). Record indices are the intersection
   graph's and are stable across the rebuild; a record belongs to one stop, so the key names one stop.
   `find_self_touch_cuts` runs once, before any cut stop exists (`build()`, `:421-446`), so the assertion
   `num_records > 0` (`:1207-1209`) cannot fire and the "cut stop never starts a cut piece" remark is
   vacuous but true. The `self_touch_cuts_added` guard is now redundant and harmless. Not pinned (section 2).
   The header's new throw documentation for `extract` (grouping failures and coincident rings counted only)
   matches `:4105-4108` and `:4218-4226`; the grouping-failure comment matches the page and the plan.
5. **The kept properties in pyGPlates** (`253fd3f19`; review 5, 1.1). The keeper collects, from the feature
   it is given (a fresh clone of `feature_without_geometry`, so holding only unpartitioned properties),
   those with the names it is told, lets `set_geometry` replace the names, and adds them back
   (`PlatePartitioning.py:21-30`). Nothing is duplicated (each returned feature is its own clone, and the
   same-named originals are removed by `set_geometry` before the put-back), nothing goes to another feature,
   and the exterior-ring-only path adds the ranges before the put-back (`:49-59`). The coverage path passes
   both names and is complete (*run*, P6a); the plain path is not (1.2). The retry path is sound (section
   2). The committed test fails without the put-back (*run*, M9).
6. **`PartitionFeatureGeometries.cc`** (`87891c1b5`; review 4, 1.2 and 1.3). The orphan removal excludes any
   matched coverage's range (`:939-945`), so a range that is a coverage of another geometry of the name, or
   of an unpartitioned single-window geometry whose range is also found at the time, is never removed; and
   it excludes what is already reported (`:946-949`). Its hole is 1.1. `has_range_in_other_windows`
   (`:441-460`) finds the current domain's coverage and asks whether its range is a piecewise aggregation of
   more than one window; `add_geometry` then returns before reporting anything (`:422-425`), so the geometry
   is kept whole, by both methods, as the decision says and the test pins (*run*, M7). A *constant* domain
   with a multi-window range is outside the decision and still partitioned with the current window's values
   (review 4, 1.3's second paragraph; pre-existing). Removing the orphan fails the test (*run*, M8).
7. **`PartitionFeatureUtils.cc`** (same commit; review 4, 1.1). `if (!max_partition || metric > max)`
   (`:534`) takes the first partition with any piece and replaces it only on a strictly greater metric, as
   pyGPlates' `(0.0, 0.0, -1)` start does; the header comments now describe the measure. The no-measure
   polygon (edges of `5e-8` radians, area exactly zero) gets plate 100 by both methods and fails without the
   change (*run*, M6).
8. **The pyGPlates most-overlapping branch, `NaN`, docstrings, stub, changelogs** (`a237155a3`, `87891c1b5`,
   review 5, 1.2 and 1.4; review 4, 1.4). The branch partitions what `_partition_feature_geometries` reports
   (`:433-439`), grouping by plate across property names (right by hashing; *run*, P4), unwrapping coverage
   and exterior-ring-only tuples; the committed two-window case is refused a plate and the single-window case
   given one, and reverting the branch gives the two-window feature a plate (*run*, M10 probe).
   `!(width >= 0)` refuses `NaN` in both bindings (`PyGeometriesOnSphere.cc:3657`, `PyPlatePartitioner.cc:75`;
   *run*, P5 and the two tests). Docstrings, the regenerated stub (the three `:raises ValueError:` lines and
   the two time-dependent paragraphs) and `pygplates-stub-test` agree (*run*, final `ctest`). The dialog's
   plural, `GeometryCookieCutter.h`, `PolygonPartitioner.h`, `PartitionFeatureTask.h` and the stale
   `PartitionFeatureUtils.h` comments say what the code does.
9. **The other new tests.** `CookieCutterOptions` runs the cut under grouped or not, both sorts and four
   speeds and finds every area equal to the first run's within `1e-14` and summing to the subject's; it
   identifies the outer plate by its hole, as its comment says. `AssignPlateIdsSkipsInconsistentFeature`'s
   addition puts the clean geometry (`Q`, the plate's own boundary) first and the inconsistent `P` second,
   as its comment says, and asserts the property count, one report and one feature.
   `AssignPlateIdsFeatureCases`' three cases match their comments (plates 100 and 101 share the equator,
   102 is the strip east of them; the three-value range on a four-point polygon is removed from both pieces;
   the strip cuts the coverage into three four-point pieces in three features).
10. **The page's amendments** are true of the code except as section 4 says: the spike rule's reason and
    scope, the common-pair bound `h / cos(L / 2)`, the self-touch key, the mirror order as a safety net, the
    bundle tie-break's swap, the hemisphere check's two sentences on unions and intersections, the grouping
    failure's reason, the sliver area's definition (`:4188-4189`, exteriors' own ring areas only), the
    polyline paragraph, the provenance cut-edge sentence (`:4605-4630`), the most-overlapping and
    time-dependent paragraphs' additions, 5.6's whole-answer check and the cookie cutter's early return
    (`GeometryCookieCutter.cc:505-513`), and the test inventory.

## 4. Page statements to amend

- **5.4, time-dependent geometry**: "Any other time-dependent geometry is not partitioned and stays
  unchanged on every feature returned (pyGPlates puts back a kept property that shares its name with a
  partitioned one...)". False for the geometry's coverage range in two ways: the C++ removes a
  time-dependent range with no value at the reconstruction time, or one left unmatched by an ambiguous
  count, as an orphan (1.1, both products); and pyGPlates loses a kept range beside a partitioned plain
  geometry (1.2). Fix the code, then keep the sentence.
- **5.4, the orphan rule**: "a range property ... that matched no coverage (its number of values is no
  geometry's)". The parenthesis is one of three causes; the others are two same-named geometries sharing
  its count, and no value at the reconstruction time. With 1.1 fixed, say "a constant range".
- **5.2 and 5.6, the whole-answer check**: "a point outside the subject tells nothing, so a subject thinner
  than the margin everywhere is not checked". Also any subject whose first clear candidate is outside it
  (its centre in a hole or outside a non-convex outline), and the skip is not counted (1.3). With 1.3 fixed:
  "the first candidate clear of both outlines and inside the subject; none is counted as skipped".
- **5.2, the anchorless chain**: "An arc whose clip partner has a direction is preferred, so that both
  projections are defined." Add what happens when none has: the clip's parity is counted at the subject's
  projection, as before the fix (1.4).
- **The test inventory**: "The reviews' fixes are each pinned by a test, most of them checked by reverting
  the fix." The self-touch key and the wiring of `check_whole_result` into `classify()` are not pinned
  (section 2); say "all but two".
- **Docstrings** (`PlatePartitioning.py:187-191`, `:583-587`, and the stub): "Any other time-dependent
  geometry is not partitioned, and is kept unchanged on every feature returned" has the same two exceptions
  until 1.1 and 1.2 are fixed.
