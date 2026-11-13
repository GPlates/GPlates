# Phase 5 review 2: steps 4 to 6 of the overlay core

Reviewed 2026-11-04 (Fable), the diff `793d97fec..e9b54acac`: `src/maths/PolygonOverlay.{h,cc}`,
`src/maths/PointInPolygon.{h,cc}`, `src/unit-test/PolygonOverlayTest.cc`,
`src/unit-test/PolygonOverlayPrototypeTest.cc` and the exports `prototype-hunt.txt` and
`prototype-anchorless.txt`, against `phase4b-design.md` sections 3.4, 3.6 to 3.9, 9.1 and 9.2,
`phase4a-run4.md` sections 3 and 7, and the prototype `overlay_proto.py` under `CONFIGS['r4t']`. Line
numbers are at `e9b54acac`. Evidence is marked *run* (a build of `gplates-unit-test` in the Release
tree with temporary probes since removed, and the prototype run from its own directory; `git status`
is clean) or *reading*.

Baseline: all 33 `PolygonOverlay*` tests pass; the comparisons report `463 prototype cases, 0 differ;
39 band pieces`, `246 ..., 0 differ; 18 band pieces` and `48 ..., 0 differ; 0 band pieces`, none
skipped.

No correctness defect was found in the translation of steps 4 to 6. The findings are a test gap that
leaves the bundle rule verified by nothing committed, a hunt observation that contradicts the design's
"none seen", two robustness points on inconsistent data, and smaller matters.

## 1. Findings, most severe first

### 1.1 Nothing committed exercises the bundle rule

