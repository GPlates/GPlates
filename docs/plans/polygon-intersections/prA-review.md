# Review: PR A (GeometryIntersect audit fixes), 601ca9e45..HEAD, src/ + changelogs

Read-only review of the eight commits. No correctness defect found in the production code.
Two low-severity findings, both in the new test file (coverage, not wrong assertions).

## Findings (most severe first)

### 1. Test "zero_length_segments / at a vertex" does not use a zero-length segment (low)

`src/unit-test/GeometryIntersectTest.cc:441-456`. The comment says "A zero-length segment of P at
Q's vertex", but P's segment 1 runs from `F.at(-0.3H, -0.3H)` to `F.at(0.3H, 0.3H)`, length
0.6*sqrt(2)*H = 0.85 H = 1.20e-6 rad. The zero-length bound is sin(theta) <= 1e-6
(`GreatCircleArc.cc:672`, `v.magSqrd() <= 0.0` as a real_t, i.e. <= EPSILON = 1e-12), so this
segment has a plane and takes the non-zero-length path through `intersect_segments`. The expected
result (two SS records, both on Q's segment 1) is the same either way, so the test passes and is not
vacuous, but the case the audit names - a genuinely zero-length segment whose ends coincide with a
vertex, which skips every plane test - is not covered. Fix: use +-0.3H/sqrt(2) or smaller
(e.g. `F.at(-0.2*H, -0.2*H)`, `F.at(0.2*H, 0.2*H)` gives 0.57 H = 0.8e-6 rad). The "straddling an
edge" case (0.5 H = 0.71e-6) is a true zero-length segment. Confidence: confirmed by reading.

### 2. Test "pruning / one touch" does not exercise the bounding-circle expansion (low)

`src/unit-test/GeometryIntersectTest.cc:699-729`. The vertex touches 0.99 H outside the midpoint of
exterior edge 50. The midpoint of an edge is ~1.6e-3 rad closer to the leaf node's centroid than the
leaf's vertices are, so it lies deep inside Q's unexpanded bounding circle, and the vertex is a point
of P's leaf; the two circles intersect with zero expansion. So the test checks the tree descent, not
that `BoundingSmallCircleBuilder::get_default_angular_expansion()` is at least the thickness (the
property the new comments at `GeometryIntersect.cc:1034-1039` and `SmallCircleBounds.cc:450-452`
state). A case that needs the expansion: two polylines collinear end to end with a 0.99 H gap
(an SS contact); each leaf's circle ends at its segment's end, so centres-distance minus radii-sum
is 0.99 H and only the expansion (2 H across the pair) makes the circles intersect. Confidence:
plausible (the leaf partitioning of a 200-gon was not computed exactly, but the midpoint argument
holds for any leaf containing edge 50).

## Checked and found correct

### 1. `interpolate_plane_crossing` (GreatCircleArc.{h,cc})
- Body is byte-for-byte the old anonymous-namespace function; only the signature changed from
  `(const GreatCircleArc &arc, ...)` to the arc's two position vectors. Same half-circle test
  (`magSqrd() <= 1e-6`, `boost::none`), same denominator, same `get_normalisation()`.
- The two callers in the distance code (`GreatCircleArc.cc:899-907`) pass
  `arc.start_point().position_vector(), arc.end_point().position_vector()` and the same dot
  products as before. Behaviour for the distance code is exactly unchanged. No other
  `interpolate_plane_crossing` exists in `src/`, so no ODR clash from making it public.

### 2. GeometryIntersect.cc
- Thickness: `1 - EPSILON` as cosine; sine^2 = 2e-12 - 1e-24. Claims verified: equals
  `PointInPolygon::get_point_on_polygon_threshold_cosine()` (`PointInPolygon.cc:2272`, `1 - 1e-12`);
  is real_t equality of unit vectors (`UnitVector3D.h:265`, `dot(u1,u2) >= 1.0` as real_t is
  dot >= 1 - 1e-12); exceeds the zero-length bound (1e-6 < 1.414e-6; the header comment's
  "about 1e-6" and "0.41e-6" are right); thickness^2 (2e-12) > EPSILON (1e-12), so
  `cross_segment_planes.magSqrd() > 0` (a real_t compare, true iff > 1e-12) always holds for a
  crossing: for p on circle 1, |p.n2| <= sin(angle between planes) + ~1e-16, and a crossing needs
  |p.n2| > 1.414e-6. Under the old thickness the margin was 1e-12 vs 1e-12, i.e. none.
- Consumers of the thickness all read the one function: `THICKNESS_THRESHOLD_COSINE/SINE`,
  `BoundingSmallCircleBuilder::get_default_angular_expansion()` (and through it the bounding tree
  nodes, `PolyGreatCircleArcBoundingTree.h:708`, and `InnerOuterBoundingSmallCircle`'s default),
  `ResolvedSubSegmentRangeInSection.cc:636,660` (on-segment-end test). `PointInPolygon.cc:101`
  uses the zero-length cosine only as an extra bounds-widening margin, independent of this.
  Pruning soundness: contact within t implies centre distance <= R1 + R2 + t < (R1+t) + (R2+t).
- Touch key: all four touch call sites pass the touched segment's plane - `segment2_plane` for
  SEGMENT1_START_ON_SEGMENT2 (`:865`, `:961`), `segment1_plane` for SEGMENT2_START_ON_SEGMENT1
  (`:898`, `:998`) - and `add_intersection` computes the angle from the touched segment's start
  point (`:207-213`). SS and crossing sites pass none and never dereference the optional. The
  projection cannot be zero (|p.n| <= 1.4e-6). A crossing's key is the angle from the segment start
  to a point on the segment's great circle; the touch key is now the same quantity, so records on
  one segment sort by along-segment position. The lune test keeps the projection in [start, end]
  up to rounding; `create_from_cosine` clamps.
- Replacement of the three interpolation branches: the fallback is unreachable (above); if reached,
  the assertion fires only when both chords have |a+b|^2 <= 1e-6 AND the planes are within 1e-6 of
  parallel, which a crossing excludes. Denominators are >= 2 * 1.414e-6. No valid input reaches it.
- `std::stable_sort`: comparator orders by segment index, then strict `>` on cosine doubles
  (`AngularDistance::is_precisely_less_than`) - a strict weak ordering (no NaN source). Insertion
  order is the bounding-tree descent (children 0 then 1, geometry1 segments outer loop), which is a
  deterministic function of the two geometries, so tied records are in a deterministic order.
- Angle-field consumers (full grep of `src/`):
  - `src/app-logic/ResolvedSubSegmentRangeInSection.cc:536-550` copies the angle into its own
    `Intersection`; it is used only by `operator<` (`.h:155-160`, segment index then precise
    cosine compare) - which `TopologyIntersections.cc:686,705` uses to decide start vs end - and by
    the precondition assert at `.cc:101-105` (epsilon `<=`). The interpolation ratio is derived from
    the position (`.cc:758-760`: `acos(dot(position, segment start)) / arc_length`), not the angle.
    So the key change moves no point; it only fixes the start/end ordering in the D1 case.
  - `src/maths/PolylineIntersections.cc:101,269,305,338,382,424,437`: `angle_in_segment_ptr` is
    passed through to `create_partitioned_polyline` and never read there (lines 130-258 contain no
    use). Ordering comes from `geometry*_ordered_intersections`. Positions from `position`.
  - `src/maths/PolygonPartitioner.cc` and `GenerateVelocityDomainCitcoms.cc` do not read the angles.
  No consumer uses the angle as a distance or fraction.

### 3. src/unit-test/GeometryIntersectTest.cc (beyond the two findings)
- `Frame::at` is a gnomonic projection (normalise(C + eE + nN)), so straight lines in (east, north)
  are great circles, points `F.at(e, 0)` are exactly on one great circle, and the projection of
  `F.at(e, n)` onto it is exactly `F.at(e, 0)`. `angle_between` (atan2 of cross and dot) is
  accurate near zero. Tangent frame is right-handed and the centre (lat 0.37, lon 0.81) is off-axis.
- `ordering_within_a_segment` pins F2: new key 3.0e-6 vs old chord key 3.31e-6 vs crossing 3.15e-6;
  the 1e-9 tolerances and the order assertion both fail with the old key. `vertex_on_edge`'s
  1e-12 tolerance on the key also fails with the old key (chord minus projection ~9e-11, with
  acos resolution ~1e-14 at 0.011 rad). `constants` pins F1 by equality with PointInPolygon.
- Distances stated in H check out: short_crossing_segment (0.75 H, 1.5 H, 1.5 H sin 20 = 0.51 H),
  corner_wedge right angle (SS iff a*sqrt(2) < H, so 0.5 yes, 0.72 and 0.99 no; corner-to-arriving-
  segment distance ~0.67 a is within H), 30-degree wedge (r sin 15 vs H: 3.8 -> 0.98 H, 4.2 ->
  1.09 H), near_parallel (2e-6 beyond, 0.5e-6 within), vertex_on_edge 0.99/1.01 H. Touches named
  as touches are touches (end points within thickness, so the crossing branch is not entered).
- Geometry index expectations in `rings` (3, 7, 5, 1 along P; 1, 3, 5, 7 along Q; SS at ring
  index 0; hole SS at 2 and 4) follow from vertex order. In "hole touching the exterior" P's two
  segments happen to be collinear with hole edges 6 and 4 beyond the shared vertex; the lune test
  excludes them, so the count of 2 is right.
- `twist_on_an_edge` has a genuine key tie (two crossings at one point) and accepts either order.
- `expect_symmetric` compares tied SS records index-by-index in both orientations
  ("zero_length_segments / at a vertex"); this relies on insertion order matching after the swap,
  which it does here because both leaf loops visit P's segments 1 then 2. Fragile, not wrong.

### 4. Deleted code
`arcs_are_near_each_other`, `arcs_lie_on_same_great_circle`, `arcs_are_directed_equivalent`,
`arcs_are_undirected_equivalent`, `SmallCircle::intersection`, the `#if 0` `GreatCircle::intersection`
and `GeometryCrossing.{h,cc}`: no reference remains in `src/`, `cmake/`, Python files or the
pygplates API (`src/api` has no SmallCircle binding); `src/maths/CMakeLists.txt` dropped both
GeometryCrossing entries (it was in both `srcs` and `gplates_only_srcs`). Only docs/plans and the
foundation page mention them historically. No platform-conditional code involved.

### 5. Changelogs
With R = 6371 km: new thickness acos(1 - 1e-12) = 1.414e-6 rad = 9.01 m ("about 9 metres");
old asin(1e-6) = 6.37 m ("about 6 metres"); "same distance as on the outline" is exact
(both `1 - 1e-12`); "typically by less than 10 metres" matches the commit's measured
1.01-1.08 thicknesses (9.1-9.7 m). `PolygonOnSphere.partition()`/`PlatePartitioner` and
Assign Plate IDs do go through `PolygonPartitioner` -> `GeometryIntersect`
(`GeometryCookieCutter.cc`, `PyGeometriesOnSphere.cc`). Wording "two geometries less than 9 m
apart" is a simplification (it is vertex-to-other-geometry distance), acceptable for a changelog.
