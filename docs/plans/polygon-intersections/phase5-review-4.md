# Phase 5 review 4: steps 9 and 10 of the overlay core

Reviewed 2026-11-05 (Fable), the diff `2e09dec73~1..9a80747f2` (code commits `799b9e091` and
`32354b4cc`): `src/maths/PolygonOverlay.{h,cc}` (`classify`'s polyline branch,
`classify_polyline_pieces`, `extract_polylines`, `get_vertex_provenance`,
`get_original_vertex_provenance`, `get_polyline_stop_provenance`, `compose_subject_location`, the
walk's `Entry` and the provenance carried through split, clean-up and orientation),
`src/app-logic/GenerateVelocityDomainCitcoms.cc`, `src/unit-test/PolygonOverlayTest.cc`,
`src/unit-test/PolygonOverlayOracle.h` and `src/unit-test/PolygonOverlayPrototypeTest.cc`, against
`phase4b-design.md` sections 2.8, 6.1, 6.2, 7.1, 7.2 (with their "As built" parts), 7.3, 8.1, 8.2,
and the prototype's `PolylineOverlay` (`overlay_polyline.py`, `CONFIGS['r5c']`). Line numbers are at
`9a80747f2`. Evidence is marked *run* (a build of `gplates-unit-test` in the Release tree with
temporary probes and environment-gated mutations in the three source files, all since removed by
`git checkout` and a touch; `git status` shows only this file) or *reading*.

Baseline: 57 `PolygonOverlay*` tests pass and `File` is skipped; the four uncommitted sets report
`4192 prototype cases, 0 differ`, `2099 ..., 0 differ`, `300 ..., 0 differ` and `1899 ..., 0 differ`
(the last the polyline set). `GenerateVelocityDomainCitcoms*` passes.

The polyline classification and output are a faithful translation of the prototype (section 3), and
the provenance works where it is tested: *run*, every location of every output point of every
committed and uncommitted polygon case resolves within `1.0h` of its point, and within `1.63h`
after a second cascade step, against a test tolerance of at least `10h`. The findings are one
correctness defect in the provenance that the oracle cannot see (1.1), one in the polyline's clip
locations (1.2), a second cause of composition mismatches that the design's "As built" text does not
know about and the test's gate hides (1.3), then what steps 11 and 12 will need, test gaps, and
design statements.

## 1. Findings, most severe first

### 1.1 Provenance is computed before the split, so a split ring's vertex can be wrong

`PolygonOverlay.cc:3619-3643` fills each vertex entry's provenance from the pieces the *walked* ring
arrives and leaves along; `:3676-3707` then splits a ring passing a vertex twice into `v, A` and
`v, B`. Each half keeps the entry it got, but at `v` the half `v, A` now arrives along `A`'s last
piece (not `B`'s), and `v, B` along `B`'s last piece (not `A`'s): the two entries' arriving pieces
are swapped. `get_vertex_provenance` (`:4056-4070`, `:4099-4101`, `:4127-4131`) reads the subject
and clip locations from the walked pieces whenever the vertex holds no subject (clip) vertex,
trying the leaving piece first, then the arriving one, so at a split vertex with no subject vertex
the stored subject location can be:

- **missing** (the entry says a cut) where the half-ring really runs along a subject piece into `v`,
  so a coverage (7.3) interpolates the point along its run instead of along the subject's segment;
- **present** where the half-ring runs along clip pieces on both sides (the design's cut), taken
  from the other passage's subject piece; or on the other passage's subject segment rather than
  this one's. The position is within `h` either way, but a coverage reads a different segment's
  vertex values.

The clip location goes the same way (a different clip segment at the vertex).

*Run*, recomputing each vertex entry after the split from its half-ring's own pieces and counting
entries that differ (first-step overlays only, every operation): catalogue 26 entries (none
missing), hunt 181 (73 missing, 57 clip differences), bundle 17 (5 missing); the large set 2098
entries in 611 of 4192 cases (900 missing, 680 clip), the chain set 989 in 266 of 2099 (116
missing, 208 clip). The committed `six#13`, `six#20`, `six#26` (4 missing each), `near#2`, `#16`,
`#18`, `#26` (5 each), `shared#38` (6) and `d4 ... hole-between` (2 each, the symmetric difference's
four-point ring at a crossing of the hole's edge: stored "on hole segment 1 at 0.5", the half-ring
arrives and leaves along the clip there) are examples. Every test passes because
`count_provenance_failures` accepts a point with a clip location and no subject location
(`PolygonOverlayOracle.h:506`), and the location it does have resolves to the point (2.1).

Fix: fill the provenance per ring **after** the split and before clean-up, which is what the
recomputation did; the `Entry` already carries the leaving piece, and the arriving piece is the
previous vertex entry's, cyclically, in a half-ring as in a whole one. Better still, store in the
`Entry` the two *stops* (the leaving piece's stop at this vertex, `end.at_end ? end_stop :
start_stop`, known in the walk at `:3573-3576`, and the arriving piece's), so that
`stop_of_side` need not guess which end of a piece is at the vertex: `:4065-4067` picks the start
stop whenever the piece starts here, which is the wrong end of a non-internal loop (a piece from
`v` round an interior original vertex back to `v`, the wedge's clip loop in the `wedge ... straight`
catalogue cases) whose two stops lie on different segments. Add a hand test (2.3).

### 1.2 A polyline stop's clip location is its identity vertex's, up to the group's extent away

`get_polyline_stop_provenance` (`:4163-4184`) takes the clip location from
`get_vertex_provenance(stop.vertex, stop.position, none, none)`, which returns the vertex's
`clip_vertex` (the first clip vertex named by any record of the group, `:4094-4098`) or the clip
segment of the vertex's first record (`:4103-4115`). A polyline's output point is the stop's own
position (step 9's deviation), and the group can extend far along a dense run of touches (design
6.1 as built), so the clip location can name a clip vertex many `h` from the point. Design 7.1's
table says "the `Q` location ... by projection".

*Run*, resolving each polyline point's clip location: in the committed polyline set 15 of 1590
points are farther than `2h` from their clip location (up to `4.0h`); in the uncommitted set 194 of
16391 (up to `11.8h`). Every subject location, and every clip location of a polygon output, is
within `1.0h`. The test tolerance is the oracle's margin `M = max(10h, 2 x cluster width)`
(`PolygonOverlayOracle.h:121`), and the cluster width is exactly this group's extent, so it cannot
see it.

No consumer reads a polyline's clip location today (7.3 reads the subject; the cascade's clip
branch is never reached for a polyline, since every edge of a run is a sub-arc of the polyline), so
this costs nothing yet; but it is a wrong value by the design's definition. Fix: use the stop's own
records (`d_stop_records[stop.first_record ..]`, `:633`, `:668`): the clip vertex one of them names,
else that record's clip segment with the stop's position projected; only a self-touch cut stop (no
record, `:593-602`) needs the vertex's.

### 1.3 A second cause of composition mismatches, which the test's gate hides

`compose_subject_location` (`:4236-4361`) composes a point inside a piece's edge from the table
entries of the edge's ends, and counts a mismatch when they share no segment. The design's "As
built" (7.2) attributes every mismatch to an identity vertex holding two vertices of one input,
and the comparison (`PolygonOverlayPrototypeTest.cc:1094-1103`) accepts any number of mismatches in
a case with such a vertex (`holds_two_vertices_of_one_input`, `:501-525`).

There is a second cause. A `Provenance` holds one location per input, but a point at a corner of an
input, within the thickness of two of its segments and not naming the vertex (the `wedge ...
r=+1.2h` and `near vertex rho=+1.05h` catalogue families), lies on both: the recorded location is on
one of them (ratio about `0` or `1`), and the piece's edge along the *other* segment then composes
from a location on segment `k` and one on segment `k - 1` or `k + 1`, which share nothing. *Run*, a
second cascade step against the clip moved by 0.37 of its radius (so that it cuts the pieces' edges
inside, which the same clip never does): catalogue 12 of 457 pieces mismatch (48 points) and none
of them holds two vertices of one input; the large set 116 of 4258 pieces (872 points), 20 without
such a vertex; the chain set 24 of 455, 1 without. Every point still resolves (0 failures of 39080
points at `2M`): the mismatch is counted, never a wrong value. So the "every one in a case with
such a vertex" statement holds only for the same-clip second step the test runs, and the gate
(which passes about 20 % of the large set's pieces, 826 of 4258, whatever their mismatch count)
would fail on the moved clip.

Two consequences:

- `provenance_mismatches` conflates two things. In the clip branch (`:4332-4354`) failure and
  success differ only in the counter: `provenance.clip` was already set to this clip's location
  (`:4091-4115`) and `subject` is none either way. In the subject branch (`:4317-4331`) a failure
  loses the point's subject value. Count them apart (step 11's diagnostics will want to know which).
- A geometric tolerance would resolve the corner case: a ratio within `h` of a segment's end is
  that end's vertex. That needs segment lengths, which the composition does not have (2.5 below).

### 1.4 What steps 11 and 12 will need (reading)

- **Shapes and order.** `Location` (`PolygonOverlay.h:594-607`: ring, index, optional ratio) and
  `Provenance` (`:614-625`) are what 7.3 reads: a vertex's value, a segment's interpolation with
  `j + 1 mod n` (the coverage knows `n`), none for a cut. `provenance_seq_type` is one entry per
  point. An `OutputPolygon`'s table is `exterior.provenances` followed by each
  `interiors[i].provenances`, which is `PolygonOnSphere::create(exterior, interiors)`'s vertex
  order and `GeometryUtils::get_geometry_points`' (exterior ring then the interiors in order,
  `GeometryUtils.cc:101-129`; `create` keeps every point given). `extract` reverses provenances
  with points (`:3925-3926`) and copies rings whole into polygons. Sound.
- **A cut point's value** (7.3 step 2) needs only `subject` none and the ring's arc lengths, which
  the output ring gives. Nothing more from the overlay.
- **The composition's clip locations are enough for 8.2** in the weak sense that nothing reads them
  but the composition's own clip branch, whose failure changes nothing (1.3). The cookie cutter
  must pass `Options::clip_index` as the plate's position and carry each piece's table forward;
  the identity table of a whole `INSIDE`/`OUTSIDE` geometry is the caller's to build.
- **Ring sizes.** The finding is right as far as it goes: a table entry "vertex 0" cannot say
  whether it ends the ring's last segment, and `Options::subject_ring_sizes` and `clip_ring_sizes`
  (`:710-711`) are needed given the `Location` shape, with the cookie cutter collecting every
  earlier plate's ring sizes by clip index. But the cleaner interface for step 11 is to give the
  composition the original subject geometry and the clips themselves (the cookie cutter has them):
  ring sizes come from them, and so do the segment lengths that 1.3's corner tolerance needs. The
  sizes-only design has no way to grow into that.
- **A vertex holding subject vertices of two rings** (a hole's apex at a self-touch cut on the
  exterior's edge, 3.5) is output at, and attributed to, the first named (`:499-518`), whichever
  ring the output ring runs along; under a full coverage the exterior's point takes the hole
  vertex's value. Deterministic (records are ordered) but arbitrary; 7.3 may prefer the walked
  ring's vertex, which the walk knows. Note it for step 12.
- **PartitionedGeometry** (8.2) carries one table per geometry; nothing here prevents that. The
  `want_provenance`-off path allocates nothing (`:3619`, `:3727`, `:4030`).

### 1.5 Minor observations (reading unless marked)

- `extract_polylines` (`:3973-3999`): if every piece is internal (a polyline whose only piece is a
  loop: both ends at one vertex, no interior original vertex, so shorter than `2h`) every class
  stays `CLASS_UNKNOWN`, the one run is output as outside, and `classify` (`:1983-1994`) returns
  `RESULT_OUTSIDE`. Degenerate (step 14's near-zero class), unreached by any case; an assertion or
  an explicit rule would be better than a default that falls out of `CLASS_UNKNOWN != CLASS_INSIDE`.
- `classify_polyline_pieces` (`:2054`) tests the first test stop only, as the prototype does
  (`overlay_polyline.py:233-240`); the debug-only disagreement count matches.
- `compose_subject_location` asserts the table's length against the subject's point count
  (`:4214-4221`), an `AssertionFailureException`, not `InconsistentOverlayException`: right for a
  caller error.
- The subject-first rule (`:4313-4331`) cannot misattribute a cut edge to the subject for polygon
  inputs: two ends on one subject segment with the edge along the clip would need one clip arc to
  meet one subject arc twice.
- `ratio_along_segment` (`:80-113`) guards the zero-length segment before `rotation_axis()`, the
  trap PLAN.md records.
- CitcomS (`GenerateVelocityDomainCitcoms.cc:272-278`): `GeometryIntersect::intersect` returns
  false with an empty graph when nothing intersects, so `unordered_intersections[0]` is guarded.
- Code style: nothing found in the diff (hard tabs, Allman braces, no line over 100 columns with
  tabs at four, references and pointers bound to the name).
- No non-determinism: every container that reaches an output is ordered.

## 2. Test gaps

*Run*, two environment-gated mutations of `PolygonOverlay.cc` (the suite and the uncommitted sets):

| Mutation | Suite | Generated sets |
| --- | --- | --- |
| M1 every polyline run's points (and provenances) reversed (`extract_polylines`) | `PolylineTouchAndEnd` only | polyline set passes |
| M2 no subject location at a vertex holding no subject vertex (`:4127-4131` skipped) | `Provenance` only | large set passes |

1. **The oracle cannot see a missing subject location** (M2, and 1.1). `count_provenance_failures`
   (`PolygonOverlayOracle.h:489-537`) takes a point with only a clip location as right. The
   generated comparison then pins nothing about *which* points have a subject location; one hand
   test does, by counting kinds (`PolygonOverlayTest.cc:2263-2280`). Make the `Provenance` test
   assert the whole expected table of one output (each point's subject and clip location, ring,
   index and ratio to `1e-9`) for the intersection and the difference of its two squares; that
   also pins the ratio's direction and the projection, which no test does exactly.
2. **The tolerances are ten times looser than the code needs.** *Run*: every subject segment
   location and every polygon clip location resolves within `1.0h` (committed and uncommitted,
   first step), within `1.63h` after the same-clip second step and `1.0h` after the moved one. The
   comparison allows `M >= 10h` (`PolygonOverlayPrototypeTest.cc:996`, `:558`) and `2M` for the
   second step (`:1092`). A wrong ratio by up to `10h` along a segment, or a wrong segment shorter
   than `10h` (the dense generators' segments are that short, and there `M` is twice the cluster
   width besides), passes. Use `2h` for the first step and `3h` for the second; the hand tests
   already use `2H`. The margin `M` belongs to the point oracles (a sample must be clear of the
   outline), not to a location that must resolve to a point *on* the outline.
3. **No test splits a ring at a vertex holding no subject vertex** (1.1). The wedge and
   `LinkBothPassTwice` split at subject vertices (exact locations, no difference). Add one: the
   symmetric difference of two squares whose edges cross twice near one point gives a four-point
   ring through a crossing, or simply assert the composed table of `d4 len=+1h phi=20
   hole-between`'s symmetric difference once 1.1 is fixed.
4. **The second-step gate is per case and one-sided** (1.3). Replace
   `!holds_two_vertices_of_one_input(overlay)` with an expectation computed from the table alone:
   the number of edges of the piece whose ends share no segment (replicate `shared_segment` with
   the ring sizes in the test), and require that every mismatch lies inside such an edge, or at
   least that a piece with a mismatch has one. And run the second step against a *moved* clip as
   well as the same one (`:1044-1104`): with the same clip every record on a subject part of the
   piece is already a vertex of it, so the subject-ratio composition at scale is exercised only by
   the two hand cascades; the moved clip exercised it on 39080 points without a failure.
5. **A polyline run's points are compared with nothing but the oracle** (M1).
   `compare_polyline_output` (`:589-649`) names the runs from the overlay's *pieces* and compares
   those names with the prototype's; of `extract_polylines`' output it checks the run count and
   classes (`:652-663`), and the oracle checks coverage within `2h`
   (`PolygonOverlayOracle.h:364`). A reversed run, a run with its points shuffled, or one missing
   a vertex whose turn is under `2h` passes. Compare `polylines[r].points` with the named stops'
   positions, in order, coordinate for coordinate.
6. **`clip_index` is not checked by the comparison's second step**: both clips are `Q`
   (`:1091`), so a wrong index resolves on the same ring. The hand cascade's plates differ and
   would catch it; fine, but say so.
7. The polyline hand tests check classes and point counts (`polyline_runs`); with (5) fixed they
   need nothing more. `ProvenancePolyline` covers the polyline cascade, where the induction holds
   (1.2).

## 3. Checked and found correct

Each item was traced against the prototype function or the design section named, by reading, and
the suite and the four uncommitted sets were run with the probes of section 1.

- **The polyline classification** (`classify_polyline_pieces`, `:2003-2171` against
  `PolylineOverlay.classify`, `overlay_polyline.py:225-283`): the along class from the option,
  `METHOD_ALONG`, counted; the vertex test at the first interior original vertex, else a dangling
  end, threshold off, with the debug disagreement count; deduction only at a vertex of one
  `SEGMENTS_CROSS` record with exactly two non-internal polyline ends, neither common (the
  prototype's `_is_ll_vertex`, `overlay_proto.py:1430-1432`, and its `ll_common` skip); the
  disagreement counted once per vertex; stalled pieces by chord-midpoint margin to the clip, the
  largest first, exactly as 3.7(c). The result from the non-internal pieces' classes matches 6.1's
  contract since internal pieces take a neighbour's class in the output.
- **The polyline output** (`extract_polylines`, `:3963-4037` against `output`, `:285-315`): an
  internal piece takes the class before it, else the first non-internal after; runs of one class;
  each stop's own position, consecutive repeats dropped (exact); provenance per emitted point. The
  deviation from 6.1 (stops' positions, not vertices' output positions) is right for the reason the
  plan gives, and adjacent runs share their boundary stop.
- **The dangling end** (review 3 1.4): `extract_rings` and `extract` assert a polygon subject
  (`:3263`, `:3780-3782`); the walk cannot reach a polyline piece.
- **CitcomS** (6.2): `GeometryIntersect::intersect(graph, *line_1, *line_2)` and the first record's
  position; `citcoms_meshes` passes.
- **Provenance at a vertex** (`get_vertex_provenance`, 7.1): a vertex holding a subject vertex is
  that vertex exactly (the output position is that vertex, `:521-528`); else the walked subject
  piece's stop, which is always inside a segment (a record at a subject vertex would have named
  it), projected; else a cut. The clip location is always set: clip vertex, walked clip piece's
  stop, or the first record's clip segment, and every vertex lies on the clip. A common pair is
  walked along its subject piece (review 3), so a point on a shared boundary gets both locations.
  The clip location is set before composing, so a table's earlier clip does not replace it.
- **Original-vertex stops** (`:4137-4159`): subject ring's → subject vertex (composed); clip
  ring's → clip vertex, a cut. A kept closed piece's ring is its stops' provenances.
- **A polyline stop** (`:4163-4184`): a vertex or a segment-and-ratio of the polyline (the last
  vertex's `segment_out` is `num_segments`, the right index), composed. The clip part is 1.2.
- **The composition** (`compose_subject_location` against 7.2): the table index is the ring's
  offset in subject order plus the vertex index, which is `get_geometry_points` order; a vertex
  location is the entry itself, keeping an earlier cut's clip and index; a segment location
  composes `t_a + t (t_b - t_a)` over the segment both ends lie on, with a vertex counted as ratio
  `0` of its own segment and `1` of the one before, the wrap-around only with a ring size, and for
  a polyline never (`:4322`); reversed pieces (a hole after orientation) compose as `1 - t`, which
  `ProvenanceCascade`'s cut hole exercises. By hand: the only configurations with two candidate
  segments are rings of three or fewer vertices. The candidate search returns a segment only when
  both ends are on it, so the "never a wrong segment" claim holds for the subject branch; the
  failure modes are counted (1.3).
- **The ring-size finding** is genuine: without `n`, "vertex 0 with a partner on segment `k > 1`"
  is the wrap-around or a group-spanning edge, and the `six` generator distinguishes them.
- **The induction failure** is genuine where the plan says (a group holding two vertices of one
  input, 826 of the large set's pieces have one) and, *run*, the same-clip second step produces
  mismatches only in such pieces (large 20 of 4258, chains 30 of 455, holes 0 of 603).
- **Clean-up, split and orientation carry provenance** in step with the points (`:3717-3740`,
  `:3703-3706`, `:3925-3926`); the collapsed-ring drop discards both.
- **The oracle's `resolve_location`** (`PolygonOverlayOracle.h:459-476`) uses
  `GreatCircleArc::point_on_arc`, the parametrisation 2.8 specifies, with a polyline's last segment
  never wrapping because its locations never reach it.
- **The polyline oracle** (`count_polyline_failures`, `:348-449`): samples along the subject beyond
  `M` of the clip, each covered within `2h` by at least one run of its class and by at most as many
  runs as the subject passes the point (consecutive segments within `2h`, so an opened hunt ring
  that doubles back is allowed two); the sample's own class from `is_point_in_polygon` threshold
  off. Sound for what it measures (2.5 for what it does not).
- **The prototype comparison of pieces, classes and runs** for polylines (`:898-947`) uses the
  polygon path's record and piece matching unchanged; the run names resolve records through the
  overlay-to-prototype map and polyline vertices as `P0.<index>`, and the comparison is skipped when
  the prototype's runs depend on the tie order.

## 4. Design statements to amend

- **7.2 as built**, "every one in a case with such a vertex": true of the same-clip second step
  only; add the corner cause (1.3) and that a clip-branch mismatch loses nothing.
- **7.1 as built**, "the subject location comes from the walked subject piece (the one the output
  ring arrives or leaves along)": true of the walked ring, not of a split half-ring, until 1.1 is
  fixed; and "the clip location is ... a walked clip piece's stop" picks the start stop of a piece
  that starts and ends at the vertex.
- **7.1 as built** for a polyline, "its points' locations are the stops'": the subject location is;
  the clip location is the identity vertex's (1.2).
- **PLAN.md, step 10**: "never a wrong value" needs 1.1's qualification, and "a location by ratio
  within the oracle's margin `M`" understates what was measured (within `1.0h`), which is the
  argument for 2.2.
- **6.1**: nothing to amend; the deviation is recorded and right.
- **2.8**: the struct sketch keeps `d_` prefixes the code dropped (the as-built note says so);
  `OutputPolyline`'s field order in the code is `points, inside, provenances`.
