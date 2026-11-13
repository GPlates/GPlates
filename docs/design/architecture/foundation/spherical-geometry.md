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
inside and outside it (the overlay `PolygonOverlay.{h,cc}`, `PolygonPartitioner.{h,cc}` on top of
it, and the plate partitioner `src/app-logic/GeometryCookieCutter.{h,cc}`, section 5). The overlay
also computes the other Boolean operations of two polygons (the union, both differences and the
symmetric difference). pyGPlates exposes partitioning through `PolygonOnSphere.partition()` and
`PlatePartitioner`, and the Boolean operations through `PolygonOnSphere.intersection()`, `union()`,
`difference()` and `symmetric_difference()` (section 5.1); GPlates exposes partitioning through
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

This page is the reference: complete and exact, and most of it is the handling of degenerate
cases. Two overviews give the ideas first, each for one section: the
[point-in-polygon overview](point-in-polygon-overview.md) for section 3 and the
[polygon overlay overview](polygon-overlay-overview.md) for section 5. Comments in the code cite
this page's sections by number (`spherical-geometry.md section 5.2`, or `section 5.2` alone; about
180 of them cite section 5), so no section or subsection is renumbered: new material goes into the
section it belongs to, or at the end of one.

## 1. Tolerances

The one table of every tolerance in this code. "Exact" means a plain `double` comparison (through
`.dval()`, or `is_precisely_less_than` / `is_precisely_greater_than` on `AngularDistance`);
"epsilon" means a `real_t` comparison, which treats values within `EPSILON` as equal.

| Tolerance | Where | What it decides | Why that value |
| --- | --- | --- | --- |
| `EPSILON = 1e-12` | `MathsUtils.h`; every `real_t` comparison (`<`, `<=`, `==` and the operators Boost derives from them), and so `AngularDistance::operator<` and `AngularExtent::operator<`, which compare cosines as `real_t` | Equality of any two `real_t` values. On a cosine (the dot product of two unit vectors) `1e-12` below 1 is about `1.414e-6` radians, so two points that close compare as coincident | The comment at its definition calls it a guess (2004) that allows for a small accumulation of rounding error. Everything below is either this number seen through a cosine, or derived from it |
| Zero-length arc, about `1e-6` radians | `GreatCircleArc::is_zero_length`: the cross product of the end points has `magSqrd() <= 0` as a `real_t`, so its square is at most `1e-12` and the sine of the arc's angle at most `1e-6`. `get_zero_length_threshold_cosine()` is the same bound as a cosine, `cos(asin(sqrt(EPSILON)))` | Whether an arc has a rotation axis (`rotation_axis()` throws without one). A zero-length arc is measured to the closer of its two end points (`minimum_distance(point, arc)`), which can still be about `1e-6` radians apart; it intersects another arc only by its start point lying on it (`intersect()`); the arc-to-arc distance does not call `intersect()` for one; in a crossing count it stands in for its own intersection with the crossing plane (section 3.4); the on-outline scan cannot pre-reject it by its plane (section 3.5); a spherical triangle with such a side has zero area (section 7); and the exact area moment skips it (section 3.3) | The square of the cross product's magnitude is compared with `EPSILON` |
| Point-on-outline threshold `s`: cosine `1 - 1e-12`, about `1.414e-6` radians (about 9 metres) | `PointInPolygon::get_point_on_polygon_threshold_cosine()` and `get_point_on_polygon_threshold_sine()` (its sine, `sin(acos(1 - 1e-12))`) | A point within `s` of any edge or vertex, of any ring, is on the outline and so inside (section 3.5), at every speed | The same number as `EPSILON` on a cosine: a distance of `s` is one that compares equal to zero. `GeometryIntersect`'s thickness is the same number (`GeometryIntersect::Intersection::get_on_segment_start_threshold_cosine()` returns `1 - EPSILON`), so a point on the outline here is a contact there |
| Bounds widening, `s` plus the zero-length threshold (about `2.4e-6` radians) | `PointInPolygon::get_bounds_widening()`: the inner bounding circle is contracted and the outer expanded by it, for the whole polygon and for each lune-tree leaf. The no-set-up path through `PolygonOnSphere::is_point_in_polygon` adds `s`, per call, to the polygon's cached outer bounding circle, which `InnerOuterBoundingSmallCircleBuilder` has already expanded by its default, `GeometryIntersect`'s thickness (`s`) | When a point is far enough inside the inner circle or outside the outer one to be decided without testing edges (section 3.7) | A bounds decision skips the on-outline test, so the bounds must be wider than `s`, or a point within `s` of the outline would be outside at one speed and on the outline at another. The extra `1e-6` covers the bounds comparing cosines, which resolve angles near zero only to about `1.5e-8` radians (the comment at `get_bounds_widening` says this) |
| Near-parallel edges, `1 - 1e-4` | `MAX_DOT_PRODUCT_CROSSING_ARC_AND_POLYGON_EDGE` in `PointInPolygon.cc`: the dot product of the crossing arc's and the edge's plane normals (planes within about `0.014` radians of each other) | Which method decides whether an edge crosses the crossing arc: the sign tests of section 3.4, or interpolating the edge's crossing of the crossing plane and testing that point against the arc | A method choice, not an answer tolerance: the comment says the actual value is unimportant, it only funnels the common case into the cheaper test |
| Lune widening `1e-4` | `SphericalLuneTree::TreeBuilder::SPHERICAL_WEDGE_PLANE_EPSILON`, a sine (about `1e-4` radians, `0.34` arc minutes) | Which edges belong to a lune: those with an end point in the lune widened by it, those crossing its first bounding half circle, and those coming within it of either bounding half circle | Protection against rounding when assigning edges, and much larger than `s`, which is what makes a leaf's edges enough for the on-outline test (section 3.7) |
| Half-circle guard, chord midpoint `magSqrd()` at most `1e-6` (magnitude `1e-3`; `<=` in `interpolate_plane_crossing`, `<` in `PointInPolygon.cc`) | `interpolate_plane_crossing` (`GreatCircleArc.{h,cc}`, used by the distance code and by `add_segments_crossing_intersection` in `GeometryIntersect.cc`) and `if_plane_divides_gca_get_intersection_of_gca_and_plane` (`PointInPolygon.cc`) | Whether an arc is too close to a half circle to interpolate along its chord. The distance code then tries the other arc, and failing that uses the end point closest to the other arc; point-in-polygon splits the arc at its midpoint and recurses; `GeometryIntersect` tries the other arc, and failing that raises an assertion, since its interpolation is unreachable (section 4.2) | Both halves of an exactly antipodal pair are the same distance from every plane, so the ratio of signed distances means nothing |
| Cosine resolution, about `1.5e-8` radians | Not a constant: the largest `double` below 1 is `1 - 2^-53`, whose arc cosine is about `1.5e-8`. Every distance here is made from a cosine (`AngularDistance::create_from_cosine`), including inside an arc's lune, where it is `sqrt(1 - sine^2)` | The smallest non-zero distance. In C++ anything closer than this is exactly `AngularDistance::ZERO` | A property of `double`, stated in the wrapper's comment and at `get_bounds_widening` |
| Cosine clamp | `clamp_cosine_rounding_error` (`AngularDistance.h`), applied by `AngularDistance::create_from_cosine` and `AngularExtent::create_from_cosine` | A cosine outside `[-1, 1]` by no more than epsilon is clamped to the end it represents; anything further out is left for `acos` to report | The dot product of a point and its exact antipode can round to just below `-1`, which exact comparisons would then see as exceeding a threshold of exactly pi (section 2) |
| pyGPlates zero clamp | `geometry_on_sphere_distance`: `angular_distance != AngularDistance::ZERO` is an epsilon comparison, so a cosine within `1e-12` of 1 (a distance up to about `1.414e-6` radians) is returned as `0.0` | What Python sees as zero | Deliberate and user-visible since pyGPlates 1.0: Python code tests `== 0.0`, and geometries that touch (share a vertex, say) can come out a tiny non-zero distance when rounding hides the contact |
| pyGPlates threshold margin, `2 * EPSILON` on the cosine | `geometry_on_sphere_distance`: `minimum_distance()` is given the cosine of the caller's threshold less `2e-12`, and the caller's threshold is then applied exactly, in radians, to the distance returned | Whether `distance()` returns `None` | `cos(acos(c))` differs from `c` by a few ULPs, so a distance passed back as the threshold was excluded by itself about a quarter of the time; and a distance clamped to `0.0` must not exceed a threshold of `0.0`. The margin covers both. A NaN threshold raises `ValueError`; the threshold is clamped to `[0, pi]` |
| `intersect()` | `GreatCircleArc.cc` | Whether two arcs cross: four signed distances compared with zero exactly (`>= 0` on `.dval()`). An epsilon enters only once the arcs are known to cross and the caller wants the point: `cross(axis1, axis2).magSqrd() > 0` (a `real_t`, so planes within about `1e-6` radians count as the same great circle) chooses between the normalised cross product and interpolation along one arc's chord (see section 2) | The sign tests are exact so that a crossing is a crossing; only *where* near-collinear arcs cross needs care, since the cross product of nearly parallel normals is inaccurate |
| `GeometryIntersect` thickness, `s` (cosine `1 - EPSILON`, about `1.414e-6` radians) | `GeometryIntersect::Intersection::get_on_segment_start_threshold_cosine()` (`1 - EPSILON`) and `get_on_segment_start_threshold_sine()` (its sine), used as `THICKNESS_THRESHOLD_COSINE` and `THICKNESS_THRESHOLD_SINE` in `GeometryIntersect.cc`; also the default expansion of a bounding small circle (`BoundingSmallCircleBuilder::get_default_angular_expansion()`), which the bounding trees use | Two vertices whose dot product is at least the cosine coincide. A vertex whose signed distance from an arc's plane is within the sine (both comparisons exact) is *on* the plane, and touches the arc if it is also in the arc's lune. Two arcs cross only if each has its end points beyond the sine on opposite sides of the other's plane (section 4.2) | The distance that compares equal to zero, and point-in-polygon's point-on-outline threshold, so "touching" means the same in both. It must not be less than the longest a zero-length arc can be (about `1e-6` radians; it exceeds it by about `0.41e-6`), so a zero-length arc never has its end points on opposite sides of a thick plane: it has both end points on any thick plane it straddles, so it can still touch that arc. Its square (`2e-12`) exceeds `EPSILON`, which keeps the cross product of two crossing arcs' planes usable (section 4.2). The comment at `get_on_segment_start_threshold_cosine` says all three. The bounding circles are expanded by the same amount so that the bounding tree never prunes a pair of arcs close enough to touch |
| Lune membership in `minimum_distance(point, arc)` | The two dot products are compared with zero as `real_t` (epsilon) | Whether the point is inside the arc's lune (closest to the arc's interior) or to an end point | Deliberately epsilon, so that a point at the arc's rotation axis, where both dot products are zero, goes to the end-point branch rather than normalising a zero vector |
| Degenerate crossing arc | `PointInPolygon::is_crossings_arc_degenerate`: `cross(R, P).magSqrd() <= 0` as a `real_t` | Whether the test point `P` is within about `1e-6` radians of `R` or of `-R`, where the crossing arc's plane is undefined, so that crossings are counted from `R2` instead (section 3.8) | The same bound as a zero-length arc |
| Hemisphere test, `dot(v, C) > 0` exactly | `is_ring_in_open_hemisphere` (`PointInPolygon.cc`), on every vertex of a ring | Whether a ring keeps parity from `-C` (and its fan sum from `C`), or is decided by the winding rule (section 3.2) | Strict, so that a ring that passes has `-C` more than 90 degrees from every vertex: `-C` is then outside the ring, the fan from `C` is well conditioned, and the old results are reproduced bit for bit. The 1e-12 of a `real_t` comparison would gain nothing, since a vertex that close to 90 degrees passes or fails harmlessly either way |
| Tie, `1e-8` steradians | `TIE_STERADIANS` in `PointInPolygon::get_ring_winding_offset`: the fan sum less it, divided by `4 pi`, is rounded half down (`ceil(x - 0.5)`) | A ring whose fan sum is within it of an odd multiple of `2 pi` (a ring halving the sphere, such as four points on the equator) takes the side to its left as its interior, so reversing the ring swaps its interior and exterior | A ring halving the sphere has two sides equal to rounding, so some rule must break the tie deterministically; the left side is the convention of GeoJSON and S2. The value need only be far above the rounding of a fan sum and below any difference that matters: it is under half a square kilometre on Earth |
| Candidate distance, `0.01` radians | `MAXIMUM_CANDIDATE_DISTANCE_TO_OUTLINE_COSINE = cos(1e-2)` in `PointInPolygon.cc`, against each candidate's `minimum_distance()` to the outline | A candidate for `R` closer than this to the outline is tried only if no candidate further away is outside | Two reasons (the comment at the constant): a candidate's status comes from fan sums of triangles with apex at its antipode, which are ill-conditioned for a candidate at a vertex; and the filled polygons are fans from `-R` rendered in perspective from the centre of the globe with a near plane `1e-3` from it (`GLCubeSubdivision`'s default `zNear`), and a fan triangle whose edge passes within `d` of `R` has its plane within about `d` of the centre, so it would be clipped for `d` under about `1e-3`. `R2` has no such minimum: its status comes from exact crossing parity and nothing is drawn from it, so it is simply the furthest candidate in its band |
| `R2` band, 60 to 120 degrees from `R` | `calculate_second_reference_point`: a candidate with `fabs(dot(candidate, R)) > 0.5` is skipped | Which candidates may be the second reference point | The arc from `R` to `R2`, on which `R2`'s status is found, and the arc from `R2` to a point within `1e-6` of `R` or `-R`, are then 60 to 120 degrees long, so their planes are well defined. A candidate always exists, since no point is more than about 23 degrees from one (the comment there) |
| Interior-centroid size switch, bounding radius `0.01` radians | `MINIMUM_RADIUS_COSINE_FOR_FAN = cos(0.01)` in `Centroid::calculate_interior_centroid`, against the angular extent of the polygon's bounding small circle | Whether the interior centroid is the exact area moment (wider) or the fan of triangles from `C` weighted by area at their vertices' mean (narrower) | Where the two methods' errors cross (section 3.3): the fan approximation is off by about `0.3` degrees for a polygon one radian across and the exact moment loses precision on tiny polygons; at this size they agree to about `1e-10` (the comment there, from measurement) |
| Overlay vertices, by identity | `PolygonOverlay::Overlay::build_vertices`: records are grouped by the original vertex they name (section 4.2), by union-find on record indices; `is_same_position` compares coordinates exactly where the overlay asks whether an arc starts at a vertex's output position (the cone) and in clean-up (repeated positions) | Which intersection records are one point of the overlay, and so one stop on a ring; no distance is involved (section 5.2) | The records already encode the snapped topology with `GeometryIntersect`'s thickness, so a second constant would be a second meaning of "the same point"; `PointOnSphere::operator==` has a tolerance of about the thickness and is never used (section 5.3) |
| Overlay test points, threshold off | `Overlay::is_inside_other`: `is_point_in_polygon(point, speed, false)` at a piece's first interior original vertex, or at a stalled single arc's chord midpoint; `classify_anchorless_chain` likewise counts exact parities | Which side of the other polygon a piece is on | A vertex in no record is more than `h` from the other outline (or it would be a touch), about `10^10` times point-in-polygon's rounding; the inclusive threshold would call a piece just outside `Q` inside. Only a midpoint test of a *band piece* (a single arc within `2 h` of the other outline) is decided by rounding, and point-in-polygon is itself inexact within about a zero-length edge's length of one (section 5.3) |
| Midpoint margin order | `Overlay::classify_pieces` step 3: `minimum_distance(midpoint, other polygon)` as an angle, largest first | The order in which stalled single arcs are tested | Rounded (a cosine, resolving about `1.5e-8` radians) but it only orders: a piece with a margin is tested before a band piece, and two band pieces' margins tying is harmless (section 5.2) |
| Self-touch cut, `h` and `1e-12` radians | `Overlay::find_self_touch_cuts`: the other input's chain vertices within the thickness sine of the piece's own plane (in doubles, even for a segment `GreatCircleArc` calls zero length), each further along than the last by more than `ALONG_ROUNDING = 1e-12` radians; at most `MAX_CHAIN = 16` pieces | Where a stretch the two inputs share is cut on one side only (an input touching itself there, section 5.2) | `h` is the thickness the chain's records were decided with; the rounding margin keeps a vertex at the segment's end from counting as inside it one way in doubles and the other in the prototype's exact arithmetic |
| An output ring's arc passing a vertex, `h` and `1e-12` radians | `Overlay::FirstArc` (`get_first_arc`), in `Overlay::split_walked_rings` (a ring twisted at a chain) and `Overlay::join_difference_rings` (a wall walked as a spike): an overlay vertex's output position within the thickness sine of the plane of the ring's arc (in doubles, as the self-touch cut's) and its projection inside the arc by more than `ALONG_ROUNDING = 1e-12` radians, the arc not ending at that vertex by identity. For the twist also the ring's first points beyond the thickness sine of that plane, each way from the vertex, and their sides of the plane of the ring's other arc at the first arc's start, again beyond the thickness sine | Whether an arc of an output ring passes an overlay vertex it has no stop at (an input touching itself, which `GeometryIntersect` never records): whether the ring crosses itself there and is cut, and whether a common piece between the two differences is joined across in the symmetric difference (section 5.2) | The self-touch cut's thickness and rounding margin, asked of an output ring: the crossing `GeometryIntersect` would have recorded had it intersected the ring with itself. A point within the thickness where it would decide, a ring that runs within the thickness past the arc's end, and a touch that is not a crossing, cut nothing |
| Bundle cone | `Overlay::measure_ray`: `0` when the arc starts at the vertex's output position on its segment's great circle, `pi` when the arc is no longer than its offset, else `atan2(delta, L - delta)`; the offset counts the start's and far end's distances off the great circle only for a stop at a touch record inside a segment or a self-touch cut (decided by the stop's type, not a threshold) | Which rays are anchored and which float at a vertex of six or more ends where both inputs pass more than once, and the admissible orders (section 5.2) | Threshold-free: the bound on how far the arc's direction seen from the output position can differ from its tangent; `h` enters only through the data. A threshold of `1e-12` for the offsets was wrong at crossings, whose stored position lies up to about `2.4e-11` radians off its planes in a dense shared edge |
| Anchorless reference point, `1e-6` | `Overlay::classify_anchorless_chain`: each input's outside point is replaced by its second reference point when within that of the great circle of the input's own segment along the chain (as a sine), or of the input's test point on it or its antipode | Which reference point the crossing parities are counted from (section 5.2) | The crossing arc must meet that great circle only at the test point, and must have a defined plane: the degenerate crossing arc bound of section 3.8 |
| Hemisphere check and grouping margins, `10 h` and `max(10 h, 2 x the widest record cluster)` | `HEMISPHERE_CHECK_MARGIN` in `PolygonOverlay.cc`. `Overlay::find_reference_point`: the reference point of the hemisphere check (`extract`) and of a whole answer's check (`check_whole_result`) must be farther than `10 h` from both inputs' outlines; the whole answer's candidates beside the subject's edges are `20 h` off them. `Overlay::extract`: the grouping margin, for `is_ring_inside_ring`'s point that decides containment, for `do_rings_cross` (outlines farther apart than it do not cross; a point within it decides nothing) and for `does_ring_cross_itself`. `PROBE_DISTANCE_IN_MARGINS = 3`: both crossing tests look three margins along the edges of a vertex within the margin, and `does_ring_cross_itself` asks for arms two margins long | Whether an output's rings, or a whole answer, read as the output or its complement; which ring contains which, which rings cross, and which ring crosses itself (section 5.2) | The point tests are exact with the threshold off, so they need a margin above the band; output rings can cross within an overlay vertex by up to its record cluster's width (a margin of `s` alone misgrouped a hole in 5 of 119,198 generated cases) |
| Sliver widths, `10 h`, `h` and none | `GeometryCookieCutter::get_default_sliver_min_width()` (`10 h`, about 90 metres; pyGPlates' `sliver_width_threshold_radians`, `None` for the default and zero for off); `polygon_on_sphere_partition` (`h`); `PolygonPartitioner::create` (none) | Which output rings, by mean width `2 * area / perimeter`, the filter drops (section 5.5) | Measured on a global coastline file cut by a topology model: below `h` the overlay cannot tell the outlines apart, up to `10 h` the rings are artefacts of an outline digitised along a plate boundary, and at `100 h` real geography begins |
| `get_mean_width` apex, `magSqrd()` within `real_t`'s tolerance of zero (about `1e-12`) | `PolygonOverlay::get_mean_width`: the fan's apex is the ring's vector area, normalised, unless its `magSqrd()` is within `real_t`'s tolerance of zero (a vector area shorter than about `1e-6`, a ring under about `5e-7` steradians, which is also where `get_normalisation` refuses it); then the vertices' mean direction, and if that cancels too the first vertex | Which apex the Van Oosterom and Strackee fan is summed from (section 5.5) | The vertices' mean lies in the plane of a ring on one great circle, where every triangle is degenerate and the sum is not the area (a 36-point equator ring measured `0`); the vector area is that plane's normal |
| `DateLineWrapper` thick plane and pole caps, cosine `1 - 1e-9`, about `4.5e-5` radians | `EPSILON_THICK_PLANE_COSINE` and its sine `EPSILON_THICK_PLANE_SINE` in `DateLineWrapper.cc`. `classify_vertex`: a vertex within the sine of the dateline's plane is on it, and within the cosine of a pole is on the pole. `intersect_line_segment`: the south pole within the sine of a segment's plane means the segment's great circle passes the poles, and two exact sign tests per pole then decide whether the segment does | Which vertices are on the dateline or at a pole, and so which segments are intersected at all (section 5.8) | The comment says the epsilon is based on a cosine because that has the least accuracy for small angles, and that `1 - 1e-9` is a displacement of about `4.5e-5`. It is about 32 times `s` (and `GeometryIntersect`'s thickness, which is `s`), and is not derived from `EPSILON`. The code gives no reason for the size |
| `DateLineWrapper`'s other epsilons: `1e-4`, and `real_t` zero tests | `does_line_segment_on_dateline_plane_cross_north_pole`: for a segment on the thick plane running between the arc and longitude zero, the dot product of the cross product of its end points (not normalised, so small for a short segment) with the dateline plane's normal, compared with `1e-4`; the sign, together with which end is on the arc, picks the pole, and below `1e-4` (the comment says the ends are then too close together) the sign of the end point's `z` decides. `calculate_intersection`: the chord interpolation's denominator and the interpolated vector's `magSqrd()` compared with zero as `real_t`; `intersect_line_segment` and the pole cases of `add_line_segment`: `is_zero_length()`. `output_non_intersecting_polygon_rings`: `minimum_distance()` of two solid polygons compared with `AngularDistance::ZERO`, an epsilon comparison of cosines (`boost::equivalent` over `real_t`) | Which pole such a segment passes; no intersection for a segment whose ends are both too near the plane or nearly antipodal; a zero-length segment straddling the plane intersects at its start point, and one at a pole gets the ratio `0`; a non-intersecting ring within about `1.4e-6` radians of an output exterior ring becomes its interior ring (section 5.8) | The first is called lenient because both paths are covered; the comments say the next two cannot happen given the thick-plane precondition; the last is the epsilon in `AngularDistance::operator==`, not a choice made here |

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
rejection is conservative: the bounding small circles are expanded by `GeometryIntersect`'s
thickness (`s`), far above the rounding of their arithmetic.

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

The [point-in-polygon overview](point-in-polygon-overview.md) explains the ideas of this section
with pictures and worked examples; read it first for the concepts, and this section for what the
code does exactly.

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
  as `INTERSECTING_BOUNDS` when the fill reaches past it;
- `is_outside_by_bounds` in `PolygonPartitioner.cc` (section 5.1), which answers *outside* without
  an overlay when the two bounding circles are disjoint, only if each polygon's interior lies within
  its own circle.

**What follows automatically**, by calling point-in-polygon or the fans: `DateLineWrapper` decides
whether a pole is inside a polygon with `is_point_in_polygon`
(`generate_entry_exit_flags_for_dateline_polygon`); the map view fills the wrapped, projected
polygons by stencil parity (`GLFilledPolygonsMapView`, `GL_INVERT`), so it agrees with the globe;
and `PolygonMesh` keeps a triangle only if `is_point_in_polygon` accepts its centroid.
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
to revisit once a polygon can be overlaid with itself (section 7): the overlay of section 5
intersects two polygons, never one with itself. Each ring's own
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
the overlay's tests of a piece's vertices and midpoints (`PolygonOverlay::Overlay::is_inside_other`,
section 5.2: a vertex in no intersection record is already more than the threshold from the
outline, and the pieces must divide the sphere exactly), its anchorless parities and its grouping
and hemisphere check, and the solid-interior tests of the distance code (section 2). It is also
off where the point is known to be far from the outline and only parity is wanted: the status
queries for `-R` and `R2` (sections 3.7 and 3.8) and the query at the antipode of a circle's centre
in `is_outside_bounding_small_circle_outside_polygon` (section 3.3).

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

Four pieces of code find where great circle arcs meet. Only one of them, `GeometryIntersect`, is
shared (it and the distance code also share one helper, `interpolate_plane_crossing`, section 1):

- **`GeometryIntersect`** (`GeometryIntersect.{h,cc}`, section 4.2) finds every intersection between
  two polylines or polygons. Topology resolving uses it: `TopologicalIntersections`
  (`src/app-logic/TopologyIntersections.cc`) intersects each section's polyline with its
  neighbours', and `ResolvedSubSegmentRangeInSection.cc` builds sub-segment ranges from the
  intersections and reuses the threshold. So does the polygon overlay (`PolygonOverlay`, section
  5.2), which partitioning is built on, and `GenerateVelocityDomainCitcoms.cc`, which takes the
  first record of two two-point polylines. Its threshold is
  also the default expansion of a bounding small circle, which the bounding trees use (section 1).
- **`GreatCircleArc::intersect`** (`GreatCircleArc.cc`) decides whether two arcs cross, and where.
  It serves the distance code only: `minimum_distance(arc, arc)` (section 2, section 4.3).
- **`PointInPolygon`** intersects an edge with the crossing arc's plane
  (`if_plane_divides_gca_get_intersection_of_gca_and_plane`, section 3.4). Its sign tests are
  exact, with no thickness: parity is exact, and the on-outline test is a separate distance pass
  (section 3.5) with the threshold that is `GeometryIntersect`'s thickness (section 4.4), so a
  vertex on the outline here is a contact there.
- **`DateLineWrapper`** (`DateLineWrapper.{h,cc}`) clips polylines and polygons to the dateline
  (section 5.8). It finds the crossings itself, against a thick dateline plane, and does not use
  `GeometryIntersect`.

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
meets there. A vertex shared by two rings of one polygon is reported once per ring, since each
ring's segments are segments of their own: a hole touching its exterior ring at a vertex gives,
against a vertex of the other geometry there, two intersections, which is what splitting both
rings needs. A polyline's last vertex has no next segment, so it is reported as the start of a
fictitious segment one past the last, whose index is the number of segments. A polygon ring wraps
round, so it never needs one (`POLYGON_NEEDS_NO_LAST_SEGMENT_INDEX`).

**Overlap is not a type.** Two arcs lying along each other report the start points that touch the
other arc, and each vertex inside a stretch where the geometries run together is reported as
touching the other geometry. The header comment gives examples. So no single intersection says
whether the geometries cross there, touch without crossing, or start or stop running together.
A consumer that needs to know derives it from the records (a stub, `GeometryCrossing`, was begun
to do it inside the primitive, and was deleted unused).

**The test for one pair of arcs** (`intersect_segments`) has four steps, in this order:

1. **Coincident vertices.** All four pairs of end points are compared, by dot product, with the
   thickness cosine (section 1). A start point on a start point gives an SS intersection, placed
   at the first geometry's vertex. At a polyline's last segment, its end point is compared in the
   same way, as the start of the fictitious segment; an SS there with the second geometry's last
   vertex is placed at that vertex.
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
   other arc. It is ordered along the other arc by its projection onto that arc's great circle,
   not by the angle to the start point itself, which near the arc's start would sort it after a
   crossing lying further along. A vertex inside a corner of the other geometry, within the
   thickness of both of the corner's arcs but farther than the thickness from the corner, touches
   both and gives one intersection per arc, both at that vertex; at an acute corner the wedge
   where this happens reaches the thickness divided by the sine of half the corner's angle (section
   4.4). End points are not tested: the next segment's start point covers them, except at a
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
product of the two planes, its sign chosen by the orientations from step 3. A crossing needs each
arc's end points beyond the thickness from the other arc's plane, on opposite sides of it, so the
sine of the angle between the planes exceeds the thickness. The square of the thickness exceeds
`EPSILON`, the bound below which the cross product is zero as a `real_t`, so the cross product is
always taken, and the point is accurate to about `2e-10` radians. Interpolation along a chord
(`interpolate_plane_crossing`, along the first arc, or the second if the first is near a half
circle) remains only as a fallback for rounding; if both arcs are near half circles it raises an
assertion. The point is not checked against either arc's lune, and does not need to be: the thick
sign tests already place each arc across the other's great circle, and the sine of the angle
between the planes exceeds the thickness, so the cross product is well conditioned.

