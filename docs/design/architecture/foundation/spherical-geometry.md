# Distance and point-in-polygon on the sphere

The design behind two pieces of `src/maths/` (the foundation area): the minimum distance between
geometries (`GeometryDistance.{h,cc}`, with the point-to-arc and arc-to-arc distances and the arc
intersection in `GreatCircleArc.{h,cc}`, and the cosine-valued `AngularDistance` and
`AngularExtent`), and point-in-polygon (`PointInPolygon.{h,cc}`, dispatched by
`PolygonOnSphere::is_point_in_polygon`). pyGPlates exposes both through
`src/api/PyGeometriesOnSphere.cc`: `GeometryOnSphere.distance()` (the wrapper
`geometry_on_sphere_distance`) and `PolygonOnSphere.is_point_in_polygon()`.

They are one design because their tolerances are one system. Point-in-polygon's point-on-outline
threshold has the cosine `1 - 1e-12`, which is the distance code's epsilon zero (a distance with
that cosine compares equal to `AngularDistance::ZERO`, and is what the pyGPlates wrapper clamps to
`0.0`); and point-in-polygon decides whether a point is on the outline by calling
`minimum_distance()` against each edge with that threshold. In the other direction, the distance
code decides whether a geometry is inside a solid polygon with point-in-polygon.

Notation: `R` is the reference point from which crossings are counted (section 3.1), `C` is the
polygon's boundary centroid (`R = -C`), `s` is the point-on-outline threshold, and "epsilon" is
`GPlatesMaths::EPSILON`.

## 1. Tolerances

The one table of every tolerance in this code. "Exact" means a plain `double` comparison (through
`.dval()`, or `is_precisely_less_than` / `is_precisely_greater_than` on `AngularDistance`);
"epsilon" means a `real_t` comparison, which treats values within `EPSILON` as equal.

