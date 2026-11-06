# Phase 4b, step 3: the design of polygon partitioning and Boolean operations

Written 2026-10-31 (Fable), from `overlay-walkthrough.md` (the algorithm), `phase4a-run2.md` and
`phase4a-run3.md` (the vertex rules as finally measured), `phase4a-variants.md` sections 10 and 11,
`phase4b-survey.md` (the consumers), `phase2-requirements.md`, the prototype in the planning
repository (`plans/polygon-intersection-tools/overlay_proto.py`, `overlay_polyline.py`,
`overlay_cut.py`) and the code at `82a72e1c5`. It is the design an implementer builds PR B from
(Phase 5); it is a plan document and stays in the plan until Phase 6 writes the design doc from the
code. Every decision in PLAN.md's Status entries of 2026-10-30 and 2026-10-31 is taken as given and is
not reargued here; section 13 records what was rejected and why, for the design doc later.

**Amended 2026-11-04 by run 4** (`phase4a-run4.md`, adopted by the developer: `CONFIGS['r4t']`): the
one-passage rule (3.7.1 case 1b), the spike rule's tip condition (3.4), and the bundle rule only where
both polygons pass more than once (3.6, 3.8). The passages so amended say so.

Where the walk-through, the run reports and the survey disagree, the later document wins (run 4 over
run 3 over run 2 over the walk-through for the vertex rules). The two places it matters: run 3 corrects the
walk-through's 7.6 and 7.7 mechanism ("along one thick plane" becomes "a short first arc leaving
from off the vertex's output position", run 3 section 2), and the walk-through's 7.4 is read with the
claim check's correction that the `LL` rule fixes only its own polygon's other class (run 2 section 12,
last bullet; the prototype's `_solve_free`).

Conventions. `P` is the subject (the geometry being cut: a polygon or a polyline), `Q` the clip (the
partitioning polygon). `h` is the thickness of `GeometryIntersect`, which is the point-on-outline
threshold `s` (about `1.414e-6` rad; `GeometryIntersect::Intersection::get_on_segment_start_threshold_sine()`,
`src/maths/GeometryIntersect.h:120-122`). `k` is the number of intersection records, `N_P` and `N_Q`
the vertex counts, `R` a ring count. File references are `path:line` at `82a72e1c5`; prototype
references are `overlay_proto.py:line` unless another file is named. Statements not checked against
code or a run report are marked *unverified*.

## 1. Scope and summary

PR B replaces the internals of `PolygonPartitioner` (`src/maths/PolygonPartitioner.{h,cc}`) and
deletes `PolylineIntersections` (`src/maths/PolylineIntersections.{h,cc}`). A new
`src/maths/PolygonOverlay.{h,cc}` holds the core: the overlay of two polygons with holes, every
Boolean operation on it, polylines through the same core, and provenance for every output point.
`PolygonPartitioner` keeps its interface as the partition layer on top (its polygon output changes
from polylines to polygons, and it gains provenance). The consumers change as the survey listed:
`GeometryCookieCutter` cascades polygons and composes provenance; `PartitionFeatureUtils` maps scalar
coverages by provenance and measures "most overlapping" by area; pyGPlates loses two warnings, gains
polygon output, coverages in `partition_features`, and area; `GLReconstructedStaticPolygonMeshes`
takes a result-only path; CitcomS mesh generation calls `GeometryIntersect` directly.

What the overlay does, in one paragraph (walk-through section 1): intersect every ring of `P` with
every ring of `Q` (`GeometryIntersect::intersect`, all rings); group the records into *vertices* by the
original vertex they name (identity, no distance); cut every ring into *pieces* between consecutive
vertices; make the loop and spike pieces internal; pair the *common* pieces (`P` and `Q` running
together); classify each piece as inside or outside the other polygon, by one exact point-in-polygon
test (threshold off) at an original vertex when it has one, else by deduction from its end vertices
(the four-end rule, the `LL` rule, the sector walk with the bundle rule at six or more ends), else by
a midpoint test in order of margin; classify common chains by the sector rule at a chain end, or by
option 3 when anchorless; check every vertex for consistency and repair; keep the pieces that bound
the requested output; pair the kept ends at every vertex and walk them into rings; split rings that
pass a vertex twice; drop collapsed rings; group the rings into polygons with holes; optionally drop
slivers. Inside and outside (partitioning) are the intersection and the difference, extracted from
one classification.

What it fixes (doc `spherical-geometry.md` section 5.1, "What goes wrong"): an intersecting polygon
comes back as polygons with holes, not polylines; a partitioner wholly inside the input cuts it; both
polygons' interior rings take part; a piece along the partitioner's boundary is classified by the
sector rule, not by ring orientation. It also fixes the survey's coverage findings: the
`partition_range` read past the end (`src/app-logic/PartitionFeatureUtils.cc:522-523`, `:539`) goes
with the code that has it, and pyGPlates' `partition_features` stops dropping coverages.

What it does not do: `DateLineWrapper` is not ported (its own PR; the data structures keep the three
hooks of run 2 section 10, section 6.3 below); self-crossing input is carried through, not
simplified (requirements 3.7); the slivers filter is designed but its default and width wait for
Phase 5's measurement; PR C exposes the Boolean operations in pyGPlates.

## 2. Data structures

Everything below lives in `src/maths/PolygonOverlay.{h,cc}`, namespace `GPlatesMaths::PolygonOverlay`.
The public header exposes the `Overlay` class, `Operation`, `Provenance`, the output types, `Options`
and `Diagnostics`; the working structures (rings view, stops, pieces, vertices, rays) are in the `.cc`
(an anonymous namespace or a `PolygonOverlayInternals` namespace, for the tests to reach). Sketches
only; names are proposals.

### 2.1 Lifetime and memory

One `Overlay` object is one call: it holds references to the two inputs, owns the `GeometryIntersect::Graph`
and every working vector, and dies with the call. Nothing is cached on the inputs beyond what
`PolygonOnSphere` already caches (its point-in-polygon tester, bounds and outside point,
`src/maths/PolygonOnSphere.h:1681-1713`). All working storage is `std::vector`, reserved from the
counts known after `intersect()` (`k` records, so at most `k` vertices, at most `k + R` pieces, at most
`2k + 2R` ends). Nothing is heap-allocated per element. The cascade (`GeometryCookieCutter`) makes one
`Overlay` per plate per geometry; if Phase 5's timing shows the allocations matter, a `Workspace`
holding the vectors can be kept in `PolygonPartitioner` and reused (mutable, not thread-safe, which
matches the polygon's own cache, doc section 3.10). Start without it.

### 2.2 The inputs

`P` is first in `GeometryIntersect::intersect(graph, P, Q)`: an `SS` record is placed at the first
geometry's vertex (`GeometryIntersect.h:72-77`), which is also the overlay's output position for a
vertex holding a `P` vertex (2.5), so the record's position and the output position agree. Today's
partitioner passes the partitioning polygon first (`PolygonPartitioner.cc:421-426`, `:474-479`); the
overlay swaps that. The overloads needed exist: polygon with polygon, polyline with polygon
(`GeometryIntersect.h:389-395`, `:408-413`), both with interior rings included (the default).

```
enum Side { SUBJECT = 0, CLIP = 1 };     // P and Q

/**
 * One ring of one input (or the whole polyline), as the overlay reads it.
 *
 * Segment indices in a Graph run through the exterior ring and then each interior ring
 * (GeometryIntersect.h:206-212); 'd_first_segment_index' maps a global index to this ring.
 * Vertex j of the ring is the start point of its segment j (PolygonOnSphere.h:613-631).
 */
class RingView
{
public:
	Side d_side;
	unsigned int d_ring_index;                 // 0 exterior (or the polyline), 1.. interior ring + 1
	unsigned int d_first_segment_index;        // global index of the ring's segment 0
	unsigned int d_num_segments;               // ring: == vertices; polyline: vertices - 1
	bool d_closed;                             // false only for a polyline (and, later, an open clip arc)
	PolygonOnSphere::ring_const_iterator d_segments_begin;   // or PolylineOnSphere::const_iterator
	unsigned int d_first_stop, d_end_stop;     // its stops, contiguous in the overlay's stop vector
};
```

`PolygonOnSphere` gives each ring's segments and counts directly (`exterior_ring_begin()`,
`number_of_segments_in_exterior_ring()`, `interior_ring_begin(i)`,
`number_of_segments_in_interior_ring(i)`, `PolygonOnSphere.h:938-987`, `:1209-1270`); the vertex
positions come from `GreatCircleArc::start_point()` (`GreatCircleArc.h:156`), and a polyline's last
vertex from its last arc's `end_point()`. A polyline's last vertex is the start of the fictitious
segment `number_of_segments()` in a record (`GeometryIntersect.h:57-61`), so the view maps that index
to vertex `d_num_segments`.

### 2.3 Records

The `Graph` is kept as is (Q9: chains are derived here, the `Graph` is unchanged). The overlay reads
`unordered_intersections[i]` for the kind, position, both segment indices and both angles
(`GeometryIntersect.h:158-192`), and the two ordered index sequences (`:249-260`). A record index is an
`unsigned int` into `unordered_intersections` everywhere below.

What a record names (`GeometryIntersect.h:214-222`): `SS` names the start vertex of both segments, `SL`
the first geometry's (`P`'s) segment start, `LS` the second's (`Q`'s), `LL` none. In the overlay's
terms (walk-through 7.1): a record *at a vertex* of `P` is `SL` or `SS`; at a vertex of `Q`, `LS` or
`SS`; an `LL` record is at a vertex of neither.

### 2.4 Stops

A *stop* is a point at which a ring is cut, or an original vertex between cuts. The stops of a ring
are built in ring order (3.3) and stored contiguously, so a piece is a cyclic range of its ring's
stops and no positions are copied.

```
class Stop
{
public:
	enum Kind { ORIGINAL_VERTEX, RECORD, DANGLING_END };

	Kind d_kind;
	unsigned int d_segment_in;                 // ring-local: the segment arriving at this stop
	unsigned int d_segment_out;                // ring-local: the segment leaving it (== in, for a record inside a segment)
	bool d_at_vertex;                          // the stop is ring vertex 'd_segment_out' (always, for ORIGINAL_VERTEX)
	boost::optional<unsigned int> d_vertex;    // the overlay vertex, for RECORD
	unsigned int d_first_record, d_num_records; // its records, in the Overlay's per-stop record list (usually one)
	PointOnSphere d_position;                  // the original vertex, or the record's position
	boost::optional<double> d_ratio;           // along 'd_segment_in' (== out), for a record inside a segment: 3.3
};
```

`DANGLING_END` is a polyline's first or last vertex when no record is there: a piece boundary with no
overlay vertex (`overlay_polyline.py:57-65`, the `End` stand-in). It is also what an open clip arc
needs (6.3).

### 2.5 Vertices

An overlay vertex is the set of records that name a common original vertex, closed transitively
(walk-through 7.2; `_build_vertices`, `overlay_proto.py:390-433`).

```
class Vertex
{
public:
	unsigned int d_first_record, d_num_records; // its records (indices into a per-vertex record list)
	boost::optional<RingVertex> d_subject_vertex;  // (ring, vertex index) of a P vertex it holds, the first found
	boost::optional<RingVertex> d_clip_vertex;     // likewise for Q
	PointOnSphere d_position;                   // P vertex if any, else Q vertex, else the first record's position
	std::vector<End> d_ends;                    // the piece ends here, after the loop and spike rules
	std::vector<Ray> d_rays;                    // in the measured rotational order (3.6)
	boost::optional<Bundle> d_bundle;           // the bundle rule's state at six or more ends (3.8)
};

class End
{
public:
	unsigned int d_piece;
	bool d_at_end;                              // false: the piece's start; true: its end
};
```

The output position rule is `_set_position` (`:448-461`): `P`'s vertex if the group holds one, so
`P`'s vertices stay exact for scalar coverages; else `Q`'s; else the record's. "First found" is the
lowest record index naming a `P` vertex, which is deterministic for given records. A chained group
(several `P` vertices within one group, walk-through 7.2 "the cost of identity") outputs one position;
its other `P` vertices are stops of pieces the loop rule makes internal, so they are not output and
their coverage values are dropped (they lie within the group's extent, which is set by the data).

### 2.6 Pieces

```
enum Class { CLASS_UNKNOWN, CLASS_INSIDE, CLASS_OUTSIDE };            // non-common: inside the other polygon?
enum CommonClass { COMMON_UNKNOWN, COMMON_SAME_SIDE, COMMON_OPPOSITE_SIDES };
enum Method { METHOD_NONE, METHOD_VERTEX_TEST, METHOD_DEDUCED, METHOD_MIDPOINT_TEST,
		METHOD_SECTOR, METHOD_ANCHORLESS, METHOD_REPAIRED, METHOD_ALONG /*polylines, Q2*/ };

class Piece
{
public:
	unsigned int d_ring;                        // index into the rings view
	unsigned int d_start_stop, d_end_stop;      // indices into the ring's stops; a closed piece: both the ring's first stop
	bool d_closed;                              // a ring with no record (one piece), or a polyline's whole length
	bool d_has_interior_vertex;                 // an ORIGINAL_VERTEX stop strictly inside the piece
	boost::optional<unsigned int> d_single_arc_segment;  // ring-local segment index when the piece is one arc
	Class d_class;
	CommonClass d_common_class;
	Method d_method;
	boost::optional<unsigned int> d_common;     // the partner piece of the other polygon, when common
	bool d_internal;                            // a loop or a spike: not classified, not a ray, not output
};
```

A piece's points are read from its ring's stops, cyclically from `d_start_stop` to `d_end_stop`
(both included); its arcs are the ring's segments between them. Its length and midpoint, needed only
for stalled single arcs and the bundle rule, are computed on demand from the two stop positions
(`normalise(a + b)` for the midpoint, as `classify()` does at `:1148`).

### 2.7 Rays

A *ray* is what leaves a vertex along one or two pieces (a common pair leaves as one ray, walk-through
section 2). Built by `_build_rays` (`:682-730`) after the loop and spike rules.

```
class Ray
{
public:
	Vector3D d_direction;                       // cross(segment plane normal, start position); not normalised (3.6)
	bool d_has_direction;                       // false when every arc of the piece is zero length (3.6)
	bool d_flips_subject, d_flips_clip;         // which memberships change across it (both, for a common ray)
	boost::optional<End> d_subject_end;         // the P piece end it carries (the P piece of a common pair)
	boost::optional<End> d_clip_end;            // the Q piece end
	// The bundle rule (run 3 section 3.1): where the ray's first arc starts relative to the
	// vertex's output position, how long that arc is, and the cone half-angle.
	double d_delta, d_first_arc_length, d_reach, d_cone;
};

class Bundle                                     // one per vertex with six or more ends
{
public:
	std::vector<unsigned int> d_anchored;       // ray indices, in the measured cyclic order
	std::vector<unsigned int> d_floating;
	std::vector< std::vector<unsigned int> > d_admissible_orders;   // the measured order first
	unsigned int d_chosen_order;                // index into d_admissible_orders, after the solve
	bool d_capped;                              // too many floating rays: the measured order was kept
};
```

### 2.8 Output and provenance

```
/**
 * Where an output point came from. 'd_subject' is relative to the ORIGINAL subject geometry
 * (composed through a cascade, section 7); 'd_clip' to the clip of the overlay that made the point.
 */
class Provenance
{
public:
	class Location
	{
	public:
		unsigned int d_ring;                    // 0 exterior (or the polyline), 1.. interior ring + 1
		unsigned int d_index;                   // a vertex index, or a segment index when d_ratio is set
		boost::optional<double> d_ratio;        // none: the vertex itself; else angle fraction along segment d_index
	};

	boost::optional<Location> d_subject;        // none: a point of the clip's boundary inside the subject (a cut)
	boost::optional<Location> d_clip;           // set for every point that lies on the clip's boundary
	unsigned int d_clip_index;                  // which clip in a cascade made d_clip (0 outside a cascade)
};

typedef std::vector<Provenance> provenance_seq_type;   // one per point, in GeometryUtils::get_geometry_points order

class OutputRing
{
public:
	std::vector<PointOnSphere> d_points;
	provenance_seq_type d_provenances;          // same length; empty when provenance was not requested
	// (d_output_on_left dropped in step 8: every ring of an OutputPolygon has the output on its left, Q1, 4.3)
};

class OutputPolygon
{
public:
	OutputRing d_exterior;
	std::vector<OutputRing> d_interiors;
};

class OutputPolyline
{
public:
	std::vector<PointOnSphere> d_points;
	provenance_seq_type d_provenances;
	bool d_inside;
};
```

Ratios are angle fractions along the segment, as `GreatCircleArc::point_on_arc` takes them
(`GreatCircleArc.h:218-227`) and as the polyline prototype's oracle had to use (`overlay_polyline.py:271-282`:
a chord fraction differs by `theta^2/8` of the segment, `2h` on a `0.03` rad segment). They are
computed from positions, not from a record's `angle_in_segment` (which only orders,
`GeometryIntersect.h:179-187`): a crossing's ratio is `angle(start, position) / angle(start, end)`; a
touch's ratio along the touched segment uses the touching vertex's projection onto the segment's great
circle, as the ordering key does (`GeometryIntersect.cc:163-173`), because the vertex itself is up to
`h` off the segment. This is the style of `DateLineWrapper::LatLonPolygon::InterpolateOriginalSegment`
(`src/maths/DateLineWrapper.h:108-129`: a ratio and an original segment index), extended with the ring
and the clip location.

