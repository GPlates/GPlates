# Distance, point-in-polygon, intersection and partitioning on the sphere

The design behind two pieces of `src/maths/` (the foundation area): the minimum distance between
geometries (`GeometryDistance.{h,cc}`, with the point-to-arc and arc-to-arc distances and the arc
intersection in `GreatCircleArc.{h,cc}`, and the cosine-valued `AngularDistance` and
`AngularExtent`), and point-in-polygon (`PointInPolygon.{h,cc}`, dispatched by
`PolygonOnSphere::is_point_in_polygon`). With point-in-polygon comes the definition of a polygon's
interior: which side of each ring is inside, and the one outside point from which every consumer of
the interior works (`PointInPolygon::InteriorReference`, `PolygonOnSphere::get_outside_point`). Its
consumers are the area and orientation (`SphericalArea.{h,cc}`, `PolygonOrientation.cc`), the
interior centroid (`Centroid.{h,cc}`), the triangle fans that fill polygons (`PolygonFan.cc`,
`src/opengl/GLFilledPolygonsGlobeView.cc`, `GLRasterCoRegistration.cc`, `GLScalarField3D.cc`), the
bounding-circle shortcuts that skip a polygon (`GeometryDistance.cc`, `CubeQuadTreePartition.h`,
`SmallCircleBounds.cc`, `src/app-logic/TopologyReconstruct.cc`) and a network's projection
(`src/app-logic/ResolvedTriangulationNetwork.h`). pyGPlates exposes the distance and
point-in-polygon through `src/api/PyGeometriesOnSphere.cc`: `GeometryOnSphere.distance()` (the
wrapper `geometry_on_sphere_distance`) and `PolygonOnSphere.is_point_in_polygon()`, and the
interior through `get_area()`, `get_signed_area()`, `get_orientation()` and
`get_interior_centroid()`.

Two more pieces build on these: the intersection of polylines and polygons
(`GeometryIntersect.{h,cc}`, section 4), and partitioning a geometry by a polygon into the pieces
inside and outside it (`PolylineIntersections.{h,cc}` and `PolygonPartitioner.{h,cc}`, and the
plate partitioner `src/app-logic/GeometryCookieCutter.{h,cc}`, section 5). pyGPlates exposes
partitioning through `PolygonOnSphere.partition()` and `PlatePartitioner`, and GPlates through
Assign Plate IDs (and, for points, the velocity layers).

They are one design because their tolerances are one system. Point-in-polygon's point-on-outline
threshold has the cosine `1 - 1e-12`, which is the distance code's epsilon zero (a distance with
that cosine compares equal to `AngularDistance::ZERO`, and is what the pyGPlates wrapper clamps to
`0.0`); and point-in-polygon decides whether a point is on the outline by calling
`minimum_distance()` against each edge with that threshold. In the other direction, the distance
code decides whether a geometry is inside a solid polygon with point-in-polygon, and the outside
point is chosen among candidates by their distance from the outline.

Notation: `R` is the polygon's outside point, from which crossings are counted (section 3.1), `C`
is the polygon's boundary centroid (`R = -C` for an ordinary polygon, section 3.2), `R2` is the
second reference point (section 3.8), `s` is the point-on-outline threshold, and "epsilon" is
`GPlatesMaths::EPSILON`.

## 1. Tolerances

The one table of every tolerance in this code. "Exact" means a plain `double` comparison (through
`.dval()`, or `is_precisely_less_than` / `is_precisely_greater_than` on `AngularDistance`);
"epsilon" means a `real_t` comparison, which treats values within `EPSILON` as equal.