| Tolerance | Where | What it decides | Why that value |
| --- | --- | --- | --- |
| `EPSILON = 1e-12` | `MathsUtils.h`; every `real_t` comparison (`<`, `<=`, `==` and the operators Boost derives from them), and so `AngularDistance::operator<` and `AngularExtent::operator<`, which compare cosines as `real_t` | Equality of any two `real_t` values. On a cosine (the dot product of two unit vectors) `1e-12` below 1 is about `1.414e-6` radians, so two points that close compare as coincident | The comment at its definition calls it a guess (2004) that allows for a small accumulation of rounding error. Everything below is either this number seen through a cosine, or derived from it |
| Zero-length arc, about `1e-6` radians | `GreatCircleArc::is_zero_length`: the cross product of the end points has `magSqrd() <= 0` as a `real_t`, so its square is at most `1e-12` and the sine of the arc's angle at most `1e-6`. `get_zero_length_threshold_cosine()` is the same bound as a cosine, `cos(asin(sqrt(EPSILON)))` | Whether an arc has a rotation axis (`rotation_axis()` throws without one). A zero-length arc is measured to the closer of its two end points (`minimum_distance(point, arc)`), which can still be about `1e-6` radians apart; it intersects another arc only by its start point lying on it (`intersect()`); the arc-to-arc distance does not call `intersect()` for one; in a crossing count it stands in for its own intersection with the crossing plane (section 3.2); and the on-outline scan cannot pre-reject it by its plane (section 3.3) | The square of the cross product's magnitude is compared with `EPSILON` |
| Point-on-outline threshold `s`: cosine `1 - 1e-12`, about `1.414e-6` radians (about 9 metres) | `PointInPolygon::get_point_on_polygon_threshold_cosine()` and `get_point_on_polygon_threshold_sine()` (its sine, `sin(acos(1 - 1e-12))`) | A point within `s` of any edge or vertex, of any ring, is on the outline and so inside (section 3.3), at every speed | The same number as `EPSILON` on a cosine: a distance of `s` is one that compares equal to zero. It is not the same as `GeometryIntersect`'s *thick* planes: `GeometryIntersect::Intersection::get_on_segment_start_threshold_cosine()` returns the zero-length cosine, about `1e-6` radians |
| Bounds widening, `s` plus the zero-length threshold (about `2.4e-6` radians) | `PointInPolygon::get_bounds_widening()`: the inner bounding circle is contracted and the outer expanded by it, for the whole polygon and for each lune-tree leaf. The no-set-up path through `PolygonOnSphere::is_point_in_polygon` adds `s`, per call, to the polygon's cached outer bounding circle, which `InnerOuterBoundingSmallCircleBuilder` has already expanded by its default, the zero-length threshold | When a point is far enough inside the inner circle or outside the outer one to be decided without testing edges (section 3.5) | A bounds decision skips the on-outline test, so the bounds must be wider than `s`, or a point within `s` of the outline would be outside at one speed and on the outline at another. The extra `1e-6` covers the bounds comparing cosines, which resolve angles near zero only to about `1.5e-8` radians (the comment at `get_bounds_widening` says this) |
| Near-parallel edges, `1 - 1e-4` | `MAX_DOT_PRODUCT_CROSSING_ARC_AND_POLYGON_EDGE` in `PointInPolygon.cc`: the dot product of the crossing arc's and the edge's plane normals (planes within about `0.014` radians of each other) | Which method decides whether an edge crosses the crossing arc: the sign tests of section 3.2, or interpolating the edge's crossing of the crossing plane and testing that point against the arc | A method choice, not an answer tolerance: the comment says the actual value is unimportant, it only funnels the common case into the cheaper test |
| Lune widening `1e-4` | `SphericalLuneTree::TreeBuilder::SPHERICAL_WEDGE_PLANE_EPSILON`, a sine (about `1e-4` radians, `0.34` arc minutes) | Which edges belong to a lune: those with an end point in the lune widened by it, those crossing its first bounding half circle, and those coming within it of either bounding half circle | Protection against rounding when assigning edges, and much larger than `s`, which is what makes a leaf's edges enough for the on-outline test (section 3.5) |
| Half-circle guard, chord midpoint `magSqrd() < 1e-6` (magnitude `1e-3`) | `interpolate_plane_crossing` (`GreatCircleArc.cc`) and `if_plane_divides_gca_get_intersection_of_gca_and_plane` (`PointInPolygon.cc`) | Whether an arc is too close to a half circle to interpolate along its chord. The distance code then tries the other arc, and failing that uses the end point closest to the other arc; point-in-polygon splits the arc at its midpoint and recurses | Both halves of an exactly antipodal pair are the same distance from every plane, so the ratio of signed distances means nothing |
| Cosine resolution, about `1.5e-8` radians | Not a constant: the largest `double` below 1 is `1 - 2^-53`, whose arc cosine is about `1.5e-8`. Every distance here is made from a cosine (`AngularDistance::create_from_cosine`), including inside an arc's lune, where it is `sqrt(1 - sine^2)` | The smallest non-zero distance. In C++ anything closer than this is exactly `AngularDistance::ZERO` | A property of `double`, stated in the wrapper's comment and at `get_bounds_widening` |
| Cosine clamp | `clamp_cosine_rounding_error` (`AngularDistance.h`), applied by `AngularDistance::create_from_cosine` and `AngularExtent::create_from_cosine` | A cosine outside `[-1, 1]` by no more than epsilon is clamped to the end it represents; anything further out is left for `acos` to report | The dot product of a point and its exact antipode can round to just below `-1`, which exact comparisons would then see as exceeding a threshold of exactly pi (section 2) |
| pyGPlates zero clamp | `geometry_on_sphere_distance`: `angular_distance != AngularDistance::ZERO` is an epsilon comparison, so a cosine within `1e-12` of 1 (a distance up to about `1.414e-6` radians) is returned as `0.0` | What Python sees as zero | Deliberate and user-visible since pyGPlates 1.0: Python code tests `== 0.0`, and geometries that touch (share a vertex, say) can come out a tiny non-zero distance when rounding hides the contact |
| pyGPlates threshold margin, `2 * EPSILON` on the cosine | `geometry_on_sphere_distance`: `minimum_distance()` is given the cosine of the caller's threshold less `2e-12`, and the caller's threshold is then applied exactly, in radians, to the distance returned | Whether `distance()` returns `None` | `cos(acos(c))` differs from `c` by a few ULPs, so a distance passed back as the threshold was excluded by itself about a quarter of the time; and a distance clamped to `0.0` must not exceed a threshold of `0.0`. The margin covers both. A NaN threshold raises `ValueError`; the threshold is clamped to `[0, pi]` |
| `intersect()` | `GreatCircleArc.cc` | Whether two arcs cross: four signed distances compared with zero exactly (`>= 0` on `.dval()`). An epsilon enters only once the arcs are known to cross and the caller wants the point: `cross(axis1, axis2).magSqrd() > 0` (a `real_t`, so planes within about `1e-6` radians count as the same great circle) chooses between the normalised cross product and interpolation along one arc's chord (see section 2) | The sign tests are exact so that a crossing is a crossing; only *where* near-collinear arcs cross needs care, since the cross product of nearly parallel normals is inaccurate |
| Lune membership in `minimum_distance(point, arc)` | The two dot products are compared with zero as `real_t` (epsilon) | Whether the point is inside the arc's lune (closest to the arc's interior) or to an end point | Deliberately epsilon, so that a point at the arc's rotation axis, where both dot products are zero, goes to the end-point branch rather than normalising a zero vector |
| Degenerate crossing arc | `PointInPolygon::is_crossings_arc_degenerate`: `cross(R, P).magSqrd() <= 0` as a `real_t` | Whether the test point `P` is within about `1e-6` radians of `R` or of `C`, where the crossing arc's plane is undefined (section 3.6) | The same bound as a zero-length arc |

