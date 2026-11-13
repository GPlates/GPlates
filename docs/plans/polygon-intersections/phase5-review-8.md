# Phase 5 review 8: steps 14 and 15

Reviewed 2026-11-08 (Fable), the diff `38c4d6417..52a495a10` (code commits `c156cd7c3`, `d771f769c`,
`4da859b02`, `eb3cd922c`, `626544744`, `8bea9c67a`, `09c1f7cb6`, `3f7bac4b0`; plan commits `b5e8a4c44`,
`919dc7049`, `bdae68e2c`, `f9bc811d8`, `ce3324a79`, `52a495a10`): `src/maths/PolygonOverlay.{h,cc}`
(`get_mean_width`, the sliver filter in `extract`, the cone in `measure_ray`, the grouping margin),
`src/maths/PolygonPartitioner.{h,cc}`, `src/app-logic/GeometryCookieCutter.{h,cc}`,
`src/app-logic/PartitionFeatureGeometries.{h,cc}`, `src/api/PyGeometriesOnSphere.cc`,
`src/api/PyPlatePartitioner.cc`, `src/qt-resources/python/api/PlatePartitioning.py`, the stub, both
changelogs, the tests in `src/unit-test/` (`PolygonPartitionerTest.cc`, `PolygonOverlayTest.cc`,
`PartitionFeatureUtilsTest.cc`, `PolygonOverlayPrototypeTest.cc`, `PolygonOverlayOracle.h`, the new
`PolygonOverlayRealDataTest.cc`) and `pygplates/test/`, against `phase4b-design.md` sections 3.8, 4.2, 4.5,
5.3, 8.4, 9.1 and 11.4 as amended, PLAN.md's 2026-11-07 and 2026-11-08 entries, and, for the cone, the
prototype's `r5o` (planning repository `82825cd`, `overlay_proto.py` `_ray_cone`/`_offset_from`). Line
numbers are at `52a495a10`. Evidence is marked *run* (two temporary C++ probe tests appended to
`PolygonOverlayPrototypeTest.cc` and `PolygonPartitionerTest.cc`, a prototype probe and a comparison script
against the committed bundle snapshot, a pyGPlates probe against the Release module, and three mutations of
`measure_ray`, each rebuilt and run against the overlay, partitioner and partition-feature suites; all since
removed by `git checkout` and a touch, both products rebuilt, the suites green and the docs built from
scratch with no warnings; `git status` clean) or *reading*. Probe scripts and logs were kept in the
session's scratchpad (`review8/`).

Baseline (*run*, before any mutation and again at the end): GPlates 195 passed and 2 skipped of 197 (the
skipped two are the opt-in `PolygonOverlayRealDataTest` cases; the plan's "198 of 198" presumably counts a
build with the data set), pyGPlates 7 of 7.

One silent wrong answer was found, reachable through the default filter in both products with a narrow but
real input class (1.1); one divergence of the C++ cone from the prototype's rule, visible in the committed
snapshots but of small consequence (1.2); the rest is edge cases, test gaps and design wording.

## 1. Findings, most severe first

### 1.1 `get_mean_width` returns 0 for a ring on one great circle, so the default filter drops a hemisphere's exterior and the hemisphere is reported wholly inside a small plate (run)

`PolygonOverlay::get_mean_width` (`PolygonOverlay.cc:263-312`) fans Van Oosterom and Strackee's triangles
from the vertices' mean direction (`:284-286`). For a ring whose points all lie on one great circle, the
mean direction lies in that circle's plane (or cancels, and the fallback apex is `ring.front()`, which is
on the circle too), so every triangle (apex, a, b) is degenerate. Its term `2 atan2(apex·(a×b), 1 + apex·a
+ a·b + b·apex)` is then `atan2(0, d)`: 0 when `d > 0`, pi when `d < 0`. The sum is 2 pi or 0 by accident
of which denominators are negative, not the enclosed area.