**Pruning.** Each polyline and polygon has a `PolyGreatCircleArcBoundingTree`: a binary tree of
bounding small circles over runs of consecutive arcs, with at most 8 arcs in a leaf by default.
`intersect_bounding_tree_nodes` descends both trees together. It skips any pair of nodes whose
circles do not intersect, and otherwise splits the larger node first. At a pair of leaves it tests
every pair of arcs. A polygon gives either its whole tree (`get_bounding_tree()`) or its exterior
ring's tree (`get_exterior_ring_bounding_tree()`).

The pruning is sound because the circles are expanded by the thickness. A leaf's circle is
centred on the centroid of its arcs, with a radius reaching the farthest point of any of them
(`BoundingSmallCircleBuilder::add(const GreatCircleArc &)`, through `maximum_distance(point,
arc)`), then expanded by the builder's default expansion, which is `GeometryIntersect`'s thickness
(section 1); an internal node's circle is the optimal circle enclosing its two children's
expanded circles (`create_optimal_bounding_small_circle`). Two nodes are skipped only if the angle
between their centres is not less than the sum of their radii (`intersect(BoundingSmallCircle,
BoundingSmallCircle)`, an exact comparison of cosines). If a point of an arc in one node lies
within the thickness of a point of an arc in the other, the centres are at most the two unexpanded
radii plus the thickness apart, which is less than the sum of the expanded radii by the thickness,
a margin far above the rounding of the comparison (the cosine resolution of section 1). A crossing
reported by the thick test is an exact crossing (end points strictly beyond the thickness on
opposite sides imply opposite sides), so the arcs share a point and the circles overlap outright.
Hence no pair of arcs that can produce an intersection is pruned, and because the expansion is
derived from the same cosine, this follows the thickness if it changes.

### 4.3 `GreatCircleArc::intersect` compared

The distance code's `intersect(arc1, arc2)` answers a narrower question, in a different way:

- **The decision is exact.** It compares the four signed distances with zero (`>= 0` on `.dval()`),
  with no thick plane, so a crossing is a crossing. `GeometryIntersect` decides with thick planes,
  and so can report arcs that touch, or nearly touch, as well.
- **It returns only whether the arcs meet.** There is no touch type and no overlap, no position
  along either arc, and no notion of a shared vertex reported once.
- **Where they cross** is the cross product, or, for near-collinear arcs, interpolation along the
  chord of the *better-conditioned* arc: the one whose end points differ most in signed distance.
  The point must then lie in both arcs' lunes (`is_in_arc_lune`). Otherwise, or if neither arc can
  be interpolated, it uses the end point of either arc that is closest to the other arc (section
  2). `GeometryIntersect` needs none of this: its thick test only reports a crossing whose planes
  are at least the thickness apart, so its cross product is always usable (section 4.2). The
  near-collinear crossings `intersect()` handles are, for `GeometryIntersect`, touches.
- **A zero-length arc** meets the other arc if its start point lies on it (`lies_on_gca`, an
  epsilon test). The arc-to-arc distance does not call `intersect()` for one.

### 4.4 Tolerances

**One thickness, and it is `s`.** `get_on_segment_start_threshold_cosine()` returns `1 - EPSILON`,
the cosine of point-in-polygon's point-on-outline threshold (section 1), and the thickness sine is
derived from it. So one distance means "the same point" for distance, point-in-polygon and
intersection: two vertices coincide when they compare equal as `real_t`s, a vertex within `s` of an
arc's plane and inside its lune touches the arc, and a point within `s` of a polygon's outline is on
it. A vertex that point-in-polygon calls on the outline is therefore an intersection here, and a
geometry with no intersection has no vertex within `s` of the other geometry's arcs, so whether it
is inside can be decided by exact parity, threshold off, without disagreeing with the inclusive
query.

The thickness must also satisfy two inequalities, which the comment at
`get_on_segment_start_threshold_cosine` states and the unit tests pin (section 6):

- **It is at least the longest a zero-length arc can be** (about `1e-6` radians; `s` exceeds it by
  about `0.41e-6`). A zero-length arc has no plane, so it cannot be crossed, only touched at its
  vertices; if its two end points lay on opposite sides of another arc's thick plane, that arc
  could pass between them undetected. A zero-length arc straddling a thick plane has both end
  points on it, so both touch.
- **Its square (`2e-12`) exceeds `EPSILON`.** A crossing needs the sine of the angle between the
  two planes to exceed the thickness (section 4.2), so the cross product of the planes' normals is
  never zero as a `real_t`, and the interpolation fallback is never reached.

The bounding circles of the trees are expanded by the same thickness
(`BoundingSmallCircleBuilder::get_default_angular_expansion()`), so a pruned pair of arcs cannot
touch.

**What the thickness does to contacts.**

- **A crossing needs both end points of each arc beyond the thickness.** A short segment crossing
  an arc with both its end points within `s` of the arc's plane gives two touches and no crossing.
  At a right angle that means a segment shorter than `2s` (about 18 metres); at an angle `phi` to
  the arc, one shorter than `2s / sin(phi)`, which has no bound as the angle shrinks.
- **A vertex inside a corner touches both of the corner's arcs** (section 4.2), so one contact is
  two intersections at the same vertex. Only the coincidence disc about the corner counts as the
  corner (step 1 of the test); beyond it the two thick planes still overlap, and a vertex there is
  on both. At an acute corner they stand for the two crossings that a
  segment passing just beyond the corner would make, collapsed onto the vertex by the thickness.
  A consumer must keep both: `Graph`'s comment says which original vertices each intersection
  names, so that intersections at one vertex can be treated as one contact.
- **A touch is placed at the touching vertex**, up to `s` off the other arc. Where the other arc
  would have been crossed nearby, the contact moves from the crossing to the vertex, by up to
  `s / sin(angle)` between the two arcs (and its projection along the other arc by up to
  `s / tan(angle)`). Measured when the thickness was raised from the zero-length
  bound to `s`: on the Müller et al. 2022 model, resolved every 5 Myr from 1000 Ma, 3 of 3,799
  resolved topologies changed, each a junction moving from a computed crossing onto an original
  vertex of a boundary section, by 1.01 to 1.08 `s`; no topology and no sub-segment appeared or
  disappeared.

**Ordering.** The angle along a segment is an `AngularDistance` made from a cosine, so it resolves
only about `1.5e-8` radians near the segment's start (section 1), and two intersections within
that of each other there compare equal. Two touches by one vertex (a corner) tie exactly along the
touching geometry. Two crossings at one point (a self-crossing of one geometry lying on the other)
are computed separately, and tie only when their positions agree to the key's resolution, which
is usual at ordinary angles; otherwise rounding orders them. The
sort is stable (`std::stable_sort`), so tied intersections keep the order in which they were
found. A consumer must accept them in either order. A touch is keyed by its vertex's projection
onto the other arc's great circle, so that it sorts by where along the arc it is, which is what a
crossing's key measures too. The key only orders. Topology resolving sorts its intersections by
the same key (`ResolvedSubSegmentRangeInSection::Intersection::operator<`) and takes the ratio at
which an intersection divides its segment from the position, not the angle
(`get_interpolate_ratio_in_segment`), so a change of key moves no point.

**What is exact.** The two lune tests are exact sign tests on the dot products (`>= 0` and `<= 0`
on `.dval()`). Every other comparison is an exact `double` comparison against the thickness (the
cosine for vertices, the sine for planes), including the orientation test of a crossing, which uses
the thick side classifications. The position of a touch or a shared vertex is an
original vertex, exactly; a crossing's position is accurate to about `2e-10` radians.

A vertex is decided the same way as the end of one segment and as the start of the next, bit for
bit: the two segments hold copies of the same vertex, and the other geometry's segment holds the
same start point and the same cached plane, so the vertex test (one dot product of the two
points) and the plane test (one dot product of the point and the plane) are the same `double`
in both pairs. That is what makes the "complementary tests" of the comment in
`intersect_segments` complementary, and what lets end points go untested.

The result is symmetric. `intersect(A, B)` and `intersect(B, A)` report the same intersections
with the types mirrored (SL for LS) and the two orders swapped; the sign tests, the lune tests
and the crossing's cross product are symmetric in the two arcs, so positions and angles are
equal, except that an SS is placed at the first geometry's vertex (section 4.2), so its position
can differ between the two orders by up to the thickness.

**Near-antipodal arcs.** `GreatCircleArc::create` refuses end points whose dot product is `-1`
as a `real_t` (`unit_vectors_are_antiparallel`), so no arc is within about `1.414e-6` radians of
a half circle, the cross product of any arc's end points has a magnitude of at least about
`1.4e-6`, and its normalised rotation axis, the arc's plane, is accurate to about `1e-10`
radians: far below the thickness, so the thick classification and the lune test are sound for
every constructible arc. The only other place a near-half-circle arc matters is the chord
interpolation's guard (section 1), and that is the unreachable fallback.

### 4.5 Rejected alternatives

**Keeping the thickness at the zero-length bound** (`1.0000444e-6` radians, its previous value). A
vertex between that and `s` from an arc was on the outline for point-in-polygon and no intersection
for `GeometryIntersect`, so partitioning's exact classification of a geometry without intersections
disagreed with the inclusive query, and the system had two meanings of "the same point". Raising it
changed topology resolving slightly and measurably (section 4.4), which is the cost accepted.

**A coincidence radius larger than the thickness**, about `sqrt(2)` times it, so that a vertex in
a right-angled corner's wedge would coincide with the corner instead of touching both arcs. It
needs two constants instead of one, and acute corners keep the wedge anyway, out to the thickness
divided by the sine of half the angle.

**Reporting a corner contact once**, by dropping one of the two touches at a vertex. At an acute
corner the two stand for two crossings; dropping one leaves the other geometry's corner tip joined
to the rest of its ring, and a consumer that classifies a stretch of ring by one of its vertices
then puts the whole ring on the wrong side. Measured with a prototype of the partitioning overlay:
dropping the first-sorted touch gave wrong output in 15 of 25 corner cases, and no consistency
check noticed. (Dropping the other one happened to give none, which is not evidence that it is
safe: both stand for real crossings.) The touches are kept, and grouped by the vertex they name.

**Ordering a touch by the angle to its vertex** (the previous key). The vertex is up to the
thickness off the arc, so near the arc's start the angle to it exceeds its distance along the arc,
and a touch could sort after a crossing lying further along when the two were within about
`sqrt(2)` thicknesses of each other: a vertex `3.0e-6` along and `0.99e-6` off sorted after a
crossing at `3.15e-6`.

**Porting `GreatCircleArc::intersect`'s refinements, or one primitive for both callers.** The
refinements (interpolating along the better-conditioned arc, the lune check, the end-point fallback,
section 4.3) handle crossings between nearly parallel arcs, which `GeometryIntersect` never reports:
it reports them as touches. The two decisions also suit their callers: the distance code needs a
crossing to be a crossing, exactly, while topology resolving and partitioning need touches, shared
vertices and overlaps reported, within a tolerance. Deciding the distance code's crossings with
thick planes would make `minimum_distance(arc, arc)`, which returns zero when `intersect()` says
the arcs meet, return zero for arcs passing within the thickness of each other, a change for every
C++ caller of the distance code. The chord interpolation that `GeometryIntersect`
had for nearly parallel planes was unreachable (section 4.4). Its body is now the shared
`interpolate_plane_crossing` in `GreatCircleArc.{h,cc}`: the distance code's main path for
near-collinear arcs, and in `GeometryIntersect` a fallback for rounding only.

