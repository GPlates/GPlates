# PR B review 3: the partition layer

Reviewed 2026-11-08 (Fable), at `02f485dd4` (the whole of PR B, `public/main...HEAD`, seen as one):
`src/maths/PolygonPartitioner.{h,cc}` (`create` and its options, `is_outside_by_bounds`, `overlay_polygon`,
`overlay_polyline`, the `GeometryPartitioner` visitor and its `add_whole`, `partition_geometry` both overloads,
`partition_polyline`, `partition_polygon`, `partition_point`, `partition_multipoint`,
`create_identity_provenance`, `CascadeProvenance`), `src/app-logic/GeometryCookieCutter.{h,cc}` (the three
constructors, the grouped and ungrouped adding, `add_partitioning_resolved_topological_network`, the two sort
comparators, `partition`, `partition_geometry`, `partition_geometries`, `partition_point`,
`get_partitioning_polygons`, `get_default_sliver_min_width`), the other callers
(`GLReconstructedStaticPolygonMeshes::find_present_day_polygon_mesh_node_intersections`,
`GenerateVelocityDomainCitcoms.cc`'s `CapDiamond::find_intersections`, `PlateVelocityUtils.cc`'s
`rigid_plates_query`), `PointInPolygon::get_crossing_parity_excluding_segment` with
`get_num_polygon_edges_crossed`, and the tests `PolygonPartitionerTest.cc` and the cascade and result-contract
cases of `PolygonOverlayPrototypeTest.cc`; against `docs/design/architecture/foundation/spherical-geometry.md`
sections 3.3, 5.1, 5.4 (the composition through the cascade), 5.5 (the widths) and 5.6, with the eight step
reviews and PR B reviews 1 and 2 for what is known. The widths' reach was checked in `AssignPlateIds.cc`,
`PyPlatePartitioner.cc` and `PyGeometriesOnSphere.cc`. Line numbers are at `02f485dd4`.

Evidence is marked *run* or *reading*. *Run*: ten temporary probe tests appended to
`src/unit-test/PolygonPartitionerTest.cc` (every speed with and without lists and filter on ten polygon pairs
and a polyline; a polyline and a point inside the helix band's interior beyond its circle; holes of the
partitioning polygon, a subject around it, points and a multi-point on the outline; a cascade of a plate with a
hole and the plate that fills it, in both orders, filter on and off, with a grid check that the pieces are
disjoint and cover the input; a ribbon `5 h` wide straddling two plates equally; the cookie cutter's
`partition_point` on a shared edge; a polyline along a shared edge through the cascade; a hemisphere as the
partitioning polygon at every speed; a subject whose outline crosses a shared edge by `0.4 h` either way; sorting
by area), and nine mutations of `PolygonPartitioner.cc`, `GeometryCookieCutter.cc` and `PointInPolygon.cc`, each
on freshly restored files, rebuilt and run against `PolygonPartitioner*:PolygonOverlay*:PartitionFeatureUtils*`
(97 tests with the first seven probes). All removed afterwards by `git checkout` and a touch; the unit-test
target rebuilt; the overlay suites green (93 tests: 91 passed, 2 skipped by design) and the full suite green
(`ctest`: 203 of 203); `git status` clean but for this file. Probe sources and the mutation script were kept in
the session's scratchpad.

Baseline (*run*, before any change): `gplates-unit-test --gtest_filter=PolygonPartitioner*:PolygonOverlay*:PartitionFeatureUtils*`
93 tests, 91 passed, 2 skipped (`PolygonOverlayPrototypeTest.File`, `PolygonOverlayRealDataTest.Coastlines`).

No silent wrong answer was found in this layer. Every probe gave the expected answer, every mutation of a
decision this layer makes was caught by a committed test except two that guard cases no test reaches (section
2), and the shortcut, switched off, changed no result. What follows is one exception-contract nuance the page
overstates (1.1), the two unpinned guards, stale comments, and page wording. Review 1's finding that
`overlay_polygon` returns a whole answer without `extract` (so the hemisphere check never runs for it) is known
and not re-reported; the layer's own code is as review 1 and 2 described it.

## 1. Findings, most severe first

### 1.1 Without lists the cascade stops at the first plate that claims a piece, so a later plate's inconsistent cut is never reached; section 5.6 says the cookie cutter raises "with or without lists" (reading)