## 2. Distance

`GeometryDistance.h` declares `minimum_distance` for every pairing of the four geometry types,
with a solid flag per polygon, an optional threshold (an `AngularExtent`), and optional closest
positions and closest indices. The distance is a cosine-valued `AngularDistance`, so that most of
the work is dot products rather than `acos`.

**A threshold exceeded is reported out of band.** Every overload returns
`boost::optional<AngularDistance>`, none only when a threshold was given and exceeded; without a
threshold the result is never none. The older design signalled "exceeded" by returning pi, and
started every search at pi, recording a candidate only if less: geometries exactly pi apart
(antipodal points, or degenerate geometries on them) then counted as exceeding a threshold of
pi, and with no threshold at all nothing was recorded, so the closest indices came back
uninitialised. Candidates at exactly pi are now ordinary results, and the clamp in the table above
keeps a rounded cosine just below `-1` from exceeding a threshold of exactly pi.

**Thresholds are exact.** A candidate is rejected only if its own distance is precisely greater
than the threshold (`is_precisely_greater_than`), so a distance passed back as the threshold is
returned unchanged; `GeometryDistanceTest.point_to_arc_threshold_equal_to_distance` and
`geometry_threshold_equal_to_distance` check this contract. An earlier quick rejection inside the
arc's lune, comparing the sine of the distance with the sine of the threshold, was dropped
because it was not conservative: near zero many sines share one cosine, so a point `1e-12`
radians off an arc was rejected by a threshold equal to its own distance. Every remaining early
rejection is conservative: the bounding small circles are expanded by the zero-length threshold,
far above the rounding of their arithmetic.

**Intersection is exact.** `is_intersection()` (`GeometryDistance.cc`) is true only for a distance
of exactly zero. It stops a bounding-tree search (a closer candidate cannot follow) and decides
whether a solid polygon's interior still needs testing. A distance within epsilon of zero is not
an intersection: the geometries can be close without touching, with one inside the other. The
polyline-to-polygon and polygon-to-polygon distances once skipped the interior test for any outline
distance within epsilon of zero, so a geometry inside a solid polygon and about `1e-7` radians from
its outline got that small distance instead of zero
(`GeometryDistanceTest.inside_solid_polygon_near_its_outline`).

**Ties keep the first candidate** (`is_closer_candidate`, and `is_precisely_less_than` in the
arc-to-arc distance). Each candidate's closest position is held locally and copied to the caller's
output only when it is recorded, because a candidate equal to the minimum so far is not recorded
and writing it straight through paired one candidate's position with another's index.

