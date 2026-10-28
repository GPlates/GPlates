# Phase 3: audit of `GeometryIntersect` against the intersection requirements

Written 2026-10-25 (Fable), for Phase 3 of `PLAN.md`. It audits `src/maths/GeometryIntersect.{h,cc}`
against requirements R1 to R12 of `phase2-requirements.md` section 2, and against
`GreatCircleArc::intersect`. Fixes are proposed, not made.

Every finding is marked **probe** (measured with the pyGPlates build in `build-pygplates/bin`, through
`PolygonOnSphere.partition`, which cuts a polyline at every record along it, so the piece boundaries are
the records), **exact** (computed in rational arithmetic by the reference tool, or derived from the
constants as the C++ computes them), or **reading** (from the code, with reasoning). Line numbers are
those of the branch at `7bc5020e7`.

The scripts are in the planning repository, `plans/polygon-intersection-tools/` (moved there from
the session scratchpad they were written in):
`constants.py` (the thresholds as the C++ computes them), `probe_common.py` (the observation method),
`probe_*.py` (one probe each, named below), `arc_ref.py` (the exact reference and the degenerate
generators) and `hunt_arcs.py` (the hunt). Section 6.2 describes the tool.

Notation as in the requirements file: `P` is geometry 2 (the polyline cut by `partition`), `Q` is geometry 1
(the partitioning polygon), `t` is the thickness, `s` the point-on-outline threshold. The four record types
are abbreviated LL (`SEGMENTS_CROSS`), SS (`SEGMENT_START_ON_SEGMENT_START`), SL
(`SEGMENT1_START_ON_SEGMENT2`) and LS (`SEGMENT2_START_ON_SEGMENT1`).

## 1. Summary

### 1.1 The constants, as the C++ computes them (exact, `constants.py`)

| Constant | Where | Value |
| --- | --- | --- |
| `THICKNESS_THRESHOLD_COSINE` (`T_COS`) | `GeometryIntersect.cc:47`, from `GreatCircleArc::get_zero_length_threshold_cosine()` (`GreatCircleArc.cc:378-384`) | `1 - 5.00044e-13` |
| `THICKNESS_THRESHOLD_SINE` (`t`) | `GeometryIntersect.cc:50`, `sin(acos(T_COS))` (`:1421`) | `1.0000444e-6` |
| Zero-length bound | `is_zero_length`: `cross(a, b).magSqrd() <= EPSILON` (`real_t`), so `sin(length) <= 1e-6` | `1e-6` |
| `s` | cosine `1 - 1e-12` | `1.4141979e-6` |
| Cosine resolution near zero | `acos(1 - 2^-53)` | `1.49e-8` rad |

So `t` exceeds the zero-length bound by `4.4e-11` (relative `4.4e-5`), which is what the tunnelling
argument needs (section 3.2), and `t^2 = 1.000089e-12 > EPSILON`, which makes the chord-interpolation
branches of `add_segments_crossing_intersection` unreachable (section 3.1).

### 1.2 Verdicts

| Req. | Verdict | One line |
| --- | --- | --- |
| R1 | Met | Both segment indices and both angles are recorded (`add_intersection`, `:144-188`); the touch position is the touching vertex. |
| R2 | **Met with caveats** | One record per shared vertex when the vertices coincide within `t` (probe, hunt). A vertex of `P` in the inner wedge of a corner of `Q`, within `t` of both edges' planes but further than `t` from the corner, gets one touch record per edge, and just beyond the coincidence disc a third (probe, exact). The SS position is not always geometry 1's vertex (reading). |
| R3 | Not provided | By design; the overlay does not need crossing/bouncing labels (section 4.2). |
| R4 | Met | Every vertex of either geometry inside an overlap is a record, interleaved, reversed, from a shared vertex, along a zero-length segment, and round a corner (probe `probe_overlap.py`, hunt `collinear`). |
| R5 | **Met with caveats** | Sorted by segment index then angle along every ring, indices global across rings (reading). The angle of a touch record is the chord angle to the touching vertex, which is up to `t` off the segment, so records within `sqrt(2) t` of each other near a segment's start can sort in the wrong order (probe `probe_misorder.py`). Ties sort in unspecified order. |
| R6 | Met | The polygon overloads (`:1290-1396`) take all rings by default; the bounding tree covers every ring with per-ring partitions (`PolygonOnSphere.cc:889-940`). |
| R7 | **Not met, by a known gap** | Contact up to `d = 1.0e-6`, none from `d = 1.0001e-6`; `is_point_in_polygon` (threshold on) calls the vertex inside up to `d = 1.4142e-6` (probe `probe_gap_r7.py`). Decision Q1, section 4.1. |
| R8 | Met | Zero length as described. Near-antipodal: the axis of an arc `pi - 1.5e-6` long is accurate to about `1e-10` rad, and the crossing position error measured `0` (probe `probe_antipodal.py`). The requirements' "known gap" is refuted. |
| R9 | Met | Re-fed pieces give touch records at their rounded end vertices; a boundary moved by `1.2e-6` gives clean crossings (probe `probe_cluster2.py`). No tunnelling in 4000 near-vertex passes (probe `probe_tunnel.py`). The near-collinear "position without lune check" gap is refuted: the thick test bounds the plane angle below by `t`, so the cross product is always well conditioned (exact, section 3.1). |
| R10 | Met | The same dot products decide a vertex as the end of one segment and the start of the next, bit for bit (reading, section 3.2). Ordering is tolerant only through R5's caveat. |
| R11 | Met | Pruning is sound with a margin of `t` less the cosine resolution (reading, section 3.4). |
| R12 | Met | The specification is symmetric (exact, 597 cases, `probe_symmetry_exact.py`); the C++ matches the specification (hunt, 3600 cases, 0 mismatches). Positions of SS records differ by up to `t` between the two orders, as documented. |

### 1.3 Defects found, ranked

1. **D1 (R5, R10). Touch records can sort in the wrong order.** `add_intersection` (`:163-172`) computes
   the angle of an SL or LS record as `acos(dot(vertex, segment_start))`, the chord angle to a vertex
   that is up to `t` off the segment. Near a segment's start this exceeds the along-segment angle of a
   crossing that lies further along. Probe: a vertex `3.0e-6` along and `0.99e-6` off a segment sorts
   after a crossing `3.15e-6` along; the records are `1.0012e-6` apart, beyond the contraction tolerance
   `t`. The misordering band is records within `sqrt(2) t` of each other. Fix F2.
2. **D2 (R7). The thickness is `1.0000444e-6`, the on-outline threshold is `1.4141979e-6`.** A vertex
   in between is on the outline for point-in-polygon and no record for `GeometryIntersect`. Decision Q1,
   recommended answer: raise the thickness to `s` (section 4.1). Fix F1.
3. **D3 (R2). Duplicate touch records at a corner.** A vertex `x` of `P` within `t` of the planes of two
   consecutive edges of `Q`, inside both lunes, and further than `t` from their shared vertex `v`, is
   reported once by each edge. At a right angle this needs `|x - v|` in `(t, sqrt(2) t)`; at an interior
   angle `theta` the wedge reaches `t / sin(theta / 2)` from `v`. Just beyond the coincidence disc a third
   record appears: `v` on the segment of `P` that arrives at `x`. Probe: `|x - v| = 1.018e-6` at a right
   angle gives three records; `3.8e-6` on the bisector of a 30-degree corner gives two. Raising the
   thickness to `s` does not remove it: the plane thickness and the coincidence radius are the same
   constant, so the wedge only scales by `sqrt(2)` (exact reference at `s`, 2026-10-28: three records
   for `a = 0.72 s` to `0.99 s` at a right angle). Fix F3, and contraction for the third record.