*Run*, probe (C++): the 4-, 5- and 6-point equator rings give area 6.283 (right, by that accident); a
**36-point equator ring gives area 0 and width 0**; three collinear points on the equator give 0 (right).
The same ring tilted to latitude 0.001 degrees gives 6.2831, so the failure is the exact great circle only.
`PolygonOnSphere::get_area()` gives 6.283 for the 36-point ring, so `SphericalArea` is right here where the
new measure is not.

Consequence (*run*): `PolygonPartitioner` with the cookie cutter's default `10 h`, subject P = the 36-point
equator ring (the northern hemisphere), clip Q = `box(10, 20, 10, 20)` wholly inside it. With the filter
off: `GEOMETRY_INTERSECTING`, inside = Q's region, outside = the hemisphere with one hole (its exterior
ring's measured width 0). With the filter on: the difference's exterior is dropped as a sliver, the
difference is empty, and the result is **`GEOMETRY_INSIDE`: the whole hemisphere (area 6.28) is inside the
10-degree box (area 0.029)**. Through the public API (*run*, probe A): `clip.partition(hemisphere)` returns
`inside` at the default `h` and at `1e-9`, `intersecting` at 0; `PlatePartitioner.partition_geometry`
puts the hemisphere in plate 7 whole; `partition_features` assigns the hemisphere feature to plate 7 with
its full area. Assign Plate IDs shares the cookie cutter, so GPlates does the same. This is the silent
wrong answer design 9.2 forbids, and it does not need the pathological widths of the filter: a hemisphere
mask, or any polygon digitised along one great circle (the equator, a meridian pair), is ordinary user
data.

Why no test caught it: `SliverFilterMeanWidth` tests a small disc and `box(-80, 80, 0, 170)` (a
non-great-circle ring over a hemisphere); nothing tests a ring in one plane. The orientation code
(`oriented`, `:4017-4030`) is unaffected: `PolygonOrientation` handles it, and the probe shows the
hemisphere's orientation computed.

Fix: choose an apex off the ring's plane. The normalised vector area `sum(a_i x a_(i+1))` is the ring's
plane normal for a great-circle ring (90 degrees from every point, antipodal to none) and close to the
centroid direction for any ordinary ring; use it when its magnitude is above rounding, else the mean
direction as now (a ring of zero vector area has zero area whichever apex is used, and the degenerate
three-and-four-point rings of the hunt have both). Alternatively detect the degenerate fan (every
`|apex·(a×b)|` under rounding while the perimeter is not) and re-fan from the plane normal. Add the
36-point equator ring and the hemisphere-with-inner-clip partition to `SliverFilterMeanWidth` and the
partitioner tests, in both products. The oracle's `has_orientation` (`PolygonOverlayOracle.h:387`) uses the
same function and would skip judging such a ring; it inherits the fix.

### 1.2 The cone's `1e-12` rule does not match the prototype's exact zero at crossings in dense shared edges: 30 of 824 bundle-case rays get a cone the prototype gives 0 (run)

`measure_ray` (`PolygonOverlay.cc:1277-1286`) counts an offset from the segment's great circle under
`OFF_SEGMENT_ROUNDING = 1e-12` radians (`:256`) as none, "as the prototype's exact zero for a stop on the
segment's own great circle (a vertex of the ring, a crossing)" (comment `:1273-1274`, design `:667-668`).
An `LL` crossing's stored position is `cross(segment1_plane, segment2_plane).get_normalisation()`
(`GeometryIntersect.cc:253-258`); for the nearly parallel planes of a dense shared edge the cross product
is tiny and its normalised direction carries a rounding error of about `eps / sin(angle between planes)`,
which exceeds `1e-12` by one to two orders.