**Solid interiors.** A solid polygon's interior is defined like point-in-polygon: anything inside
is at distance zero. For a polyline or a polygon, the outline distance is found first and the
interior is tested only if the outlines did not intersect, in which case each ring (or the
polyline) is wholly inside or wholly outside, so one arbitrary point of it is tested: its first
vertex (the polyline's start point). The point-in-polygon call turns the point-on-outline threshold
**off**, for every geometry type including a lone point. The outline distance decides whether the
geometries touch; since on-outline became a distance pass over the edges (section 3.3), the
threshold would only repeat it. In C++ the consequence is that a point up to about `1.4e-6`
radians outside a solid polygon gets its true distance rather than exactly `ZERO`, as a polyline
or polygon that close already did. pyGPlates does not see this because of the wrapper's zero clamp.
The GPlates clients that do are in co-registration: the region-of-interest filter
(`RegionOfInterestFilter.h`) passes both geometries as solid with the region as the threshold, so
with a region of 0 km such a point is no longer selected; and the distance attribute
(`DataMiningUtils::shortest_distance`) passes both as solid with no threshold, so such a point's
distance is no longer 0.

**Closest positions are on the outline.** When a geometry is inside a solid polygon the threshold
is not exceeded, and the closest positions and indices, if asked for, are found by a second search
with no threshold and refer to the polygon's outline (its exterior or interior rings), not to the
interior.

**Where near-collinear arcs cross.** When `intersect()` finds that two arcs cross and their planes
are the same great circle within epsilon, it does not use the cross product of the normals, which
is inaccurate for nearly parallel planes. It interpolates along the chord of the better-conditioned
arc (the one whose end points differ most in signed distance from the other's plane) by those
signed distances, as `GeometryIntersect::intersect_segments()` does: the point is on both great
circles to within rounding, however small the angle. Its position *along* them is less certain,
and these sign tests are exact (unlike `GeometryIntersect`'s thick planes), so the point must also
lie within both arcs' lunes; otherwise, or if neither arc can be interpolated (both near half
circles), the end point of either arc closest to the other arc is used.

**The pyGPlates wrapper** (`geometry_on_sphere_distance`) is the only place a distance is turned
into radians. It applies the threshold margin, the exact radians threshold and the zero clamp from
the table, in that order: the margin so that C++ does not reject a distance the caller's threshold
admits, the exact test on the distance the caller will see, and the clamp last. Its distance does
not depend on whether closest positions were requested: inside an arc's lune the distance comes
from the sine whether or not the closest point is calculated.

## 3. Point-in-polygon

### 3.1 Definition

A point `P` is inside a polygon if the great-circle arc from `R` to `P` crosses an odd number of
edges, counted over the exterior ring and every interior ring, where `R` is the antipode of the
exterior ring's boundary centroid `C` (`PolygonOnSphere::get_boundary_centroid`, the
arc-length-weighted mean of the exterior ring's edge midpoints, from
`Centroid::calculate_outline_centroid`), and `R` is assumed to be outside. Separately, a point
within `s` of the outline is inside (section 3.3). The rings' winding orders play no part.

Parity, rather than winding, is the definition because the globe's filled polygons are drawn that
way and the two must agree. `GLFilledPolygonsGlobeView::FilledDrawables::add_filled_polygon` draws
each ring as a triangle fan from `C` (`add_polygon_ring_mesh_to_current_filled_drawable`) into the
stencil buffer with `GL_INVERT`, so a pixel is filled when an odd number of fan triangles cover it.
A fan triangle `(C, v_i, v_i+1)` covers `P` exactly when the arc from `P` away from `C`, towards
`-C`, crosses the edge `v_i v_i+1`, so the number of triangles covering `P` is the number of edges
crossed by the arc from `R = -C` to `P`: coverage parity is crossing parity. Reconstructed-raster
meshes keep a triangle only if point-in-polygon accepts its centroid (`PolygonMesh.cc`), and the
area and orientation of a polygon are also fan sums from `C` (`SphericalArea.cc`,
`PolygonOrientation.cc`), so all of these answer consistently for one polygon.

What parity means for awkward polygons: an interior ring that crosses the exterior ring, or a
self-intersecting ring, is defined by parity (a point outside the exterior ring but inside an
interior ring is inside), and reversing a ring changes nothing. Shapefile and GeoJSON define
interiors by winding (exterior one way, holes the other); GPlates does not, and never forces
interior rings to the opposite orientation.

The assumption that `R` is outside fails for a ring large enough, or C-shaped enough, to contain
the antipode of its own centroid; the `FIXME` at `get_polygon_centroid` records it, and every
consumer above is then wrong together. When the weighted sum degenerates (a ring near a great
circle), `C` falls back to the first vertex of the exterior ring, so `R` is that vertex's antipode.
Choosing `R` by a rule for each ring's interior is deferred (section 5).

### 3.2 Strict parity

`get_num_polygon_edges_crossed` counts, over a sequence of edges, those that cross the arc from
`R` to `P`, with no tolerance in the counting (the two arcs either cross or they do not), so that
a vertex on the crossing arc counts once if the arc passes from outside to inside there and twice
if it passes through and back. The side of the crossing arc's plane each vertex is on is an exact
sign (`< 0` on the dot product), computed once per vertex and carried from one edge to the next,
and an edge is a candidate only if its two end points are on different sides. The comment at
`get_num_polygon_edges_crossed` says why the signs must be compared as booleans and not by the
sign of their product.

For a candidate edge whose plane is not nearly parallel to the crossing arc's, the arcs cross if,
and only if (the comment in the code numbers these):

1. the two end points of the crossing arc, `R` and `P`, are on opposite sides of the edge's plane,
   and
2. the start points of the two arcs are on different sides of each other's plane: if the edge
   starts on the negative side of the crossing plane, `R` must be on the positive side of the
   edge's plane (and `P` on the negative), and the mirror image otherwise.

The candidate test and (1) say that each arc's great circle separates the other arc's end points,
so each arc crosses the other's great circle once. Two great circles meet at two antipodal points,
and (2) picks out the case where both arcs pass through the same one of them: without it an edge
and a crossing arc on opposite sides of the globe, each dividing the other's end points, would
count as a crossing. The tests in (1) and (2) are inclusive (`>= 0`, `<= 0`), so a crossing arc
that ends exactly on an edge's plane counts as crossing it; a vertex exactly on the crossing plane
is handled by the boolean sides, which put it on the non-negative side consistently for both of
its edges.

Nearly parallel planes (the `1e-4` branch of the table) make these signs unreliable, so instead
the edge's crossing of the crossing plane is interpolated by signed distances
(`if_plane_divides_gca_get_intersection_of_gca_and_plane`) and tested for lying on the crossing
arc (`does_polygon_edge_intersection_with_crossing_gc_lie_on_crossing_gca`: closer to `R` than `P`
is, and on the half circle starting at `R`). A zero-length candidate edge has no plane, and its end
point stands in for the crossing.

### 3.3 The on-outline test

If the crossing count is even and the threshold is on, `is_point_on_polygon_outline` scans every
edge of every ring and returns true if `minimum_distance(P, edge, s)` finds a distance (one not
exceeding `s`). The distance to an edge includes its end points, so vertices need no separate
test. One dot product rejects most edges first: a point within `s` of an edge is within `s` of the
edge's great circle, so an edge whose plane is further from `P` than the threshold's sine (plus
`1e-9`, more than the precision with which the distance, compared as a cosine, resolves the sine
near `s`) is skipped; a zero-length edge has no plane and goes straight to the
distance. The on-outline test is independent of parity, and runs only when parity says outside.

The threshold is inclusive, and on by default, for two reasons found in the code and the data:

- The comment at `get_point_on_polygon_threshold_cosine`: a point exactly on the dateline must be
  included by a polygon with an edge exactly aligned with the dateline.
- Adjacent plates in topological models share boundaries whose vertices are not all bit-identical.
  Measured on the Müller 2016 and 2019 and Merdith 2021 models, about 94% of the vertices shared by
  adjacent plates are bit-identical and the rest coincide only within epsilon, so with exact
  semantics a point on a shared boundary could fall into neither plate. With the threshold every
  point on a shared boundary is in both plates, and the callers that assign a point to a plate
  take the first polygon tested. `PlateBoundaryStats.cc` relies on this by name: it tests a point
  on a shared sub-segment against the plates that share it, relying on the outline being included,
  and when it has to test every plate instead it first moves the point ten thresholds to one side
  (`10 * get_point_on_polygon_threshold_sine()`) so as not to land in the other plate.

The threshold is turned off where a point near a polygon but outside it must stay outside:
`PolygonPartitioner::is_non_intersecting_polyline_or_polygon_fully_inside_partitioning_polygon`
(the polyline intersections have already guaranteed no intersection within the threshold), the
solid-interior tests of the distance code (section 2), and
`GLReconstructedStaticPolygonMeshes.cc`.

### 3.4 Rejected alternatives

**Deciding on-outline by relaxing the pairing test against the edge's plane.** This was the
previous implementation: with the threshold on, condition (1) of section 3.2 compared
`dot(edge_normal, P)` with the threshold's sine in place of zero, so that a point within the
threshold of an edge's *plane* counted as a crossing, and as on the outline if within it on both
sides. The crossing arc then no longer had to reach
the edge's plane, so the pairing tests no longer implied an intersection; the comment at
`get_num_polygon_edges_crossed` summarises the three consequences:

- a point near an edge's great circle but beyond the edge was inside, up to `s` divided by the
  sine of the angle between the edge and the crossing arc, capped at about `1e-4` radians (640
  metres) by the near-parallel branch, in ordinary polygons and at every speed;
- for a tiny polygon, whose edges' planes all pass near `R`, a point near `R` was inside without
  set-up (the no-set-up method has no bounds to hide it);
- a point at a vertex, or near an edge almost parallel to the crossing arc, was outside, so that
  5-10% of the vertices shared by adjacent plates were in one plate only, and about one vertex per
  snapshot in neither.

**Exact, semi-open edges** (each edge owning one end, as planar scan conversion does) would make
shared boundaries exclusive instead of inclusive. Rejected because the shared vertices are not all
bit-identical (above), so the two plates' edges are not the same edges, and points on the boundary
could be in neither.

### 3.5 The three speeds

`PolygonOnSphere::is_point_in_polygon` runs the same definition at three speeds
(`PointInPolygonSpeedAndMemory`), which must, and do, give the same answer:

- **Low** (no set-up): the free function `PointInPolygon::is_point_in_polygon`, O(N) over every
  edge of every ring, and the on-outline scan over all of them. If the polygon's bounding small
  circle happens to be cached, `PolygonOnSphere::is_point_in_polygon` first rejects a point outside
  it, widened by `s`.
- **Medium** (bounds): a `PointInPolygon::Polygon` built with `build_ologn_hint = false`, holding
  the inner and outer bounding small circles of all edges about `R`, and whether `C` is inside. A
  point inside the inner circle is outside the polygon (no edge lies between it and `R`); a point
  outside the outer circle has `C`'s status (no edge lies between it and `C`, but `C` itself can be
  inside or outside); otherwise all edges are tested as at low speed.