`src/unit-test/data/polygon-overlay/*.txt`; `PolygonOverlayPrototypeTest.cc:590-640` asserts
`bundle_vertices == 0` on its eight cases. *Run*, a probe summing the diagnostics over every committed
case: `bundle_vertices` and `both_pass_twice` are zero in all 463 catalogue, 246 hunt and 48
anchorless cases. Step 4's commit message measured the bundle rule against run 3's residue, but run 4
moved every one of those vertices to the one-passage rule (design 3.8: "run 3's residue vertices are all
one-passage ones"), so `build_bundle` (`PolygonOverlay.cc:2127-2369`) and the enumeration, admissible
test, tie-break and agreement-over-orders paths of `solve_vertex` are now reached by no committed
case, and a regression in any of them would pass the suite.

*Run*, to close the gap for this review: the prototype (`r4t`) on 3000 `shared`, `six`, `chain`,
`near`, `tinyhole`, `rays` and `zero` cases each at seed 11 and 4000 `shared` at seeds 12 to 14,
keeping cases with a bundle vertex (a floating ray). Only `shared` produces them: 18 in about 15,900
(1 in 900; none in 18,000 cases of the other six generators). Exported and run through the `File`
test: `18 prototype cases, 0 differ; 5 band pieces have the other class; 3 skipped (the prototype
fails)`, and on the 15 clean cases the C++'s per-vertex statistics equal the prototype's in every
case: floating rays (1 to 6), admissible orders (2 to 5040), consistent placements (1 to 144),
ambiguous and moved counts. That is good evidence step 4 is faithful, but it is not in the repository.

Fix: commit those cases (`prototype-bundle.txt`, from `overlay_export.py --config r4t --hunt
shared:11:107,2100,2893 --hunt shared:12:1925,2240 --hunt shared:13:366,2729,2859,3176,3890 --hunt
shared:14:1265,1373,1707,3346,3653`), and a count test in the form of `OnePassageRule` asserting
`bundle_vertices`, `bundle_floating_max`, `bundle_admissible_max`, `bundle_placements_max` and
`bundle_moved` per case (the prototype's values are in the table below), so that the enumeration's
size is pinned and not only the classes.

| Case | floating | admissible | placements | ambiguous | moved |
| --- | --- | --- | --- | --- | --- |
| shared#107 seed 11 | 3 | 20 | 4 | 1 | 2 |
| shared#2100 seed 11 | 4 | 360 | 16 | 1 | 0 |
| shared#2893 seed 11 | 3 | 24 | 1 | 0 | 1 |
| shared#1925 seed 12 | 6 | 720 | 32 | 2 | 2 |
| shared#2240 seed 12 | 6 | 5040 | 144 | 2 | 2 |
| shared#366 seed 13 | 4 | 60 | 3 | 1 | 1 |
| shared#2729 seed 13 | 1 | 3 | 1 | 0 | 0 |
| shared#2859 seed 13 | 4 | 360 | 16 | 1 | 1 |
| shared#3176 seed 13 | 4 | 48 | 2 | 1 | 1 |
| shared#3890 seed 13 | 6 | 3360 | 60 | 2 | 2 |
| shared#1265 seed 14 | 2 | 6 | 2 | 1 | 1 |
| shared#1373 seed 14 | 4 | 68 | 8 | 1 | 1 |
| shared#1707 seed 14 | 4 | 70 | 8 | 1 | 1 |
| shared#3346 seed 14 | 3 | 40 | 4 | 1 | 1 |
| shared#3653 seed 14 | 1 | 2 | 1 | 0 | 0 |

(Counts are maxima over the case's vertices; `moved` is the number of bundle vertices not using the
measured order.)

### 1.2 Three of the eighteen bundle cases are inconsistent, in the prototype and the C++ alike

`shared#6 seed 11`, `shared#1434 seed 12`, `shared#2587 seed 13`. *Run*: the prototype needs 16, 20
and 20 repairs and ends with 2 check failures in each (the comparison skips them as "the prototype
fails too"); the C++ agrees with it on the last two (20 repairs, 2 vertices still failing, so
`InconsistentOverlayException`) and repairs the first in 14 rounds (the whole-chain candidate, which
the prototype lacks, is the likely difference). Not a translation defect: the two agree. But design
9.1 says a vertex the bundle rule could not resolve was "none seen in any normal run", and here one
in six of the vertices where both inputs pass more than once leads to a failure. In `#2587` the bundle
vertex (`v2`: eight ends, two of them the wedge-tip loops `x, v, x` of each input, 3.6h and 3.7h long,
vertex-tested at apices under 2h from the other outline) has two floating rays, four admissible orders
and a single consistent placement, from which the classes at the vertex were deduced; the check then
fails at the chain end `v3` and at the crossing `v10`, and repair flips nine classes on its way to the
cap, among them deduced pieces 5629h and 7341h long (`METHOD_DEDUCED` ranks before `VERTEX_TEST`),
before giving up. The failing vertex reported by the exception is `v3` (`LS(6,4) SS(8,4)`).

For the hunt of step 15, not for this PR: these are the first bundle-rule residue cases under `r4t`,
and the single-placement deduction at `v2` is the place to look (is the true order outside the
admissible set, that is, is the cone bound too tight for a wedge tip whose apex is within the band?).
Record the three names in the plan.

### 1.3 The anchorless loop can fail to make progress on inconsistent data

`PolygonOverlay.cc:1840-1871`. Each round picks the first unclassified common subject piece,
collects its chain, classifies the chain's best arc and reruns deduction. The best arc is chosen over
the *whole* chain, classified pieces included (`:2453-2470`), and the loop's only exit is "no
unclassified common subject piece". If deduction does not carry the class from the best arc back to
`start`, the next round picks the same `start`, the same chain, the same best arc (already classified:
set again to the same value, `anchorless_resolved` incremented again) and never terminates.

When can deduction fail to reach `start`? Along a chain every vertex has only common rays, each of
which flips both memberships, so `in P xor in Q` is the same in every sector and one classified ray
deduces every other, under any order (so the bundle rule's "every consistent order agrees" holds
there too). The only block is an all-common vertex that is *inconsistent*: four or more common rays
with two classified to conflicting classes (two chains crossing at a vertex both inputs pass twice,
their classes arriving from different ends of bad data). *Reading*; not constructed. Low likelihood,
but a hang is the one failure mode section 9.2 does not allow, and the guard is one line: after
`deduce_to_fixed_point()`, if `d_pieces[*start]` is still unclassified, `break` (it is then counted
unresolved and thrown). Alternatively choose the best arc among the chain's unclassified pieces only.

### 1.4 One undirected chain stops the resolution of every later chain

`PolygonOverlay.cc:1863-1868`. When no arc of a chain has a direction (every arc on a zero-length
segment) the loop `break`s, so chains found later in piece order are not tried, although they could be
resolved. The result is a throw either way (the first chain is unresolved), so this changes only how
many pieces the exception reports as unresolved. *Reading*. A `continue` over a set of chains already
tried would be exact; or leave it and say so in the comment.

### 1.5 The second reference point is never exercised, and not re-checked

`PolygonOverlay.cc:2499-2507`. The fallback to `get_second_reference_point()` fires when the outside
point is within `1e-6` of the chain's great circle or of `X` or its antipode. *Run*, a probe printing
each firing: it never fires in the 48 committed anchorless cases (their chain arcs' great circles are
all about `0.01` rad from the outside point; a sliver ring would be needed, and a sliver triangle's
third vertex touches its own base, which breaks the common pairing). So the branch is untested, and by
*reading* it is also not held to the condition it exists for: the second reference point (a candidate
60 to 120 degrees from the outside point, `PointInPolygon.cc:2156-2195`) is not itself tested against
the chain's great circle. It cannot be within `1e-6` of `X` (it is 60 degrees from the outside point,
which is within `1e-6` of `X`'s great circle but `X` itself is on the outline, far from both), but it
can lie near the great circle with the probability of any point lying near a great circle. Suggest
asserting `|dot(reference, axis)| > min_sine` after the fallback in debug builds, so that the one
unmodelled case is a loud failure rather than a parity counted twice.

### 1.6 Repair's counters accumulate across calls, against the comment

`PolygonOverlay.cc:1890-1902`: "This pass's counters" resets `unresolved_pieces`, `check_failures`
and the per-vertex statistics, but `repairs`, `whole_chain_repairs` and `trusted_flipped` are not
reset, so a second `check_and_repair()` (which the tests make, after `set_piece_class`) reports the
sum, and the exception message's "after N repairs" is cumulative. Harmless today (the first call makes
none), but the two tests at `PolygonOverlayTest.cc:1271-1327` pass only because of that. *Reading*.
Decide which it is and say so in `Diagnostics`.

### 1.7 Minor observations (reading)

- `bundle_capped` (`PolygonOverlay.cc:2199-2207`): the "no ray anchors" exit comes before the cap,
  so a vertex with more than six floating rays and none anchored is counted neither capped nor, as
  the prototype counts it, `unanchored` (the C++ has no such counter). Diagnostic only.
- `repair()` ranks `METHOD_ANCHORLESS` with `METHOD_SECTOR` (`:2594-2596`). Design 3.9's list has no
  rank for it; it is an exact test, so `VERTEX_TEST`'s rank is as arguable. State the choice.
- The whole-chain candidate is offered only at a chain end (`:2682-2696`). When two ends deduce
  conflicting classes and the chain's interior vertices are visited before the second end (vertex
  order follows record order), the conflict surfaces at an interior all-common vertex, where only
  single flips are offered; each round then moves the failure one vertex along the chain, and at the
  end the two whole-chain flips alternate until the cap if both ends' plain classes are vertex-tested.
  This is the design's accepted limit (the net is for the flips measured, not this), but the round
  cap is then reached by chain length, not by difficulty; worth a sentence in the comment.