| Tolerance | Where | What it decides | Why that value |
| --- | --- | --- | --- |
| `EPSILON = 1e-12` | `MathsUtils.h`; every `real_t` comparison (`<`, `<=`, `==` and the operators Boost derives from them), and so `AngularDistance::operator<` and `AngularExtent::operator<`, which compare cosines as `real_t` | Equality of any two `real_t` values. On a cosine (the dot product of two unit vectors) `1e-12` below 1 is about `1.414e-6` radians, so two points that close compare as coincident | The comment at its definition calls it a guess (2004) that allows for a small accumulation of rounding error. Everything below is either this number seen through a cosine, or derived from it |
| Zero-length arc, about `1e-6` radians | `GreatCircleArc::is_zero_length`: the cross product of the end points has `magSqrd() <= 0` as a `real_t`, so its square is at most `1e-12` and the sine of the arc's angle at most `1e-6`. `get_zero_length_threshold_cosine()` is the same bound as a cosine, `cos(asin(sqrt(EPSILON)))` | Whether an arc has a rotation axis (`rotation_axis()` throws without one). A zero-length arc is measured to the closer of its two end points (`minimum_distance(point, arc)`), which can still be about `1e-6` radians apart; it intersects another arc only by its start point lying on it (`intersect()`); the arc-to-arc distance does not call `intersect()` for one; in a crossing count it stands in for its own intersection with the crossing plane (section 3.4); the on-outline scan cannot pre-reject it by its plane (section 3.5); a spherical triangle with such a side has zero area (section 7); and the exact area moment skips it (section 3.3) | The square of the cross product's magnitude is compared with `EPSILON` |
| Point-on-outline threshold `s`: cosine `1 - 1e-12`, about `1.414e-6` radians (about 9 metres) | `PointInPolygon::get_point_on_polygon_threshold_cosine()` and `get_point_on_polygon_threshold_sine()` (its sine, `sin(acos(1 - 1e-12))`) | A point within `s` of any edge or vertex, of any ring, is on the outline and so inside (section 3.5), at every speed | The same number as `EPSILON` on a cosine: a distance of `s` is one that compares equal to zero. It is not the same as `GeometryIntersect`'s *thick* planes: `GeometryIntersect::Intersection::get_on_segment_start_threshold_cosine()` returns the zero-length cosine, about `1e-6` radians |
| Bounds widening, `s` plus the zero-length threshold (about `2.4e-6` radians) | `PointInPolygon::get_bounds_widening()`: the inner bounding circle is contracted and the outer expanded by it, for the whole polygon and for each lune-tree leaf. The no-set-up path through `PolygonOnSphere::is_point_in_polygon` adds `s`, per call, to the polygon's cached outer bounding circle, which `InnerOuterBoundingSmallCircleBuilder` has already expanded by its default, the zero-length threshold | When a point is far enough inside the inner circle or outside the outer one to be decided without testing edges (section 3.7) | A bounds decision skips the on-outline test, so the bounds must be wider than `s`, or a point within `s` of the outline would be outside at one speed and on the outline at another. The extra `1e-6` covers the bounds comparing cosines, which resolve angles near zero only to about `1.5e-8` radians (the comment at `get_bounds_widening` says this) |
| Near-parallel edges, `1 - 1e-4` | `MAX_DOT_PRODUCT_CROSSING_ARC_AND_POLYGON_EDGE` in `PointInPolygon.cc`: the dot product of the crossing arc's and the edge's plane normals (planes within about `0.014` radians of each other) | Which method decides whether an edge crosses the crossing arc: the sign tests of section 3.4, or interpolating the edge's crossing of the crossing plane and testing that point against the arc | A method choice, not an answer tolerance: the comment says the actual value is unimportant, it only funnels the common case into the cheaper test |
| Lune widening `1e-4` | `SphericalLuneTree::TreeBuilder::SPHERICAL_WEDGE_PLANE_EPSILON`, a sine (about `1e-4` radians, `0.34` arc minutes) | Which edges belong to a lune: those with an end point in the lune widened by it, those crossing its first bounding half circle, and those coming within it of either bounding half circle | Protection against rounding when assigning edges, and much larger than `s`, which is what makes a leaf's edges enough for the on-outline test (section 3.7) |
| Half-circle guard, chord midpoint `magSqrd() < 1e-6` (magnitude `1e-3`) | `interpolate_plane_crossing` (`GreatCircleArc.cc`), `if_plane_divides_gca_get_intersection_of_gca_and_plane` (`PointInPolygon.cc`) and `add_segments_crossing_intersection` (`GeometryIntersect.cc`, which tests `> 1e-6` before interpolating) | Whether an arc is too close to a half circle to interpolate along its chord. The distance code then tries the other arc, and failing that uses the end point closest to the other arc; point-in-polygon splits the arc at its midpoint and recurses; `GeometryIntersect` tries the other arc, and failing that splits the first arc at its midpoint (section 4.2) | Both halves of an exactly antipodal pair are the same distance from every plane, so the ratio of signed distances means nothing |
| Cosine resolution, about `1.5e-8` radians | Not a constant: the largest `double` below 1 is `1 - 2^-53`, whose arc cosine is about `1.5e-8`. Every distance here is made from a cosine (`AngularDistance::create_from_cosine`), including inside an arc's lune, where it is `sqrt(1 - sine^2)` | The smallest non-zero distance. In C++ anything closer than this is exactly `AngularDistance::ZERO` | A property of `double`, stated in the wrapper's comment and at `get_bounds_widening` |
| Cosine clamp | `clamp_cosine_rounding_error` (`AngularDistance.h`), applied by `AngularDistance::create_from_cosine` and `AngularExtent::create_from_cosine` | A cosine outside `[-1, 1]` by no more than epsilon is clamped to the end it represents; anything further out is left for `acos` to report | The dot product of a point and its exact antipode can round to just below `-1`, which exact comparisons would then see as exceeding a threshold of exactly pi (section 2) |
| pyGPlates zero clamp | `geometry_on_sphere_distance`: `angular_distance != AngularDistance::ZERO` is an epsilon comparison, so a cosine within `1e-12` of 1 (a distance up to about `1.414e-6` radians) is returned as `0.0` | What Python sees as zero | Deliberate and user-visible since pyGPlates 1.0: Python code tests `== 0.0`, and geometries that touch (share a vertex, say) can come out a tiny non-zero distance when rounding hides the contact |
| pyGPlates threshold margin, `2 * EPSILON` on the cosine | `geometry_on_sphere_distance`: `minimum_distance()` is given the cosine of the caller's threshold less `2e-12`, and the caller's threshold is then applied exactly, in radians, to the distance returned | Whether `distance()` returns `None` | `cos(acos(c))` differs from `c` by a few ULPs, so a distance passed back as the threshold was excluded by itself about a quarter of the time; and a distance clamped to `0.0` must not exceed a threshold of `0.0`. The margin covers both. A NaN threshold raises `ValueError`; the threshold is clamped to `[0, pi]` |
| `intersect()` | `GreatCircleArc.cc` | Whether two arcs cross: four signed distances compared with zero exactly (`>= 0` on `.dval()`). An epsilon enters only once the arcs are known to cross and the caller wants the point: `cross(axis1, axis2).magSqrd() > 0` (a `real_t`, so planes within about `1e-6` radians count as the same great circle) chooses between the normalised cross product and interpolation along one arc's chord (see section 2) | The sign tests are exact so that a crossing is a crossing; only *where* near-collinear arcs cross needs care, since the cross product of nearly parallel normals is inaccurate |
| `GeometryIntersect` thickness, the zero-length threshold (about `1e-6` radians) | `GeometryIntersect::Intersection::get_on_segment_start_threshold_cosine()` (the zero-length cosine) and `get_on_segment_start_threshold_sine()` (its sine), used as `THICKNESS_THRESHOLD_COSINE` and `THICKNESS_THRESHOLD_SINE` in `GeometryIntersect.cc`; also the default expansion of a bounding small circle (`BoundingSmallCircleBuilder::get_default_angular_expansion()`), which the bounding trees use | Two vertices whose dot product is at least the cosine coincide. A vertex whose signed distance from an arc's plane is within the sine (both comparisons exact) is *on* the plane, and touches the arc if it is also in the arc's lune. Two arcs cross only if each has its end points beyond the sine on opposite sides of the other's plane (section 4.2) | The longest a zero-length arc can be, so a zero-length arc never has its end points on opposite sides of a thick plane: it has both end points on any thick plane it straddles, so it can still touch that arc (the comment at `get_on_segment_start_threshold_cosine`). The bounding circles are expanded by the same amount so that the bounding tree never prunes a pair of arcs close enough to touch |
| Lune membership in `minimum_distance(point, arc)` | The two dot products are compared with zero as `real_t` (epsilon) | Whether the point is inside the arc's lune (closest to the arc's interior) or to an end point | Deliberately epsilon, so that a point at the arc's rotation axis, where both dot products are zero, goes to the end-point branch rather than normalising a zero vector |
| Degenerate crossing arc | `PointInPolygon::is_crossings_arc_degenerate`: `cross(R, P).magSqrd() <= 0` as a `real_t` | Whether the test point `P` is within about `1e-6` radians of `R` or of `-R`, where the crossing arc's plane is undefined, so that crossings are counted from `R2` instead (section 3.8) | The same bound as a zero-length arc |
| Hemisphere test, `dot(v, C) > 0` exactly | `is_ring_in_open_hemisphere` (`PointInPolygon.cc`), on every vertex of a ring | Whether a ring keeps parity from `-C` (and its fan sum from `C`), or is decided by the winding rule (section 3.2) | Strict, so that a ring that passes has `-C` more than 90 degrees from every vertex: `-C` is then outside the ring, the fan from `C` is well conditioned, and the old results are reproduced bit for bit. The 1e-12 of a `real_t` comparison would gain nothing, since a vertex that close to 90 degrees passes or fails harmlessly either way |
| Tie, `1e-8` steradians | `TIE_STERADIANS` in `PointInPolygon::get_ring_winding_offset`: the fan sum less it, divided by `4 pi`, is rounded half down (`ceil(x - 0.5)`) | A ring whose fan sum is within it of an odd multiple of `2 pi` (a ring halving the sphere, such as four points on the equator) takes the side to its left as its interior, so reversing the ring swaps its interior and exterior | A ring halving the sphere has two sides equal to rounding, so some rule must break the tie deterministically; the left side is the convention of GeoJSON and S2. The value need only be far above the rounding of a fan sum and below any difference that matters: it is under half a square kilometre on Earth |
| Candidate distance, `0.01` radians | `MAXIMUM_CANDIDATE_DISTANCE_TO_OUTLINE_COSINE = cos(1e-2)` in `PointInPolygon.cc`, against each candidate's `minimum_distance()` to the outline | A candidate for `R` closer than this to the outline is tried only if no candidate further away is outside | Two reasons (the comment at the constant): a candidate's status comes from fan sums of triangles with apex at its antipode, which are ill-conditioned for a candidate at a vertex; and the filled polygons are fans from `-R` rendered in perspective from the centre of the globe with a near plane `1e-3` from it (`GLCubeSubdivision`'s default `zNear`), and a fan triangle whose edge passes within `d` of `R` has its plane within about `d` of the centre, so it would be clipped for `d` under about `1e-3`. `R2` has no such minimum: its status comes from exact crossing parity and nothing is drawn from it, so it is simply the furthest candidate in its band |
| `R2` band, 60 to 120 degrees from `R` | `calculate_second_reference_point`: a candidate with `fabs(dot(candidate, R)) > 0.5` is skipped | Which candidates may be the second reference point | The arc from `R` to `R2`, on which `R2`'s status is found, and the arc from `R2` to a point within `1e-6` of `R` or `-R`, are then 60 to 120 degrees long, so their planes are well defined. A candidate always exists, since no point is more than about 23 degrees from one (the comment there) |
| Interior-centroid size switch, bounding radius `0.01` radians | `MINIMUM_RADIUS_COSINE_FOR_FAN = cos(0.01)` in `Centroid::calculate_interior_centroid`, against the angular extent of the polygon's bounding small circle | Whether the interior centroid is the exact area moment (wider) or the fan of triangles from `C` weighted by area at their vertices' mean (narrower) | Where the two methods' errors cross (section 3.3): the fan approximation is off by about `0.3` degrees for a polygon one radian across and the exact moment loses precision on tiny polygons; at this size they agree to about `1e-10` (the comment there, from measurement) |
| `DateLineWrapper` thick plane and pole caps, cosine `1 - 1e-9`, about `4.5e-5` radians | `EPSILON_THICK_PLANE_COSINE` and its sine `EPSILON_THICK_PLANE_SINE` in `DateLineWrapper.cc`. `classify_vertex`: a vertex within the sine of the dateline's plane is on it, and within the cosine of a pole is on the pole. `intersect_line_segment`: the south pole within the sine of a segment's plane means the segment passes a pole | Which vertices are on the dateline or at a pole, and so which segments are intersected at all (section 5.2) | The comment says the epsilon is based on a cosine because that has the least accuracy for small angles, and that `1 - 1e-9` is a displacement of about `4.5e-5`. It is about 45 times `GeometryIntersect`'s thickness and 30 times `s`, and is not derived from `EPSILON`; it serves a map display, where a vertex that close to the dateline is better snapped onto it |
| `DateLineWrapper`'s other epsilons: `1e-4`, and `real_t` zero tests | `does_line_segment_on_dateline_plane_cross_north_pole`: the dot product of a segment's normal with the dateline plane's normal, for a segment on the thick plane running between the arc and longitude zero, compared with `1e-4`; below that the sign of the end point's `z` decides. `calculate_intersection`: the chord interpolation's denominator and the interpolated vector's `magSqrd()` compared with zero as `real_t`; `intersect_line_segment`: `is_zero_length()`. `output_non_intersecting_polygon_rings`: `minimum_distance()` of two solid polygons compared with `AngularDistance::ZERO`, an epsilon comparison of cosines (`boost::equivalent` over `real_t`) | Which pole such a segment passes; no intersection for a segment whose ends are both too near the plane or nearly antipodal; a zero-length segment straddling the plane intersects at its start point; a non-intersecting ring within about `1.4e-6` radians of an output exterior ring becomes its interior ring (section 5.2) | The first is called lenient because both paths are covered; the comments say the next two cannot happen given the thick-plane precondition; the last is the epsilon in `AngularDistance::operator==`, not a choice made here |

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
geometries touch; since on-outline became a distance pass over the edges (section 3.5), the
threshold would only repeat it. In C++ the consequence is that a point up to about `1.4e-6`
radians outside a solid polygon gets its true distance rather than exactly `ZERO`, as a polyline
or polygon that close already did. pyGPlates does not see this because of the wrapper's zero clamp.
The GPlates clients that do are in co-registration: the region-of-interest filter
(`RegionOfInterestFilter.h`) passes both geometries as solid with the region as the threshold, so
with a region of 0 km such a point is no longer selected; and the distance attribute
(`DataMiningUtils::shortest_distance`) passes both as solid with no threshold, so such a point's
distance is no longer 0.

With a threshold, each of the four overloads against a polygon returns none at once when the
geometries' bounding small circles are further apart than the threshold. For a solid polygon that
is only right if the region outside its circle is outside the polygon, which a polygon that wraps
round the globe can break (section 3.3), so the exit is taken for a solid polygon only when
`PolygonOnSphere::is_outside_bounding_small_circle_outside_polygon` says so for the circle used
(the root of the polygon's bounding tree, or the polygon's own circle for a multi-point). Before
this, a point inside such a polygon but further than the threshold from its outline's circle got
none instead of zero (`PointInPolygonTest.interior_beyond_bounding_circle`).

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

A polygon's interior is the set of points inside an odd number of its rings, exterior and interior
rings alike. Each ring's interior is defined by its winding number `w(x)`, the number of times the
ring goes round `x` (positive to the ring's left; on the sphere it is defined up to a constant):
the ring's *outside* is the winding level `w*`, its mean winding number over the sphere rounded to
the nearest integer, and its interior is the points whose winding number differs from `w*` by an
odd number (the comment at `PointInPolygon::InteriorReference`). For a ring that does not cross
itself this is its **smaller side**: the winding number is 1 on one side and 0 on the other, so the
mean is under one half exactly when the side wound once has area under `2 pi`. A ring that crosses
itself only slightly (a small twist or loop, often unintentional) gets the answer parity from
outside the ring would give. The region round the ring covers most of the sphere and has one
winding level, so it is the outside. Every lobe is inside, whichever way it is wound: a twisted
lobe's winding number differs from the outside level by -1, which is odd. No separate path decides
this: it is the same rounding (section 3.2), with a large margin because the lobes are small. The
outside level is unclear only where the winding-weighted area nears `2 pi`, which is the limit
below. Two refinements:

- **A tie.** A ring whose area is within about `1e-8` steradians of half the sphere's (a ring
  halving the sphere, such as four points on the equator) rounds half down, so its interior is the
  side to its **left**, and reversing it swaps its interior and exterior. Everywhere else the rings'
  winding orders play no part.
- **The hemisphere exception, for bit-identity.** A ring within the open hemisphere about `C`
  (every vertex has a positive dot product with `C`; its edges, each shorter than a half circle,
  are then in it too) keeps the old definition: parity from `-C`. The two definitions differ only
  for a ring that winds round some region more than once, with a winding-weighted area over
  `2 pi`, and keeping the old answer keeps every ordinary polygon bit-identical to the code before
  the rule (section 3.2).

A ring's signed area (section 3.3) is the area of its interior, weighted by winding number relative
to the outside level. It is positive when the interior is to the ring's left (counter-clockwise
seen from above the sphere) and negative when it is to the right; interior rings are no different.
For a ring with a twist, the twisted lobe counts against the main one, so the sign is the larger
lobe's orientation. The sign follows from the interior and plays no part in choosing it, except at
a tie.