- **High** (bounds plus the lune tree): `build_ologn_hint = true`, taken only for more than 16
  segments (`TreeBuilder::AVERAGE_NUM_EDGES_PER_LEAF_NODE`). The sphere is split into four
  equal-area lunes about the axis through `C` and `R`, each split in two recursively while it holds
  more than 16 edges and the depth allows. Every crossing arc from `R` lies in one lune, so a leaf
  holds the edges that can cross such arcs (and some more, near its boundary), as runs of
  consecutive edges (`EdgeSequence`), with its own inner and outer bounds. A query descends to its
  leaf, tries the leaf's bounds, and then counts crossings over the leaf's edges only. A leaf with
  no edges is outside (the polygon does not reach that lune, as a horseshoe may not).

The bounds and the leaves are what make the speeds a risk to agreement, and two things keep
them agreeing:

- The bounds are widened by more than `s` (the table): a bounds decision skips the on-outline
  test, so a point within `s` of the outline must never be decided by the bounds. (They were once
  widened by about `1e-6` radians, less than `s`, and a point `1e-6` to `1.4e-6` radians from the
  outline got different answers at different speeds.)
- A leaf's edges are enough for the on-outline test, which needs every edge within `s` of a point
  in the lune, not only the edges that enter it. An edge can come that close without entering: round
  the lune's apex (`C` or `R`, where it can cross a bounding great circle on its other half), or
  running nearly parallel to a bounding half circle with both end points outside. So an edge is
  assigned to a lune if it enters the lune widened by `1e-4` (an end point in it, or crossing its
  first bounding half circle), or comes within `1e-4` of either bounding half circle
  (`minimum_distance()` to its two quarter circles, made only for the few edges whose end points do
  not both lie beyond the widening on one side, since an arc not crossing a great circle is nowhere
  closer to it than at its nearer end point). `1e-4` is far more than `s`, so every edge within `s`
  of a point in the lune is in the leaf, and a leaf with no edges also means the point is not on the
  outline. (Without the half circle test, a self-intersecting polygon whose crossing edges pass
  through its centroid put points just over `1e-6` radians from the centroid, on the outline,
  outside at this speed only.)