**Classifying contacts inside the primitive**: crossing, touch, or the start or end of an
overlap, as Foster, Hormann and Popa's labels do. It would give the primitive a meaning of
"crossing" that depends on the thickness, and change the contract topology resolving relies on,
for one consumer. A `GeometryCrossing` stub was begun for it and never finished. A consumer derives
what it needs from the records: their types, segment indices and order along both geometries.

**Placing an SS intersection at the first geometry's vertex everywhere**, including at the second
geometry's last vertex, where it is now placed at that vertex (section 4.2). It would make the rule
uniform, but would move topology junctions at a section's last vertex by up to the thickness, for
no consumer that needs it. The header documents the exception instead.

## 5. Partitioning

Partitioning cuts a geometry by a polygon into the part inside it and the part outside. In this
section `P` is the geometry being cut (the *subject*: a polygon or a polyline), `Q` the polygon
cutting it (the *clip*, or partitioning polygon), and `h` the thickness of `GeometryIntersect`,
which is `s` (section 4.4): about `1.414e-6` radians, 9 metres on the Earth.

The [polygon overlay overview](polygon-overlay-overview.md) explains the overlay's generic case
with pictures and worked examples (pieces, classes, the keep table, pairing, the split and the
grouping); read it first for the concepts. This section is what the code does exactly, and most of
it is what the overview leaves out: the cases where several records lie at or near one point.

### 5.1 The layers

- **`GeometryCookieCutter`** (`src/app-logic/GeometryCookieCutter.{h,cc}`) holds one
  `PolygonPartitioner` per partitioning polygon, which can be a resolved plate boundary, a resolved
  network's boundary with its rigid blocks as interior rings (preceded by each rigid block as a
  polygon of its own, `add_partitioning_resolved_topological_network`), or a reconstructed static
  polygon. The polygons are kept in the order added, networks, then boundaries, then static
  polygons, and each group can be stable-sorted by plate ID or by area, highest first (Assign Plate
  IDs sorts by plate ID); or, when a caller asks not to group them (pyGPlates'
  `SortPartitioningPlates.by_plate_id` and `by_plate_area`), the whole list is sorted as one. It
  passes every partitioner the same point-in-polygon speed and the same sliver width (section 5.5),
  `get_default_sliver_min_width()` by default.
- **`PolygonPartitioner`** (`src/maths/PolygonPartitioner.{h,cc}`) partitions one geometry by one
  polygon: points and multi-points by point-in-polygon (`partition_point`, threshold on, a point on
  the outline inside); polylines and polygons through the overlay below, each call making one
  `Overlay` (`overlay_polygon`, `overlay_polyline` in `PolygonPartitioner.cc`). Before any overlay,
  `is_outside_by_bounds` answers *outside* without one when the two bounding small circles are
  disjoint, provided each polygon's interior lies within its own circle
  (`is_outside_bounding_small_circle_outside_polygon`, section 3.3; a polyline's outline is all
  there is). The cascade tests every geometry against every plate, most of them far apart, so this
  is where most calls end.
- **`PolygonOverlay`** (`src/maths/PolygonOverlay.{h,cc}`, namespace
  `GPlatesMaths::PolygonOverlay`) is the core: the overlay of two polygons with holes, or of a
  polyline with a polygon, every Boolean operation on it, and provenance for every output point
  (section 5.2). One `Overlay` object is one call, holding references to its two inputs, the
  `GeometryIntersect::Graph` and every working vector; nothing is cached on the inputs beyond what
  `PolygonOnSphere` already caches (its point-in-polygon tester, bounds and outside point).

The other callers of `PolygonPartitioner` are `PolygonOnSphere.partition()` in pyGPlates
(`src/api/PyGeometriesOnSphere.cc`, `polygon_on_sphere_partition`) and
`GLReconstructedStaticPolygonMeshes::find_present_day_polygon_mesh_node_intersections`, which uses
only the inside/outside/intersecting answer for a quad-tree node's bounding polygon. That overload
serves a static polygon drawn as a fan, and the fan fallback is under `#if 0` (section 3.3), so
nothing reaches it today: a meshed polygon's nodes are found from its triangles. CitcomS mesh
generation (`GenerateVelocityDomainCitcoms.cc`), which needs one crossing of two two-point
polylines, takes `GeometryIntersect::intersect`'s first record directly.

**The Boolean methods.** pyGPlates' `PolygonOnSphere.intersection()`, `union()`, `difference()`
and `symmetric_difference()` (`src/api/PyGeometriesOnSphere.cc`) each take another polygon and the
sliver width of section 5.5 (`sliver_width_threshold_radians`, as `partition()`'s), and return a
list of polygons, empty when the result has no area. They are named as Python's `set` methods and
Shapely's are. There are no `&`, `|`, `-` and `^` operators: a result is a list of polygons and not
a polygon (there is no multi-polygon geometry), so an operator's result could not be an operand.
`intersection` and `difference` are partitioning's two outputs: they cut this
polygon with a `PolygonPartitioner` of the other (`polygon_on_sphere_intersection`,
`polygon_on_sphere_difference`, through `partition_polygon`), so they return exactly what
`other.partition(polygon)` puts inside and outside, under the result contract below: this polygon
itself where it is whole (the same `PolygonOnSphere`; in Python a new wrapper of it, equal to the
input and not identical to it). `union` and `symmetric_difference` call the overlay
directly, through `PolygonOverlay::extract_polygons` (one `Overlay`, classified and extracted, its
output polygons made `PolygonOnSphere`s by `PolygonOverlay::create_polygon`, which the partitioner
uses too); there is no bounding-circle shortcut before it, and a whole input comes back as the
overlay's copy of it, its rings oriented with the output on their left. Extracting the intersection
and the difference from the overlay too was rejected: with the sliver filter on, an intersection
dropped as a sliver leaves the extracted difference with a notch where the sliver was, while the
partitioner returns the subject whole outside, so the two methods would disagree with each other
and with `partition()`. The symmetric difference as the two differences side by side is in
section 5.7. No method has a coverage form (section 5.7).

**The cascade.** `GeometryCookieCutter::partition` gives the input to the first partitioning
polygon; what lies outside it goes to the second, and so on, and whatever is left after the last is
outside all of them. A piece belongs to the first polygon that contains it, which is how overlapping
plates are resolved; so does a point on a shared boundary (within `s`), and a point on a rigid
block's outline belongs to the network when the sort puts the network first (a hole's outline is
inside its polygon). Since the pieces are polygons with holes, plate `N + 1` cuts pieces whose edges
lie along the boundary it shares with plate `N`: the common-chain case of section 5.2 is the
cascade's normal case. A network's rigid blocks are holes of the network's partitioning polygon, so
the network never claims a block's area, in whichever order the sort leaves them.

Feeding rounded pieces back in is safe because of the thickness. Rounding an intersection point can
in general put it on the wrong side of the edge it was computed on, and so make a crossing that is
not there when the piece meets that edge again (Hobby 1999, on segment intersection with
finite-precision output). Here a computed crossing lies within rounding of both its planes (at most
about `2.4e-11` radians in a dense shared edge, section 5.2), far inside the thick plane, so where
the piece meets that edge again (the next plate's copy of a shared boundary) the vertex is a touch
on it, never a crossing beside it. Nor does anything accumulate from step to step: each cut is an
overlay of its own, which sees the previous output's vertices as ordinary vertices. Measured in the
prototype over 157 cascade steps with shared edges jittered by up to `2 h`, no identity group
joined two records at any step, the widest cluster of records stayed between `1.5 h` and `1.8 h`
from the first step to the fourth (the jitter's size, with no trend), and no step failed.

**The result contract** (`PolygonPartitioner::Result`). `GEOMETRY_INSIDE` means the difference (`P`
less `Q`) is empty, `GEOMETRY_OUTSIDE` that the intersection is, and `GEOMETRY_INTERSECTING` that
both have a ring, each judged after the overlay's clean-up and the sliver filter
(`overlay_polygon`). The classes alone do not give it (`Overlay::classify`'s own result can be
intersecting where an output comes out empty): a kept piece can bound nothing once cleaned up, even
one with a vertex test of its own, such as a tip just over the thickness long that leaves an overlay
vertex and comes back to it, a ring of two positions. So a geometry that only touches the
partitioning polygon, from either side, or
runs along its outline, or is identical to it, comes back whole, as the input object itself, with
`INSIDE` or `OUTSIDE`; pieces are made only when both sides have one. A multi-point wholly on one
side is returned itself too. A polyline is `INSIDE` when no run is outside and some run is inside,
else `OUTSIDE` (`overlay_polyline`; a run at a single stored position is dropped first, as a
collapsed ring is). The result is the same with or without output lists: the result-only path (no
lists, as the cookie cutter's plate lookup and the raster meshes' unreached path call it) makes the same overlay and the same extraction and only skips
building the `PolygonOnSphere`s, and raises the same exception. An earlier design stopped early
after the vertex tests when a subject piece was on each side, and answered `INTERSECTING` where the
overlay would have raised, so one polygon gave an error with lists and an answer without; the
developer chose one contract. When the sliver filter empties both outputs (a subject thinner than
the width, cut across), the subject is whole on the side that lost the larger area, and outside on
an exact tie (as when nothing was dropped).

Deciding the result from the records alone (any record means intersecting, the old rule) was
rejected: a touch from one side would split a geometry at a boundary it only touches, and the
cascade would re-split it at every shared boundary.

### 5.2 The overlay

`Overlay::Overlay` intersects every ring of `P` with every ring of `Q`
(`GeometryIntersect::intersect`, all rings, the subject first), builds the structures below
(`build`), and `classify()` then classifies every piece and checks and repairs the result;
`extract(operation)` links the pieces an operation keeps into polygons with holes, and
`extract_polylines` gives a polyline subject's runs. Partitioning's inside is
`extract(INTERSECTION)` and its outside `extract(DIFFERENCE)`, on one classification. Each
decision is one of three kinds: **exact** (a sign test, right for its own inputs), **rounded** (a
measured quantity or a tolerance, where a flip must be harmless or caught), or **combinatorial** (no
geometry at all); the design's rule is that nothing rounded decides where the exact or the
combinatorial would do.

**Records and vertices.** The subject goes first in `intersect()` because an SS record is placed at
the first geometry's vertex (section 4.2), which is also the overlay's output position for a vertex
holding a subject vertex, so the two agree. An overlay *vertex* is the set of records that name a
common original vertex (section 4.2: an SS names both segments' start vertices, an SL the subject's,
an LS the clip's, an LL none), closed transitively: union-find over record indices
(`build_vertices`), keyed by (side, global segment index), so a corner's two touches of one subject
vertex are one vertex, and two LL records at one point (a twist of `P` lying on `Q`) are two
vertices by design. Grouping is by *identity, never by distance*: no constant is involved, and the
records already encode the snapped topology. A vertex's output position is the subject vertex it
holds if any (so the subject's vertices stay exact, for scalar coverages), else the clip's, else
the first record's position. A *chained* group, several subject vertices within one group (dense
shared edges, where a boundary is digitised along another with vertices under `h` apart), outputs
one position; its extent is set by the data and has no fixed bound (`28.8 h` for 65 records in one
measured case), which is why the stages below are built to tolerate rays leaving a vertex from off
its output position.

**Stops and pieces.** Each ring (`RingView`: a polygon's exterior ring, each interior ring, or the
whole polyline) is walked once (`build_stops_and_pieces`) into *stops* in ring order: at each ring
vertex either one `RECORD` stop holding every record at that vertex (they all name it, so identity
makes them one vertex; the code asserts it) or an `ORIGINAL_VERTEX` stop (a polyline's first or last
vertex in no record is a `DANGLING_END`); then the records inside the segment leaving it, in
`GeometryIntersect`'s order along it, each a `RECORD` stop with the record's position and its angle
fraction along the segment (`ratio_along_segment`, by projection onto the segment's great circle,
as the ordering key is). A *piece* is the stretch between consecutive record stops (cyclically for a
ring; a ring with one record stop has one piece all the way round to itself); a ring with no record
is one *closed* piece; a polyline's pieces also end at its dangling ends. A piece with no interior
original vertex whose two stops lie in one segment is a *single arc*
(`Piece::single_arc_segment`). Two records of different vertices tied along a segment (the ordering
key resolves only about `1.5e-8` radians, section 4.4) come in either order; the piece between them
is then a zero-length single arc whose class is deduced, and either order is a valid output that
differs only in ring structure.

**The loop rule and the spike rule** (`apply_loop_and_spike_rules`), both combinatorial. A single
arc from a vertex back to itself lies within the vertex's identity group and bounds nothing: it is
*internal*, not classified, not a ray and not output. A loop *with* an interior original vertex
(the tip `x, v, x` of an acute corner's wedge, section 4.4) is real geometry and stays. Two
consecutive single arcs of a polygon ring from vertex `A` to `B` and straight back are both
internal (the hairpin bounds no area, whatever its length), but only if the other input has at most
two ends at the tip `B`: with two passages of the other input there, the tip is a self-touch of that
input, the hairpin can lie between its passages, and removing it would remove a crossing. After
both rules each vertex's ends drop the internal pieces', and the count of ends at a vertex stays
even for each polygon input (each pass of a ring arrives and leaves). Without the loop rule two SS
records at one subject vertex make a loop arc with no test point and no usable direction: in the
prototype, grouping by identity without this rule failed every case.

**Common pairs** (`find_common_pairs`). A subject single arc and a clip single arc between the same
two distinct vertices run together within the thickness (about `h / cos(L / 2)` for arcs of
length `L`, which is `1.4 h` at 90 degrees and has no bound as `L` nears a half circle, where two
arcs between the same nearly antipodal end points can lie on quite different great circles; nothing
tests the length, and real segments are far shorter): an original vertex of either inside the
stretch would itself be a record. They are a *common pair*, one piece of each, and leave each vertex
as one ray. Any other mix between one pair of vertices (two subject arcs and one clip arc, say)
pairs nothing and is counted (`Diagnostics::common_ambiguous`). A run of common pairs through
vertices whose every ray is common is a *chain*. The pairing needs no direction, and neither does
the linking: GPlates never normalises ring orientation, so a clip piece runs either way relative to
the subject pieces it joins.

**Self-touch cuts** (`find_self_touch_cuts`). `GeometryIntersect` never intersects an input with
itself, so where an input touches itself on a stretch it shares with the other input (a hole's tip
on its own exterior's edge, which the other input runs along), the other input's segment is cut
there (a record names the tip) and the touched segment is not. Its one piece then faces a chain of
the other input's pieces, nothing pairs, all of them are band pieces (below) with arbitrary classes,
and the sector walk could deduce wrong common classes from them that every check accepted: a silent
wrong intersection and difference, found by review. So a single arc with no partner, whose two
vertices the other input joins by a chain of up to 16 single arcs through vertices within `h` of the
arc's own plane (its plane in doubles, since a segment `GreatCircleArc` calls zero length has no
rotation axis) and in order along it, each further than the last by more than `1e-12` radians, is
cut at those vertices: a stop of the vertex that no record of the ring names, placed after the stop
the cut piece starts at (keyed by that stop's first record, since a vertex can have two stops on
one segment). The stops, pieces,
loops, spikes and pairs are then built again, once; the cut pieces pair with the chain's. The
rounding margin is so that a vertex at the segment's end is not taken for one inside it, one way in
doubles and the other in the prototype's exact arithmetic. The cuts fire in 14% to 43% of the
cases of the dense shared-edge generators (`chain`, `rays`, `shared`) and almost nowhere else.

**Rays and their order** (`build_rays`, `measure_ray`, `sort_rays_anticlockwise`). What leaves a
vertex along a piece, or along a common pair, is a *ray*. Its direction is the exact tangent of the
piece's first arc that lies on a segment of non-zero length (`cross(rotation_axis, start)`, negated
for an arriving end;
the segment's cached plane, never a difference of two nearby positions), taken from the arc's own
start, which is where it really starts. A piece whose arcs all lie on zero-length segments has no
direction; it bounds no sector any order needs at two or four ends, and at six or more it floats
under the bundle rule below, or, where one input passes once, is placed by its class (the
one-passage rule). A common ray takes the subject's direction, or the clip's if the subject's
first arc is zero length, and the wider of its two pieces' cones (with that piece's `delta`, length
and `reach`, below).
The rays are sorted anticlockwise about the vertex's output position, seen from outside the sphere,
by two sign tests on normalised directions (the half-plane relative to the first ray, ties by the
dot product, then the orientation within a half-plane). In principle that is a rounded decision;
measured in the prototype, the order in doubles agreed with the exact order at every vertex where
it was used, down to rays `1.8e-15` radians apart, and a swap of two rays that close changes neither
the memberships between them nor the pairing. For classification the order is consulted only at
six or more ends where neither input passes exactly once (below): at four ends it is not used, and
where one input passes once neither; the one-passage rule's linking order keeps the measured
relative order of the rays inside (below).

Each ray also carries the quantities of the bundle rule: `delta`, the angle from the vertex's output
position to the arc's start plus how far the arc's start and far end lie off the segment's great
circle; `first_arc_length`; `reach`, the angle from the output position to the far end; and the
*cone* within which the arc's direction, seen from the output position, can differ from its exact
tangent: `0` when `delta` is `0`, `pi` when the arc is no longer than `delta`, else
`atan2(delta, L - delta)` (the planar bound, whose spherical correction is far below rounding at
these scales). The start's distance is `0` exactly when the start *is* the output position (the same
stored point, compared coordinate for coordinate; `PointOnSphere::operator==` has a tolerance of
about the thickness and is never used here). The offsets off the great circle are measured only for
a stop at a touch record of the other input's vertex inside a segment, and a self-touch cut: those
lie at the other input's vertex, up to `h` off this ring's segment, so an arc `1 h` to `2 h` long
between such stops can leave tens of degrees from the segment's tangent, which a cone of the
distance alone did not bound (in four dense shared-edge cases of the hunt the true rotational order
was then not admissible and the solve deduced wrong classes). A vertex of the ring, and a crossing,
lie on the segment by construction and get `0`, as the prototype's exact zero does; deciding by a
threshold instead was wrong, since a crossing's stored position (the normalised cross product of
two nearly parallel planes) lies off them by up to about `2.4e-11` radians in a dense shared edge,
and a threshold of `1e-12` gave 30 of 824 rays cones of up to `1.5e-5` radians where the prototype
gives `0`.

**Classification** (`classify_pieces`), in this order:

1. **Vertex tests.** Every non-internal, non-common piece with an interior original vertex is
   classified by point-in-polygon of its first such vertex against the other input, threshold
   **off** (`is_inside_other`: the clip at the partitioner's speed, `Options::clip_speed`, so that
   the clip reaches high speed as the cascade reuses it; the subject at `ADAPTIVE`). Exact, with
   margin: a vertex in no record is more than `h` from the other input's outline, or it would be a
   touch, about `10^10` times point-in-polygon's rounding. In debug builds every interior original
   vertex of the piece is tested and a disagreement counted (`piece_side_disagreements`): two
   vertices of one piece on different sides mean a crossing with no record. The inclusive
   threshold suits a user's query, but here it would call a piece running just outside `Q`, within
   `s` of it, inside; the pieces must divide the sphere exactly.

   The midpoint of a piece's longest arc, the first proposal for the test point, was rejected on
   the developer's T-intersection: an arc leaving a touch at a small angle stays within `h` of `Q`
   for much of its length and can cross it with no record, so its midpoint is decided by rounding,
   where the vertex is more than `h` away whatever happens.
2. **Deduction to a fixed point** (`deduce_to_fixed_point`, `solve_vertex`), combinatorial. At
   each vertex the rules below deduce what they can; repeat while anything changes.
   - **Four ends, two of each input, none common: the geometry-free rule**
     (`solve_four_end_vertex`). `P` crosses `Q` at the vertex iff `P`'s two classes differ, `Q`
     crosses `P` iff `Q`'s do, and the two must agree: three known classes fix the fourth, four
     that disagree are a check failure. **At a vertex that is one LL record** the crossing is known
     from the record (an LL needs each arc's end points beyond `h` on opposite sides of the other's
     plane, section 4.2),
     so each input's two classes must differ: one known class fixes its *own* input's other class
     (it says nothing about the other input's), and two equal classes of one input are a check
     failure. A touch record can be a crossing or a bounce, so only the first rule applies there.
     Without the LL rule a single arc between two crossings could not be deduced until three
     neighbours were known and went to a midpoint test (218 midpoint tests in 300 random cases of
     one generator, against 1 with the rule), and two arbitrary band classes once made an LL vertex
     a consistent bounce, with wrong pieces deduced from them that every check accepted.
   - **Six or more ends where one input has exactly two (it passes once): the one-passage rule.**
     That input's boundary divides the neighbourhood into an inside half and an outside half, and
     each ray of the other input is in one half by its class alone, so any assignment of classes is
     realisable by some rotational order: the measured order adds nothing and is not consulted.
     The one relation is `m_in(b) = m_in(a) xor n`, where `a` and `b` are the passing input's two
     ends, `m_in(e)` the other input's membership on the inside side of end `e` (its class for a
     plain ray; same-side for a common ray) and `n` the parity of the other input's rays inside.
     One unknown term follows; all known and false is a check failure. For the linking an order is
     built from the classes, `a`, the rays inside in their measured relative order, `b`, the rays
     outside, or its mirror if the walk finds no placement on the first (a safety net: when the
     relation holds the first has one). The rays' relative order within a half is the other
     input's own arrangement, which decides only which kept rays pair with which (the ring
     structure), never the point set. The rule deduces strictly less than the sector walk would
     on some order, so it cannot make a consistent vertex fail. Before this rule the
     sector walk ran on the measured order at such vertices, where the order is exact but carries
     no information, and failed 19 of 2393 random `chain` cases in three ways (a crossing counted at
     both ends of a band arc, a long ray inside a `2 h` common ray's direction gap, a corner cut off
     by the spike rule). Valid inputs reach them: the second was reproduced in the prototype with a
     polygon that does not cross itself, and each needs only an input within `h` of itself at the
     vertex (a neck narrower than `h`, or a hole within `h` of its exterior).
   - **Otherwise the sector walk**, on the measured order, or on every admissible order under the
     bundle rule below. Each classified ray constrains the two sectors beside it: a subject ray of
     class `c` fixes `in Q = c`, a clip ray fixes `in P`, a common ray fixes `in P xor in Q`
     (`0` same side, `1` opposite). Walking round the order from a start, flipping `P` at a subject
     ray, `Q` at a clip ray and both at a common ray, gives every sector's memberships; the four
     starts (and every order) are tried, and those meeting every constraint are the solutions. No
     solution is a check failure. A class is deduced for an unclassified ray only if every solution
     agrees on it; for the linking, the memberships of the chosen order are reported where its
     solutions agree. A vertex with two rays both common (inside a chain) goes through the walk
     with xor constraints, which is how a chain's class travels along it from its end.

   The sector rule for a common chain follows: at a chain end three rays leave, `P`'s piece `p`,
   `Q`'s piece `q` and the chain `c`, so every pair of rays is adjacent and no order is needed. With
   `a = [q inside P]` and `b = [p inside Q]` the sector between `p` and `q` is `(a, b)`, so the
   chain's two sides are `(not a, b)` and `(a, not b)`, and the chain's interiors are on the same
   side iff `a != b`. Ring orientation plays no part, which is what fixes the old classification
   of a piece along the boundary by the polygon's orientation. Either end of a chain decides it,
   and the other end is an independent check.