Computationally, a point `P` is inside if the great-circle arc from `R` to `P` crosses an odd
number of edges, counted over the exterior ring and every interior ring, where `R`
(`PolygonOnSphere::get_outside_point`) is a point *outside the polygon* under this definition:
inside an even number of rings, not necessarily outside each. Section 3.2 says how it is chosen.
Separately, a point within `s` of the outline is inside (section 3.5).

Parity, rather than winding, is the combining rule because the globe's filled polygons are drawn
that way and the two must agree. `GLFilledPolygonsGlobeView::FilledDrawables::add_filled_polygon`
draws each ring as a triangle fan from the apex `-R`
(`add_polygon_ring_mesh_to_current_filled_drawable`) into the stencil buffer with `GL_INVERT`, so a
pixel is filled when an odd number of fan triangles cover it. A fan triangle `(-R, v_i, v_i+1)`
covers `P` exactly when the arc from `P` away from `-R`, towards `R`, crosses the edge `v_i v_i+1`,
so the number of triangles covering `P` is the number of edges crossed by the arc from `R` to `P`:
coverage parity is crossing parity. Every other consumer of the interior (the area, the
orientation, the other fans, the bounding-circle shortcuts) takes the same `R` from the one
accessor (section 3.3), so all of them answer consistently for one polygon.

What parity means for awkward polygons: an interior ring that crosses the exterior ring, or a
self-intersecting ring, is defined by parity (a point outside the exterior ring but inside an
interior ring is inside), and reversing a ring changes nothing but a tie. Shapefile and GeoJSON
define interiors by winding (exterior one way, holes the other); GPlates does not, and never forces
interior rings to the opposite orientation.

**The limit.** A ring cannot mean more than a hemisphere: a ring round a region larger than `2 pi`
means the region's complement. Saying which side is meant needs an explicit side supplied by the
caller, which is deferred (section 7). A degenerate boundary centroid (a ring near a great circle,
whose weighted sum of edge midpoints vanishes) no longer puts `R` on the outline: `C` falls back to
the first vertex of the exterior ring (`Centroid::calculate_outline_centroid`). A ring spread round
a great circle has vertices outside the open hemisphere about that vertex, so it fails the
hemisphere test and `R` comes from the candidates of section 3.2 (`C` itself, a vertex, is too close
to the outline to be one; `PointInPolygonTest.ring_halving_the_sphere`).

### 3.2 Choosing R

`PointInPolygon::calculate_interior_reference` chooses `R` and records, per ring, whether the ring
passed the hemisphere test; `PolygonOnSphere::get_interior_reference` caches the result and
`get_outside_point` returns `R` from it. `R` has to be outside the polygon, and should be as far
from the outline as practical: point-in-polygon's bounds are circles about `R` and its lune tree is
built about the axis through `R`, so the further the outline the more the bounds decide; and a fan
triangle with apex `-R` is ill-conditioned when `-R` is nearly antipodal to a vertex, that is,
when `R` is near the outline.

**The hemisphere test.** Each ring is tested for lying in the open hemisphere about `C`
(`is_ring_in_open_hemisphere`: one exact dot product per vertex). If every ring passes, `R = -C`
and nothing else is computed. Each ring's interior is then the side away from `-C` (the other side
contains the whole closed opposite hemisphere, so it is the larger), `-C` is more than 90 degrees
from every vertex, and negation is exact, so point-in-polygon, area, orientation and the fans give
results bit-identical to the code before the rule, which assumed `-C` outside. Almost every real
polygon passes.

**The candidates.** Otherwise `R` is chosen from `-C`, `C` and 32 fixed directions, the 12
vertices and 20 face centres of an icosahedron (`get_fixed_candidate_directions`), so that no
point of the sphere is more than about 23 degrees from a candidate. Each candidate's distance to
the outline is `minimum_distance()` of the point to the polygon, outline only, through the
polygon's bounding tree (`get_candidates`). The candidates are tried in order of decreasing
distance (a stable sort, so equal distances keep the order `-C`, `C`, fixed), and the first that
is outside is `R`. A candidate closer than `0.01` radians to the outline (the table) ends the
first pass, since every later one is closer still; if none further away is outside, a second pass
takes the furthest outside candidate at least `1e-5` radians off the outline (its fan sums are less
accurate there, but rounding to the nearest multiple of `4 pi` tolerates large errors). It skips
the candidates the first pass found inside. An `R` from the second pass can be within the near
plane's reach (the table), so the filled polygon may be clipped near it, but point-in-polygon, by
crossing parity, is still right. That is better than the fallback below, where `-C` may be inside
and every answer is inverted.

**A candidate's status** is the parity (XOR) over the rings of whether the candidate `Q` is in the
ring's interior (`is_candidate_in_polygon`, `is_candidate_in_polygon_ring`):

- A ring that passed the hemisphere test: `Q` outside the open hemisphere about `C` is outside the
  ring; otherwise its crossing parity on the arc from `-C` to `Q`. (A `Q` within about `1e-6`
  radians of `C` takes `C`'s parity, on the arc from `-C` to `C` exactly, which is well defined
  despite being antipodal, section 3.7. Whether `C` is inside doesn't matter: `Q` is at least
  `1e-5` radians from the outline, in either pass, so no edge passes between them.)
- Any other ring: the fan sum `F` of the signed areas of the triangles `(-Q, v_i, v_i+1)` over the
  ring's edges (`SphericalArea::calculate_polygon_ring_fan_signed_area`). Each triangle covers,
  with sign, the points whose arc away from `-Q` crosses its edge, so `F` is the integral over the
  sphere of `w(x) - w(Q)`. Then `F / 4 pi` is the mean winding number less `w(Q)`, and rounding
  it gives `w* - w(Q)` (`get_ring_winding_offset`, which rounds half down with the tie
  tolerance: `ceil((F - 1e-8) / (4 pi) - 0.5)`). **`Q` is in the ring's interior if that integer
  is odd.** For a simple ring `|F|` is the area of the side not containing `Q`, so this says `Q` is
  in the smaller side when `|F| > 2 pi`; a tiny ring gives `|F|` near 0 or near `4 pi`, far from
  the decision point. Only accuracy matters, and that comes from `Q` being far from the ring,
  which the candidate distance ensures in the first pass.

**Why the winding test works.** On the sphere only differences of `w` are defined: crossing an edge
into the ring's left side adds 1, but there is no "far away" where `w` is 0. In the fan sum, `Q`
plays that part, since no triangle `(-Q, v_i, v_i+1)` contains `Q`. Writing `m` for the mean of
`w` over the sphere (with any constant), `F / 4 pi = m - w(Q)`, and since `w(Q)` is an integer,
`round(F / 4 pi) = round(m) - w(Q) = w* - w(Q)`: the fan sum from `Q` alone decides, without
knowing `m`. Some examples:

- A ring counter-clockwise round a region `L` of area `a`: `w` is 1 in `L` and 0 elsewhere, so
  `m = a / 4 pi`, and the interior is `L` if `a < 2 pi` and its complement otherwise. Run
  clockwise, `w` is 0 in `L` and -1 elsewhere, which gives the same sides.
- A small twist: a main lobe with `w = 1` and a twisted lobe with `w = -1`. `m` is near 0, so
  `w* = 0` and both lobes are inside, as parity from outside the ring gives.
- A loop twice round a small region: `w = 2` there and 1 in the region around it, so `w* = 0`.
  The once-wound region is inside and the twice-wound one is not, again as parity gives.

The answer changes only where `m` crosses a half-integer, which for a simple ring is where its area
crosses `2 pi`, so a ring of area `a` tolerates an error in `F` of up to `|a - 2 pi|`. This is
unlike finding a tiny ring's orientation from the sign of its area, where rounding can flip the
sign: for a tiny ring `F` is near 0 or near `4 pi`. For example, take a counter-clockwise cap of
radius 2 radians (8.9 steradians, more than a hemisphere) with `Q` inside it. Then
`F = -3.67`, and `F / 4 pi = -0.29` rounds to 0, which is even, so `Q` is not in the interior. That
is right: the interior is the smaller side, the 3.67 steradians beyond the cap.

No Gauss-Bonnet turning sum is needed to find a ring's area independently of a reference point:
the fan sum from any far-off apex already gives both the ring's outside level (its rounding) and
its signed area (its remainder, section 3.3), so one O(N) pass per candidate serves everything.
Choosing `R` costs O(N) per candidate tried, after the 34 distance queries, and is paid once, and
only by polygons that fail the hemisphere test.

**The fallback.** If no candidate off the outline is outside, `R` stays `-C` with the ring flags
as computed. That needs a polygon whose interior covers almost the whole sphere (rings that tile
it, say). `-C` may then be inside, and point-in-polygon and the fills are inverted while the area
follows the rule; storing `R`'s status and counting parity relative to it would remove this, but
no realistic polygon reaches it.

**Caching.** The `InteriorReference` (`R` and the ring flags), and the second reference point
`R2` with its status (section 3.8), live in the polygon's `CachedCalculations` beside the centroid
and the bounds, created lazily by `const` methods without synchronisation (section 3.10). The
free function `PointInPolygon::is_point_in_polygon` and the `Polygon` tester both take `R` from
`polygon.get_outside_point()`, so it is computed once per polygon at every speed.

### 3.3 Consumers of the interior

Every consumer of the interior uses the one outside point, as `R` (crossing parity from it, or "a
region containing `R` and no edge is outside") or as the fan apex `-R` (a parity fan from apex `X`
fills the side not containing `-X`):

