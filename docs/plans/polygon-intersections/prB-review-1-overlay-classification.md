# PR B review 1: the overlay's construction and classification

Reviewed 2026-11-08 (Fable), at `6081e9496` (the whole of PR B, `public/main...HEAD`, seen as one):
`src/maths/PolygonOverlay.{h,cc}` from `Overlay::Overlay` through `classify()` and `check_and_repair()`
(`build_vertices`, `build_stops_and_pieces`, `apply_loop_and_spike_rules`, `find_common_pairs`,
`find_self_touch_cuts`, `build_rays`, `measure_ray`, `sort_rays_anticlockwise`, `build_bundle`,
`one_passage_side`, `solve_four_end_vertex`, `solve_vertex`, `deduce_to_fixed_point`, `classify_pieces`,
`get_common_chain`, `classify_anchorless_chain`, `check_and_repair`, `repair`, `find_failing_vertex`,
`get_result_from_classes`, `classify_polyline_pieces`, the `Diagnostics` and
`InconsistentOverlayException`), with `PointInPolygon::get_crossing_parity_excluding_segment`
(`PointInPolygon.cc:2289-2345`) and the top of `PolygonPartitioner.cc`'s `overlay_polygon` where
classification feeds it, against `docs/design/architecture/foundation/spherical-geometry.md` sections 1,
4.2, 5.2 (to "Checks and repair", and "Polylines"), 5.3 and 5.6, and the eight step reviews
(`phase5-review-1.md` to `-8.md`) for what was found and fixed before. Line numbers are at `6081e9496`.

Evidence is marked *run* or *reading*. *Run*: two temporary probe tests appended to
`src/unit-test/PolygonOverlayTest.cc` (an anchorless chain with a short clip segment, and an octagram for
the LL rule; both also run through `PolygonPartitioner::partition_polygon`), and nine mutations of
`PolygonOverlay.cc`, each applied to a freshly restored file, rebuilt and run against
`PolygonOverlay*:PolygonPartitioner*:PartitionFeature*` (93 tests). All removed afterwards by
`git checkout` and a touch of both files; the unit-test target rebuilt; the overlay suites green (91 passed,
2 skipped) and the full suite green (`ctest`: 203 of 203, `PolygonOverlayPrototypeTest.File` and
`PolygonOverlayRealDataTest.Coastlines` skipped by design); `git status` clean but for this file. Probe
sources and the mutation script were kept in the session's scratchpad.

Baseline (*run*, before any change): `gplates-unit-test --gtest_filter=PolygonOverlay*:PolygonPartitioner*`
82 tests, 80 passed, 2 skipped.

One silent wrong answer was found, on an input class the design names as a target (1.1); one structural
edge case by reading (1.2); the rest is dead branches, test gaps and page wording. The fixes of reviews 1
to 8 that fall in this area were checked and hold (section 3).

## 1. Findings, most severe first

### 1.1 An anchorless chain's clip parity is taken at the subject's midpoint, which is off the clip's great circle: a short clip segment there classifies the whole chain wrongly, and the partitioner returns the wrong whole answer without extracting (run)