- A chain that starts and ends at the failing vertex is pushed as a whole-chain candidate twice (one
  per common ray); the second is tried only if the first failed, with the same outcome. Harmless.
- `PointInPolygon.h:109-130` says the numbering is `get_segment()`'s; true (`PolygonOnSphere.h:830-845`
  walks the exterior then the interiors), and `RingView::first_segment_index` is built the same way
  (`PolygonOverlay.cc:226-247`).
- Code style: nothing found in the diff (no line over 100 columns, pointers and references bound to
  the name, Allman braces, hard tabs). The plain-data members without `d_` are review 1's known
  choice.

## 2. Test gaps

1. **The bundle rule** (1.1): commit the fifteen clean cases and pin their statistics. Also add one
   unit test that builds a both-pass-twice vertex by hand (two pinched rings of each input through one
   point, the clip's pinch offset within `h` so that its two rays float) and asserts `anchored`,
   `floating` and `orders.size()`, so that `build_bundle` is pinned independently of the export.
2. **The second reference point** (1.5): no case reaches it.
3. **The orders built for linking** are not in the export (`overlay_export.py` writes classes and
   methods only), so the one-passage rule's built order and the bundle's chosen order are verified
   only by the `moved` counts of `OnePassageRule` (eight cases) and of 1.1's table. Step 7's output
   comparison covers them indirectly; until then a difference in the order used would be invisible.
   Either extend the export with each vertex's order (ray indices as piece ends) or note the
   dependency on step 7 in the plan.
4. **The spike restriction** (`PolygonOverlay.cc:785-802`) has no unit test for the spike it *keeps*:
   `SpikeRule` asserts `spikes == 1` for the removed one. `chain#28 seed 2` is in the hunt file and
   exercises it against the prototype; a direct test (a hairpin of `P` at a self-touch of `Q`, asserting
   `spikes == 0` and the four ends kept) would say what the rule is for.
5. **The common-class exemption** (`PolygonOverlayPrototypeTest.cc:465-470`) was measured: with every
   common class inverted before the comparison, 108 of 108 catalogue cases with common pieces fail,
   135 of 139 hunt cases and 48 of 48 anchorless; the four hunt cases that pass are those with a band
   difference, where the exemption applies to every common piece of the case. With every `DEDUCED`
   class inverted, 244/244, 210/232 and 0/1 fail (the rest are band pieces); with every
   `MIDPOINT_TEST` class inverted nothing fails, as the design intends; with every `ANCHORLESS` class
   inverted 47 of 47 fail. So the exemption hides little; narrowing it to common pieces whose class
   was deduced at a vertex that also has a band piece would close the four, at some complexity.
6. **The "prototype fails too" skip** (`:508-520`) only prints and counts; a committed export in which
   the prototype fails a case would pass silently, since the count is not asserted. Assert
   `num_prototype_failures == 0` for the committed files (the `File` test can keep printing).
7. **Repair at the cap** is tested only with `max_repair_rounds = 0`; a case that thrashes to the cap
   with rounds available (1.2's `#2587`, or a hand-made ping-pong) would pin that the cap is a bound on
   rounds, that `check_failures` counts what is left, and that `repairs` counts the successful flips
   only.
8. `trusted_flipped` is asserted zero only; no test flips a vertex-tested class.
9. The result for identical polygons and for a subject equal to a clip hole (`ClassifyAnchorless`) is
   asserted; the `jittered` variant with the *subject* jittered (the prototype's `hole-equals-ring
   jittered`) is covered by the export, not the unit test. Fine.

## 3. Checked and found correct

Each item was traced against the prototype function the design names, by reading, and the three
committed comparisons plus the eighteen generated bundle cases were run with per-case statistics
printed.

- **The bundle rule** (`build_bundle` against `_bundle_split`, `_ray_angles`, `_admissible`,
  `_bundle_orders`): certainty order by (cone ascending, reach descending), ties in measured order (a
  stable sort on both sides); a ray anchors only with a direction and a cone below `pi`, and only if
  against every ray anchored so far the angle between directions is at least the sum of cones when that
  sum is positive (so cones of zero all anchor, as 3.8 says); the reference is the most certain
  anchored ray (the first of the certainty order, which is also the prototype's `min` in measured
  order); angles by `atan2` in the frame `(e1, e2)` about the output position, in `[0, 2pi)`, the
  reference's own angle `0`; widths cone plus the reference's cone; the admissible test is the same
  greedy earliest placement over the three shifts with a width of `pi` or more free; enumeration
  `k^m` gap assignments (last floating ray fastest) times a lexicographic permutation per gap (first
  gap outermost), the measured order found by cyclic rotation and swapped to the front or inserted;
  the cap and the no-anchored exit keep the measured order alone. Both `angle_between`s are
  `atan2(|cross|, dot)`. The bundle is built only at six or more ends where no input passes once, in
  `build_rays`, so the orders depend on geometry only. *Run*: 15 of 15 clean bundle cases agree with the
  prototype on every class and on floating, admissible, placement, ambiguous and moved counts.
- **The solve over orders** (`solve_vertex`, `:1453-1657`): prefix parities per order, the four starts,
  constraints on the two sectors beside a ray, a deduction only where every solution of every order
  agrees, memberships from the chosen order where its solutions agree; the tie-break by inversions
  (Kendall distance from the measured order, both linearised from ray 0; the C++ counts by positions,
  the prototype by measured pairs, the same number) with ties to the lowest index; `moved` is "not
  order 0"; `num_consistent_orders` counts distinct orders.
- **The one-passage rule** (`one_passage_side`, the branch at `:1331-1451` against `_is_once`,
  `_solve_once`): ends counted per input with a common ray counting for both; six or more ends with
  exactly two of one input and four or more of the other; `m_in` is the class for a plain end and
  "same side" for a common end (`c xor 1` with `same = 0`); `n` the parity of the other input's rays
  classified inside; the three deductions (`m_a = m_b xor n`, `m_b = m_a xor n`, the unknown ray's
  `m_a xor m_b xor n`), a common end's deduced class being `m` itself; the check when all is known;
  nothing deduced from two or more unknowns; the order `a, [inside, measured relative order from a],
  b, [outside]` and its mirror, the first with a placement used (the C++ walks both and takes the
  first consistent, which is the prototype's "mirror only if the first fails"); `moved` by cyclic
  comparison with the measured order; no bundle built there. *Run*: the committed data has 117
  one-passage cases (93 with a moved order), `0 differ`, and `OnePassageRule`'s counts hold.
- **The spike restriction** (`:785-802` against `_contract` under `spike_tip_once`): counted at the
  tip over the other input's piece ends not yet internal, at the moment the spike is tested, rings and
  pieces in the same order on both sides (loops first, then subject rings then clip rings in ring
  order), so an earlier spike's removal is seen by a later one exactly as in the prototype.
- **Anchorless chains** (`get_common_chain`, `classify_anchorless_chain` against `_chain_of`,
  `_anchorless_pick`, `_classify_anchorless`, and design 3.9's stated differences): the chain search
  through vertices whose every ray is common; the arc maximising `min(|dot(R_P, axis)|, |dot(R_Q,
  axis)|)` over arcs with a direction; `X` the normalised sum of the piece's two stop positions
  (within `h` of the great circle at most, which the parity entry allows: the excluded segment is
  then either not crossed or crossed uncounted, both giving the membership on the reference's side);
  each input's membership as `status xor parity` from its own reference point, leaving out the global
  index of its own segment (`first_segment_index + single_arc_segment`, set for every common piece
  since its two stops lie in one segment); the clip's membership flipped when the two references are
  on opposite sides of the subject arc's great circle; `same` iff equal. Termination: each round
  classifies an unclassified arc and deduction carries it along the chain (every all-common vertex
  deduces from one known ray), so `start` is resolved each round on consistent data (1.3 for the
  other case). *Run*: 47 of 48 anchorless cases resolve a chain (the 48th has a plain piece), all
  agree with the prototype, and inverting the anchorless class fails 47.
- **`PointInPolygon::get_crossing_parity_excluding_segment`** (`PointInPolygon.cc:2289-2345`):
  `get_num_polygon_edges_crossed` decides each edge from its own two end points' sides of the crossing
  plane and only forbids wrapping past a ring's last edge, so a ring's count is the sum over
  `[begin, excluded)` and `[excluded + 1, end)`, both non-wrapping and either possibly empty (the
  function returns 0 for an empty range); the excluded index is mapped through the rings by a running
  offset in exterior-then-interiors order; an index past the last ring excludes nothing. Nothing
  changes for existing callers: the new function shares the file-local helpers and adds no state.
  Exercised with the excluded segment in an interior ring by the `hole-equals-ring` cases.
- **Repair** (`repair` against the prototype's, plus the design's whole-chain candidate): candidates
  are the classified pieces at the failing vertex, each once, a common pair as its subject piece,
  ranked by method then length (stable, so ties keep ray order as the prototype's stable sort does);
  the whole chain of each classified common ray at a vertex with a non-common ray, ranked with
  `SECTOR` and before any single piece of that rank; a flip flips the partner and is undone exactly
  if the vertex stays inconsistent; on success every flipped piece (and partner) becomes `REPAIRED`,
  `trusted_flipped` counts the vertex-tested ones, one round per flip up to the cap; a round with no
  fixing candidate ends repair. *Run*: `RepairSingleFlip` and `RepairWholeChain` pass; in `shared#6
  seed 11` the C++ reaches consistency where the prototype does not (1.2).
- **`classify` / `check_and_repair`**: `classify_pieces` is (a) to (e) of 3.7 unchanged from review 1
  plus (d); `check_and_repair` counts the failing vertices, repairs only if any, counts unresolved
  pieces (a common pair once), reruns every vertex for the final counts and the per-rule statistics
  (one-passage, both-pass-twice, bundle: only at vertices with a floating ray, as the prototype), and
  throws `InconsistentOverlayException` with the diagnostics, the first failing vertex's position and
  its records when anything fails and `throw_if_inconsistent` is set. The exception derives from
  `MathematicalException` as 9.2 asks. `get_result_from_classes` is the keep-rule of 3.10 and 5.1
  (checked against `f` for the intersection and the difference), with an unclassified piece keeping
  both so that a failure reads as `INTERSECTING`.
- **Robustness**: `x_ends` has exactly two entries by `one_passage_side`; `piece_of` and `m_in` read
  the subject piece of a common ray; `std::find` for ray 0 and for `r` in an order always succeeds
  (orders are permutations of all rays); `single_arc_segment` is set for every common piece;
  `rotation_axis()` is called only after `is_zero_length()`; `*v` in `get_common_chain` is set for a
  common piece; `solutions.empty()` returns before `consistent_orders.front()`; the enumeration's
  carry loops terminate; `get_vertex_records` on the failing vertex is in range. The enumeration's
  size is `(k + m - 1)! / (k - 1)!` for `k` anchored and `m <= 6` floating rays (5040 seen), fine for
  any `k` the hunts produce.
- **The comparison test**: the `repairs != 0` failure, the common exemption only beside a band
  difference, the "prototype fails" skip, the `File` test on an environment variable, and the
  `OnePassageRule` counts behave as their comments say. Methods are still not compared (review 1 did
  it by probe).
