# PR B review 2: the overlay's extraction and provenance

Reviewed 2026-11-08 (Fable), at `d958f07be` (the whole of PR B, `public/main...HEAD`, seen as one):
`src/maths/PolygonOverlay.{h,cc}` after `classify()`: `is_in_output`, `is_kept`, `extract_rings` (the
pairing, the walk, the split, clean-up, the per-ring provenance fill), `extract` (the hemisphere check,
grouping, orientation, the sliver filter), `extract_polylines`, `is_ring_inside_ring`, `get_mean_width`,
`get_vertex_provenance`, `get_original_vertex_provenance`, `get_polyline_stop_provenance`,
`compose_subject_location`, the linking, grouping, sliver and provenance counters of `Diagnostics`; with
`apply_loop_and_spike_rules` where it feeds the polyline output, and `overlay_polygon` /
`overlay_polyline` in `PolygonPartitioner.cc` where they consume the results; against
`docs/design/architecture/foundation/spherical-geometry.md` sections 5.2 (from "Linking"), 5.4, 5.5,
5.6 and 3.1 to 3.2 (the interior definition the hemisphere check and the grouping rest on), the step
reviews 3, 4 and 8 for what was found and fixed before, and review 1 of PR B. Line numbers are at
`d958f07be`.

Evidence is marked *run* or *reading*. *Run*: six temporary probe tests appended to
`src/unit-test/PolygonOverlayTest.cc` (a ring through one vertex three times, thin slivers, bands round
the globe, a polyline hairpin, a polyline ending on the clip, a two-step cascade with an identity group
holding two subject vertices), and eleven mutations of `PolygonOverlay.cc`, each applied to a freshly
restored file, rebuilt and run against `PolygonOverlay*:PolygonPartitioner*:PartitionFeature*` (97 tests
with the probes). All removed afterwards by `git checkout` and a touch of both files; the unit-test target
rebuilt; the overlay suites green (82 tests, 80 passed, 2 skipped by design) and the full suite green
(`ctest`: 203 of 203); `git status` clean but for this file. Probe sources, the mutation list and the
scripts were kept in the session's scratchpad.

Baseline (*run*, before any change): `gplates-unit-test --gtest_filter=PolygonOverlay*:PolygonPartitioner*`
82 tests, 80 passed, 2 skipped (`PolygonOverlayPrototypeTest.File`, `PolygonOverlayRealDataTest.Coastlines`).

One silent wrong answer was found, for a polyline subject (1.1); one place where a failure is counted but
not thrown (1.2); the rest is page wording and test gaps. The fixes of reviews 3, 4 and 8 that fall in
this area were checked and hold (section 3).

## 1. Findings, most severe first

### 1.1 The spike rule makes a polyline's hairpin internal, so a polyline that runs from one clip vertex to another and straight back is classified by its neighbours, and a hairpin across the clip's interior is reported wholly outside (run)

`apply_loop_and_spike_rules` (`PolygonOverlay.cc:930-996`) applies the spike rule to every ring, the
polyline subject's included: two consecutive single arcs from vertex `A` to vertex `B` and back are both
made internal. For a polygon ring that is right whatever the hairpin's length, since the hairpin bounds
no area. For a polyline it is not: the polyline's pieces are output as runs, not as region boundaries,
and `extract_polylines` (`:4137-4163`) gives an internal piece the class of the piece before it (after
it, at the start), so the hairpin takes its neighbour's class whatever side of the clip it lies on.
`classify_polyline_pieces` never tests an internal piece (`:2091-2261`, the `piece.internal` guards), and
the polyline result (`:2083`) is read from the non-internal pieces alone.