### 2.9 The public class

```
enum Operation { INTERSECTION, DIFFERENCE, REVERSE_DIFFERENCE, UNION, SYMMETRIC_DIFFERENCE };

enum Result { RESULT_INSIDE, RESULT_OUTSIDE, RESULT_INTERSECTING };   // maps onto PolygonPartitioner::Result

class Options
{
public:
	PolygonOnSphere::PointInPolygonSpeedAndMemory d_clip_speed;   // for tests against Q (the partitioner's)
	bool d_want_provenance;
	const provenance_seq_type *d_subject_provenance;               // the subject's own table, for composition (7.2)
	unsigned int d_clip_index;                                     // recorded in every clip provenance
	boost::optional<double> d_sliver_min_width;                    // radians; none: filter off (4.5)
	bool d_polyline_along_is_inside;                               // Q2: true
};

class Overlay
{
public:
	Overlay(
			const PolygonOnSphere &subject,
			const PolygonOnSphere &clip,
			const Options &options);
	Overlay(
			const PolylineOnSphere &subject,
			const PolygonOnSphere &clip,
			const Options &options);

	/**
	 * Intersect, build, classify, check and repair (steps 1 to 9 of section 3). With
	 * @a stop_when_result_known, stops at the first certain answer (section 10.2).
	 * Throws InconsistentOverlayException if a check still fails after repair (section 9).
	 */
	Result
	classify(
			bool stop_when_result_known = false);

	/**
	 * The output of one operation: rings linked, split, cleaned and grouped (sections 3.10 to 3.12, 4).
	 * Throws InconsistentOverlayException on a hemisphere failure (section 5.2).
	 */
	void
	extract(
			Operation operation,
			std::vector<OutputPolygon> &output) const;

	//! For a polyline subject: the pieces inside and outside (section 6).
	void
	extract_polylines(
			std::vector<OutputPolyline> &output) const;

	const Diagnostics &
	diagnostics() const;
};
```

`Diagnostics` is the prototype's `new_stats()` (`:278-309`) cut down to what Phase 5 asserts on and
what a failure report needs: vertex tests, deductions, midpoint tests (and how many were band
pieces), anchorless chains, check failures, repairs, trusted classes flipped, unresolved pieces,
pairing failures, bundle vertices and the largest bundle, collapsed rings dropped, repeated positions
removed, slivers dropped, and the all-vertices disagreements of 9.3.

## 3. The algorithm, step by step

The map from the prototype to the C++. Each step names the prototype function it translates, the
walk-through section, and the kind of every decision it makes: **exact** (a sign test whose answer is
right for its own inputs; a flip needs the inputs to be within rounding of the boundary), **rounded**
(a tolerance or a measured quantity, where a flip must be shown harmless or caught), or
**combinatorial** (no geometry at all). `classify()` runs steps 1 to 9; `extract()` runs 10 to 12 and
section 4.

| Step | Prototype | Walk-through | Decisions |
| --- | --- | --- | --- |
| 1 Intersect | `intersect_rekeyed` (`:113-121`), replaced by `GeometryIntersect::intersect` | 2 | made by the primitive (doc 4.4) |
| 2 Vertices | `_build_vertices` (`:390-433`), `_set_position` (`:448-461`) | 7.2 | combinatorial |
| 3 Stops and pieces | `_build_pieces` (`:465-521`); polylines `_build_polyline_pieces` (`overlay_polyline.py:112-162`) | 2 | combinatorial |
| 4 Loop and spike rules | `_contract` under V1b (`:578-606`) | 7.3, 7.6 | combinatorial |
| 5 Common pairs | `_find_common` (`:610-627`) | 4.1, requirements 3.1 step 3 | combinatorial |
| 6 Rays and their order | `_direction` (`:631-639`), `_build_rays` (`:682-730`), `_angular_sort_float` (`:739-771`), `_ray_cone` (`:652-680`) | 4.3, 7.6 | exact signs in doubles (rounded in principle; 3.6) |
| 7 Classification | `classify` (`:1108-1187`), `_classify_point` (`:804-812`), `_solve_vertex` (`:814-892`), `_solve_free` (`:1074-1106`), `_walk` (`:895-921`) | 3, 4.2, 7.4, 7.5 | exact (vertex tests), combinatorial (deduction), rounded (margin order) |
| 8 The bundle rule | `_bundle_split`, `_ray_angles`, `_admissible`, `_bundle_orders`, `_choose_order` (`:923-1062`) | 7.6 as corrected by run 3 section 3 | rounded (cones), combinatorial (the solve) |
| 9 Anchorless chains, checks, repair | `_chain_of`, `_anchorless_pick`, `_classify_anchorless` (`:1199-1265`); `check` (`:1269-1315`); `repair` (`:1317-1350`) | 5, 9 | exact (option 3), combinatorial |
| 10 Link | `kept` (`:1354-1362`), `link` (`:1381-1505`) | 6, 14 | combinatorial |
| 11 Split and clean | `split_repeats` (`:1510-1531`), `clean_rings_tagged` (`:1543-1571`) | 6.4, 7.9 | combinatorial; exact position equality |
| 12 Group, orient, filter | not in the prototype (its oracle reads parity over all rings) | 11, 14 | exact (containment, the hemisphere check's reference point), rounded (orientation, slivers) |

### 3.1 Intersect

`GeometryIntersect::intersect(d_graph, P, Q)` with every ring of both (the defaults). No records
and the two bounding circles disjoint is `RESULT_OUTSIDE` at once; no records otherwise goes to the
ring classification of section 4.1. The records come back ordered along each geometry, ring by ring,
with ties in discovery order (doc 4.4 "Ordering"), and the overlay accepts either order of a tie:
records of one vertex are one stop (3.3), and a tie between different vertices is a zero-length piece
whose class is deduced (walk-through 7.8).

### 3.2 Vertices by identity

Union-find over record indices. For each record, its named vertices are keys `(SUBJECT, segment_index1)`
for `SL` and `SS`, `(CLIP, segment_index2)` for `LS` and `SS` (a segment index is its start vertex's
index, 2.2). A hash map from key to the first record that named it; a later record naming the same key
is unioned with that one. `LL` records are singletons. The groups, in order of their lowest record
index, become the vertices; each vertex's `d_subject_vertex` and `d_clip_vertex` are the first `P` and
`Q` vertices named by its records in record order, and its position follows 2.5. This is exactly
`_build_vertices`; cost `O(k alpha(k))`. Records at one point that name no common vertex (two `LL`
records of a twist on `Q`) stay separate vertices by design (7.8).

### 3.3 Stops and pieces

For each ring (2.2), walk its records in the ordered sequence (they are contiguous per ring, since
segment indices run ring by ring) and its vertices in index order, producing stops in ring order,
as `_build_pieces` does per segment `k`:

1. The records on segment `k` that are *at the ring's vertex `k`* (`SL`/`SS` for a `P` ring, `LS`/`SS`
   for a `Q` ring; they have angle zero and sort first in the segment) make one `RECORD` stop at the
   vertex per distinct overlay vertex among them; a second record of the same vertex at the same ring
   position joins the stop (`:489-490`). *Corrected (Phase 5 review 1, 1.4):* there is only ever one
   such vertex. Every record at ring vertex `k` names the key `(side, k)`, so identity makes them one
   group, and they are one stop (the C++ asserts it). The "two stops at the same position" case does
   not occur.
2. If no record is at vertex `k`, an `ORIGINAL_VERTEX` stop (or `DANGLING_END` for a polyline's first
   or last vertex).
3. Then the records *inside* segment `k` (`LL`, and the other polygon's touches on this segment), in
   the ordered sequence's order, each a `RECORD` stop with `d_segment_in == d_segment_out == k`, the
   record's position, and its ratio (2.8).

A ring with no `RECORD` stop is one closed piece. Otherwise each consecutive pair of `RECORD` stops
(cyclically; the last with the first) bounds a piece, with the stops between them as its interior; a
ring with exactly one record stop has one piece from that stop all the way round to itself (`:511-512`).
Each non-closed piece registers its two ends with its vertices. A polyline's pieces are the same with
`DANGLING_END` stops as boundaries and no wrap-around (`_build_polyline_pieces`): a dangling end
registers nothing.

`d_single_arc_segment` is set when a piece has no interior stop and its two stops lie in one segment:
then the piece is one arc and a candidate for deduction and the midpoint test (3.7); it is also the
segment the anchorless fallback leaves out (3.9).

### 3.4 The loop rule and the spike rule

Combinatorial, on piece ends (`_contract` under V1b, `:578-606`):

- **Loop**: a non-closed piece with no interior stop whose two ends are the same vertex is internal
  (walk-through 7.3). A loop *with* an interior original vertex (the wedge tip `x, v, x`) is not.
- **Spike**: two consecutive pieces of one ring, neither closed, neither with an interior stop, going
  from vertex `A` to `B` and straight back to `A` (`a.end == b.start && b.end == a.start && A != B`),
  are both internal (7.6, run 2 section 1 item 6), **but only if the other polygon has at most two
  ends at the tip `B`** (ends of its pieces not yet internal, counted when the spike is tested; run 4
  sections 2.4 and 3.3, `spike_tip_once` in `_contract`). With two passages of the other polygon at the
  tip, the tip is a self-touch of that polygon, the hairpin can lie between its passages, and removing
  it removes a crossing (`chain#28` seed 2). A piece with a dangling end is skipped (run 3 section 1
  item 5: a dangling end is not a vertex).

After both, each vertex's `d_ends` drops the ends of internal pieces (`:604-605`). The count of ends at
a vertex stays even for each polygon (6.2).

### 3.5 Common pairs

Group the non-internal, non-closed pieces with no interior stop and distinct end vertices by the
unordered pair of their end vertices. A pair with exactly one `P` piece and one `Q` piece is common
(`:616-620`); any other mix (two `P` pieces and one `Q` between the same vertices) pairs nothing and is
counted. This is requirements 3.1 step 3 (an original vertex of either polygon inside the stretch
would itself be a record, so a piece with an interior vertex is not on the other polygon) and was
confirmed in every Phase 4a run (requirements "R4 is confirmed by use"). Direction is not needed:
the linking uses none (6.3).

**Self-touch cuts** (review 3, finding 1.2; configuration `r5c`, which the prototype comparison now
runs). The argument fails for a vertex of *another ring of the same input*: `GeometryIntersect` never
intersects an input with itself, so where an input touches itself on a stretch it shares with the
other input (a hole's tip on its own exterior's edge), the other input's segment is cut there (a
record names the tip) and the touched segment is not. Its one piece then faces a chain of the other
input's pieces, nothing pairs, all of them are band pieces with arbitrary classes, and the sector
walk could deduce wrong classes from them that every check accepted: a silent wrong intersection and
difference. So after the pairing, a single-arc piece with no partner, whose two vertices the other
input joins by a chain of single-arc pieces through vertices within the thickness of the piece's
segment (its own plane, also for a segment `GreatCircleArc` calls zero length) and in order along it
(each further than the last by more than rounding, `1e-12` radians, so that a vertex at the
segment's end is not taken for one inside it differently in doubles and in the prototype's
arithmetic), is cut at those vertices (a stop of the vertex that no record of the ring names), and
the stops, pieces, loops, spikes and pairs are built again, once (`Overlay::find_self_touch_cuts`,
`Diagnostics::self_touch_cuts`). The chain's arcs lie within the thickness of the piece, so the cut
pieces run with the chain's as common pairs; without the cut they were band pieces anyway. How often
it fires is measured in PLAN.md (the review 3 entry).

### 3.6 Rays and the rotational order

For each end at a vertex, one ray, except that the `Q` end of a common pair joins its `P` end's ray
(`:685-699`). The ray's direction is the exact tangent of the piece's arc leaving the vertex: the
cross product of the arc's plane normal (`GreatCircleArc::rotation_axis()`, `GreatCircleArc.h:216`)
with the arc's start position (the stop's `d_position`, which is where the arc really starts; for an
arriving end the arc's end position, negated), as `_direction` computes it (`:631-639`). For a common
ray, `P`'s direction, or `Q`'s if `P`'s first arc is zero length (`:697-698`).

**Zero-length arcs.** `rotation_axis()` throws `IndeterminateArcRotationAxisException` for a
zero-length arc (`GreatCircleArc.h:205-216`, `GreatCircleArc.cc:441-443`; `direction_on_arc` likewise,
`GreatCircleArc.h:236`). The C++ takes the first non-zero-length arc of the
piece from that end, as today's bend test does (`get_first_or_last_non_zero_great_circle_arc`,
`PolygonPartitioner.cc:322-360`), with that arc's start as `s_r` for the cone (3.8). A piece whose
arcs are all zero length has `d_has_direction == false`; it bounds no sector that any order needs
(at two to four ends the order is not used), and at six or more ends it floats with a cone of `pi`
(3.8), so the parity solve places it. The prototype did not model this (it used the exact cross
product of a near-zero normal); Phase 5 checks it on the catalogue's zero-length cases.

**The order.** Rays are sorted anticlockwise about the vertex's output position (seen from outside
the sphere) by the two sign tests of `_angular_sort_float` (`:739-771`): the half-plane of each
direction relative to the first ray (`dot(c, cross(d0, d))`, ties broken by `dot(d0, d)`), then
`dot(c, cross(a, b)) > 0` within a half-plane. Directions are normalised doubles. In principle a
rounded decision; measured (run 2 section 5, run 3 `bndflt`): no disagreement with the exact order at
any vertex where the order was used, down to rays `1.8e-15` rad apart, and a swap of two rays that
close changes neither the memberships between them nor the pairing (walk-through 7.6). The order is
used only at six or more ends where **both** polygons pass more than once (3.7, 3.8): at four ends it
is not consulted, and where one polygon passes once (3.7.1 case 1b, run 4) neither.

Each ray also records the bundle rule's quantities (`_ray_cone`, `:652-680`): `delta` = angle from the
vertex's output position `c` to the arc's start `s_r` (exactly `0` when `s_r` *is* the vertex's output
position: compared by vertex identity, not coordinates), `L` = the first arc's length (to the piece's
next stop), `reach` = angle from `c` to that stop, and the cone `theta = 0` if `delta == 0`, `pi` if
`L <= delta`, else `atan2(delta, L - delta)`. For a common ray both arcs are measured and the wider
cone kept. `atan2(delta, L - delta)` is the planar bound; at these scales the spherical correction is
far below rounding (run 3 open question 2), which the code comment should say.

### 3.7 Classification

`classify()` (`:1108-1187`), in this order:

**(a) Vertex tests.** Every non-internal, non-common piece with an interior original vertex is
classified by `is_point_in_polygon` of its *first* interior original vertex against the other
polygon, **threshold off** (`PolygonOnSphere::is_point_in_polygon(point, speed, false)`,
`PolygonOnSphere.h:1681-1685`). Exact, with margin: a vertex in no record is more than `h` from the
other polygon's outline (or it would be a touch), about `10^10` times point-in-polygon's rounding
(walk-through section 3). Against `Q` the speed is the partitioner's (`Options::d_clip_speed`, ADAPTIVE
by default, so `Q` reaches high speed as the cascade reuses it); against `P` it is ADAPTIVE on the
subject polygon, whose own cache builds bounds after 8 calls (doc 3.9). In debug builds every interior
original vertex is tested and a disagreement counted (9.3).

**(b) Deduction to a fixed point.** For every vertex, `solve_vertex(v)` (3.7.1) returns the classes it
can deduce; apply them; repeat while anything changed. Combinatorial.