`GeometryCookieCutter::partition` (`GeometryCookieCutter.cc:505-513`): when a plate's inside list is
non-empty and the caller gave neither list, it returns `true` at once. The plates after it never see the
pieces, so an overlay that would have thrown `InconsistentOverlayException` there does not run. The boolean
answer is right (the geometry is partly inside some plate), and the partitioner itself does raise the same
exception with or without lists (`partition_polygon` always extracts into local vectors,
`PolygonPartitioner.cc:694-704`; `AssignPlateIdsSkipsInconsistentFeature` pins it). So this is not a wrong
answer; it is that 5.6's "`PolygonPartitioner` and `GeometryCookieCutter` let the exception propagate, with or
without lists (every plate must get an answer...)" holds for the cookie cutter only as far as the cascade runs.
The result-only cookie-cutter path is reached from pyGPlates (`PlatePartitioner.partition_geometry(geometry)`
with no lists, `PyPlatePartitioner.cc:350`), where a user testing "is this geometry inside any plate" gets
`True` where the same call with lists would raise. Fix: either say so in 5.6 (the exception is raised for the
plates consulted, and the boolean needs no later plate), or drop the early return, which costs the remaining
plates' overlays for a yes-or-no question. The page's wording should follow whichever is chosen.

### 1.2 Minor (reading unless marked)

- **The both-emptied tie-break sends a thin polygon outside at every plate that ties** (`PolygonPartitioner.cc:144-149`):
  on `inside_area_dropped == outside_area_dropped` the result is `OUTSIDE`, so a polygon narrower than the
  filter width, cut into equal halves by every plate it meets, would be outside all of them and get no plate ID.
  *Run*, `ProbeRibbonTie`: a ribbon `5 h` wide straddling plates A and B equally (the symmetric construction
  most likely to tie) did not tie in doubles: A answered `OUTSIDE` and B `INSIDE`, so the ribbon went to B. An
  exact tie needs exactly equal areas, so this is theoretical; if it is to be closed, `INSIDE` on a tie when the
  filter emptied both (a sliver straddling a boundary is at least half inside) assigns it to the first plate it
  meets, and the no-filter tie (both sides collapsed, both areas zero) keeps `OUTSIDE` by testing
  `sliver_exteriors_dropped` instead of comparing areas.
- **`GeometryCookieCutter.h:144-145`** says `partition_point_speed_and_memory` "determines the speed versus
  memory trade-off of the point-in-polygon tests for partitioned *point* geometries"; it is now also the
  overlay's `Options::clip_speed` for every cut (`PolygonPartitioner.cc:484`, `:655`, `:695`), as
  `PolygonPartitioner::create`'s comment says. Assign Plate IDs therefore runs the overlay's clip tests at high
  speed, which is fine, but the comment should say so.
- **`PolygonPartitioner.h:280-281`** (`partition_multipoint`): "`GEOMETRY_INTERSECTING` is returned if any
  points were on the boundary of the partitioning polygon" describes a case `partition_point` cannot produce
  (threshold on, a point on the outline is `INSIDE`, `:736-745`), as the body's own comment at `:795-796`
  says. Pre-existing text; a point on the outline is inside, and the result is `INTERSECTING` only when points
  fall on both sides.
- **A point on a rigid block's outline is claimed by the network when the network is sorted before its
  block.** The network's partitioning polygon has the block as a hole
  (`add_partitioning_resolved_topological_network`, `:649-654`), and `partition_point` with the threshold on
  counts a point on any ring's outline as inside, so `GeometryCookieCutter::partition_point` (`:561-564`)
  returns the network for a point within `s` of the block's boundary when the sort has put the network first.
  Pre-existing (the same polygon and test were used before this PR), and the page's "the network never claims
  a block's area" is about area; worth a clause there since the velocity layers' `partition_point` is where it
  shows.
- **Two adjacent runs of one class after a collapsed run are left as two polylines** (`overlay_polyline`,
  `:212-216`): dropping a one-position run leaves its neighbours, which share that position and have the same
  class, as separate pieces rather than one. Not wrong, and the guard is for a case the code's own comment
  says may not arise (section 2).

## 2. Test gaps

