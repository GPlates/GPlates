# Phase 2: the papers, the intersection requirements, and the candidate algorithms

Written 2026-10-23, for the checkpoint at the end of Phase 2 of `PLAN.md`. The description of
`DateLineWrapper` is in the design doc, `spherical-geometry.md` section 5.2; this file holds what
does not belong in a design doc: the reading, the requirements that Phase 3 audits
`GeometryIntersect` against, and the choice of algorithm for Phase 4. Every statement about the
code names its file and function; statements about the papers name the section or figure.

Notation as in the design doc: `P` is the geometry being partitioned, `Q` the partitioning polygon,
`R` the outside point of a polygon, `s` the point-on-outline threshold (about `1.414e-6` radians),
and "the thickness" is `GeometryIntersect`'s thick-plane tolerance (about `1e-6` radians). So `P` is
the subject and `Q` the clip. Foster, Hormann and Popa's section 2 names them the other way round
("two polygons P and Q, often called the clip and the subject polygon"), but their algorithm and
figures treat the two symmetrically, so only that phrase differs; partitioning is not symmetric
(`P \ Q` is an output), which is why the roles are fixed here.

## 1. The papers

### 1.1 Greiner and Hormann 1998, "Efficient clipping of arbitrary polygons"

**What it does.** Clips one closed polygon by another in three phases (section 5). Phase 1 tests
every edge pair (`O(nm)`, Fig. 11 and 14), creates two copies of each intersection point, one in
each polygon's doubly linked list, sorted into place by the `alpha` along the edge, and links the
copies by a `neighbor` pointer (Fig. 9 and 10). Phase 2 marks the copies entry or exit: for each
polygon, one point-in-polygon test of its first vertex against the other polygon decides the
status at the start, and the status is then toggled at every intersection along the polygon
(Fig. 12). Phase 3 traces the result: start at an unvisited intersection of the subject, walk
forward from an entry and backward from an exit until the next intersection, switch to the
neighbour, repeat until the start is reached; repeat while unvisited intersections remain
(Fig. 13). Union and difference come from the same structure by marking the parts *outside* the
other polygon instead (Fig. 8).

**Interior.** The winding number, with the interior being the points of odd winding number
(section 2, the Definition), so a point crossing the curve once moves between interior and
exterior; the unbounded component has winding number zero. That is the even-odd rule, and it is
what makes toggling at each crossing correct for self-intersecting polygons too.