**(c) Stalled single arcs, by margin.** Pieces still unclassified (non-common) are single arcs between
records. Compute each one's midpoint and its distance to the other polygon's outline
(`minimum_distance(point, polygon, false /*solid*/)`, `src/maths/GeometryDistance.h:223-227`, over
every ring); test the one with the **largest** distance at its midpoint (exact parity, threshold off),
then go back to (b). The distance is rounded, but it only orders: a piece with a margin is tested
before a band piece, and two pieces whose margins are within rounding of each other are either both
band pieces (then only slivers depend on the order, run 2 section 4) or both reliable (then neither
order propagates a wrong class). The principle is walk-through 7.5: a class decided inside the band
must never be the source of a deduction for a piece with a margin. Measured: with the `LL` rule the
remaining midpoint tests are band pieces (1 in 300 `near`, 24 in 300 `zero`; up to 250 in 300 dense
`shared`), and under the fair inversion adversary only slivers under `0.5h` wide changed (run 2
section 4; run 3 section 11 shows the C++ test cannot make the flips that broke the 10 residue cases).

**(d) Anchorless chains** (3.9), then (b) again.

**(e) Common `Q` pieces** take their `P` partner's class (`:1185-1187`).

#### 3.7.1 `solve_vertex`

The sector walk at one vertex (`_solve_vertex`, `:814-892`), returning whether the vertex is consistent,
the deductions, the per-sector memberships and the order used:

1. **Four ends, two of each polygon, none common** (`_is_four_end`): the geometry-free rule
   (`_solve_free`, `:1074-1106`), walk-through 7.4, combinatorial:
   - at a vertex that is **one `LL` record**, each polygon's two classes must differ (the record says
     the segments cross; doc 4.4: an `LL` needs both end points of each arc beyond `h` on opposite
     sides, so it is a true crossing). One known class fixes its *own* polygon's other class; two
     equal known classes of one polygon are a check failure. Nothing is said about the other polygon.
   - otherwise (touch records): `P` crosses iff `P`'s two classes differ, and the same for `Q`, and the
     two must agree. Three known classes fix the fourth; four that disagree are a check failure.
1b. **Six or more ends, one polygon with exactly two ends** (it passes once; a common ray counts as an
   end of both): the one-passage rule (run 4 section 3.1; `_is_once`, `_solve_once`), combinatorial.
   Any classes of the other polygon's rays can be realised by some order, so the measured order adds
   nothing and is not consulted. Let `X` be that polygon, `a` and `b` its two ends, and `m_in(e)` the
   other polygon's membership on the inside-`X` side of end `e`: the end's class for a plain ray, and
   for a common ray `1` if same side, `0` if opposite. The one relation is
   `m_in(b) = m_in(a) xor n`, `n` the number of the other polygon's rays classified inside `X`, mod 2.
   With exactly one term unknown (an end's class, or the one unclassified other ray's) it is deduced;
   with all known, a false relation is a check failure. Memberships for linking: the sector walk on
   the order `a, [other rays inside X, in measured relative order], b, [the rays outside]`, or its
   mirror if that has no solution. Run 4 found the sector walk wrong at such vertices in three ways,
   all with an exact but uninformative order (a crossing counted at both ends of a band arc; a long
   ray inside a `2h` common ray's direction gap; a corner cut off by the spike rule), in 0.8% of
   `chain` cases.
2. **Otherwise** the sector walk (`_walk`, `:895-921`), on the measured order of rays or, at six or more
   ends where both polygons pass more than once, with the bundle rule, on every admissible order (3.8). Each classified ray contributes a
   constraint on the two sectors beside it: a `P` ray with class `c` fixes `in Q = c` there, a `Q` ray
   fixes `in P`, a common ray with class same/opposite fixes `in P xor in Q = 0/1`. Walking round the
   order from a start `(a, b)` and flipping `P` at each `P` ray, `Q` at each `Q` ray, both at a common
   ray gives every sector's memberships; the four starts are tried and those meeting every constraint
   are the solutions. **No solution is a check failure.** A class is **deduced** for an unclassified
   ray only if every solution (over every consistent order, when bundled) agrees on it (`:860-881`);
   the memberships reported for linking are those of the chosen order's solutions, and a membership
   that differs between its solutions is left undetermined.

The vertices with two rays both common (inside a chain) go through case 2 with `xor` constraints,
which is how a chain's class travels from its end along the chain (walk-through 4.1).

### 3.8 The bundle rule (six or more ends)

Run 3 section 3.1, built in `_bundle_split`, `_ray_angles`, `_admissible`, `_bundle_orders` and
`_choose_order` (`:923-1062`). At a vertex with six or more ends (a common ray counting two) where both
polygons pass more than once (run 4: where one passes once, 3.7.1 case 1b applies instead; the hunts
reach this in about 1 vertex in 3500 cases, and run 3's residue vertices are all one-passage ones):

1. **Anchored and floating rays** (`_bundle_split`). Rays in order of certainty (smallest cone, then
   largest reach); a ray is anchored if, against every ray already anchored, the angle between their
   directions is at least the sum of their cones; else it floats. A ray with a cone of `pi` (or no
   direction) never anchors (run 4). At a vertex whose rays all start at
   its output position every cone is `0`, every ray is anchored, and nothing below changes anything.
   Rounded: the separability test. A flip at the margin moves one ray between anchored and floating,
   which either changes nothing (one consistent placement, the usual case: run 3 section 4.2, the
   placement moved from the measured order in 13 of 48 firings) or offers a swap across a neighbour
   the ray cannot be told from, touching only a sliver no wider than the group's extent (run 3
   section 3.2; measured within `0.42h` of a boundary, 4.2a).
2. **Admissible orders** (`_bundle_orders`, `_admissible`). The reference ray is the most certain
   anchored one; every ray's measured angle about the vertex is taken from it (`_ray_angles`), and
   each ray's interval is its angle plus or minus (its cone plus the reference's). An order is a
   placement of the floating rays into the gaps between the anchored rays (which keep their measured
   cyclic order), in any permutation within a gap, that can be realised with non-decreasing angles
   round the turn, each inside its interval (greedy earliest position, complete for interval
   feasibility). The measured order is always admissible and is placed first. Enumeration is
   `k^m` gap assignments times the permutations within gaps, filtered; measured maxima 4 floating
   rays, 120 admissible orders, 12 consistent placements (run 3 section 4.2). **Cap**: more than
   `BUNDLE_CAP = 6` floating rays keeps the measured order alone and counts the vertex (never reached
   in 3013 cases).
3. **The solve** (3.7.1 case 2 over every admissible order). Deduce only what every consistent order
   agrees on. **Tie-break** for the order used in pairing: the consistent order with the fewest
   inversions (Kendall distance) against the measured order, ties by enumeration order
   (`_choose_order`). The enumeration order must be deterministic in C++ (run 3 open question 3):
   gaps in anchored order, floating rays in certainty order, permutations lexicographic.

The rejected `nearest` deduction (deduce from the tie-break's order alone) is in section 13.

### 3.9 Anchorless chains, checks and repair

**Anchorless chains** (walk-through section 5, option 3; `_chain_of`, `_anchorless_pick`,
`_classify_anchorless`, `:1199-1265`). After the fixed point, a common `P` piece still unclassified
belongs to a chain no non-common piece reaches. Collect the chain (common pieces reachable through
vertices whose every ray is common); pick the arc whose great circle is farthest from the reference
point(s) below (largest `|dot(ref, axis)|`); classify it; rerun deduction so the sector walk carries
the class along the chain.

The classification is exact and needs **a new entry in `PointInPolygon`**, since
`get_num_polygon_edges_crossed` is file-local (`src/maths/PointInPolygon.cc:301`):

```
/**
 * The parity of crossings between the arc from @a from_point to @a to_point and every edge of
 * @a polygon except segment @a excluded_segment of ring @a excluded_ring, counted as
 * is_point_in_polygon counts them (exact sign tests, no threshold, low speed). @a to_point lies on
 * the excluded segment; the parity is that of the points just before it on @a from_point's side of
 * the excluded segment's great circle.
 */
bool
crossing_parity_excluding_segment(
		const PolygonOnSphere &polygon,
		const UnitVector3D &from_point,
		const PointOnSphere &to_point,
		unsigned int excluded_ring,
		unsigned int excluded_segment);
```

The C++ differs from the prototype in one respect: the prototype used one reference point for both
polygons and a random waypoint kept on its side (`_parity_on_side`, `:239-264`); the C++ has a
reference point per polygon (`PolygonOnSphere::get_outside_point()`, `PolygonOnSphere.h:1712-1713`,
which is outside that polygon) and a direct arc, which meets the excluded great circle only at `X`
whenever the reference point is not on that great circle. So:

- `X` is the midpoint of the chain arc maximising `min(|dot(R_P, axis)|, |dot(R_Q, axis)|)`; the
  chain arc's great circle must also be more than about `1e-6` rad from `R` and `-R` (the degenerate
  crossing arc bound, doc 3.8), or the second reference point `R2` and its cached status are used for
  that polygon instead (`PolygonOnSphere::get_second_reference_point`, doc 3.8).
- `in P` on `R_P`'s side of the great circle is the parity from `R_P` (outside `P`) leaving out `P`'s
  segment; `in Q` on `R_Q`'s side likewise. If `R_P` and `R_Q` are on opposite sides of the great
  circle (`sign(dot(R_P, axis)) != sign(dot(R_Q, axis))`), flip one membership, since crossing the
  chain flips it. The chain is `COMMON_SAME_SIDE` iff the two memberships on one side are equal.

Cost: one `O(N)` pass per polygon per anchorless chain, which run 2 section 7 and run 3 section 4.3
found right in every case (identical rings, jittered within `h`, holes equal to rings, thin faces, two
rings).

**Checks** (`check`, `:1269-1315`). Rerun `solve_vertex` at every vertex; a vertex with no solution
fails. This is both of walk-through 9.1's checks: at an ordinary vertex the redundant constraints
(`P`'s two classes against `Q`'s; a bounce at an `LL` vertex) and at a chain end the chain's class
arriving from its other end. Combinatorial. In debug builds, the all-vertices diagnostic of 9.3.

**Repair** (`repair`, `:1317-1350`; walk-through 9.3). Up to 20 rounds: take the first failing vertex;
its candidate pieces are the classified pieces at it (non-common, and the `P` side of a common pair),
ranked least reliable first by method (`MIDPOINT_TEST`, `DEDUCED`, `SECTOR`, `REPAIRED`, then
`VERTEX_TEST`) and then by length; flip the first whose flip makes the vertex consistent (a common
piece's flip flips its partner). **Whole-chain repair** (run 3 open question 4, kept as the safety
net): when the failing vertex is a chain end, one more candidate, ranked with `SECTOR`, flips the
class of every piece of the chain at once; piece-by-piece flips of a chain fail the chain's interior
vertices on the next round and run into the cap (the inversion residue's mechanism, run 3 section 3.4).
Count repairs and trusted flips. What Phase 4a measured: no repair in any normal run of the chosen
configuration (run 3 section 7), and the flips that could reach the residue cannot be made by the C++
test (run 3 section 11). A vertex still failing after repair is section 9's failure.

### 3.10 Linking

`kept(piece, op)` (`:1354-1362`) is walk-through section 14's table in one line: with `f` the
operation's Boolean function of `(in P, in Q)`, a `P` piece with class `c` is kept iff
`f(1, c) != f(0, c)`, a `Q` piece iff `f(c, 1) != f(c, 0)`, a same-side chain iff `f(1, 1) != f(0, 0)`,
an opposite-sides chain iff `f(1, 0) != f(0, 1)`. Section 5 lists `f` per operation.

`link(op)` (`:1381-1505`) pairs the kept ends at every vertex, then walks:

| Vertex | Pairing | Geometry used |
| --- | --- | --- |
| any end's piece unclassified | none: counted `unresolved` (a section 9 failure) | |
| an odd number of kept ends | none: `pairing_failure` (cannot happen with consistent classes, 6.2) | |
| two kept ends | the one pairing | none |
| four ends, two per polygon, none common (the free rule) | at a crossing, the sector `(mP, mQ)` with `f(mP, mQ)` is bounded by the `P` piece whose class is `mQ` and the `Q` piece whose class is `mP`; pair those. At a bounce, `P`'s two ends pair and `Q`'s two pair (the split pairing of 6.4, so the cut of 3.11 is never needed here) | none |
| only one polygon has ends left (the other's were a loop or spike) | **ring continuity** (run 3 section 5): each arriving end pairs with the end leaving from the same stop of its ring, the ring passing straight through | none |
| otherwise (six or more ends, or common rays) | `solve_vertex` without the free rule, on the chosen order (3.8): output membership per sector is `f(mP, mQ)`; a ray is kept iff the output differs across it (counted if not as the table says, `keep_mismatches`); pair consecutive kept rays across the output sectors | the rotational order |
| every ray common (identical inputs that touch themselves; review 3 1.1) | the sector walk knows the memberships only up to swapping both (a common ray fixes `mP != mQ`), so not which sectors are the output: pair consecutive kept rays in the order used, either phase; the split (3.11) then cuts a ring through the vertex twice into the one non-crossing decomposition | the rotational order |

The walk: start at any unused kept ray, append its vertex's position, then the piece's interior stop
positions (reversed when walking from the piece's end), arrive at the piece's other end's ray, take
its pair, until back at the start. Every kept piece has two ends and each is paired, so every walk
closes (9.1; a walk that does not is a pairing failure). Kept closed pieces (rings with no record)
are rings on their own. Along a common pair the walk follows the subject's piece, so the output's
positions there are the subject's (the clip's can differ by up to `h`), which is right for
partitioning; provenance (step 10, section 7) gives such a point both locations.

### 3.11 Split and clean-up

**Split** (`split_repeats`, `:1510-1531`; walk-through 6.4, Q11): a walked ring that passes an overlay
vertex twice (by vertex *identity*, not position) is `v, A, v, B`; cut it into `v, A` and `v, B` and
repeat until no ring repeats a vertex. Jordan's argument in 6.4 says the cut is always the other
non-crossing pairing. A twist of `P` on `Q` passes its two vertices `E` and `F` once each and is not
cut (7.8), as decided.

**Clean-up** (`clean_rings_tagged`, `:1543-1571`; 7.9), last and counted, never silent: remove
consecutive repeated positions (and a last position equal to the first), comparing position vectors
exactly (the repeats come from one stored position, so exact equality finds them); drop a ring with
fewer than three distinct positions. The dropped rings are the regions the records had already
collapsed (the wedge tip `x, v, x`), zero area by construction. Nothing else is merged or dropped
here; slivers are section 4.5's separate, optional rule.

### 3.12 Group, orient, filter

Section 4: exteriors and holes, the output side, the hemisphere check, orientation, the sliver filter,
then `PolygonOnSphere::create(exterior, interior_rings)` (`PolygonOnSphere.h:708-720`), which cannot
throw for a cleaned ring (at least three distinct points; `check_distinct_points` left false).

## 4. Holes, rings with no records, grouping, slivers

### 4.1 Rings with no records

Every ring of both polygons is a piece (walk-through section 10). A ring with no record is one closed
piece, classified by one vertex test, threshold off, with the full margin `h`. Before any test:
`P`'s and `Q`'s bounding small circles (`PolygonOnSphere::get_bounding_small_circle()`,
`PolygonOnSphere.h:1802`, cached) disjoint means no ring of either is inside the other, so the result is
`RESULT_OUTSIDE` with no test; and a `Q` ring whose bounding circle does not meet `P`'s cannot be inside
`P`, so it is skipped. (The cascade tests every geometry against every plate, most of them far apart,
so this is where the `O(N_P)` low-speed tests against the subject would otherwise go.)

As built (step 11): the whole-pair test only, in the partitioner before any overlay is made
(`is_outside_by_bounds` in `PolygonPartitioner.cc`), and only where the partitioning polygon's interior
lies within its own circle, and the subject's too when it is a polygon (a polyline's outline is all
there is): `PolygonOnSphere::is_outside_bounding_small_circle_outside_polygon`, which a polygon wrapping
round the globe so far that its interior reaches past its outline's circle fails
(`InteriorBeyondBoundingCircle` tests the helix band of `PointInPolygonTest` both ways). The per-ring
skip is not built: a polygon's rings have no circles of their own.

This fixes doc 5.1's three no-intersection bugs, each a regression test in Phase 5 (section 11):

| Case | Today | Overlay |
| --- | --- | --- |
| `Q` wholly inside `P` | `P`'s first vertex is outside `Q`: all of `P` outside | `Q`'s ring is inside `P`: inside is `Q`, outside is `P` with `Q` as a hole |
| `P` crosses only a hole of `Q` | no record (holes excluded): one vertex decides | the hole's pieces cut `P`; `P`'s exterior ring, with no record, is classified against `Q` |
| `Q` crosses only a hole of `P` | as above | likewise with the roles swapped |

### 4.2 Grouping: exteriors, holes and their polygons

The rings of one operation's output do not cross each other (twists apart) and may touch at vertices.
Grouping is by **containment with parity depth**, not by the output side, because the parity reading
of the output (points inside an odd number of output rings) is what the oracle verified in every run
(PLAN.md Phase 4a: "the output read by parity over its rings"):

1. For each pair of output rings, decide whether ring `r` lies inside ring `r'`: take the first vertex
   of `r` farther than `s` from `r'`'s outline (so the exact test has a margin) and test it against a
   polygon of `r'` alone, threshold off. If there is none, test the farthest of `r`'s vertices and its
   edges' midpoints instead (step 8 found both: a neck ring thinner than `s` along another ring, in
   the `six` hunt; a triangle of three shared touch vertices, in the `chain` hunt). Only if every one
   of those is on `r'` do the rings coincide, and neither contains the other (counted). Every pair is
   tested; the bounding-circle pre-check was not needed.