Mutations (*run*; each on restored files, against the 97 tests of
`PolygonPartitioner*:PolygonOverlay*:PartitionFeatureUtils*` with seven probes present; a probe failing only on
its own assumption that the cookie cutter's polygons are the fixture's objects is not listed):

| Mutation | Caught by |
| --- | --- |
| M1: shortcut ignores the subject's interior (`!subject_polygon \|\|` -> `true \|\|`, `:93`) | `PolygonPartitionerTest.InteriorBeyondBoundingCircle` |
| M2: shortcut ignores the clip's interior (`:91-92` -> `true`) | `InteriorBeyondBoundingCircle` (and the band probe) |
| M3: both-emptied tie-break reversed (`:146`) | `SliverFilterEmptiesBoth` |
| **M4: collapsed runs kept** (`:216` removed) | **nothing** |
| M6: the cascade drops the input's table (`GeometryCookieCutter.cc:488-492` passes `PartitionedGeometry(outside_geometry.geometry)`) | `CookieCutterCascade`, `CookieCutterOtherGeometryTypes`, `PartitionFeatureUtilsTest.RunAcrossRingStart` |
| M7: a whole geometry always gets the identity table (`add_whole`, `:530-532`) | `CookieCutterCascade`, `CookieCutterOtherGeometryTypes` |
| **M10: the excluded segment counted** (`PointInPolygon.cc`, `excluded + 1` -> `excluded`) | **`PolygonOverlayPrototypeTest.Anchorless` only**; `PolygonOverlayTest.ClassifyAnchorless` passes |
| M12: plate IDs sorted lowest first (`:788`) | `CookieCutterCascade`, `CookieCutterOtherGeometryTypes`, `SliverFilterCookieCutter` |
| M14: the shortcut off (`:87` always true) | nothing: every result unchanged, as an optimisation's should be |

Gaps:

- **`get_crossing_parity_excluding_segment`'s exclusion** (M10) is pinned by the prototype comparison alone.
  `ClassifyAnchorless`'s identical, reversed and jittered rings put the test point within rounding of both
  inputs' excluded planes, so counting the excluded edge flips both parities together or neither, and the
  same-side class is unchanged. Review 1's needle with `a != 0` (the clip's segment off the test point) is
  the hand-built case where only one side would flip; committing it pins both review 1's 1.1 and this.
- **The collapsed-run guard** (M4) is reached by no test. `extract_polylines` (`PolygonOverlay.cc:4126-4201`)
  makes a one-point run only from a non-internal piece whose two stops share a stored position, and the loop
  rule makes such a piece internal (it merges into its neighbour's run), so the guard may be unreachable; if a
  case exists, `PolylineOnSphere::create` would throw on it without the guard. Either construct one or state
  in the comment why none arises.
- **No test uses a speed other than `ADAPTIVE`**, and none the cookie cutter's ungrouped constructor, the
  `reconstruction_geometries` constructor, `SORT_BY_PLATE_AREA`, or a network with a rigid block. *Run*, all
  as expected: `ProbeSpeeds` (ten polygon pairs including the helix band both ways, a sliver, a touch and a
  subject in the clip's hole, at the four speeds, with and without lists, filter off and at `10 h`: the same
  result, piece counts and areas at every speed); `ProbeHemispherePartitioner` (a 36-point equator ring as
  the partitioning polygon at every speed: inside, outside, a straddling box cut into equal halves, a
  polyline, points); `ProbeSortByArea` (feature order X, Y, Z with areas large, small, medium gives X, Z, Y
  by area, Z, Y, X by plate ID, X, Y, Z unsorted); `ProbeCascadeHolePlate` (a plate with a hole and a plate
  equal to the hole, in both orders, filter on and off: two partitions, the hole plate's piece whole and
  the other's with one hole, areas summing to the input's, a 0.125-degree grid of the input in exactly one
  piece each, every provenance resolving; this stands in for a network with its rigid block). These are
  cheap to commit; the hole-plate cascade in particular is the structure the cookie cutter builds for every
  network and nothing committed exercises it.
- **Digitising noise across a shared edge** is tested at the overlay level (common chains) but not through the
  cascade. *Run*, `ProbeNoiseAcrossSharedEdge`: plate A's box with its eastern edge moved `0.4 h` into B,
  and `0.4 h` into A, is whole in A and returned as itself; a box in B whose western edge is `0.4 h` into A,
  either way, is whole in B. Worth committing as the cascade's statement of 5.2's common-chain rule.
- **A polyline along a shared edge in the cascade** (*run*, `ProbePolylineAlongSharedEdgeCascade`): whole in
  A, returned as itself, nothing to B; a polyline along the edge then into B gives A the along part and B the
  rest, nothing outside. Not committed.
- **The cookie cutter's `partition_point` on a shared edge** gives the first plate (*run*,
  `ProbeCookieCutterPointOnSharedEdge`). Not committed; the velocity layers rely on it.