3. **Stalled single arcs, by margin.** Pieces still unclassified are single arcs between records.
   Each one's chord midpoint is measured to the other input's outline (`minimum_distance`), and the
   one with the **largest** distance is tested there, threshold off; then deduction resumes. The
   distance is rounded (a cosine, resolving about `1.5e-8` radians near zero), but it only orders
   the tests. The principle: a class decided inside the band (an arc within `2 h` of the other
   outline over its whole length, a *band piece*, whose class is arbitrary by nature) must never be
   the source of a deduction for a piece with a margin. Testing stalled arcs longest first, or in
   piece order, let a `1.2 h` band piece in a neck of `Q` fix, through a four-end vertex, a piece
   `10^4 h` long, wrongly, in 9 of 300 random cases. With the LL rule as well, the midpoint tests
   that remain are band pieces (1 in 300 cases of one generator, 24 in another, up to 250 in dense
   shared edges), and inverting every one of them changed only slivers under `0.5 h` wide, in every
   generator but the dense shared edges. There, inverting them all gave a wrong output in 10 of
   1,155 cases of the prototype, with repair at its round cap. Rounding cannot do that: of the 69
   midpoints inverted in the broken cases examined, 64 lay `0.05 h` to `0.8 h` from the other
   outline, where an exact test cannot flip, and inverting only the 5 within rounding of it
   (about `3e-15` radians, arcs running along the other outline) broke nothing. So a midpoint
   test is a coin toss only for an arc within rounding of the other outline; elsewhere in the band
   it is exact for its own point, though the topology the records have snapped can still disagree
   with it (the inconsistent cut of section 5.6).
4. **Anchorless chains** (`get_common_chain`, `classify_anchorless_chain`). A common pair still
   unclassified belongs to a chain no non-common piece reaches: a whole ring of `P` coinciding with
   a whole ring of `Q` within the thickness, as when static polygons are partitioned by themselves,
   a hole of `P` equals a ring of `Q`, or what is left of `P` after the earlier plates is exactly
   the next plate. Among the chain's arcs with a direction, the one whose great circle is farthest
   from both inputs' outside points (the largest of the smaller `|dot(R, axis)|`) is classified at
   its midpoint `X` by two exact crossing parities: for each input, the crossings of the arc from
   that input's outside point `R` (section 3.2) to `X_i`, the projection of `X` onto the input's
   own segment along the chain, with every edge of the input except that segment
   (`PointInPolygon::get_crossing_parity_excluding_segment`, an entry of its own, since the
   crossing counter is file-local). The arc from `R` meets the excluded great circle only at `X_i`, so the
   parity is that of the points just before `X_i` on `R`'s side, and `R` is outside, so odd parity
   means inside. (Counted to `X` itself, the clip's parity was wrong where `X`, on the subject's
   arc, lies off the clip's great circle by up to `h`: the arc from `R` then meets that great circle
   at a point displaced along the chain, which near a short clip segment lies past its end, so the
   clip's membership and the chain's class were inverted with every vertex consistent, and a thin
   polygon partitioned by a near-copy of itself came back wholly outside it.) An arc whose clip
   partner has a direction is preferred, so that both projections are defined; failing that (every
   such partner a segment `GreatCircleArc` calls zero length), the clip's parity is counted to the
   subject's projection instead. `X` is the midpoint of the *subject* piece's stops, and the clip
   piece's stops at the same two vertices can lie up to the vertices' cluster width along the chain
   from them, so `X`'s projection can still fall past the end of a clip segment no longer than
   that width; no such input has been built. If `R` is within
   about `1e-6` radians of the great circle (where the arc could meet it twice) or of `X_i` or its
   antipode, the input's second reference point `R2` with its cached status is used instead
   (section 3.8); `R2` is not tested against the great circle in turn (it is 60 to 120 degrees
   from `R`, so both lying on one great circle of the chain is unlikely, not excluded), and no
   committed test was built to reach the fallback. If the two reference points are on opposite sides of their inputs' great circles
   (the two within the thickness of each other, their axes aligned by sign), one membership is
   flipped, since crossing the chain flips both; the chain is same-side iff the memberships on one
   side are equal. Deduction then carries the class along the chain. Nothing is offset beyond the
   projection onto each input's own segment, so no thin face can defeat it; the
   prototype's version used one reference point and a random waypoint kept on its side, and the C++
   uses each input's own outside point and a direct arc. The alternatives: a point offset sideways
   by more than the tolerance (fails every face thinner than the offset); Foster, Hormann and Popa's
   rule, keep the ring iff both rings are holes or both are not (needs each ring's hole status by
   point tests that are fragile when rings touch); an offset arc intersected with both inputs
   (`GeometryIntersect` cannot see a boundary within `h` of the arc's start, which is on the chain);
   and a path through an unrestricted waypoint, which arrived from the wrong side in about half the
   prototype's runs, so both memberships described the other side. No hunt generator produces an
   anchorless chain; the committed cases are built for it.
5. **A common clip piece takes its subject partner's class.**

**The bundle rule** (`build_bundle`), at a vertex with six or more ends (a common ray counting two)
where neither input passes exactly once (both pass more than once, or only one has ends left). What
defeated a single measured order in the prototype was not parallel rays but short arcs leaving a
chained identity group from off its output position: their directions, seen from that position, are
noise. Rays are taken in order of certainty (smallest cone, then
largest reach); a ray is *anchored* if, against every ray already anchored, the angle between their
directions is at least the sum of their cones, else it *floats*; a ray with no direction or a cone
of `pi` never anchors. At a vertex whose rays all start at its output position, with no touch inside
a segment and no self-touch cut at either end of their first arcs (so no offset off the segment,
above), every cone is `0`, every ray with a direction is anchored and nothing changes. The *admissible orders* place the floating rays into the
gaps between the anchored rays (which keep their measured cyclic order), in any permutation within a
gap, such that angles can be chosen non-decreasing round the turn, each within its cone plus the
reference ray's of its measured angle (the reference is the most certain anchored ray; greedy
earliest placement, which is complete for interval feasibility). The measured order is always
among them, and first: it is admissible in principle, and is added if rounding keeps it out of the
enumeration. The sector walk runs over every admissible order, deduces only what
every consistent order agrees on, and for the pairing uses the consistent order with the fewest
inversions (Kendall distance) against the measured order, ties by enumeration order with the
measured order first (it is swapped with the first enumerated order; gaps in anchored order,
floating rays in certainty order, permutations lexicographic, so that the C++ and the prototype
agree). More than six floating rays (`BUNDLE_CAP`) keeps the measured order alone and counts the
vertex; with every ray floating there is no frame, and the measured order is kept. The cap bounds
the floating rays only: `m` of them among `k` anchored rays are enumerated as `(k + m - 1)! / (k -
1)!` orders (5,040 for six among two) before the admissibility test, and the sector walk then runs
on each admissible one. The criterion is
geometric but threshold-free: the cone is the bound on how far the arc's direction seen from the
output position can differ from its tangent, and `h` enters only through the data. Either side of
separability nothing jumps: a barely separable pair keeps its measured order; barely inseparable,
the less certain ray floats and the solve either confirms the measured order (one consistent
placement, the usual case) or offers a swap across a neighbour the ray cannot be told from, touching
only a sliver no wider than the group's extent (measured within `0.42 h` of a boundary). Rejected
bundle criteria: single arcs float and pieces with an original vertex anchor (wrong both ways: a
long single arc's order is real geometry, and a short arc with an original vertex is noise); rays to
one neighbouring vertex bundle (misses the misplaced common ray); bundle everything and solve by
parity (two frames with no anchor); and deducing from the tie-break's order alone, which left two
cases failing, since committing early to what a noisy order implies is what made those vertices
inconsistent. The rule is reached in about one vertex in 3,500 random cases. What the cones do not
cover is the gap between a common ray's two tangents (the subject's and the clip's, tens of degrees
apart for a common arc a few `h` long, while the ray's cone is `0` when both arcs start at the
output position): a third ray inside that gap is ordered by the subject's tangent alone. Where one
input passes once the order is not consulted (the one-passage rule, whose second failure this was);
where both pass more than once the exposure remains, and no case of it has been seen.