| Consumer | What it takes from `R` |
| --- | --- |
| `PointInPolygon`, at every speed | The crossing arc starts at `R`; the bounds and lunes are about `R`; the status cached with the bounds is that of `-R` (section 3.7) |
| `SphericalArea::calculate_polygon_signed_area` and the per-ring variants | Each ring's signed area (`calculate_polygon_ring_signed_area`): for a ring that passed the hemisphere test, the fan sum from apex `C`, as before; otherwise the fan sum from apex `-R` less `4 pi` times `get_ring_winding_offset` of it, the same sum and the same rounding as decided `R`'s status. For a simple ring that is `F` reduced into `[-2 pi, 2 pi)`. The rings then combine as before: the exterior ring's signed area less the interior rings' absolute areas |
| `PolygonOrientation` | Unchanged: a gnomonic projection about `C` when every vertex is within 45 degrees of it (such a polygon passes the hemisphere test, so the projected sign is the rule's), otherwise the sign of the signed area, which follows the rule |
| `GLFilledPolygonsGlobeView::FilledDrawables::add_filled_polygon` (the globe's filled polygons) | Fan apex `-R`, into the stencil with `GL_INVERT` |
| `PolygonFan::create` | Fan apex `-R`. Its only caller, the fallback in `GLReconstructedStaticPolygonMeshes.cc` for a polygon that `PolygonMesh` could not mesh, is under `#if 0` |
| `GLRasterCoRegistration::render_fill_region_of_interest_geometries` (a fill region of interest) | Fan apex `-R` |
| `GLScalarField3D` (`SurfaceFillMaskGeometryOnSphereVisitor` and `VolumeFillBoundaryGeometryOnSphereVisitor`: the surface fill mask and the volume fill boundary's cap) | Fan apex `-R`. The cap's geometry shader (`volume_fill_spherical_cap_geometry_shader.glsl`) subdivides each arc from a vertex to the apex a fixed number of times, chosen for an apex near the polygon; for a wrap-around polygon, whose arcs to `-R` approach a half circle, the cap is coarser (it sags between subdivisions). Before the rule such a polygon was inside out here |
| `PolygonOnSphere::is_outside_bounding_small_circle_outside_polygon` (below) | True at once if `R` is outside the circle |
| `ResolvedTriangulationNetwork` | Its azimuthal equal-area projection is centred on `-R`, so that its singular point, the centre's antipode, is `R`: outside the network and far from it. A network that passes the hemisphere test is unchanged |
| `Centroid::calculate_interior_centroid` | Through the signed areas (below) |

**"Outside the outline's bounding circle" no longer means "outside the polygon".** The region
outside a small circle that bounds every ring contains no edge, so it is all inside or all
outside; the old code assumed outside, which a ring wrapping round the globe breaks (the antipode
of a helix band's centroid is outside its bounding circle and inside the band,
`PointInPolygonTest.interior_beyond_bounding_circle`).
`is_outside_bounding_small_circle_outside_polygon` answers for a given circle: true if `R` is
outside it (one dot product; true for every polygon that
passes the hemisphere test and a circle centred on `C`, as the polygon's own is, since its radius
is then under 90 degrees), true if the circle covers the sphere (there is no region outside it),
and otherwise one point-in-polygon query, threshold off, at the antipode of the circle's centre.
Its callers each skip a polygon, or shrink its extent, by a circle bounding its outline while
meaning its interior:

- the no-set-up path of `PolygonOnSphere::is_point_in_polygon`, which rejects a point outside the
  cached bounding circle (widened by `s`) only when `R` is outside that circle (it tests `R`
  directly, not through the helper);
- the four threshold exits in `GeometryDistance.cc` (section 2), when the polygon is solid;
- the two predicates in `TopologyReconstruct.cc` that drop a resolved boundary or network whose
  circle misses the geometry points' circle: the topology is kept if its interior reaches past its
  circle;
- `CubeQuadTreePartition::add` for a polygon: one whose interior reaches past its circle is added
  unpartitioned, at the root, so that its fill is not culled from the tiles outside the circle;
- `BoundingSmallCircle::test_filled_polygon` and
  `InnerOuterBoundingSmallCircle::test_filled_polygon`, which report an outline inside the circle
  as `INTERSECTING_BOUNDS` when the fill reaches past it.

**What follows automatically**, by calling point-in-polygon or the fans: `DateLineWrapper` decides
whether a pole is inside a polygon with `is_point_in_polygon`
(`generate_entry_exit_flags_for_dateline_polygon`); the map view fills the wrapped, projected
polygons by stencil parity (`GLFilledPolygonsMapView`, `GL_INVERT`), so it agrees with the globe;
`PolygonMesh` keeps a triangle only if `is_point_in_polygon` accepts its centroid; and
`GLReconstructedStaticPolygonMeshes` tests its quad-tree node polygons the same way.
`get_boundary_centroid` keeps its meaning, and its other callers use it as a bounding-circle centre
or a projection's tangent point (`PolygonMesh`, `PolygonOrientation`), which is valid whatever the
interior is.

**Area for crossing rings.** Combining the rings as the exterior ring's signed area less the
interior rings' absolute areas is exact only when the interior rings are disjoint and inside the
exterior ring. Otherwise it differs from the parity area, which is what point-in-polygon measures:
an interior ring `H` poking out of the exterior ring `E` has parity area `|E| + |H| - 2 |E n H|`
(the part of `H` outside `E` is inside one ring, so inside the polygon) where the formula gives
`|E| - |H|`; and the overlap of two holes inside `E` is inside three rings, so inside the polygon,
but the formula subtracts it twice. The parity area needs the rings intersected with each other
(an overlay), so it is kept as it is and documented (`SphericalArea.h`, the `get_area` docstring),
to revisit after the partitioning PR brings ring-intersection code (section 7). Each ring's own
area is at most `2 pi`, and for a polygon whose rings do not cross, the area agrees with the
fraction of random points inside (`PointInPolygonTest.wrap_around_band` checks this by Monte
Carlo).

**Orientation** keeps its meaning: whether the exterior ring's interior is to its left. For a
polygon the rule changes it can flip, and with it which plate is on the left of a topological
section (`TopologyInternalUtils::is_resolved_topology_on_left_of_boundary_sub_segment`); that is
the intended fix, since the old answer described the wrong side.

**The interior centroid** (`Centroid::calculate_interior_centroid`, pyGPlates
`get_interior_centroid()`) is split by size, not by the hemisphere test. A polygon whose outline's
bounding circle has a radius over `0.01` radians uses the exact area moment: by Stokes' theorem the
moment (the integral of position over area) of the region to a ring's left, weighted by winding
number, is half the sum over its edges of arc length times unit normal (the edge's rotation axis;
`Implementation::calculate_area_moment_of_polygon_ring`, with the arc length from `atan2` of the
cross product's magnitude and the dot product, and zero-length edges skipped). The sphere's own
moment is zero, so no winding offset is needed. Each ring's moment is negated when its signed area
(under the rule) is negative, interior rings' moments are subtracted, and the sum is normalised,
falling back to the first exterior vertex when it is zero. A smaller polygon keeps the fan of
triangles from `C`, each weighted by its area at its vertices' normalised mean, bit-identical to
before (its vertices all lie within `0.01` radians of `C`, so it passes the hemisphere test). The
split was measured against a subdivided high-precision reference: the fan approximation is off by
about `3e-6` at `0.1` radians and `0.3` degrees at one radian, and with apex `-R` its triangles for
a wrap-around polygon would be long, where it is worse; the exact moment's terms round at about
epsilon each while the moment is about the square of the polygon's width, so its direction errs by
about epsilon over the width squared, accurate above about `1e-3` radians and worse than the
polygon's own size at `1e-7`; at `0.01` radians the two agree to about `1e-10`.

### 3.4 Strict parity

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

### 3.5 The on-outline test

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
`GLReconstructedStaticPolygonMeshes.cc`. It is also off where the point is known to be far from
the outline and only parity is wanted: the status queries for `-R` and `R2` (sections 3.7 and
3.8) and the query at the antipode of a circle's centre in
`is_outside_bounding_small_circle_outside_polygon` (section 3.3).

### 3.6 Rejected alternatives

**Deciding on-outline by relaxing the pairing test against the edge's plane.** This was the
previous implementation: with the threshold on, condition (1) of section 3.4 compared
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

**Keeping the heuristic** (`R = -C`, assumed outside). It fails for C-shaped and wrap-around
rings, and for a degenerate centroid, where `R` was the antipode of a vertex, on or near the
outline; and every consumer was wrong together, consistently. GPlately's `ContouredContinent`,
which merges continental polygons into landmasses and sorts the contours by point-in-polygon, hit
this in practice.

**Orientation by default** (the interior to the left of a counter-clockwise ring, as in GeoJSON
and S2). GPlates has never enforced winding: edits and imports leave rings either way, and interior
rings were never forced to the opposite orientation, so a winding rule would turn about half of all
existing polygons inside out. The smaller-side rule makes them all mean what they visibly enclose,
and winding is used only to break a tie.

**A different centroid** as the heuristic's reference (the interior centroid, say). Still a
heuristic: a C-shaped ring can have any centroid inside or outside it.

**One outside point per ring.** The fill would work, since each ring's stencil fan is independent,
but point-in-polygon's lune tree and bounds are organised about a single axis, and with one `R` a
crossing count over all rings gives the whole polygon's parity directly.

**Choosing `R` by local geometry**: the idea in the old `FIXME` at `get_polygon_centroid`, to find
the nearest edge to a trial point and which side of it the point is on. That depends on the ring's
orientation, which is what is unknown; it has the near-parallel and through-a-vertex cases the old
comment listed; and it fails for self-intersecting rings, where the left and right sides swap at
each crossing.

**A Gauss-Bonnet turning sum** to find each ring's area without a reference point (on a unit
sphere the turning angles of a simple loop fall short of `2 pi` by the area on its left), to
compare with a fan sum. Not needed: the fan sum from a far-off apex already gives the ring's
outside level by rounding (section 3.2).

**Fan sums for point-in-polygon itself.** The winding test of section 3.2 decides any point, so
it could replace the crossing test, not just choose `R`. It is used only once per polygon, on
candidates chosen to be far from every edge. There are three reasons:

- Cost. A fan sum computes a spherical triangle's area (three `atan2`s) for every edge, on every
  query. Crossing parity needs a few dot products per edge, and the bounds and lune tree limit it
  to a handful of edges.
- Points near the outline. As the point nears an edge, the triangle on that edge has its apex
  nearly antipodal to the edge, so its area is ill-conditioned. Within about `1.4e-6` radians of a
  vertex, the apex is antipodal to it as far as `SphericalArea` can tell, and it rotates the apex
  by `1e-5` radians to get a triangle at all. Near the outline is where answers must be precise,
  and there the crossing test uses exact signs.
- Agreement. The fill is crossing parity (section 3.1), and the on-outline threshold (section 3.5)
  and the three speeds (section 3.7) are built on the crossing test.

**Per-ring outside points plus crossing parity**: find each ring's outside point first, then each
candidate's status by crossing parity from it. The winding formulation gives each candidate's
status from the fan sums directly, and also covers rings that cross themselves, which an earlier
draft left unspecified when they did not fit the hemisphere about `C`.

**Switching the interior centroid at the hemisphere test**, keeping the fan from `C` for every
ordinary polygon. Rejected once measured: the fan is already `0.3` degrees off for an ordinary
polygon one radian across, so the switch is by size (section 3.3).

### 3.7 The three speeds

`PolygonOnSphere::is_point_in_polygon` runs the same definition at three speeds
(`PointInPolygonSpeedAndMemory`), which must, and do, give the same answer:

- **Low** (no set-up): the free function `PointInPolygon::is_point_in_polygon`, O(N) over every
  edge of every ring, and the on-outline scan over all of them. If the polygon's bounding small
  circle happens to be cached, and `R` is outside it, `PolygonOnSphere::is_point_in_polygon` first
  rejects a point outside it, widened by `s` (section 3.3 says why `R` must be outside it).
- **Medium** (bounds): a `PointInPolygon::Polygon` built with `build_ologn_hint = false`, holding
  the inner and outer bounding small circles of all edges about `R`, and whether `-R` is inside
  (`d_is_outside_point_antipode_in_polygon`, found by counting crossings on the arc from `R` to
  `-R`: that arc is exactly antipodal, so the arbitrary plane chosen for it contains `R` as well,
  and any arc joining exactly antipodal points gives the same parity). A point inside the inner
  circle is outside the polygon (no edge lies between it and `R`); a point outside the outer circle
  has `-R`'s status (no edge lies between it and `-R`, which can itself be inside or outside);
  otherwise all edges are tested as at low speed.
- **High** (bounds plus the lune tree): `build_ologn_hint = true`, taken only for more than 16
  segments (`TreeBuilder::AVERAGE_NUM_EDGES_PER_LEAF_NODE`). The sphere is split into four
  equal-area lunes about the axis through `R` and `-R`, each split in two recursively while it
  holds more than 16 edges and the depth allows. Every crossing arc from `R` lies in one lune, so a
  leaf holds the edges that can cross such arcs (and some more, near its boundary), as runs of
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
  the lune's apex (`R` or `-R`, where it can cross a bounding great circle on its other half), or
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

### 3.8 Degenerate crossing arcs

When `P` is within about `1e-6` radians of `R` or of `-R` (`is_crossings_arc_degenerate`), the
crossing arc is too short or too nearly antipodal for its plane to be defined. The arbitrary plane
chosen then (`get_crossings_arc_plane_normal`) contains `P` but not, in general, `R`, so the test of
which side of each edge's plane `R` is on means nothing. Before this was handled, a bow-tie at
`R`, or a tiny polygon or a sliver at the antipode of one of its vertices, could put such a point
inside, at low speed only (the bounds hid it at the other speeds).

Such a point's crossings are counted instead from the **second reference point** `R2`
(`is_point_in_polygon_with_degenerate_crossings_arc`). `calculate_second_reference_point` takes,
from the same candidates as section 3.2, the one between 60 and 120 degrees from `R` that is
furthest from the outline (one always exists, since no point of the sphere is more than about 23
degrees from a candidate; the code asserts it), and finds its status by crossing parity on the
arc from `R` to `R2`, threshold off, an arc of 60 to 120 degrees and so well defined. `R2` need
not be outside; its status is what is cached (`SecondReferencePoint`). The degenerate query then
counts crossings on
the arc from `R2` to `P`, also about 60 to 120 degrees long, and the point is inside if that
parity differs from `R2`'s status, or if the threshold is on and it is on the outline. The answer
is as exact as any other query's, with or without the threshold.
`PolygonOnSphere::get_second_reference_point` computes `R2` lazily, by the first degenerate query
at any speed, so a polygon never queried within `1e-6` radians of `R` or `-R` pays nothing.

Rejected: taking the status of the nearby reference point, which was the first fix. Such a point
took `R`'s status (outside) or `-R`'s (cached by the bounds). With the threshold on that was
exact, since `1e-6` radians is less than `s`: if `P` was further than `s` from the outline no edge
could pass between it and the reference point, and if not, it was on the outline and inside. But
without the threshold it was wrong when `P` was also within about `1e-6` radians of the outline,
where an edge could pass between it and the point whose status it inherited, and a bow-tie's or a
sliver's centroid lies on its outline (`PointInPolygonTest.near_centroid_on_outline`). A two-arc
path from `R` through a waypoint to `P` was also rejected: the same idea as a second reference
point, at the cost of two crossing counts per such query instead of one cached status.

### 3.9 The adaptive policy

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
| Low | the centroid and the outside point, once, about 25 | 5.6 / 4.6 / 3.4 |
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

### 3.10 Thread safety

The polygon's `CachedCalculations` (the centroid, the bounds, the interior reference with `R`,
the second reference point `R2` and its status, the point-in-polygon tester and the call count) is
created and filled lazily by `const` methods without synchronisation, and so is the tester's count
of edge-testing queries; the comments at both say so. One polygon must therefore not be queried
from two threads at once. This is acceptable because pyGPlates holds the Python GIL during these
calls, and in GPlates only a script running in its Python thread, sharing a polygon with the GUI
thread, could do so.

## 4. Intersection

### 4.1 Who intersects what

Five pieces of code find where great circle arcs meet, each with its own arithmetic. Only one of
them, `GeometryIntersect`, is shared:

- **`GeometryIntersect`** (`GeometryIntersect.{h,cc}`, section 4.2) finds every intersection between
  two polylines or polygons. Topology resolving uses it: `TopologicalIntersections`
  (`src/app-logic/TopologyIntersections.cc`) intersects each section's polyline with its
  neighbours', and `ResolvedSubSegmentRangeInSection.cc` builds sub-segment ranges from the
  intersections and reuses the threshold. So does `PolylineIntersections`, which partitioning is
  built on (section 5.1) and which `GenerateVelocityDomainCitcoms.cc` also uses. Its threshold is
  also the default expansion of a bounding small circle, which the bounding trees use (section 1).
- **`GreatCircleArc::intersect`** (`GreatCircleArc.cc`) decides whether two arcs cross, and where.
  It serves the distance code only: `minimum_distance(arc, arc)` (section 2, section 4.3).
- **`PointInPolygon`** intersects an edge with the crossing arc's plane
  (`if_plane_divides_gca_get_intersection_of_gca_and_plane`, section 3.4).
- **`DateLineWrapper`** (`DateLineWrapper.{h,cc}`) clips polylines and polygons to the dateline
  (section 5.2). It finds the crossings itself, against a thick dateline plane, and does not use
  `GeometryIntersect`.
- **`GeometryCrossing`** (`GeometryCrossing.{h,cc}`) is meant to reduce a `GeometryIntersect` graph
  to the intersections where the geometries cross, dropping touches and overlaps that do not.
  It is a stub: `find_vertex_crossing` returns `boost::none`, so no vertex intersection survives.
  Nothing calls it, and it is compiled into GPlates only (it is in `gplates_only_srcs` in
  `src/maths/CMakeLists.txt`).

### 4.2 `GeometryIntersect`

**What it returns.** `intersect()` has four overloads: polyline with polyline, polygon with polygon,
and polyline with polygon both ways. Each polygon contributes either all its rings (the default)
or only its exterior ring.

The result goes into a `Graph`:
- `unordered_intersections`, the intersections in the order found;
- for each geometry, indices into them sorted along it (`geometry1_ordered_intersections`,
  `geometry2_ordered_intersections`).

The sort is by segment index, then by the angle from the segment's start point (an exact
comparison, `is_precisely_less_than`). A polygon's segments are numbered through its exterior ring
and then through each interior ring in turn, as `PolygonOnSphere::get_segment()` numbers them. So
"along" a polygon means ring by ring, each ring from its first vertex. The order does not say which
intersection follows the last one on a ring.

Each `Intersection` records:
- its position;
- the index of the segment it is on in each geometry;
- the angle along each of those segments from its start point;
- one of four types.

**Four types out of nine.** Where two arcs meet, each arc can meet the other at its start point
(S), at its end point (E), or in between (L). That gives nine combinations, drawn in
`GeometryIntersect.h`. An end point is the next segment's start point, so the combinations with an
E are dropped, leaving four types:
- `SEGMENTS_CROSS` (LL);
- `SEGMENT_START_ON_SEGMENT_START` (SS);
- `SEGMENT1_START_ON_SEGMENT2` (SL);
- `SEGMENT2_START_ON_SEGMENT1` (LS).

A vertex shared by the two geometries is therefore reported once, not once for each segment that
meets there. A polyline's last vertex has no next segment, so it is reported as the start of a
fictitious segment one past the last, whose index is the number of segments. A polygon ring wraps
round, so it never needs one (`POLYGON_NEEDS_NO_LAST_SEGMENT_INDEX`).

**Overlap is not a type.** Two arcs lying along each other report the start points that touch the
other arc, and each vertex inside a stretch where the geometries run together is reported as
touching the other geometry. The header comment gives examples. So no single intersection says
whether the geometries cross there, touch without crossing, or start or stop running together.
`GeometryCrossing` was begun to answer that from the graph.

**The test for one pair of arcs** (`intersect_segments`) has four steps, in this order:

1. **Coincident vertices.** All four pairs of end points are compared, by dot product, with the
   thickness cosine (section 1). A start point on a start point gives an SS intersection, placed
   at the first geometry's vertex. At a polyline's last segment, its end point is compared in the
   same way, as the start of the fictitious segment.
2. **Sides.** Each end point of each arc is classified against the other arc's plane, thickened
   by the thickness sine: on the positive side, on the negative side, or on the plane.
3. **Crossing.** A crossing needs four things:
   - both arcs are not zero length;
   - no end points coincide (step 1);
   - each arc's end points are strictly on opposite sides of the other's thick plane;
   - the orientations agree. When the first arc starts on the positive side, the second must
     start on the negative side, and the other way round. Without this, each arc could cross the
     other's great circle at antipodal points and never meet. The same sign later chooses between
     the cross product and its antipode.

   This gives one LL intersection, and the test stops there.
4. **Touching.** Otherwise, a start point lying on the other arc's thick plane, away from that
   arc's end points, and inside its lune (`point_is_in_segment_lune`, two exact sign tests) gives
   an SL or LS intersection, placed at that start point. So it can be up to the thickness off the
   other arc. End points are not tested: the next segment's start point covers them, except at a
   polyline's last vertex, which is tested here too. A zero-length arc has no plane, so nothing
   touches *it* except at a coincident vertex (step 1). Its own start point can still touch the
   other arc.

**Why vertices come first.** The comment in `intersect_segments` gives the reason. Take a point
near a vertex shared by two consecutive arcs. It can pass the point-to-vertex test (step 1) but
fail the point-to-plane test against one of the two arcs (step 2), since the two tests are not
exactly the same at the threshold. If the vertex test came only after the plane test, the point
could be judged on one arc's end vertex but not on the next arc's start vertex, which is the same
vertex. An arc passing through that vertex could then tunnel between the two arcs undetected. For
the same reason, a crossing is never tested once any end points coincide.

**Where the arcs cross** (`add_segments_crossing_intersection`). The point is the normalised cross
product of the two planes, its sign chosen by the orientations from step 3. The cross product can
be zero as a `real_t` (planes within about `1e-6` radians of each other). Then the point is
interpolated instead, by signed distances from the other arc's plane:
- along the first arc's chord, using its end points' signed distances from the second arc's plane;
- along the second arc's chord, if the first arc is near a half circle (section 1);
- if both arcs are near half circles, the first arc is split at its midpoint, and the point is
  interpolated along whichever half crosses.

The point is not checked against either arc's lune afterwards. Each end point's signed distance is
at least the thickness sine, which keeps the interpolation's denominator away from zero.

**Pruning.** Each polyline and polygon has a `PolyGreatCircleArcBoundingTree`: a binary tree of
bounding small circles over runs of consecutive arcs, with at most 8 arcs in a leaf by default.
`intersect_bounding_tree_nodes` descends both trees together. It skips any pair of nodes whose
circles do not intersect, and otherwise splits the larger node first. At a pair of leaves it tests
every pair of arcs. A polygon gives either its whole tree (`get_bounding_tree()`) or its exterior
ring's tree (`get_exterior_ring_bounding_tree()`). The circles are expanded by the thickness, so a
pruned pair of arcs cannot touch.

### 4.3 `GreatCircleArc::intersect` compared

The distance code's `intersect(arc1, arc2)` answers a narrower question, in a different way:

- **The decision is exact.** It compares the four signed distances with zero (`>= 0` on `.dval()`),
  with no thick plane, so a crossing is a crossing. `GeometryIntersect` decides with thick planes,
  and so can report arcs that touch, or nearly touch, as well.
- **It returns only whether the arcs meet.** There is no touch type and no overlap, no position
  along either arc, and no notion of a shared vertex reported once.
- **Where they cross** is the cross product, or, for near-collinear arcs, interpolation along the
  chord of the *better-conditioned* arc: the one whose end points differ most in signed distance.
  `GeometryIntersect` always tries the first arc first. The point must then lie in both arcs'
  lunes (`is_in_arc_lune`). Otherwise, or if neither arc can be interpolated, it uses the end point
  of either arc that is closest to the other arc (section 2). `GeometryIntersect` does neither the
  lune check nor the fallback.
- **A zero-length arc** meets the other arc if its start point lies on it (`lies_on_gca`, an
  epsilon test). The arc-to-arc distance does not call `intersect()` for one.

## 5. Partitioning

### 5.1 Partitioning today

**The layers.**
- **`GeometryCookieCutter`** (`src/app-logic/`) holds one `PolygonPartitioner` per partitioning
  polygon, which can be:
  - a resolved plate boundary;
  - a resolved network's boundary with its rigid blocks as interior rings, preceded (before
    sorting) by each rigid block as a polygon of its own;
  - a reconstructed static polygon.

  The polygons are kept in the order they were added: networks, then plate boundaries, then
  static polygons. Each of those groups can be stable-sorted, by plate ID or by area, highest
  first. Assign Plate IDs sorts by plate ID. Sorting can move a rigid block after its network.