*Run*: a probe test dumped every vertex's rays (direction, delta, first arc length, cone) for the 14 cases
of `prototype-bundle.txt`, and a prototype probe (`r5o`, the exporter's own `run_overlay_loose_ties`) did
the same; 215 vertices on each side, 824 rays all matched by kind and direction (3 C++ vertices had no
prototype vertex within `0.01h`, consistent with the plan's "two crossings 1e-16 apart are two vertices in
the C++, one in the prototype"). Deltas and lengths agree to `2.6e-5 h` and `0.54 h` (the latter one common
ray where the two measured different arcs, `shared#2557 seed 8`). But **30 rays in 12 of the 14 cases have
a C++ cone of up to `1.5e-5` radians where the prototype's is exactly 0**: their C++ deltas are `2.3e-12`
to `2.4e-11` radians, the far end (or start) of the arc being an `LL` crossing off its segment's plane by
that much. (17 rays go the other way with prototype deltas under `7e-17`, the prototype's own float noise
in `pip_ref.angle`, of no consequence.) One further difference is the rule's own discontinuity:
`shared#1373 seed 14` has `L == delta` to rounding, pi in the C++ (`L <= delta`) and pi/2 in the prototype
(`atan2(delta, ~0)`); both are the design's formula.

Consequence: anchoring is decided by `angle between directions >= sum of cones` (`:2726-2739`), not by
`cone == 0`, so a cone of `1e-5` floats a ray only against another ray within `1e-5` radians of it that is
not paired as common; the committed snapshots' floating counts (`BundleRule`) and the hunt's
119,198-case comparison show it has not happened there. It is a divergence from the stated rule rather
than a known wrong answer. But the design's claim that a crossing counts as on the segment is false as
built, and the mutation evidence below shows the threshold is unpinned: **mutation M2**
(`OFF_SEGMENT_ROUNDING = 1e-9`) passes all 86 overlay, partitioner and partition-feature tests (*run*), so
raising it changes no committed outcome.

Fix: make the rule structural rather than numeric: a stop that is an `LL` record, an original vertex of
this ring, or a record whose position was taken from this ring's vertex has no offset by construction (the
stop's type is known at `d_stops`), and only a stop at the *other* polygon's vertex (an `LS` on the
subject's ring, an `SL` on the clip's) or a self-touch cut is measured. Or raise the threshold to `1e-9`
(the crossing rounding PLAN.md 2026-11-04 already names), which M2 shows is safe for every committed case.
Either way amend design 3.8 (section 4 below).

### 1.3 A dropped sliver hole, or a dropped sliver exterior, can leave an island inside it as a kept polygon that overlaps the filled area (reading)

`extract` (`PolygonOverlay.cc:4042-4082`) drops a sliver hole alone, its polygon filling it (`:4068-4073`),
and drops an exterior with its holes (`:4049-4053`, `:4064-4067`); an island (depth 2) is "filtered on its
own" (`:4042`). If a hole is dropped but an island inside it is not, the island is output as a polygon
lying inside the filled hole, so the output polygons overlap; likewise an island inside a hole of a
dropped exterior. Mean width is `2A/P`, and a hole ring's area is at least its island's, so this needs a
hole whose perimeter exceeds its island's by more than their area ratio: a wiggly hole boundary round a
smooth island, at the filter's scale. Not reachable from ordinary data at `h` or `10 h`; recorded because
the result contract (one partition of the subject) silently fails there and nothing counts it.

Fix: drop a hole only if no kept exterior has it as parent (or drop its islands with it, and count them);
cheap, since `parent` is known.

### 1.4 A single-window geometry's coverage range keeps the original (possibly time-dependent) range property as its clone (reading)

`get_geometry_property` (`PartitionFeatureGeometries.cc:471-475`) makes the domain's clone a constant
property for a single time window, but the range's clone is still `(*coverage.range_property)->clone()`
(`:497`). `ScalarCoverageFeatureProperties` finds a range inside a `GpmlPiecewiseAggregation` at the
reconstruction time (`ScalarCoverageFeatureProperties.cc:272-293`), so a domain and range both in single
windows are partitioned, and GPlates' "most overlapping plate" path (`PartitionFeatureUtils.cc:426-472`),
which writes the clones back, adds a constant domain beside a time-dependent range. The split paths
(Assign Plate IDs' `add_partitioned_geometries_to_feature`, pyGPlates' `set_geometry`) build new range
properties and are unaffected. Rare (GPlates writes coverages as constants), but the statement "written
back constant" (design 8.4, both changelogs) is then not true of the range.

Fix: when `d_single_time_window_value` is set and the range property's value is a
`GpmlPiecewiseAggregation` of one window, clone its window value as a constant property too, through the
same `create_constant_property`.

### 1.5 Minor

- `:type sliver_width_threshold_radians: float` in both `PlatePartitioner.__init__` overloads
  (`PyPlatePartitioner.cc`) and `PolygonOnSphere.partition` (`PyGeometriesOnSphere.cc:4487`) while `None`
  is accepted and documented as the default; `docs/pygplates/README.md` rule 2 spells that `float, or
  None`. It follows `anchor_plate_id`'s precedent (`int`, `None` accepted), so it is consistent rather than
  wrong; the pure-Python `partition_into_plates` stub shows `float | None = None` from its signature, so
  the three disagree in the stub.
- `PolygonOverlayRealDataTest.cc:518` measures `area_cut` from the exterior ring alone (holes' areas not
  subtracted); the ratios printed are slightly low. Reporting only.

## 2. Test gaps

- **The great-circle ring (1.1)**: no test of `get_mean_width` on a ring in one plane, nor of the filter on
  a subject whose piece's exterior is such a ring.
- **The far end's offset is pinned only by `BundleRule`'s counts** (*run*, mutation M1: `ray.delta` without
  `offset_from_segment(far)` fails only `BundleRule`, on `shared#3653 seed 14`; `Bundle`, `Hunt` and the
  four committed cases of `8bea9c67a` all pass). The start's offset is pinned by correctness (*run*,
  mutation M3: without `offset_from_segment(start)`, `Bundle`, `BundleRule` and `RaysAcuteWedge` fail).
  PLAN.md's "mutations reverting either fix are caught by the committed cases" holds for the start, not the
  far end; a bundle case that needs the far end's offset would pin it as a result rather than a count.
