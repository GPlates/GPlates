# Phase 5 review 1: steps 1 to 3 of the overlay core

Reviewed 2026-11-04 (Fable), the diff `2f4d072b2..da06e8537`: `src/maths/PolygonOverlay.{h,cc}`,
`src/unit-test/PolygonOverlayTest.cc`, `src/unit-test/PolygonOverlayPrototypeTest.cc` and the exported
catalogue `src/unit-test/data/polygon-overlay/prototype-catalogue.txt`, against `phase4b-design.md`
sections 2 to 3.7 and the prototype `overlay_proto.py` under `CONFIGS['bnd']`. Line numbers are at
`da06e8537`. Evidence is marked *run* (a build of `gplates-unit-test` in the Release tree, with
temporary probes since removed; `git status` is clean) or *reading*.

Baseline: all 23 `PolygonOverlay*` tests pass; the prototype comparison reports `463 prototype cases,
0 differ; 39 band pieces have the other class`.

## 1. Findings, most severe first

### 1.1 The prototype comparison never checks a common pair's class

`src/unit-test/PolygonOverlayPrototypeTest.cc:397-414`. The band exemption applies to every non-closed
piece without an interior original vertex whose midpoint is within `2h` of the other polygon's
outline. A common pair's two arcs lie within `h` of each other by construction, so every common piece
qualifies, and any `same`/`opposite` disagreement with the prototype is counted as a band difference
and accepted. The design (11.2, item 3) exempts only midpoint-tested band pieces and names "a sector
class" as a failure.

Scenario: a sector-walk defect that classified every common pair `same` would pass the comparison.
*Run*: with the overlay's class of every common piece replaced by the string `WRONG` before the
comparison, the test still passes, `463 prototype cases, 0 differ; 317 band pieces have the other
class` (278 of the 317 are common pieces). The catalogue holds 312 prototype common pieces (118 `same`,
194 `opposite`) that are therefore not verified by this test. The actual classes do agree (the
probe's first form printed every band difference with its `common` flag: none was common), so this
is a test defect, not a code defect, but it is the one check the design relies on for step 3's sector
walk, and it also hides step 5's gap: an unresolved anchorless chain (class `none`) would pass against
the prototype's exact class.

Fix: exclude `piece.common` from the exemption. Also restrict the exemption to cases in which a
midpoint test happened on either side (`overlay.get_diagnostics().midpoint_tests > 0`, or a prototype
piece with method `midpoint` in the matched order): the only band differences seen (section 1.2) are
a midpoint test and the deductions made from it, so in a case with no midpoint test every class is
deterministic and a difference is a defect. With that rule the `d4 lone` family, whose short subject
segment between two touches is deduced by the touch rule and lies in the band, is compared strictly.
And assert `check_failures == 0` and `unresolved_pieces == 0` whenever the matched prototype order
had neither; today neither diagnostic is read by the test (*run*: both are zero in all 463 cases).

### 1.2 What the 39 accepted band differences are (no defect)

*Run*, printed per piece: all 39 are in the thirteen `d4 ... hole-between` cases, three per case: the
clip's two single arcs between the subject's feet and the hole's apex (`Q0` pieces 7 and 8, margins
`0.02h` to `0.13h`) and the subject's short segment (`P0` piece 0, margin 0). In each case one piece
is midpoint-tested and the other two deduced from it, and the C++ and the prototype differ on the
midpoint test's answer, or on which piece is tested first (the two clip arcs have the same rounded
margin in the C++, `minimum_distance` resolving about `0.01h`, so the first in piece order is taken;
the prototype's exact margins order them the other way). Methods agree in every other piece of every
case (*run*: a method comparison printed only these 22 midpoint/deduced swaps). This is the behaviour
design 3.7(c) describes; nothing to fix, but it is why the restriction in 1.1 is safe.

### 1.3 The spike rule wraps round on a polyline