4. **D4 (specification, not code). A segment shorter than `2t` that crosses an edge gives two touch
   records up to `2t` apart and no crossing.** Probe: a `1.5e-6` segment gives two records `1.5e-6`
   apart. The requirements file says such touches are "within `1e-6` of each other"; they are not, so a
   contraction tolerance of `t` leaves a piece lying along `Q` whose midpoint test is meaningless. The
   contraction tolerance must be at least `2t` (or `2s`), or a piece between two consecutive touch
   records with no crossing must be classed as common. Section 7, correction 5.
5. **D5 (dead code).** The three chord-interpolation branches of `add_segments_crossing_intersection`
   (`:233-396`) are unreachable in practice: a crossing needs `sin(phi) > t = 1.0000444e-6` between the
   planes, and the cross-product branch is taken whenever `sin(phi)^2 > EPSILON = 1e-12`. The window
   has negative width (exact). The plan's intention to port `GreatCircleArc::intersect`'s
   better-conditioned-arc choice and lune check is therefore moot. Also dead, confirmed by search of
   `src/`: `arcs_are_near_each_other`, `arcs_are_directed_equivalent`, `arcs_are_undirected_equivalent`
   (and `arcs_lie_on_same_great_circle`, used only by it), `SmallCircle::intersection`,
   `GreatCircle::intersection`, and all of `GeometryCrossing`. Fix F4.
6. **D6 (documentation).** The header (`GeometryIntersect.h:72-75`) and design doc section 4.2 say an SS
   record is positioned at the first geometry's vertex. At the second geometry's last vertex (its
   fictitious segment, `:604-622`) the position is the second geometry's end point. Section 7.

No missed contact, spurious contact or wrong position was found: 3600 generated cases against the exact
reference (section 6.2), 4000 near-vertex passes, and every hand-built degenerate case agree with the
specification. The defects are in the ordering key, the choice of tolerance, and duplicate reporting.

## 2. The requirements, one by one

### R1. One record per contact, positioned on both geometries

**Met** (reading, probe). `add_intersection` (`GeometryIntersect.cc:144-188`) stores both segment indices
and, for LL, both angles; for SL the angle along segment 2 and zero along segment 1; for LS the reverse;
for SS both zero. The position of an LL record is the normalised cross product of the two planes with the
sign chosen by the orientation (`:211-232`). The position of a touch is the touching vertex (`:954`,
`:986`, `:1046`, `:1082`), up to `t` off the other segment. Every probe recovered the records at the
expected positions (section 6.1).

### R2. A shared vertex is reported once

**Met with caveats.**

Met for coincident vertices (reading, probe, hunt). For two consecutive segments `A_prev`, `A` of one
geometry sharing vertex `v`, and a segment `B` of the other starting at `x` with `dot(v, x) >= T_COS`:
the pair `(A, B)` makes the SS (`:537-550`); `(A_prev, B)` excludes the crossing (`:750-753`) and its LS
test is blocked by `!segment1_end_point_on_segment2_start_point` (`:973`); `(A_prev, B_prev)` and
`(A, B_prev)` are blocked the same way. Probe `probe_cluster.py` (d), (e): a `P` vertex `0.5e-6` from, or
identical to, a `Q` vertex gives one record. Probe `probe_overlap.py` (e), (f): `P` running vertex to vertex
along one or two edges of `Q` gives one SS per shared vertex and nothing else. A polyline's last vertex is
covered by the fictitious segment (`:584-641`, `:1021-1091`); probe `probe_last_vertex.py` shows a
polyline whose only contact is its last vertex, on an edge or on a vertex, is `intersecting` up to
`d = 0.99e-6` and not at `1.01e-6`, forwards and reversed alike.

Not met in the inner wedge of a corner (probe `probe_corner.py`, exact). Take `Q`'s north-west corner
`v = (0.5 deg, 0)`, edges running west into `v` and south out of it, and a vertex `x` of `P` at
`(-a, +a)` from `v` (south, east), `a <= t`. Then `x` is within `t` of both planes, inside both lunes, and
`|x - v| = a sqrt(2)`. Measured:

| `a` | `|x - v|` | Records along `P` (exact reference, confirmed by probe) |
| --- | --- | --- |
| `0.5e-6` | `0.707e-6` | one: SS |
| `0.72e-6` | `1.018e-6` | three: SL (`v` on `P`'s segment arriving at `x`), LS on `Q` segment 2, LS on `Q` segment 3, the last two both positioned at `x` |
| `0.99e-6` | `1.400e-6` | three, as above |

At a 30-degree corner, with `x` on the bisector, two LS records (one per edge) for `r` up to `3.8e-6`
(`t / sin(15 deg) = 3.86e-6`); at `4.2e-6` two LL records, which is right (`P` cuts the corner). The
wedge beyond the coincidence disc, in which this happens, has area about `t^2 (1 - pi / 4)` at a right
angle and grows as the angle sharpens. The cascade can put a vertex there: a plate corner that a piece's
rounded vertex lands near.

What the duplicates do to a consumer: along `P` the two LS records have equal keys (same vertex, angle
zero), so the piece between them is two identical points; along `Q` they bracket `v`, so `Q` has a piece
`[x, v, x]` of length up to `2 t / sin(theta / 2)`. Both must be contracted or merged by the consumer.

The SS position caveat (reading): `:616` uses `segment2_end_point` as the position when segment 2's last
vertex coincides with segment 1's start, against the header's statement.

### R3. Crossing, touch and overlap distinguishable, the same from both sides

**Not provided**, as the requirements file says. The information to derive it is present (R1, R4, R5).
Whether to derive it is Q9 (section 4.2): under the overlay it is not needed.

### R4. An overlap is recoverable as a chain

**Met** (probe `probe_overlap.py`, hunt `collinear`, 600 cases). `P` along `Q`'s west edge (the meridian
`lon = 0`, from `lat -0.5` to `0.5` degrees):

| Case | Records along `P` |
| --- | --- |
| (a) `P` vertices at `lat -0.2, 0.1, 0.3`, `Q` vertex at `lat 0` inside the stretch | 4: `P1`, `Q4`, `P2`, `P3`, in that order |
| (b) overlap from a shared vertex | 2: SS at the shared vertex, then `P2` |
| (c) two `P` vertices `0.3e-6` apart (a zero-length segment) in the stretch | 4: both zero-length end points reported |
| (d) `P` reversed | 4, in `P`'s order |
| (e) `P` exactly along the whole edge | 2: SS, SS |
| (f) `P` along the edge and on round the corner along the next | 3: SS, SS, SS |

So the chain is the run of consecutive records along both geometries, every vertex inside it is a record,
and the direction follows from the record order along each geometry, as section 3.1 step 3 of the
requirements assumes. The header's four overlap diagrams (`GeometryIntersect.h:291-322`) are reproduced by
the hunt's `collinear` generator with offsets `0`, `1e-12`, `1e-9`, `5e-7`, `0.95e-6`, `1.0e-6`, `1.05e-6`,
`1.2e-6`, `1.4e-6`, `1.5e-6`, `3e-6` either side, interleaved and reversed: 0 mismatches against the exact
reference.

### R5. Ordering along each ring, interior rings included, with wrap-around

**Met with caveats.**