- **`OFF_SEGMENT_ROUNDING` is unpinned** (*run*, M2 above).
- `SliverFilterEmptiesBoth` covers the both-emptied rule in C++; nothing covers it through pyGPlates (fine,
  the rule lives below the binding).
- No test of a single-window geometry **with a coverage** in either product (1.4).
- No test that a sliver hole with a kept island is handled (1.3).
- The real-data harness runs with the filter off by design (it measures widths); its `EXPECT`s do not
  exercise the filter. Fine, but worth saying in the plan.
- The grouping margin's committed cases (`09c1f7cb6`) are in the hunt snapshots; a direct unit test of
  `is_ring_inside_ring` with two output rings crossing by a cluster's width would survive a snapshot
  re-export.

## 3. Checked and found correct

- **The cone against `r5o`** (*run*, the 824-ray comparison): `far` is the next stop along the piece
  (`to.position`), the prototype's `pts[1]`; for `at_end` the stops are reversed and the segment is
  `from.segment_in`, the prototype's `end_stop['seg_in']`; the direction is `-tangent`.
  `from_output_position` tests the same stored point as the prototype's `start == v.centre`. Apart from 1.2
  and the `L == delta` boundary, deltas agree to `2.6e-5 h`, cones to `1e-6` radians. Common rays take the
  wider cone of the two arcs (`:1446-1452`), as `_ray_cone`'s `best`. A touch record's position is the
  touching vertex, not its projection (*run*: `RaysAcuteWedge` measures clip offsets of `r sin 15`
  degrees; reading: `GeometryIntersect.cc` passes `intersection_position` through unchanged).
- **Can the new delta float a ray the prototype would anchor?** Only through 1.2, by a cone of at most
  `1.5e-5` radians against a neighbour within that angle; not in the 14 bundle cases (counts pinned) and
  not in the hunt's comparison.