`src/maths/PolygonOverlay.cc:757-760`: `b` is piece `(i + 1) % num_ring_pieces`, so for an open ring
the last piece and the first are treated as consecutive. A polyline whose first and last vertices are
records of one overlay vertex and whose only other record is a touch elsewhere is `v0 -> v1 -> v0`:
both its pieces become internal and the polyline has no non-internal piece left. *Run*: subject
polyline `(-0.01 + 0.3h, 0.01), (0, 0.01 + 0.3h), (-0.01, 0.01 - 0.3h)` against the square
`(-0.01, -0.01)..(0.01, 0.01)` gives records `SL SS SS`, `internal_pieces = 2`, `spikes = 1`, every
subject piece internal. The prototype does the same (`PolylineOverlay` inherits `_contract`,
`overlay_polyline.py:107`), so the translation is faithful, but design 3.4 speaks of "two consecutive
pieces of one ring" and 6.1's output rule gives an internal polyline piece "the class of the piece
before it (or after it)", which this one does not have. Polylines are step 9; recording it here so the
step decides: skip the wrap (`i + 1 == num_ring_pieces` on an open ring) or define the output of a
polyline whose every piece is internal. Low severity: a hairpin lying along the clip within `h`.

### 1.4 A branch that cannot run: a second overlay vertex at one ring vertex

`src/maths/PolygonOverlay.cc:480-502` (and design 3.3 item 1, "rare but allowed"). Every record at
ring vertex `k` of side `S` names key `(S, k)` in `build_vertices` (`:316-348`), so all such records
are one group and a ring vertex holds exactly one overlay vertex. The `else` path that starts a second
stop with `segment_in = k` is unreachable, and its choice of `segment_in` (which differs from the
prototype's `(k - 1) % n`) cannot be tested. *Reading*. Suggest replacing the branch by an assertion
that `d_stops.back().vertex == vertex` after the first record, and correcting the design text; or keep
it with a comment saying why it is unreachable under identity.

### 1.5 The result is computed from kept pieces, not from the output

`src/maths/PolygonOverlay.cc:1626-1674`. Design 5.3 defines `RESULT_INSIDE`/`OUTSIDE` by the emptiness
of the difference/intersection *after clean-up*. Step 3 has no linking, so the piece-level rule is the
right interim, and it gives the contract's answers for the cases probed (*run*: a touch from inside
`RESULT_INSIDE`, from outside `RESULT_OUTSIDE`; identical polygons `RESULT_INTERSECTING` with
`unresolved_pieces = 8`, which is step 5's anchorless chain). Two things for the step that adds
`extract()`: an unclassified piece sets both flags (so a failure reads as `INTERSECTING`, never a
silent whole-geometry answer, which is right), and a kept piece whose ring clean-up collapses would
still make the result `INTERSECTING` here. Also not yet implemented, and worth a note in the plan so it
is not forgotten: 3.1's "no records and disjoint bounding circles is `RESULT_OUTSIDE` at once" and
4.1's bounding-circle skip before the closed-ring vertex tests (the cascade's cost argument depends on
them).

### 1.6 Minor code observations (reading)

- `Diagnostics::unresolved_pieces` (`:1609-1615`) counts a common clip piece as well as its unresolved
  subject partner, so an anchorless chain of `n` arcs reports `2n`. The prototype's `anchorless_common`
  counts the subject side only. Cosmetic; the tests assert zero.