Met (reading): `SortGeometryIntersection` (`:64-111`) sorts by segment index, then by angle with
`is_precisely_less_than` (`AngularDistance.h:201-206`, an exact comparison of cosines). Segment indices
come from the bounding tree leaves (`:1212`, `:1222`), which index the polygon's whole arc sequence
(`PolyGreatCircleArcBoundingTree.h:185-189`), the same numbering as `PolygonOnSphere::get_segment`
(`PolygonOnSphere.h:837-848`): exterior ring, then each interior ring. Each ring's last segment ends at the
ring's first vertex, so a contact there is recorded on the ring's first segment, index relative to the
ring's start. The consumer must know the rings' segment ranges to close each ring; the `Graph` does not
say. The hunt's `--ring` mode (900 cases, `P` a polygon ring, so its wrap-around is exercised through
`PolylineIntersections`' ring-as-polyline cut) found 0 mismatches.

Caveat 1, the ordering key of a touch (probe `probe_misorder.py`, reasoning). For SL and LS the angle is
`acos(dot(vertex, segment_start))` (`:165-171`): the chord angle to a vertex up to `t` off the segment,
`sqrt(x^2 + y^2)` for a vertex at `x` along and `y` off. A crossing at `x2 > x` on the same segment has
angle `x2`. They sort wrongly when `x2^2 - x^2 < y^2 <= t^2`. Measured, `P` along the equator from
`lon 0`, `Q` with a vertex `v` at `(lat y, lon x)` and an edge crossing the equator at `lon x2`:

| `x` | `y` | `x2` | Order along `P` | Separation |
| --- | --- | --- | --- | --- |
| `1.3e-6` | `0.9e-6` | `1.5e-6` | crossing, then `v` (wrong) | `0.92e-6` |
| `3.0e-6` | `0.99e-6` | `3.15e-6` | crossing, then `v` (wrong) | `1.0012e-6` |
| `3.0e-6` | `0.99e-6` | `3.5e-6` | `v`, then crossing (right) | |

The second row is beyond the contraction tolerance `t`, so contraction at `t` would not hide it: the
consumer would get a piece of `P` running backwards from `3.15e-6` to `3.0e-6`. The misordering band is
records within `sqrt(2) t` of each other (`2.0e-6` if the thickness becomes `s`). The same key is used by
topology resolving to order intersections within a segment (`ResolvedSubSegmentRangeInSection.h:155-160`)
and in its precondition (`ResolvedSubSegmentRangeInSection.cc:102-104`); its interpolation ratio is
computed from the position, not the angle (`:742-774`), so a corrected key does not move any point.