- **A polyline, and a point, inside the band's interior beyond its circle** are `INSIDE` with and without
  lists (*run*, `ProbeBandInteriorOtherTypes`; the committed `InteriorBeyondBoundingCircle` covers polygons
  only), and a polyline near the pole, inside the circle but outside the band, `OUTSIDE`.
- **Holes of the partitioning polygon** (*run*, `ProbeHolesAndAround`): a subject in the hole, equal to the
  hole (either orientation), and an annulus round the hole within the polygon come back whole on the right
  side; a polyline along the hole's edge is inside; a subject surrounding the polygon (the polygon in its
  hole, disjoint) is whole outside; a vertex, an edge point and a hole-edge point are inside; a multi-point
  with one point on the edge is whole inside, itself. `SubjectAcrossPartitionerHole` covers the crossing
  case only.
- `GLReconstructedStaticPolygonMeshes`' catch and the kept redundant test (`:962-1002`) have no test, as the
  file says (no rendering coverage); the result-only path it calls is covered by `ResultOnlyPath`.

## 3. Checked and found correct

- **The bounding-circle shortcut** (`is_outside_by_bounds`, `:79-96`): disjoint circles (both built over
  every ring, `InnerOuterBoundingSmallCircleBuilder::add(PolygonOnSphere)`, expanded by the touching
  threshold), the clip's interior within its circle, and the subject's when it is a polygon; a polyline's
  outline is all there is. A clip whose interior reaches past its circle gives no shortcut and the overlay's
  vertex tests answer (`INSIDE` for a subject in that region, *run*); a subject whose interior does gives no
  shortcut and the clip's ring inside it is `INTERSECTING` (*run*). `is_outside_bounding_small_circle_outside_polygon`
  (`PolygonOnSphere.cc:846-868`) is as 3.3 states: `R` outside the circle, a circle covering the sphere, else
  one threshold-off query at the centre's antipode. M14 shows it is an optimisation only.
- **The result contract** (`overlay_polygon`, `:108-160`; `overlay_polyline`, `:189-231`): whole-in or
  whole-out from `classify()` without extracting; else both extractions on one classification, the emptied
  side's rule, the both-emptied area rule with the diagnostics' deltas; the polyline's runs after the collapse
  filter. The input object itself is returned for every whole answer, for every geometry type, in every entry
  (`partition_polygon`, `partition_polyline`, the visitor's `add_whole`, *run* across the suite's `.get()`
  checks and the probes), and no piece is ever made for a whole answer.
- **Result-only equals lists** at every speed, with and without the filter (*run*): the visitor's no-list
  branches call `partition_polygon` / `partition_polyline` / `partition_multipoint`, which build the same
  `Options` (`clip_speed`, `sliver_min_width`; provenance options cannot change a result) and the same
  overlay and extraction; the `partition` helper in the test file compares all four forms on every polygon
  case.