`PointInPolygonTest.speeds_agree` and `point_on_outline_at_every_speed` call each speed explicitly,
with the threshold on and off, and the Python suite's `_is_point_in_polygon_at_each_speed` forces
each speed through fresh copies of a polygon.

### 3.6 Degenerate crossing arcs

When `P` is within about `1e-6` radians of `R` or of `C` (`is_crossings_arc_degenerate`), the
crossing arc is too short or too nearly antipodal for its plane to be defined. The arbitrary plane
chosen then (`get_crossings_arc_plane_normal`) contains `P` but not, in general, `R`, so the test of
which side of each edge's plane `R` is on means nothing. Before this was handled, a bow-tie at
`R`, or a tiny polygon or a sliver at the antipode of one of its vertices, could put such a point
inside, at low speed only (the bounds hid it at the other speeds).

Such a point now takes the status of the nearby reference point
(`is_point_in_polygon_with_degenerate_crossings_arc`): `R`'s, which is outside, or `C`'s, which the
bounds cache and which is otherwise computed on the arc from `R` to `-R`. That arc is exactly
antipodal, so the arbitrary plane through `-R` contains `R` as well, and any arc joining exactly
antipodal points gives the same parity. With the threshold on this is exact: `1e-6` radians is less
than `s`, so if `P` is further than `s` from the outline no edge can pass between it and the
reference point, and if not, it is on the outline and inside. Without the threshold the answer can
be wrong only when `P` is within about `1e-6` radians of the outline as well, where an edge can pass
between it and the point whose status it inherits; the `FIXME` there records this. It matters mostly
near `C`, since a bow-tie's or a sliver's centroid can lie on its outline (`R` is near the outline
only if the outline passes near the antipode of its own centroid, as for a degenerate centroid, or a
ring about 90 degrees from its centroid). Everywhere else the answer without the threshold is exact
to rounding. The fix is a second reference point about 90 degrees from `R`, with its status cached,
from which such a point's crossing arc is well defined (section 5).

