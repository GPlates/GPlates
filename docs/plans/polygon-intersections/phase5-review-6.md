# Phase 5 review 6: step 12, coverages, area and Assign Plate IDs' failures

Reviewed 2026-11-06 (Fable), the diff `c40a212a0..c6866acd0` (code commit `df06def14`, plan commit
`c6866acd0`): `src/app-logic/PartitionFeatureUtils.{h,cc}` (`Range`, `partition_geometry`,
`partition_geometries`, `partition_range`, `interpolate_ring_values`, `GeometrySize`,
`GeometrySizeMetric`), `src/app-logic/AssignPlateIds.{h,cc}`,
`src/qt-widgets/AssignReconstructionPlateIdsDialog.{h,cc}`, `src/cli/CliAssignPlateIdsCommand.cc`
and `src/unit-test/PartitionFeatureUtilsTest.cc`, against `phase4b-design.md` sections 7.2, 7.3, 8.3,
9.2 and 14 (questions 1 to 3), and PLAN.md's 2026-11-05 and 2026-11-06 entries. The G1-1/G1-4 merge
(`ScalarCoverageFeatureProperties`' `exterior_ring_only` and `Coverage::domain`) was read for how step
12 uses it, not reviewed. Line numbers are at `c6866acd0`. Evidence is marked *run* (a build of
`gplates-unit-test` in the Release tree with seven temporary probe tests appended to
`PartitionFeatureUtilsTest.cc` and twelve one-at-a-time mutations of `PartitionFeatureUtils.cc`, all
since removed by `git checkout` and a touch; both suites rebuilt and green afterwards, GPlates
185 of 185 and pyGPlates 7 of 7; `git status` shows only this file) or *reading*.

Baseline: the four `PartitionFeatureUtilsTest` tests pass.

One silent wrong answer was found (1.1), reachable with any cascade of two or more plates. The rest
is test gaps (the committed suite pins the polygon path for a ring that starts at a valued point, and
not much else) and minor observations.

## 1. Findings, most severe first

### 1.1 A ring that starts at a point with no value gets wrong values before its first valued point (run)

`interpolate_ring_values` (`PartitionFeatureUtils.cc:694-775`) walks a closed ring once round from
its first valued point `first` (`:736-763`), so the run that wraps past the ring's start (indices
`0 .. first - 1`) is interpolated by arc length in the loop, as design 7.3 step 2 says. But the two
loops after it (`:765-774`), meant for "a polyline's run at its end, and at its start", are not
guarded on `!closed`: the second one overwrites `values[ring_start + 0 .. first - 1]` with the value
at `first`. So on a closed ring every point before the first valued point gets the constant value of
that point instead of its interpolation.

The committed tests never see it because every output ring of their inputs starts at a crossing on
the subject's outline (a valued point; the overlay starts a ring at the first stop in subject order).
A ring starts at an unvalued point as soon as a piece is cut again: the second overlay's first stop
lies on the first plate's cut edge, which has no subject location (7.2). Concrete failing input
(*run*, probe `CascadeRingStart`): the subject of `CutInput` without its hole, values the vertices'
latitudes; plates A = `box(-5, 30, -10, 10)` and B = `box(0, 5, 5, 30)` in either order. With B
applied first (plate IDs sort it first), the piece inside A is the ring
`(0, 10), (0, 5), (5, 5), (5.08, 10), (10.15, 10), (10, 0), (-5.08, 0), (-5, 10)`, whose first four
points lie on B's cut edges and have no subject location; they get `10, 10, 10, 10` (the value at
`(10.15, 10)`, index 4) where arc length between `(-5.08, 0)` (value `-5.08`) and `(10.15, 10)`
(value `10`) gives `1.37, 3.52, 5.67, 7.82`. With A first, the piece inside B starts at `(0, 10)` on
A's cut edge and its first two points get `-5.08` where the rule gives `-2.03` and `-3.05`. A real
plate model is such a cascade for every feature that spans three plates.