- **`PolygonPartitioner`** (`src/maths/`) partitions one geometry by one polygon:
  - points and multi-points by point-in-polygon;
  - polylines and polygons through `PolylineIntersections`, which cuts both geometries into
    polylines at the intersections `GeometryIntersect` finds.

The other callers of `PolygonPartitioner` are `PolygonOnSphere.partition()` in pyGPlates, and
`GLReconstructedStaticPolygonMeshes.cc`. The latter uses only its inside/outside/intersecting
answer, for a quad-tree node's bounding polygon.

**The cascade.** `GeometryCookieCutter::partition_geometries` gives the input to the first
partitioning polygon. What lies outside that polygon goes to the second, and so on. Whatever is
left after the last polygon is outside all of them. A piece belongs to the first polygon that
contains it, which is how overlapping plates are resolved.

**`PolylineIntersections`** cuts each geometry into polylines between consecutive intersections
along it ("partitioned polylines"), each linked to the intersections at its ends. Every polygon is
treated as its exterior ring alone, as a closed polyline from its first vertex round to that
vertex again:
- **Interior rings are left out of the intersection.** The polygon overloads pass `false` for
  `include_polygon_interior_rings`, and say "TODO: Remove ... when proper polygon intersections are
  implemented (on top of GeometryIntersect)".