`classify_anchorless_chain` (`PolygonOverlay.cc:3002-3100`). The chosen arc's test point `X` is the
chord midpoint of the *subject* piece's two stops (`:3047-3049`). Both parities are counted to that one
point: `membership(*d_subject_polygon, ..., X)` and `membership(d_clip, ..., X)` (`:3080-3089`), each
through `PointInPolygon::get_crossing_parity_excluding_segment(polygon, R, X, excluded)`. The argument
the page and the header (`PointInPolygon.h:109-130`) give is that the crossing arc from `R` meets the
excluded segment's great circle *only at `X`*, so the parity is that of the points just before `X` on
`R`'s side. That holds for the subject when `X` is on the subject's great circle. It does not hold for
the clip: `X` lies on the subject's arc, which is a different great circle from the clip partner's, up to
the thickness away. The arc from `R` then crosses the clip's great circle not at `X` but at a point `Y`
displaced along the chain by `d / tan(theta)`, where `d` is `X`'s offset from the clip's great circle and
`theta` the angle at which the arc from `R` meets the chain. If `Y` lies beyond an end of the excluded
clip segment, the arc reaches `X` across the clip's great circle without crossing the excluded segment,
the parity is the membership of `X` itself (on the far side of the clip's edge), and the code reads it
as the membership on `R`'s side: the clip's membership is inverted, so the chain's class is inverted.

The same displacement affects the subject's parity when a subject stop is a touch record at the clip's
vertex (an LS record is placed at the clip's vertex, up to `h` off the subject's segment, section 4.2),
but the excluded subject segment is the whole original segment, which is normally long, so `Y` stays
within it. The clip side fails whenever the clip's *original* segment partnering the chosen arc is short
compared with `d / tan(theta)`: `d` is up to about `h / 2`, and `tan(theta)` is small for a thin
polygon, where every edge's great circle runs close to the outside point's antipode (the farthest-great-
circle choice at `:3014-3036` maximises the perpendicular distance, but for a thin shape that is at most
the shape's width).

*Run*, probe `ProbeAnchorlessShortClipSegment`, in the test file's tangent frame `F` (radians; `H` the
thickness). The subject is a thin needle, 0.01 rad long and `2 w` wide:
`P = [Pa = (0.001, 0), M1 = (0.004, w), B = (0.01, 0), M2 = (0.005, -w), A = (0, 0)]`. The clip is the
same ring with `Pa` replaced by `Qa = Pa + a H p` and one extra vertex `Qb = Pa + 2 H e + b H p`, where
`e` is the unit direction of `Pa -> M1` and `p` its perpendicular pointing out of `P`; so `Q`'s segment
`Qa -> Qb` is `2 h` long and the two rings coincide within the thickness everywhere (records
`LS SS SS SS SS SS`, six common pairs, one anchorless chain, no other piece). With `(a, b)` over
`{0.3, -0.3, 0} x {0.5, -0.5, 0}` and `w` in `{1e-4, 1e-5}`:

- `(0.3, 0.5)`, `(0.3, 0)` and `(-0.3, -0.5)`, at both widths: every piece is classified **`opposite`**
  and `classify()` returns **`RESULT_OUTSIDE`**; the right answer is `same` and `RESULT_INSIDE` (the
  `ClassifyAnchorless` test's answer for a jittered copy). The chosen arc is `p0`, the `2 h` subject
  piece `Pa -> Qb` (`dump()`: `p0 P0[v0->v1] len=2.06h cls=opposite anchorless common`), whose clip
  partner is the whole segment `Qa -> Qb`. Every vertex is consistent (a chain's vertices carry only
  xor constraints), `check_failures = 0`, `repairs = 0`, so nothing in `check_and_repair` can notice.
- With `a = 0` (`Qa` exactly on `Pa`, so `X` is on the clip's great circle) every case is right, as the
  analysis predicts.
- **Through `PolygonPartitioner::partition_polygon(P, inside, outside)` with the default options**
  (throwing on), both widths: **`GEOMETRY_OUTSIDE`, `inside` empty, `outside` holding the subject whole,
  no exception.** `overlay_polygon` returns as soon as `classify()` says whole-in or whole-out
  (`PolygonPartitioner.cc:124-129`), so the hemisphere check of `extract` never runs for a whole answer,
  and the one guard that could have caught the wrong class is not reached. (With
  `throw_if_inconsistent = false` and an explicit `extract(INTERSECTION)`, the check does count a
  failure at `w = 1e-4`, where the bounding-circle centre is inside both; at `w = 1e-5` the centre is
  within `10 h` of the outline, the first clear candidate is its antipode, outside both, and the check
  passes: so even an extracting caller is protected only for a fat subject.)

Consequence for users: a polygon partitioned by a near-copy of itself along a short clip segment (the
page's own examples: static polygons partitioned by themselves, a hole of `P` equal to a ring of `Q`,
what is left of `P` after earlier plates being exactly the next plate; here with the two copies not
bit-identical, as reprojection, rounding on save, or a boundary digitised along another leaves them) is
reported wholly *outside* the plate it coincides with, and in the cascade goes on to the next plate.
Thin polygons are the exposed class (slivers between plates are exactly that), but the condition is only
that the clip's segment at the chosen arc be shorter than `d / tan(theta)`.

**Fix.** Count each input's parity to a point on *its own* excluded segment's great circle, and compare
the reference points' sides against each input's own plane: `X_P = normalise(X - (X . axis_P) axis_P)`
with `axis_P` the subject segment's rotation axis, `X_Q` likewise with the clip partner segment's axis
(both within the arcs' extents, since the pieces lie inside their segments), the `membership` lambda
taking the axis and test point as arguments (its degeneracy guard, `:3068-3069`, then uses the right
plane too); and flip `in_clip` when `(dot(R_P, axis_P) > 0) != (dot(R_Q, axis_Q') > 0)`, where
`axis_Q'` is `axis_Q` negated if `dot(axis_P, axis_Q) < 0`, so that the two planes' positive sides
agree. With `X_P` and `X_Q` exactly on their great circles the arc from `R` meets each excluded great
circle only at the test point, and the header's argument is restored. The probe above, with `a != 0`,
is the regression test; one of the two needle widths is enough. The `ClassifyAnchorless` test did not
see this because its rings have only long segments, and the identical and reversed copies have `d = 0`.

### 1.2 A self-touch cut is placed after the vertex's first stop on the segment, which is not always the cut piece's start (reading)

`d_self_touch_cuts` is keyed by `(ring, ring-local segment, start vertex)` (`PolygonOverlay.h:1015-1016`,
filled at `PolygonOverlay.cc:1198-1206`), and on the rebuild `add_self_touch_cuts` appends the cuts after
the *first* stop of that vertex on that segment, with `self_touch_cuts_added` refusing a second placement
(`:633-659`, called at `:690` and `:724`). If the vertex has two stops on the same segment, a vertex stop
at the segment's start and a record inside the segment in the same identity group, and the cut piece
starts at the second, the cuts land inside the loop piece between the two stops instead of between the
piece's own stops. The loop then splits into pieces between distinct vertices (no longer a loop), the
first of them pairs with the other input's first chain piece, and the piece that was to be cut stays
unpaired, which is the band-pieces-facing-a-chain structure the cuts exist to remove.

Can a vertex have two stops on one segment? Not from a clip vertex coinciding with the segment's start
(section 4.2's step 4 emits no touch at an arc whose end point coincides, and step 1 comes first), so it
needs a self-touch of the input: ring vertices `k` and `k'` (not adjacent) within the thickness of one
clip vertex `Y` (two SS records chain them into one group), and a second clip vertex `Y'` coinciding
with `k'` and touching segment `k` inside its lune, farther than `h` from `k`'s end points. Rare, and not
constructed. The structure would then most likely fail a check and throw, not answer wrongly, but it is
an input state the code assumes impossible. Fix: key the cuts by the start stop's identity rather than by
`(segment, vertex)`, for example by the index of its first record (stable across the rebuild; a cut stop
has none, but a cut stop never starts another cut piece, since the cuts make the piece common), or
record the ordinal of the vertex's stop on the segment.

### 1.3 Minor (reading unless marked)

- **The one-passage rule's mirror order never fires** (`:1787-1808`). When every class is known and the
  relation `m_in(b) = m_in(a) xor n` holds, the first built order (`a`, the rays inside in measured
  relative order, `b`, the rest) always has a placement: start the sector after `a` at `in X = 1`,
  `in Y = m_in(a)`; each inside ray flips `Y`, so the sector before `b` has `Y = m_in(b)`; `b` flips `X`
  (and `Y` if common, which its xor constraint allows); the outside rays need `X = 0`; and the walk
  closes because `Y`'s end count is even. So the mirror is dead code, harmless; the page describes a
  branch that cannot run (section 4).
- **The bundle rule's tie-break is not exactly "enumeration order"** (`:2908-2929`). The measured order is
  moved to index 0 by `iter_swap`, so the first enumerated order takes the measured order's old index
  and ties among the others resolve in that permuted order. Harmless (any consistent admissible order is
  valid, and the measured order wins when consistent), and the prototype comparison passes, so either the
  prototype does the same or no committed case has a tie; say so, or `std::rotate` instead of swapping.
- **The second reference point is not checked against the chain's great circle** after the fallback
  (`:3068-3073`): review 2's 1.5, still as it was. Not re-reported; with 1.1's fix the guard moves to each
  input's own axis, which is the place to add the debug assertion review 2 suggested.
- **Common pairs for long arcs.** Section 5.2 says a subject single arc and a clip single arc between the
  same two vertices "run together within the thickness". Two great-circle arcs whose end points are within
  `h` of each other's are at most about `h / cos(L / 2)` apart along their length, `L` the arc's length:
  `1.4 h` at `90` degrees, `11 h` at `170`. Negligible for real segments, but the page's statement is for
  short arcs; the output takes the subject's arc along a common pair, so for a very long shared segment the
  clip's boundary is in effect moved by that much.
- `trusted_flipped` counts a piece whose method was `METHOD_VERTEX_TEST` at the time of the flip
  (`:3289-3301`); a vertex-tested class flipped once and flipped back in a later round is counted twice
  but ends as it began. Diagnostic only.

## 2. Test gaps

Mutations (*run*; each on a restored `PolygonOverlay.cc`, against the 93 tests of
`PolygonOverlay*:PolygonPartitioner*:PartitionFeature*`):

| Mutation | Caught by |
| --- | --- |
| M2: stalled single arcs tested smallest margin first (`:2338-2364`) | `PolygonOverlayPrototypeTest.OnePassageRule`, `.BundleRule`; and `PartitionFeatureUtilsTest.AssignPlateIdsSkipsInconsistentFeature` *stops* throwing (the known inconsistent case becomes consistent). **No hand-built `PolygonOverlayTest` case notices.** |
| M3: spike rule without the other-input guard (`:986`) | `PolygonOverlayPrototypeTest.Hunt`, `.Bundle`, `.BundleRule` |
| M4: one-passage rule off (`one_passage_side` returns none) | `PolygonOverlayPrototypeTest.Hunt` (`chain#28 seed 2`), `.OnePassageRule`, `.BundleRule`; `AssignPlateIdsSkipsInconsistentFeature` stops throwing |
| **M5: LL rule off** (`is_crossing_record` forced false, `:1537-1539`) | **Only `AssignPlateIdsSkipsInconsistentFeature`, and only because the inconsistent fixture then happens to be consistent.** No test asserts the rule's effect. |
| M6: no self-touch cuts | `PolygonOverlayTest.ShortSegmentSelfTouch`, `.SelfTouchOnSharedEdge`, five prototype suites |
| M7: cone without the far end's offset (`:1289`) | `PolygonOverlayTest.RaysFarEndOffset`, `PolygonOverlayPrototypeTest.BundleRule` |
| M8: repair ranks the vertex test least reliable | `PolygonOverlayTest.RepairSingleFlip`, `.RepairWholeChain` |
| M9: one-passage `m_in` of a common ray inverted (`:1721-1723`) | `PolygonOverlayTest.ShortSegmentSelfTouch`, five prototype suites |

Gaps:

- **The LL rule has no test of its own** (M5). *Run*, probe `ProbeOctagramLLRule`: two squares of
  radius 0.01 rotated 45 degrees to each other (eight LL records, every piece between crossings a single
  arc). Unmutated: `vertex_tests = 8`, `deduced = 8`, `midpoint_tests = 0`, oracle clean. With M5:
  `deduced = 7`, `midpoint_tests = 1`. A committed octagram asserting `midpoint_tests == 0` pins the rule
  (the existing `ClassifyTwoSquares` asserts the same count but its pieces all have interior vertices, so
  the rule is not needed there).
- **The margin order is pinned only by the prototype comparison** (M2). A hand-built case where
  smallest-first deduces a long piece wrongly (the page's neck of `Q`, 9 of 300 cases) would make the
  rule's purpose visible in the unit suite; the design calls it the principle of step 3.
- **The anchorless chain's offset case** (1.1): `ClassifyAnchorless` covers identical, reversed and
  jittered rings with long segments only. Add the needle with a `2 h` clip segment and `a != 0`.
- **The second reference point** is still not exercised (review 2's 1.5; the fallback at `:3068-3073`).
- **The self-touch cut's double-stop case** (1.2) is not constructed; a test for it would need the
  self-touch-plus-two-clip-vertices input described there.
- Nothing asserts the one-passage rule's *mirror* branch; it cannot fire (1.3), so a test cannot be
  written for it, which is one more reason to remove it.

## 3. Checked and found correct

- **Vertices** (`build_vertices`, `:461-582`): union-find keyed by (side, global segment), roots kept
  lowest, groups emitted in order of lowest record, the output position's priority (subject vertex, clip
  vertex, first record's position) as the page says; `ring_of_segment` handles a polyline's fictitious last
  segment.
- **Stops and pieces** (`:585-857`): the assertion that every record at ring vertex `k` is one overlay
  vertex holds by the key; inside-segment records keep `GeometryIntersect`'s order; a one-record ring's
  piece goes round to itself with `num_ring_stops - 1` interior stops; `single_arc_segment` is set exactly
  when the piece has no interior stop and both stops lie in one segment, which (since a ring vertex in a
  record is a stop) is every piece with no interior original vertex; a polyline with no record is one
  closed piece whose dangling ends are its test points.
- **Loop and spike rules** (`:909-1009`): the loop rule needs `start == end` and no interior vertex;
  the spike rule's `other_ends > 2` guard counts the other input's non-internal ends at the tip, as the
  page states; a polyline's last and first pieces do not pair (review 1's 1.3 holds); internal ends are
  removed from every vertex afterwards, so `ends` is even per input.
- **Common pairs** (`:1012-1073`): exactly one of each side between two distinct vertices, else counted
  `common_ambiguous`; deterministic by first-seen key order.
- **Self-touch cuts** (`:1076-1209`) apart from 1.2: the segment's plane in doubles for a zero-length
  segment, the strict `along` ordering with the `1e-12` margin, the thickness test, `MAX_CHAIN`, "one
  piece would have paired", the single rebuild. Review 3's 1.2 fix holds (M6 is caught by its tests).
- **Rays** (`measure_ray`, `:1212-1315`; `build_rays`, `:1392-1475`): the exact tangent from the arc's own
  start, negated when arriving (the arc between ring-order stops `i-1` and `i` is on stop `i`'s arriving
  segment); `delta` from the output position by exact coordinate comparison plus the offsets of a touch
  stop and a self-touch cut only (a ring vertex and a crossing give 0; review 8's 1.2 fix holds); the cone
  `0 / pi / atan2`; a common ray takes the subject's direction, the clip's when the subject's is missing,
  and the wider cone with its quantities (a subject arc on a zero-length segment then keeps a cone of
  `pi`, so the common ray floats: conservative, and what the page says).
- **Rotational order** (`:1318-1389`): two sign tests, ties by dot product, insertion after every ray not
  preceding, rays without direction last.
- **The free rule** (`:1505-1597`): applied only at four non-common rays, two per input; at a one-record
  LL vertex each input's classes must differ and a known class fixes its own input's other class only; at
  a touch, three known fix the fourth and four known are checked; a piece whose both ends are at the vertex
  is handled (a touch is then consistent; a crossing with one ring crossing once is a check failure, as a
  missed intersection should be).
- **The one-passage rule** (`:1677-1809`): `one_passage_side` requires six or more ends with exactly two of
  one input; `others` are never common; `m_in` of a common ray is same-side; one unknown deduced, all
  known checked; the built order keeps the inside rays' measured relative order. M4 and M9 are caught.
- **The sector walk** (`:1811-2017`): constraints on both sectors beside a ray; four starts per order; no
  solution is a failure; deductions only where every solution of every order agrees; the Kendall choice
  linearised from ray 0; memberships reported where the chosen order's solutions agree.
- **The bundle rule** (`:2683-2932`): certainty order (cone, then reach, stable), separability against
  every anchored ray by the sum of cones, no direction or `pi` never anchors, the cap, the no-anchor
  exit, greedy interval placement with the three shifts, lexicographic permutations per gap, the measured
  order at index 0. Built only at six or more ends where neither input passes once, before any class.
- **Classification order** (`classify_pieces`, `:2264-2438`): vertex tests on the first interior vertex
  (all in debug); deduction to a fixed point; stalled arcs by largest chord-midpoint distance to the other
  outline (`minimum_distance` to the outline, interior not solid), margins computed once, so at each
  midpoint test every remaining unclassified non-common piece has a margin no larger than the tested one,
  which is what confines a band class's deductions to pieces of smaller margin; the anchorless loop skips
  classified arcs and sets aside undirected chains (review 2's 1.3 and 1.4 hold); a common clip piece
  takes its partner's class.
- **Checks and repair** (`:2441-2558`, `:3139-3317`): every vertex solved before and after repair, the
  counters reset per pass (review 2's 1.6 holds), unresolved pieces counted once per pair, the throw with
  the diagnostics, position, latitude, longitude and record kinds; repair's rank, shortest first, whole
  chains before single pieces of their rank, flips undone on failure, the cap; nothing is both uncounted
  and unthrown. `get_result_from_classes` treats a clip piece inside the subject as keeping both outputs.
- **Polylines** (`classify_polyline_pieces`, `:2091-2261`): along pieces by `polyline_along_is_inside`;
  tests at interior vertices then dangling ends; deduction only across a one-record LL vertex with exactly
  two non-common polyline pieces; `ll_disagreements` counted once per vertex; midpoint tests by margin;
  no unclassified piece can remain, so `classify()`'s result is well defined. No sector walk, repair or
  throw, as the page says.
- **Deterministic ties**: records tied along a segment (either order, a zero-length arc deduced), equal
  margins (first in piece order), equal cones (measured order), equal anchorless scores (first in chain
  order, which is piece 0, then the ring's last piece, then ring order).
- **Code style** in the files read: tabs, Allman braces, broken signatures, `&name`, nothing over the
  neighbours' width.

## 4. Page statements to amend

- **5.2, Anchorless chains**: "The arc from `R` meets the excluded great circle only at `X`, so the parity
  is that of the points just before `X` on `R`'s side" is true for the subject only; for the clip, `X`
  (the subject's chord midpoint) lies off the clip partner's great circle by up to the thickness, and the
  arc meets that great circle at a point displaced along the chain by `d / tan(theta)`, which can fall
  outside a short clip segment (1.1). "Nothing is offset, so no thin face can defeat it" does not hold
  for the clip side as built. After the fix, state that each input's parity is counted to the projection
  of `X` onto its own segment's great circle, and that the flip compares the reference points' sides
  against each input's own plane.
- **5.2, One-passage rule**: "or its mirror if the walk finds no placement on the first" describes a
  branch that cannot run (1.3); either drop the mirror from code and page or say that it is a safety net
  that is not expected to fire.
- **5.2, Bundle rule**: "ties by enumeration order" is, after the swap that puts the measured order first,
  enumeration order with the first enumerated order moved to the measured one's position (1.3).
- **5.2, Common pairs**: "run together within the thickness" holds for arcs that are short compared with
  the sphere; for arcs of length `L` the bound is about `h / cos(L / 2)` (1.3).
- **5.1 / 5.2, the hemisphere check** ("throws for every operation") and **5.6**: a whole-in or whole-out
  result returns from `overlay_polygon` without any `extract`, so the hemisphere check runs only for
  `INTERSECTING`. The page should say that a wrong *whole* answer (every piece classified, every vertex
  consistent, as 1.1 produces) is outside the guarantee of 5.6 as the code stands, or `overlay_polygon`
  should run the check for whole answers too (one point test, no ring building).
- **5.2, Self-touch cuts**: "a stop of the vertex that no record of the ring names", placed after the
  vertex's stop on the segment: add that the placement assumes the vertex has one stop on that segment
  (1.2), or fix the keying and say nothing.
- **PointInPolygon.h:115-120**: the sentence "For a `to_point` on (or within the thickness of) the
  excluded segment, this is the parity at the points just before `to_point`" should add the condition
  that the crossing arc's meeting with the segment's great circle lies within the segment, which a
  `to_point` within the thickness but off the great circle does not guarantee near a short segment's end.