- **The grouping margin** (*run* via the suite, reading): `cluster_width` in `extract` (`:3960-3975`) runs
  over every vertex's records on both polygons, exactly `PolygonOverlayOracle::get_cluster_width`
  (`Oracle.h:146-165`); `max(10h, 2 x width)` is the oracle's margin (`:177`). With the margin at least
  `10h` the fallback (farthest vertex or edge midpoint, `:224-241`) is taken for more rings than before,
  but it is the same exact test and is wrong only if every point of the ring is within the crossing error
  `c <= margin / 2` of the other ring, i.e. a ring narrower than `c` everywhere, which is degenerate.
  `minimum_distance` with the margin as threshold returns none beyond it, so "farther than the margin" is
  read correctly.
- **The sliver filter in `extract`** (reading; *run* via `SliverFilterExteriorWithItsHole`,
  `SliverFilterHoleAlone`, `SliverFilterEmptiesBoth`, `SliverFilterCookieCutter`): holes of a dropped
  exterior skipped uncounted, a hole alone counted in `sliver_holes_dropped`, `sliver_area_dropped` the
  exteriors' ring areas (holes included, which is what the both-emptied comparison needs), `ring_areas`
  filled by `is_sliver` before use. The partitioner's rule (`PolygonPartitioner.cc:130-152`) diffs the
  accumulated diagnostic across the two `extract` calls; the tie goes outside, as documented.
- **The result contract (5.3) with and without lists**: `partition_polygon` (`:690-740`) and the cascade's
  `visit_polygon_on_sphere` both go through `overlay_polygon`, which always extracts into local vectors;
  the lists only decide whether `create_polygon` runs. So the filter's decision is the same either way,
  and pyGPlates' `partition()` without lists (`PyGeometriesOnSphere.cc:3663-3714`) agrees with it (*run*,
  probe A: `intersecting`/`inside` consistent with the lists' contents).
- **`get_mean_width` otherwise** (*run*, probe): empty, one-point, two-point and collinear rings give 0; the
  400-edge disc of radius `10h` gives its radius to `1e-3`; `box(-80, 80, 0, 170)` both ways gives one
  width equal to `2 x get_area / get_arc_length` to `1e-9`; a ring over most of a hemisphere (latitude 1
  degree, 36 points) gives 6.173, the smaller side. The perimeter by `atan2` keeps short edges. Only the
  exact great circle (1.1) fails.
- **The single-window rule** (reading; *run* via `AssignPlateIdsTimeDependentGeometry`,
  `test_partition_features_single_time_window`, probe C): `GmlTimePeriod::contains` is inclusive at both
  ends (`GmlTimePeriod.h:208-214`; probe: window (10, 0) at 10 partitions, at 10.0000001 does not, a
  distant-past begin contains 500); `create_top_level_property` with `check_property_value_type = false`
  applies the GPGIM's time-dependent wrapper for the name and falls back to a plain
  `TopLevelPropertyInline` for an unknown name (`ModelUtils.cc:510-565`,
  `PartitionFeatureGeometries.cc:426-443`); the constant clone is what both of Assign Plate IDs' methods
  write (`PartitionFeatureUtils.cc:447-472` reads the geometry from the clone, sets the
  reverse-reconstructed geometry on it, adds it). A `GpmlIrregularSampling` stays the base visitor's no-op.
  Multi-window and non-containing windows return early and are not in `partitioned_properties`.
- **The pyGPlates property-position mapping** (reading; *run*, probe B): `Feature.__iter__` is
  `bp::iterator<FeatureHandle>` over `begin()..end()` (`PyFeature.cc:4133`), `feature_handle_clone` adds
  each property in that same iteration (`:1474-1491`), and `RevisionAwareIterator` skips removed slots on
  both sides (`RevisionAwareIterator.h:252`), so the indices computed on the original
  (`PyPlatePartitioner.cc:478-487`) apply to the clone (`PlatePartitioning.py:378-382`). Probe B: a feature
  with a removed property and two geometries under different names yields two features each keeping
  `gml:name` and `gml:validTime` and exactly its own geometry. A feature with nothing partitioned is
  returned whole (`:365-367`; *run*, `test_partition_features_no_geometry`).