- **The ring is not wrapped.** The piece before the first intersection and the piece after the last
  are separate polylines, although they meet at the ring's first vertex.
- **An intersection at a polyline's first or last vertex** (a T-junction) has no piece on that side.

**Classifying a piece** (`is_partitioned_polyline_inside_partitioning_polygon`). The test is made at
the intersection where the piece starts, or, for the piece before the first intersection, where it
ends. It takes three arcs:
- the partitioning polygon's last non-zero-length arc before the intersection;
- the partitioning polygon's first non-zero-length arc after it;
- the piece's first (or last) non-zero-length arc.

If any of the three cannot be found (all the arcs are zero length), the piece is inside.

It decides which way they bend (`do_adjacent_great_circle_arcs_bend_left`, an exact `> 0` test on
a triple product), and so whether the piece leaves the intersection into the narrower angle
between the polygon's two arcs. The partitioning polygon's orientation then says whether that angle
is inside.

Consecutive inside pieces are joined into one polyline (`InsidePartitionedPolylineMerger`), but not
across the ring's first vertex. Each join repeats the intersection point, so a merged polyline has
a zero-length segment there. Outside pieces are not merged.

The decision is local, and that has consequences. Each piece is judged at one intersection, from
three arcs:
- **A piece that runs along the partitioning boundary** (an overlap) is collinear with the
  polygon's arcs. Every triple product is then zero up to rounding, and the exact `> 0` tests make
  the answer the polygon's orientation: inside for a clockwise polygon, outside for a
  counter-clockwise one. This was measured both ways, with the piece running in either direction
  along the edge. `InsidePartitionedPolylineMerger`'s comment assumes an overlap is inside. For the
  usual counter-clockwise polygon it is not.
- **An intersection where the geometries only touch** is treated like a crossing.
- **Only the partitioning polygon's exterior ring** is consulted.

**No intersection.** When `PolylineIntersections` finds none, the geometry is taken to be wholly
inside or wholly outside, decided by one vertex. That is its first vertex, or a polygon's first
exterior vertex, tested by point-in-polygon with the on-outline threshold off
(`is_non_intersecting_polyline_or_polygon_fully_inside_partitioning_polygon`). This test does count
the partitioning polygon's interior rings.

**What goes wrong.** These were measured with pyGPlates built from `main` at `601ca9e45`:

- **A polygon that intersects comes back as polylines.** The warnings in the `PolygonOnSphere.partition`
  and `PlatePartitioner.partition_geometry` docstrings describe this, and GPlates' Assign Plate IDs
  does the same. Cutting a square with a polygon covering its east half gives one of two results,
  because the ring is split at its first vertex whichever side that vertex lies on:
  - one inside polyline and *two* outside polylines, meeting at the square's first vertex, when
    that vertex is outside;
  - two inside and one outside, when it is inside.

  In the cascade, every
  partitioning polygon after the first that cuts it sees only polylines.
- **A partitioning polygon wholly inside the input polygon** cuts nothing. The input's first vertex
  is outside it, so the whole input is outside, and the part inside the partitioning polygon is
  lost. `GLReconstructedStaticPolygonMeshes` makes up for this itself, by testing whether the
  quad-tree node's polygon contains the partitioning polygon's first exterior vertex (with the
  on-outline threshold off).
- **Interior rings are invisible to the cut.** An input that crosses only a partitioning polygon's
  interior ring has no intersections. So one vertex decides all of it, whichever side the rest
  lies on:
  - a polygon reaching from inside the hole out past it is wholly outside if its first vertex is
    in the hole, and wholly inside if it is not;
  - a polyline across the hole is wholly inside when it starts in the solid part, and wholly
    outside when it starts in the hole.

  Likewise, an input polygon with a hole is wholly outside a partitioning polygon that crosses only
  that hole's boundary, even where the partitioning polygon covers the input's solid part.

**Scalar coverages** (`src/app-logic/PartitionFeatureUtils.cc`). Partitioning a feature carries each
scalar coverage of its geometries over to the pieces. A coverage's domain is the geometry's
exterior points only, because the scalar-coverage extraction (`ScalarCoverageFeatureProperties`)
and `PartitionFeatureUtils` both take exterior points only
(`GeometryUtils::get_geometry_exterior_points`). The TODO in `PartitionFeatureUtils.cc` asks for
interior rings.
- **A piece's point that matches an original point** takes that point's values. The match is
  through a `std::map` keyed with `PointOnSphereMapPredicate`, an epsilon comparison, so two
  original points close together can share one entry.
- **Any other point** (an intersection) is located on the nearest original segment, searched within
  0.5 degrees, or with no limit if nothing is found within that. Its values are interpolated along
  that segment. On a zero-length segment it takes the start point's values.

That relies on every point of a piece lying on the input's outline, which is true while the pieces
are polylines (the TODO there says so). The TODO asks for the partitioner to record where each point
came from instead, as `DateLineWrapper` does for its output.

### 5.2 Clipping by `DateLineWrapper`

`DateLineWrapper` (`DateLineWrapper.{h,cc}`) clips polylines and polygons to the dateline for the
rectangular map projections: a geometry that crosses longitude 180 is cut into pieces that each
stay on one side, so that no piece draws a line across the map. It also wraps longitudes into
`[-180 + central_meridian, central_meridian + 180]` and can tessellate the output. It is the only
polygon-by-polygon clipper in the code. The header cites Greiner and Hormann (1998) and, to a
lesser extent, Liu et al. (2007), and the implementation is Greiner-Hormann's three phases run
against a degenerate clip polygon: the **dateline polygon**, the whole globe less the dateline arc
from the south pole to the north pole, whose four corners are `(-90, 180)`, `(90, 180)`,
`(90, -180)` and `(-90, -180)`, in that order (clockwise seen from above the dateline, the comment
at `d_dateline_corner_*`). Its front edge runs up the dateline at longitude `+180`, its back edge
down it at `-180`, and its other two edges are the poles. Every point of the globe off the arc is
inside it, so clipping by it removes nothing: an output piece follows the geometry, and where it
crosses the dateline it follows the dateline's front or back edge instead. The pieces along the
front edge come out at longitude `+180` and those along the back at `-180`, which is the wrapping.
A geometry entirely *on* the arc is, the header says, outside the polygon, and is passed through
unchanged with every longitude set to `central_meridian - 180`.

`wrap_polygon` and `wrap_polyline` first reject a geometry whose bounding small circle misses the
dateline (`possibly_wraps`, `intersects_dateline`), rotate it about the north pole into the frame
where the central meridian has longitude zero (`CentralMeridian`) when that is not already so,
build an `IntersectionGraph` from its rings, and rotate the output points back.