2. Each ring's parent is the containing ring of greatest depth; depth counts containing rings.
3. Even depth is an exterior, odd depth a hole, and a hole belongs to its parent (which has even
   depth). An island inside a hole is an exterior of its own.

`O(R_out^2)` containment tests, each a distance scan of one ring's vertices against the other and a
point test; `R_out` is small. This agrees with walk-through section 11 ("the smallest exterior that
contains it").

### 4.3 The output side, and the hemisphere check

**As built (step 8, 2026-11-04)**, replacing the per-ring output side below, which step 8 rejected
(section 13):

- **Orientation** from the grouping alone: an exterior (even depth) is made anticlockwise and a hole
  clockwise, by `PolygonOrientation::calculate_polygon_orientation` on a polygon of the ring (it
  projects the ring onto a tangent plane where it can, and falls back to the spherical signed area).
  Not `SphericalArea`'s signed area, which is zero for a ring the thickness across: every edge of it
  is zero length to `GreatCircleArc` (the `tinyhole` hunt's holes, 0.3h to 1.5h across).
- **The hemisphere check** is the oracle at one point: a reference point farther than `10h` from both
  inputs' outlines (the first of the inputs' bounding-circle centres, their antipodes and the six
  axis points that is), read by parity over the output rings, must be in the output exactly when
  `f(in P, in Q)`. Why one point suffices (review 3's statement): given rings along the right edges,
  the parity over their smaller sides is everywhere either the output or its complement, since each
  ring whose output side is its larger side complements the reading (its smaller side differs from
  its output side by the whole sphere), and an even number of them cancel. So the check neither
  misses nor misfires on a correct output, touching rings and crossing inputs included. What it
  detects is an odd number of rings read by the wrong side: an output exceeding a hemisphere (a
  union or a symmetric difference), or wrong classes (the only cause for the intersection and the
  differences, which lie inside an input). It cannot tell the two apart, and its message says so. A
  failure throws `InconsistentOverlayException` for every operation, unless
  `Options::throw_if_inconsistent` is false (then counted, `hemisphere_check_failures`, and the rings
  returned as read, as every other failure is). No point clear of both outlines (never seen) is
  counted too (`hemisphere_check_skipped`).

Measured: all 774 committed and 6291 uncommitted prototype cases pass the oracle read polygon by
polygon, with every ring oriented, where no input ring crosses another (the hunt's generators move
vertices onto the other polygon and place holes near their ring's vertex centroid, so 2642 of the
4192 large-set cases have crossing input rings, where only the hemisphere check is asserted); no
hemisphere check fails anywhere.

The rejected design, for the record. Each output ring also knows **which side of it the output is
on**, from the keep-rule and the inputs, without any rotational order (so it is reliable at snapped
four-end vertices where the order is not):

- For each input polygon, once per call, the containment forest of its own rings (4.2's method, on
  the input rings): the side of ring `r` that is inside its polygon is `r`'s interior side as
  `PointInPolygon::InteriorReference` defines it (its smaller side, doc 3.1) iff `r`'s depth among its
  polygon's rings is even. Non-crossing rings assumed; with crossing input rings the check below may
  misfire, which is acceptable because it is only an assertion for partitioning (next paragraph).
- For a kept `P` piece of ring `r` with class `c`, the output is on `P`'s interior side iff `f(1, c)`;
  for a kept `Q` piece of ring `r` with class `c`, on `Q`'s interior side iff `f(c, 1)`; for a kept
  chain, on `P`'s interior side iff `f(1, 1)` (same side) or `f(1, 0)` (opposite sides). Whether the
  polygon's interior side of `r` is `r`'s left is the sign of `r`'s signed area
  (`SphericalArea::calculate_polygon_exterior_ring_signed_area`, `..._interior_ring_signed_area`,
  `src/maths/SphericalArea.h:91`, `:110`; positive iff the interior is to the ring's left, doc 3.1),
  combined with the depth parity above; and whether the walk traversed the piece forwards or
  reversed says whether that is the output ring's left. One kept piece per ring suffices; all its
  pieces must agree (a debug assertion).
- Whether the output ring's own interior (its smaller side) is on its left is the sign of *its*
  signed area, computed on the cleaned ring (`calculate_polygon_signed_area` on a polygon of the ring,
  `SphericalArea.h:71`).

**The check**: an exterior must have the output on its interior side and a hole on its exterior side.
For `INTERSECTION`, `DIFFERENCE` and `REVERSE_DIFFERENCE` this holds whenever the inputs' rings
enclose less than a hemisphere (walk-through section 8), so a violation is a bug or a crossing-ring
input: a debug assertion, and in release a counted diagnostic with the output returned as read. For
`UNION` and `SYMMETRIC_DIFFERENCE` a violation means the result exceeds a hemisphere and would be read
as its complement: `extract()` throws (section 5.2). The known limit is a ring within about `1e-8`
steradians of halving the sphere, whose interior is its left side by the tie rule (doc 3.1); the
check inherits it.

**Orientation** (Q6, decided: the output on the left; section 14): every output ring with the output
on its left (exteriors anticlockwise seen from outside the sphere, holes clockwise), by reversing
rings as needed. The interior rule does not care; GeoJSON asks for this and Shapefile for the
reverse. The alternative, keeping each ring's walk direction, is arbitrary (a `Q` piece runs either
way relative to the `P` pieces it joins, 6.3). A ring that crosses itself (where an input ring
does, 4.4) cannot have the output on its left all round.

### 4.4 Pinched rings

The split of 3.11 runs before grouping, so no output ring passes a vertex twice; a hole may touch its
exterior at a vertex (walk-through 6.4's two rings), which OGC simple features allow (Q7 of the
requirements, proposed OGC style). The exceptions inherited from the input are the twist of `P`
(section 8 of the walk-through) and the twist on `Q` (7.8); the output crosses itself where the input
did.

### 4.5 The optional sliver filter

Decided: an optional output filter, whether to enable it and its width decided in Phase 5 by
measuring thin rings when partitioning real data by a real topology model.

- **Where**: in `extract()`, after grouping and before `PolygonOnSphere::create`, on the cleaned rings
  of each output polygon; controlled by `Options::d_sliver_min_width` (radians; none means off), set
  from `PolygonPartitioner::create` and, in the cascade, from the `GeometryCookieCutter` constructor.
  Not exposed in pyGPlates until Phase 5 decides the default.
- **What**: a ring's mean width is `2 * area / perimeter` (`SphericalArea::calculate_polygon_..._area`
  for the ring's area; the sum of its arcs' `arc_length()` for the perimeter, `GreatCircleArc.h:187`).
  An exterior ring below the width is dropped with its holes; a hole below it is dropped alone (the
  polygon fills it). Counted in `Diagnostics`.
- **What it does to the invariants**. Without the filter the inside and outside outputs partition
  `P` exactly (every piece is kept by exactly one of intersection and difference, section 5.1). With
  it, a dropped sliver belongs to neither output when it is a separate ring in one output and part of
  a larger ring's boundary in the other (the usual case: a sliver of `P ∩ Q` bounded by a `P` piece
  and a `Q` piece is, in `P \ Q`, the `Q` piece joined into a larger ring), and moves to the other
  output when it is a hole there that is also dropped. So the area sum of the oracle (section 11.3)
  holds only with the filter off, and the result contract (5.3) is evaluated after filtering: an
  output emptied by the filter makes the geometry wholly the other class.
- **What it is not**: it is a distance rule on the output and nothing else; it does not merge, snap
  or move anything, and the overlay's own decisions never see it (run 3 section 7: a region thinner
  than `h` between two boundaries is the true answer, not an artefact).

As built (step 14):

- `Options::sliver_min_width` (`boost::optional<double>`, radians), set by `PolygonPartitioner::create`'s
  third argument and by the last argument of each `GeometryCookieCutter` constructor, which passes it to
  every partitioning polygon's partitioner. Off by default, and not in pyGPlates.
- **The measure is `PolygonOverlay::get_mean_width`, not `SphericalArea`.** `SphericalArea`'s fan
  drops the triangle of every edge that `GreatCircleArc` takes for zero length (under about `1e-6`
  radians), and a cut makes such edges next to a vertex. At a width of `h` each one is an error of up
  to about a quarter of a sliver's area, and a disc drawn with edges under `h` has no area at all. The
  function sums Van Oosterom and Strackee's triangles from the vertices' mean direction, modulo
  `4 pi`, and takes the smaller side, so the ring's direction doesn't matter; the perimeter is by
  `atan2`. The real-data harness (11.4) measures with the same function.