**Checks and repair** (`check_and_repair`, `repair`). Two properties hold by construction and are
not checked on their own: every input has an even number of ends at every vertex (nothing counts
it), and every walk of the linking closes (a walk that did not would be a pairing failure, counted
and thrown, section 5.6). What the data can get wrong is the classes, and the check is
`solve_vertex` at every vertex: a vertex with no consistent placement fails. That covers both
redundancies: at an ordinary vertex `P`'s two classes against `Q`'s (a bounce at an LL vertex is a
failure), and at a chain end the chain's class arriving from its other end. Repair runs up to
`Options::max_repair_rounds` (20): at the first failing vertex the classified pieces there (a common
pair as its subject piece) are ranked least reliable first by method (midpoint test, deduced, sector
or anchorless, repaired, then vertex test) and then shortest first, and the first whose flip makes
the vertex consistent is flipped (a common piece's flip flips its partner); at a chain's end one
more candidate flips every piece of the chain at once, ranked with the sector classes and tried
before any single piece of that rank, since flipping a chain piece by piece fails the chain's
interior vertices on the next round and runs into the cap. The whole-chain candidate is offered
only at a vertex with a ray that is not common (the chain's end); a failure that first shows at a
vertex inside a chain has single flips alone, each moving it one vertex along. Every repair is
counted (`repairs`,
`whole_chain_repairs`, and `trusted_flipped` when a vertex-tested class went). No normal input needs
repair: the prototype's runs of the design needed none, and the prototype
comparison fails a case on any repair of generated data. A vertex still failing after repair, or a
piece left unclassified, is the failure of section 5.6.

**Linking** (`is_kept`, `pair_and_walk_rings`, from `extract_rings`). With `f(in P, in Q)` the operation's function
(`is_in_output`), a subject piece of class `c` is kept iff `f(1, c) != f(0, c)`, a clip piece iff
`f(c, 1) != f(c, 0)`, a same-side chain iff `f(1, 1) != f(0, 0)`, an opposite-sides chain iff
`f(1, 0) != f(0, 1)`: the output differs across the piece. Per operation:

| Operation | `f` | `P` in `Q` | `P` out | `Q` in `P` | `Q` out | chain same | chain opposite |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `INTERSECTION` (partitioning's inside) | `p and q` | kept | | kept | | kept | |
| `DIFFERENCE` (partitioning's outside) | `p and not q` | | kept | kept | | | kept |
| `REVERSE_DIFFERENCE` | `q and not p` | kept | | | kept | | kept |
| `UNION` | `p or q` | | kept | | kept | kept | |
| `SYMMETRIC_DIFFERENCE` | `p != q` | kept | kept | kept | kept | | |

A clip piece inside `P` is kept by both of partitioning's outputs. The kept ends are paired at
every vertex, by the first rule that applies: an end whose piece is unclassified, or an odd number
of kept ends, is a pairing failure (impossible with consistent classes, and counted and thrown);
two kept ends pair, with no geometry; at a four-end vertex of the free rule, at a crossing the
output sector `(mP, mQ)` is bounded by the subject piece whose class is `mQ` and the clip piece
whose class is `mP`, and at a bounce `P`'s two ends pair and `Q`'s two pair (the *split pairing*,
so that the two rings touch at the vertex and no cut is needed there, unless one ring comes back to
the vertex by way of other vertices, which the split below then cuts); where only one input has ends
left (the other's were a loop or a spike) each kept end pairs with the end, at this vertex, of its
ring's next non-internal piece (a neighbour that is elsewhere or not kept is a pairing failure), the
ring passing straight through (*ring continuity*, which no
valid input is known to show, since where the passages do not cross any non-crossing pairing gives
the same rings after the split); where every ray is common (identical inputs that touch themselves)
the walk knows the memberships only up to swapping both, so consecutive kept rays pair in the order
used, in either phase, and the split below decomposes the result; otherwise the sector walk,
without the free rule, on the order used: the output membership of each sector is `f`, a ray is
kept iff the output differs across it (counted as `keep_mismatches` if that disagrees with the
keep-rule), and consecutive kept rays pair across the output sectors. Of the free rule's two cases
only the bounce is reached with four kept ends today: a crossing keeps exactly two ends in the
intersection, the union and both differences (each keeps one class of each input's pieces), which
pair as two kept ends, and the symmetric difference, which keeps all four, is no longer paired by
its own row (below). The walk then starts at any
unused paired ray, appends its vertex's position and the piece's interior stop positions (reversed
when leaving from the piece's end), arrives at the piece's other ray, takes its partner, and so on
until back at the start. A kept closed piece (a ring with no record) is a ring on its own. Along a
common pair the walk follows the subject's piece. A common pair has no interior stops, so the output
there is its two vertices' output positions whichever piece is walked; what the choice gives is the
subject piece's stops, for provenance (section 5.4).

**The symmetric difference** (`walk_rings`, `join_difference_rings`) is not paired and walked by its
own row of the table. Its rings are the difference's and the reverse difference's, each walked and
split as below, then joined across the *walls* between them and split again. A wall is a run of
opposite-sides common pieces that both differences walk one after the other (both keep them, the
symmetric difference does not): at each end of a joined wall the two rings' neighbouring steps are
linked to each other, and the wall's steps dropped. A wall is left unjoined (`spike_walls`) where
an arc by which either ring reaches one of its ends, or leaves it, passes its other end: that
vertex within `h` of the arc's plane and inside the arc by more than `1e-12` radians, the measures
of the self-touch cuts (section 1). That ring comes back along the whole wall within the thickness,
a spike, so its output has no width there, and the two rings are left touching along the wall, as
at a vertex.

Its own pairs were the design until the operations were exposed, and gave crossing rings from
inputs whose rings do not cross in 37 of the hunt's 39,313 such cases (57 with the inputs swapped),
with every point of the output still right. The symmetric difference keeps all four ends of every
crossing, so a walk of its pairs passes most vertices twice, and the split's cuts are right in any
order only while no two passages cross. Two things made passages cross. Where two rays of one input
leave a vertex parallel, about a thickness apart (in `six#29 seed 10`, two holes' edges `1.34 h`
apart either side of the other input's vertex), their measured order is noise and the pairs made
from it can cross: a cut at that vertex undoes them, but a cut made first at an ordinary crossing
between the vertex's two passages leaves two rings that each pass the vertex once, crossing there
and at the crossing (the 12 of the 37 from the `six` generator; with the split's cuts made in
another order, 39 of 47). A difference keeps two ends of a crossing, so its walk comes back to such
a vertex with nothing repeated in between, and its own cut there undoes the noise. And a wall can
separate nothing: at a crossing of the inputs where one has an edge under the thickness long that
doubles back along the edge before it (in `zero#133 seed 10`, three vertices of the subject within
the thickness of the clip's edge), the pair that edge makes with the other input's is classified as
any other, from band pieces, both differences walk it, and pairing through it gives the two inputs'
boundaries passing straight through the crossing (the other 25, from the `zero` generator, and the
same 25 if every wall is joined across).
Joined but for the spikes, the two differences' rings repeat a vertex only where the two outputs
touch, and touches can be cut in any order: the hunt then has no crossing group from such inputs in
any operation, the inputs either way round, and the same with the split's cuts made in the other
order. The cost is two walks, two splits, the join and a third split: measured on three sets of
generated cases, the symmetric difference's rings take 2 to 2.8 times as long as its own pairs did,
and its whole `extract`, with the grouping, 10% to 26% longer. (The same measurement put the cost to
partitioning of keeping the walked rings for the join, and of the test for a ring twisted at a
chain below, at 2% to 5% of its `extract`.)

The spike rule leaves more walls than it must, since leaving one is always right (the two
differences' rings, sharing the wall, read as the output does) and joining one is right only if it
separates the two outputs. Of the 538 walls it left in the hunt's cases with valid inputs (528
cases), 455 were under `2 h` long, which is any wall whose far end lies within the thickness of the
next arc; the other 83 (up to `7,300 h`, all from the `shared` generator) are shared edges along
all of which a boundary doubles back (in `shared#508 seed 26` the clip passes the stretch three
times). Not by the wall's length, which would be a second
tolerance; and only an arc that passes the whole wall, since a spike along part of one leaves the
rest a wall between the two outputs.

**Split and clean-up.** A walked ring that passes an overlay vertex twice (by identity, not
position) is `v, A, v, B`; it is cut into `v, A` and `v, B`, repeatedly, until no ring repeats a
vertex (`rings_split`). Where the pairing at `v` crosses nothing, the cut is the other
*non-crossing* pairing of the four ends at `v`: of the three pairings, exactly one of the other two
crosses; if the cut were the crossing one the two rings would cross at `v` and nowhere else, but two
closed curves on the sphere cross an even number of times. Every pairing rule above crosses nothing
in the order it uses; where that order is noise the pairs can cross, and the cut at that vertex is
then the pairing that does not, provided no other repeated vertex between its two passages was cut
first (which is why the symmetric difference is not walked by its own pairs, above). OGC simple
features forbid a ring that touches itself
and allow a hole touching its exterior at a point, so the split is always made (the developer's
decision); it is Foster, Hormann and Popa's split-candidate clean-up, found without their labels. A
twist of `P` on `Q` passes its two LL vertices once each and is not cut: the output crosses itself
where the input did, and a simple output needs the self-overlay of section 7. Clean-up, last and
counted and never silent: consecutive repeated positions, and a last position equal to the first,
are removed, comparing coordinates exactly (the repeats come from one stored position); a ring with
fewer than three distinct positions is dropped (`collapsed_rings_dropped`): the regions the records
had already collapsed, such as the wedge tip, zero area by construction. Nothing else is merged or
dropped here.

**A ring twisted at a chain** (`split_walked_rings`, counted as `rings_cut_at_chains`). A ring that
repeats no vertex can still cross itself between two vertices `u` and `w` joined by common pieces
its operation does not keep. `GeometryIntersect` never intersects an input with itself, so an arc
`T` that leaves `u` between the two boundaries of a common pair has no stop where it passes the
pair's far end `w`, between the rays that continue the two there (it cannot cross either on the
way: a crossing of the other input's is a record, and an input whose rings do not cross does not
cross its own). The pair is one ray at each end, with `T` on one side of it, so no order of the rays
at `u` or at `w` has `T` between the two, and no pairing there is wrong: the measured orders at the
two ends do not show it. But a ring that leaves `u` along `T` and passes `w` from one of the two
boundaries to the other is twisted between `u` and `w`, its two parts wound opposite ways. Parity
reads such a ring right, point for point, so the oracle passes it; its signed area is the
difference of the parts, and it has no orientation with the output on its left. The catalogue's 13
`d4 hole-between` cases are this (a hole's tip on its own exterior's short edge, on the clip's
edge: the hole's edge leaves the tip between the two, and the reverse difference's ring comes back
along the clip's edge to the short edge's far end and leaves along the exterior, with a part `318
h` wide wound backwards); with the inputs swapped it is partitioning's difference. No test caught
it: the oracle reads by parity, and the one area summed was the intersection's with the
difference's, with the inputs the way round that is right.

So after the cuts at repeated vertices, a ring that passes two vertices of one unkept chain without
walking the chain between them is tested for the crossing `GeometryIntersect` would have recorded
had it intersected the ring with itself, with the same thickness (section 1): `w` within `h` of the
plane of the ring's arc at `u` and inside the arc by more than `1e-12` radians, the arc not ending
at `w`; and the ring's first points beyond `h` of that plane, each way from `w`, in different
sectors of the ring's passage at `u`, its two arcs there taken as rays. (Not the two sides of the
arc's plane alone: with a chain under the thickness long the passage at `u` is a corner beside `w`,
not a line.) A point within `h` of the other arc's plane where that would decide, and a ring that
reaches the passage at `u` first or runs within the thickness past the arc's end, decide nothing,
and the ring is left whole. A ring that is twisted is cut at `u` and `w` as at one vertex, each
part closed along the chain's pieces (the subject's, with their stops, for provenance), so that the
two parts touch along the chain. The part that keeps `T` still leaves `u` along it, so where `w`
lies on the other side of `T` from that part, its edge into `w` crosses `T` within the thickness of
`w`: a sliver under the thickness wide wound backwards, as a dense shared edge's rings have
elsewhere (below).

Not by the chain's length, which would be a second tolerance: `T` runs along a chain of any length
(140 thicknesses in the test). And not at a touch: a ring that passes both ends of an unkept chain
without crossing itself (a notch along a shared edge), or only comes back within the thickness of
itself there, is left whole. In the hunt the cut is made in 10 of the 39,313 cases with valid
inputs, as exported: 3 intersections and 7 differences, all from the `zero` generator, each cutting
off a sliver of `1.1` to `1.7 h^2` that was wound backwards, after which the intersection's and the
difference's areas sum to the subject's to rounding (they were off by twice the sliver). No valid
case of the hunt had a twisted ring whose narrower part was over a thickness wide, before or
after. The cut is also made in cases whose input rings cross (96 operations of the hunt as
exported, in the intersection, the differences and the union; 59 cases in the intersection or the
difference), where a ring crosses itself because an input does: each is one ring more, reading the
same by parity.

**Grouping, orientation and the hemisphere check** (`extract`). For inputs whose rings do not cross,
the rings of one output do not cross each other (twists apart) but within an overlay vertex, where a
vertex's output position can be up to its record cluster's width off its records; they may touch at
vertices. Where an input's rings cross each other, or a ring crosses itself, output rings cross at
points the overlay never computes (it intersects each input only with the other). Two polygons are
read as their union, so crossing rings in separate polygons overlap (kept apart, 11,212 of the
hunt's 70,291 cases with crossing inputs read wrong as polygons, section 6). So
rings that cross (`do_rings_cross`) are first joined into **groups**, and a group's rings all go
into one polygon, which reads by parity as the output does. Rings whose outlines are farther apart
than the margin below do not cross (one lies inside the other, or they are apart), which a
distance query over both rings' bounding trees settles for almost every pair from a valid input.
Otherwise two rings cross if either has a point inside the other and a point outside it, each
farther than the margin from its outline (`has_points_on_both_sides`), the points being its
vertices and, at a vertex within the margin of the other ring, the midpoints of its two edges and
the points three margins along them (`PROBE_DISTANCE_IN_MARGINS`). That sees rings that swap sides
along a stretch within the margin of each other (a dense shared edge); an edge between two points
on the other's outline, a chord through it (rings that meet only at vertices on each other's
outline, such as a square with an interior ring through three of its corners); and an edge that
leaves the other's outline at a vertex and leaves the other ring again through one of the other's
vertices (two rings each with vertices on the other's edges, a pinwheel): overlaps of any
size with no vertex of either inside the other. Or two rings cross if an edge of each crosses an
edge of the other with the ends of both farther than the margin from the other's great circle (two
rings crossing like a plus sign, no vertex of either inside the other), by both rings' bounding
trees. The point test walks one ring's bounding tree against the other ring's bounding circle: a
node farther than the margin from that circle is outside it at once, so a large exterior is not
walked vertex by vertex for each of its many holes. The cost, against the old grouping's (one
point a pair): about 1.1 times for an exterior with 1,481 holes and 100,000 points, 1.1 times for an
annulus of two 100,000-point rings, 1.4 times for a cut leaving 50 long pieces side by side, and
2.7 times for 100 of them, whose 4,950 pairs each need the distance query (without it, 25 times,
since such rings lie in each other's bounding circles and the walk can prune nothing). So a touch at
a vertex, a crossing within a vertex's cluster, and stretches within the thickness of each other
are not taken for crossings, and an overlap reached only within a few margins of the outlines is
missed: a random probe of polygons with several crossing rings, snapped to a grid of 0.004 radians
so that rings share vertices and stretches, finds 2 such operations in 11,668 cases, and none
generic or on a grid of 0.002.
Groups do not cross each other, so a group lies
wholly inside or wholly outside each ring of another (a ring crossing one inside a ring `R` reaches
inside `R`, so lies inside it), and the number of rings containing it is the same all over it. The
groups are then nested by **containment with parity depth**, tested on each group's first ring
(`is_ring_inside_ring`): whether ring `r` lies inside ring `r'` is
decided by one point of `r` tested exactly against a polygon of `r'` alone, the first vertex farther
than a margin from `r'`'s outline, else the farthest of `r`'s vertices and its edges' midpoints, and
if every one of those is on `r'` the rings coincide (counted). The margin is `max(10 h, twice the
widest record cluster)`, the oracle's sampling margin: a margin of `s` alone let a vertex `1.6 h`
across a gap under `2 h` between two facing holes decide that a hole lay outside its exterior, in 5
of 119,198 generated cases, the intersection's area wrong by about a quarter. Every pair of rings
not already in one group is given to the crossing test (which returns at once if their bounding
circles do not meet); containment is asked of each group's first ring against every ring of the
other groups, one point each. A group's depth is the
number of rings containing it and its parent the containing group of greatest depth. An even depth
starts a polygon, whose exterior is the group's ring of largest area (any would read the same by
parity) and whose holes are its other rings; an odd depth puts all of the group's rings, as holes,
into its parent's polygon (a hole group's parent's own polygon, in a stretch the parent group
covers twice); an island inside a hole is a polygon of its own. That is a nesting only if the
group's depth is its parent's plus the number of its parent's rings containing it; a hole group
for which it is not (rings whose overlap the crossing test misses, reached only within a few
margins of the outlines)
is kept as a polygon of its own and counted (`grouping_failures`), not thrown, since partitioning has always accepted inputs whose rings cross
(self-intersecting polygons, common in real data). For inputs whose rings do not cross, every group
is one ring, in every operation, and this is plain containment with parity depth (no such input of
the hunt gives a crossing group). A ring of such an input can still cross itself within the
thickness, as drawn through its vertices' output positions: a sliver under `0.5 h` in mean width
wound backwards, in about 270 of the hunt's 39,320 such cases (and one between `0.5 h` and `h`),
which is the band's noise and not a twist. Each exterior is oriented anticlockwise
seen from outside the sphere and each hole clockwise, so that the output is on every ring's left
(the developer's decision; GeoJSON asks for this and Shapefile for the reverse, and the walk's own
direction is arbitrary), by `PolygonOrientation::calculate_polygon_orientation`, not
`SphericalArea`'s signed area, which is zero for a ring the thickness across (every edge of it is
zero length to `GreatCircleArc`). A ring that crosses itself, or a ring of a group, has no
orientation with the output on its left all round.

The **hemisphere check** is the oracle at one point. Given rings along the right edges, the parity
over their smaller sides (section 3.1) is everywhere either the output or its complement: each ring
whose output side is its larger side complements the reading, and an even number of them cancel.
So one reference point, the first of the inputs' bounding-circle centres, their antipodes and the
six axis points that lies farther than `10 h` from both inputs' outlines, read by parity over the
output rings, must be in the output exactly when `f(in P, in Q)` says so (all three tests threshold
off). A complement means the output exceeds a hemisphere or that the classes are wrong; the check
cannot tell which, and its message says so. Only a union or a symmetric difference can exceed a
hemisphere, and not every one that does reads as a complement (one whose rings each bound their
output on their smaller side, a parity output of several rings, reads right); an intersection or a
difference cannot, since each of its rings' inside lies within one input ring's smaller side. It
throws for every operation (section 5.6); no point clear of both outlines (never seen) is counted
(`hemisphere_check_skipped`). The check reads the rings as walked, before they are oriented, so an
output ring within the tie tolerance of halving the sphere (section 3.1, where the side to a ring's
left is its interior) is read by a direction that is arbitrary: the union of two quarter spheres,
exactly a hemisphere, is returned or refused according to the inputs' order and directions.

A whole answer (`classify()` finding the subject wholly inside or outside, which the partitioner
returns without extracting) has the same check at one point (`check_whole_result`): the first
candidate clear of both outlines and inside the subject among the subject's bounding-circle centre,
its interior centroid and points twice the margin off either side of four of its exterior edges (for
a C shape or an annulus) must be inside the clip exactly when the answer is inside. A wrong but
consistent classification (every vertex consistent, as the inverted anchorless chain above was)
passes every other check. A point outside the subject tells nothing, so with no candidate inside it
(a subject thinner than the margin everywhere, which is routine) the check is counted as skipped
(`whole_check_skipped`).

**The sliver filter** (section 5.5), applied in `extract` after grouping: groups are decided
outermost first, and a group inside a dropped group is dropped with it, since a dropped exterior's
region goes to the other output and a dropped hole is filled, with everything inside either (an
island of a filled hole, kept, would overlap its polygon). A group of crossing rings is not filtered
itself (though it is dropped with a dropped ring it lies in): its region is no one ring's. Nor is a
ring that crosses itself (`does_ring_cross_itself`, asked only of a ring that measures as a
sliver): its mean width uses the fan's signed area, which weights each lobe by its winding number,
so lobes wound opposite ways cancel, and a bow-tie whose lobes are near equal would measure as a
sliver and be dropped whole, however large. A ring crosses itself if two of its edges cross
clear of their ends (the edge test, its bounding tree against itself), or if at a vertex within the
margin of a part of the ring not adjacent to it the ring's path through the vertex passes from one
side of the path there to the other, judged at points three margins along both paths, each farther
than the margin from the other path (a bow-tie with a vertex at its crossing). A touch
(lobes wound alike) is not a crossing, nor is anything decided only within a few margins of the
crossing point, nor a side judged against a path whose arms are under two margins long (past the end
of a short arm the ring can pass round it without crossing): so a ring of a valid input is not taken
as crossing itself (none of the intersections' and differences' output rings in the hunt's 39,320
cases with valid inputs, asked of every ring), and a self-crossing within a few margins is missed.

**Polylines** (`classify_polyline_pieces`, `extract_polylines`). A polyline subject shares the
structures above (the loop rule skips a piece with a dangling end; the self-touch cuts apply); the
spike rule does not apply to a polyline, whose hairpin from one clip vertex to another and straight
back is a run of its own (made internal, it took its neighbours' class untested, and a polyline
across the clip came back whole outside). The clip's pieces bound nothing and are not classified,
and there is no sector walk, linking or repair. A common piece (along the clip's outline), and a
single arc between two vertices that a clip single arc also joins (a hairpin along the clip's edge
is two subject arcs against one clip arc, which pair as nothing), is **inside** (`METHOD_ALONG`; the
developer's
decision, so that in the cascade a polyline along a boundary shared by two plates goes to the first;
`Options::polyline_along_is_inside` lets a caller wanting the two outputs to partition the polyline
exactly make it outside). A piece with a test point is tested there, threshold off: its interior
original vertices, or a dangling end (an end point in no record is more than `h` from `Q`).
Deduction crosses only a vertex that is one LL record with exactly two polyline pieces, neither
common, whose classes then differ; never a touch (the polyline can bounce) and never from an along
piece (so its class cannot leak). Stalled pieces are tested at their midpoints by margin. The one
check: two *classified* pieces at an LL vertex with equal classes (a missed intersection, or a class
deduced across a neighbouring crossing), counted (`ll_disagreements`) and both kept. The output
joins consecutive pieces of one class into runs; an internal piece takes the class of the piece
before it (after it, at the start) so the runs cover the polyline. A run's points are its stops' own
positions, not the vertices' output positions a polygon's rings use: an identity vertex along a
dense run of touches can have its output position far off the polyline (the oracle found 78 of 300
sampled points uncovered in one case), and a polyline's adjacent runs need only share the stop
between them.

### 5.3 Tolerances in the overlay

The overlay builds and classifies with no distance threshold of its own: vertices are grouped by
identity, and the one distance is `GeometryIntersect`'s thickness `h` (in the records, and again
wherever an input's touch of itself, which no record holds, has to be found: the self-touch cuts,
and the linking's two tests of an output ring's arc passing a vertex). What it adds are margins for
its checks and for reading its output, all in
section 1's table: `10 h` for the hemisphere check and a whole answer's check, and the grouping
margin, `max(10 h, twice the widest record cluster)`, for containment and for the crossing tests,
which look three margins along an edge. A margin keeps an exact test clear of the band. It also
bounds what the crossing tests see: an overlap or a self-crossing reached only within a few margins
is missed (section 5.2). Its decisions are of section 5.2's three kinds:

- **Exact**: the identity of vertices (union-find on record indices); whether an arc starts at a
  vertex's output position, and clean-up's repeated positions (coordinates compared exactly, never
  `PointOnSphere::operator==`, whose tolerance is about the thickness); the record kinds the LL
  rule reads, which `GeometryIntersect` decided with its thick planes; the rotational order among
  rays, treated as exact (sign tests in doubles on directions that are themselves rounded, measured
  to agree with exact arithmetic wherever the order is used, and consulted only where a swap of two
  near rays is harmless, section 5.2); the vertex
  tests and the anchorless parities (point-in-polygon's sign tests, threshold off, with a margin of
  at least `h` at a vertex); the containment tests of the grouping, and the point and edge tests of
  its crossing tests and of the sliver filter's self-crossing test (point-in-polygon with the
  threshold off and sign tests, at points farther than the margin above from what they are tested
  against); and the point tests of the hemisphere check and of a whole answer's check (with `10 h`
  from the inputs' outlines).
- **Rounded**: the midpoint tests of band pieces, whose class is arbitrary by nature and confined
  to pieces within `2 h` of the other outline; the margins that order them (a cosine, resolving
  about `1.5e-8` radians, so two band pieces' margins can tie and the first in piece order is
  tested, which is harmless because both are band pieces); the cones and the separability and
  admissibility of the bundle rule, where a flip at the margin moves a ray between anchored and
  floating and changes nothing or offers a swap confined to a sliver; the self-touch cuts (a vertex
  within `h` of a piece's own plane and further along it by more than `1e-12` radians, section
  5.2); the linking's tests of an output ring's arc passing a vertex with the same two measures (the
  cut of a ring twisted at a chain, where a flip at the margin is a part about the thickness wide
  cut off or left wound backwards, and a wall left unjoined in the symmetric difference, where a
  flip leaves two rings that read the same as the one); which points the crossing tests take as
  near (a distance compared with the margin, where a
  flip changes only whether a crossing within a few margins is seen); the orientation of an output
  ring; and the sliver filter's widths. Point-in-polygon itself is not exact near an edge
  `GreatCircleArc` calls zero length (under about `1e-6` radians): its crossing count stands such an
  edge's crossing in by its end point (section 3.4), so within about the edge's length of it the
  answer can be wrong (measured: a point `0.019 h` inside a polygon called outside at every speed,
  and `minimum_distance` likewise off there). A vertex in no record is more than `h` from the
  outline, beyond that zone, so only a band piece's midpoint test can be affected.
- **Combinatorial**: the loop, spike and common-pair rules, the free, LL and one-passage rules,
  the sector walk and the bundle solve, the keep-rule, the pairing, the split at a repeated vertex,
  the join of the two differences' rings across a wall and the grouping's depth parity.

Why identity and not distance. The records already encode the snapped topology (which vertices
touch which segments, decided once with `GeometryIntersect`'s thickness), and the rays describe
the unsnapped one; with a piece inside the band at a vertex the two can disagree, and every
rotational-order failure traced in the prototype was of that kind. Grouping records by identity
needs no constant, keeps the four-end case free of geometry, and leaves `GeometryIntersect`'s
thickness the one meaning of "the same point". Contracting pieces shorter than `2 h` into clusters
(the design's first proposal) is section 5.7's main rejected alternative.

### 5.4 Provenance and scalar coverages

**Provenance.** With `Options::want_provenance`, every output point carries a `Provenance`: its
`Location` on the subject (a vertex, as ring and index, or a segment and the angle fraction along
it; none for a *cut* point, a point of the clip's outline inside the subject) and, for a point on a
clip's outline, its location on that clip, with the clip's index in a cascade. Ratios are angle fractions along the
segment, as `GreatCircleArc::point_on_arc` takes them, computed from positions by projection onto
the segment's great circle (`ratio_along_segment`), not from a record's `angle_in_segment`, which
only orders. At an overlay vertex (`get_vertex_provenance`) the point's provenance is its output
position's: a vertex holding a subject vertex is that vertex exactly; otherwise the subject location
comes from the subject piece the output ring arrives or leaves along, at the ratio of the position's
projection onto that piece's segment, and if neither walked piece is a subject piece the point is a
cut, even where the vertex lies on `P`. The clip location is the vertex's clip vertex, else a
walked clip piece's stop, else the clip segment of the vertex's first record; every vertex lies on
`Q`. The provenance is filled per ring **after the split**, from the stops of the pieces the ring
walks (each vertex entry keeps the stop it leaves from and the leaving piece's far stop, and an
entry arrives at the previous entry's far stop, cyclically), so that each half of a split ring has
its own; filled before the split, a split ring's vertex lost its subject location or took the other
passage's in 611 of 4,192 generated cases. A polyline's points are its stops', and a record's clip
location is the stop's own record's, not its identity vertex's, which along a dense run of touches
can name a clip vertex up to `12 h` away. An overlay vertex holding several subject vertices (a
chained group, or a vertex of the exterior and one of a hole joined through a clip vertex, section
5.2) is output at one of them, the first its records name (`Vertex::subject_vertex`), whichever
ring walks through it: the others are not output, and a coverage's values at them are not carried
(they lie within the group's extent).

**Composition through the cascade.** `GeometryCookieCutter::partition_geometry(...,
want_provenance)` makes every piece a `PartitionedGeometry` carrying its table, and the next
partitioner composes its outputs' locations with it (`Options::subject_provenance`,
`compose_subject_location`): a subject vertex of the piece keeps its table entry; a point inside the
piece's segment `j`, between its points `a` and `b`, lies on the one original segment `S` that both
`a` and `b` lie on, at `t_a + t (t_b - t_a)`, since a sub-arc of a great circle arc is parametrised
by angle along the same great circle (not quite exactly: a piece's vertex can be off `S` by up to
its identity group's extent, so a composed location is a projection). Whether vertex `0` ends a
ring's last segment cannot be told from a table, so the composition is given the original geometry
and the cascade's partitioning polygons (`Options::original_subject`, `cascade_clips`;
`CascadeProvenance` in the partitioner); without them no wrap-around is assumed, and such an edge is
a counted mismatch, never a wrong segment (guessing it once put a location `0.03` radians off). The
induction that `a` and `b` share a segment fails where an identity vertex holds two vertices of one
input (a piece's edge then spans the group, from a vertex before it to one after) and where a point
at an input's corner lies within `h` of both its segments but names one; such points get no subject
location and are counted (`provenance_subject_mismatches` when both ends had one, else
`provenance_clip_mismatches`, when nothing is lost), about 0.2% of second-step points in the large
generated set and 3% in the dense chain set. A coverage then interpolates such points along their
run, as cut points. An edge whose two ends both have subject locations but lie on one earlier clip's
segment (a crossing to a crossing along that cut) is a cut edge, composed as a cut and not counted.
Only the
subject's locations reach a caller; an earlier clip's composed location only tells an edge along an
earlier cut from a lost location. The cookie cutter offers provenance through its single-geometry
`partition_geometry` alone: `partition_geometries` mixes several geometries' pieces in one list, and
a table does not say which geometry it refers to. A geometry entering the cascade has no table (it
is the original); a partitioner that returns it whole gives it the identity table
(`PolygonPartitioner::create_identity_provenance`), and the clip index is the partitioning polygon's
position in the cookie cutter's order (`get_partitioning_polygons`).

**Scalar coverages** (`src/app-logic/PartitionFeatureGeometries.cc`, `partition_range`). Assign
Plate IDs and pyGPlates' `partition_features` partition a feature's geometry properties with their
coverages through the same `PartitionFeatureUtils::partition_feature` (pyGPlates does not apply
Assign Plate IDs' check that the feature exists at the reconstruction time; it never did; kept in
its own file so that
pyGPlates does not pull `AssignPlateIds.h` and the reconstructions, about 320 GPlates-only files,
into its closure). The cookie cutter is asked for provenance only for a property with a coverage,
and each piece's values are made in three steps: (1) a point at a vertex of the original takes its
scalars, and a point on a segment of the original interpolates its two ends', a ring's last
segment wrapping to its first vertex (the old code read one past the end of the values there);
(2) each maximal run of valueless points along an output ring (cyclic; walked once round from the
ring's first valued point, so that a run wrapping past the ring's start is interpolated too) or
polyline is interpolated by arc length between the valued points at its ends: a cut's two ends are
intersection points valued in step 1 (the developer's decision for cut points); (3) a ring with no
valued point at all (an output made wholly of the clip's points: `Q` inside `P`, or a hole of the
intersection) takes, at each point, the value at the nearest point of the original's outline
(`minimum_distance`, no threshold), at the ratio of the point's projection onto the nearest segment,
clamped (the old fallback used the distance to the point itself, which is the ratio of no point on
the segment). The nearest outline value was chosen (by the developer) over dropping the coverage
from such a piece, or a fixed value such as NaN at each point, so that every piece of a geometry
with a coverage has one. Per-tuple scalars are interpolated independently; a point or multi-point's point
takes its own index's scalars. A coverage that an older file gives for the exterior ring only
(`Coverage::exterior_ring_only`, `ScalarCoverageFeatureProperties.h`) stays in that form. The
polygon is cut whole, and each piece has scalars for its exterior ring alone: a hole the cut opens
into an exterior ring gets values by step 2, and a hole left whole gets none. So, where the input's rings cross, does a ring of the input's exterior
that its group (section 5.2) puts in as a hole, beside a larger ring. The previous mapping, an
epsilon map of the original points and a search for the nearest original segment within 0.5
degrees, relied on every point of a piece lying on the input's outline, which a cut across the
interior defeats. A ring's first point often has no value (whenever a piece is cut a second time,
the second overlay's first stop is on the first plate's cut edge), which is why step 2 walks once
round from the first valued point rather than applying a polyline's end rule.

**Most overlapping plate** (`GeometrySizeMetric` in C++, and the
`PartitionMethod.most_overlapping_plate` branch of `partition_features` in `PlatePartitioning.py`)
compares a plate's pieces by area for polygons, then arc length for polylines, then the number of
points, lexicographically (in C++ with `real_t`'s comparisons; pyGPlates compares the floats
exactly), so that a feature with polygons is decided by
them: arc length, the old measure, is inflated by a polygon's cuts. (Keeping the old measure's
meaning, the length of the input's own outline within each plate, which provenance could give, was
the alternative: area is what overlapping the most means for a region.) The C++ comparison is not a
strict weak ordering, since equality within `real_t`'s tolerance is not transitive; it serves a
running maximum and must not be handed to a sort. The first plate with any piece
is taken whatever its size, so that a geometry of no measure (a polygon whose edges are all too
short for an area, a polyline at one position) still gets a plate, in both products. pyGPlates
measures what splitting would partition (`_partition_feature_geometries`), so a time-dependent
geometry that is not partitioned decides no plate. A piece whose rings cross (section 5.2) is
measured as `PolygonOnSphere::get_area` measures it, its exterior's area less its interiors', which
is not its parity area (section 3.3): a plate whose hole pokes half out of its exterior measures
0.0121 steradians against 0.0304 sampled. A ring that crosses itself is measured by its signed area,
in which lobes wound opposite ways cancel. A piece with two equal lobes measures zero: a bow-tie
with seven eighths of its area in one plate, cut across both lobes, goes to the plate holding the
other eighth.

**Time-dependent geometry.** A geometry property whose value is a `GpmlPiecewiseAggregation` with
exactly one time window containing the reconstruction time is partitioned as a constant geometry,
and its pieces, and a single-window coverage's range, are written back constant (the shared visitor
in `PartitionFeatureGeometries.cc`): with one window there are no others to leave undefined, and
early GPlates builds wrapped single geometries this way. One whose coverage's range has more
windows is not partitioned (written back constant beside a time-dependent range, or cut with one
window's values, it would lose the rest). Any other time-dependent geometry is not partitioned and
stays unchanged on every feature returned (pyGPlates puts back a kept property that shares its name
with a partitioned one, which setting the pieces replaces), and a `GpmlIrregularSampling` never is.
Which properties are partitioned is decided once, in that visitor: when splitting, pyGPlates'
`partition_features` removes exactly the properties it reports as partitioned, so the two products
split alike. With the most overlapping plate they differ. Assign Plate IDs removes the same
properties and puts each geometry back whole: a single-window geometry and its range come back
constant, and an unmatched constant range (below) does not come back. pyGPlates returns the feature
unchanged but for the copied properties (section 7). A
constant range property under a partitioned geometry's range name that matched no coverage (its
number of values is no geometry's, or two same-named geometries share it) is reported as partitioned
too, with a warning, and so removed: copied into every piece's feature, it could be read back as the
coverage of a piece that happened to have its number of points. A time-dependent one is kept: it
normally belongs with a time-dependent geometry, which is kept, and is unmatched for having no value
at the reconstruction time. One that has a value then, but a number of values that is no
geometry's, is kept too, and a piece with that number of points reads it back as its coverage
(removing it could take the coverage of a kept time-dependent geometry). pyGPlates puts back, after setting the pieces, every kept property of the
geometry's name and of its range name, which setting a geometry also replaces.

### 5.5 The sliver filter

A cut leaves *slivers*: pieces of a polygon only metres wide where its outline was digitised along a
plate boundary. `Options::sliver_min_width` (radians; none, the default, is off) drops a ring of an
output whose mean width, twice its area over its perimeter, is below it: an exterior with its holes,
a hole alone (its polygon fills it), counted with the area of the dropped exterior rings (their
own, holes not subtracted, and not counting rings dropped with a parent). It is a distance rule
on the output and nothing else: it merges, snaps and moves nothing, and the overlay's own decisions
never see it (a region thinner than `h` between two boundaries is the true answer, not an artefact).
With it on, the intersection and the difference no longer partition the subject exactly: a sliver
that is a separate ring in one output and part of a larger ring's boundary in the other belongs to
neither. The result contract is evaluated after it (section 5.1).

The measure is `PolygonOverlay::get_mean_width`, not `SphericalArea`: `SphericalArea`'s fan drops
the triangle of every edge `GreatCircleArc` takes for zero length, under about `1e-6` radians, and
a cut makes such edges next to a vertex, so at a width of `h` each is an error of up to about a
quarter of a sliver's area and a disc drawn with edges under `h` has no area at all. The function
sums Van Oosterom and Strackee's triangles from an apex, modulo `4 pi`, and takes the smaller side,
so the ring's direction does not matter; the perimeter is by `atan2`. The sum is the area for any
apex no ring point is antipodal to *and that is not in the plane of a ring lying on one great
circle*, where every triangle is degenerate (each term `0` or `2 pi` by the sign of its
denominator). So the vertices' mean direction will not do as the apex: for a ring on a great circle
it lies in that plane, a 36-point equator ring would measure `0`, and the default filter would drop
a hemisphere's exterior and report the hemisphere wholly inside a 10-degree box. The apex is the
ring's vector area, `sum(a_i x a_(i+1))`, normalised: the plane's normal for a great-circle ring,
near the centre for any other; where its `magSqrd()` is within `real_t`'s tolerance of zero (about
`1e-12`: a ring under about `5e-7` steradians, whose mean direction is near its centre) the mean
direction, and if that cancels too the first vertex. The sum is signed by winding
number, so for a ring that crosses itself it is not its parity area, and lobes wound opposite ways
cancel: such a ring, and a group of rings that cross each other, are not dropped themselves
(section 5.2, which says which self-crossings are seen).

**The widths.** `GeometryCookieCutter`'s default is `10 h` (`get_default_sliver_min_width`, about
90 metres), so it covers Assign Plate IDs and pyGPlates' `PlatePartitioner` and
`partition_into_plates`, whose `sliver_width_threshold_radians` is `None` for the default, zero for
off, and raises `ValueError` if negative or NaN. `PolygonOnSphere.partition()` takes the same argument and
defaults to `h`: two outlines within `h` count as touching, so a ring narrower than `h` on average
has collapsed by the overlay's own rule (the hunt found such rings of no area, bounded by an input
that overlaps itself along a stretch), and a cut always separates boundaries more than `h` apart,
so what `h` drops is thin triangles and degenerate rings. `PolygonPartitioner` keeps none as its
default, so a caller that asks for no filter (the raster meshes' unreached path among them, section
5.1) gets an exact cut. So the three widths are `h` (the overlay's resolution),
`10 h` (global data's) and zero (exact). The decision came from measuring Assign Plate IDs' cut of
the Müller et al. 2022 coastlines (2,937 polygons, 100,000 points) by its topologies at 0, 100 and
200 Ma: of the 783 output rings of the 282 polygons cut, 5 were under `h` wide, 16 under `10 h`
and 73 under `100 h` (0.47, 4.5 and 373 square kilometres). The thinnest are triangles where a
continental outline was digitised along a plate boundary (South New Zealand and the Campbell
Plateau, the San Jorge plate, Western Cameroon at 100 Ma, 71 km long and 5 metres wide): below `h`
the overlay cannot tell the two outlines apart; up to `10 h` they are artefacts of the digitising,
below the resolution of global data, and would take the neighbouring plate's ID; at `100 h` the
filter starts on pieces that may be real small geography. Filtering the pieces afterwards is not
the same: it cannot drop a sliver hole, return the geometry whole when a side empties, or measure
short edges exactly (`get_area` is `SphericalArea`'s). On that data the overlay added no near-zero
output edges of its own (every edge under `h` was an input's own), so no filter of them was built.

### 5.6 Failure

The overlay never returns a best guess as if it were the answer. Classes that describe no consistent
arrangement after repair, or a piece left unclassified, throw
`PolygonOverlay::InconsistentOverlayException` from `classify()` (a `MathematicalException` carrying
the `Diagnostics` and a position: the first failing vertex's, with its latitude, longitude and
record kinds in the message, or a hemisphere check's reference point), and on a whole answer's
hemisphere check; `extract` throws it on a hemisphere failure and a pairing failure. The symmetric
difference is made of the two differences' rings (section 5.2), so its pairing failures are theirs
together, and it throws wherever either difference would. A grouping
failure (above, rings whose overlap the crossing test misses) and coincident rings
(`coincident_rings`: rings of no area that clean-up missed, or a ring an invalid input repeats,
section 7) are counted only.
`Options::throw_if_inconsistent` (false for hunts and tests) counts instead, and the rings are then
returned as read, which a caller must not take for the output. The causes a check failure can have:
a missed or spurious intersection, a band piece's midpoint test propagated where the margin order
should have prevented it, or a vertex the vertex rules could not resolve. The sliver filter is
outside this guarantee: it is a distance rule, and a wrong measure changes a result silently (a ring
on a great circle, and a ring that crosses itself, section 5.5, are why the measure is as it is). A
ring that crosses itself only within a few margins can still be dropped (section 7).

`PolygonPartitioner` and `GeometryCookieCutter` let the exception propagate, with or without lists
(every plate must get an answer, and a wrong one is worse than none); without lists the cookie
cutter stops at the first plate that claims a piece (its answer is then known), so only the plates
it consulted can raise. pyGPlates raises it as
`PartitionError` (a `MathematicalError`, `src/api/PyExceptions.cc`) from
`PolygonOnSphere.partition()`, `PlatePartitioner.partition_geometry()`, `partition_features()` and
`partition_into_plates()`, and from the Boolean methods `PolygonOnSphere.intersection()`,
`union()`, `difference()` and `symmetric_difference()` (section 5.1). For `union()` and
`symmetric_difference()` it is also the answer to a result whose outer ring would enclose more than
a hemisphere. The overlay's hemisphere check cannot tell that from wrong classes (section 5.2), and
wrong classes, its only possible cause for the intersection and the differences (which cannot
exceed an input), have never been seen, so one exception serves both. A separate exception for the
hemisphere case was rejected
(developer, 2026-11-13): the overlay could only guess which of the two it was, and the name fits a
union too, since the overlay cuts both polygons into pieces.
GPlates' `AssignPlateIds::assign_reconstruction_plate_id` catches it around each feature's
partition task and records the feature (`get_features_not_partitioned`); a task partitions every
geometry before it modifies the feature (`GenericPartitionFeatureTask`, as `PartitionFeatureTask`
requires), so the feature is left as it was. The dialog shows one
warning at the end, naming up to ten features by name and ID and counting the rest, also when the
user cancels part-way; the command line writes one `qWarning` per feature. Aborting the whole
operation would lose the work on every other feature for a case no real data has produced.
`GLReconstructedStaticPolygonMeshes` catches it itself and takes the node as intersecting, which
would cost rendering work on a node the polygon may not reach and nothing else (on a path nothing
reaches today, section 5.1).

One generated case is known to fail, and is the failure tests' input
(`src/unit-test/data/polygon-overlay/inconsistent.txt`). Both its inputs are invalid: each has a
hole crossing its own exterior ring, and with either hole removed the cut succeeds. Along a dense
shared edge whose long stretches are not paired as common and lie within `0.15 h` to `0.35 h` of
each other throughout, midpoint tests on band pieces `1 h` to `1.7 h` long fix a chain's class
wrongly, and repair cannot find the flip of several pieces it would take within 20 rounds. Consistent labellings exist, so the
vertex orders are not at fault. Untried remedies: a rule for pieces running along the other outline,
or a repair candidate flipping a chain of band pieces. It was one of 67,639 dense shared-edge cases
among 119,198 generated, and no other generator and no real data produced one.

### 5.7 Rejected alternatives

**Greiner-Hormann tracing with Foster, Hormann and Popa's labelling** (the other candidate
algorithm; `DateLineWrapper` is Greiner-Hormann against a degenerate clip, section 5.8). Each
intersection is labelled crossing, bouncing, or the start or end of a delayed crossing or
bouncing, and the output is traced by the labels. Rejected because one wrong label inverts the rest
of the ring silently, where the overlay's wrong class affects one piece and is caught at the
vertex; because it needs two traces with reversed rules for the two outputs, and a separate version
for polylines, where one classification serves every operation and polylines; and because its one
documented failure, a twist of `P` lying on `Q`, is two ordinary vertices here, where the tracing
would need `P` split into simple rings first. Its case table for the intersection phase was the
starting specification of what partitioning needs from `GeometryIntersect`, its chain reasoning defines the common pieces,
and its split-candidate clean-up is what the pairing and the split achieve. Classifying the
stretches between intersections by point tests is close to Kim and Kim's alternative, which Foster,
Hormann and Popa set aside as costing a point test for each of `k` intersections, `O(k n)`; with
point-in-polygon's bounds and lune tree (sections 3.7 and 3.9) the tests are `O(n + k log n)`, the
order of the intersection phase. The nearest published relative is Martínez, Rueda and Feito's
planar sweep (2009), which also subdivides both polygons' edges at every intersection, labels each
sub-edge on its own and joins the kept ones into rings afterwards, with a separate rule for
overlapping edges ("same transition" or "different transition"); the sector rule of section 5.2 is
that rule's reasoning applied at a vertex, and is not itself from a paper. Tracing by the labels and
checking each traced piece by a point
test, with the overlay as the fallback, was also rejected: two algorithms to maintain for the
robustness of one.

**Exact arithmetic** (an exact arrangement of geodesic arcs, as CGAL's, robust at every degeneracy).
It would not change the answers that matter here: plate boundaries that should coincide agree only
within epsilon (section 3.5), so an exact overlay would compute, faithfully, the slivers between
them. The thickness is what makes two such boundaries one.

**Every record its own vertex** (no grouping). Zero-length pieces between records at one point can
only be deduced, never tested; the most midpoint tests of any variant; the only variant whose
output depended on the tie order; 33 of 300 cases of one generator failed; and a corner's two
records *are* one point.

**Identity without the loop rule**: two SS records at one subject vertex make a loop arc with no
test point and no usable direction (19 catalogue cases).

**Contracting pieces shorter than `2 h` into clusters** (and making any arc with both ends in one
cluster internal). It fails in exact arithmetic (19 of 300, 3 of 60 and 18 of 299 cases of three
generators): a subject edge running down a neck of `Q` narrower than `2 h` is two four-end vertices
under identity, each resolved without geometry, and contraction joins them into one six-end cluster
whose rays leave from different member positions, so their order round the cluster is not the order
at any one point. It also moved the output by up to `20 h`, dropped real thin regions, removed the
anchors of common chains (a touch inside a cluster is internal), and was the only variant merging by
distance, with a threshold tied to `GeometryIntersect`'s. What contraction bought, few midpoint
tests and no tie dependence, the LL rule and the margin order buy as well. A tolerance pass on the
inputs before the overlay (snapping) fails the same way at every cluster and adds a second meaning
of "the same point".

**The rotational order at four-end vertices.** The records encode the snapped topology and the rays
the unsnapped one; under about `2 h` they disagree, and 10 catalogue cases failed.

**The sector walk where one input passes once** (section 5.2, the one-passage rule), **and one
local rule per mechanism** found there: an along rule, a gap cone (a common ray's cone at least its
two tangents' angle), and vertices where one input is silent. The first two each fix one mechanism
only. The third, deducing and checking nothing at a vertex where only one input has ends left, is
wrong: where a tiny hole of `Q` inside `P`'s band is removed whole by the loop and spike rules, the
two such vertices it leaves on `P`'s ring are where the sector walk makes `P`'s pieces either side
take one class (`in Q` is constant round a vertex with no end of `Q`), and without that the band
piece between them took an arbitrary midpoint class that its exact neighbours contradicted. So
`solve_vertex` runs at every vertex, whatever ends it has.

**No spike rule**: as clean, but 77 degenerate rings dropped in the catalogue against 48, and 10 to
40 more midpoint tests per 300 dense shared-edge cases.

**Chord directions for the rotational order, and a one-sided cone for common rays**: chords of
snapped arcs can cross with no record, and the one-sided cone needs an exact symbolic tie.

**The output side of each ring from its pieces' classes** (the first design of the orientation and
the hemisphere check: the keep-rule's side of each kept piece, with the input's containment forest
and the ring's signed area). A band piece's class is arbitrary, and the output ring runs between the
vertices' output positions, not along the piece, so a piece about `h` long put the output on the
side opposite to the ring's; a length-weighted vote fixed those but not rings whose every piece is
a band piece (holes `0.3 h` to `1.5 h` across, a false hemisphere failure for a union of two small
polygons); and an input whose rings cross has no one side per ring (every twist's union and
symmetric difference threw). Grouping by that output side, instead of parity depth, was rejected
with it.

**The symmetric difference walked by its own pairs** (the design until the Boolean operations were
exposed; section 5.2 has the two mechanisms): crossing ring groups from inputs whose rings do not
cross, in 37 of the hunt's 39,313 such cases, with every point of the output right. Two repairs of
it were rejected with it. An order for the split's cuts: the vertex to cut first is the one whose
pairs cross, which is what is not known, and each of the two orders tried fails somewhere (37 cases
one way, 47 the other). And joining the two differences' rings across every wall: the 25 cases
where a wall separates nothing remain. A third was not tried: a better order for two parallel rays
of one input (an input whose rings do not cross has no two passages of its own crossing at a
vertex, which picks the right order where two holes' edges leave a vertex side by side). It changes
the order `solve_vertex` uses, which the prototype comparison pins, and does nothing for a wall
that separates nothing.

**The symmetric difference as the two differences side by side**, unjoined
(`polygon.difference(other) + other.difference(polygon)`): always as robust as the differences, and
never over a hemisphere, since each piece lies inside an input; but two polygons sharing an edge
would come back as two pieces along it rather than one polygon, unlike `union()`. Rejected by the
developer (2026-11-13), who wanted shared edges merged and a result over a hemisphere reported, as
for the union. The join keeps the differences' robustness, since it starts from their own rings
(section 5.2).

**For the ring twisted at a chain** (section 5.2). *Cutting wherever an arc of a ring passes one of
its vertices*, a touch with no test of a crossing (the first form of the rule): it cuts a ring in
362 of the hunt's intersections and differences from valid inputs, where the rule cuts 10 (206 of
them from the `tinyhole` generator and 78 each from `shared` and `zero`: a ring comes back within
the thickness of itself beside a tiny hole or along a dense shared edge without being twisted), and
partitioning's outputs there are right as they are. *The order of the rays at the chain's
two ends*, with no geometry: the measured orders do not show the twist (in the catalogue's case the
ring turns the same way about the chain at both ends). The common ray is measured where the
subject's boundary is, and the arc is beside that, between it and the clip's. *The chain's length*
(cut when the two vertices are within some distance): a second tolerance, and the arc runs along a
chain of any length. *Another margin for the far points* than the thickness: with half a thickness
one more of the hunt's valid differences is cut (a part of `0.77 h^2`), and the grouping's `10 h`
would by construction leave a twist up to `10 h` wide; the thickness is the scale at which the
records themselves decide a crossing. *A cut at the passed vertex alone* (the arc given a stop
there, and each part closed at it): both parts would then be simple as drawn, where the part that
keeps the arc now keeps a sliver wound backwards (section 5.2); but the part cut off would run from
the passed vertex and not along the arc, moving that edge by up to `h` over its whole length, and
its first point would be at a vertex its piece has no stop at, which provenance and the symmetric
difference's join both assume it has. Not built. *A stop for the arc at the vertex it passes*,
before classification: untried.
It is what the self-intersection mode of section 7 would give, and it changes the pieces
themselves, which the prototype comparison pins record for record (three pieces would then join
the two vertices where the pair does, which the common-pair rule does not pair).

**Dropping one of a corner's two records**: section 4.5.

**A parallel list of provenance tables beside the geometry lists** couples two lists by position
across three consumers; the table travels in `PartitionedGeometry` instead.

**Repairing silently and returning in release builds** (the first proposal for a check failure):
the developer's requirement is never a silent wrong answer.

**Partitioning by all plates in one overlay**: a different algorithm, for a spatial index of many
polygons. Cutting the original geometry by every plate independently, so that no rounded piece is
fed back, and giving each piece to the first plate that contains it, is sound only where plates do
not overlap, which topological plates satisfy and static polygons do not: the part of a piece
inside an earlier, overlapping plate has to be removed, a difference against the earlier plates,
which is the cascade again. A full arrangement of the geometry with every plate boundary, each face
labelled by the plates containing it, would need the plates overlaid with each other, where every
shared boundary is a common chain, and serves no present caller better than the cascade, which is
safe as it is (section 5.1).

**A side rule for a polyline piece along the clip's outline** (inside when the clip's interior is
to its left, say, where the code gives every such piece one fixed class, section 5.2). Decided by a
test point offset to one side, it fails in a face thinner than the offset, as the offset point did
for anchorless chains, and it would be asked exactly where such pieces occur, thin faces and dense
shared edges. The fixed class has no test and no failure mode, matches the inclusive threshold of a
point query, and was measured to change nothing but the along pieces themselves.

**Asking once whether each input's rings cross**, and running the crossing tests of the grouping
(section 5.2) only on the output of an input whose rings do: output rings can cross only then, a
plate could be asked once for every feature it cuts, and a cascade's later steps cut pieces whose
rings cannot cross if the feature's do not. Not built: on the coastline data of section 6
`partition_features` took the same time with the tests as without, to within a few percent, and
the question needs a form of the self-intersection mode of section 7 that stops at the first
crossing, to see a ring that crosses itself.

**A coverage form of a geometry-level entry point**: `PlatePartitioner.partition_geometry`,
`PolygonOnSphere.partition()` or the Boolean methods taking `(geometry, scalars)` and returning each
piece's scalars, or `DateLineWrapper.wrap` and `to_tessellated` likewise. Rejected (developer,
2026-11-13): a coverage belongs to a feature's geometry property, so coverages stay at the feature
level, where Assign Plate IDs and `partition_features` carry them (section 5.4), and a
geometry-level form would need rules the feature level does not (a union's point can come from
either input's coverage, or from both where they meet). `partition_features` calls a
private helper (`PlatePartitioner._partition_feature_geometries`) that returns each property's
pieces with their coverages and the positions of the properties partitioned. No two pieces with a
coverage share a feature under one property name (pyGPlates gives each its own feature; Assign
Plate IDs reuses a feature only while that name is absent from it), because two coverages with the
same number of points under one name would be ambiguous on read-back.

### 5.8 Clipping by `DateLineWrapper`

`DateLineWrapper` (`DateLineWrapper.{h,cc}`) clips polylines and polygons to the dateline for the
rectangular map projections: a geometry that crosses longitude 180 is cut into pieces that each
stay on one side, so that no piece draws a line across the map. It also wraps longitudes into
`[-180 + central_meridian, central_meridian + 180]` and can tessellate the output. It is separate
from the overlay (section 5.2) and shares no code with it or with `GeometryIntersect` (section 4.1):
it finds its own crossings, against its own tolerance. The header cites Greiner and Hormann (1998) and, to a
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

`wrap_polygon` and `wrap_polyline` first pass through a geometry whose bounding small circle misses
the dateline arc (`possibly_wraps`, `intersects_dateline`), converted to lat/lon but otherwise
untouched. Otherwise they rotate it about the north pole into the frame
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
`CLASSIFY_ON_SOUTH_POLE` (a cap of angular radius about `4.5e-5` radians, tested first because, the comment
says, it eats into the region tested next) and sets the touched-pole flag. Otherwise an exact sign
against the hemisphere containing the dateline puts it `CLASSIFY_ON_DATELINE_ARC` or, near
longitude zero, `CLASSIFY_OFF_DATELINE_ARC_ON_PLANE`. The thickness (section 1) is about `4.5e-5`
radians, some 32 times `GeometryIntersect`'s thickness and the point-on-outline threshold (which
are the same), and is not derived from `EPSILON`.

**Finding crossings.** `add_line_geometry` classifies the vertices and hands each segment with its
two classes to `add_line_segment`, a six-by-six case table; a ring's first vertex is handled as the
end of its last segment, a polyline's first vertex is added first unless it is on the arc or a
pole. Only a segment going from front to back, or back to front, is intersected. Every other case
is decided from the classes alone:

- front to front, back to back, front or back to off-arc-on-plane and the reverse, and off-arc
  to off-arc: the end vertex is appended as a regular vertex (`add_vertex`);
- a segment ending on the arc (front or back to on-arc): one intersection vertex at its end point,
  on the front or the back dateline, flagged as exiting. The next segment, starting on the arc,
  adds an intersection at its start, flagged as entering, and then its end vertex, if it ends on
  the front or the back. So a vertex on the dateline between two front-or-back vertices produces
  **two intersection vertices at the same point**, one for each adjacent segment. A crossing
  through the vertex puts one on each side, and a touch puts both on the same side, an exit then
  an entry. A segment starting on the arc and ending off it on the plane adds a pole vertex at its
  end instead, and one ending on the arc or at a pole adds nothing (below);
- a segment with both ends on the thick plane, one on the arc and one off it, passes a pole.
  `does_line_segment_on_dateline_plane_cross_north_pole` decides which. It takes the dot product
  of the cross product of the end points (not normalised, so small for a short segment) with the
  plane's normal. The sign, together with which end is on the arc, picks the pole. Its own epsilon
  of `1e-4` applies here, and below it the sign of the end point's `z` decides instead. The
  intersection is at the pole, with the longitude of the off-arc end point. Its ratio is the start
  point's distance to the pole over the segment's arc length, or `0` for a zero-length segment;
- a segment with an end at a pole: an intersection at the pole with the other end's longitude,
  and the end vertex when the segment leaves the pole;
- both ends on the arc or at poles: nothing is added, the segment being outside the dateline
  polygon (the comments say "as odd as it sounds"). A ring of such segments adds no vertex at all.

`intersect_line_segment` handles the front-to-back case. A zero-length segment returns its start
point (the comment says this cannot happen, since the ends are on opposite sides of a plane wider
than such a segment). The dateline is a half circle from pole to pole, so every segment's plane
separates its end points, and only the second of section 3.4's two tests remains, the orientation
test (2). The south pole's signed distance from the segment's plane, beyond the thickness
sine, must have the sign opposite to the segment's start point's side of the dateline plane;
otherwise the segment crosses the dateline's great circle on the far side of the globe and there is
no intersection. If instead the south pole is on the segment's thick plane, the segment's great
circle passes within the thickness of both poles. Each pole is then tested for lying on the
segment itself, with two exact signs (closer to the start point than the end point is, and on the
half circle starting at the start point). A pole that passes is returned as the intersection. If
neither passes, there is no intersection. `calculate_intersection` interpolates along the
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
dateline copy by a linear scan on that one key, between the right pair of corners
(`splice_self_before`). For the south pole, the scan runs from the south-back corner to the end
of the list, since the south-front corner is the list's head; an equal key goes before the vertex already there. Since the clip polygon's
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
copy starts the next. A touch at a vertex splits the polyline there too, both pieces carrying the
point. A stretch along the dateline is dropped, and a polyline entirely on the dateline is output
unchanged (`output_polyline_if_entirely_on_dateline`).

**Where each output point came from.** Every geometry vertex carries an
`InterpolateOriginalSegment`: the ring index, the segment index and the ratio along the segment.
There are two conventions for an original vertex:
- **Through the graph**, it is recorded as the *end* of its incoming segment (ratio 1; a ring's
  first vertex as the end of its last segment). A polyline's first vertex is the exception: ratio
  0 of segment 0. A ring that goes through the graph without intersecting keeps this convention,
  and comes out starting at its second vertex.
- **Passed through by `output_input_line_geometry`**, it is recorded as the *start* of its
  outgoing segment (ratio 0, segment index equal to the vertex index). That happens only when the
  geometry's bounding circle misses the dateline, or the geometry lies entirely on it.

A computed crossing has the ratio `acos(dot) / arc_length`: from `calculate_intersection`, or the
same formula with the pole, from `intersect_line_segment` or `add_line_segment`. A vertex on the
dateline has 0 or 1 and is unwrapped.

`LatLonPolygon` returns, for each output point, flags (`ORIGINAL_POINT`, `ON_DATELINE`,
`ON_ORIGINAL_SEGMENT`, `TESSELLATED_POINT`) and an optional record (ring, segment, ratio).
`LatLonPolyline` returns three flags, without `ON_ORIGINAL_SEGMENT` since every polyline point is
on one, and a record for every point, with no ring index.

Tessellation (`add_tessellated_points`) subdivides the arc between consecutive untessellated
points. A tessellated point on an original segment gets a ratio interpolated between its end
points' records. Its both-original-points branch, selected by `is_unwrapped_point`, reconciles the
two conventions above from the two segment indices and from whether the end point's ratio is above
`0.5`. One
along the dateline has no record and its longitude pinned to the end points'. A corner vertex has
no record and no flags at all, being neither original nor an intersection. The arc between the two
poles, antipodal, which the comment attributes to the arbitrary entry/exit choice when both poles
are touched, is tessellated in latitude.

**Degeneracies.** Nothing is perturbed. A vertex near the plane or a pole is snapped by the thick
classes; a vertex on the arc becomes a pair of intersection vertices, which makes a crossing
through a vertex and a touch at a vertex both consistent with the toggle (a touch gives an exit
and an entry at one point, the traversal leaves the geometry at one and returns at the other, and
the output ring repeats the point; since the walk starts from the exit copy, it runs the geometry
backwards, so the output's orientation is not preserved); a stretch along the arc is dropped and a whole geometry on it
passed through; a zero-length segment that still straddles the plane intersects at its start
point; a near-antipodal segment, or one whose ends are both too near the plane, gives no
intersection; and the both-poles case is guessed. Its main tolerance, the thick plane, is one
global number whose size the code does not justify, and not the one the rest of this document uses
(section 1 lists its other epsilons).

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
  a crossing-or-touch classification. Here each of the pair's flags is fixed by whether its segment
  arrives or leaves. The neighbouring vertex's class decides which edge of the clip polygon the
  copy lies on, which a general clipper would have to decide by a side test.

The overlay takes nothing from this code: tracing by entry and exit flags was rejected for a general
clip polygon (section 5.7), and putting `DateLineWrapper` onto the overlay is deferred (section 7).

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

The C++ tests of `GeometryIntersect` (`src/unit-test/GeometryIntersectTest.cc`) build most of their
geometries in local tangent coordinates at a centre on no axis, with distances in units of the
thickness, so that each case stays where it is meant to be if the thickness changes. They cover a
crossing; a vertex shared bit for bit, at an inner vertex, at a polyline's first and last vertices
(the fictitious segment, on either or both geometries) and at a polygon ring's first vertex; a
touch at `0.99` thicknesses and none at `1.01`, with the touch's projected key; overlaps
(interleaved, reversed, along a whole edge, round a corner), where every vertex in the stretch is
an intersection and nothing else is; zero-length segments straddling an arc and at a vertex; short
crossing segments at right angles and at 20 degrees (two touches from a segment three thicknesses
long); the ordering of a touch before a crossing just beyond it; segment indices through the
rings, a contact at a ring's first vertex, and a hole touching its exterior (one intersection per
ring); a near-antipodal arc and nearly parallel arcs; pruning over a 300-segment polygon with two
holes, both ways round and against a one-leaf polyline, and two polylines end to end `0.99`
thicknesses apart, whose bounding circles meet only when expanded; the corner wedge at a right
angle and at 30 degrees; symmetry (`intersect(A, B)` against `intersect(B, A)`); clearing a stale
`Graph`; a self-crossing on an arc; and the two inequalities of section 4.4, with the thickness
equal to `s`. Reverting the projected key makes the ordering case fail, and removing the bounding
circles' expansion makes the end-to-end case fail.

`GeometryIntersect` itself was checked against an exact reference: the specification of
`intersect_segments` in rational arithmetic on the double inputs (thick planes, coincident vertices
first, the four types, the fictitious segment, ring wrap-around, the projected key), with a band
round every decision, tolerant or exact, so that a case decided within rounding of a threshold is
skipped. A hunt observed `GeometryIntersect` through `PolygonOnSphere.partition()`, whose pieces
then ended at the intersections along the polyline, on generated cases: shared vertices and collinear runs
offset across the thickness, nearly parallel and nearly antipodal arcs, short segments and
self-crossings on an arc, geometries `1e-5` radians across at a pole, arcs of 100 to 170 degrees,
pieces fed back in, against the same polygon and against copies jittered by `1e-13` to `1e-8`
radians, and windows of adjacent plate boundaries from the Müller et al. 2022 model, both
bit-identical and moved sideways. 14,000 cases (8,000 with the second geometry as a polyline and
6,000 as a polygon ring; 36,559 intersections) gave no mismatch. The same hunt, with the reference
modelling the code as it was before the thickness was raised and the key changed, gives 319
mismatches in 2,000 cases, so it is sensitive to the two changes together. The observation cannot
see an intersection positioned exactly at the polyline's first or last vertex when it is the only
one there, cannot order two occurrences of one position with another between them, and saw nothing
on a polygon's interior rings, since partitioning then intersected exterior rings only (the
overlay now intersects every ring); those cases are covered by the C++ tests.

In C++, `GeometryIntersect` is also reached through `GenerateVelocityDomainCitcomsTest.cc`
(`citcoms_meshes`, which compares generated meshes with stored files and fails if the
intersection's point is replaced by a line's end), through the overlay tests below, and from Python
through the topology and partition tests. `GreatCircleArc::intersect` is exercised through the
distance tests above.

The overlay's C++ tests (`src/unit-test/PolygonOverlayTest.cc`) build families of the prototype's
catalogue in local tangent coordinates at a centre on no axis, with distances in units of the
thickness, and check the structure stage by stage (vertices as record sets, stops, pieces, internal
pieces, common pairs, rays with their cones) and then the classes and the output: two squares, a
clip inside the subject, a hole crossed only, the acute wedge, a twist on the clip, two clip
vertices at one subject vertex, a shared edge, a polyline; the loop and spike rules; a crossing's
and a far end's offsets in the cones; the classification of the apex on an edge, the neck, the
T-intersection, the touch rule, touches returned whole, a short segment's self-touch, anchorless
chains and a near-copy's anchorless chain past a short clip segment, the LL rule (an octagram), and
a whole answer's hemisphere check (and its skip for a subject too thin for it); repair by a
single flip and by a whole chain, and the throw
without repair; linking (two squares, a clip inside, a pinch, and the catalogue families under every
operation against the oracle; a ring twisted at a chain `0.37 h` and `140 h` long, the
inputs either way round, with a notch along a shared edge that is left whole; and the symmetric
difference's join, where two holes' parallel rays leave a vertex and where a wall is a spike);
grouping (holes in both inputs, crossing holes, islands in holes, the
containment test directly, inputs whose rings cross, a hole across its exterior, two rings like a
plus sign, rings meeting only at shared vertices both ways round, groups three deep, and the
crossing tests directly, between rings (a pinwheel among them) and of a ring with itself (a bow-tie
with a vertex at its crossing, and a touch that is not a crossing), and against the same rules
written as plain loops over 400 generated pairs of rings), orientation independent of the
inputs' and a union
over a hemisphere throwing; self-touching inputs and the pairings only a rare vertex shows; polylines (across, no
record, along under both settings, a touch and an end on the edge, a pinch, a hairpin across the
clip and along its edge); and provenance (each kind of point, a two-plate cascade, a polyline, a
split at a vertex holding no subject vertex). The **oracle** (`PolygonOverlayOracle.h`) reads an
output by parity over its rings: for random points farther than `M = max(10 h, twice the widest
record cluster)` from both outlines, a point is in an operation's output exactly when `f(in P, in
Q)`; the intersection's and the difference's areas (each ring by a fan of Van Oosterom and
Strackee's triangles, signed by its depth among its polygon's rings) sum to the subject's, within `2
h` times the inputs' perimeters over the area, for inputs whose rings do not cross; points sampled
along a polyline lie within `2 h` of a run of their class; every output point's provenance resolves
to it; and a point beside an edge clearly along the subject has a subject location. No exact
reference is needed, since point-in-polygon was audited separately. A ring twisted between two
vertices (section 5.2) reads right by parity, point for point, so the oracle's points pass it and
only an area shows it: the linking's two tests above also sum every operation's area against the
inputs' (`check_areas`: the intersection with each difference, the union with the intersection, and
the symmetric difference against the two differences, each within four thicknesses times both
perimeters).

**The prototype comparison** (`PolygonOverlayPrototypeTest.cc`). The overlay was derived by a
Python prototype in exact rational arithmetic, run on a catalogue of configurations with several
records at or near one point and on random generators (`near`, `chain`, `tinyhole`, `rays`, `six`,
`zero`, `shared`, `holes`), and the C++ is compared with frozen snapshots of it, record for record,
piece for piece and class for class, under `src/unit-test/data/polygon-overlay/`: the catalogue in
every tie order (`GeometryIntersect` orders records along a segment by a cosine in doubles, so
records whose cosines agree within a few ulps come out in either order where the prototype's exact
keys see no tie); a sample of every generator, with the second run's residue and the cases that
alone show a pairing; polylines from the prototype's polyline generators and opened rings;
anchorless chains; and the bundle vertices, whose counts of floating rays, admissible orders and
consistent placements are pinned. Classes compare exactly except a band piece's (and a common
pair's in any case where a band piece's differs, since a chain's class can follow a band piece's
through the sector walk; the comparison does not trace that it did); output rings compare by the
oracle for every operation, and as rings for the intersection, the difference and the union where
the prototype's did not depend on the tie order, no band piece differs, and the rings do not touch at
two vertices (which decompose either way), and the overlay did not cut a ring at a chain, which the
prototype does not do (those ring sets are counted, and the count pinned for each committed file:
two, both in the generators' sample, a difference of inputs whose rings cross and an intersection
of valid inputs); the reverse difference's and the symmetric
difference's rings are checked by the oracle only. Output polygons compare by the oracle for every
operation, and for inputs whose rings do not cross also by orientation and by having no crossing
group of rings, in any operation (a touch taken for a crossing, or rings paired or joined across
what does not separate them). Any repair of generated
data fails the case, and the one-passage rule's and the result contract's cases are pinned
(`OnePassageRule`, `PartitionerCollapsedRegion`), as are the cases with a ring cut at a chain, by
operation, and the walls the symmetric difference leaves unjoined, with the inputs as exported and
swapped (`CutsAtChainsAndSpikeWalls`: the catalogue's 13 `d4 hole-between` cases and two of the
generators' sample; 13 walls and 10). The comparison relies on the overlay being deterministic: no
unordered container's order reaches a result (`find_common_pairs` keeps its keys in the order first
seen), and each floating-point operation is rounded on its own, so the build turns the contraction
of `a * b + c` into one fused instruction off (`-ffp-contract=off`, `src/CMakeLists.txt`); with it
on, as Clang has it by default on arm64, records tie and sign tests fall differently and the
comparison fails. The prototype, its generators and its exporter
are kept outside this repository (its configuration name is `r5p`, which mirrors for polylines the
spike rule's restriction to polygon rings and the along rule for a hairpin along the clip's edge,
section 5.2); a larger export is run through
`GPLATES_POLYGON_OVERLAY_PROTOTYPE_CASES`. The hunt at scale ran 119,198 generated cases, and is
what set the grouping margin and the cone's offsets (section 5.2); 9 polygon cases differ, none in a
partition's geometry: 4 with coincident rings (at least two of them rings of no area), 4
second-step provenance losses with no point's provenance wrong, and one where two crossings `1e-16`
radians apart are two vertices in the C++ and one in the prototype, the output right (the first
eight are in section 7). One more case fails in both, the inconsistent cut of section 5.6. The
70,291 cases whose input rings cross are checked as polygons too, and none is wrong; with crossing
rings kept as separate polygons 11,212 would be (polygons overlapping), and 3 with a crossing test
of vertices and edges alone, without the edges' midpoints (rings meeting only at shared vertices).

The hunt was run again, before and after the linking's two rules (the cut of a ring twisted at a
chain and the symmetric difference's join, section 5.2), on a fresh export of the same 119,198
cases. The prototype's ring decompositions vary from one export to the next, and with this one a
tenth case differs before the rules and after alike (`six#13 seed 11`: three intersection rings
against the prototype's two, which share one point). The same ten differ after as
before, for the same reasons. Before, 37 more had a crossing group of rings in the symmetric
difference from inputs whose rings do not cross; after, 96 ring sets of the intersection, the
difference and the union are not compared, for a cut at a chain. Three scans of the 109,613 polygon
cases stand beside the comparison, each with the inputs as exported and swapped:

- every operation's area against the inputs' (`check_areas`' sums), in the 39,313 cases whose
  rings do not cross: none off and no crossing group in any operation, where 26 cases were off
  before as exported and 41 swapped, all in the symmetric difference;
- every output ring searched for two of its edges crossing with the two parts wound opposite ways,
  in the 39,320 cases with valid inputs: before and after, none whose narrower part is over a
  thickness in mean width, and about 270 with one under half a thickness (the catalogue's 13
  twisted rings were `318 h`);
- every operation's polygons compared with the build before the rules, coordinate for coordinate:
  the intersection, both differences and the union are the same but where a ring is cut at a chain
  (the 10 valid cases of section 5.2; and, of the cases whose input rings cross, 59 as exported in
  the intersection or the difference, each with one ring more). The symmetric difference's rings
  are made another way and differ in most cases, in their order and start if nothing else.

The partition layer's tests (`PolygonPartitionerTest.cc`) cover the three old no-intersection bugs
(a partitioner inside the subject, a subject across a partitioner's hole, a partitioner across the
subject's hole) and a piece along the boundary for both orientations; the result contract (touches
and identical inputs returned as the same object, with and without lists); the bounding-circle
shortcut for an interior past its circle (the helix band of section 3.3, both ways); provenance (a
multi-point and a piece keeping two holes through the partitioner's own entry; through the cookie
cutter a cascade of two plates with a hole across their shared edge and the input's closing
segments across both, a far plate between them returning a piece whole, and the other geometry
types); and
the sliver filter (an exterior with its hole, a hole alone, a hole with an island, both sides
emptied, the cookie cutter's default, the mean width, a ring on one great circle, and a bow-tie,
with a vertex at its crossing on one diagonal or both, and as the plate); inputs whose rings cross
(a plate whose hole crosses its exterior, rings meeting only at shared vertices, and a pinwheel,
each as the partitioning polygon and as the subject, sampled as the union of the pieces as a
feature's polygons are read); and the
cookie cutter's options (grouped or not, sorted by plate ID or area, every speed) on a plate with a
hole and the plate filling it. `PartitionFeatureUtilsTest.cc` covers the coverages (across a cut, an
exterior-ring-only coverage whose hole the cut opens, a ring with no valued point, a run across a
ring's start, two scalar types, the nearest outline on a hole and its clamp, the other geometry
types), the size metric by area, Assign Plate IDs skipping the committed inconsistent input (also as
a second geometry property), time-dependent geometry (with a coverage in one window or in more), a
polygon of no measure, a range matching no geometry (removed if constant, kept if time-dependent),
and coverage pieces in features of their own.
The pyGPlates tests (`test_partition*` in `test_geometries_on_sphere.py` and
`test_app_logic/test.py`, and `test_most_overlapping_plate_by_area`) cover polygons with holes,
touches returned whole, `PartitionError` on the same input, the sliver argument (`None`, zero,
negative and NaN), coverages through `partition_features` and their pieces, time-dependent geometry
(a kept property sharing a partitioned one's name, and most overlapping), removing exactly the
partitioned properties, a feature with no geometry, and the most overlapping plate by area. The
Boolean methods' tests (`test_boolean_operations*` there) check every method's polygons against
the inputs' memberships on a grid of points (each covered once), the area identities, that the
intersection and the difference are `partition()`'s inside and outside lists, inputs apart, nested,
sharing an edge, touching at a corner and identical (the pieces and their holes), the hemisphere
errors of the union and the symmetric difference, and the arguments. `ExtractPolygons`
(`PolygonOverlayTest.cc`) checks that `extract_polygons` gives `extract`'s polygons point for
point, for every operation. The fixes made after review are pinned by tests, each checked by
reverting the fix, all but one (the
self-touch cuts' key, which no constructed input reaches). In the grouping, no test holds the point
test in one argument order (every crossing built is seen both ways), the nesting check's fallback,
the exemption of a group from the sliver filter, the filter's condition that a hole is filled only
when its parent is an exterior, the same-crossing test of two arcs, the order of holes, the
self-crossing test's two-margin arms, or the count of a node far from the other ring as outside (a
ring with a point inside the other has a point outside it in a near node too). The linking's two
rules (the cut of a ring twisted at a chain, and the symmetric difference's join) were mutated 35
ways against the committed tests, and 28 of the mutations no longer pass: 25 failed a test at
first, and three more since three of the hunt's cases were committed to hold them. Two of those
fail a test: one of the two directions in which a pair of vertices is tested, and, in the join,
that an arc ending at the wall's far end is no spike. With the third, the walk's stop at the other
passage removed, the walk does not end in a dense shared edge and the test never finishes. No
committed test holds, in the cut: the sectors of the passage against the two sides of the
arc's plane; the three conditions under which a doubtful walk decides nothing (an arc that ends at
the vertex, a point within the thickness past the arc's end, or within it of the other arc's
plane); that the chain is one the operation does not keep, and that the ring does not walk it; and
the thickness as the far points' margin. Run over the hunt, of these only the margin changes an
output of valid inputs (at half a thickness one more difference is cut), and two change nothing at
all (the arc that ends at the vertex, which is the identity rule, and the ring that walks the
chain, which states what the cut assumes and saves the arcs' arithmetic). No pyGPlates test raises
`PartitionError` through `PlatePartitioner` or `partition_features`; the C++ test of Assign Plate
IDs' skipped feature covers the code they share.

**Real data** (`PolygonOverlayRealDataTest.Coastlines`, skipped unless
`GPLATES_POLYGON_OVERLAY_REAL_DATA` names a plate model's directory, and optionally the times in
`GPLATES_POLYGON_OVERLAY_REAL_DATA_TIMES`, 0, 100 and 200 Ma by default) reconstructs a model's coastlines and cuts them through
the cascade of its topologies as Assign Plate IDs does, but by the overlay itself, so that every
cut's diagnostics and the oracle can be read, and reports the histogram of output ring widths the
sliver decision was made from (section 5.5). Measured on Müller et al. 2022 at 0, 100 and 200 Ma:
15,255 overlays past the bounding circles, 313 of them intersecting, no inconsistency, no repair, no
check failure even before repair (the 6 cuts at 0 Ma with crossing input rings read by the oracle
as rings, not as polygons), no bundle vertex (at most six ends at a vertex), 11 midpoint tests, and
2 collapsed rings dropped; the piece counts equal pyGPlates' at each time with its sliver filter off
(431 polygons from the 149 cut at 0 Ma, against 703 polylines before; at pyGPlates' default width
the 16 rings under `10 h` of section 5.5 are dropped, 415 at 0 Ma), and pyGPlates'
`partition_features` over the 2,136 features takes about 45% longer than before (0.29 s against
0.20 s at 0 Ma, after about 0.06 s resolving the topologies for either).

## 7. Deferred work

- **An explicit side for a ring larger than a hemisphere.** Under the rule a ring cannot mean more
  than a hemisphere, so a plate polygon enclosing more than `2 pi` means its complement. The fix is
  a way to say which side is meant: a reference point supplied by the caller or by the topology
  resolver, or a winding convention chosen per call. Deferred because no real plate needs it yet,
  and it changes the API. Resolved plates every 10 to 20 Myr in Müller 2019 (0-250 Ma), Merdith 2021
  (0-1000 Ma) and Cao 2024 (0-1800 Ma) never exceed half the sphere; the closest are India-Australia
  at 720 Ma (47.7% in Merdith 2021) and Rodinia at 860 Ma (47.1% in Cao 2024), so a
  supercontinent-era model could cross the limit.
- **An output of exactly a hemisphere** (a union of two quarter spheres, say) is returned or refused
  by the inputs' order and directions (section 5.2, the hemisphere check); accepted as a tie.
- **Walls the symmetric difference leaves unjoined** (the spike rule, section 5.2). Along a shared
  edge where an input folds back on itself along all of it, or a short one (its far end within the
  thickness of the arc beside it), the two differences' rings are left sharing the edge, unless
  other walls join them: the output reads the same, as two rings, but is not merged there
  (`symmetric_difference()` can then return two polygons along an edge where `union()` returns
  one). A sharper rule would tell which of a ring's passes along the
  wall has the output beside it. The rule also looks only at the arc beside the wall, so a ring
  doubling back over a whole wall in two arcs would be joined across it, giving a crossing group,
  which reads right; the hunt has none.
- **The inconsistent cut along a dense shared edge** (section 5.6): band classes that repair cannot
  make consistent in 20 rounds, in the one known case, whose inputs both have a hole crossing
  their exterior ring. Untried: a rule for pieces running along the other outline, or a
  repair candidate that flips a chain of band pieces. Also open from the hunt: 4 generated cases
  whose second cascade step loses a subject location (no point's provenance wrong), and 4 with
  coincident rings (at least two of them rings of no area). For a location lost at an input's
  corner (a point within `h` of both its segments that names one, section 5.4) a geometric fallback
  is possible and not built: the composition has the original geometry, and so the segments'
  lengths, to tell which of the two the edge runs along.
- **Invalid inputs with rings in common.** From random probes of polygons with several crossing
  rings: two coincident rings with area (an interior ring repeating its exterior, empty by parity)
  are not grouped, so they come out as two polygons over the same area; and inputs snapped to a
  grid, with whole rings in common and rings crossing at once, gave a wrong parity over the output
  rings without throwing in 5 of 5,985 cases at 0.002 radians and 52 of 5,832 at 0.004.
- **Crossings the grouping and the sliver filter miss** (section 5.2): an overlap of two output
  rings reached only within a few margins of their outlines (2 operations in 11,668 random cases
  snapped to a grid of 0.004 radians, none generic), whose polygons then overlap there; and a ring
  that crosses itself only within a few margins of the crossing point, which a sliver filter can
  still drop whole where its lobes cancel. Both tests decide only from points, and from edges whose
  ends are, farther than the margin from the other outline; the cure is the self-intersection mode
  below, after which every output ring is simple.
- **pyGPlates' feature assembly in C++.** `partition_features` assembles the partitioned features
  in Python, where Assign Plate IDs uses the C++ partition tasks (`PartitionFeatureTask`). Routing
  pyGPlates through those tasks would make the two products agree under the most overlapping plate
  too (section 5.4), and reduce the time spent outside the C++ partitioning.
- **Clipping by an open arc**, which would put `DateLineWrapper` (section 5.8) onto the overlay. The
  prototype showed three additions suffice: pieces of an open clip whose first and last pieces
  dangle (the structures for a polyline subject, `RingView::closed` and `Stop::DANGLING_END`,
  serve); classifying the arc's pieces, without classes for `P`'s pieces, by parity along the arc,
  flipping where an odd number of `P`'s non-common rays leave on one side of the arc's thick plane;
  and a linking by faces in which an arc ray inside `P` carries one node per side, where the pairing
  is keyed by ray today (`pair_and_walk_rings`). In the prototype's linking a piece of `P` needs no
  direction: on arriving, the walk takes whichever side of the `P` ray is inside `P` (a first
  version that left along the previous ray assumed `P` has one left side, and failed on inputs that
  cross themselves); the side of a ray against the arc is the thick decision `GeometryIntersect`
  made for its records; and a ring of `P` with no record is attached to the face that contains it.
  What the prototype did not run: it took the arc's two dangling end pieces as outside `P` by
  construction, and the dateline arc ends at the poles, so a polygon round a pole, the dateline's
  own case, is untried; no vertex of six or more ends arose (one ring of `P` near a simple arc); and
  its only failures, 13 of 600 cuts, were invalid inputs (an exterior ring that crosses itself, or a
  hole outside its exterior). The port would cure four defects of section 5.8 (a hole that crosses
  the dateline merged into an exterior, the guess when both poles are touched, the grouping of the
  other rings by an epsilon distance, and the ring run backwards at a touch) and keep, as a thin layer, the latitude and longitude output, the
  snapping to the dateline and the poles, the central meridian, the tessellation and the per-point
  provenance. It would change the map views' and exports' wrapped polygons and pyGPlates'
  `DateLineWrapper` output (holes stay holes, rings keep their direction).
- **Time-dependent geometry** is partitioned only in a single time window containing the
  reconstruction time (section 5.4); what partitioning should do with several windows, or an
  irregular sampling, is undecided.
- **A self-intersection mode of `GeometryIntersect`**, to split a self-crossing ring into simple
  rings with the same machinery: under parity every piece of the self-overlay is kept except a
  stretch the polygon covers twice (a common chain of the polygon with itself), and the overlay's
  pairing, split and grouping (section 5.2) then make the lobes touch where they crossed. It would
  also serve export to formats whose polygons must be simple. `intersect_segments` itself would
  serve, with four additions. It would also give an arc its stop where it passes a vertex of its
  own input, which the overlay now finds only in three places, each with the thickness again (the
  self-touch cuts, the cut of a ring twisted at a chain and the walls the symmetric difference
  leaves unjoined, section 5.2); what those leave is the band's noise, such as a sliver under half a
  thickness wide wound backwards on a ring. (The cut looks only between two vertices of one unkept
  chain: a ring twisted where an arc passes a vertex of its own input with no common piece between
  them is not looked for, and no valid input of the hunt gives one wider than a thickness.) After
  it every output ring would be simple, and so oriented with the output on its left all round, which an output ring that crosses itself where an
  input ring does is not today (section 5.2), in partitioning as in the Boolean methods.
  A segment must never be tested against itself: its start coincides with its own start, which is
  an SS. Adjacent segments need no special skip: their shared vertex is a coincident end and start,
  which excludes a crossing and the later segment's start touching the earlier one (section 4.2),
  and the earlier segment's start touches the later one only if the later doubles back along it, a
  genuine self-overlap; but a zero-length segment's start and end coincide, so it and its successor
  give an SS at their shared vertex, which such a mode would have to drop. The bounding tree is
  descended against itself, each unordered pair of nodes once, including a node with itself, and at
  a leaf with itself only the pairs with a smaller first index. The `Graph`'s two orders become one
  order along the geometry in which every intersection appears twice, once per segment index.
- **The exact parity area** for interior rings that cross the exterior ring or overlap each other
  (section 3.3), and for a ring that crosses itself (section 5.4: its lobes cancel, which misleads
  the most overlapping plate). It needs a polygon overlaid with itself (the self-intersection mode above, then
  the overlay's split and grouping), which the two-polygon overlay of section 5 does not do.
- **Thin fan triangles.** `calculate_spherical_triangle_signed_area` (the three-edge overload in
  `SphericalArea.cc`) returns zero when any side is zero length (under about `1e-6` radians), even
  for a long, thin triangle, so the fans lose area and moment. Measured: a sliver about `1e-6`
  radians wide gets about a quarter of its true area; a polygon `1e-5` radians across with short
  edges gets an interior centroid off by about its own size; and at `1e-7` radians every fan side
  is zero length, so the centroid falls back to the first vertex. Plate areas are unaffected
  (Müller 2019 at 0, 100 and 200 Ma, worst `8e-14` steradians). The fix is a triangle formula that
  handles thin triangles (Van Oosterom and Strackee, say), which changes every area by rounding,
  so it is its own change.