### 3.7 The adaptive policy

`ADAPTIVE`, the default, moves a polygon through the speeds as calls accumulate
(`PolygonOnSphere::is_point_in_polygon`): the bounds are built on the eighth call
(`NUM_CALLS_BEFORE_MEDIUM_SPEED = 8`), and the lune tree once 20 calls at medium speed have had to
test edges (`NUM_MEDIUM_SPEED_CALLS_TESTING_EDGES_BEFORE_HIGH_SPEED = 20`, counted by
`PointInPolygon::Polygon::get_num_queries_testing_edges`), or on the thousandth call regardless
(`NUM_CALLS_BEFORE_HIGH_SPEED = 1000`). A speed can be raised by a caller but never lowered.

The rule is to pay a set-up once the calls it would have made cheaper have cost about as much as
it does. The measurements behind the numbers (2026, one machine, MSVC 2022; a temporary timing
harness in the unit-test binary, not committed; polygons of 16 to 4096 edges and 0.2 radians
radius, with points uniform over the globe, near the polygon, and near its outline) are summarised
at the top of `PointInPolygon.h` and in the comment at the policy. At `N = 1024`, in microseconds
per query for uniform / near / outline points:

| Speed | Set-up | Query |
| --- | --- | --- |
| Low | the centroid, once, about 25 | 5.6 / 4.6 / 3.4 |
| Medium | 84 | 0.03 / 0.69 / 3.3 |
| High | 150 | 0.004 / 0.03 / 0.14 |

Medium's set-up costs about 15 low-speed queries at every size (both are O(N)), so the old 4 calls
was too early; 8, earlier than that break-even, gave the best worst case (below).
High's extra set-up costs about 20 medium-speed queries that have to test
edges, but medium's bounds decide points away from the outline almost for free, so no fixed count
suits both: the tree pays off after about 20 calls near the outline but only after about 2000 far
from it. Counting the queries that tested edges is what lets one rule serve both.

Emulated against six other policies over 180 cases (2 to 20,000 calls), as a ratio to the best of
the seven in each case:

| Policy | Worst | Geometric mean |
| --- | --- | --- |
| 4 / 200 (the old fixed counts) | 3.2 | 1.3 |
| 16 / 20 edge-testing calls, no cap | 2.0 | 1.21 |
| **8 / 20 edge-testing calls / 1000 calls (chosen)** | **1.75** | **1.13** |
| 8 / 20 / 500 | 2.6 | 1.13 |
| 4 / 20 / 1000 | 2.1 | 1.10 |
| 8 / 40 / 1000 | 1.74 | 1.15 |
| 8 / 10 / 1000 | 1.9 | 1.12 |