- **pyGPlates arguments** (*run* via the tests and probe A): `None` gives the default (`10 h` for
  `PlatePartitioner`, `h` for `partition`), 0 turns the filter off (`boost::none`), negative raises
  `ValueError` in both bindings; `partition_into_plates` forwards it. The stub carries the new parameters on
  all four signatures; `pygplates-stub-test` passes.
- **Changelogs**: both entries describe user-visible behaviour in user terms, 90 m and 9 m right for `h =
  1.4142e-6` radians on a 6371 km sphere; the pyGPlates bug-fix line for the dropped no-geometry feature is
  accurate (PLAN.md confirms it was on `main` too).
- **Who else gets the default filter**: only `AssignPlateIds.cc:84,174` and `PyPlatePartitioner.cc:170,282`
  construct a `GeometryCookieCutter` for cutting; `PlateVelocityUtils.cc:970` constructs one for
  `partition_point` only, which the filter does not touch.
- **The harness changes** (reading): the provenance tolerance `max(2h, cluster width + 0.1h)`
  (`PolygonOverlayPrototypeTest.cc:113-118`) only widens where a vertex's output position can legitimately
  be that far from its record; a wrong location farther than a cluster from its point still fails.
  `has_orientation` skips rings under `0.01h` mean width, which have no orientation to judge; a misoriented
  ordinary ring is still counted. The `\r` strip and `describe_cases` are harmless.
  `describe_overlay_outcome` constructs its own overlay, so a prototype-failed case now reports the C++'s
  repairs and failures.
- **Snapshots and pinned counts**: `BundleRule`'s new table matches the re-exported `r5o` snapshots (*run*,
  the suite; and M1/M3 show the counts are sensitive to each offset).
- **Style**: tabs, Allman braces, broken signatures, `d_` members, lines within the neighbourhood rule; the
  GPL header on the new test file.

## 4. Design statements to amend

- **3.8 (`:667-668`)**: "an offset under `1e-12` radians counts as none, as the prototype's exact zero for a
  stop on the segment's own great circle (a vertex of the ring, a crossing)". A crossing's floating-point
  position lies off the plane by up to about `2.4e-11` radians in dense shared edges (1.2), so either the
  rule becomes structural (by stop type) or the threshold rises to `1e-9`; state which, and that the C++
  cone can then exceed the prototype's by up to `1.5e-5` radians at such rays if the numeric rule stays.
- **4.5 "As built (step 14)" (`:951`)**: "Off by default, and not in pyGPlates" is superseded by the
  "Decided" bullet below it; mark it as the state before `d771f769c`, or drop it.
- **4.5 `get_mean_width` (`:956-957`)**: "sums ... triangles from the vertices' mean direction, modulo
  `4 pi`, and takes the smaller side" must record the great-circle failure (1.1) and the apex chosen to
  avoid it; also that the measure is exact "for any apex no ring point is antipodal to *and that is not in
  the ring's plane*" until fixed.
- **4.5 the filter's semantics**: note that a dropped sliver hole's islands (and a dropped exterior's hole's
  islands) are kept as polygons of their own (1.3), or the rule chosen to avoid it.
- **8.4 "as built" (`:1446-1452`)** and both changelogs: "written back constant" holds for the domain; the
  range clone of a single-window coverage is the original range property (1.4).
- **9.2**: the great-circle case is a silent wrong answer in the filter, outside the overlay's own
  decisions, so the "never a silent wrong answer" claim should name the filter as a distance rule on the
  output that can only drop, and its measure as the one place a wrong number silently changes a result.
- **11.4**: say that the real-data harness measures with the filter off and does not exercise it, and that
  `area_cut` is exterior-ring area.