The fix is one guard: the two trailing loops run only when `!closed` (*run*: with it, all seven
probes and the four committed tests pass; with it, mutation M1 below still fails `CoverageAcrossCut`,
so the closed walk is what the committed suite pins). The comment at `:765-766` is then right. Add
the two-plate case above as a committed test, checking every run of every piece against an
independent arc-length interpolation (the probe's `check_runs`, section 2).

### 1.2 Minor observations (reading unless marked)

- `GeometrySizeMetric::operator<` (`:1662-1675`) compares `real_t`s, whose `<` is `d > EPSILON`
  (`Real.h:274-280`) and whose `!=` (from `boost::equivalent`) is "differ by more than `EPSILON`"; a
  tolerance equality is not transitive, so the lexicographic order is not a strict weak ordering in
  theory. Pre-existing (the old `operator<` did the same with arc length then points), used only by
  the running maximum in `find_partition_containing_most_geometry` (`:1228-1290`), and `EPSILON` is
  `1e-12` on a unit sphere; not worth changing, but 8.3 as built says "equal within its epsilon
  falls through", which is this.
- `get_nearest_outline_value` (`:312-377`) clamps `along / segment_angle` to `[0, 1]`, where
  `along` is in `(-pi, pi]`. A point whose projection lies more than `180 - theta/2` degrees behind
  the segment's start (`theta` the segment's angle) is nearer the segment's *end* round the far side
  of the great circle but is clamped to the start. Unreachable in practice: the point is inside the
  subject and this is its nearest outline segment, so the subject would have to span most of a
  hemisphere; noted only because it is the one case the clamp gets wrong.
- The dialog reports the skipped features only when `partition_features(plate_id_assigner)` returns
  true (`AssignReconstructionPlateIdsDialog.cc:309-317`); a run the user cancels part-way
  (`:519-528`) has already left some features unpartitioned and reports none of them. Minor; say
  so, or report before returning false.
- `#include "maths/AngularExtent.h"` (`PartitionFeatureUtils.cc:51`) is no longer used directly
  (`POLY_GEOMETRY_DISTANCE_THRESHOLD` is gone); harmless.
- Style. Hard tabs, Allman braces and broken signatures throughout. Doc-comment continuation lines
  at `PartitionFeatureUtils.cc:308-310`, `:566-579` and `PartitionFeatureUtils.h:570-571` lack the
  leading space before `*` that every other block in both files has (` * `), and the wraps at
  `:566-567` ("(phase4b-design.md" / "section 7.3") and `:728-730` ("polyline)," alone on a line)
  read like the rewrap script's work that PLAN.md's trap entry describes. Line length under the new
  rule (tab = 4; longest existing code line within 30 lines, up to 130): every over-100 line has a
  neighbour that allows it except the `//!` comment lines `:256-257` (103) and `PartitionFeatureUtils.h:554`
  (102, a comment), and `:679` (128), which is the old `range_tuple_element` line re-indented one tab
  deeper (127 before); none is worth a change on its own. Comments were checked against the code; all
  say what it does, except `:765-766` and the `interpolate_ring_values` header (`:688-692`), which
  describe the intended behaviour that 1.1 breaks.

## 2. Test gaps

*Run*: twelve mutations, one at a time, each rebuilt and run against the committed
`PartitionFeatureUtilsTest` (4 tests) and against the seven probe tests described below. The
session's seven were not repeated.

| Mutation | Committed suite | Probe |
| --- | --- | --- |
| M1 `interpolate_ring_values`: a closed ring walked like a polyline (`num_steps = ring_size - first - 1` always, `:736`) | caught (`CoverageAcrossCut`) | caught |
| M2 a run point's distance measured straight from the run's start, not cumulatively (`:745`) | passes | caught (`RunsAcrossRingStart`: the exterior-ring-only hole run of four points) |
| M3 `operator<`: the number of points never compared (`:1674`) | passes | passes (no probe) |
| M4 `get_nearest_outline_value`: the nearest segment always taken as the exterior ring's (`:341`) | passes | caught (`NearestOutlineOnHole`) |
| M5 `partition_range`: every scalar type written from the first (`:676`) | passes | caught (`TwoScalarTypes`) |
| M6 (the fix for 1.1, `if (!closed)` round `:767-774`) | passes | all pass: the diagnosis |
| M7 a polyline piece treated as one ring per point (`:616`) | passes | passes: equivalent while every polyline point has a value (7.3 as built says all do; a composition mismatch would break the equivalence) |
| M8 a multi-point's ring with no value takes point 0 (`:334-336`) | passes | passes: unreachable (a multi-point's point always has its own index) |
| M9 step 1 ignores the ratio (a crossing takes its segment's start vertex, `:646-651`) | caught (`CoverageAcrossCut`) | caught |
| M10 the nearest-outline value at ratio 0 rather than the projection (`:376`) | caught (`RingWithNoValuedPoint`) | caught |
| M11 arc length by chord rather than angle (`:708`) | caught (`CoverageAcrossCut`) | caught |
| M15 the projection's ratio not clamped to the segment (`:374`) | passes | caught (`NearestOutlineClamp`) |

So the committed suite catches the closed walk, the segment ratio, the projection and the angle, and
misses cumulative arc length over a run of two or more points, the ring search in the nearest-outline
fallback, more than one scalar type, the clamp, the point-count tie-break, and 1.1. Inputs and tests
to add, each run here as a probe:

1. **A second-step ring starting on the first plate's cut edge** (1.1): the two-plate case of 1.1 in
   both orders, checked by a generic rule rather than by hand: for every ring of every piece, mark
   the points that lie on the coverage domain's outline (`minimum_distance` under `1e-6`; a vertex's
   distance computes as about `1e-8` through `acos`), and require each maximal run of the others to
   be the arc-length interpolation between its valued ends, cyclically. That check also catches M2
   on the committed inputs, where the hole opened into an exterior ring makes a run of four.