**Holes and self-intersection.** The paper's polygon is one closed vertex list; it says nothing of
multiple components. Self-intersection is allowed (the title's "arbitrary"), and Fig. 3 shows the
interior of a self-intersecting polygon. Phase 2 works on such a polygon because the toggle only
needs the even-odd rule. Foster, Hormann and Popa describe it as clipping "concave and
self-intersecting polygons with holes" (their abstract), the holes being extra components under
the even-odd rule.

**Degeneracies.** Not handled: a vertex on an edge of the other polygon, or on a vertex of it, is
detected in `intersect` by `alphaP = 0`, and the vertex is then perturbed by less than a pixel and
the algorithm continues (section 5, Fig. 15). The paper says the two possible perturbations give
different results, and leaves it at that.

**What transfers.** Everything but the arithmetic. The interior definition is ours: section 3.1 of
the design doc defines a ring's interior by winding parity relative to its outside level, and
combines rings by parity, so one point-in-polygon test from `R` followed by toggling at each
crossing is exactly phase 2. The data structure is what `DateLineWrapper::IntersectionGraph` uses.
The intersection test (`WEC` values, Fig. 14) is a planar signed-area test; the sphere's is the
triple product, which `GeometryIntersect::intersect_segments` and `PointInPolygon` already use.

### 1.2 Liu, Wang, Bao, Gomboši and Žalik 2007, "An algorithm for polygon clipping, and for determining polygon intersections and unions"

**What it does.** The same three steps with a smaller structure (section 3, Fig. 2): singly linked
lists, and one record per intersection, with `next_1` continuing along the subject and `next_2`
along the clip polygon, so an intersection record is shared rather than copied. Lemma 1: if both
polygons have the same orientation, an intersection that is an entry for one is an exit for the
other. So the entry/exit status is computed once, at the first intersection found (Pseudocode 1,
`EntryDetermine`), the subject's orientation is reversed if the two statuses do not agree, and
every later intersection alternates with no further test (section 4.1). The tracing step is
Greiner-Hormann's (Pseudocode 3). Edge intersections use Liu's line-clipping test (section 4.2).

**Holes.** A polygon is a loop plus rings oriented the opposite way, so that the interior is on the
same side of every edge (section 2); the rings are kept in a separate list per polygon (section 3,
Fig. 4), and the paper says the algorithm then applies unchanged to them (section 4.1).

**Self-intersection.** Not supported: the polygons are simple (section 2), and Lemma 1 and the
orientation reversal assume one interior side per polygon.

**Degeneracies** (section 4.3, Fig. 7). Two methods are named: Liu and Liu 1993's case analysis,
which deletes the intersections of a line passing a vertex or edge from outside to outside or
inside to inside, and keeps one of the two for a pass from outside to inside, so that entries and
exits still alternate; and Greiner-Hormann's perturbation, which they used, moving the vertex
temporarily rather than permanently so that errors do not accumulate.

**What transfers.** Little that Greiner-Hormann does not already give. The shared record is an
implementation choice. Lemma 1 and the single status test rely on consistent orientation, which
GPlates never has (design doc section 3.6, "Orientation by default"), and the case analysis for
degeneracies is the crossing-or-bouncing idea that Foster, Hormann and Popa make systematic.

### 1.3 Foster, Hormann and Popa 2019, "Clipping simple polygons with degenerate intersections"

**What it does.** Extends Greiner-Hormann so that a vertex of one polygon on an edge or vertex of
the other, and edges lying along each other, are handled without perturbation (abstract,
section 4). Phase 1 is reworked to detect every degenerate case, phase 2 labels intersections as
crossing or bouncing and alternates entry/exit over the crossing ones only, and phase 3 is
unchanged except that it skips bouncing vertices (sections 4.1 to 4.3). An implementation in C++
is published (section 5).

**Scope, precisely.** Section 2: a *complex* polygon is a set of components with the interior
defined by the even-odd rule, so components inside others are holes (Fig. 1b); a polygon is
*simple* if "it does not cross itself, that is, its edges intersect only at common endpoints", so
every component is a topological circle; and "we consider single polygons as complex polygons
with one component and refer to complex polygons simply as polygons". So **polygons with holes
are covered** throughout, nested to any depth (Fig. 18; section 4.3 ends "even in the case of
nested holes"). **Self-crossing rings are not** covered by the title, but section 6 narrows the
restriction: "the only limitation of our algorithm are degenerate intersections involving
self-intersection points. In fact, if a self-intersection of one polygon lies on an edge or
coincides with a vertex of the other polygon, then the algorithm may fail (see Fig. 21)." Fig. 21's
caption: it "handles self-intersecting polygons correctly, as long as the self-intersection does
not lie on the other polygon". Their remedy is to split a self-crossing polygon into simple pieces
first. So for our data: a twisted ring (design doc section 3.1) is fine unless its twist lies on
the partitioning polygon's boundary, which the cascade can arrange. The paper does not say why it
fails. As we read Fig. 21, the twist is one point of `Q` but two of `P` (`P` passes through it
twice), and phase 1's one-to-one neighbour link cannot represent that: `Q` gets two copies of the
point a zero length apart, one per passage of `P` (Fig. 21b and c), or one vertex of `Q` needs two
partners (Fig. 21d). The side test at either copy then reads the other copy as `Q-` or `Q+`, along
an edge with no direction. Section 3.3 says how the overlay avoids this.

**Phase 1** (section 4.1, Fig. 6 and 7). Edges are half open, `[P1, P2)` against `[Q1, Q2)`, so a
contact at an end point is reported once, by the next edge. For non-parallel edges the
intersection `I = (1 - alpha) P1 + alpha P2 = (1 - beta) Q1 + beta Q2` is classified by `alpha`
and `beta`; for collinear edges the end points are parameterised along each other. In every
degenerate case a copy of the vertex is *inserted* into the other polygon and linked, so that
after phase 1 "all intersections of P and Q occur at common intersection vertices, which are
linked by neighbour pointers, or along common segments, which are now represented as common edges
in both P and Q with common intersection vertices as endpoints" (Fig. 8). The cases map onto
`GeometryIntersect::Intersection::Type` (design doc section 4.2) as follows:

| Foster-Hormann-Popa case | Condition | `GeometryIntersect` | Difference |
| --- | --- | --- | --- |
| X-intersection | `0 < alpha < 1`, `0 < beta < 1` | `SEGMENTS_CROSS` (LL) | Decided with thick planes, not exact `alpha`, `beta`; the position is the cross product or the chord interpolation of section 4.2 |
| T-intersection, `P1` on `[Q1, Q2)` | `alpha = 0`, `0 < beta < 1` | `SEGMENT1_START_ON_SEGMENT2` (SL) | `alpha = 0` means within the thickness; the position is `P1`, up to the thickness off `Q`'s edge; `beta` is the angle along `Q`'s segment |
| T-intersection, `Q1` on `[P1, P2)` | `beta = 0`, `0 < alpha < 1` | `SEGMENT2_START_ON_SEGMENT1` (LS) | as above |
| V-intersection | `alpha = beta = 0` | `SEGMENT_START_ON_SEGMENT_START` (SS) | Coincidence within the thickness cosine; the position is `P1` |
| X-overlap (collinear, each start point inside the other edge) | `0 < alpha, beta < 1` | two records, SL and LS | The header's "SL and LS" example |
| T-overlap | one start point inside the other edge, the other start point outside it | one touch record (SL or LS) | The header's third example gives SL, LS, LS over three edges |
| V-overlap | `alpha = beta = 0` | SS | The header's fourth example, "SS and SS" |

The structural difference is that Foster-Hormann-Popa's phase 1 *splits both polygons* at every
contact, so a common segment becomes a common edge, whereas `GeometryIntersect` only *reports*
each contact with its position along each geometry's segment, and leaves the splitting to the
consumer. Both candidate algorithms below therefore begin by splitting both geometries at every
reported intersection; after that, a sub-edge whose two ends are consecutive intersection vertices
along both geometries, with no original vertex between them on either, is a common edge.

**Phase 2** (section 4.2, Fig. 9 to 11). At an intersection vertex `I` of `P`, with neighbours
`P-` and `P+` on `P` and `Q-` and `Q+` on `Q`: if none of the four edges overlap, `I` is
*crossing* when `Q-` and `Q+` lie on different sides of the chain `(P-, I, P+)` and *bouncing*
when on the same side (Fig. 10). The side of a chain is found from three signed areas, with the
left and right turn cases. If an edge at `I` is common (`P+` is linked to `Q+` or `Q-`, which is
how a common edge is recognised), the five cases left/on, right/on, on/on, on/left and on/right
apply (Fig. 11), and a maximal chain `I1 .. Ik` of intersection vertices joined by common edges is
a *delayed crossing* when its two ends change side and a *delayed bouncing* when not; `I1 .. Ik-1`
are marked bouncing and `Ik` crossing or bouncing accordingly. The labels are copied to `Q`, since
`P` crosses `Q` at `I` iff `Q` crosses `P` there. Then the entry/exit alternation of
Greiner-Hormann runs over the crossing vertices only, from one non-intersection vertex per
component whose status is found by a point-in-polygon test; a component with no such vertex uses
the midpoint of an edge that is not common, or, if every edge is common (the component equals one
of the other polygon's), is kept iff both are holes or both are not (Fig. 13a). Section 4.4 adds
three clean-ups: collinear vertices inside common chains are removed; a bouncing vertex that lies
between an entry and an exit is split in two so that the result's components are simple (Fig. 14);
and "glued" edges are avoided by also labelling the end points of delayed crossings and bouncings
as entry/exit (Fig. 15 and 16, following Popa et al.). Labelling costs `O(k)` for `k`
intersections; the paper rejects Kim and Kim's alternative, which classifies the midpoints of the
edges adjacent to each intersection as inside or outside, as `O(kn)` (sections 1 and 6).

**Degeneracies.** All handled except the self-intersection case above, and the paper reports
stress tests with hundreds of T- and V-intersections (Fig. 17, 19).

**What transfers.** The phase-1 case table is a specification for what partitioning needs from
`GeometryIntersect`, and the requirements in section 2 are written from it. The side tests are
triple products on the sphere (`PolygonPartitioner.cc`'s `do_adjacent_great_circle_arcs_bend_left`
is one). The chain reasoning is what `GeometryCrossing` set out to do. The even-odd interior is
ours, so the labelling and alternation are valid under the interior rule, with the start status
taken from `R`. What does not transfer is the exactness of `alpha = 0`: Foster-Hormann-Popa's
phase 1 is exact in principle (their arithmetic is plain floating point, so it is not exact in
practice either, but they do not discuss it), whereas `GeometryIntersect` decides "on" with the
thickness, and that difference is what section 3 is about.

### 1.4 Schettino 1999, "Polygon intersections in spherical topology: application to plate tectonics"

**What it does.** Cuts a polygon `P` on the sphere by a base set of plate polygons `B_j` in
sequence (eq. 1 in section 1, eq. 2 in section 2): each piece `Q_k(j-1)` left by the previous
plate is intersected with `B_j`, which is the cascade of
`GeometryCookieCutter::partition_geometries`. For
one pair, `FindIntersections` first finds a vertex `r0` of the second boundary that is not inside
the first (`PointIsInternal`), walks from it finding pairs of edges that enter and leave the other
polygon, computes each crossing as `b = +-lambda (p x q)` from the two great circles (eq. 3 and 4)
and inserts it into both lists; then `PolygonIntersect` generates one path per intersection
vertex, following the first list to the next intersection and the second list back
(section 2, Fig. 2), and removes duplicate paths by cyclic comparison (`PathCmp`). Point-in-polygon
is a meridian arc from the point towards the south pole, with `EncompassesPoles` deciding by the
mean distances of the vertices from each pole (eq. 5) whether a polygon encloses a pole
(section 4). Cost `O(nm)` (section 5).

**Holes, self-intersection, degeneracies.** No holes: a polygon is one ring. Self-intersecting
rings are excluded by the input's self-consistency (section 1, condition (a)). Degeneracies are not
handled: `PointIsInternal` returns false for a point on the boundary, which only avoids an
undefined answer, and no touch or overlap case is treated. Pairs of edges whose inner path has
fewer than a user threshold `v >= 2` points are dropped "to avoid the generation of the large
number of small polygons" (section 3), which loses area.

**What transfers.** The confirmation that the cascade is the natural formulation for plates, and
that slivers are the problem it produces. Its pole handling is what the interior rule's `R`
replaces. The rest is a less careful Greiner-Hormann.

### 1.5 Also read

- **Martínez, Rueda and Feito 2009**, "A new algorithm for computing Boolean operations on
  polygons" (planar sweep). Edges of both polygons are subdivided at every intersection as the
  sweep finds them; each sub-edge then gets two flags, `inside` (the other polygon) and `inOut`,
  copied and toggled from the edge immediately below it on the sweep line (section 4, Fig. 9); the
  result edges are those whose flags fit the operation, and they are joined into rings afterwards
  (section 5). Overlapping edges are handled by marking one copy non-contributing and the other
  "same transition" or "different transition" by comparing `inOut` flags (section 7.2, Fig. 10).
  The sweep is planar, but its edge-by-edge classification with a separate overlap rule is the
  shape of candidate (b).
- **Fogel, Setter and Halperin 2008**, "Arrangements of geodesic arcs on the sphere" (CGAL). An
  exact arrangement of great circle arcs, robust for every degeneracy because the arithmetic is
  rational. It shows the exact route exists; it would not help here, because plate boundaries that
  should coincide agree only within epsilon (design doc section 3.5), so an exact overlay would
  faithfully compute the slivers between them.
- **Hobby 1999**, "Practical segment intersection with finite precision output". Rounding an
  intersection point to finite precision can create extraneous intersections with the segments
  near it, and iterating does not converge quickly (Fig. 1 and 2). This is the cascade's problem in
  one picture: a piece's rounded crossing vertex, fed to the next plate, can lie a rounding error
  on the wrong side of an edge it was computed on.

### 1.6 What transfers to the sphere and to the interior rule

- The even-odd interior of Greiner-Hormann and Foster-Hormann-Popa is the parity of the interior
  rule, ring by ring and across rings. The start of the alternation is `R`, whose status is known
  (outside), or any other point whose status `is_point_in_polygon` gives with the threshold off.
- Signed areas become triple products: the side of a point relative to an arc's plane, and the
  turn at a vertex from `dot(v, cross(axis1, axis2))`.
- Half-open edges are `GeometryIntersect`'s "no end points" rule and a polygon ring's wrap-around.
- The plane has an unbounded face of winding number zero; the sphere does not. Every ring's
  interior is its smaller side (or the rule's choice), and every output piece of partitioning is a
  subset of `P`, so if `P` is under a hemisphere so is every output ring's interior, and the rule
  reads the output rings as intended. A `P` over a hemisphere is the deferred case of design doc
  section 7, and nothing here changes that.
- Arcs near a half circle have no planar analogue; `GeometryIntersect` and `GreatCircleArc` guard
  the chord interpolation, and the requirements below keep that.
- `DateLineWrapper` already shows the Greiner-Hormann structure working on the sphere, with the
  shortcuts listed in design doc section 5.2.

## 2. Requirements for the intersection primitive

What partitioning will do with the result, whichever candidate is chosen: split both `P` and `Q`
into *pieces* at the intersection vertices, with each piece a run of original vertices between two
intersection vertices (or a whole ring); identify the pieces common to both; classify pieces; and
build output rings. The requirements follow from that. "Today" states a first impression from
reading `GeometryIntersect.cc`, for Phase 3 to confirm or refute.

**R1. One record per contact, positioned on both geometries.** Every place where the two
geometries meet is reported once, with its position, and for each geometry the segment index and
the distance along that segment, so that both geometries can be split at it. *Today: met.
`add_intersection` records both segment indices and both angles (`angle_in_segment1`,
`angle_in_segment2`, the angle being zero on a geometry whose vertex it is). The position of a
touch is the touching vertex, up to the thickness off the other geometry.*

**R2. A shared vertex is reported once**, as one record, whichever segments meet there, including
a polyline's last vertex. *Today: met by design. The SS record is made only by the pair of
segments that both start there (`intersect_segments`, the coincident-vertices block), a crossing
is never tested when any end points coincide, and a polyline's last vertex is the start of the
fictitious segment `number_of_segments`. The audit should check the "complementary tests" claim in
that block, and what happens for two vertices between the thickness and `s` apart (R7).*

**R3. Crossing, touch and overlap are distinguishable, and the same from both sides.** From the
records it must be possible to say, at every intersection vertex, whether the geometries cross
there, touch without crossing, or begin or end a stretch of running together; and the answer must
be the same whether it is read along `P` or along `Q`. *Today: not provided directly. The four
types say where on each segment the contact is (interior or start), not whether the geometries
cross; "overlap is not a type" (design doc section 4.2). The information to derive it is present
(R1, R4): a touch vertex whose neighbours on the other geometry lie on opposite sides of its own
chain is a crossing, as Foster-Hormann-Popa's phase 2 does it. Whether this is derived in the
partitioner or added to `GeometryIntersect` is an open question (section 4).*

**R4. An overlap is recoverable as a chain**, with its two end vertices and its direction (same or
opposite) along the two geometries. Every original vertex of either geometry inside the stretch
must itself be an intersection record, so that the chain is a sequence of consecutive records
along both geometries. *Today: the header's overlap examples show every vertex inside an overlap
reported as a touch (SS, SL or LS), which gives the chain as consecutive records along both; the
direction follows from the record order along each geometry. Not verified beyond the examples; the
audit should test overlaps where the two geometries' vertices interleave, overlaps that start or
end at a shared vertex, and overlaps along a zero-length segment.*

**R5. Ordering along each ring**, interior rings included, with wrap-around. The records sorted
along a polygon must follow its exterior ring and then each interior ring, each from its first
vertex, so that the piece after a ring's last record joins the piece before its first. *Today: met
per polygon, ring boundaries implicit. `sort_geometry_intersections` sorts by segment index then by
angle (`is_precisely_less_than`); segment indices run through the exterior ring and then each
interior ring as `PolygonOnSphere::get_segment()` numbers them, so the consumer splits the sorted
list by the rings' segment ranges and closes each ring itself. Two caveats for the audit: the angle
along a segment is an `AngularDistance` made from a cosine, whose resolution near zero is about
`1.5e-8` radians (design doc section 1), so two records that close to a segment's start compare
equal and `std::sort` leaves their order unspecified; and `PolylineIntersections` passes
`include_polygon_interior_rings = false`, which is the consumer's choice, not the primitive's.*

**R6. Both polygons' interior rings participate.** *Today: supported by the polygon overloads of
`intersect()` (the default `true`); excluded only by `PolylineIntersections`.*

**R7. Consistency with the point-on-outline threshold.** A point of an output piece's boundary
must not be classified one way by partitioning and the other by `is_point_in_polygon` on the same
polygon, except as the inclusive threshold itself requires. Concretely: if partitioning classifies
with the threshold off (exact parity), the only disagreement allowed is a threshold-on query
calling a point within `s` of the outline inside, which is the documented inclusive rule. The
intersection tolerance should either equal `s` or be documented against it. *Today: the thickness
is the zero-length bound, about `1e-6` radians, and `s` is about `1.414e-6`; the design doc's table
says so. Measured with `PolygonOnSphere.partition` on a polyline whose first vertex is `d` from a
square's edge, outside it (scratchpad `phase2/gap_probe.py`): at `d = 1.1e-6` and `1.3e-6` radians
there is no intersection and the polyline is wholly outside, while `is_point_in_polygon` (threshold
on) calls that vertex inside; at `0.9e-6` the vertex touches and at `1.5e-6` both agree it is
outside. Under candidate (b) the classification is exact, so this gap only means that a touch
within `(1e-6, 1.414e-6)` is not a split point; that is acceptable, but it must be a decision, not
an accident.*

**R8. Zero-length and near-antipodal arcs.** A zero-length segment (under about `1e-6` radians) has
no plane; it must still be reported where its start point lies on the other geometry, and it must
never let the other geometry tunnel between it and its neighbours. A segment near a half circle
has an ill-conditioned plane; the crossing position must not be interpolated along it. *Today:
zero length is handled as described (design doc section 4.2, and the comment block at
`intersect_segments`), and the thickness was chosen as the zero-length bound for exactly the
tunnelling reason. Near-antipodal arcs: `add_segments_crossing_intersection` guards the chord
interpolation with the `1e-6` chord-midpoint test and splits the arc; but the thick-plane
classification uses `rotation_axis()`, a normalised cross product that loses accuracy as the arc
nears 180 degrees, and no refinement handles that. `GreatCircleArc::create` refuses end points
antipodal within epsilon (`unit_vectors_are_antiparallel`, `dot <= -1.0` on a `real_t`, so within
about `1.4e-6` radians of a half circle), and allows anything shorter. Two arcs between the same
nearly antipodal end points can lie on different great circles, so "same end points" does not
imply "same arc" there (see section 3.3).*

**R9. Cascade feedback.** A piece produced by one cut, with vertices that are rounded intersection
points and edges lying on the next plate's boundary within epsilon, fed back in, must yield
either a clean overlap chain (R4) or a clean crossing or no contact, never a missed crossing
(tunnelling) and never a crossing placed outside both arcs. *Today: within the thickness the
contact becomes SS, SL and LS records (an overlap chain); beyond it the arcs are nearly parallel
and may cross at a tiny angle, and that crossing's position is interpolated along the first arc
without the lune check, the known gap against `GreatCircleArc::intersect` (design doc section 4.3),
which is the first thing the audit should port. Shared plate vertices coincide only within `s`,
and `s` exceeds the thickness, so some are not SS records (R7); the resulting sliver is then
classified exactly. Hobby's extraneous intersections (section 1.5) are this case.*

**R10. What is exact and what is tolerant.** Detection may use a tolerance (thick planes,
coincident vertices), provided the same test decides a vertex as the end of one segment and the
start of the next, and provided it is symmetric in the two geometries. Positions, segment indices
and angles must be what the consumer splits with, so that both geometries are split at the same
point. The classification of pieces must be exact, by `is_point_in_polygon` with the threshold
off, and must never be inferred from the side information of a tolerant decision. *Today: the
detection tests are the same for start and end points (the comment block in `intersect_segments`
on "complementary" tests, and the vertices-first order); the angle along a segment is tolerant
only by the cosine resolution (R5).*

**R11. Completeness and pruning.** Every crossing is found; the bounding-tree pruning never skips a
pair of arcs that could touch. *Today: the bounding small circles are expanded by the thickness
(design doc section 1), which the comment in `intersect_bounding_tree_nodes` says is for this.
Soundness is for the audit.*

**R12. Symmetry.** `intersect(A, B)` and `intersect(B, A)` report the same contacts with the types
mirrored (SL for LS). *Today: by construction of `intersect_segments`, with the SS position taken
from the first geometry's vertex, so positions can differ by up to the thickness.*

Requirements met today, on first reading: R1, R2, R5, R6, R8 (zero length), R10, R11, R12. Not
met, or met only by reconstruction in the consumer: R3, R4. Known gaps: R7 (the thickness is not
`s`), R8 (near-antipodal planes), R9 (the near-collinear crossing position).

## 3. Candidate algorithms

### 3.1 Common ground

Both candidates start the same way, and so does `DateLineWrapper`:

1. `GeometryIntersect` on `P` and `Q`, all rings of both (R6).
2. Split every ring of both into pieces at the intersection vertices, in ring order with
   wrap-around (R5). A piece carries its provenance: each point is an original vertex (ring,
   vertex index) or an intersection vertex (segment index and ratio on *both* geometries, which
   is what `PartitionFeatureUtils` needs and what `DateLineWrapper::InterpolateOriginalSegment`
   records). A ring with no intersection is one piece.
3. Mark the common pieces: a piece of `P` between consecutive intersection vertices `u` and `v`
   is common with a piece of `Q` when `u` and `v` are consecutive along `Q` too and neither piece
   has an original vertex between them (R4; see section 1.3 on why this is exact in the
   combinatorial sense: an original vertex of either geometry lying on the other within the
   thickness is itself an intersection vertex, so a piece with an interior vertex is not on the
   other geometry). Its direction is the same if `u` precedes `v` along both.
4. Classify the rings with no intersection at all, each by one vertex, threshold off, against the
   other polygon (R10). Both polygons' rings: a ring of `Q` inside `P` is a cut too, which is the
   first of the four bugs.

The candidates differ in how the pieces of intersecting rings are classified and joined.

### 3.2 (a) Greiner-Hormann tracing with Foster-Hormann-Popa labelling

Label each intersection vertex crossing or bouncing from the sides of its neighbours (section 1.3),
with chains of common pieces as delayed crossings or bouncings; copy the labels to the other
polygon; for each ring of each polygon, take the status of one non-intersection vertex (threshold
off), or of a piece midpoint if there is none, and alternate entry/exit over the crossing vertices;
trace the inside output by Greiner-Hormann's walk, skipping bouncing vertices; trace the outside
output (`P \ Q`) by the reversed rules of section 4.4 of the paper; apply the split-candidate and
collinear clean-ups.

For it: the labelling is `O(k)`, one point-in-polygon test per ring, every output ring closes by
construction, and the algorithm is published with a reference implementation and worked
degenerate examples. Against it:

- **An error propagates.** The entry/exit flags are a running parity along the ring; a single
  crossing mislabelled as bouncing, or the reverse, inverts the classification of every piece from
  there to the end of the ring, silently: the output still closes, and nothing in it says which
  flip was wrong. The labels are exactly the tolerance-scale decisions: the side of `Q+` relative
  to `(P-, I, P+)` when `Q+` is within the thickness of `P` (a chain end), and chain
  reconstruction over SS, SL and LS records that are each up to the thickness off the geometry
  they are "on". In the cascade every shared boundary is such a chain.
- **Self-crossing on `Q`.** A twist of `P` lying on `Q`'s boundary is outside the method's scope
  (section 1.3), and the cascade can put one there (the twist is an original feature of the input;
  the next plate's boundary is wherever the model puts it).
- **Polylines** need their own version: there is nothing to trace, so the pieces would be
  classified by the same alternation from the first vertex, with the same fragility.
- **Two traces** for the two outputs, with reversed rules and reversed chain handling, each a
  separate opportunity for the rule sets to disagree.
- The paper's own fallbacks (a ring made only of intersection vertices; every edge common) and
  clean-ups (splitting bouncing vertices between an entry and an exit) are extra machinery on top.

### 3.3 (b) Overlay: split, classify each piece, link

**Classify each piece on its own.** A piece of `P` that is not common is inside `Q` or outside it,
decided by one `is_point_in_polygon` of an interior point of the piece against `Q`, threshold off
(R10): the midpoint of the piece's longest arc, which is the point furthest from the piece's ends,
the only places `Q` is known to be near. A piece of `Q` likewise against `P`. The piece has no
crossing in its interior (R11), so its interior is on one side of the other polygon except within
the tolerance of that polygon's boundary, and the midpoint test is the piece's status.

**Common pieces: the sector rule.** A common piece (a chain of them) bounds `P` on one side and
`Q` on one side, and belongs to the inside output exactly when one of its two sides is inside
both, and to the outside output exactly when one side is inside `P` and outside `Q`. Which needs
no new geometry: at any intersection vertex, the piece-ends divide the neighbourhood into sectors;
crossing a piece of `P` alone toggles membership of `P`, a piece of `Q` alone toggles `Q`, and a
common piece toggles both. A non-common piece's own test fixes the *other* polygon's membership
in both sectors beside it. Walking round the vertex from any sector whose two memberships are
fixed, toggling at each piece-end, fixes both memberships in every sector. At the usual vertex at
one end of a common chain, with one piece `p` of `P`, one piece `q` of `Q` and the chain `c`, this
reduces to: `c` is in the inside output iff `[q inside P] != [p inside Q]`, and in the outside
output otherwise (one of the two, always). A chain forming a whole ring with no non-common piece
anywhere (`P`'s ring equals `Q`'s) has no anchor; the fallback is a point-in-polygon test of a point
offset sideways from the ring by more than the tolerance, or the paper's rule (section 1.3). The
walk round a vertex needs the piece-ends in rotational order about it (the tangent of each
piece's first arc, `cross(axis, v)`, sorted by angle in the tangent plane), and the memberships
that two different pieces both fix must agree, which is a check.

**Link the kept pieces into rings.** For the inside output, keep `P`'s pieces inside `Q`, `Q`'s
pieces inside `P`, and the common chains the sector rule assigns; for the outside output, `P`'s
pieces outside `Q`, `Q`'s pieces inside `P` (reversed), and the other common chains. At each
intersection vertex the kept piece-ends are paired: the piece arriving along one boundary of an
inside-both sector continues along the sector's other boundary. With two kept ends at a vertex
there is one way; with four or more (two output regions touching at the vertex, or a self-crossing
of `P` on `Q`) the rotational order pairs them so that the rings are simple there, which is
Foster-Hormann-Popa's split-candidate clean-up for free. Walk the pairings to close the rings.

**Contract pieces shorter than the tolerance.** Step 2 of section 3.1 splits at each record, so two
records at one point give two vertices with a piece of zero or rounding length between them, and
that piece's midpoint test means nothing. A twist of `P` lying on `Q` does exactly this: `P` crosses
`Q`'s edge twice at the same point, the case that defeats Foster-Hormann-Popa (section 1.3). Two
touches within the tolerance do it too (section 4, question 4). So every piece shorter than the
tolerance is *contracted*: removed, with its two end vertices merged into one. A merged vertex
still has an even number of piece-ends of each polygon; the twist on `Q` becomes one vertex with
four ends of `P` and two of `Q`. The walk round a vertex and the pairing take any number of ends,
and the pairing makes `P`'s two lobes touch there rather than cross, the result Fig. 21 expects.
Without contraction the checks below would still catch the short piece's wrong class, but only as
an inconsistency to repair. A run of short pieces contracts to one vertex, which can be wider than
the tolerance; the point set then moves by up to that width, and the Phase 5 hunt should measure
how wide such clusters get in the cascade.

**A twist away from `Q`** is not a vertex at all. It lies inside a piece, the piece's test is
unaffected, and the output ring keeps the twist. The output is still right under the parity rule:
counted mod 2, the boundary of `P ∩ Q` is the part of `P`'s boundary inside `Q` plus the part of
`Q`'s boundary inside `P` (common chains by the sector rule), whether or not the rings are simple,
and those are exactly the kept pieces. Foster-Hormann-Popa handle this case too; what they cannot
represent is the twist on `Q`.

**Checks.** At every vertex the number of kept ends is even and each inside-both sector has
exactly two; memberships fixed twice agree; every walk closes. A failure is local, points at the
vertex and the pieces concerned, and is the sign of a missed or spurious intersection (R9, R11) or
a piece shorter than the tolerance whose midpoint test fell on the wrong side. The repair policy
(prefer the longer pieces' tests, flip the short piece, and carry on; or assert in debug builds) is
an open question.

**Rings with no intersection** are pieces too (section 3.1, step 4) and join the output under the
same rule. **Grouping** the output rings of each output into polygons with holes is by containment
(a vertex of one ring not on another, threshold off, against that ring; nesting depth even for an
exterior and odd for a hole of its immediate container); under the interior rule the point set does
not depend on the grouping, so the grouping is for the user, and may be the simplest consistent
one. **Polylines** stop after classification: split at the intersections with `Q`, each piece in
or out by its midpoint, a piece along `Q`'s boundary by the rule chosen for it (open question).
**Provenance** is carried by the pieces from step 2.

**Union and the other Boolean operations** (the developer asked for them, 2026-10-25).
Partitioning needs two of them, `P ∩ Q` (inside) and `P \ Q` (outside), and the same
classification gives every one. Write the operation as `f(in P, in Q)`: intersection is *and*,
union *or*, `P \ Q` is *in `P` and not in `Q`*. A non-common piece or a common chain is on the
output's boundary exactly when `f` differs between its two sides, and the walk round a vertex gives
both sides' memberships for every piece-end. A non-common piece of `P`, for example, is in `P` on
one side and not on the other, and in `Q` on both sides or on neither (its test), so it is kept for
the intersection when it is inside `Q` and for the union when it is outside. Each kept piece is
oriented with the output on the same side (its left, say), which reverses `Q`'s pieces for
`P \ Q`, and the linking is then the same for every operation. The symmetric difference keeps every
non-common piece and no common chain.

One limit is new. `P ∩ Q`, `P \ Q` and `Q \ P` are subsets of an input, so they are under a
hemisphere whenever the inputs are (section 1.6). A union, or a symmetric difference, of two
polygons under a hemisphere can exceed one, and a ring cannot mean more than a hemisphere (design
doc section 3.1; the fix is deferred in its section 7), so such a result would be read as its
complement. The overlay knows which side of each output ring is meant, so it can detect this and
fail rather than return the complement.

For it:

- **An error stays where it is made.** Each piece's class comes from one exact test of one point
  far from the piece's ends; nothing is integrated along the ring. The linking then either
  succeeds or reports a local inconsistency. Compare (a), where the same wrong sign inverts the
  rest of the ring and nothing reports it.
- **Agreement with `is_point_in_polygon` is by construction**: the classifier *is*
  `is_point_in_polygon`, and the oracle of PLAN.md Phase 5 (random points inside an inside piece
  iff inside both) tests exactly the same function at other points.
- **Holes, self-crossing rings and the cascade's overlaps are not special cases.** Every ring of
  both polygons is pieces; a common chain is a piece with the sector rule; a self-crossing of `P`
  on `Q` is a vertex with more piece-ends once short pieces are contracted, affecting only the
  rotational pairing.
- **Every Boolean operation from one classification**, partitioning's two outputs among them, and
  polylines through the same code.
- **The four bugs**: `Q` wholly inside `P` (step 4 classifies `Q`'s ring against `P`), the two
  interior-ring bugs (R6; all rings are pieces), and the overlap-by-orientation bug (the sector
  rule replaces the bend test).

Against it:

- **Cost**: about `2k` point-in-polygon tests per cut for `k` intersections, plus one per ring
  without intersections. This is Kim and Kim's `O(kn)` that Foster-Hormann-Popa reject. Without
  set-up it is `O(kN)` per cut; with `PolygonOnSphere::is_point_in_polygon`'s adaptive policy
  (design doc section 3.9) the bounds are built after 8 calls and the lune tree after 20
  edge-testing calls, after which it is `O(N + k log N)`, the same order as the intersection
  phase. At `N = 10^4` and `k = 10^2` a low-speed test is about 50 microseconds (the table in
  section 3.9 scaled linearly from `N = 1024`) and the whole classification well under 10
  milliseconds even without set-up; the intersection phase, and the cascade over hundreds of
  plates, dominate.
- **The rotational order at a vertex** is new code and is tolerance-sensitive when two arcs leave
  a vertex nearly tangent. It affects only how touching rings are decomposed, not the point set.
- **The sector rule is not from a paper.** It is the Boolean reasoning of Martínez et al.'s overlap
  rule applied at a vertex; Phase 4 must state it precisely and the Phase 5 hunt must test it,
  especially its anchorless case.
- **Pieces shorter than the tolerance** (two touches within `1e-6` of each other, as a `1.5e-6`
  segment crossing an edge produces, or a twist of `P` on `Q`) would get an exact but arbitrary
  class. Contracting them (above) removes the problem, at the price of moving the point set by up
  to the width of a contracted cluster.
- **The sliver problem is not solved, only confined.** Where two plates' boundaries differ by more
  than the thickness, the cascade produces slivers that wide, exactly classified. Only a snapping
  tolerance equal to `s` would absorb them (R7).

### 3.4 Alternatives considered

**All plates at once, cascade priority kept otherwise.** Cut the original `P` by every plate
independently, so that no rounded piece is ever fed back, and give each resulting piece to the
first plate in order that contains it. This is sound only when the plates do not overlap, which
topological plates satisfy and static polygons do not: a piece of `P` inside plate `k` may straddle
the boundary of an earlier overlapping plate, and removing that part is a boolean difference
against the earlier plates, which is the cascade again. A full arrangement of `P` with all plate
boundaries at once, with each face labelled by its containing plates, avoids feedback and
classifies every edge once, but it is a rewrite of `GeometryCookieCutter`, needs the plates
overlaid with each other (where every shared boundary is an overlap chain), and serves no present
caller better than the cascade. Rejected for now; it becomes worth measuring if the Phase 5 hunt
shows the cascade's slivers matter. Schettino's cascade dropped slivers by a vertex-count
threshold, which loses area and is rejected.

**Exact arithmetic** (the CGAL route): would not change the answers for data that agree only
within epsilon (section 1.5).

**(a) with (b)'s test as a check**: trace by Greiner-Hormann and verify each traced piece by its
midpoint, falling back to (b) on disagreement. Two algorithms to maintain for the robustness of
one; rejected.

### 3.5 Assessment

| Criterion | (a) Greiner-Hormann + Foster-Hormann-Popa | (b) Overlay | All plates at once |
| --- | --- | --- | --- |
| Degeneracies, the cascade's shared boundaries | Handled by chain labelling; one wrong label inverts the rest of the ring, silently | Common chains by the sector rule; a wrong test affects one piece and is detectable at the linking | Same as (b) per cut, with fewer rounded inputs |
| Holes in both polygons | Yes (every component is pieces) | Yes, uniformly | Yes |
| Self-crossing rings under the parity rule | Yes, unless the crossing lies on the other polygon | Yes; a crossing on the other polygon is a vertex with more ends | As (b) |
| The sphere | Side tests become triple products; no unbounded face needed since the start status comes from `R` | Same; the midpoint test is `is_point_in_polygon` | Same |
| Agreement with `is_point_in_polygon` | At one vertex per ring by construction, elsewhere by the labels | At every piece by construction | As (b) |
| Inside and outside pieces | Two traces with reversed rules | One classification, two linkings | One classification |
| Polylines through the same machinery | A separate alternation along the polyline | Classification only, same code | Same |
| Grouping rings into polygons with holes | Separate containment step | Separate containment step | Same |
| Provenance | From the split, both | From the split | From the split |
| The four bugs | Fixed by steps 1 to 4 of section 3.1 and chain handling | Fixed | Fixed |
| Cost per cut | Intersection phase + `O(k)` + one test per ring | Intersection phase + `O(N + k log N)` | A multi-polygon arrangement; heavier, once |
| Implementation complexity | Labelling with five overlap cases and two chain kinds, two traversal rule sets, three clean-ups, two fallbacks | Piece classification, sector rule, rotational order, linking, checks | A new cookie cutter |

### 3.6 Recommendation

**(b), the overlay**, with the sector rule for common chains and the rotational pairing at
vertices. The reasons, in order of weight:

1. Robustness is the whole problem. The cascade makes overlaps the normal case, and the data
   coincide only within epsilon, so tolerance-scale decisions are unavoidable. (b) makes each such
   decision affect one piece and makes a wrong one visible at the linking; (a) integrates them
   along the ring and hides a wrong one in a closed, plausible ring.
2. Agreement with `is_point_in_polygon` by construction, everywhere, which is also what makes the
   Phase 5 oracle meaningful.
3. One classification serves both outputs, every other Boolean operation and polylines, and holes,
   twists and overlaps need no separate paths.
4. The cost objection does not apply at plate sizes.
5. Foster-Hormann-Popa's one documented failure, a twist of `P` lying on `Q`, is an ordinary vertex
   for the overlay once short pieces are contracted. (a) would need `P` split into simple rings
   first, their own remedy, and that split is the overlay's machinery applied to `P` alone
   (section 3.7).

Foster-Hormann-Popa is still used: its phase-1 case table is the audit's specification (section 2),
its chain reasoning defines the common pieces (section 3.1, step 3), and its split-candidate
clean-up is what the rotational pairing achieves. `GeometryCrossing`'s intent (reduce contacts to
crossings) is subsumed by the common-piece marking and is not needed as a separate pass.

Risks that remain with (b):

- The sector rule and the anchorless ring need a precise statement and tests (Phase 4 and 5).
- The rotational order at a vertex with nearly tangent arcs; harmless to the point set, but the
  ring decomposition there is arbitrary.
- Contracted clusters wider than the tolerance, and slivers wider than it between adjacent plates.
- The inconsistency-repair policy, which must not turn a detected error back into a silent one.
- `GeometryIntersect`'s known gaps (R7, R8, R9) are inherited by both candidates and are Phase 3's
  work; (b) does not remove the need for the audit.

### 3.7 The same machinery for one polygon

Splitting a polygon into simple polygons with holes, with the same interior, is the overlay of the
polygon with itself (raised by the developer, 2026-10-25, as a possible later PR):

1. Intersect every ring with itself and with every other ring of the polygon. This is the one new
   primitive. `GeometryIntersect` intersects two geometries; a self-intersection must also skip each
   segment against itself and against its neighbours, which always meet at their shared vertex
   (looking past zero-length segments, and across a ring's wrap-around), and pair the bounding tree
   with itself, each pair of nodes once.
2. Split, and contract short pieces, as in sections 3.1 and 3.3.
3. Classify without a point-in-polygon test. Under the parity rule, crossing any piece changes the
   parity by one, so every piece is boundary and is kept, except a stretch the polygon covers twice
   (a spike, a ring retracing itself, two rings sharing an edge), which is a common chain of the
   polygon with itself and cancels.
4. Link by the rotational pairing, so that lobes touch at the former crossings instead of crossing,
   and group the rings by containment.

The result keeps GPlates' meaning of the polygon, parity, rather than reinterpreting it by winding
number. Two uses: export to formats whose polygons must be simple (OGC simple features), and the
exact parity area for rings that cross each other, deferred in design doc section 7, which needs
exactly these simple rings. Partitioning does not need it, since the overlay takes twists as they
are (section 3.3).

## 4. Open questions for the developer

1. **Threshold semantics of the output.** Classify pieces with the threshold off, exact parity
   (recommended), accepting that a threshold-on query calls a point within `s` of the boundary
   inside? And should the intersection thickness be raised to `s`, so that a vertex
   `is_point_in_polygon` calls on the outline is always a split point (R7)?
2. **A polyline piece along the boundary.** Inside, matching the inclusive threshold (recommended),
   or decided by which side of the boundary the polygon's interior is?
3. **Common chains.** The sector rule as primary and the offset test as the anchorless fallback;
   or the offset test throughout, which is simpler and has a thin-face failure mode. How large an
   offset?
4. **Pieces shorter than the tolerance**: contract them, merging their end vertices (recommended,
   section 3.3; it is also what makes a twist of `P` on `Q` work), rather than keep them as
   duplicated points or drop them?
5. **Inconsistency policy**: repair and continue in release builds, assert in debug builds, or
   return a failure to the caller?
6. **Orientation of output rings**: keep each piece's original direction (so an output ring can
   reverse direction where it switches from `P` to `Q`), or normalise every output ring? The
   interior rule does not care; users and exports might.
7. **Grouping**: strict nesting by containment, with rings allowed to touch; is a stricter
   Shapefile-style output wanted?
8. **The cascade**: keep it sequential on (b) (recommended), and decide on all-plates-at-once only
   after the Phase 5 hunt measures the slivers.
9. **Where R3 and R4 live**: derive crossings and chains in the partitioner from the four types
   (recommended, keeping `GeometryIntersect`'s contract and its other callers untouched), or extend
   the `Graph` to report them?
10. **`GeometryCrossing`**: delete it.
11. **Scope of self-crossing input**: accept figure-of-eight output rings where the rotational
    order is ambiguous, or always split them?