**Data structures.** A `Vertex` holds the point in both forms (`lat_lon_point`, `point`); whether
it is an intersection (`is_intersection`); its twin in the other list and that list
(`intersection_neighbour`, `intersection_neighbour_list`, `void *` because of the cyclic types);
whether it is an original point (`is_unwrapped_point`); its provenance
(`interpolate_original_segment`, below); the entry/exit flag (`exits_other_polygon`); and whether
it has been output (`used_to_output_polygon`). Vertices are nodes of a doubly linked list with a
sentinel (`GPlatesUtils::SmartNodeLinkedList`), allocated from a `boost::object_pool`. The
`IntersectionGraph` keeps one list per input ring, exterior first (a polyline has one), each with a
flag `intersects_dateline`, and, for a polygon graph only, the one list `d_dateline_vertices`
holding the four corners and the dateline copies of every intersection from every ring. Two flags
record whether the geometry touched the north or the south pole. So the structure is Greiner and
Hormann's: an intersection is two linked nodes, one in each polygon (their `neighbor` pointer),
not Liu's single shared node.

**Vertex classes** (`classify_vertex`). The dateline's plane is thick. A vertex whose signed
distance from it (the dot product with the plane's normal, the `y` axis) exceeds
`EPSILON_THICK_PLANE_SINE` is `CLASSIFY_FRONT` (longitude in `(0, 180)`), and below minus that it
is `CLASSIFY_BACK`. On the thick plane it is first tested against the poles: a dot product with a
pole above `EPSILON_THICK_PLANE_COSINE` makes it `CLASSIFY_ON_NORTH_POLE` or
`CLASSIFY_ON_SOUTH_POLE` (a cap about `4.5e-5` radians across, tested first because, the comment
says, it eats into the region tested next) and sets the touched-pole flag. Otherwise an exact sign
against the hemisphere containing the dateline puts it `CLASSIFY_ON_DATELINE_ARC` or, near
longitude zero, `CLASSIFY_OFF_DATELINE_ARC_ON_PLANE`. The thickness (section 1) is about `4.5e-5`
radians, some 45 times `GeometryIntersect`'s thickness and 30 times the point-on-outline
threshold, and is not derived from `EPSILON`.

**Finding crossings.** `add_line_geometry` classifies the vertices and hands each segment with its
two classes to `add_line_segment`, a six-by-six case table; a ring's first vertex is handled as the
end of its last segment, a polyline's first vertex is added first unless it is on the arc or a
pole. Only a segment going from front to back, or back to front, is intersected. Every other case
is decided from the classes alone:

- front to front, back to back, front or back to off-arc-on-plane and the reverse, and off-arc
  to off-arc: the end vertex is appended as a regular vertex (`add_vertex`);
- a segment ending on the arc (front or back to on-arc): one intersection vertex at its end point,
  on the front or the back dateline, flagged as exiting; the next segment, starting on the arc,
  adds an intersection at its start, flagged as entering, and then its end vertex. A vertex of the
  geometry on the dateline therefore produces **two intersection vertices at the same point**, one
  per adjacent segment. A crossing through the vertex puts one on each side; a touch puts both on
  the same side, an exit then an entry;
- a segment with both ends on the thick plane, one on the arc and one off it, passes a pole.
  `does_line_segment_on_dateline_plane_cross_north_pole` decides which from the sign of the
  segment's normal against the plane's normal, with its own epsilon of `1e-4`, and below that from
  the sign of the end point's `z`. The intersection is at the pole with the longitude of the
  off-arc end point and a ratio from the arc length to the pole;
- a segment with an end at a pole: an intersection at the pole with the other end's longitude,
  and the end vertex when the segment leaves the pole;
- both ends on the arc or at poles: nothing is added, the segment being outside the dateline
  polygon (the comments say "as odd as it sounds"). A ring of such segments adds no vertex at all.

`intersect_line_segment` handles the front-to-back case. A zero-length segment returns its start
point (the comment says this cannot happen, since the ends are on opposite sides of a plane wider
than such a segment). The dateline is a half circle from pole to pole, so every segment's plane
separates its end points and only Greiner-Hormann's second test remains, the orientation test of
section 3.4 (2): the south pole's signed distance from the segment's plane, beyond the thickness
sine, must have the sign opposite to the segment's start point's side of the dateline plane;
otherwise the segment crosses the dateline's great circle on the far side of the globe and there is
no intersection. If instead the south pole is on the segment's thick plane, the segment passes
within the thickness of a pole, and each pole is tested for lying on the segment with two exact
signs (closer to the start point than the end point is, and on the half circle starting at the
start point) and returned as the intersection. `calculate_intersection` interpolates along the
chord by the end points' signed distances (their `y`), with `real_t` tests on the denominator and
on the interpolated vector's magnitude, which rejects a near-antipodal pair, normalises, and gives
the ratio `acos(dot) / arc_length`. A crossing of the dateline appends two intersection vertices to
the geometry's list, the front copy and the back copy in the order crossed, the first flagged
exiting and the second entering, and then the end vertex. A crossing of a pole likewise appends
two vertices at the pole, with the start point's and the end point's longitudes, so that a
meridian stays vertical in the projection. Every intersection vertex is snapped: its longitude is
set to exactly `+180` or `-180`, or its latitude to `+90` or `-90`, and its `PointOnSphere` is
rebuilt from that (the `Vertex` constructor given no point).

**Entry and exit.** The geometry's flags are set at creation, from the direction of the crossing
(`exiting_dateline_polygon`): a geometry leaves the dateline polygon at the first copy and
re-enters at the second, and a vertex on the arc exits at the end of the segment arriving and
enters at the start of the segment leaving. The clip polygon is the globe, so every intersection
exits and re-enters it, and nothing is toggled along the geometry. The dateline's flags are
Greiner-Hormann's phase 2 (`generate_entry_exit_flags_for_dateline_polygon`): one point-in-polygon
test of the north pole against the input polygon (`is_point_in_polygon`, low speed, the
on-outline threshold on by default), then, from the north-front corner, toggling at each
intersection vertex round the dateline list. If the geometry touched the north pole the south pole
and the south-front corner are used instead; if it touched both, the start is assumed inside. The
comment calls that arbitrary and says a polygon touching both poles is large, so being inside out
is "not as bad". This is the only point-in-polygon call, and it is at a corner of the clip polygon
that the geometry is known not to touch.

**The dateline list is kept in order** so that the toggle walks round the clip polygon: up the
front from the south-front corner at increasing latitude, across the north pole from the
north-front to the north-back corner at decreasing longitude, down the back at decreasing latitude,
and across the south pole at increasing longitude. Each `add_intersection_vertex_on_*` inserts the
dateline copy by a linear scan between the right pair of corners on that one key
(`splice_self_before`); an equal key goes before the vertex already there. Since the clip polygon's
edges are two half meridians and two points, a position along an edge is a latitude or a
longitude, and no segment index or ratio along the clip polygon is needed.

**The traversal** (the private `output_intersecting_polygons`) is phase 3. An output polygon
starts at a dateline vertex that is an intersection, exits the geometry polygon, and is unused.
From there: append the vertex to the output ring and mark it used; at an intersection vertex jump
to its twin, mark the twin used, and set the direction, forward if going forward from the twin
does not exit the other polygon and backward if it does (`list_traversal_is_forward =
!exits_other_polygon`); step, wrapping round the list; stop on returning to the start node. The
ring alternates between stretches of the geometry (all inside the dateline polygon) and stretches
of the dateline inside the geometry polygon. Each ring traced is the exterior ring of a new
`LatLonPolygon`, and the loop over start vertices runs until every exit vertex is used, so one
input ring can give several pieces. A corner is passed through when the geometry contains a pole,
and the output then runs along the top or bottom of the map.

**Interior rings.** Each ring has its own list but every ring's intersection copies go into the
one dateline list, so the toggle round the dateline counts crossings of all rings: parity across
rings, as section 3.1 defines the interior. A traced ring can pass from the exterior ring to an
interior ring by way of the dateline, which is how a hole crossing the dateline becomes a notch in
the pieces (the note at `get_interior_ring_point_flags` says a wrapped polygon can follow part of
an exterior and part of an interior ring). No traced ring is ever made an interior ring of the
output, and there is no containment test among them. The `FIXME` in the traversal lists what is
not handled, self-intersecting rings, holes touching the exterior ring, and a hole that intersects
both clipped halves of the exterior ring, and asks for the input to be made Shapefile-standard
first.

**Rings that don't cross** (`output_non_intersecting_polygon_rings`). For each ring,
`get_line_geometry_intersection_results` reports `INTERSECTS_DATELINE` (the flag),
`ENTIRELY_ON_DATELINE` (its list is empty, every vertex having been swallowed) or
`ENTIRELY_OFF_DATELINE`. If every ring intersects there is nothing to do; if every ring is off, the
polygon is output as it is, from the graph's lists. Otherwise the off rings are taken in order,
exterior ring first. With `group_interior_with_exterior_rings` (the default) each becomes an
interior ring of the first output polygon whose exterior ring it intersects, lies inside or contains (both are treated as solid), or
else the exterior ring of a new output polygon, which then becomes a candidate itself. The
candidates start as the traced pieces. The test builds a `PolygonOnSphere` from each ring's
untessellated points and compares `minimum_distance()` of the two solid polygons with
`AngularDistance::ZERO`, an epsilon comparison (section 1), so a ring within about `1.4e-6`
radians of an exterior ring joins it too. Without the grouping every off ring is its own polygon;
the header says the map view's filled polygons do not need it, since stencil parity makes the
rings' roles irrelevant. A ring entirely on the dateline is output as its own polygon from the
input ring, every point at `central_meridian - 180`.

**Polylines.** A polyline graph has no dateline list; its intersection vertices are appended to
the one list and never linked. `output_polylines` ends a polyline at each intersection vertex and
starts the next at the following vertex, so a crossing's front copy ends one piece and its back
copy starts the next. A stretch along the dateline is dropped, and a polyline entirely on the
dateline is output unchanged (`output_polyline_if_entirely_on_dateline`).

**Where each output point came from.** Every vertex carries an `InterpolateOriginalSegment`: the
ring index, the segment index and the ratio along the segment. An original vertex is recorded as
the *end* of its incoming segment (ratio 1; a ring's first vertex as the end of its last segment)
when it comes through the graph, and as the *start* of its outgoing segment (ratio 0, segment
index equal to the vertex index) when the geometry is passed through unclipped
(`output_input_line_geometry`); the tessellation code tells the two apart by `is_unwrapped_point`.
A computed crossing has the ratio from `calculate_intersection`; a vertex on the dateline has 0
or 1 and is unwrapped. The public `LatLonPolygon` and `LatLonPolyline` return, per output point,
flags (`ORIGINAL_POINT`, `ON_DATELINE`, `ON_ORIGINAL_SEGMENT`, `TESSELLATED_POINT`) and the
optional record. Tessellation (`add_tessellated_points`) subdivides the arc between consecutive
untessellated points; a tessellated point on an original segment gets a ratio interpolated between
its end points' records (the case analysis there reconciles the two conventions above), and one
along the dateline has no record and its longitude pinned to the end points'. A corner vertex has
no record and no flags at all, being neither original nor an intersection. The arc between the two
poles, antipodal, which the comment attributes to the arbitrary entry/exit choice when both poles
are touched, is tessellated in latitude.

**Degeneracies.** Nothing is perturbed. A vertex near the plane or a pole is snapped by the thick
classes; a vertex on the arc becomes a pair of intersection vertices, which makes a crossing
through a vertex and a touch at a vertex both consistent with the toggle (a touch gives an exit
and an entry at one point, the traversal leaves the geometry at one and returns at the other, and
the output ring repeats the point); a stretch along the arc is dropped and a whole geometry on it
passed through; a zero-length segment that still straddles the plane intersects at its start
point; a near-antipodal segment, or one whose ends are both too near the plane, gives no
intersection; and the both-poles case is guessed. The tolerance is one global number chosen for
display, not the one the rest of this document uses.

**What depends on the dateline, and what generalises.** Dateline-only, in that each would need
replacing for a general clip polygon:

- the clip polygon's edges are half meridians and points, so a position along one is a scalar, the
  dateline copies are placed by a scan on that key, and the orientation test needs only one sign;
  crossings are sought only at front-to-back transitions of a classification against one fixed
  plane, and vertex classes decide every other case;
- the clip polygon's interior is the whole globe, so the geometry's entry/exit flags come from the
  crossing direction alone, with no point-in-polygon test and no toggling, and every crossing is
  two copies (front and back), which is the wrapping itself;
- the dateline's flags start from one point-in-polygon test at a corner the geometry does not
  touch, falling back to the other corner and then to a guess;
- the thickness, and the snapping of intersections to the dateline;
- every traced ring is an exterior ring, with no containment test among them, and a hole that
  crosses the clip boundary is always merged into an exterior;
- the off rings are grouped by an epsilon distance test against the output exteriors, in a fixed
  order.

General, and the same as Greiner-Hormann's:

- the two linked lists with twin links and node pool, with one list per ring of the geometry and
  one shared list for the clip polygon, which is what makes the toggle count parity across rings;
- the phase-2 toggle from one vertex of known status, and the phase-3 walk with the direction rule
  `forward = !exits`, continued until every exit vertex is used;
- the per-vertex provenance (ring, segment, ratio) and flags, kept through the output;
- the per-ring intersection status, and the separate handling of rings that do not intersect;
- the pair of intersection vertices for a vertex lying on the other boundary, which stands in for
  a crossing-or-touch classification; here the pair's flags are read from the neighbouring
  vertices' classes, which a general clipper would have to decide by a side test.

## 6. Testing

The Python suite (`pygplates/test/test_maths/test_geometries_on_sphere.py`) carries references
independent of the code under test. `_brute_force_point_in_polygon` applies the definition of
section 3.1 ring by ring. A ring that passes the hemisphere test takes the anchor `-C`, outside;
any other ring takes an anchor far from it (the best of up to 200 random points by distance to the
ring's edges) and decides whether the anchor is inside from the fan sum of signed triangle areas
about the anchor's antipode, by the Van Oosterom and Strackee formula (pyGPlates sums interior
angles), returning `None` for a fan sum within `1e-7` steradians of an odd multiple of `2 pi`, a
tie, where either answer is accepted. From each anchor it counts crossings to the point with exact
orientation predicates (`_orient`: a `float` filter with a fixed error bound, falling back to
`fractions.Fraction`, which is exact for doubles) and S2's four-orientation crossing rule, through
a random waypoint so that neither arc is nearly antipodal or nearly zero length, rerouting if any
orientation is exactly zero; the vertices are perturbed by about `1e-12` so that no orientation
involving a vertex is zero. The rings combine by parity. It returns `None` for a point in a band
around the outline (`1.3e-6` to `1.5e-6` radians), where either answer is accepted but every speed
must agree; within `1.3e-6` the answer must be inside (without the threshold, the band extends to
the outline). `test_point_in_polygon_against_brute_force` runs it over tiny, ordinary,
hemisphere-sized, holed, sliver and bow-tie polygons, bands along a helix that wrap round the globe
(`_helix_band`, sweeping 300 to 700 degrees of longitude, so that the antipode of the boundary
centroid can be inside) and an exterior and an interior ring on opposite sides of the globe, and
the points listed in its comment, at every speed. `_brute_force_distance` (every pair of segments,
with its own point-to-arc distance) uses the same point-in-polygon, threshold off, to decide solid
interiors; `test_distance_against_brute_force` and `test_distance_threshold_is_exact` check the
wrapper's contract. `test_interior_centroid_of_large_polygon` checks the exact area moment against
a fan of triangles each split 4096 ways, for two large lopsided polygons.

The C++ tests (`src/unit-test/PointInPolygonTest.cc`, `GeometryDistanceTest.cc`) call each speed
explicitly, with the threshold on and off, and check the C++ contracts that the wrapper hides: a
threshold equal to the distance (for a point and an arc, and for every pairing of geometry types,
solid or not, half of them near-antipodal), a threshold of pi for exact antipodes and for
geometries exactly pi apart, and zero distance inside a solid polygon near its outline. The
zero-length last segment and the wrapper's zero clamp and exact radians threshold are checked
from Python (`test_distance_to_zero_length_segment`, `test_distance_threshold_is_exact`).

The interior rule's C++ tests: `wrap_around_band` (a helix band `0.065` radians wide sweeping 576
degrees, whose boundary centroid's antipode is inside: the band's centre line is inside and the
gaps between its turns outside at every speed, the area is the band's and agrees with a Monte
Carlo estimate, the signed area is positive and the orientation counter-clockwise);
`ring_halving_the_sphere` (four points on the equator, in both orders: the hemisphere to the
ring's left is inside and its signed area is `2 pi`); `antipodal_rings` (an exterior and an
interior ring on opposite sides of the globe, both inside); `outside_point_close_to_outline` (the
20 faces of an icosahedron as rings, one vertex's corners cut off `5e-3` radians along its edges,
so only a gap round that vertex is outside: `R` comes from the second pass of section 3.2, the cut
vertex about `4e-3` radians from the outline; the gap is outside and the rest inside, at every
speed; with the second pass disabled `R` falls back to `-C` and every answer is inverted);
`near_centroid_on_outline` (a
symmetric bow-tie whose centroid is its crossing: points `1e-8` to `5e-7` radians from it, into a
lobe or between the lobes, threshold off, at every speed); and `interior_beyond_bounding_circle`
(the helix band's bounding circle excludes a point inside it: the shortcut helper says so, the
solid distance to the point is zero despite a threshold smaller than its distance to the circle,
`test_filled_polygon` reports `INTERSECTING_BOUNDS`, and an ordinary polygon gives the old
answers).

The regression cases near `R` and `-R` (`near_antipodal_centroid_and_centroid`,
`test_point_in_polygon_near_antipodal_centroid`: a bow-tie at `R`, a tiny polygon and a sliver at
a vertex's antipode) came from a randomised hunt at a far larger scale than the suite, because once
strict parity was in, the suite's random cases no longer reached them. Likewise the case of edges
passing round a lune's apex (`edges_through_centroid`,
`test_point_in_polygon_near_centroid_of_crossing_edges`: a self-intersecting polygon whose crossing
edges pass through its centroid) came from a review, and depends on the polygon's orientation (the
tree's lunes are placed by it). The hunt was rerun for the interior rule over 4,701 polygons
(172,659 queries) in 15 families, adding helix bands, wrap-around bands with a small twist
(self-intersecting) and rings on opposite sides of the globe, with no mismatch against the
reference and no disagreement between speeds; the code before the rule fails in each of the three
new families. A bit-identity check over 1,500 polygons compared point-in-polygon at every speed
(with the threshold on, as Python calls it), the area, the signed area and the orientation against
a build of the code before the rule: every polygon that passes the hemisphere test was identical,
the only differences being in the interior centroid of the 879 polygons wider than its size switch,
which is the change intended.

Intersection and partitioning have no C++ tests of their own. In C++, `GeometryIntersect` is
reached only through `GenerateVelocityDomainCitcomsTest.cc` (via `PolylineIntersections`).
Otherwise it is reached through the pyGPlates topology and partition tests.
`GreatCircleArc::intersect` is exercised through the distance tests above. The pyGPlates partition tests (`test_partition` in `test_geometries_on_sphere.py`,
`test_partition_geometry` in `pygplates/test/test_app_logic/test.py`) assert today's behaviour,
including the polylines from an intersecting polygon ("will be fixed in future") and three outside
pieces "instead of 2".

## 7. Deferred work

- **An explicit side for a ring larger than a hemisphere.** Under the rule a ring cannot mean more
  than a hemisphere, so a plate polygon enclosing more than `2 pi` means its complement. The fix is
  a way to say which side is meant: a reference point supplied by the caller or by the topology
  resolver, or a winding convention chosen per call. Deferred because no real plate needs it yet,
  and it changes the API. Resolved plates every 10 to 20 Myr in Müller 2019 (0-250 Ma), Merdith 2021
  (0-1000 Ma) and Cao 2024 (0-1800 Ma) never exceed half the sphere; the closest are India-Australia
  at 720 Ma (47.7% in Merdith 2021) and Rodinia at 860 Ma (47.1% in Cao 2024), so a
  supercontinent-era model could cross the limit.
- **Polygon partitioning.** An intersecting polygon is partitioned into polylines, interior rings
  are left out of the cut, and a partitioning polygon inside the input polygon is missed
  (section 5.1). Finishing it needs intersections classified as crossings, touches and overlaps,
  which `GeometryCrossing` was begun for (section 4.1).
- **The exact parity area** for interior rings that cross the exterior ring or overlap each other
  (section 3.3). It needs the rings intersected with each other, which the partitioning PR brings,
  so it is revisited after that.
- **Thin fan triangles.** `calculate_spherical_triangle_signed_area` (the three-edge overload in
  `SphericalArea.cc`) returns zero when any side is zero length (under about `1e-6` radians), even
  for a long, thin triangle, so the fans lose area and moment. Measured: a sliver about `1e-6`
  radians wide gets about a quarter of its true area; a polygon `1e-5` radians across with short
  edges gets an interior centroid off by about its own size; and at `1e-7` radians every fan side
  is zero length, so the centroid falls back to the first vertex. Plate areas are unaffected
  (Müller 2019 at 0, 100 and 200 Ma, worst `8e-14` steradians). The fix is a triangle formula that
  handles thin triangles (Van Oosterom and Strackee, say), which changes every area by rounding,
  so it is its own change.