- `deduce_to_fixed_point` (`:1445-1452`) can receive two deductions for one piece from one vertex (a
  piece with both ends there, such as the wedge's tip) and applies the first silently; a disagreement
  is then a check failure at that vertex. Same as the prototype; a comment would save the next reader
  the analysis.
- `sort_rays_anticlockwise` (`:998-1001`) takes the first ray *with a direction* as the reference,
  where the prototype uses `rays[0]`'s exact (possibly degenerate) direction. They can differ only when
  a ray has no direction, which the catalogue produces only at four-end vertices (`d4 len=+0.5h
  hole-between`, `zero_length_rays = 2`), where the order is not consulted. Justified.
- The zero-length decision in `measure_ray` (`:893-902`) is per *segment* (`GreatCircleArc::is_zero_length`,
  about `0.7h`), as in the prototype's `_direction`, not per arc of the piece as design 3.6 words it.
  A zero-length piece on a non-zero-length segment (the twist's two crossings at one point) therefore
  gets the segment's tangent, `delta = 0`, `cone = 0`. That direction is the ring's true direction
  through the point, so it is the better choice; the header's wording (`PolygonOverlay.h:239-243`) is
  the accurate one and the design should follow it.
- `is_same_position` instead of `PointOnSphere::operator==` (`:135-145`) is necessary, not merely
  justified: `operator==` compares dot products to `1e-12`, about the thickness, so a clip stop within
  `h` of the subject's vertex would get `delta = 0` and every cone would collapse to zero.
- Public plain-data classes in the header (`Stop`, `Piece`, `Vertex`, `Ray`, `Options`...) have
  unprefixed members where AGENTS.md asks for `d_`; the `Overlay` class itself follows the rule. The
  one clear style deviation; the design's own sketches used `d_`.

## 2. Test gaps

1. **Common classes** (1.1): the comparison's one loophole; fix first.
2. **Prototype orders needing repair** (`PolygonOverlayPrototypeTest.cc:334-337`, `:432-436`): an order
   with repairs or check failures is skipped, and a case in which every order needed repair returns a
   match with nothing compared. The committed data has none (`order 0 0` in all 595 orders), but a
   future export would pass silently. Report such a case (`ADD_FAILURE`, or at least a count printed
   with the summary), and when step 6 lands, require the C++ `check_failures > 0` for it.
3. **The touch rule's deduction** (three known classes fix the fourth; `solve_four_end_vertex`
   `:1177-1207`) is pinned by no unit test. `ClassifyApexOnEdge` has one `LS` but all three pieces
   there are vertex-tested (`deduced = 0`); `ClassifyHoleCrossedOnly` pins the `LL` rule only. In the
   catalogue it is exercised by the `d4 lone` subject segment, which the band rule exempts (1.1).
   Add a case: the subject's vertex on the clip's edge with a single arc from it to a crossing, and
   assert `deduced == 1` and the class.
4. **The margin order** is asserted nowhere: `ClassifyNeck` (`PolygonOverlayTest.cc:1046-1064`)
   checks only the result and no check failure. Assert that the clip's far-side pieces are
   `METHOD_VERTEX_TEST`, that the midpoint tests are confined to the neck's arcs, and (for one width)
   which arc is tested first.
5. **Rays without a direction** (design 11.1's "zero-length segments" family) have no unit test; the
   `ss zero-length` catalogue cases produce none (*run*: `zero_length_rays = 0`; the segment is a loop,
   or a common pair that takes the clip's direction). Add a subject segment under `0.7h` between two
   distinct vertices (the `d4 lone` geometry at `0.5h`), asserting `zero_length_rays == 2`, that those
   rays sort last, and that classification succeeds.
6. **The result contract** (design 11.1's row): a touch from inside (`RESULT_INSIDE`) and from outside
   (`RESULT_OUTSIDE`) are not in the unit tests; the code gives the right answers (*run*, 1.5). Identical
   polygons wait for step 5.
7. **`piece_side_disagreements`** is computed only under `GPLATES_DEBUG`
   (`PolygonOverlay.cc:1527-1536`), so `classify_and_check`'s `EXPECT_EQ(..., 0u)` is vacuous in a
   Release tree. CI's Debug build covers it; a comment at the expectation would stop a reader taking
   the Release pass as evidence.
8. **Piece matching by key** in the comparison (`:339-354`) is greedy in list order; two pieces of one
   ring with the same key (a ring with one record stop: the wedge's tip `x, apex, x` and the rest `x,
   x`) could be matched crosswise and report a false *failure*. Cannot produce a false pass. Match by
   (key, point count) or by first interior stop to make it deterministic.
9. `check_structure` asserts even ends per vertex for the clip only (`PolygonOverlayTest.cc:354`); for
   a polygon subject it could assert the subject too.
10. `Stop::ratio` is never asserted (it feeds provenance, step 10); fine for now.

## 3. Checked and found correct

Each item was traced against the prototype function the design's table names, by reading, and the
catalogue comparison was run with every piece's method printed (methods agree in all 463 cases except
the midpoint/deduced swaps of 1.2).

- **Vertices** (`build_vertices` against `_build_vertices`, `_set_position`): union-find keyed by
  (side, global segment index) for `SL`/`SS` and `LS`/`SS`; `LL` singletons; the lower record index is
  kept as root, so visiting records in index order meets each root first and the vertices come out in
  order of lowest record; `subject_vertex`/`clip_vertex` are the first named in record order; the output
  position is the subject's vertex, else the clip's (*run*: an `LS`-only vertex has the clip apex's
  exact coordinates), else the record's. A polyline's last vertex is keyed through the fictitious
  segment `num_segments` and `ring_of_segment` maps it (`num_vertices()`).
- **Stops and pieces** (`build_stops_and_pieces` against `_build_pieces`): records taken from the
  ordered sequence so the order within a segment is the primitive's; at-vertex records (`SL`/`SS` on
  the subject, `LS`/`SS` on the clip) make one `RECORD` stop at the vertex, else `ORIGINAL_VERTEX` or
  `DANGLING_END`; inside-segment records follow with `segment_in == segment_out == k` and a projected,
  clamped ratio; the fictitious segment is asserted to hold no inside record. Pieces between
  consecutive boundary stops cyclically, the one-record-stop ring's piece going all the way round
  (`num_interior_stops = num_ring_stops - 1`), a ring with no record one closed piece, a polyline with
  no record one closed piece between its dangling ends and otherwise no wrap-around; dangling ends
  register nothing. `has_interior_vertex` counts `ORIGINAL_VERTEX` stops strictly inside;
  `single_arc_segment` equals the prototype's `seg` wherever the prototype sets it (the one case where
  the C++ condition `segment_out == segment_in` could fail is the unreachable branch of 1.4).
- **Loop and spike rules** (`apply_loop_and_spike_rules` against `_contract` under V1b): a single arc
  from a vertex to itself is internal, a loop with an interior vertex is not; consecutive single arcs
  `A -> B -> A` of one ring with `A != B` are both internal, pieces with a dangling end skipped; ends of
  internal pieces removed from every vertex. Pieces are stored ring by ring in ring order, which the
  ring-grouping loop relies on.
- **Common pairs** (`find_common_pairs` against `_find_common`): the same eligibility (not closed, not
  internal, no interior stop, distinct end vertices), keyed by the unordered vertex pair, paired only
  for exactly one subject and one clip piece, any other mix with both sides counted ambiguous.
- **Rays** (`measure_ray`, `build_rays` against `_direction`, `_build_rays`, `_ray_cone`): direction
  `cross(axis, start)` at the stop's own position, negated for an arriving end, from the first arc
  whose segment is not zero length; `delta` exactly zero when the arc's start is the vertex's stored
  position, `L`, `reach`, and the cone `0 / pi / atan2(delta, L - delta)`; the clip end of a common
  pair joins the subject's ray, the partner's end at this vertex found by `start_vertex != v` (valid
  because a common pair has distinct end vertices), the subject's direction unless its segment is zero
  length and the clip's is not, the wider cone's quantities kept, the subject winning ties as in the
  prototype's `theta > best`. The end order feeding the sort is the prototype's (pieces in order,
  subject rings then clip, start before end).
- **The order** (`sort_rays_anticlockwise` against `_angular_sort_float`): the same two sign tests on
  normalised doubles, the same tie-break by dot product, the same insertion ("after every ray it does
  not precede"); rays with no direction last.
- **`solve_four_end_vertex`** against `_is_four_end`, `_is_ll_vertex`, `_solve_free` under `freex`:
  four rays, two per side, none common; the `LL` case (one `SEGMENTS_CROSS` record) fixes each side's
  other class from its one known class and fails on two equal known classes, saying nothing across
  sides; the touch case compares "crosses" per side, deduces the fourth from three, fails on four that
  disagree, and deduces nothing from fewer. The degenerate case where a side's two rays are one piece
  behaves as the prototype's (that side "does not cross").
- **The sector walk** (`solve_vertex` against `_walk` and `_solve_vertex`): constraints `IN_CLIP` from
  a subject ray, `IN_SUBJECT` from a clip ray, `XOR` from a common ray read off the *subject* piece;
  prefix parities from ray 1; sector `i` after ray `i`, a constraint on sectors `i - 1` and `i`; the
  four starts; no solution is inconsistent; a deduction only when every solution agrees, `METHOD_SECTOR`
  for a common piece, `METHOD_DEDUCED` otherwise; reported memberships from the chosen order where its
  solutions agree, `none` otherwise. At three rays (a chain end) the two cyclic orders give the same
  deduction, so the measured order being unused below six ends holds there too.
- **`classify()`** against `classify`: (a) vertex tests at the first interior original vertex in ring
  order (a closed piece from the ring's first stop; the one-record-stop piece from the stop after it),
  threshold off, the clip at `Options::clip_speed` and the subject at `ADAPTIVE`; (b) the fixed point
  with deductions applied vertex by vertex as found; (c) one midpoint test on the stalled piece of
  largest margin (`minimum_distance` to the outline, not solid; ties to the first in piece order, the
  prototype's stable sort), then back to (b); (e) the clip partner's class; then the check at every
  vertex. Anchorless chains and repair are absent as the step intends, and the result stays
  `INTERSECTING` with `unresolved_pieces > 0` rather than a wrong whole-geometry answer.
- **Robustness**: no out-of-range access found: `segments[k]` is contiguous (`std::vector<GreatCircleArc>`
  per ring); `vertex(k)` for `k == num_segments` only on a polyline; `side_pieces[s][0..1]` guarded by
  the caller's count; `std::find` in the deductions always succeeds; `vertex_stops.front()` is
  non-empty whenever `has_interior_vertex`; every optional dereference is guarded by the condition that
  sets it; `classify()` asserts a polygon subject; the margin loop and the fixed point each make
  progress per iteration.
- **Structure of the comparison** apart from 1.1 and the gaps above: records matched by (kind,
  index1, index2) with the count checked, every tie order tried, pieces by (polygon, ring, start
  records, end records) with `internal` and `common` compared before the class.
- Unit tests: the structural invariants in `check_structure` are sound, and each family test asserts
  what its comment says (the acute wedge's ray cycle `P Q Q P Q Q` with adjacent clip rays of one
  piece, the shared edge's cones with and without offset, the spike, the loop, the polyline's dangling
  ends and record at its last vertex, the T-intersection's `out`/`out`).

## 4. Session check and what was done (Opus, 2026-11-04)

Each finding was checked against the code before acting on it.

- **1.1, confirmed and fixed.** A common piece has no interior vertex, is not closed and lies on the
  other outline, so the exemption covered every one. The comparison now never exempts a common piece,
  exempts a band piece only in a case where a midpoint test was made (by either side), and fails a
  case with a check failure, an unresolved piece, or no tie order free of repairs. Still `0 differ` and
  the same 39 band pieces (all non-common, all in `d4 hole-between`); the review's mutation (every
  common class reported as `WRONG`) now fails 108 cases.
- **1.3, fixed now rather than at step 9.** The spike rule no longer wraps round a polyline (a
  polyline's last and first pieces are not consecutive).
- **1.4, confirmed.** The branch is replaced by one stop and an assertion; design 3.3 is corrected.
- **1.5** stands as noted: the result from kept pieces is step 3's interim; step 11 brings 5.3's
  contract after clean-up, and 3.1/4.1's bounding-circle shortcuts.
- **1.6:** `unresolved_pieces` now counts a common pair once. The rest are left as they are: two
  deductions of one piece at one vertex follow the prototype (the check catches a conflict); the
  first ray with a direction as the sort's reference matters only with zero-length segments below six
  ends; the plain-data members without `d_` follow `GeometryIntersect::Intersection`'s style. Design
  3.6's "zero-length arc" means an arc on a zero-length segment, as the header says.
- **Section 2's gaps:** new unit tests for the touch rule's deduction (`ClassifyTouchRuleDeduction`),
  rays with no direction (`RaysWithoutDirection`, the d4 0.5h hole-between geometry) and the result for
  touches from either side (`ClassifyResultOfTouches`). The margin order and the vacuous Release-tree
  `piece_side_disagreements` remain covered only by the comparison and by a Debug run; identical
  polygons (anchorless) wait for step 5.