*Run*, probe `ProbePolylineSpike`, `Q` the square `[-0.01, 0.01]^2` in the test frame, `Y` its top-left
corner and `Z` its bottom-right corner:
`L = [(-0.02, 0.02), Y, Z, Y, (-0.02, 0.0)]`. Records `SS SS` (two polyline vertices at `Y`, one at `Z`;
`Y`'s two records name the clip's vertex, so they are one overlay vertex by identity, which is what the
spike rule needs). `internal_pieces = 2`, `spikes = 1`, `classify()` returns **`RESULT_OUTSIDE`**, the
output is one run, outside, of all five points, and the polyline oracle fails **208 of 300** samples:
the two diagonals `Y -> Z -> Y`, `0.028` radians each, cross the clip's interior. Through
`overlay_polyline` (`PolygonPartitioner.cc:205-209`) this is `GEOMETRY_OUTSIDE`, the polyline returned
whole and passed on to the next plate of a cascade; nothing counts it, since no check applies to a
polyline but the LL disagreement (`ll_disagreements = 0` here). With `Z` the top-right corner instead
(the hairpin along the clip's top edge) the result is the same, outside, where the design's along rule
(section 5.2, `METHOD_ALONG`) says inside; the oracle cannot see that case (it samples only beyond
`2 h` of the clip's outline), and the design's intent for it is less clear-cut (below). A polygon with
the same hairpin (`P` a square with `Y, Z, Y` inserted in its ring) is right under every operation
(`check_oracle` clean: the hairpin's intersection is a collapsed region, no ring).

The configuration needs the hairpin's two tips to be clip vertices (records that name the same clip
vertex are what make the two polyline vertices one overlay vertex); a hairpin whose tips are touches
inside the clip's segments gives two overlay vertices at each tip and no spike. So: a polyline whose
consecutive vertices are two vertices of the partitioning polygon, visited there and back. Unusual in
real data, but constructible, OGC-valid, and silent.

**Fix.** Apply the spike rule to polygon rings only (skip the subject's ring when `d_subject_polyline`;
the clip's rings bound nothing for a polyline subject either way). The hairpin's two pieces are then
single arcs with no test point and go to the midpoint tests of step (4) of `classify_polyline_pieces`:
across the clip they are tested inside, and the runs become `[outside 2][inside 3][outside 2]`. Along
the clip's edge the two polyline arcs and the clip's one arc between `Y` and `Z` are the "other mix"
`find_common_pairs` leaves unpaired (`common_ambiguous`), so both arcs become band pieces decided by
midpoint tests on the outline (arbitrary) rather than `METHOD_ALONG`; if the along rule should hold
there too, pair a polyline piece with a clip piece whenever the two run between the same vertices,
allowing several subject pieces per clip piece for a polyline (the pairing serves only classification
there, not linking). The probe's across case, with its run classes and the oracle, is the regression
test; it also gives the suite its first hand-built internal polyline piece (section 2). The loop rule
(a polyline's consecutive vertices within one identity group, an arc under `2 h`) can stay: the class
it inherits is wrong by at most the thickness.

### 1.2 A grouping failure and a coincident pair are counted but never thrown, so a hole made an exterior is returned as the answer (reading)

`extract` (`:4106-4122`): a hole whose parent is not a kept exterior is "kept as an exterior, and
counted" (`grouping_failures`), and two rings with no point off each other are counted
(`coincident_rings`, `:3999-4002`) and left as siblings. Neither consults `Options::throw_if_inconsistent`,
unlike the hemisphere check, the pairing and every check of `classify()`; section 5.6 names the
hemisphere and pairing failures as `extract`'s throws and nothing else, so the page and the code agree,
but the guarantee "never returns a best guess as if it were the answer" has this exception. A hole output
as an exterior overlaps the polygon it was a hole of: for partitioning, an inside piece that covers a
region of the outside piece as well. The page's own argument that this cannot happen ("a containment
that is not a nesting, which rings that do not cross cannot give") holds for rings that do not cross;
the rings can cross within an overlay vertex by its cluster's width (the reason for the grouping margin),
and the hunt's four coincident-ring differences (PLAN.md "Resume here") are the counter firing on
generated data. No input reaching the branch was constructed here. Fix: throw
`InconsistentOverlayException` from both branches when `throw_if_inconsistent` is set, as the hemisphere
check does, so that a caller gets an exception rather than overlapping outputs; or state in 5.6 that these
two are counted only, and why that is acceptable (a coincident pair bounds no area; a grouping failure
has not been seen outside the cluster-crossing case).

### 1.3 Minor (reading unless marked)

- **The hemisphere check does not throw for every union over a hemisphere, and should not** (*run*, probe
  `ProbeBands`): `P` a polygon whose parity interior is two caps (exterior ring a cap of 80 degrees
  about a pole, interior ring a cap of 89.5 degrees about the antipode: everything but a band of latitude
  0.5 to 10 degrees) and `Q` a cap of 85 degrees about the pole: the union is everything but the band of
  0.5 to 5 degrees, over a hemisphere, right, two output polygons, `hemisphere_check_failures = 0`, the
  oracle clean under all five operations. What the check detects is the rings' smaller sides reading as
  the complement, which a single ring round more than a hemisphere does and several rings need not; the
  header of `extract` says this exactly, but the page's "A complement means the output exceeds a
  hemisphere" invites the converse, and the test `UnionOverHemisphereThrows` is named and commented as if
  every such union threw. Likewise "the intersection and the differences lie inside an input and cannot
  [exceed a hemisphere]": a parity polygon's interior can (the two caps above, 3.65 pi), and the right
  statement is that every output ring of those operations has its structural inside within one input
  ring's smaller side, so it is the ring's own smaller side and the reading cannot be complemented. The
  same probe's two bands (rings whose structural inside is the larger side on the intuitive reading)
  group as one polygon with one hole by smaller-side containment and pass the oracle: the grouping,
  orientation and check are consistent with each other under section 3.1's definition, as section 3
  below records.
- **The spike rule's stated reason is not the true one.** "The hairpin they bound is no wider than the
  identity groups at its ends" (`:931-932`, and section 5.2) describes its width; the probe's hairpin is
  `0.028` radians long and crosses the other input. What makes the rule right for a polygon ring is that
  a hairpin bounds no area, so removing it changes no region; that reason does not extend to a polyline
  (1.1). Both places should say so.
- **`sliver_area_dropped` counts the dropped sliver exteriors only** (`:4079-4084`): not the islands and
  holes dropped with a parent (`:4073-4076`), and an exterior's area is its ring's, holes included. Only
  `overlay_polygon`'s tie-break reads it (both outputs emptied: whole on the side that lost the larger
  area, `PolygonPartitioner.cc:144-149`), where a dropped sliver with a hole is unlikely; diagnostic.
- **`extract_polylines` outputs an unclassified non-internal piece as outside** (`classes[i] ==
  CLASS_INSIDE`, `:4183`): review 4's 1.5, still as it was; `classify_polyline_pieces` leaves none
  unclassified (review 1 of PR B, section 3), so a default rather than an assertion.
- **The free rule's crossing pairing is general but only the symmetric difference reaches its four-kept
  branch** (`:3457-3477`): for the other four operations a crossing keeps exactly two ends (the
  keep-rule's table), so `kept.size() == 2` pairs them first (`:3453-3456`); the loop over the output
  sectors would overwrite `partner` if three sectors were output (the union's), which no operation reaches
  with four kept ends. Right as built; a comment would save the next reader the derivation.
- **The all-common pairing's other phase is equivalent** (*run*, mutation M10, survives): as the page
  says, either phase gives the same rings after the split, for inputs whose rings do not cross themselves.
  The argument: the arcs leaving the vertex along consecutive kept common rays return consecutively (a
  pair leaving at `c1` and returning at `c4` with `c2` between would cross the pair leaving at `c2`), so
  the wrong phase joins arcs that all return to the vertex, which the split cuts apart.
- **`get_vertex_provenance` tries the leaving stop before the arriving one** (`:4233-4243`); the reverse
  is equally valid (*run*, mutation M5 survives). Not a gap; a property.

## 2. Test gaps

Mutations (*run*; each on a restored `PolygonOverlay.cc`, against the 97 tests of
`PolygonOverlay*:PolygonPartitioner*:PartitionFeature*` with the six probes present):

| Mutation | Caught by |
| --- | --- |
| M1: the split disabled (`:3763` always `true`) | `LinkCatalogueFamilies`, `IdenticalSelfTouching`, `SelfTouchOnSharedEdge`, three prototype suites (and `ProbeThreeLobes`) |
| M3: a ring inside a dropped ring not dropped with it (`:4073`) | `SliverFilterExteriorWithItsHole`, `SliverFilterHoleWithIsland` (review 8's 1.3 fix is pinned) |
| **M4: an internal polyline piece output as inside** (`:4149`) | **`PolygonOverlayPrototypeTest.Polyline` only**; no hand test has an internal polyline piece |
| M5: provenance from the arriving stop before the leaving one (`:4235`) | nothing (equivalent, 1.3) |
| M6: the hemisphere check's comparison off (`:3943`) | `UnionOverHemisphereThrows` |
| M7: a ring's parent the shallowest container (`:4014`) | `GroupIslandsInHoles`, `SliverFilterHoleWithIsland` (and `ProbeBands`) |
| M8: holes oriented like exteriors (`:4035`) | eleven tests |
| M9: the keep-rule's two chain cases swapped (`:3337`) | fifteen tests |
| M10: the all-common pairing's other phase (`:3573`) | nothing (equivalent, 1.3) |
| M13: no ring wrap-around in the composition (`:4450`) | `ProvenanceCascade`, `CookieCutterCascade`, `RunAcrossRingStart` |
| M17: the composition's earlier-clip branch removed (`:4515`) | `ProvenanceCascade` (`provenance_subject_mismatches` rises: the branch is what makes a cut edge between two crossings a cut, not a lost subject location) |

Gaps:

- **The polyline hairpin** (1.1): no test has a polyline with an internal piece, so M4 is pinned by the
  prototype comparison alone and the spike rule's effect on a polyline by nothing. Commit the probe's
  across case (expect runs `[false 2][true 3][false 2]` and the oracle clean after the fix) and its along
  case with whatever class the fix decides.
- **A union over a hemisphere that is right** (1.3): nothing asserts that the check passes a correct
  multi-ring output exceeding a hemisphere, or that the grouping handles rings whose structural inside is
  the larger side on the intuitive reading. Commit `ProbeBands` (two bands' intersection: one polygon,
  one hole, oracle clean; the two-cap union: two polygons, no throw).
- **A vertex visited three times** by one walked ring (`ProbeThreeLobes`, *run*, right: the identical
  three-lobed flower gives `{3, 3, 3}` under the intersection and the union, `rings_split = 14` over the
  five operations, every half's provenance its own by `count_missing_subject_locations`; two flowers with
  alternating lobes give six rings of three under the union). The committed tests split at two visits
  only. Cheap to add.
- **Rings narrower than the thickness** (`ProbeThinSlivers`, *run*): two squares overlapping by `0.3 h`
  and `0.7 h` are `RESULT_OUTSIDE` (the overlap is within the thickness, as designed); by `1.5 h`, `3 h`
  and `20 h` the intersection is a four-point ring of mean width `1.50 h`, `3.00 h`, `19.97 h`, oriented
  right (`count_misoriented_rings = 0`), oracle clean. No committed test measures a sub-`10 h` output
  ring's orientation; `LinkCatalogueFamilies`' shared edges use `d >= 0`.
- **A polyline ending on the clip's outline** carries the right provenance (`ProbePolylineEndProvenance`,
  *run*: the last point is subject vertex 2, no ratio, a clip location set, both for an end inside a clip
  segment and at a clip vertex). `PolylineTouchAndEnd` checks the position only.
- **A cascade step through an identity group holding two subject vertices** (a hole's tip at the
  exterior's corner, cut by a plate whose corner is there too: `ProbeCascadeTwoSubjectVertices`, *run*)
  composes with no mismatch and no provenance failure. Fine; the mismatch path itself is pinned only by
  the prototype's second step.
- **`grouping_failures` and `coincident_rings`** (1.2) are asserted zero everywhere and reached by no
  committed input; a test of the branch would need two output rings crossing within a cluster.
- **Ring continuity** is still verified by nothing (review 3's finding 3; the comment at `:3501-3503`
  says why).

## 3. Checked and found correct

- **The keep-rule** (`is_kept`, `:3320-3345`): the four tests against `is_in_output` reproduce the
  page's table for all five operations (derived by hand for each row).
- **The pairing** (`:3389-3624`): an unclassified end or an odd count of kept ends is a failure, counted
  and thrown; two kept ends pair; the free rule's crossing pairs the subject ray of class `mQ` with the
  clip ray of class `mP` per output sector and its bounce pairs by input; ring continuity follows the
  ring to its next non-internal piece and checks that it is kept at this vertex; the sector walk (no free
  rule) pairs consecutive kept positions across output sectors starting at a ray followed by an output
  sector, and the memberships of a fully classified vertex are always known unless every ray is common
  (a non-common ray fixes one membership absolutely and the xor constraints carry it round), so
  `keep_mismatches` cannot fire with consistent classes and the all-common branch is the one unknown
  case, handled as the page says (1.3 for the phase).
- **The walk** (`:3631-3718`): deterministic from the first unused paired ray; the interior stops in the
  walked direction (reversed when leaving from a piece's end); a one-record ring's piece collects every
  other stop; the guard and the `closed` flag count a non-closing walk as a failure; a kept closed piece
  is a ring of its own.
- **The split** (`:3742-3774`): the first vertex seen twice by identity, repeated until none; the cut is
  the non-crossing re-pairing for rings produced by a non-crossing pairing (two closed curves meeting
  at one point cannot cross there), including a vertex visited three times (`ProbeThreeLobes`); the
  ordering rebuilt in walk order.
- **Clean-up** (`:3811-3853`): exact coordinate comparison, consecutive repeats and a last equal to the
  first, under three distinct positions dropped and counted, provenance kept in step.
- **Provenance after the split** (`:3786-3809`): the arriving stop is the previous vertex entry's far
  stop, cyclically, initialised from the ring's last vertex entry, so each half of a split ring reads its
  own pieces (review 4's 1.1 fix holds; `ProvenanceAtSplit` pins it). `get_vertex_provenance`
  (`:4217-4298`): a subject vertex if held (the output position's own), else a walked subject stop's
  segment with the position projected, else a cut; the clip's vertex, else a walked clip stop, else the
  first record's clip segment; `compose_subject_location` after the clip's so that an earlier clip's
  location never replaces this clip's. A self-touch cut stop on a subject ring takes the touching
  vertex's subject vertex, at whose position it is output.
- **Original-vertex and polyline stops** (`:4301-4364`): a subject ring's vertex composed, a clip
  vertex a cut; a polyline stop's clip location from its own record (review 4's 1.2 fix holds), a cut
  stop's from its vertex; a polyline's last vertex is index `num_segments` (`segment_out` of its stop,
  `:662-705`; *run*, `ProbePolylineEndProvenance`).
- **The composition** (`:4367-4549`): the table index in `get_geometry_points` order; the size
  assertion; a vertex takes its entry (and its earlier-clip location when none is set yet, which only an
  original-vertex stop of a subject ring reaches); a ratio composes along the one segment both ends lie
  on, the wrap-around only with the original polygon given (M13 caught), the subject before an earlier
  clip (M17 caught: the clip branch is what makes a cut edge between two crossing points a cut), and
  every failure counted in one of the two counters, never a wrong segment.
- **The hemisphere check** (`:3900-3963`): the first of the twelve candidates clear of both outlines by
  `10 h`; parity over the rings' smaller sides against `f` at that point; sound under section 3.1 for
  the reason in 1.3 (the rings' structural insides are their smaller sides by construction, so the parity
  is the output or its complement everywhere), counted and thrown for every operation, skipped and counted
  when no candidate is clear.
- **Grouping** (`:3972-4019`, `is_ring_inside_ring` `:192-237`): the margin `max(10 h, 2 x cluster
  width)` with the cluster width over every vertex's record pairs; the first vertex beyond the margin,
  else the farthest of vertices and edge midpoints, none when all on the other ring; parent the deepest
  container (M7 caught); parity depth; an island in a hole an exterior of its own (three deep,
  `GroupIslandsInHoles`).
- **Orientation** (`:4029-4041`): `PolygonOrientation` on the ring alone (a gnomonic projection, the
  spherical signed area when that fails), reversing points and provenances together; holes clockwise (M8
  caught); right for rings `1.5 h` wide (*run*).
- **The sliver filter** (`:4046-4091`): outermost first by container count, a ring inside a dropped ring
  dropped with it (M3 caught), an exterior by its own width, a hole alone only under an exterior parent,
  the hole polygon's fill; the hemisphere check and the grouping run on every ring before it, as the page
  says. `get_mean_width` (`:240-300`): the vector-area apex with its two fallbacks, the smaller side
  modulo `4 pi`, the perimeter by `atan2`; a ring whose vector area is under the `1e-9` threshold has
  area under about `5e-10`, which is a sliver at any length a mean-direction apex could misjudge.
- **Polyline output** (`:4126-4201`) apart from 1.1: runs of one class, each stop's own position, a
  repeated position at a join dropped, an internal piece's class from the piece before it, provenance per
  point.
- **The partition layer's use** (`overlay_polygon`, `overlay_polyline`): `extract(INTERSECTION)` then
  `extract(DIFFERENCE)` on one classification; a collapsed run dropped; the emptied-side rules.
- **Counters**: every pairing failure path (`unresolved`, odd kept, ring continuity, the walk's missing
  partner and non-closing walk, the sector walk's unknown memberships, an odd sector sequence) adds to
  `num_pairing_failures` and throws once at the end; `d_diagnostics` accumulates across extracts as the
  header says.
- **Code style** in the files read: tabs, Allman braces, broken signatures, `&name`, nothing over the
  neighbours' width.

## 4. Page statements to amend

- **5.2, Polylines**: "A polyline subject shares the structures above (the loop and spike rules skip a
  piece with a dangling end; ...)" describes the code; after 1.1's fix say that the spike rule applies to
  polygon rings only, and why (a hairpin bounds no area, but a polyline's hairpin is output as a run), and
  say what a polyline's hairpin along the clip's edge is classified as.
- **5.2, The loop rule and the spike rule**: "the hairpin is no wider than the identity groups at its
  ends" is the width, not the reason; the reason for a polygon ring is that a hairpin bounds no area, so
  it may be any length (1.3).
- **5.2, The hemisphere check**: after "A complement means the output exceeds a hemisphere (a union or a
  symmetric difference can; the intersection and the differences lie inside an input and cannot)", say
  that a union or symmetric difference over a hemisphere does *not* always complement the reading: when
  every output ring's structural inside is its smaller side (a parity output of several rings) the
  check passes and the polygons are right under section 3.1's definition (1.3, *run*); and that an
  intersection or difference cannot complement it because each of its rings' structural inside lies
  within one input ring's smaller side, not because its area is under a hemisphere (a parity interior
  can exceed one). The comment and name of `UnionOverHemisphereThrows` make the same over-claim.
- **5.6**: "`extract` throws it on a hemisphere failure and on a pairing failure" is right; add that a
  grouping failure and a coincident pair are counted only (1.2), or make them throw and leave the
  sentence.
- **5.2, Grouping**: "A hole whose parent is not a kept exterior (a containment that is not a nesting,
  which rings that do not cross cannot give)": add that rings can cross within a cluster (the page says
  so a paragraph earlier) and that this is the case the counter is for.
- **5.4, Composition**: "such points get no subject location and are counted (`provenance_subject_mismatches`
  when both ends had one, ...)" is for the induction's failures; say also that an edge whose two ends both
  have subject locations but lie on one earlier clip's segment is a cut edge (a crossing to a crossing
  along the cut), composed as a cut and not counted, which M17 shows is deliberate and needed.
- **5.5**: "counted with the dropped exteriors' area" could say the area is the exterior ring's own
  (holes not subtracted) and excludes rings dropped with a parent (1.3).