- Counted: `sliver_exteriors_dropped`, `sliver_holes_dropped` (alone) and `sliver_area_dropped` (the
  dropped exteriors' areas).
- **A rule this section left open**: when the filter empties both outputs (a subject thinner than the
  width, cut across), the subject is whole on the side that lost the larger area. With no filter the
  tie keeps the old rule, outside.
- **Decided (developer, 2026-11-07, from the step 14 measurements, 11.4 and PLAN.md's Status entry of
  that date): on at `10 h`** (about 90 m) in the cascade. That is `GeometryCookieCutter`'s default
  (`get_default_sliver_min_width`), so it covers Assign Plate IDs and pyGPlates' `PlatePartitioner`
  and `partition_into_plates`. pyGPlates exposes it as `sliver_width_threshold_radians`: `None` (the
  default) gives the default width, and zero turns the filter off. `PolygonPartitioner` keeps none
  (off) as its default, so `PolygonOnSphere.partition` and the raster meshes stay exact;
  `PolygonOnSphere.partition` takes `sliver_width_threshold_radians` too, defaulting to zero.
  - Why `10 h`: below `h` the overlay can't tell the two outlines apart. Up to `10 h`, the measured
    slivers are artefacts of an outline digitised along a plate boundary, below the resolution of
    global data, and they would take the neighbouring plate's ID. At `100 h` the filter starts on
    pieces that may be real small geography.

## 5. Boolean operations

### 5.1 The keep-rule

One classification serves every operation (walk-through section 14). `f(in P, in Q)` per operation,
and what `kept` (3.10) then keeps:

| Operation | `f` | `P` in `Q` | `P` out | `Q` in `P` | `Q` out | chain same | chain opposite |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `INTERSECTION` (partitioning's inside) | `p and q` | kept | | kept | | kept | |
| `DIFFERENCE` (partitioning's outside, `P \ Q`) | `p and not q` | | kept | kept | | | kept |
| `REVERSE_DIFFERENCE` (`Q \ P`) | `q and not p` | kept | | | kept | | kept |
| `UNION` | `p or q` | | kept | | kept | kept | |
| `SYMMETRIC_DIFFERENCE` | `p != q` | kept | kept | kept | kept | | |

Linking, splitting, cleaning and grouping use only `f` (the output membership of each sector), so
they are the same code for every operation. Partitioning calls `extract(INTERSECTION)` and
`extract(DIFFERENCE)` on one `Overlay`; a `Q` piece inside `P` is kept by both, walked in opposite
senses relative to the `P` pieces it joins (6.3), which the direction-free linking does not notice.

### 5.2 The hemisphere failure

`INTERSECTION`, `DIFFERENCE` and `REVERSE_DIFFERENCE` lie inside an input, so their rings enclose less
than a hemisphere whenever the inputs' do (walk-through section 8). `UNION` and `SYMMETRIC_DIFFERENCE`
can exceed one, and a ring cannot mean more than a hemisphere (doc 3.1; the explicit side is deferred
in doc section 7). Detection is 4.3's check: the output read by parity at a reference point clear of
both inputs is not `f` there (as built; the per-ring output side first designed here was rejected). `extract()` then throws
`InconsistentOverlayException` (section 9) with a message naming the operation and the ring; it never
returns the complement. PR C exposes the methods and the error in pyGPlates; PR B's overlay supports
all five operations and tests them (section 11), so PR C adds no core code.

### 5.3 The result contract for partitioning (survey Q4)

`RESULT_INSIDE` iff the difference output is empty; `RESULT_OUTSIDE` iff the intersection output is
empty; else `RESULT_INTERSECTING`. "Empty" is after clean-up (and the sliver filter if on): an output
whose only rings were collapsed regions is empty. So a geometry that only touches the partitioner,
from either side, or runs along it, or is identical to it, comes back whole with `INSIDE` or `OUTSIDE`;
`INTERSECTING` with pieces only when both outputs have a ring. This is what the record-free case
already does (section 4.1), so the contract is uniform. The partition layer (8.1) returns the original
geometry object itself in the whole case, as today (`PolygonPartitioner.cc:154-167`, `:430-448`), with
an identity provenance table. As built: a table only when one is asked for, and then the input's own
table when it has one (a piece of an earlier cut, 7.2), else the identity. **The same result with or
without lists** (review 5 1.1, decided by the developer): the result-only path of 10.2 finds this
result sooner where it can, never a different one, and an inconsistent overlay raises either way.

## 6. Polylines and open arcs

### 6.1 A polyline subject

`PolylineOverlay` (`overlay_polyline.py:68-266`) through the same core, walk-through section 12 with
run 3's two refinements (6.1 of the run report). Steps 1 to 6 are shared: `intersect(graph, polyline,
polygon)`; vertices by identity (the polyline's last vertex is named through the fictitious segment,
2.2); stops with `DANGLING_END` at an end that is no record (3.3); the loop and spike rules (a piece
with a dangling end is skipped); common pairs (a polyline piece along a `Q` edge pairs with the `Q`
single arc between the same vertices). `Q`'s pieces are never classified: they bound nothing here.

Classification of the polyline's pieces (`PolylineOverlay.classify`, `overlay_polyline.py:178-234`):

1. A common piece gets the Q2 class: **inside** (decided), `METHOD_ALONG`. `Options::d_polyline_along_is_inside`
   lets a caller that wants the two outputs to partition the polyline exactly use `outside` for the
   difference pass; the partition layer leaves it `inside` (the piece goes to the inside list only).
2. A piece with a test point is tested there, threshold off: its interior original vertices, and a
   dangling end of the polyline (an end point that is no record is an original vertex more than `h`
   from `Q`). Exact with margin.
3. Deduction only across a vertex that is **one `LL` record** with exactly two non-internal polyline
   pieces there, neither common: the two classes differ. Never across a touch (a bounce is possible),
   never from an along-piece (so Q2 cannot leak: measured 0 cases in 1199, run 3 section 6.2).
4. Stalled pieces tested at their midpoint in order of margin, as 3.7(c).

There is no sector walk, no linking and no repair. The one check: two *tested* pieces at an `LL`
vertex with equal classes (`ll_disagreements`), which means a missed intersection; counted, reported in
debug builds, and the tested classes kept (both have margin).

Output (`output`, `overlay_polyline.py:238-266`): an internal piece (a loop or spike inside an
identity vertex) takes the class of the piece before it (or after it, at the start) so the outputs
cover the polyline without a gap; consecutive pieces of one class are joined into one polyline, with
a repeated position at the join dropped. **As built (step 9)**, each point is its stop's own position
(a record's position on the polyline, or the polyline's vertex), not its vertex's output position as a
polygon's rings use: an identity vertex can extend far along a dense run of touches (the `along`
generator's), so its output position can be far off the polyline, which the oracle found (78 of 300
samples uncovered in one case); a polygon's rings must share a vertex's position, a polyline's
adjacent runs only the stop between them. A polyline whose end point is a record (236 of 300 `ends`
cases) therefore starts its output at the end point itself, so run 3's open question 5 has no choice
to make. `extract_polylines` returns each run with its class and
provenance; the partition layer (8.1) builds `PolylineOnSphere`s (`PolylineOnSphere::create` needs two
points; a run has at least two distinct stops or is a whole-polyline piece).

Result contract: `RESULT_INSIDE` iff no run is outside, `RESULT_OUTSIDE` iff none is inside.

Measured (run 3 section 6.2): no failure in 3395 cases; a misclassified point never farther than
`1.09h` from `Q`'s boundary.

As built (step 9, `Overlay::classify_polyline_pieces`, `extract_polylines`): the polygon core's rules
apply to a polyline subject unchanged, the spike rule restricted (3.4) and the self-touch cuts (3.5)
included, and the prototype's polyline overlay now runs them too (`r5c`). The comparison test covers
polylines (`prototype-polyline.txt`: the four generators and an opened ring of each hunt generator):
pieces, classes and runs agree with the prototype in all 189 committed and 1899 uncommitted cases, and
the oracle along the polyline passes (sampled points beyond `M` lie within `2h` of a run of their
class; an opened ring can pass a point twice, then as many runs may cover it).

### 6.2 CitcomS

`GenerateVelocityDomainCitcoms.cc:272-277` partitions two two-point polylines with
`PolylineIntersections::partition` and takes the first intersection's point. It becomes
`GeometryIntersect::intersect(graph, *line_1, *line_2)` and `graph.unordered_intersections[0].position`
(`GeometryIntersect.h:159`). Same semantics (the first record found). `GenerateVelocityDomainCitcomsTest.citcoms_meshes`
(`src/unit-test/GenerateVelocityDomainCitcomsTest.cc:172`) compares generated meshes with stored
files; done in step 9, and verified to cover the change (it fails when the intersection's point is
replaced by a line's end). `PolylineIntersections` keeps one caller, `PolygonPartitioner`, until the
partition layer moves onto the overlay (step 11), and is deleted there, with
`src/maths/CMakeLists.txt:95-96` (the pyGPlates source closure test will say if a list is stale).

### 6.3 The three hooks for an open clip arc (Q7)

Not built in PR B; the structures are shaped so that the `DateLineWrapper` port is a layer (run 2
section 10, the three additions in `overlay_cut.py`):

1. **Pieces of an open geometry** (`_build_arc_pieces`, `overlay_cut.py:105`): a clip whose first and
   last pieces dangle. `RingView::d_closed == false` and `Stop::DANGLING_END` already exist for the
   polyline subject; the clip side only needs the same builder.
2. **Classifying the arc's pieces without classes for `P`'s pieces** (`Cut.classify`, `:203-250`): a
   piece with an original arc vertex tested there, a dangling piece outside `P` by construction, the
   rest by parity along the arc, flipping at each vertex where an odd number of `P`'s non-common rays
   leave on the positive side of the arc segment's thick plane (`_ray_side`, `:189-201`, the same thick
   decision `GeometryIntersect` made). A second classifier over the same `Piece`/`Vertex`/`Ray`
   structures; `Ray` needs nothing new.
3. **Linking by faces, pieces walked in both directions** (`faces`, `:254-379`): an arc ray inside
   `P` carries two nodes, one per side. The pairing map of 3.10 is therefore keyed by a
   `Node { ray, side }`, with `side` always `0` for polygon overlays, rather than by a ray; that is the
   one structural choice PR B makes for the port, and it costs nothing.

## 7. Provenance

### 7.1 Per output point

Every output point carries a `Provenance` (2.8), filled from the stop it came from:

| Output point | `d_subject` | `d_clip` |
| --- | --- | --- |
| an original `P` vertex (an `ORIGINAL_VERTEX` stop, or a vertex holding a `P` vertex) | vertex (ring, index) | the `Q` location if the vertex also holds a `Q` vertex (`SS`) or lies on a `Q` segment (`SL`: ring, segment, ratio by projection) |
| a record inside a `P` segment (`LL`, or `LS`: `Q`'s vertex on `P`'s segment) | segment (ring, index, ratio) | `LL`: `Q` segment and ratio; `LS`: the `Q` vertex |
| a point of a `Q` piece inside `P` (a cut), whether a `Q` vertex or a record on a `Q` segment | none | the `Q` location |

A vertex is output at one position (2.5), so its provenance is that position's: a vertex holding a `P`
vertex is that vertex exactly, whatever else it holds. The provenance of the point says where the
*position* is; the clip location is extra information for callers (and for the cut interpolation,
which only needs to know the point has no subject value).

As built (step 10, `Overlay::get_vertex_provenance`, `Options::want_provenance`; `Location`,
`Provenance`, `provenance_seq_type` as in 2.8, without the `d_` prefixes): at a vertex holding no `P`
vertex the subject location comes from the **walked** subject piece (the one the output ring arrives
or leaves along), its stop's segment and the ratio of the output position's projection onto it; if
neither walked piece is a subject piece the point is a cut (no subject location), even where the
vertex lies on `P`. The walked pieces are those of the ring **after the split** (3.11): each vertex
entry of the walk keeps the stop it leaves from and the leaving piece's far stop, and an entry
arrives at the previous vertex entry's far stop, cyclically, so each half of a split ring has its
own (review 4 found the provenance filled before the split, wrong at 611 of 4192 large-set cases).
The clip location is the vertex's `Q` vertex, else a walked clip piece's stop, else the clip segment
of the vertex's first record, by projection: every vertex lies on `Q`. A polyline's output is at
its stops (6.1), so its points' locations are the stops': the polyline's vertex, or its segment at
the stop's ratio; and the clip location is the stop's own record's (its `Q` vertex, or its `Q`
segment by projection), not its identity vertex's, which along a dense run of touches can name a `Q`
vertex up to `12h` away (review 4). Checked on every committed and generated case: each point's
provenance resolves to it (a subject vertex exactly; a location by ratio within `2h`, measured
within `1.0h`), and a point beside an output edge clearly along the subject's outline (its midpoint
within `h` of it and farther than `2h` from the clip's) has a subject location.

### 7.2 Composition through the cascade

`GeometryCookieCutter` feeds plate `N`'s outside pieces to plate `N + 1`
(`GeometryCookieCutter.cc:405-417`), so the second overlay's subject is a piece whose points already
have provenance relative to the original. `Options::d_subject_provenance` is that table (one entry per
point of the subject, in `GeometryUtils::get_geometry_points` order, `src/app-logic/GeometryUtils.h:121`),
and the overlay composes as it fills each output point:

- a subject vertex `(r, j)` of the piece: `table[(r, j)]`, unchanged;
- a subject segment `(r, j, t)` of the piece, between the piece's points `a = (r, j)` and `b = (r, j + 1 mod n)`:
  every edge of a piece is a sub-arc of one edge of its own input (an original `P` edge, or a clip
  edge), by induction through the cascade, so `table[a]` and `table[b]` lie on one location: both on
  original segment `S` (one may be a vertex of `S`, ratio `0` or `1`) gives `S` with ratio
  `t_a + t (t_b - t_a)`; both on one earlier clip's segment gives that clip location likewise, keeping
  its `d_clip_index`; `d_subject` is none in that case. Ratios compose because a sub-arc of a great
  circle arc is parametrised by angle along the same great circle. Not quite exactly: a piece's edge
  ends at its vertices' output positions (2.5), which for a clip edge can be a subject vertex up to the
  identity group's extent off the clip's segment, so a clip location composed this way is a
  projection, within that extent (session review);
- a cut point of this overlay: `d_subject` none, `d_clip` this clip's location, `d_clip_index` this
  plate's index in the cascade.

If `table[a]` and `table[b]` do not share a location (which the induction says cannot happen), the
point gets `d_subject` none and the mismatch is counted: a diagnostic, never a wrong value.

As built (step 10, `Overlay::compose_subject_location`, `Options::subject_provenance`):

- **The induction fails**, measured: where an identity vertex holds two original vertices of one input
  (dense shared edges), a piece's edge spans the group, from a vertex before it to one after it, so
  its ends lie on no one segment. Such points get no subject location and are counted
  (`provenance_subject_mismatches`): 44 of 22670 second-step points in the large generated set, 124 of
  3646 in the chain set, none in the `holes` set, every one in a case with such a vertex. A coverage
  then interpolates them along their run (7.3 step 2), as cut points. **A second cause** (review 4):
  a point at an input's corner within `h` of both its segments but naming one holds one location, so
  the edge along the other cannot compose; against a clip moved off the first (a second step that
  cuts the pieces anew) it loses a subject location in pieces with no such vertex too. A mismatch
  whose two ends were both cut points loses nothing (the point's clip location is this clip's) and
  is counted apart (`provenance_clip_mismatches`).
- **The wrap-around needs the ring's size**: whether vertex 0 ends a ring's last segment cannot be told
  from the table, and guessing turned a span across an identity group into a wrong segment (0.03 rad
  off, the `six` generator). The original geometries give it (step 11, the developer's decision on
  review 4's suggestion): `Options::original_subject` (the geometry the cascade started from) and
  `Options::cascade_clips` (the cascade's partitioning polygons, by clip index); without them no
  wrap-around is assumed, so such an edge is a mismatch. They also give the segment lengths a
  geometric fallback at an input's corner (the second cause above) would need, if the hunt shows one
  is wanted.
- **Only the subject's locations reach a caller.** A coverage takes its values from the subject
  location alone (7.3), and every output point at an overlay vertex lies on this overlay's clip, so
  its clip location is this clip's; an untouched piece vertex keeps its table entry by lookup. An
  earlier clip's composed location is therefore never output: it only tells an edge along an earlier
  cut whose two ends both lie on the subject's outline, on different segments, from a lost subject
  location, so that `provenance_subject_mismatches` counts only real losses.
- **Subject first, then the earlier clip**: an edge whose ends share a subject segment is along the
  subject; else, if they share an earlier clip's segment, it is a cut edge of that clip (an edge
  from the subject's outline to a cut point is along the earlier clip though one end is on the
  subject too).
- **A vertex's clip location is always this clip's** (every vertex lies on this clip), set before
  composing, so that the table's earlier clip location does not replace it.
- **Checked**: on every committed and generated polygon case whose rings do not cross, each piece of
  the difference, with its table, is overlaid with the clip again (the pieces run along it, the
  hardest case), and with the clip moved by 0.37 of its bounding radius (cutting the pieces anew;
  its locations by clip index 1), and every point of both outputs must resolve against the original
  subject, within `3h` (measured within `1.63h`); and a hand two-plate cascade, polygon and polyline
  (`ProvenanceCascade`, `ProvenancePolyline`), and a split at a vertex holding no subject vertex
  (`ProvenanceAtSplit`).

The piece returned to the cascade carries its composed table, which becomes the next step's
`d_subject_provenance`. A geometry entering the cascade has the identity table. The cookie cutter
builds tables only when asked (`PartitionFeatureUtils`, and pyGPlates' coverage form of
`partition_geometry`, 8.4); otherwise `d_want_provenance` is false and nothing is composed.

As built (step 11): `GeometryCookieCutter::partition_geometry(geometry, inside, outside,
want_provenance)`, one geometry only. `partition_geometries` (several at once, which pyGPlates'
`partition_features` uses to measure overlap) offers none: its pieces are mixed in the lists, and a
table does not say which geometry it refers to, so offering it there would need a subject index in
every piece. A geometry entering the cascade has no table (it is the original); the first partitioner
that returns it whole gives it the identity table (`PolygonPartitioner::create_identity_provenance`),
and later overlays compose through that. The tables' clip index is the partitioning polygon's position
in the cookie cutter's order (after sorting).

### 7.3 Scalar coverages from provenance

This replaces `partition_range` (`PartitionFeatureUtils.cc:429-590`): the epsilon map of original
points (`:180-194`, `:458-470`) and the nearest-segment search within 0.5 degrees (`:473-514`, `:593-594`),
and with them the read past the end at `:522-523` and `:539` (a polygon's closing segment, index
`N - 1`, interpolates to index `N`), which the survey found.

The domain after the G1-1/G1-4 PR (branch `fix/coverage-interior-rings`, worktree-2 at `c1f2f3c7b`,
not yet merged): a polygon coverage covers all its rings in `get_geometry_points` order, and an
old-file coverage with scalars for the exterior ring alone is read as `Coverage::exterior_ring_only`
with `Coverage::domain` the exterior ring alone (`ScalarCoverageFeatureProperties.h` on that branch;
`Range(domain, range, exterior_ring_only)` in its `PartitionFeatureUtils.cc`). PR B merges `main`
once that lands (PLAN.md 2026-10-31) and then rewrites `partition_range` as:

1. For each point of a piece (all rings, or the exterior ring alone when `exterior_ring_only`,
   matching how the partitioned coverage will be read), its value:
   - `d_subject` a vertex with a value: that value (a hole vertex has none under `exterior_ring_only`);
   - `d_subject` a segment `(r, j, t)` whose ends both have values: `(1 - t) v[j] + t v[j + 1 mod n]`,
     with `j + 1 mod n` the ring's wrap-around (the closing segment interpolates to the ring's first
     vertex, which is the fix);
   - otherwise **no value yet**: a cut point (`d_subject` none), or a point on an input hole under
     `exterior_ring_only`.
2. **Q1**: each maximal run of consecutive valueless points along an output ring (cyclic) or output
   polyline is bounded by two valued points `A` and `B`; each point of the run takes
   `v_A + (s / s_AB) (v_B - v_A)`, with `s` its cumulative arc length from `A` along the run and
   `s_AB` the run's total length (`GreatCircleArc::arc_length()` of the output ring's arcs). A cut's
   two ends are intersection points whose own values were interpolated along `P`'s segments in step 1.
   This is the decided answer for cut points; for hole points under `exterior_ring_only` it is the same
   rule applied to the only fallback that exists for them (the survey said hole points have no good
   answer; this one needs no extra code).
3. **A ring with no valued point at all** (an output ring made wholly of `Q` points: a `Q` ring inside
   `P`, which is `P ∩ Q` when `Q` is inside `P`, or a hole of the intersection) has no `A` and `B`.
   Open question 2 of section 14; the recommendation is the value at the nearest point of the
   input's outline, which is today's fallback (`:493-543`) kept only for this case, so that no
   coverage is dropped.

Per-tuple scalars (`range_tuple_size`, `:445`) are interpolated independently, as today. A point or
multi-point domain is unchanged (no new points, `:474-476`).

As built (step 12, `PartitionFeatureUtils.cc`'s `partition_range` and `Range`):

- The cookie cutter is asked for provenance only for a geometry property with a coverage
  (`partition_geometry(..., want_provenance)`); the epsilon map of the original points and the
  nearest-segment search within 0.5 degrees are gone. `Range` keeps the coverage's domain points and
  its rings' sizes and offsets in `get_geometry_points` order (the exterior ring alone under
  `exterior_ring_only`, so that a hole's location finds no scalars there).
- Step 1 as designed; a point or a multi-point's point takes its own index's scalars.
- Step 2 per output ring, cyclic (a polyline's points all lie on the input, so all have values; a run
  at a polyline's end would take its one neighbour's, none seen). Arc lengths between consecutive
  points by `atan2` of their cross and dot products. The walk starts at a ring's first valued point
  and goes once round, so a run wrapping past the ring's start is interpolated like any other.
  **Review 6 1.1**: the polyline's end-run loops ran for closed rings too, overwriting the points
  before the first valued one with its value; a ring starts at a point with no value whenever a piece
  is cut a second time (the second overlay's first stop is on the first plate's cut edge), so every
  feature spanning three plates was affected. Fixed, and `RunAcrossRingStart` checks every run of
  every ring against an independent arc-length interpolation, in both plate orders.
- Step 3 by `minimum_distance` from each point to the coverage's domain, with no threshold (the
  plate inside the input can be anywhere in it), the ratio along the nearest segment that of the
  point's **projection** onto its great circle, clamped to the segment (the old fallback used the
  distance to the point itself, which is not the ratio of any point on the segment), and the ring's
  last segment wrapping to its first vertex.
- Tests (`PartitionFeatureUtilsTest`, through `partition_feature` with a cookie cutter of features):
  a polygon with a hole cut by a plate whose corner is inside it (original vertices exact, the hole's
  too; a crossing on the ring's closing segment by its segment; the plate's corner, a cut point, by
  arc length between its neighbours), an exterior-ring-only coverage whose hole the cut opens into
  exterior rings (each piece's coverage its exterior ring's, the hole's points interpolated), and a
  plate wholly inside the input (each point the projected nearest outline value). Mutations:
  seven, each failing a committed test (the closing segment's wrap, arc-length interpolation along a cut, the nearest-outline value and its projection, the exterior-ring-only domain, the rings' offsets, area in the size metric); the exterior-ring-only mutation survived until a piece keeping its hole was added.

## 8. The consumers

### 8.1 `PolygonPartitioner` (the partition layer)

Keeps its interface (Q8) with these changes to `src/maths/PolygonPartitioner.h`:

| Today | After PR B | Why |
| --- | --- | --- |
| `partition_polygon` fills `partitioned_polyline_seq_type` lists (`:160-164`) and its comment says an intersecting polygon is returned as polylines (`:122-125`, `:145-158`) | fills `partitioned_polygon_seq_type` (`std::list<PolygonOnSphere::non_null_ptr_to_const_type>`); the whole polygon is appended when `INSIDE`/`OUTSIDE` (today the caller must, `:156-158`) | polygon output; one rule for whole and cut |
| `partition_geometry` (`:127-131`): `partitioned_geometry_seq_type` of `GeometryOnSphere` | unchanged signature; the lists now hold polygons for a polygon input; a new overload or extra arguments take an input `provenance_seq_type` and return one per output geometry | the cascade and coverages (section 7) |
| `partition_polyline` (`:138-142`) | unchanged signature; pieces by section 6; a polyline that only touches comes back whole | survey Q4 |
| `partition_point`, `partition_multipoint` (`:171-203`) | unchanged (threshold on, as today, `PolygonPartitioner.cc:506-515`) | decided: points unchanged |
| `create(polygon, speed)` (`:72-79`) | also takes `boost::optional<double> sliver_min_width = boost::none` | 4.5 |
| `d_partitioning_polygon_orientation` (`:207`) | removed with the bend test | the sector rule replaces it |
| private `partition_intersecting_geometry`, `is_partitioned_polyline_inside_partitioning_polygon`, `is_non_intersecting_polyline_or_polygon_fully_inside_partitioning_polygon` (`:218-235`) | removed; `PolygonOverlay::Overlay` does it | |

The `GeometryPartitioner` visitor (`PolygonPartitioner.cc:46-220`) stays as the dispatch; its polygon
branch (`:134-181`) appends polygons instead of copying polylines. The `InsidePartitionedPolylineMerger`
(`:228-314`), the bend test (`:366-383`) and the classification (`:685-897`) go.

For the geometry forms the layer returns:

- polygon input: `PolygonOnSphere::create(exterior, interior_rings)` per `OutputPolygon`;
- polyline input: `PolylineOnSphere::create` per run;
- the whole input object when the result is `INSIDE` or `OUTSIDE` (5.3), with the identity table as
  its provenance.

`Result` stays `GEOMETRY_INSIDE` / `GEOMETRY_OUTSIDE` / `GEOMETRY_INTERSECTING` (`:85-95`).

As built (step 11):

- `PartitionedGeometry` and its list are `PolygonPartitioner`'s (the cookie cutter's are typedefs of
  them). The provenance overload is `partition_geometry(const PartitionedGeometry &, inside, outside,
  const CascadeProvenance &)`: the input's own table travels in the `PartitionedGeometry`, and
  `CascadeProvenance` carries the rest (the original geometry, the cascade's partitioning polygons,
  this one's index). The plain `partition_geometry` keeps its signature and builds no tables.
- **A multi-point** wholly on one side is returned itself too (5.3 for every geometry type); before,
  a new multi-point of the same points. With provenance its points' locations are their indices in
  the input (ring 0), composed through the input's table.
- `partition_polygon` with neither list is the result-only path (10.2); with a list it is the full
  path, both outputs extracted so that 5.3's emptiness after clean-up decides the result.
- The bend test, `InsidePartitionedPolylineMerger` and `PolylineIntersections` are deleted;
  `PolygonOverlay` joined the pyGPlates sources (the partitioner includes it).
- **Trap**: Windows' `WinUser.h` defines `DIFFERENCE`, which broke `PolygonOverlay::Operation` in
  every translation unit that included `<windows.h>` before the overlay's header; the header
  undefines it (a comment there says why).

### 8.2 `GeometryCookieCutter` (the cascade)

`partition_geometries` (`GeometryCookieCutter.cc:340-455`) keeps its shape: the outside list of one
plate is the input of the next (`:405-417`), the inside pieces go to that plate's `Partition`
(`:393-394`, `:431-434`), and the early return when no lists are wanted stands (`:421-428`). Two
changes:

1. **Polygons through the cascade.** The outside pieces are polygons with holes, so plate `N + 1` cuts
   pieces whose edges lie along the boundary it shares with plate `N`: the common-chain case, run on
   157 cascade steps without failure (run 2 section 8, run 3 section 4.3), with no growth of identity
   groups from step to step.
2. **Provenance travels with each piece.** `partitioned_geometry_seq_type`'s element becomes
   `PartitionedGeometry { geometry; boost::shared_ptr<const provenance_seq_type> provenance; }` in
   both `PolygonPartitioner` and `GeometryCookieCutter` (`GeometryCookieCutter.h:64`, `:78`), the table
   null unless requested, and the cookie cutter passes a piece's table as the next step's subject
   table (7.2) and `d_clip_index` as the plate's position in `d_partitioning_geometries`. The four
   sites that read the lists change to `.geometry`: `PartitionFeatureUtils.cc:407-413`,
   `PyPlatePartitioner.cc:330-345`, `PyGeometriesOnSphere.cc:3698-3716`, and the cookie cutter itself.
   (The alternative, a parallel optional list of tables, keeps every signature but couples two lists
   by position; rejected for that.)

**Networks' rigid blocks** (`GeometryCookieCutter.cc:534-569`): each block is added as a partitioning
polygon (`:559-560`) and then the network's boundary with the blocks as interior rings (`:564-568`,
`ResolvedTriangulation::Network::get_boundary_polygon(true)`, `src/app-logic/ResolvedTriangulationNetwork.h:338-339`).
Today the holes are invisible to the cut (doc 5.1), so the order matters and sorting can move a block
after its network (`GeometryCookieCutter.h:99-103`, doc 5.1). With holes cut, the network never claims a
block's area, in either order. No code change; a behaviour change worth a line in the GPlates
changelog under Assign Plate IDs.

`partition_point` (`:458-485`) is unchanged.

As built (step 11): as above, with provenance only through `partition_geometry` (7.2 as built). The
three other sites read `.geometry` (`PartitionFeatureUtils.cc`, `PyPlatePartitioner.cc`,
`PyGeometriesOnSphere.cc`); none asks for provenance yet (step 12's coverages will). The exception
propagates from the cookie cutter (9.2); the products' handling of it is steps 12 and 13.

### 8.3 `PartitionFeatureUtils` (Assign Plate IDs)

- **Coverages**: `partition_range` rewritten from provenance (7.3). `Range` (`PartitionFeatureUtils.cc:197-251`)
  loses `domain_to_range_map` and keeps the domain points; `partition_geometry` (`:357-398`) asks the
  cookie cutter for provenance. Needs the G1-1/G1-4 merge first (7.3), which conflicts in this file
  (PLAN.md 2026-10-31).
- **Most overlapping** (Q3, decided: area for polygons, arc length for polylines, count for points).
  `GeometrySize` (`:618-668`) accumulates arc length for polygons (`:649-654`) and polylines, and
  point counts; it accumulates `PolygonOnSphere::get_area()` (`PolygonOnSphere.h:1595-1596`, cached)
  for polygons instead, and `GeometrySizeMetric` (`PartitionFeatureUtils.h:544`) compares
  `(area, arc_length, num_points)` lexicographically. That is a new rule in GPlates, not an
  extension: today's `operator<` compares arc length if either side has visited a line geometry and
  point counts otherwise (`PartitionFeatureUtils.cc:1480-1491`), while the Python one compares the
  tuple `(arc_length, num_points)` (`PlatePartitioning.py:369-372`); the two then agree. A feature
  with polygons and polylines is decided by its polygons.
- **Failure**: section 9.2.

As built (step 12): `GeometrySize` adds a polygon's area and not its arc length (its cuts would
inflate that), and `GeometrySizeMetric::operator<` compares area, then arc length, then the number of
points (`real_t`'s comparisons, so equal within its epsilon falls through to the next). Tested by a
thin piece with long edges against a fat one, a polygon against a longer polyline, and two
multi-points of different sizes. pyGPlates' `most_overlapping_plate` is step 13's. Review 6 noted that
an equality within a tolerance is not transitive, so the order is not strictly a weak ordering;
pre-existing (the old comparison did the same), consumed only by a running maximum, with
`real_t`'s epsilon `1e-12` on a unit sphere: left as it is.

### 8.4 pyGPlates

- **`PolygonOnSphere.partition`** (`src/api/PyGeometriesOnSphere.cc:3641-3719`, docstring `:4446-4518`):
  no code change beyond the list element type (8.2); the warning at `:4488-4494` goes, replaced by the
  statement that an intersecting polygon is returned as polygons with interior rings, that a geometry
  which only touches the polygon is returned whole as `inside` or `outside`, and a
  `.. versionchanged:: 1.1`. Docstring via the `api-docstring` skill; the stub regenerated.
- **`PlatePartitioner.partition_geometry`** (`src/api/PyPlatePartitioner.cc:251-345+`, docstring
  warning `:707-708`): warning removed likewise. **Coverages**: the method also accepts the coverage
  form the rest of the API uses, `(geometry, scalars)` (as `Feature.set_geometry` does, survey 4.2),
  and then returns each piece as `(piece, scalars)`, the scalars mapped by 7.3 through a shared
  app-logic function (`PartitionFeatureUtils::partition_coverage_range(provenance, range)`, exposed in
  its header). This is the one new pyGPlates API surface in PR B. **Superseded 2026-11-04 (section
  14, question 5):** no public coverage form in PR B; `partition_features` calls a private helper,
  and PR C decides any geometry-level form. As built (step 13b): the private helper is
  `PlatePartitioner._partition_feature_geometries(feature)` (`PyPlatePartitioner.cc`), which calls the
  same `PartitionFeatureUtils::partition_feature` as Assign Plate IDs and returns, per geometry
  property, its name, its range property's name and its pieces (a geometry, or a `(geometry,
  {ScalarType: values})` coverage); `partition_features` removes the range properties with the
  geometries and sets the pieces back. That code moved to `app-logic/PartitionFeatureGeometries.{h,cc}`
  (same namespace), since `PartitionFeatureUtils.h` includes `AssignPlateIds.h` and reconstructions,
  which pulled about 320 GPlates-only files (data mining among them) into the pyGPlates closure. **A
  piece of an exterior-ring-only coverage (an older file's) keeps that form** (the developer): it has
  scalars for its exterior ring only, which `Feature.set_geometry` refuses (since 1.1 it needs one per
  point of every ring), so such a property's pieces are set alone and each range is added as a
  `GmlDataBlock` property, as Assign Plate IDs writes them, after checking the count is the exterior
  ring's. A hole the cut opens into an exterior ring gets values (7.3 step 2) and a hole left whole
  gets none: inherent in that form.
- **`partition_features`** (`src/qt-resources/python/api/PlatePartitioning.py:86-407`): in the
  `split_into_plates` branch (`:285-340`) each geometry property is read with its coverage when it has
  one (`coverage_return=CoverageReturn.geometry_and_scalars`, as the survey used it; the exact call is
  for Phase 5), partitioned in the coverage form, and set back with `set_geometry((piece, scalars),
  name)` (`:326`, `_plate_partitioning_set_geometries`), so coverages are carried over as Assign Plate
  IDs does (survey 4.3). `partition_into_plates` (`:416-`) goes through `partition_features` and
  needs nothing of its own.
  - As built (review 7 1.2, then the `src/api` audit's G8-1, the developer, 2026-11-07): **which
    properties are partitioned is decided once, in the shared visitor** (`PartitionFeatureGeometries.cc`).
    The private helper returns the positions of the properties it partitioned, and `partition_features`
    removes exactly those, as Assign Plate IDs removes the iterators `partition_feature` returns. So
    the two products can't disagree on which geometry is partitioned and which kept.
  - **A time-dependent geometry** is partitioned only if it is a `GpmlPiecewiseAggregation` with a single
    time window that contains the reconstruction time. It is then partitioned as a constant geometry.
    Its property clone is a constant property of the same name, so a cut piece and, by "most
    overlapping plate", the whole geometry are both written back constant. Any other time-dependent
    geometry is not partitioned and stays unchanged; a `GpmlIrregularSampling` is never partitioned.
  - **A feature with nothing partitioned** (no geometry, or only geometry that isn't partitioned) is
    returned unpartitioned, whole. GPlates already left such a feature as it was.
- **`most_overlapping_plate`** (`:342-392`): the measure at `:357-374` becomes
  `(area, arc_length, num_points)`: `geometry.get_area()` for a `PolygonOnSphere`, `get_arc_length()`
  for a polyline, `len(get_points())` for the rest. Same rule as 8.3.
- **Tests that must change**: `pygplates/test/test_maths/test_geometries_on_sphere.py:1849-1868`
  (two inside polylines and one outside become one inside polygon and one outside polygon; the
  "will be fixed in future" comment goes); `pygplates/test/test_app_logic/test.py:2131-2139` (the
  intersecting polygon: 3 inside and 2 outside polygons, replacing "3 polylines outside (instead of
  2)"). New tests: section 11.5. **Done in step 11** (the suite must stay green there): as predicted,
  with the pieces' areas checked against the input's in the first; the second's input crosses itself
  (its closing edge crosses its `-30` meridian edge near `16` degrees north), so its pieces are its two
  lobes' parts and their areas sum to its lobes' (`0.185`), not its signed area (`0.038`). Tests
  only: the docstrings' warnings (`PyGeometriesOnSphere.cc` and `PyPlatePartitioner.cc`, "partitioned
  polylines (not polygons)") and the `versionchanged` are step 13's, with `PartitionError` (review 5
  1.2).
- **Stub**: `python pygplates/stub/generate_stub.py --output ... --check ...` after the docstring and
  the coverage overload (its `:rtype:` changes).

### 8.5 `GLReconstructedStaticPolygonMeshes` (result only)

`find_present_day_polygon_mesh_node_intersections` (`src/opengl/GLReconstructedStaticPolygonMeshes.cc:949-989`)
calls `partition_polygon(node_polygon)` with no lists (`:964-965`) once per quad-tree node, then works
round the first no-intersection bug itself (`:966-983`). It calls the result-only path (10.2) through
the same `partition_polygon` without lists, which now gives `INTERSECTING` when the node polygon
surrounds the mesh polygon (a `Q` ring inside `P`, 4.1), so the extra test at `:971-976` is redundant.
Recommend keeping it in PR B (harmless, and there is no rendering test coverage) and removing it in
the Phase 6 visual check if the developer prefers. No user-visible change.

As built (review 5): the call catches `InconsistentOverlayException` itself and takes the node as
intersecting (9.2's policy for this caller, now at the caller: the partitioner raises with or without
lists).

### 8.6 User-visible changes (changelog entries, both products)

GPlates (`CHANGELOG-GPlates.md`, under 2.6 unreleased), Assign Plate IDs:

- a polygon cut by a plate is now assigned as polygons with holes, not polylines, and the pieces
  outside one plate are cut by the next as polygons;
- a polygon or polyline that only touches a plate's boundary, or runs along it, is no longer split;
- plates' interior rings (a network's rigid blocks, a polygon's holes) now take part in the cut; a
  plate lying wholly inside a feature's polygon now cuts a hole in it;
- scalar coverages are carried to the pieces of a polygon, values along a cut interpolated between
  its ends, on every ring (with the G1-1 entry);
- "most overlapping plate" compares area for polygons;
- (if 9.2 decides so) a feature the cut cannot partition consistently is reported and left as it was.

pyGPlates (`CHANGELOG-pyGPlates.md`, under 1.1 unreleased), `PolygonOnSphere.partition()`,
`PlatePartitioner.partition_geometry()`, `partition_features()`, `partition_into_plates()`: the same
items in API terms, plus `partition_geometry()` accepting and returning coverages, and
`partition_features()` carrying coverages over (previously dropped).

## 9. Checks, diagnostics and failure behaviour

### 9.1 What can fail, and what cannot

By construction (assertions on the overlay's own code, never on data): every polygon has an even
number of ends at every vertex (6.2), every walk closes (3.10), the kept rays at a sector-walked
vertex equal the keep-rule's (`keep_mismatch`), and the whole-chain and single flips of repair only
ever touch classified pieces. As built, the first three are counted in every build and not asserted
(`Diagnostics::pairing_failures`, `keep_mismatches`), and a pairing failure throws
`InconsistentOverlayException` (counted when not throwing): review 3 found inputs on which the first
was false (the all-common vertex of 3.10, since fixed).

What the data can get wrong is the classes, and the checks of 3.9 test them. A check failure after
repair means the classes describe no consistent arrangement: a missed or spurious intersection (R9,
R11), a band piece's midpoint test propagated where the margin order should have prevented it, or a
vertex the bundle rule could not resolve. None was seen in any normal run (run 3 section 7). Run 4
found, and the one-passage rule removes, a fourth: the sector walk at a six-end vertex where one
polygon passes once, on an order that is exact but uninformative there (19 of 2393 `chain` cases).

### 9.2 What happens then: never a silent wrong answer

The overlay never returns a best guess as if it were the answer. The failure surfaces in each product
as follows.

- **`PolygonOverlay::Overlay`**: `classify()` throws `InconsistentOverlayException` (a new class in
  `src/maths/`, derived from `GPlatesMaths::MathematicalException`, `src/maths/MathematicalException.h:38`,
  as the other maths exceptions are) carrying the `Diagnostics` and the failing vertex's position and
  record kinds, so that a report can be made from it. `extract()` throws the same on a hemisphere
  failure (5.2), and on a pairing failure. Review 3 found one consistent input that reached the
  latter (identical inputs touching themselves: the all-common vertex of 3.10), since fixed; none is
  known now.
- **`PolygonPartitioner`**: lets it propagate, with or without lists. (As first designed, the
  result-only path answered `GEOMETRY_INTERSECTING` instead, for its one intended caller, 8.5; review
  5 found pyGPlates' `PolygonOnSphere.partition` without lists a second caller, so that one polygon
  gave an error with lists and an answer without. The developer decided on one contract: the
  partitioner raises either way, and 8.5 catches it, marking the node as possibly intersecting, which
  costs rendering work and nothing else.) The exception is documented on `partition_geometry`,
  `partition_polyline` and `partition_polygon`.
- **`GeometryCookieCutter`**: lets it propagate (every plate must get an answer, and a wrong one is
  worse than none).
- **pyGPlates**: registered in `src/api/PyExceptions.cc` with `export_exception` (`:217-229`, as
  `IndeterminateArcRotationAxisException` at `:397`) as a Python exception, name to be decided
  (section 14, question 3), raised by `PolygonOnSphere.partition()`, `PlatePartitioner.partition_geometry()`
  and so by `partition_features()`; the docstrings list it.
- **GPlates**: `AssignPlateIds::assign_reconstruction_plate_id` (`src/app-logic/AssignPlateIds.cc:244-259`)
  has no exception handling; the dialog (`src/qt-widgets/AssignReconstructionPlateIdsDialog.cc:491`)
  and the CLI command (`src/cli/CliAssignPlateIdsCommand.cc:386`) call it per feature. Recommended
  (section 14, question 3): catch `InconsistentOverlayException` per feature in `AssignPlateIds`,
  leave that feature unpartitioned, and report the features so skipped once at the end (the dialog
  has no such report today; the CLI writes to `qWarning`). The alternative, aborting the whole
  operation, loses the work on every other feature for a case no run has produced. As built (step
  12, the developer's answer to question 3): `AssignPlateIds` catches it around each feature's
  partition task and records the feature (`get_features_not_partitioned`); the task partitions every
  geometry before it modifies the feature, so the feature is left as it was. The dialog shows one
  warning at the end, naming up to ten features (`get_feature_description`: the name and the ID) and
  counting the rest; the CLI writes one `qWarning` line per feature, and the dialog reports them
  also when the user cancels part-way (review 6 1.2). Tested (review 6) on a committed inconsistent
  input, `polygon-overlay/inconsistent.txt` (the bundle failure `shared#1434 seed 12`, step 15's to
  fix, then to replace with another inconsistent input): the partitioner raises with and without
  lists, and Assign Plate IDs leaves the feature's properties and plate ID as they were and reports
  it (`AssignPlateIdsSkipsInconsistentFeature`). The dialog is GUI, not tested.

Repairs that succeeded are not failures: the output is consistent. They are counted and, in debug
builds, logged with the vertex, so that Phase 5's hunt can require zero on generated data.

### 9.3 Debug-only checks

- **Every original vertex of a piece** is tested, not only the first, and a disagreement is counted
  (`piece_side_disagreements`, the prototype's third check, walk-through 9.1) and asserted in debug
  builds: two original vertices of one piece on different sides of the other polygon means a
  crossing with no record (R11), since a vertex within `h` of the other boundary is a record. It
  costs a test per vertex and is not for release builds.
- (The output-side consistency of every ring's pieces, first designed here, went with 4.3's per-ring
  output side; the hemisphere check of `INTERSECTION` and the differences is counted in release
  builds too.)
- The provenance composition mismatch of 7.2.
- A `GPLATES_DEBUG`-only dump of the overlay (records, vertices as record sets, stops, pieces with
  classes and methods, rays, the pairing) in the text form section 11.2 compares with the prototype.

## 10. Costs

### 10.1 Where the time goes

Per cut of `P` (`N_P` vertices, `R_P` rings) by `Q` (`N_Q`, `R_Q`), with `k` records:

| Step | Cost | Notes |
| --- | --- | --- |
| intersect | `O((N_P + N_Q) log + k)` | existing; the bounding trees reject disjoint geometries at the root |
| vertices | `O(k alpha(k))` | |
| stops and pieces | `O(N_P + N_Q + k)` | one pass per ring |
| loop, spike, common pairs | `O(k)` | hashing on vertex pairs |
| rays | `O(k)` directions, `O(d log d)` sort per vertex of degree `d` | |
| vertex tests | one per non-common piece with an original vertex, at most `k + R_P + R_Q` | against `Q` at its adaptive speed (medium after 8 calls, high after 20 edge-testing calls, doc 3.9: `O(log N_Q)` per test as the cascade reuses `Q`); against `P` at `P`'s adaptive speed, where the first 8 are `O(N_P)` each and the bounds then cost about 15 low-speed tests once (doc 3.9) |
| deduction | `O(passes * sum of d)` | passes is one plus the number of stalled tests |
| midpoint tests | rare (band pieces); each needs `minimum_distance` to the other outline, `O(N log N)` with its bounding tree | 1 to 250 per 300 random cases by generator, run 2 section 3, run 3 section 3.4 |
| bundle enumeration | `k^m * permutations`, filtered; `m <= 6` | fired in about 3% of dense shared-edge cases, never elsewhere (run 3 section 4.2) |
| anchorless chains | `O(N_P + N_Q)` per chain | identical rings only |
| checks, repair | `O(sum of d)` per round, at most 20 rounds | zero rounds in normal runs |
| link, split, clean | `O(k + N_P + N_Q)` | |
| grouping, orientation, hemisphere check | `O(R_out^2)` containment tests (a point's distance to a ring, then a point test), an orientation per ring, one reference point against both inputs and every ring | `R_out` small |
| provenance | `O(points)` | only when requested |

The intersection phase and the cascade over hundreds of plates dominate, as the requirements
estimated (3.3, "Cost": well under 10 ms for `N = 10^4`, `k = 10^2` without set-up). What is new
against today's `PolylineIntersections` is the point-in-polygon tests (today one per cut, now one per
piece with a vertex) and the tests of `Q`'s rings against `P` in the no-record case, which the
bounding-circle pre-check of 4.1 removes for the plates far from `P`, which is most of them.

### 10.2 The result-only path

`classify(stop_when_result_known = true)` stops at the first certain answer:

1. after `intersect`: bounding circles disjoint, `OUTSIDE`; no records, the ring classification of
   4.1 (`INSIDE` iff every `P` ring is inside `Q` and no `Q` ring is inside `P`; `OUTSIDE` iff every
   `P` ring is outside `Q` and no `Q` ring is inside `P`; else `INTERSECTING`);
2. after the vertex tests (3.7a): a `P` piece inside `Q` and another outside, `INTERSECTING`;
3. otherwise the full classification, and the answer from the keep-rule (5.3) without linking: the
   difference output is empty iff no `P` piece is outside `Q`, no `Q` piece is inside `P` and no chain
   is opposite-sides (and the intersection likewise).

Step 3's answer counts collapsed regions that clean-up would drop and slivers the filter would
remove, so it can say `INTERSECTING` where the full path, with the filter on, would return the
geometry whole; the one caller (8.5) wants the conservative answer. With the filter off, 5.3 and this
path agree except for zero-area collapsed regions, for which this path says `INTERSECTING` and the
full path says whole; acceptable for the same reason, and documented on `partition_polygon`.
**Superseded (review 5, developer):** the path must give 5.3's result exactly, so step 3 extracts and
looks, as the full path does; only steps 1 and 2 are shortcuts. **Superseded again (review 6,
developer): step 2 is dropped too.** Its early stop returned before the checks, so on an inconsistent
input (`shared#1434 seed 12`) the call without lists answered `INTERSECTING` where the call with lists
raised: one contract was not met. Without it the path is the full path less the building of output
geometries; its one caller with a need for speed builds a raster's polygon meshes once, at load.

As built (after review 6): the bounding circles (4.1 as built), then the full overlay, classification
and checks, and, when the classes say `INTERSECTING`, the extraction and 5.3's emptiness rule; only the
`PolygonOnSphere`s are not built. The same result and the same exception as with lists in every case.
`Overlay::classify` has no early stop, and the guard below is gone with it (no piece's region is judged
before extraction). The account of step 11 that follows is kept for the record.

As first built (step 11, amended after review 5; superseded above): `Overlay::classify(stop_when_result_known)` and
`Overlay::stopped_early()`. Step 1's disjoint circles are the partitioner's pre-check (4.1 as built).
Its no-record case needs no code of its own: with no records every ring is one closed piece with an
interior vertex, so all are classified by the vertex tests and step 2 or step 3 answers at once. Step
2 also stops at a clip piece inside the subject (kept by both outputs), and **counts only a piece whose
own positions in its output ring (its end vertices' output positions and its original vertices
between them) are three distinct ones**: clean-up drops a ring with fewer, and a vertex-tested piece
can leave an overlay vertex and come back to it (a tip just over the thickness long, `shared#247 seed
1`), making a ring of two positions; without the guard the path said `INTERSECTING` there while the
outputs left the intersection empty (review 5 1.1; the review and the session had both taken the
vertex test's margin to make the stop exact, and a probe of that case showed otherwise). With the
guard such a ring can't collapse: it holds three distinct positions, and the split (3.11) cuts only at
overlay vertices, so the piece stays whole in one ring. Step 3 then extracts both outputs and applies
5.3's emptiness rule, as the full path does. An overlay stopped early refuses to extract (an
assertion). The prototype comparison checks on every polygon case, committed and generated, that the
partitioner's results with and without lists agree, and that a whole result is the subject itself. A
polyline subject has no early stop (its classification is cheap and never throws); a run at one
position, if one can arise (none seen in the polyline sets), is dropped as a collapsed ring is, a side
left with none making the polyline whole.

### 10.3 What Phase 5 measures

Assign Plate IDs on a global coastline file by a real topology model (survey section 5 item 6), the
wall-clock time before and after, at two or three reconstruction times, with `partition_into_plates`
in pyGPlates as the harness. The bundle enumeration cap and the midpoint-test counts are read off the
`Diagnostics` over the same run, and the sliver measurement (4.5) with them.

## 11. Testing plan for Phase 5

GoogleTest throughout (`src/unit-test/`, listed in `src/unit-test/CMakeLists.txt`; the `cpp-test` skill
and `docs/design/testing/README.md` for the conventions). The `GeometryIntersectTest.cc` fixture style
is reused: geometries built in a local tangent frame at a centre on no axis, distances in units of
`H = get_on_segment_start_threshold_sine()` (`src/unit-test/GeometryIntersectTest.cc:42-110`).

### 11.1 Unit tests per stage (`PolygonOverlayTest.cc`)

The prototype's catalogue (`overlay_catalogue.py`, run 1 sections 4.1 to 4.7) translated, one test
per family, each asserting the structure (vertices as record sets, pieces, internal pieces, common
pairs) and the classes, and then the output against the oracle of 11.3:

| Family (run 1) | What it exercises |
| --- | --- |
| acute wedge (4.1), right-angle wedge (4.2) | two `SL` records as one six-end vertex; the tip piece `x, v, x` classified at `v`, dropped by clean-up |
| twist near and on `Q` (4.3) | two `LL` vertices with a zero-length piece between, both tie orders; the `LL` rule |
| D4 segments (4.4) | two touches from a short segment; a `P` hole apex between the foots |
| narrow neck, touching rings, shared edges, T-intersection, tangent rays (4.5) | the margin order (the neck), four-end vertices with the free rule, chain ends and the sector rule, the T-intersection's vertex test |
| SS clusters (4.6) | the loop rule (V1's failure) |
| near vertex (4.7) | ordinary crossings beside a corner |
| run 2's `six#135`, `rays#81` | the margin order and the `LL` rule as regression cases |
| run 3's `shared#28`, `#169`, `six#5` (seed 7), `#188`, `near#291` | the bundle rule, and ring-continuity pairing |
| walk-through 6.4 | a hole touching its exterior: the split pairing; Q11's cut at a six-end vertex |
| anchorless (run 2 section 7's five kinds) | option 3, identical rings and thin faces, with the new `PointInPolygon` entry |
| doc 5.1's four bugs | `Q` inside `P`; `P` across a `Q` hole; `Q` across a `P` hole; a piece along the boundary for both orientations of `Q` |
| the result contract | a touch from outside and from inside, a shared edge, identical polygons: whole geometry and `INSIDE`/`OUTSIDE` |
| polylines (run 3 section 6.1's five generators) | `ends`, `along`, `touch`, `pinch`, and an opened ring; Q2 |
| Boolean operations | the two-squares example under all five operations; a union over a hemisphere throws; symmetric difference |
| provenance | every output point's provenance resolves to its position (vertex, segment ratio, or clip), on holes too; two-step cascade composition |
| coverages | a polygon with a coverage, cut: values at original vertices, at intersections (segment interpolation, including the closing segment), along a cut (Q1), on holes; the `exterior_ring_only` fallback |
| zero-length segments | rays without a direction; a piece all of whose arcs are zero length |

The hunt's generators (`overlay_hunt.py`'s `near`, `chain`, `tinyhole`, `rays`, `six`, `zero`,
`shared`) are not translated into C++; they feed 11.2.

### 11.2 The C++ against the prototype

Record for record, piece for piece, class for class, as PLAN.md Phase 5 requires. The wiring:

1. The prototype gains an exporter (`overlay_export.py`, in the planning repository) that runs
   `run_overlay` under the chosen configuration (`--config`, now `r4t`; it was `bnd` until run 4) on a case from any generator or the catalogue, skips cases
   `arc_ref` flags ambiguous (as the PR A hunt did), and writes one text file per case: the inputs
   as doubles (lat/lon or xyz), the records (kind, both segment indices) in both orders, the vertices
   as sets of record indices, each piece as (polygon, ring, start record, end record, internal), each
   piece's class and method, each common pair, and each output ring of each operation as its sequence
   of (vertex or stop) references.
2. `PolygonOverlayPrototypeTest` reads the files from `GPLATES_UNIT_TEST_DATA_DIR` (a committed set
   of a few hundred cases, chosen to cover every generator and every catalogue family) and compares
   its own dump (9.3) with them. Records compare exactly (PR A's hunt: no mismatch in 36559); where
   a tie order differs, the piece and class comparison is made per vertex and per piece identity, not
   by position in the list.
3. Classes compare exactly except for midpoint-tested band pieces (`METHOD_MIDPOINT_TEST` with the
   arc within `2h` of the other boundary), whose class is arbitrary by nature: for those the test
   accepts either, and the oracle of 11.3 checks the output. Everything else differing is a failure:
   a vertex test, a deduction, a sector class, the anchorless class, the bundle's chosen order.
4. Output rings compare as point sets by the oracle, and as ring structure where the prototype's
   output did not depend on the tie order (its `tie-dependent` flag).

A larger run (thousands of cases, not committed) is Phase 5's hunt, driven the same way from a
scratch directory.

### 11.3 The oracle

As PLAN.md Phase 5: no new exact reference. For random points `p` away from both boundaries (beyond
`M = max(10h, 2 x the widest record cluster)`, run 2's convention):

- `p` is inside the inside output exactly when it is inside `P` and inside `Q` (`is_point_in_polygon`,
  threshold off, parity over each output polygon's rings), and inside the outside output exactly when
  inside `P` and not `Q`; for each operation `f`, inside the output exactly when `f(p in P, p in Q)`;
- the inside and outside outputs are disjoint and their areas sum to `P`'s, with the sliver filter
  off (4.5) and for inputs whose rings do not cross. As built (`PolygonOverlayOracle::area_sum_error`):
  each ring's area by a fan from its vertices' mean, Van Oosterom and Strackee's formula per triangle
  (`SphericalArea`'s fan drops the triangle of every edge that is zero length to `GreatCircleArc`), and
  each polygon's rings signed by their depth among its rings (as point-in-polygon reads them, so an
  island in a hole adds); within `2h` times both inputs' perimeters over `P`'s area, since an output
  ring runs through its vertices' output positions, up to `h` off an input's edge;
- for a polyline, points sampled along it (spherically, `overlay_polyline.py:271-282`) lie in exactly
  one output polyline whose class is their parity against `Q`.

The sampling is global plus annuli round every record cluster (`overlay_oracle.py`), so that the
hard regions are sampled densely.

### 11.4 Real data

Timing (10.3); the sliver measurement for 4.5's decision (a histogram of output rings' mean widths
over the same run, and how many fall under candidate widths of `h`, `10h`, `100h`); and a
before-and-after of Assign Plate IDs on a coastline file by Muller et al. 2022 at a few times, read
as: every output ring passes the oracle, no `InconsistentOverlayException`, repairs zero, bundle cap
never hit. Topology resolving is not touched by PR B.

As built (step 14), in two harnesses:

- **The timing** in pyGPlates (a script in the session, not committed). The coastlines are
  reconstructed by their own plate IDs, then `partition_features` runs over them by the topologies
  resolved at that time, against a pyGPlates built from `main` before PR B.
- **The checks and the sliver measurement** in C++: `PolygonOverlayRealDataTest.Coastlines`, skipped
  unless `GPLATES_POLYGON_OVERLAY_REAL_DATA` names a plate model's directory (as the plate model manager
  lays it out). It runs the cascade as Assign Plate IDs does: `GeometryCookieCutter`'s order (through
  `get_partitioning_polygons`), the partitioner's bounding-circle shortcut and result rule. It calls the
  overlay itself, so every cut's diagnostics and the oracle can be read. Its piece counts equal
  pyGPlates' at every time measured.

### 11.5 pyGPlates tests

- `test_geometries_on_sphere.py:1849-1868` and `test_app_logic/test.py:2131-2139` updated (8.4).
- New: a polygon with a hole partitioned by a polygon with a hole (pieces are polygons with the right
  rings); the three no-intersection bugs; a touch from either side (whole geometry, `inside`/`outside`);
  a polyline along the boundary (`inside`); `partition_geometry` with a coverage and the values at a
  cut; `partition_features` preserving a coverage (`get_geometry(..., coverage_return=...)` on the
  result); `most_overlapping_plate` choosing by area where arc length would choose otherwise (a thin
  piece with long cuts against a fat piece); `PlatePartitioner` over resolved topologies with a
  network's rigid block.
- `pygplates-stub-test` after the regeneration.

## 12. Implementation order for Phase 5

Commits a reviewer can follow, each with its tests; a Fable review after each major one (PLAN.md,
"Who does what"). Steps 1 to 8 are the core and can land as one PR B or be reviewed in sequence; the
PR is one.

1. **Skeleton and structure**: `PolygonOverlay.{h,cc}` in `src/maths/CMakeLists.txt`; `RingView`,
   stops, pieces, vertices by identity (3.1 to 3.3). Tests: the structure of the catalogue families.
2. **Loop and spike rules, common pairs, rays** (3.4 to 3.6), including zero-length arcs. Tests:
   wedge, SS clusters, shared edges, D4.
3. **Classification** (3.7): vertex tests, the free and `LL` rules, the sector walk, the margin order;
   checks (3.9 without repair). Tests: neck, `rays#81`, the T-intersection; the first prototype
   comparison on classes (11.2).
4. **The bundle rule** (3.8). Tests: run 3's residue cases; the dense `shared` prototype cases.
5. **Anchorless chains** (3.9): the `PointInPolygon` entry and option 3. Tests: run 2 section 7's kinds.
6. **Repair and failure** (3.9, 9): single flips, the whole-chain flip, `InconsistentOverlayException`,
   `Diagnostics`. Tests: provoked inconsistencies (a class forced wrong) repaired or thrown.
7. **Linking, split, clean-up** (3.10, 3.11), `extract()` producing rings for every operation. Tests:
   6.4's pinch, the two squares under five operations, the oracle (11.3) on the catalogue.
8. **Grouping, output side, hemisphere check, orientation** (4.2, 4.3), output polygons. Tests:
   holes in both polygons, islands in holes, a union over a hemisphere throws.
9. **Polylines** (6.1), CitcomS switched (6.2); `PolylineIntersections` is deleted in step 11, since
   `PolygonPartitioner` uses it until then. Tests: run 3's polyline generators as cases;
   `citcoms_meshes` still passes.
10. **Provenance** (7.1, 7.2) in the overlay, with its composition (the overlay's own; the wiring
    through `PolygonPartitioner` and the cookie cutter is step 11's, with the partition layer).
    Tests: 11.1's provenance rows.
11. **The partition layer** (8.1) and the result contract (5.3), the result-only path (10.2),
    `PolylineIntersections` deleted (6.2), provenance through the partitioner and the cookie
    cutter (7.2, 8.2: the tables and the original geometries),
    `GeometryCookieCutter` with polygons and provenance (8.2), `GLReconstructedStaticPolygonMeshes`
    unchanged in behaviour (8.5). Tests: doc 5.1's four bugs; the contract cases; a two-plate cascade.
    Both products build; the GPlates visual check waits for Phase 6.
12. **Merge `main`** once the G1-1/G1-4 PR has landed (the `PartitionFeatureUtils.cc` conflict), then
    **coverages and area** in `PartitionFeatureUtils` (7.3, 8.3), and Assign Plate IDs' handling of
    the overlay's exception (9.2). Tests: 11.1's coverage rows through `partition_feature`. (As built:
    the merge did not conflict in `PartitionFeatureUtils.cc`; and after review 6 the overlay's early
    stop was dropped, 10.2.)
13. **pyGPlates** (8.4): docstrings, the coverage form of `partition_geometry`, `partition_features`,
    area, tests, stub, changelog entries in both products (8.6).
14. **The sliver filter** (4.5) behind its option, off; the real-data timing and sliver measurement
    (11.4); the developer decides the default and width; wire the option through if enabled.
15. **The prototype comparison at scale and the hunt** (11.2), zero repairs required on generated
    data outside the cases built to provoke them; Linux Docker run; distillation (Phase 6).

## 13. Rejected alternatives

For the design doc later (Phase 6, doc 5.3 onwards). Each with where the evidence is.

| Rejected | Why | Evidence |
| --- | --- | --- |
| **V0**: every record its own vertex | zero-length pieces between records at one point can only be deduced, never tested; the most midpoint tests of any variant; the only variant whose output depends on the tie order; 33 of 300 `six` cases fail under run 1's rules, 5 of 60 dense shared edges under the inversion adversary; and the wedge's two records *are* one point | run 2 section 11; variants section 2 |
| **V1**: identity alone, no loop rule | two `SS` records at one `P` vertex make a loop arc with no test point and no usable direction: every V1 failure (19 catalogue cases) | run 1 section 4.6; variants section 3 |
| **V2 / V2b**: contraction of pieces shorter than `2h` into clusters (the walk-through's first section 7) | fails in exact arithmetic (19 of 300 `six`, 3 of 60 `chain`, 18 of 299 `shared`): contraction joins two four-end vertices that need no geometry into one six-end cluster whose rays leave from different points; moves output by up to `20h`; drops real thin regions; removes anchors from common chains; is the only variant that merges by distance, with a threshold tied to `GeometryIntersect`'s | run 2 sections 6, 7 and 11; walk-through 7.7 |
| **The rotational order at four-end vertices** | the records encode the snapped topology and the rays the unsnapped one; under about `2h` they disagree; 10 catalogue cases fail | run 1 section 5; walk-through 7.4; variants section 8 |
| **Contraction's six-end order without cones** (reading a chained group's order off the measured angles) | run 2's residue: a short arc leaving from off the output position has a noise direction; 4 of 299 dense shared edges and `six#5` | run 3 section 2 |
| **The `nearest` bundle deduction** (deduce from the tie-break's order alone) | changes no clean output but leaves `shared#28` and `#169` failing: committing early to what a noisy order implies is what made those vertices inconsistent | run 3 section 4.2a |
| **Bundle criteria tried on paper**: single arcs float; rays to one neighbour bundle; bundle everything and solve by parity | wrong both ways (a long single arc's order is real geometry; a short arc with an original vertex is noise); misses the misplaced common ray; two frames with no anchor | run 3 section 3.2 |
| **Option 1 for anchorless chains** (a point offset sideways by more than the tolerance) | fails every thin face | run 2 section 7; walk-through section 5 |
| **Option 2** (FHP's rule: both holes or both not) | needs each ring's hole status by point tests that are fragile when rings touch | walk-through section 5 |
| **Option 4** (an offset arc intersected with both polygons) | `GeometryIntersect` cannot see a boundary within `h` of the arc's start, which is on the chain; option 3 does the same with existing exact tests | walk-through section 5 |
| **An unrestricted waypoint for option 3's path** | arrived from the wrong side in about half the runs, so both memberships described the other side | run 2 section 7; walk-through section 5 |
| **The midpoint of the longest arc as a piece's test point** | an arc leaving a touch at a small angle stays near `Q` and can cross it with no record; the T-intersection | walk-through section 3 (the developer's example) |
| **Testing stalled arcs longest first, or in piece order** | a band piece's arbitrary class propagated through a four-end vertex to a piece `10^4 h` long (9 of 300 `six`) | run 2 section 11; walk-through 7.5 |
| **Snapping or contracting before the overlay** (a tolerance pass on the inputs) | the same failure as V2 at every cluster, plus a second meaning of "the same point" beside `GeometryIntersect`'s; identity vertices need no constant | walk-through 7.2, 7.7; variants section 10 |
| **Dropping one of a corner's two records** (audit F3) | both stand for real crossings; dropping the first-sorted gave wrong output in 15 of 25 cases with no check noticing | run 1 section 4.1; doc 4.5 |
| **Greiner-Hormann tracing with FHP labelling** (candidate a) | one wrong label inverts the rest of the ring silently; cannot represent a twist on `Q`; polylines need their own version; two traces with reversed rules | requirements 3.2 and 3.5 |
| **The output side of each ring from its pieces' classes** (4.3 as first designed: the keep-rule's side of each kept piece, with the input's containment forest and the ring's signed area, then the hemisphere check as an exterior with the output outside it) | a band piece's class is arbitrary, and the output ring runs between the vertices' output positions, not along the piece, so a piece about `h` long puts the output on the side opposite to the ring's (the `d4` catalogue cases); a length-weighted vote fixes those but not rings whose every piece is a band piece (the `tinyhole` hunt's holes, a false hemisphere failure for a union of two small polygons); an input whose rings cross has no one side per ring (the twists, and most hunt cases: every union and symmetric difference threw) | step 8 (PLAN.md) |
| **Grouping by the output side instead of parity depth** | needs the output side at every ring, which at snapped four-end vertices comes from an order the free rule does not trust; parity depth needs only containment tests with margin | 4.2, 4.3 |
| **Partitioning by all plates in one overlay** | a different algorithm; where many-polygon operations belong is the inbox's spatial-index note | survey section 3 |
| **A parallel list of provenance tables beside the geometry lists** | couples two lists by position across three consumers | 8.2 |
| **Deciding the result from the records alone** (any record means `INTERSECTING`, today's rule) | a touch from one side would split a geometry at a boundary it only touches, and the cascade would re-split it at every shared boundary | survey Q4, decided |
| **Repairing silently and returning in release builds** (walk-through 9.3's first proposal) | the developer's requirement: never a silent wrong answer | section 9 |
| **The sector walk at a six-end vertex where one polygon passes once** (the chosen configuration before run 4) | the order there is exact but carries no information; 19 of 2393 `chain` cases fail (a crossing counted at both ends of a band arc, a ray inside a common ray's direction gap, a corner cut off by the spike rule) | run 4 section 2 |
| **One local rule per mechanism**: the along rule, the gap cone (a common ray's cone at least its two tangents' angle), silent one-polygon vertices | each fixes one mechanism only; silent vertices break `tinyhole#123` | run 4 section 2.6 |
| **No spike rule** (`r4n`) | as clean, but 77 against 48 degenerate rings dropped in the catalogue and 10 to 40 more midpoint tests per 300 dense shared edges | run 4 section 3.3 |
| **Chord directions for the rotational order; a one-sided cone for common rays** | chords of snapped arcs can cross without a record; the one-sided cone needs an exact symbolic tie (reasoning only, not built) | run 4 section 7 |

## 14. Open questions for the developer

Everything above is decided or follows from a decision; these four are not.

**Answered 2026-11-04 (developer; PLAN.md Status):** 1 output on the left; 2 the nearest outline
point; 3 `PartitionError`, Assign Plate IDs skips and reports; 4 keep it in PR B; 5 neither form in
PR B: the private helper only, and any geometry-level coverage form is PR C's decision.

1. **Output ring orientation** (requirements Q6). Recommendation: orient every output ring with the
   output on its left (exteriors anticlockwise seen from outside the sphere, holes clockwise), since
   the output side is known and the reversal costs nothing (4.3). The alternative is the walk's
   direction, which is arbitrary. Either way the whole-geometry return (5.3) keeps the input's
   orientation.
2. **A coverage value for an output ring with no valued point** (7.3 step 3: a ring made wholly of the
   partitioner's points, such as `P ∩ Q` when `Q` lies inside `P`). Recommendation: the value at the
   nearest point of the input's outline, interpolated along that segment, which is today's fallback
   kept for this case alone, so no coverage is dropped. Alternatives: drop the coverage from that
   piece; a fixed value (NaN) per point.
3. **Failure reporting** (9.2). Recommendations: the Python exception named `PartitionError`
   (PR C can reuse it for the Boolean operations, or add a more general name then); in GPlates,
   `AssignPlateIds` catches per feature, leaves the feature unpartitioned, and reports the skipped
   features once (dialog and CLI). The alternative, propagating to the top, aborts the whole
   operation.
4. **`GLReconstructedStaticPolygonMeshes`' own containment test** (8.5), redundant once the overlay
   classifies the no-record case correctly. Recommendation: keep it in PR B (harmless; no rendering
   test coverage) and let the Phase 6 visual check decide whether it goes.
5. **The coverage form of `PlatePartitioner.partition_geometry`** (8.4; added by the session review,
   since the decision was only that `partition_features` carries coverages over). Either (a) the
   public coverage form of 8.4, a new API with docstring, stub and changelog entry, or (b) a private
   helper that `partition_features` alone calls, keeping PR B's public API to what was decided and
   leaving a public form for PR C or later. Session recommendation: (b), unless a coverage form of
   `partition_geometry` is wanted for its own sake.

## 15. Session review (Opus, 2026-10-31)

Checked against the code: the four findings of the hand-back (`SS` at the first geometry's vertex,
`GeometryIntersect.h:72-77`; `get_num_polygon_edges_crossed` declared in no header,
`PointInPolygon.cc:301`; `rotation_axis()` throws for zero length; no exception handling in
`AssignPlateIds.cc` or `AssignReconstructionPlateIdsDialog.cc`), the G1-1 branch's
`Coverage::exterior_ring_only` (worktree-2, `c1f2f3c7b`), and the citations of 8.4 (the pyGPlates
tests at `test_app_logic/test.py:2131-2139`, the `partition_geometry` warning at
`PyPlatePartitioner.cc:707-708`). Corrected: 8.3's *unverified* (the C++ metric is not
lexicographic), 3.6's citation for `rotation_axis()`, 7.2's "compose exactly". Added open question 5.
Not checked: the remaining two *unverified* statements (whether `citcoms_meshes` exercises
`find_intersections`; the `coverage_return` call form), both left to Phase 5 as the design says.