Against the old policy, points near the outline are up to 2.6 times faster and five calls about
1.7 times faster; uniform points are up to 1.3 times slower over 20 to 5000 calls. Rejected: the
old fixed 4 and 200 (200 was up to 4.7 times slower than a fixed choice of speed); and an uncapped
count of edge-testing calls, under which points far from the polygon waited thousands of calls for
the tree, 1.6-1.8 times slower than 4 / 200.

### 3.8 Thread safety

The polygon's `CachedCalculations` (the centroid, the bounds, the point-in-polygon tester and the
call count) is created and filled lazily by `const` methods without synchronisation, and so is the
tester's count of edge-testing queries; the comments at both say so. One polygon must therefore not
be queried from two threads at once. This is acceptable because pyGPlates holds the Python GIL
during these calls, and in GPlates only a script running in its Python thread, sharing a polygon
with the GUI thread, could do so.

## 4. Testing

The Python suite (`pygplates/test/test_maths/test_geometries_on_sphere.py`) carries references
independent of the code under test. `_brute_force_point_in_polygon` applies the definition of
section 3.1 with exact orientation predicates (`_orient`: a `float` filter with a fixed error
bound, falling back to `fractions.Fraction`, which is exact for doubles) and S2's four-orientation
crossing rule, reaching the point from `R` through a random waypoint so that neither arc is nearly
antipodal or nearly zero length, and rerouting if any orientation is exactly zero; the vertices are
perturbed by about `1e-12` so that no orientation involving a vertex is zero. It returns `None`
for a point in a band around the outline (`1.3e-6` to `1.5e-6` radians), where either answer is
accepted but every speed must agree; within `1.3e-6` the answer must be inside (without the
threshold, the band extends to the outline).
`test_point_in_polygon_against_brute_force` runs it over tiny, ordinary, hemisphere-sized,
holed, sliver and bow-tie polygons and the points listed in its comment, at every speed.
`_brute_force_distance` (every pair of segments, with its own point-to-arc distance) uses the same
point-in-polygon, threshold off, to decide solid interiors; `test_distance_against_brute_force` and
`test_distance_threshold_is_exact` check the wrapper's contract.

The C++ tests (`src/unit-test/PointInPolygonTest.cc`, `GeometryDistanceTest.cc`) call each speed
explicitly, with the threshold on and off, and check the C++ contracts that the wrapper hides: a
threshold equal to the distance (for a point and an arc, and for every pairing of geometry types,
solid or not, half of them near-antipodal), a threshold of pi for exact antipodes and for
geometries exactly pi apart, and zero distance inside a solid polygon near its outline. The
zero-length last segment and the wrapper's zero clamp and exact radians threshold are checked
from Python (`test_distance_to_zero_length_segment`, `test_distance_threshold_is_exact`).

The regression cases near `R` and `C` (`near_antipodal_centroid_and_centroid`,
`test_point_in_polygon_near_antipodal_centroid`: a bow-tie at `R`, a tiny polygon and a sliver at
a vertex's antipode) came from a randomised hunt at a far larger scale than the suite, because once
strict parity was in, the suite's random cases no longer reached them. Likewise the case of edges
passing round a lune's apex (`edges_through_centroid`,
`test_point_in_polygon_near_centroid_of_crossing_edges`: a self-intersecting polygon whose crossing
edges pass through its centroid) came from a review, and depends on the polygon's orientation (the
tree's lunes are placed by it).

## 5. Deferred work

- **An interior rule per ring.** Each ring's interior would be its smaller side (area under
  `2 pi`), the rings combining by parity as now; only the choice of `R` changes, and every consumer
  of `C` (point-in-polygon, area, orientation, the stencil fan) would take the one outside point
  from one accessor. For a polygon whose vertices all lie in the open hemisphere about `C` the
  result is `R = -C` as today, bit for bit. A ring larger than a hemisphere would then mean its
  complement, which needs an explicit-side override supplied by the caller.
- **A second reference point** about 90 degrees from `R`, with its status cached, for the crossing
  arcs that are degenerate from `R` (section 3.6), removing the `FIXME` at
  `is_point_in_polygon_with_degenerate_crossings_arc`.
- **Polygon partitioning.** `GeometryIntersect`, `PolylineIntersections` and `PolygonPartitioner`
  use the same tolerances and will extend this document.