Caveat 2, ties (reading): two records with equal `(segment index, angle)` sort in unspecified order
(`std::sort`). Equal keys arise for two crossings at one point (a twist on an edge: probe
`probe_cluster.py` (c) and `probe_cluster2.py` (g), two LL records at one point, adjacent), for two
touches by the same vertex (D3), and for records within the cosine resolution (`1.5e-8` rad near a
segment's start). All lie within the contraction tolerance, so the consumer's contraction makes the
order immaterial, provided it contracts by *key equality* as well as by distance.

### R6. Both polygons' interior rings participate

**Met** (reading). `intersect(polygon, polygon, true, true)` (`:1298-1308`) uses `get_bounding_tree()`,
which covers all rings with partition separators at each interior ring's start
(`PolygonOnSphere.cc:907-928`, `PolyGreatCircleArcBoundingTree.h:453-605`). Only `PolylineIntersections`
passes `false` (`PolylineIntersections.cc:500-501`, `:546`, `:590`). A hole sharing a vertex with the
exterior ring gives one record per ring at that vertex, which is what splitting both rings needs
(reading: the two rings' segments are distinct segments of geometry 1, each making its own SS).

### R7. Consistency with the point-on-outline threshold

**Not met** (probe `probe_gap_r7.py`). A polyline whose first vertex is `d` outside the middle of a
square's north edge, heading away:

| `d` (rad) | `partition` | `is_point_in_polygon` (threshold on) |
| --- | --- | --- |
| `0.5e-6` to `1.0e-6` | intersecting | inside |
| `1.0001e-6` to `1.4142e-6` | outside (no record) | inside |
| `1.415e-6` and above | outside | outside |

The detection threshold is `t = 1.0000444e-6` exactly as computed in section 1.1. The gap is
`(1.0000444e-6, 1.4141979e-6)`. Under the overlay it only means that such a vertex is not a split point
and the sliver, at most `0.41e-6` wide, is classified exactly; but it also means `PolygonPartitioner`'s
"no intersection, so classify by one vertex with the threshold off"
(`PolygonPartitioner.cc:598-610`) is not the same answer as the threshold-on query for that vertex. The
decision is Q1 (section 4.1).

### R8. Zero-length and near-antipodal arcs

**Met.**

Zero length (reading, probe). A zero-length segment has no plane (`:645-646`) and cannot cross
(`:746-747`); its start point is tested against the other segment's plane and lune (`:930-960`) and its
end point is the next segment's start. Probe `probe_overlap.py` (c): both end points of a `0.3e-6`
segment lying on `Q`'s edge are records. Probe `probe_cluster2.py` (h): a `P` segment of about `1e-6` at a
`Q` vertex gives two SS records (one per `P` vertex), positioned at `Q`'s vertex. The tunnelling premise
(a zero-length segment's end points cannot be on opposite sides of a thick plane) needs `t` at least the
zero-length bound: `1.0000444e-6 >= 1e-6`, with a margin of `4.4e-11`, which is far above rounding
(`1e-16`) but is a coincidence of two constants derived separately; a test should pin it (section 6.1,
test 16).

Near-antipodal (reading, probe `probe_antipodal.py`). `GreatCircleArc::create` refuses
`dot <= -1.0` as a `real_t` (`UnitVector3D.h:318-323`), so an arc is at most `pi - 1.414e-6` long and
its end points' cross product has magnitude at least `1.4e-6`; the normalised axis is then accurate to
about `2e-16 / 1.4e-6 = 1.4e-10` rad, far below `t`. So the thick-plane classification is sound for every
constructible arc. A triangle with an edge `pi - gap` long crossed by a short meridian arc, for `gap` in
`1e-2`, `1e-4`, `1e-5`, `3e-6`, `1.5e-6`: one LL record each, position error `0` against an independent
double cross product. The requirements' "no refinement handles that" is true but no refinement is
needed.

### R9. Cascade feedback

**Met** (probe `probe_cluster2.py` (i), exact; probe `probe_tunnel.py`; reasoning).

A square `P0` straddling `Q`'s east edge is cut (two inside pieces, one outside). Fed back:

- to `Q` itself: the exact reference shows an LS record at each piece end (the rounded crossing points,
  `1e-16` off `Q`'s edge, are on its thick plane), and `partition` reports `intersecting`;
- to `Q'` sharing that edge bit for bit, or moved `5e-7`: the same;
- to `Q'` with the edge moved `1.2e-6`: the inside pieces are `outside` (no record), and the outside
  piece gets two LL records `1.2e-6` from its end points. That is the R7 gap seen from the cascade: a
  sliver `1.2e-6` wide, exactly classified.

No tunnelling: 4000 random passes of a two- or three-vertex polyline within `+-3t` of a `Q` vertex at
random angles, with the end points on different sides by `is_point_in_polygon`, all produced a record.

The "near-collinear crossing position without the lune check" gap is refuted (section 3.1): the crossing
position is always the normalised cross product, accurate to about `1e-10` rad.

### R10. What is exact and what is tolerant

**Met** (reading). The coincidence tests are `dot >= T_COS` (`:448-456`); for `(A_prev, B)` the relevant
operand is `dot(A_prev.end, B.start)` and for `(A, B)` it is `dot(A.start, B.start)`: the same two
`UnitVector3D`s in the same order, so the same `double`. Likewise `dot(v, segment2_plane)` for `v` as
`A_prev`'s end and as `A`'s start (`:712`, `:718`). So the "complementary tests" claim (`:466-476`)
holds bit for bit, and a vertex cannot be the end of one segment and not the start of the next. The
plane tests are exact `>` and `<` against `+-t` (`:673-682`, `:713-722`), the lune test exact `>= 0`,
`<= 0` (`:411-421`). Positions, indices and angles are what the consumer splits with; angles are tolerant
only through R5's caveat 1.

### R11. Completeness and pruning

**Met** (reading, section 3.4; hunt). Every pair of segments whose expanded bounding circles intersect
reaches `intersect_segments`; a pair that can touch within `t` is never pruned, with a margin of `t` less
the cosine resolution. The hunt's polygons have at most 8 segments (one leaf), so the internal-node
recursion is not exercised by it; the GoogleTests of section 6.1 (test 11) should cover it with larger
geometries.

### R12. Symmetry

**Met** (exact, hunt). The specification evaluated exactly with the roles swapped, on 597 generated
polyline pairs (3 ambiguous skipped): identical record sets with types mirrored, positions equal except
SS (geometry 1's vertex in each order, within `t`). The C++ matches the specification in every mode of the
hunt. Residual asymmetries, all by design: the SS position (and D6); the chord interpolation tries
segment 1 first, which is unreachable; and the traversal order of the bounding trees, which only affects
`unordered_intersections`' order.

## 3. The specific items

### 3.1 The thick-plane decisions against `GreatCircleArc::intersect`

`GreatCircleArc::intersect` (`GreatCircleArc.cc:859-1049`) decides with four exact sign tests (`>= 0`
on `.dval()`, `:879-905`), so an exact crossing at any angle between the planes is a crossing. For the
position, when the planes are within about `1e-6` of parallel (`cross(axes).magSqrd() > 0` false,
`:913`), it interpolates along the better-conditioned arc, checks the result against both lunes, and
otherwise takes the closest end point (`:936-995`). It needs all of that because its crossings can be at
angles down to `1e-12`, where the interpolated point's position *along* the arcs is uncertain by
`1e-16 / sin(phi)`.

`GeometryIntersect` cannot reach that regime. A crossing requires each end point of segment 1 beyond
`+-t` from segment 2's plane (`:761-850`). The height of a point of segment 1's great circle above
segment 2's plane is `sin(u) sin(phi)` for `u` the distance from the circles' node, at most `sin(phi)`.
So `sin(phi) > t = 1.0000444e-6` for every reported crossing, and `sin(phi)^2 > 1.000089e-12 > EPSILON`,
so the test at `:214` is true and the cross-product branch is always taken (exact, `constants.py`). The
cross product of two unit normals at angle `phi > 1e-6` has components of size `sin(phi)` with absolute
rounding about `2e-16`, so its direction, and the crossing position, is accurate to about `2e-10` rad.
The interpolation branches (`:233-396`) are reachable only through rounding, and only when an arc within
about `2e-6` of a half circle (axis error up to `1.4e-10`) meets another at an angle within `4.4e-11` of
`t`. If reached, the interpolation along segment 1 is still correct to about `1e-10` (its denominator is
at least `2t`, `:263-268`), and the point is within segment 2's lune up to that error because the
exact crossing is inside both arcs.

So: the plan's expectation that the better-conditioned-arc choice and the lune check must be ported is
**refuted**. Neither improves `GeometryIntersect`. What should be done instead is to state the invariant
`sin(phi) > t` in `add_segments_crossing_intersection` and simplify the fallback (fix F4).

Should the two be unified? **No.** They answer different questions: distance needs an exact crossing
decision (a crossing is a crossing, at any angle) and a best-effort position; partitioning needs a
tolerant decision with touch types, and gets a guaranteed well-conditioned position for free. Routing
the distance code through thick planes would make arcs that pass within `t` of each other "intersect"
at distance zero, a change to `minimum_distance` for every C++ caller. Keep `GreatCircleArc::intersect`
for distance. The one thing worth sharing is the chord-interpolation helper
(`interpolate_plane_crossing`, `GreatCircleArc.cc:332-355`, duplicated in spirit at
`GeometryIntersect.cc:256-285` and `PointInPolygon.cc:239-272`).

### 3.2 The tunnelling argument (`GeometryIntersect.cc:479-535`)

**Sound** (reading). The claim is that testing coincident vertices before thick planes prevents a point
`x` from being coincident with `A`'s start vertex `v` yet not with `A_prev`'s end vertex `v`. Section R10
shows the two tests are the same `double`, so they cannot disagree; and the crossing test is excluded
whenever any end points coincide (`:750-753`), so a coincident pair can never also produce a crossing
with a slightly different position. The "complementary tests" claim is true: `>= T_COS` for coincident
and, implicitly through `!`, `< T_COS` for not.

What the argument does not cover, and need not: the point-to-vertex test (a cosine) and the
point-to-plane test (a sine) are different functions of `x`, so a point at distance within rounding of
`t` from both `v` and a plane can pass one and fail the other. The consequences are bounded: a start
point that is within `t` of `v` is coincident (SS); one that is not, but is within `t` of `A`'s plane and
in `A`'s lune, is an LS; one that is not in `A`'s lune is behind `A`'s start and so, if its foot lies
within `t` of `v`, within rounding of being in `A_prev`'s lune. The only way to get no record while an
exact crossing exists is for `x` to fall within about `1e-16` of three thresholds at once. That is a
property of any mixed sine/cosine threshold scheme, not a defect to fix. The comment could say so.

One more consequence of the ordering, not stated in the comment: it is also what makes the corner wedge
of D3 produce two records rather than one, since only the coincidence disc, not the thick planes'
intersection, is treated as "the vertex".

### 3.3 End-point suppression, the fictitious segment, ring wrap-around, global indices

Verified (reading, probe, hunt):

- An end point is never a record; the next segment's start is (`:852-928`). Every overlap probe shows
  each vertex inside the stretch exactly once.
- A polyline's last vertex is the start of segment `number_of_segments` (`:584-641`, `:1021-1091`).
  `probe_last_vertex.py`: a contact only at the last vertex is detected, on an edge and on a vertex, with
  the same threshold as at the first vertex. `PolylineIntersections::create_partitioned_polyline`
  (`PolylineIntersections.cc:119-135`, `:161-187`) and `ResolvedSubSegmentRangeInSection`
  (`ResolvedSubSegmentRangeInSection.cc:208-230`) both handle the index `number_of_segments`.
- Polygons pass `POLYGON_NEEDS_NO_LAST_SEGMENT_INDEX` (`:58`, `:1305`); the ring's last segment ends at
  its first vertex, which is segment 0's start. The hunt's `--ring` mode agrees with the exact reference
  (which models rings as wrapping segment lists) in 900 cases.
- Segment indices are global across rings (R5). The `Graph` records no ring boundaries; the consumer
  splits the sorted list by `PolygonOnSphere::number_of_segments_in_exterior_ring()` and the interior
  rings' sizes.

### 3.4 Bounding-tree pruning soundness (R11)

**Sound** (reading). A leaf's circle is built from the arcs' *maximum* distance to the leaf centroid
(`BoundingSmallCircleBuilder::add(const GreatCircleArc &)`, `SmallCircleBounds.h:509-517`, through
`maximum_distance(point, arc)`, `GreatCircleArc.cc:1325-1365`, which covers arc interiors), then expanded
by `get_default_angular_expansion()` = the thickness cosine (`SmallCircleBounds.cc:443-453`;
`get_bounding_small_circle`, `SmallCircleBounds.h:607-613`). Internal nodes take the optimal circle
enclosing both children's expanded circles (`PolyGreatCircleArcBoundingTree.h:655-682`,
`create_optimal_bounding_small_circle`). Two nodes are tested unless their circles fail
`angle_between_centres < extent1 + extent2` (`SmallCircleBounds.h:285-293`, exact on cosines).

If a point `p` of an arc in node 1 is within `t` of a point `q` of an arc in node 2, then
`d(c1, c2) <= r1 + t + r2 < (r1 + t) + (r2 + t)`, with margin `t`. The rounding in `r1`, `r2` (through
cosines, about `1.5e-8` rad at worst) and in the angle sum is far below `t`. A crossing reported by the
thick test is an exact crossing (its sign conditions imply the exact ones), so the arcs share a point
and the circles overlap outright. Hence no pair that can produce a record is pruned. The expansion is
derived from `GeometryIntersect`'s cosine, so it follows any change of thickness (Q1).

### 3.5 Near-antipodal arcs (R8)

Covered in R8: the axis of any constructible arc is accurate to about `1.4e-10` rad; the thick-plane
classification and the lune test are unaffected; the position error measured `0` at `gap = 1.5e-6`.
The only genuinely degenerate quantity is the chord midpoint of a near-half-circle arc, which only the
unreachable interpolation branches use.

### 3.6 Cascade feedback (R9)

Covered in R9. One more observation (reasoning): a piece's rounded crossing vertex is about `1e-16` from
the edge it was computed on, well inside the thick plane, so it is always a touch on re-feeding, never a
Hobby-style spurious crossing; Hobby's problem is confined to the `(t, s)` band of D2 (a vertex
*deliberately* that far off an edge, as a model's near-coincident boundary) and to pieces fed to a
*different* polygon whose boundary differs by more than `t`.

### 3.7 Symmetry (R12)

Covered in R12.

### 3.8 `PointInPolygon.cc`'s arc-plane intersection

`if_plane_divides_gca_get_intersection_of_gca_and_plane` (`PointInPolygon.cc:172-273`) interpolates an
edge's crossing of the crossing arc's plane by signed-distance ratios, guards the near-half-circle case
with the same `1e-6` chord-midpoint test (`<`, complementary to `GeometryIntersect`'s `>` at `:259`) and
splits the arc, and falls back to the arc's midpoint when the denominator is zero as a `real_t`
(`:242-261`), which `GeometryIntersect` never needs because its denominators are at least `2t`. Its sign
tests are exact, with no thickness: parity is exact and the on-outline test is separate (section 3.5 of
the design doc). So the two are consistent in method and differ by design in tolerance, which is exactly
R7: a `P` vertex within `(t, s)` of `Q`'s outline is "on" for point-in-polygon and nothing for
`GeometryIntersect`. No other inconsistency found.

### 3.9 Dead code

Confirmed unused by search of all of `src/` (declarations, definitions and `CMakeLists.txt` entries
only):

- `arcs_are_near_each_other` (`GreatCircleArc.h:515`, `.cc:832-855`);
- `arcs_are_directed_equivalent`, `arcs_are_undirected_equivalent` (`GreatCircleArc.h:753`, `:763`;
  `.cc:1398-1432`), and `arcs_lie_on_same_great_circle` (`.cc:1369-1394`), whose only caller is the
  latter;
- `SmallCircle::intersection` (`SmallCircle.cc:40`), and also `GreatCircle::intersection`
  (`GreatCircle.h:109`, `.cc:42`);
- `GeometryCrossing` (`GeometryCrossing.{h,cc}`, listed twice in `src/maths/CMakeLists.txt` at `:40-41`
  and `:162-163`, GPlates-only). `find_vertex_crossing` returns `boost::none` unconditionally
  (`GeometryCrossing.cc:73-79`). Nothing calls `find_crossings`.

`points_are_coincident` is live (`MultiPointOnSphere.cc:313`).

### 3.10 What a self-intersection mode would need (requirements section 3.7)

By reading, so that the fixes below do not make it harder:

1. Never call `intersect_segments(seg, seg)`: a segment's start coincides with itself, so it would make
   an SS. Pairs `(i, j)` with `i < j` only.
2. Adjacent segments need no special skip. For `(seg_i, seg_i+1)` the shared vertex is a coincident
   end/start pair, so the crossing is excluded (`:750-753`) and the LS test for `seg_i+1`'s start is
   blocked (`:973`); the SL test for `seg_i`'s start on `seg_i+1` passes only if `seg_i+1` doubles back
   over `seg_i`, which is a genuine self-overlap and should be reported. Across a zero-length segment,
   `seg_i`'s end is within the zero-length bound of `seg_i+2`'s start, hence coincident (this needs
   `t >= zero-length bound`, which both Q1 options keep).
3. Traverse one bounding tree against itself: visit each unordered pair of nodes once, including a node
   with itself (child pairs `(0,0)`, `(0,1)`, `(1,1)`); at a leaf with itself, `i < j`.
4. The `Graph`'s two orderings become one ordering in which each record appears twice, once per segment
   index. Either keep `segment_index1`/`segment_index2` as two indices into the one geometry and let the
   consumer merge, or add a third ordering. The post-pass of fix F3, if adopted, must then look at both
   indices of a record when finding adjacent duplicates.

None of F1 to F4 touches these points; F3 should be written over `(segment index, is-start)` pairs rather
than over "geometry 1" and "geometry 2" specifically.

## 4. The open questions assigned to this audit

### 4.1 Q1: keep the thickness at `1.0000444e-6`, or raise it to `s = 1.4141979e-6`

What changes if the thickness becomes `s` (reading, exact):

- **One constant.** `Intersection::get_on_segment_start_threshold_cosine()` (`:1400-1411`) returns
  `1 - EPSILON` (the value of `PointInPolygon::get_point_on_polygon_threshold_cosine()`) instead of
  `GreatCircleArc::get_zero_length_threshold_cosine()`. Everything else derives from it:
  `THICKNESS_THRESHOLD_SINE` (`:1421`), the bounding-circle expansion
  (`BoundingSmallCircleBuilder::get_default_angular_expansion`, `SmallCircleBounds.cc:443-453`), and
  topology resolving's on-start/on-end tests (`ResolvedSubSegmentRangeInSection.cc:634-660`).
- **The zero-length bound stays `1e-6`** (it is `GreatCircleArc`'s, about having a rotation axis). The
  tunnelling premise `t >= zero-length bound` holds with a margin of `0.41e-6` instead of `4.4e-11`.
- **The chord branch stays unreachable**: `s^2 = 2.0e-12 > EPSILON`.
- **Coincidence becomes `dot >= 1 - 1e-12`**, which is exactly `real_t` equality of unit vectors, the
  same notion as `PointOnSphere::operator==` (`PointOnSphere.h:500-505`) and `points_are_coincident`.
  There is then one meaning of "the same point" in `src/maths`.
- **The corner wedge of D3 stays, scaled by `sqrt(2)`.** (Corrected 2026-10-28: an earlier version said
  the right-angle wedge disappears. That compared a coincidence radius of `s` with a plane thickness of
  `t`, but `get_on_segment_start_threshold_cosine()` sets both, so they rise together. The exact
  reference at `s` gives three records for `a` from `0.72 s` to `0.99 s` at a right angle.) Removing
  the right-angle wedge would need a coincidence radius of `sqrt(2)` times the thickness, two
  constants instead of one, and acute corners would keep it anyway; F3 is the fix at every angle.
- **The R7 gap closes**: no record within `s` means no vertex of `P` within `s` of `Q`'s outline, so
  `PolygonPartitioner`'s threshold-off query for a non-intersecting geometry agrees with the inclusive
  query, and the overlay's "rings with no intersection" step is consistent with `is_point_in_polygon` as
  users see it. pyGPlates already reports a distance below `s` as `0.0` (design doc section 1), so
  "touching" would mean the same in `distance` and in partitioning.
- **Costs.** A crossing needs end points beyond `s`, so a segment shorter than `2s = 2.83e-6` (18 m)
  crossing an edge is two touches, not one crossing (today `2t`, 12.7 m); the contraction tolerance of
  the overlay must be at least `2s`. The misordering band of D1 widens to `2.0e-6` until F2 lands (F2
  should land first). Bounding circles grow by `0.41e-6`, harmless. The expansion is also the default
  for `InnerOuterBoundingSmallCircleBuilder` (`SmallCircleBounds.h:1380-1401`), which point-in-polygon's
  no-set-up path widens further by `s` (design doc section 1); wider is safe there.
- **Topology resolving changes, slightly.** Two sections passing within `(t, s)` of each other without
  a record today would get one (a T-junction or a touch), and a crossing at a vertex within `(t, s)` of
  the other section's plane becomes a touch placed at that vertex, moving the intersection point by up
  to `s` (about 9 m). The design doc (section 3.5) measured that about 6% of shared plate vertices
  coincide only within epsilon; those within `(t, s)` today produce D3's clusters and the
  backward-compatible multiple-intersection path (`TopologyIntersections.cc:396-430`, `:555-660`); with
  `s` they become single SS records. So the PR A verification "topology-resolving output is unchanged on
  a real model" must be replaced by "every changed intersection point moves by less than `s`, and the
  number of multiple-intersection sections does not rise". This is a deliberate correction, and needs a
  changelog entry in both products.

What changes if the thickness stays `t`: nothing in the code; the R7 gap and the `4.4e-11` margin stay,
and must be documented as decisions.

**Recommendation: raise it to `s`.** The system then has one tolerance for "the same point", stated once
(`EPSILON` seen through a cosine), the partitioner's exact classification agrees with the inclusive
query by construction. (D3 is not a reason either way; see above.) The costs are a measurable, bounded
change to topology resolving and a `2s` contraction tolerance in Phase 4. If the developer prefers to
keep `t` for PR A's "unchanged output" guarantee, D2 and D3 become documented limits and the contraction
tolerance is `2t`, and the choice should be revisited before PR B, since the overlay's consistency
argument (requirements section 3.3, "Agreement with `is_point_in_polygon` by construction") is only
exact for the `s` choice.

### 4.2 Q9: derive crossings and chains in the partitioner, or extend the `Graph`

Under the overlay (chosen), **R3 is not needed**: pieces are classified by a point-in-polygon test of
their midpoint and common chains by the sector rule; no record is ever labelled crossing or bouncing.
**R4 is a derivation**: after splitting both geometries at the records, a piece of `P` between
consecutive records `u`, `v` is common when `u` and `v` are consecutive along `Q` too and neither piece
has an original vertex between them (requirements section 3.1 step 3). That needs the sorted orders,
the segment indices and the types (`is_on_segment1_start()` etc.), all of which the `Graph` has. Cost:
one pass over the records, `O(k)`, in the partitioner.

Extending the `Graph` would mean defining chain semantics inside `GeometryIntersect`: a `ChainStart` /
`ChainEnd` flag or a seven-type enumeration after Foster-Hormann-Popa, decided with the thickness (a
vertex is "on" the other geometry up to `t`), plus the direction. `GeometryCrossing` is what that
looked like when attempted. It would also change a contract that topology resolving relies on, for no
caller but the partitioner.

**Recommendation: derive in the partitioner; leave the `Graph` as it is.** Two small additions to
`GeometryIntersect` are worth making because every consumer rediscovers them: a documented helper giving
the ring index and ring-relative segment index of a record (or a note on the `Graph` saying how to split
the orders by ring), and the corrected ordering key (F2), without which the derivation of consecutive
records is wrong in the misordering band.

## 5. Proposed fixes, ranked

The order is by importance for partitioning. "Risk" is to topology resolving, the only other caller.

**F1. Decide the thickness (Q1), and make the decision visible.** If `s`: in
`Intersection::get_on_segment_start_threshold_cosine()` (`GeometryIntersect.cc:1400-1411`) return
`1.0 - GPlatesMaths::EPSILON` (the same value as `PointInPolygon::get_point_on_polygon_threshold_cosine`;
reference it, do not include `PointInPolygon.h`), and rewrite the comment: the thickness is the distance
that compares equal to zero, it must not be less than `GreatCircleArc`'s zero-length bound (say why), and
`t^2 > EPSILON` keeps the cross product well conditioned. Update `get_on_segment_start_threshold_sine`'s
comment (`:1417-1420`). If `t` is kept: rewrite the same comments to say the gap to `s` is accepted and
why. Risk: section 4.1; measure on a real model by diffing resolved topologies, expecting moves below
`s` only. Test: section 6.1, tests 3, 12, 16.

**F2. Order touch records by their projection onto the segment (D1).** In `add_intersection`
(`:144-188`), for SL compute `angle_in_segment2` from the vertex's projection onto segment 2's great
circle, and for LS likewise on segment 1: with `n` the segment's plane and `v` the vertex,
`proj = v - dot(v, n) n`, `angle = acos(dot(proj, start) / |proj|)` (or `atan2` on the tangent component
for full resolution). That needs the planes passed in, which `intersect_segments` already has
(`:670`, `:710`). Then the key of a touch is its position along the segment, the same quantity a
crossing's key approximates, and two records sort by where they are. Risk: none to positions (topology
resolving computes its interpolation ratio from the position, `ResolvedSubSegmentRangeInSection.cc:759`);
ordering changes only for records within `sqrt(2) t` of each other on one segment, which today sort
wrongly. Test: section 6.1, test 7.

**F3. Report a corner contact once (D3).** Two options. (a) A post-pass in `intersect_geometries`
(`:1243-1270`), after sorting: for two consecutive records along one geometry that are both touches by
the same vertex of the other geometry (same other-side segment index, both on that segment's start),
on consecutive segments of this geometry (indices differing by one, or the ring's last and first), drop
the second. `O(k)`, local, no geometry. (b) Accept the duplicates and let Phase 4's contraction merge
them, since they have equal keys along one geometry and bracket the corner along the other by at most
`t / sin(theta / 2)` each side, which exceeds any fixed contraction tolerance for sharp corners. Prefer
(a). Risk: topology resolving today sees such a vertex as two or three intersections and takes its
backward-compatible path, which uses the first along geometry 1; (a) keeps that record, so the chosen
intersection is unchanged (the two have equal keys along that geometry, so "first" is only defined
once F7 makes the sort stable). (a) does not touch the third record of the right-angle case, the SL of
`v` on `P`'s arriving segment: it comes from a different pair of segments and is a correct record in
its own right, and it lies within `sqrt(2)` thicknesses of the kept LS, so the overlay's contraction
(tolerance at least `2s`) merges the two.

**F3 (a) is wrong at an acute corner; under review (2026-10-28).** The rule has no distance test, so
it also drops one of the two LS records when `x` is far from `v`, for example at `r = 1.2 h` to
`3.8 h` on the bisector of a 30-degree corner (exact reference at `s`: LS on `Q` segments 0 and 3,
both by `P`'s vertex 1). There the two records are not duplicates. They are the two crossings that
`P` makes with the corner's edges at `r = 4.2 h` (LL on segments 0 and 3), collapsed onto `x` by the
thickness. Together they cut `Q`'s ring into a tip piece `[x, v, x]`, which holds the original vertex
`v`, and the rest of the ring. `v` is further than `h` from `P`, so its exact classification can
differ from the rest of the ring's (in the probe, `v` is north of `P` and `Q`'s far vertices are
south). Dropping a record makes the ring one piece holding both, and a test at one original vertex
then classifies all of `Q` on one side. The overlay should keep both records: the two coincide at
`x`, so they form one vertex with six ends, and the sector rule classifies the ends from their
rotational order. The tip piece is classified at `v`, and its output ring `x, v` is degenerate (two
vertices, zero area, a sliver at most `2h` wide), so it is dropped. Write it over `(segment index, is-start)` pairs so that a self-intersection
mode can reuse it (section 3.10). Test: section 6.1, test 12.

**F4. Remove the dead code and state the invariant (D5).** Delete `arcs_are_near_each_other`,
`arcs_are_directed_equivalent`, `arcs_are_undirected_equivalent`, `arcs_lie_on_same_great_circle`,
`SmallCircle::intersection`, `GreatCircle::intersection`, and `GeometryCrossing.{h,cc}` with its two
`CMakeLists.txt` entries. In `add_segments_crossing_intersection` (`:195-397`) replace the three
interpolation branches (`:233-396`) by one call to a shared `interpolate_plane_crossing` (move it from
`GreatCircleArc.cc:332-355` to a header both can use; `PointInPolygon.cc:239-272` could use it too),
tried on segment 1 then segment 2, with a debug assertion if both fail, and a comment stating that
`sin(phi) > t` for every crossing so the cross product is well conditioned and this path is a rounding
fallback. Risk: none (unused code; the fallback is unreachable). Test: section 6.1, test 16 pins the
constants the invariant depends on.

**F5. Document the record positions (D6).** In `GeometryIntersect.h:72-75` say the SS position is the
first geometry's vertex except at the second geometry's last vertex, where it is that vertex; or make
`:616` use `segment1_start_point` so the statement becomes true. The latter changes a position by at
most `t` for topology resolving at a section's last vertex; prefer the comment unless Phase 4 wants the
rule uniform.

**F6. Give consumers the ring structure.** Either a note on `Graph` saying the orders must be split by
the polygon's ring segment ranges, or a helper `ring_index_and_segment(polygon, segment_index)` on
`PolygonOnSphere` if there is none. No risk.

**F7. Use `std::stable_sort` in `sort_geometry_intersections`** (`:117-137`) so that equal keys keep
discovery order. Cosmetic; deterministic either way.

Not proposed: porting `GreatCircleArc::intersect`'s refinements (section 3.1), unifying the two
primitives (section 3.1), or extending the `Graph` (section 4.2).

## 6. Tests

### 6.1 Proposed GoogleTest cases for `GeometryIntersect` (none exist today)

Each case lists the geometries, then the expected `Graph`: record count, types, segment indices, angles
(zero where "on start"), positions (exact for vertex records, within `1e-9` rad for crossings) and both
orders. Build them with `PolylineOnSphere::create` / `PolygonOnSphere::create` from `UnitVector3D`s
constructed in local tangent coordinates at a random centre (as `arc_ref.py`'s `offset` does), so that
nothing is axis-aligned by accident. Thresholds below mean `t` as returned by
`Intersection::get_on_segment_start_threshold_sine()`, so the tests follow the Q1 decision.

1. **Proper crossing.** Two two-point polylines crossing at a right angle: one LL, indices `(0, 0)`,
   position the normalised cross product, angles the arc distances from each start. Then the same pair
   swapped: mirrored, same position.
2. **Shared vertex, bit-identical.** `P` passes through a vertex of `Q` as its own vertex: one SS, angles
   zero, position `Q`'s vertex. Variants: at `P`'s first vertex (index 0), at `P`'s last vertex (index
   `number_of_segments`), both geometries' last vertices (both fictitious indices); `P` a polygon ring
   with the shared vertex its first vertex (index 0, no fictitious index).
3. **Vertex on an edge.** A `P` vertex `0.99 t` off the interior of a `Q` edge: one LS (polyline `P` as
   geometry 2), position `P`'s vertex, angle along `Q` set by F2. At `1.01 t`: no record and `intersect`
   returns false. The same at `P`'s last vertex (fictitious index) and first vertex.
4. **Overlaps.** The header's four diagrams (`GeometryIntersect.h:291-322`) with the stated records;
   interleaved vertices (R4 table row (a)), reversed (d), a zero-length segment in the stretch (c), the
   whole edge (e), round the corner (f). Assert that every vertex in the stretch is a record and that
   nothing else is.
5. **Zero-length segments.** A `0.5 t` segment of `P` straddling a `Q` edge's plane: two touches, no LL.
   A `P` segment of about `t` at a `Q` vertex: two SS. A zero-length `Q` edge against a crossing `P`
   segment: no LL from it; the records come from its end points.
6. **Short crossing segment.** A `1.5 t` segment crossing an edge: two touches `1.5 t` apart and no LL
   (pins D4 for the Phase 4 contraction tolerance). A `3 t` segment: one LL at the middle.
7. **Ordering within a segment.** The D1 case: a `Q` vertex `3.0e-6` along and `0.99 t` off `P`'s first
   segment and a `Q` edge crossing it at `3.15e-6`: the orders along `P` are vertex then crossing.
   Before F2 this test fails; it is the regression test for F2. Also two records in one segment well
   apart, and records on segments `i` and `i + 1`.
8. **Rings.** `Q` with an interior ring; records on the exterior ring's last and first segments, on the
   interior ring's segments (global indices equal `number_of_segments_in_exterior_ring()` plus the
   ring-relative index), and a hole touching the exterior at a shared vertex of both rings against a
   `P` vertex there: two records, one per ring. `include_polygon_interior_rings = false` drops the
   interior records.
9. **Near-antipodal edge.** A `Q` triangle with an edge `pi - 1.6e-6` long crossed by a short `P` arc:
   one LL within `1e-9` of the cross product of the great circles; a `P` vertex at the long edge's end:
   one SS.
10. **Near-parallel crossing.** A long `Q` edge (`1` rad) and a `P` segment crossing it at `2e-6` rad:
    one LL within `1e-9` of the exact crossing (compute it with `long double` or by the exact method of
    `arc_ref.py`). At `0.5e-6` rad with the `P` end points within `t` of the plane: touches only.
11. **Pruning.** A `Q` polygon with 200 segments and two interior rings of 50, against a `P` polyline of
    100 segments that touches `Q` once, far from any vertex, at exactly `0.99 t` off an edge: the record
    is found (the leaf circles overlap only through the expansion). Also the four recursion branches
    (both internal with geometry 1 larger, with geometry 2 larger, internal against leaf, leaf against
    internal) by shaping the two geometries' sizes; assert the record sets equal a brute-force
    `intersect_segments` over all pairs (expose it in a test-only header, or compare against the
    `PolylineIntersections` cut).
12. **Corner wedge.** Written in units of the thickness `h` (`s` after F1), so that the cases stay in
    the wedge whichever thickness is chosen. `Q`'s right-angle corner `v` and a `P` vertex `x` at
    `(-a, +a)` from it, with `P` arriving from `(-0.01, 0.003)` rad as in `probe_corner.py`, for `a` in
    `0.5 h`, `0.72 h`, `0.99 h`; a 30-degree corner with `x` on the bisector at `1.2`, `2`, `3.8` and
    `4.2` times `h`. Expected after F3: `0.5 h` one SS; `0.72 h` and `0.99 h` two records, the SL of
    `v` on `P`'s arriving segment and one LS of `x` (the pair on consecutive edges reduced to one); the
    bisector cases inside the wedge (up to `h / sin(15 deg) = 3.86 h`) one LS; `4.2 h` two LL. Before
    F3 this documents D3. (Corrected 2026-10-28: the cases were in units of `t`, which F1 would have
    moved inside the coincidence disc, and the right-angle expectation omitted the SL record.)
13. **Symmetry.** For every geometry pair above, `intersect(A, B)` and `intersect(B, A)` have the same
    record count, mirrored types, equal positions except SS within `t`, and the two orders swapped.
14. **No intersection and clearing.** Disjoint geometries: returns false, `Graph` empty; a `Graph` with
    stale contents is cleared on entry (`:1251`).
15. **Twist on an edge.** `P` a bow tie whose crossing lies on a `Q` edge: two LL at one position with
    equal keys along `Q`, adjacent in `Q`'s order; and `Q` a bow tie against a straight `P`.
16. **Constants.** `get_on_segment_start_threshold_cosine() <= GreatCircleArc::get_zero_length_threshold_cosine()`
    (the thickness is at least the zero-length bound: the tunnelling premise), and
    `get_on_segment_start_threshold_sine()^2 > EPSILON` (the cross product is always taken). These pin
    the two coincidences the code relies on against future changes of either constant.

Follow the `cpp-test` skill for layout; no data files are needed.

### 6.2 The exact reference and the hunt (draft delivered)

`arc_ref.py` evaluates `intersect_segments`'s specification in rational arithmetic (`fractions.Fraction`,
exact for `double` inputs): coincidence `dot >= T_COS`, the thick-plane sides `h^2` against
`T_SINE^2 |n|^2` with the sign of `h`, the exact lune signs, zero length `|a x b|^2 <= EPSILON`, the
four record types, the fictitious segment of a polyline, polygon rings as wrapping segment lists, LL
positions as the exact cross product, and the ordering keys as exact `cos^2` of the along-angle (it
reproduces the C++'s chord key for touches, so it does not flag D1; a corrected key is a one-line
change there). Every tolerant decision is checked against a band around its threshold (absolute
`1e-15` on dot products and determinants, relative `4e-9` on squared heights and zero-length tests);
a case that depends on a decision inside a band raises `Ambiguous` and is skipped and counted. It also
has exact geometric predicates (`proper_crossing`, `vertex_on_edge_exact`) for reasoning about what
records mean.

Generators (`GENERATORS`): `shared_vertex` (a path through or within an offset of a `Q` vertex, at a
random angle, sometimes with a `P` vertex there, sometimes bit-identical), `collinear` (a run along a `Q`
edge, offset by one of twelve values either side across `t` and `s`, interleaved and reversed, sometimes
with a zero-length segment or a `Q` vertex inside), `near_parallel` (long edges crossing at `1e-7` to
`1e-3` rad), `near_antipodal` (edges within `1e-2` to `1.6e-6` of a half circle, crossed or touched at
an end), `cluster` (short segments across an edge, a twist of `P` on an edge, two touches close
together), `random`. `hunt_arcs.py` runs them against pyGPlates, observing the records as the piece
boundaries of `PolygonOnSphere.partition` (`probe_common.records_along`: chain the pieces, count repeated
points), with `--ring` putting `P` through the polygon-against-polygon path. Records at `P`'s first or
last vertex positioned exactly there are invisible to the observation and are excluded from the
comparison (they are checked indirectly: `intersect` returns `intersecting`).

Results: seeds 1, 2 and 4 (900 polyline cases each) and seed 3 (900 ring cases): **0 mismatches**, about
6 ambiguous cases per 150 `shared_vertex` cases skipped. A deliberately wrong threshold in the reference
produces 27 mismatches in 300 cases, so the hunt is sensitive. Each run takes about one second.

Still to add for the full hunt (PLAN.md Phase 3): a `refeed` generator (cut a polygon by `Q`, feed each
piece to `Q` and to jittered copies, as `probe_cluster2.py` (i) does by hand); interior rings, including
a hole touching the exterior; shared edges taken from a real topological model (bit-identical and
within-epsilon vertex pairs); tiny geometries (scale `1e-5`) at high latitude; edges longer than 90
degrees; and, once the C++ exposes `GeometryIntersect` to tests, direct comparison of the `Graph` instead
of the partition observation, which would make end-point records visible and remove the merged-piece
heuristics.

## 7. Corrections to the design doc and the requirements file (not applied)

Design doc `spherical-geometry.md`:

1. Section 4.2, step 1 and the header `GeometryIntersect.h:72-75`: "placed at the first geometry's
   vertex" is false at the second geometry's last vertex (`GeometryIntersect.cc:616`): there it is that
   vertex (D6).
2. Section 4.2, "Where the arcs cross": the three interpolation branches are unreachable (section 3.1).
   Say that every reported crossing has planes at least `t` apart, so the cross product is always used
   and accurate to about `2e-10` rad, and that the interpolation is a rounding fallback.
3. Section 4.3: "`GeometryIntersect` does neither the lune check nor the fallback" is true but should
   not read as a gap; add that it does not need them (section 3.1).
4. Section 1, the `GeometryIntersect` thickness row: give the computed value `1.0000444e-6`, not
   "about `1e-6`", since the margin over the zero-length bound (`4.4e-11`) is what the tunnelling
   premise rests on; and note that `t^2 > EPSILON` is what keeps the cross product branch live.
5. Section 4.2, "The test for one pair of arcs", step 4: add that a vertex in the inner wedge of a corner
   is reported by both edges (D3).

Requirements file `phase2-requirements.md`:

1. Section 2, the closing list "Requirements met today": R2 and R5 are met with caveats (D3, D1), R8's
   near-antipodal gap and R9's near-collinear gap are refuted.
2. R2 "Today": add the corner wedge (D3); the "complementary tests" claim is confirmed.
3. R5 "Today": the caveat that matters is the chord key (D1), not the cosine resolution; the resolution
   only produces ties.
4. R8 "Today": replace "no refinement handles that" with: the axis of a constructible arc is accurate to
   about `1.4e-10` rad, which is enough; the crossing position error measured zero at `pi - 1.5e-6`.
5. Section 3.3, "Against it", fourth bullet, and section 4 question 4: "two touches within `1e-6` of each
   other, as a `1.5e-6` segment crossing an edge produces" is wrong: the touches are `1.5e-6` apart, and
   a segment shorter than `2t` gives touches up to `2t` apart (D4). The contraction tolerance must be at
   least twice the thickness (`2t`, or `2s` under Q1's recommended answer), and the Phase 4 design should
   also contract by key equality (two records with equal `(segment, angle)` on one geometry).
6. R9 "Today": "the known gap against `GreatCircleArc::intersect`, which is the first thing the audit
   should port" is refuted (section 3.1); nothing needs porting.
7. Section 1.3's case table, V-intersection row: "the position is `P1`" has the D6 exception.
8. R7 "Today": the thickness is `1.0000444e-6`; the measured gap is `(1.0000444e-6, 1.4141979e-6)`.

`PLAN.md`, Context, "GeometryIntersect lacks its last two refinements": it does not need them
(section 3.1); and Phase 3's "Decide and record which primitive partitioning uses": `GeometryIntersect`,
with F1 to F3; `GreatCircleArc::intersect` stays separate for distance.