- **Points and multi-points**: `partition_point` is `is_point_in_polygon(point, speed)` with the threshold on,
  unchanged from `main`; `partition_multipoint` unchanged; the visitor's multi-point branch agrees with it and
  adds the whole-side-as-itself rule and per-point provenance (the input's entry, or its index). A point on
  the outline is inside in every path (*run*).
- **The cascade** (`GeometryCookieCutter::partition`, `:388-539`): the outside list of each plate is the next
  plate's input; pieces are polygons with holes; the far plate returns pieces whole with their tables (M7);
  the tables compose through `partition_geometry(PartitionedGeometry, ..., CascadeProvenance)` (M6);
  `clip_index` is the position in `d_partitioning_geometries`, which `get_partitioning_polygons` returns in
  the same order (`:341-351`, `:464`), and the committed test checks index 2 for the third plate; the
  no-plates path gives the identity table. Area is conserved and no region is claimed twice (*run*,
  `CookieCutterCascade` and `ProbeCascadeHolePlate`'s grid).
- **The order** (`:103-324`, `:572-767`): networks (each block, then the network with blocks as holes), then
  boundaries, then static polygons; each group `std::stable_sort`ed by plate ID or area, highest first; the
  ungrouped path sorts the whole list; the third constructor's ungrouped path follows feature order. Sort by
  area is stable on a tie (*run*).
- **The widths**: `get_default_sliver_min_width` is `10 * GeometryIntersect::Intersection::get_on_segment_start_threshold_sine()`
  (`:327-331`), `h` of section 4.4; every constructor defaults to it and passes it to every partitioner
  (`PartitioningGeometry`, `:770-780`); Assign Plate IDs' two constructions take the default;
  `PyPlatePartitioner.cc:64-82` maps `None` to the default, `0` to off and negative to `ValueError`;
  `PyGeometriesOnSphere.cc:3648-3708` defaults `PolygonOnSphere.partition()` to `h`;
  `GLReconstructedStaticPolygonMeshes.cc:918-919` creates its partitioner with no width, so the meshes stay
  exact; `partition_polyline` never sets a width and the overlay reads it only in `extract` (`PolygonOverlay.cc:4049`).
- **Exceptions**: `PolygonPartitioner` and the cookie cutter catch nothing; `GLReconstructedStaticPolygonMeshes`
  catches `PolygonOverlay::InconsistentOverlayException` and takes the node as intersecting (`:969-977`);
  `AssignPlateIds.cc:300-305` catches it per feature; pyGPlates exports it (`PyExceptions.cc:483`).
- **`GenerateVelocityDomainCitcoms.cc:269-280`**: `GeometryIntersect::intersect` on two two-point polylines,
  false with an empty graph when they miss, else `unordered_intersections[0].position`.
- **`PlateVelocityUtils.cc`**: `GeometryCookieCutter::partition_point` as before (first plate containing the
  point, `:552-568`), constructed without networks at high speed; the unused width default is harmless.
- **`get_crossing_parity_excluding_segment`** (`PointInPolygon.cc:2289-2345`): the global segment index
  through exterior then interior rings as `get_segment` numbers them; each edge's crossing is decided from its
  own two end points' sides of the crossing plane and its own plane (`get_num_polygon_edges_crossed`,
  `:301-440`), so summing the two ranges either side of the excluded edge is the ring's count less that edge's;
  the degenerate-arc guard is the caller's (`PolygonOverlay.cc:3064-3073`), as the header says. Review 1's 1.1
  (the clip-side test point) stands and is not this function's fault.
- **Identity provenance** (`:539-598`, `:827-834`): ring 0 for a point, multi-point and polyline; a polygon's
  interior ring `i` is ring `i + 1`; no ratio.
- **Code style** in the files read: tabs, Allman braces, broken signatures, `&name`, nothing over the
  neighbours' width.

## 4. Page statements to amend

- **5.6**, "`PolygonPartitioner` and `GeometryCookieCutter` let the exception propagate, with or without
  lists": for the cookie cutter without lists the cascade returns at the first plate that claims a piece, so
  only the plates consulted can raise (1.1). Say so, or remove the early return and leave the sentence.
- **5.1**, "A piece belongs to the first polygon that contains it": add that a *point* on a shared boundary
  (within `s`) belongs to the first plate too, and that a point on a rigid block's outline belongs to the
  network when the sort puts the network first, since the block is a hole of the network's polygon and a hole's
  outline is inside (1.2); "the network never claims a block's area" is about area.
- **5.1**, the both-emptied rule, "and outside on a tie": add that a tie needs exactly equal dropped areas
  (the no-filter case, where both are zero; *run*: a symmetric ribbon did not tie), or change the rule (1.2).
- **5.1**, "points and multi-points by point-in-polygon (`partition_point`, threshold on, a point on the
  outline inside)": could add "a hole's outline included", which the probes confirm and which matters for
  the network-with-holes polygon.
- **`GeometryCookieCutter.h:144-145`** and **`PolygonPartitioner.h:280-281`** as in 1.2.