2. **Two or more scalar types** (M5): the same coverage twice, the second negated, and every piece's
   second range the negative of its first.
3. **The nearest outline on a hole** (M4): `CutInput` with its hole and values `100..103`, the plate
   `box(-1, 1, 11, 12)` beside the hole's left edge; every value of the inside piece between 100
   and 103.
4. **The clamp** (M15): an L-shaped subject `(10, 10), (10, 0), (-10, 0), (-10, 20), (0, 20), (0, 10)`
   with value 1000 at the reflex vertex `(0, 10)` and 0 elsewhere, the plate `box(-3, -1, 7, 9)`
   inside it: every point of the inside piece is nearest to the reflex vertex and gets exactly 1000
   (*run*); without the clamp the ratio runs past 1.
5. **The point-count tie-break** (M3): two multi-point partitions of different sizes, or the same
   polyline length with more points.
6. **Other geometry types through `partition_feature` with a coverage**: a polyline (values its
   longitudes, cut at the plate's edge: *run*, both pieces right, the crossing interpolated), a
   multi-point split across the plate (*run*, each point its own value) and a point (*run*). None
   is in the committed suite; M7 shows the polyline path is pinned only by equivalence.
7. **A geometry returned whole** (*run*, `WholeGeometryKeepsCoverage`: a polygon with a hole outside
   every plate keeps its eight values exactly, through the cookie cutter's identity table). Not
   committed.
8. **Assign Plate IDs' catch** (`AssignPlateIds.cc:300-306`): no test, as the plan says; removing it
   survives trivially. A `PartitionFeatureTask` stub that throws would pin "the feature is left as
   it was" and `get_features_not_partitioned` without an inconsistent input.
9. A polyline whose end run has no value (`:767-770`, "takes its one neighbour's") and a ring with
   exactly one valued point: neither reachable from a committed input (both need a composition
   mismatch); by reading, the one-valued ring interpolates every point between that value and itself.

## 3. Checked and found correct

- **Provenance against the coverage domain** (brief item 1). `Range` (`:189-388`) is built from
  `coverage.domain`, which under `exterior_ring_only` is the exterior ring alone
  (`ScalarCoverageFeatureProperties.cc:599-605`), so `ring_sizes` has one entry and a hole vertex's
  location (ring `i + 1`) finds no scalars (`get_vertex_value`, `:263-266`), as 7.3 step 1 wants; a
  location's ring and index are relative to the geometry *property* (`Location`,
  `PolygonOverlay.h:605-617`: ring 0 the exterior, `i + 1` interior `i`), which is `Range`'s
  numbering too (`:234-253`). `get_segment_value` (`:277-305`) wraps a polygon ring's last segment
  to vertex 0 and not a polyline's (whose segment `N - 1` does not exist). *Run*: `CoverageAcrossCut`
  (the closing-segment crossing), and the probes' hole values, polyline, multi-point and point.
- **Point order.** `partition_range` (`:590-626`) takes `get_geometry_points` order (exterior, then
  interiors, `GeometryUtils.cc:101-129`) and trims to the exterior ring under `exterior_ring_only`
  (`:607-611`); the provenance table is in the same order (`provenance_seq_type`,
  `PolygonOverlay.h:639`), so `provenance[i]` is `points[i]` for the kept prefix; the assertion
  `:629-632` guards the length. The old `get_geometry_exterior_points` gave the same prefix.
- **Rings and the wrap.** A closed ring's walk (`:736-763`) takes `ring_size` steps from `first`,
  ends at `first` itself (valued), so every run closes, including one wrapping past index 0 (M1 and
  the probes); `previous` and `distance` accumulate along the ring; a zero total length gives ratio
  0 (`:750`); a ring with one valued point interpolates between that value and itself. The bug of
  1.1 is after the walk, not in it.
- **The nearest-outline fallback.** `minimum_distance(point, polygon)` (`GeometryDistance.cc:1323-1414`)
  walks the polygon's bounding tree, which is built over the polygon's `const_iterator` covering all
  rings in order (`PolygonOnSphere.cc:890-905`), and stores `gca_index` from the tree node's begin
  index (`:736-764`), so the segment index is numbered exterior then interiors like the points
  (`PolygonOnSphere.h:810-847`); a ring has as many segments as vertices, so `ring_offsets` serve
  both (`:339-346`). The ratio by projection (`:356-375`): `along` is the signed angle from `a` to
  the projection about the segment's normal, clamped to the segment; for a point inside a convex
  subject the projection onto its nearest segment is within it, and for a reflex vertex the clamp
  gives the vertex (probe 4). Polyline (`:347-351`, no wrap) and multi-point (`:329-337`, the closest
  point's index) branches as designed. Under `exterior_ring_only` the domain searched is the
  exterior ring alone, consistent with the values. *Run*: `RingWithNoValuedPoint`, M10, probes 3
  and 4.
- **What is written to features** (brief item 1, last point). `add_partitioned_geometries_to_feature`
  (`:931-989`) reverse-reconstructs the piece's domain and appends the range as computed; under
  `exterior_ring_only` a piece that keeps a hole gets a range of its exterior ring's length, which
  `ScalarCoverageFeatureProperties` reads back as exterior-ring-only (the second `match_domains`
  pass, `.cc:234-257`, `READ_EXTERIOR_RING_ONLY_COVERAGES` true), and a piece with no hole gets a
  full-length range read by the first pass. *Run*: `ExteriorRingOnlyCoverage`'s second part.
- **The size metric** (8.3). `GeometrySize` (`:806-851`) adds a polygon's `get_area()` and not its
  arc length, a polyline's arc length, a point's or multi-point's count; `operator<` compares area,
  arc length, points in that order, so a feature with any polygon piece is decided by area and one
  with polylines by length (the only behaviour change for mixed features, and the one 8.3 decides).
  *Run*: `SizeMetricByArea`; swapping the order fails it (reading, the test's `thin < fat`).
- **Assign Plate IDs' failure handling** (9.2, question 3). `GenericPartitionFeatureTask::partition_feature`
  (`GenericPartitionFeatureTask.cc:53-125`) calls `PartitionFeatureUtils::partition_feature`
  (which reads the feature and clones properties, modifying nothing) before any `remove` (`:92`) or
  the `PartitionedFeatureManager`'s clone; `VgpPartitionFeatureTask` (`VgpPartitionFeatureTask.cc:63-130`)
  uses `partition_point` only, which cannot throw. Nothing after the partition touches the overlay.
  The `NotificationGuard`s (the task's `:64`, `AssignPlateIds.cc:278`) are released by unwinding or
  by the catch's normal exit, with no model change to notify. The catch (`AssignPlateIds.cc:300-306`)
  is in `assign_reconstruction_plate_id_internal`, which both public entry points use (`:236-263`),
  and `AssignPlateIds` has no other callers than the dialog, the CLI and the tasks (grep over `src/`;
  none in `src/api`). The dialog reads `get_features_not_partitioned` after the run
  (`AssignReconstructionPlateIdsDialog.cc:312-317`) and the CLI after `for_each`
  (`CliAssignPlateIdsCommand.cc:392-398`); each run makes a new `AssignPlateIds`, so the list does
  not carry over. `tr("%n feature(s) ...", nullptr, n)` is Qt's plural form; `%n` is substituted
  with no translator; `size()` to `int` narrows silently (fine). `get_feature_description`
  (`:315-337`) gives name and ID, or the ID alone.
- **Provenance asked for only with a coverage** (`:504-508`, `static_cast<bool>(geometry_range)`),
  and `Range::range_matches_domain` (`:193-215`) still gates the range on the (exterior-only)
  domain's point count, so a mismatched old file gets no range rather than a wrong one.
- **pyGPlates**: `PartitionFeatureUtils` has no caller in `src/api`, so step 12 changes nothing a
  Python user sees until step 13; `pygplates-source-closure-test` passes with the committed matrix.

## 4. Design statements to amend

- **7.3 as built**, "Step 2 per output ring, cyclic": not as built until 1.1 is fixed; say that the
  trailing copy is for polylines only.
- **7.3 as built**, "a polyline's points all lie on the input, so all have values ... none seen":
  add that a composition mismatch (7.2) is the one way a polyline point can lack a value, so that
  the end-run copy has a reason to exist.
- **8.3 as built**: add that the running maximum, not a sort, consumes `operator<` (1.2's first
  point), so the tolerance order is harmless.
- **9.2 as built**: add that a cancelled dialog run reports nothing (1.2), and that the catch is
  untested by construction (section 2, item 8 gives a way).
- **PLAN.md step 12**, "seven mutations, each failing a committed test": add that of twelve further
  mutations six survive the committed suite (section 2), and that the suite's inputs all start their
  rings at a valued point, which is what hid 1.1.
