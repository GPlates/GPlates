# PR C: the twisted ring, and the symmetric difference's crossing rings

Done 2026-11-13 (Fable), taking over a draft fix that a stopped agent had left uncommitted in the working
tree of `feature/polygon-intersections` (HEAD `208792025`). Two defects had been diagnosed before the
draft: a twisted output ring in the difference and the reverse difference (silent), and crossing ring groups
in the symmetric difference from inputs whose rings do not cross. This is the verdict on the draft, what was
kept and changed, and the evidence. Nothing is committed: the changes are in the working tree.

Every number below is from a run on this machine (Windows, the `gplates` conda environment, Qt6), on one of
three builds of `gplates-unit-test`: **head** (HEAD's `PolygonOverlay.{h,cc}` with the two new diagnostics
added as unused fields, so that the same test files compile), **final** (the working tree), and the
**draft** as found (kept for the experiments its scratch switches allowed). "The hunt" is the scratch export
of 119,198 generated cases in 217 chunks (109,613 of them polygon cases, 39,313 with rings that do not cross
by `PolygonOverlayOracle::is_area_sum_applicable`, 39,320 by `rings_cross` alone); "swapped" means the same
case with subject and clip exchanged. Scripts and logs are in the session's scratch directory, under `work/`
(section 9).

## 1. Summary

- **The draft's design is kept**: its three parts are sound and each is needed. What changed is its tests
  (one of its two hand tests for the symmetric difference did not fail without the fix; a comparison it
  had switched off is now counted and pinned), its comments where they said more or less than the code
  does, the scratch switches (removed), and the line endings its scripts had rewritten.
- **Defect 1** (a twisted ring): a ring's arc passes a vertex of its own input with no stop there, because
  `GeometryIntersect` never intersects an input with itself; inside a common pair that hides a crossing of
  the ring with itself. Fixed by cutting such a ring at the chain's two vertices
  (`split_walked_rings`, `rings_cut_at_chains`).
- **Defect 2** (crossing rings in the symmetric difference) has two mechanisms, both in walking the
  symmetric difference by its own pairs: the split's cuts are right in any order only if no two passages
  cross, and (a) pairs made from a noise order of two parallel rays cross, (b) pairs made through a common
  piece that separates nothing cross. Fixed by making the symmetric difference of the two differences'
  rings, joined across the common pieces between them (`join_difference_rings`) but for a wall that either
  walks as a spike (`spike_walls`).
- **Proof**: all 215 committed tests pass (the symmetric difference's exemption from the crossing-group
  check removed from both test files); the hunt comparison has the same 10 differing cases after as before
  and 37 fewer; no crossing group from valid inputs in any operation, and no area identity off, with the
  inputs either way round; partitioning's outputs are the same coordinate for coordinate but where a ring is
  cut at a chain.
- **What it changes in partitioning** (section 6): one committed case and 69 hunt cases as exported, each
  with one ring more. Ten of the 69 have valid inputs, and there the cut removes a real twist of a sliver
  (`1.1` to `1.7 h^2`), after which the two outputs' areas sum to the subject's to rounding.
- **Open** (section 8): the conditions of the crossing test that no committed test pins; walls the join
  leaves unjoined that are real shared edges; a sliver under half a thickness wound backwards on the part
  cut off; the symmetric difference now fails wherever either difference does.

## 2. Verdict on the draft

Read in full (`git diff` against HEAD: 801 lines in `PolygonOverlay.cc`, 126 in the header, about 500 in
the two test files), then tested part by part with the switches it had left in (`SCRATCH_NO_JOIN`,
`SCRATCH_NO_CHAIN_CUT`, `SCRATCH_NO_SPIKE`) and one I added for an experiment (the split's cuts in another
order).

| Part of the draft | Verdict | Evidence |
| --- | --- | --- |
| `extract_rings` split into `pair_and_walk_rings`, `split_walked_rings`, `walk_rings`; `WalkedPoint` | Kept. Needed by the join, which works on walked rings with their pieces, and changes nothing by itself | The intersection, both differences and the union are the same polygons in head and final, coordinate for coordinate, in every operation without a cut at a chain: the 804 committed polygon cases and the hunt's 109,613, both orders (section 6) |
| The cut of a ring twisted at a chain (`FirstArc`, the crossing test) | Kept, as written. The test is geometric, with the thickness; I found no combinatorial one (section 3) | 13 catalogue cases fixed; in the hunt, every cut on valid inputs removes a twist that an area shows (section 6) |
| The symmetric difference as the two differences' rings joined (`join_difference_rings`) | Kept. It makes the order of the split's cuts not matter, which neither order of cuts did for its own pairs | With the join, the hunt's results are the same with the split's cuts made in the other order; with the old pairing they are not (section 4) |
| The spike rule for walls | Kept, but its comment said it finds a ring that "runs back along the whole wall", and the test is wider: any wall whose far end is within the thickness of the next arc | 455 of the 538 walls it leaves in the hunt's valid cases are under `2 h` long (section 4) |
| `LinkTwistAtChain` (hand test) | Kept; a notch added (a ring through both ends of an unkept chain that must not be cut) | Fails on head in both of its lengths and both orders |
| `LinkSymmetricDifference`, "parallel rays at a vertex" | **Replaced.** It passed on head: its walk repeated one vertex only, so the split could not go wrong | The new geometry follows `six#29 seed 10` and fails on head (section 5) |
| `LinkSymmetricDifference`, "a spike is no wall" | Kept | Fails on head |
| The prototype comparison skips a ring set when the overlay cut a ring at a chain | Kept, but it was unpinned: a rule that cut too much would have switched off its own check | The count is now pinned per committed file, and a new test pins the cuts and unjoined walls by operation, both orders (`CutsAtChainsAndSpikeWalls`) |
| Two branches of the join: a guard where two joined pieces meet, and a second count of a pairing failure | Removed: both were no-ops | Mutating each away failed no test; the guard relinks two ends to each other as they are, and the failure is counted where it happens. Every hunt and committed output is identical without them (all five operations, both orders) |
| Scratch switches and debug output in `PolygonOverlay.cc` | Removed | |
| Three files rewritten with LF line endings by its edit scripts | Restored to CRLF, as the checkout has them | `git ls-files --eol` |

Two of the brief's questions, answered:

- *Does the chain cut's geometric test bring in a tolerance or a decision under `h` that the design
  avoids?* It brings in no new constant: it is the self-touch cut's two measures (the thickness sine and
  `1e-12` radians along the arc), asked of an output ring. The decision it makes under `h` is whether a
  vertex lies on an arc, which is what a record is; the crossing itself is decided by points beyond `h`.
  It is in section 1's table, and in 5.3 among the rounded decisions.
- *Can a legitimate ring pass both ends of a long unkept chain without crossing itself, and is it left
  alone?* Yes and yes: a notch along a shared edge (the intersection of a square with a polygon whose
  hole lies along part of the square's edge). It is now a committed case, and a rule that cut at a touch
  fails the committed counts (mutations C2, C2a, section 7).

What the draft's scratch files were, as far as I could establish: `area-scan-x*.log` is a sweep of an
earlier form of the spike rule, a wall-length threshold in units of `h` (`spec5.py` has the code); it shows
the rule is needed (25 crossing groups as exported with it off) and that any threshold from `0.75 h` up
cleared them; the draft's last form has the touch test in the length's place. `before-failures.txt` is a
hunt run of HEAD's overlay with the symmetric difference's exemption removed from the prototype test, and
I reproduced it exactly (the same 47 cases). `cut-scan-*.log` are counts of chain cuts by rule version:
6,483 reports for the first, a touch alone (`new_functions.cc` has it), and 241 with the crossing test.

## 3. Defect 1: a ring twisted at a chain

**Root cause.** `GeometryIntersect` intersects each input only with the other. So an arc `T` of one input
that passes within the thickness of a vertex `w` of the same input has no stop there. When `T` leaves an
overlay vertex `u` between the two boundaries of a common pair (which run from `u` to `w` within the
thickness of each other), it passes `w` between the rays that continue the two. The pair is one ray at `u`
and one at `w`; `T` is on one side of it at `u` and is not at `w` at all. No order of rays shows `T` between
the two boundaries, and every pairing is right as far as each vertex can tell. But a ring that leaves `u`
along `T` and later passes `w` from one of the two boundaries to the other crosses itself between `u` and
`w`.

**Evidence**, `d4 len=+1h phi=20 hole-between` (`work/` dumps, and the draft's `d4.txt`):

- vertices `v3` (the hole's tip) and `v1` (the short edge's far end) are joined by the common piece `p1`,
  0.37 `h` long, classified same side, which the reverse difference does not keep;
- the reverse difference's one ring is `v0 v3 (hole vertex) v4 v1 (P's vertex) ...`: it leaves `v3` along
  the hole's edge and passes `v1` from the clip's edge to the subject's exterior;
- the measured ray orders are `v3: [PQ*0 PQ*1 P4 P5]` and `v1: [PQ*1 P2 Q7]`: the ring turns the same way
  about the chain's ray at both, so the orders do not show a twist;
- drawn through its vertices' output positions, two of the ring's edges cross with the two parts wound
  opposite ways, the narrower `318 h` in mean width (the twist scan, section 6): all 13 `hole-between` cases,
  in the reverse difference as exported and the difference swapped;
- the identity `area(i) + area(r) = area(Q)` is off by twice the part (the caller's scan; the hand test
  `LinkTwistAtChain` shows the same on head).

**Fix.** In `split_walked_rings`, after the cuts at repeated vertices: a ring that passes two vertices of
one chain its operation does not keep, without walking the chain between them, is tested for the crossing
`GeometryIntersect` would have recorded had it intersected the ring with itself. `w` must lie within `h` of
the plane of the ring's arc at `u` and inside the arc by more than `1e-12` radians (and the arc must not
end at `w`, by identity); and the ring's first points beyond `h` of that plane, each way from `w`, must be
in different sectors of the ring's passage at `u`. Then the ring is cut at `u` and `w` as at one vertex,
each part closed along the chain's pieces.

**Rejected** (the design page's section 5.7 has them):

- *The order of the rays at the two ends*, with no geometry: the measured orders do not show the twist
  (above).
- *A cut wherever an arc passes a vertex* (a touch, no crossing; the draft's first form, and mutation
  C2a): it cuts a ring in 362 of the hunt's intersections and differences from valid inputs, as exported
  (225 and 137; 206 of them from the `tinyhole` generator, 78 each from `shared` and `zero`), where the
  rule cuts 10, and in 599 from inputs whose rings cross. Those rings are not twisted, and
  partitioning's outputs there are right as they are.
- *The chain's length*: a tolerance, and the test runs a chain of 140 thicknesses.
- *Another margin for the far points*: with `0.5 h` (mutation C18) one more of the hunt's valid differences
  is cut, a part of `0.77 h^2`. A margin of `10 h`, as the grouping's, was not tried: it would by
  construction leave a twist up to `10 h` wide. The thickness is the scale at which the records
  themselves decide a crossing.
- *A cut at `w` alone* (the arc given a stop at `w`, each part closed there): both parts would then be
  simple as drawn, where the draft's first part keeps a sliver wound backwards (below). But the part cut
  off would run from `w` and not from `u` along `T`, moving that edge by up to `h` over its whole length,
  and its first point would be at a vertex its piece has no stop at, which the provenance and the join
  both assume it has. Not tried in code.
- *A stop for the arc at `w` before classification*: the repair at the root, and out of reach here: it
  changes the pieces, which the prototype comparison pins.

**What the fix leaves.** The part that keeps `T` still leaves `u` along it. Where `w`'s output position is
on the other side of `T` from that part, its last edge crosses `T` within the thickness of `w`: a sliver
under half a thickness wide, wound backwards. The twist scan sees it in the 13 catalogue cases after the
fix (16 committed cases have such a sliver after, 3 before), and the hunt has about 270 cases with one
before and after, from the band's noise elsewhere.

## 4. Defect 2: crossing ring groups in the symmetric difference

**Root cause.** The symmetric difference keeps all four ends of every crossing, so a walk of its own pairs
passes most vertices twice. The split cuts at the first repeated vertex it meets. A cut at a vertex replaces
the walked pairs there by the two loops' own, and that is the pairing that does not cross only if the
loops cross nowhere else. So the cuts are right in any order while no two passages cross, and otherwise the
vertex whose pairs cross has to be cut first, which is what is not known. Two things made pairs cross:

1. **A noise order** (the `six` generator; 12 cases as exported, 25 swapped). `six#29 seed 10`: two holes of
   `P` with parallel edges `1.34 h` apart, a vertex of `Q` between them within `h` of both (one overlay
   vertex `v6`, which both holes pass straight through), `Q`'s edge running on between the two. The two
   holes' rays to the east are parallel (the edges are copies of each other, moved sideways), so their
   order by direction is noise: the measured order has the north hole's first, anticlockwise from `Q`'s
   other edge, which reaches `v6` across the south hole. The walk shows the pairs made from it: the south
   hole straight through `v6`, and `Q`'s edge with the north hole's ray, which cross. The walk passes `v8`
   (an ordinary crossing of `Q`'s edge and the south hole) twice between its two passages of `v6`; the
   split met `v8` first, cut there, and left the south hole's whole ring crossing a ring through it. The
   reverse difference's walk has the same crossed pairs at `v6`, but two ends at `v8`, so it comes back to
   `v6` first and the cut there undoes them.
2. **A wall that separates nothing** (the `zero` generator; 25 as exported, 32 swapped). `zero#133 seed
   10`: `P` crosses an edge of `Q` with three vertices within `h` of it (`v2`, `v3`, `v4`), its two short
   edges doubling back and forth. The pair `v2`-`v3` (0.64 `h`) is classified opposite sides from band
   pieces, both differences walk it, and the symmetric difference's pairs run through it: `Q`'s boundary
   straight through the cluster, and `P`'s. The two differences' own rings are right there (opposite
   sectors of the crossing, touching).

**Evidence** (the draft build, with its switches):

| Symmetric difference built as | Crossing groups from valid inputs, as exported | swapped |
| --- | --- | --- |
| its own pairs, cuts at the first repeat (HEAD) | 37 (12 `six`, 25 `zero`) | 57 (25, 32) |
| its own pairs, cuts at the last repeat | 47 (39 `six`, 8 `zero`) | 27 (26, 1) |
| the differences joined across every wall | 25 (`zero`, the same 25 cases) | 32 (the same 32) |
| the differences joined, but for spike walls (final) | 0 | 0 |
| the same, cuts at the last repeat | 0 | 0 |

In every row the intersection, both differences and the union have none. The second row shows the old
result was an accident of the cut order; the last shows the new one is not.

**Fix.** `walk_rings` makes the symmetric difference of the difference's and the reverse difference's rings,
each walked and split on its own, joined across the *walls* between them (runs of opposite-sides common
pieces both walk, which both keep and the symmetric difference does not), then split again. Once joined, a
ring repeats a vertex only where the two outputs touch, so the order of the cuts no longer matters. A wall
is left unjoined where an arc by which either ring reaches one of its ends, or leaves it, passes its other
end (within `h` of the arc's plane and inside the arc): that ring comes back along the whole wall, so its
output has no width there. Leaving a wall is always right (the two rings share it and read as the output
does); joining one is right only if it separates the two outputs.

**Rejected:**

- *An order for the split's cuts* (the vertices not of the free rule first, say): no order is right without
  knowing which pairs cross; each of the two tried fails somewhere.
- *A better order of the parallel rays* (by ring continuity: an input whose rings do not cross has no two
  passages of its own crossing at a vertex, which picks the right order at `six#29`'s `v6`): it changes
  `solve_vertex`'s order, which the prototype comparison pins, and does nothing for mechanism 2. Not tried
  in code.
- *The wall's length* as the spike rule (the draft's first form): a tolerance.
- *The difference plus the reverse difference, unjoined*: always right, and what the join falls back to;
  the developer wants shared edges merged.

**What the fix leaves.** The spike rule leaves more walls than it must. In the hunt's valid cases as
exported it leaves 538 walls in 528 cases (1.3%): 455 under `2 h` long (228 under `0.5 h`), none between
`2 h` and `5 h`, 2 up to `100 h`, and 81 longer (up to `7,307 h`), all 83 from the `shared` generator. In
the one I traced (`shared#508 seed 26`) `Q` passes the same stretch three times, folding back on itself
along a shared edge. There the two differences' rings share the wall unjoined, unless other walls join
them (as in that case, where the result is the union's six rings). And it looks only at the arc next
to the wall: a ring that doubled back over a wall in two arcs would not be found. None is in the hunt
(no crossing group remains), and the result would be a crossing group, which reads right.

## 5. Tests

Committed tests, `gplates-unit-test` unfiltered: **215 pass on final** (7 skipped: `File`, `RealData`, and
five scratch tests that need an environment variable). **On head, five fail**, which is the check that each
new test fails without the fix:

| Test | On head |
| --- | --- |
| `PolygonOverlayTest.LinkTwistAtChain` | ring sizes `{11}` for `{4, 9}` and no cut counted, at both lengths and both orders; at the short length also a crossing group in the symmetric difference, and `i + r` (swapped, `i + d`) off by `2.7e-6` against a tolerance of `1.4e-6`. (At the long length the part wound backwards is about `3 h` wide, under both checks.) |
| `PolygonOverlayTest.LinkSymmetricDifference`, parallel rays | with the wider gap, inputs as written: ring sizes `{4, 6, 11}` for `{4, 4, 6, 7}`, a crossing group, the area off |
| `PolygonOverlayTest.LinkSymmetricDifference`, a spike is no wall | a crossing group and the area off, both orders |
| `PolygonOverlayTest.LinkSymmetricDifference`, joined across a wall | passes on head too (its own pairs give the same two rings); it pins the join and the split after it against mutation |
| `PolygonOverlayPrototypeTest.Catalogue` | the 13 `d4 hole-between` cases: a crossing group in the symmetric difference |
| `PolygonOverlayPrototypeTest.Hunt`, `CutsAtChainsAndSpikeWalls` | the pinned counts |

What was added or changed in the tests:

- `check_oracle` (`PolygonOverlayTest.cc`) and the prototype comparison no longer exempt the symmetric
  difference from "no crossing group from inputs whose rings do not cross".
- `check_areas` (new helper): all four area identities, within `4 h` times both perimeters. Used by the two
  linking tests; a twist is invisible to the point oracle.
- `LinkTwistAtChain`: the catalogue's case at a chain `0.37 h` and `140 h` long, both orders, pinning the
  ring sizes, the cut's count and the areas; and a notch that must not be cut.
- `LinkSymmetricDifference`: a plain join (two differences of one ring each, sharing an edge and
  touching at a crossing: joined, then split at the touch, into the outline of the two and a hole);
  mechanism 1 after `six#29` (both gaps, both orders; ring sizes of both differences and of the symmetric
  difference, no spike wall, no cut); and mechanism 2 (a spike wall, counted).
- `CutsAtChainsAndSpikeWalls` (prototype test): over the catalogue and the hunt's sample, both orders, the
  cases with a cut by operation (13 in the reverse difference as exported, in the difference swapped; one
  in the hunt's sample) and the walls left (13 and 8).
- The ring sets not compared with the prototype's for a cut at a chain are pinned per file (0, 1, 0, 0).

The scratch tests at the end of `PolygonOverlayPrototypeTest.cc` are still there (`ScratchSymDiff`,
`ScratchAreaScan`, `ScratchChainCuts`, and three I added: `ScratchOutputDump`, `ScratchTiming`,
`ScratchTwistScan`), with `<chrono>` included for one of them. They are to be removed before committing.
(Removed before the commit; kept in the planning repository's
`plans/polygon-intersection-tools/cpp/prC/scratch_tests.cc`, with the scripts that drive them.)

## 6. Numbers, before and after

**The hunt comparison** (`PolygonOverlayPrototypeTest.File` over the 217 chunks):

| | head | final |
| --- | --- | --- |
| cases that differ | 47 | 10 |
| of which: a crossing group in the symmetric difference, valid inputs | 37 | 0 |
| of which: the 9 known at PR B's merge (4 coincident rings, 4 second-step provenance, 1 pieces) | 9 | 9, the same reasons |
| of which: `six#13 seed 11`, 3 intersection rings against the prototype's 2 | 1 | 1 |
| ring sets not compared, touching at two vertices | 783 | 783 |
| ring sets not compared, cut at a chain | 0 | 96 |
| cases with crossing input rings, wrong as polygons | 0 of 70,291 | 0 of 70,291 |

`six#13 seed 11` is new against PR B's count of 9 and is not from this change: head has it. The export is
a fresh one, and the prototype's ring decompositions vary between exports.

**The scans** (39,313 polygon cases whose rings do not cross; 39,320 for the twist scan):

| | head, as exported | head, swapped | final, as exported | final, swapped |
| --- | --- | --- | --- | --- |
| cases with an area identity off (`4 h` times both perimeters) | 26 | 41 | 0 | 0 |
| operations with a crossing group | 37 | 57 | 0 | 0 |
| polygons with a misoriented ring | 0 | 0 | 0 | 0 |
| cases with a twisted ring, the narrower part over `1 h` in mean width | 0 | 0 | 0 | 0 |
| the same, `0.5 h` to `1 h` | 1 | 1 | 1 | 1 |
| the same, under `0.5 h` | 271 | 262 | 275 | 260 |

All of head's area failures and crossing groups are in the symmetric difference. In the committed data the
twist scan finds the catalogue's 13 on head (`318 h`) and none over `0.5 h` on final.

**Every operation's polygons, head against final** (`ScratchOutputDump`: a hash of every coordinate in
order):

| | I | D | R | U |
| --- | --- | --- | --- | --- |
| committed, as exported: operations that differ | 0 | 1 | 13 | 0 |
| committed, swapped | 0 | 13 | 1 | 0 |
| hunt, valid inputs, as exported | 3 | 7 | 0 | 0 |
| hunt, valid inputs, swapped | 3 | 0 | 7 | 0 |
| hunt, crossing input rings, as exported | 29 | 31 | 10 | 26 |
| hunt, crossing input rings, swapped | 28 | 10 | 32 | 24 |
| of all of these, without a cut at a chain | 0 | 0 | 0 | 0 |

So the restructuring changes nothing by itself, and partitioning's two outputs change only where the cut
is made. The symmetric difference's polygons differ in most cases (34,502 of 39,320 valid, as exported),
since its rings are made another way; its area differs from head's by more than `1 h^2` in 40 of the 39,283
valid cases that had no crossing group before (13 by more than `1,000 h^2`, the most `11,157 h^2`; 32 of
the 40 from the `shared` generator and 8 from `zero`), and both builds are within the area tolerance
there: every area failure on head has a crossing group, and the identity off is always the symmetric
difference's against the two differences'.

**Every changed case of partitioning's outputs** (the intersection or the difference), each with one ring
more than before:

- Committed, as exported: `chain#108 seed 5` (the difference; its input rings cross). Swapped: the 13
  `d4 hole-between` cases (the difference), which is Defect 1.
- Hunt, valid inputs, as exported (10): the intersection of `zero#323 seed 2`, `zero#426 seed 7`,
  `zero#563 seed 11`; the difference of `zero#13 seed 6`, `zero#110 seed 9`, `zero#213 seed 3`,
  `zero#239 seed 5`, `zero#511 seed 2`, `zero#543 seed 4`, `zero#543 seed 5`. In each the ring cut off is a
  sliver of `1.1` to `1.7 h^2` that was wound backwards: `area(i) + area(d) - area(P)` goes from between
  `-4.2e-12` and `-6.7e-12` steradians to under `3e-17` in size. (`zero#511 seed 2`: two vertices of `P`
  `0.0007 h` apart, joined by a same-side pair; the difference's ring crossed itself between them, with a
  triangle about `3 h` long on the wrong side.) These are real instances of Defect 1 that the area scan's
  tolerance hid; the caller's "never in the hunt" holds for a twist wider than the thickness.
- Hunt, crossing input rings, as exported (59 cases, 60 operations). The intersection (29): `chain#2 seed
  14`, `#105 s17`, `#138 s18`, `#155 s21`, `#158 s16`, `#182 s13`, `#214 s14`, `#319 s22`, `#387 s21`,
  `#424 s19`, `#533 s13`, `#555 s15`, `#596 s13`; `near#393 s2`; `shared#194 s18`, `#1041 s6`, `#1341 s4`,
  `#2149 s7`, `#2457 s7`, `#2791 s6`, `#3085 s10`, `#3639 s11`, `#3818 s4`, `#3879 s11`, `#4348 s4`;
  `zero#14 s5`, `#419 s10`, `#420 s10`, `#566 s9`. The difference (31, one case counted in two chunks):
  `chain#1 s14`, `#154 s17`, `#239 s15`, `#275 s21`, `#340 s13`, `#398 s19`, `#417 s16`, `#527 s14`,
  `#533 s22`, `#542 s18`, `#599 s22`; `shared#115 s2` (and its jittered copy), `#135 s28`, `#658 s3`,
  `#1820 s10`, `#2078 s4`, `#2407 s10`, `#3293 s9`, `#4744 s2`; `zero#139 s5`, `#182 s11`, `#212 s11`,
  `#214 s4`, `#262 s9`, `#361 s8`, `#374 s5`, `#449 s10`, `#460 s5`, `#465 s7`. A ring there crosses itself
  because an input does; cut, it is two rings that read the same by parity (none of the 70,291 is wrong as
  polygons, before or after).

**The symmetric difference fails where a difference does.** In three cases with crossing input rings whose
ends cannot be paired in the other operations either (`shared#2288 seed 2` as exported, the known
inconsistent cut; `near#73 seed 2` and `shared#607 seed 3` swapped), the symmetric difference had 0, 0 and
1 pairing failures on head and has the sum of the two differences' on final (16, 20, 10). With
`throw_if_inconsistent` on, it now throws where either difference would.

**Timing** of the extraction alone (classification excluded; `ScratchTiming`: microseconds per overlay,
the best of 5 passes of 10 repeats, three rounds of each binary on a quiet machine, given as the range of the
rounds; head, then final):

| Data | rings, I + D | polygons, I + D (the whole `extract`) | rings, symmetric difference | polygons, symmetric difference |
| --- | --- | --- | --- | --- |
| catalogue and hunt sample (737 overlays) | 5.65 to 5.70, then 6.16 to 6.27 (+9%) | 20.3 to 20.4, then 20.8 to 21.2 (+3%) | 4.76 to 4.90, then 10.3 to 10.6 (2.1 times) | 40.4 to 40.9, then 46.9 to 47.2 (+16%) |
| `shared-s2`, dense shared edges (600) | 9.28 to 9.29, then 10.8 to 11.0 (+17%) | 34.5 to 34.7, then 36.0 to 36.5 (+5%) | 7.38 to 7.58, then 20.7 to 21.6 (2.8 times) | 56.1 to 57.2, then 70.8 to 71.9 (+26%) |
| `holes`, `near`, `six`, `zero`, seed 2 (2,399) | 9.46 to 9.85, then 10.2 to 10.4 (+7%) | 37.2 to 37.6, then 37.9 to 38.1 (+2%) | 8.03 to 8.30, then 15.9 to 16.4 (2.0 times) | 81.5 to 82.1, then 90.1 to 90.3 (+10%) |

So partitioning's ring extraction costs 7% to 17% more (about half a microsecond to a microsecond and a
half per overlay), and its whole `extract`, with the grouping, 2% to 5% more. I did not look for where
the time goes or try to win it back. The symmetric difference's rings take 2 to 2.8 times as long (two
walks, two splits, a join and a third split), and its whole `extract` 10% to 26% longer. Nothing calls the
symmetric difference yet.

**pyGPlates** compiles the same sources: `build-pygplates` rebuilt with the final ones, and its seven
CTests pass (`pygplates-test`, which has the partition tests; the source closure, the linkage check and
the stub among the rest). No file under `src/api/` or `pygplates/` is touched.

## 7. Mutations

35 mutations of the two rules (20 of the cut, 15 of the join), each applied to the final
`PolygonOverlay.cc`, rebuilt, and run against the committed tests of
`PolygonOverlay*:PolygonPartitioner*:PartitionFeatureUtils*` (`work/mutate.py`, `mutations.log`). 25 fail a
test; 10 survive. The survivors were then run over the hunt as exported and the committed files, and every
operation's polygons compared with the final build's (`work/mutant_hunt.py`, `mutant-hunt-*.log`; "valid"
and "crossing" are the inputs).

The cut of a ring twisted at a chain:

| | Mutation | Committed tests | The hunt, if it survives |
| --- | --- | --- | --- |
| C1 | no cut at a chain | fail: `Catalogue`, `Hunt`, `CutsAtChainsAndSpikeWalls`, `LinkTwistAtChain` | |
| C2 | a crossing wherever both far points are found (no sectors) | fail: `Hunt`, `CutsAtChainsAndSpikeWalls` | |
| C2a | a cut at any touch | fail: the same two | (362 valid intersections and differences change) |
| C3 | the sides of the arc's plane, not the sectors of the passage | **survives** | valid: nothing; crossing: 3 operations cut more (and 8 symmetric differences) |
| C4 | the vertex need not be within `h` of the arc | fail: the same two | |
| C5 | the vertex need not be inside the arc | fail: the same two | |
| C6 | an arc ending at the vertex passes it | **survives** | nothing changes |
| C7 | the walk from `w` goes on through the passage at `u` | **survives** | does not end: 55 of the 194 files were still running after five minutes of processor time each, where a file takes seconds (`shared#541 seed 2`, the symmetric difference, is one that hangs) |
| C8 | a point within `h` past the arc's end is walked over | **survives** | valid: nothing; crossing: 3 operations cut more |
| C9 | a point within `h` of the other arc's plane decides | **survives** | valid: nothing; crossing: 3 operations cut more |
| C10 | the leaving arc at a vertex only | fail: the same two | |
| C11 | the arriving arc only | fail: `Catalogue`, `CutsAtChainsAndSpikeWalls`, `LinkTwistAtChain` | |
| C12 | the first vertex's arcs against the second vertex only | **survives** | 3 of the 10 valid cuts are lost, and 48 crossing ones |
| C12b | the second vertex's arcs against the first only | fail: `Catalogue`, `Hunt`, `CutsAtChainsAndSpikeWalls`, `LinkTwistAtChain` | |
| C13 | a ring that walks the chain between the two is tested too | **survives** | nothing changes |
| C14 | the parts not closed along the chain | fail: `LinkTwistAtChain` | |
| C15 | kept chains count as well | **survives** | valid: nothing; crossing: 3 operations cut more |
| C16 | the sector on the other side | fail: `Catalogue`, `Hunt`, `CutsAtChainsAndSpikeWalls`, `LinkTwistAtChain` | |
| C18 | far points beyond `0.5 h`, not `h` | **survives** | valid: one more difference cut (`zero#254 seed 3`, a part `0.77 h^2` in area); crossing: 31 operations cut more |
| C19 | the first part closed with the other path | fail: `LinkTwistAtChain` | |

The symmetric difference's join:

| | Mutation | Committed tests | The hunt, if it survives |
| --- | --- | --- | --- |
| J1 | walked by its own pairs | fail: `Catalogue`, `CutsAtChainsAndSpikeWalls`, `LinkSymmetricDifference`, `LinkTwistAtChain` | |
| J2 | no wall joined | fail: `Anchorless`, `LinkSymmetricDifference`, `UnionOverHemisphereThrows` | |
| J3 | every wall joined (no spike rule) | fail: `CutsAtChainsAndSpikeWalls`, `LinkSymmetricDifference` | |
| J4, J4b | a spike of one difference's ring only (each) | fail: the same two | |
| J5, J5b | the arc at one end of the wall only (each) | fail: `CutsAtChainsAndSpikeWalls` | |
| J6 | a spike whatever the far end's distance from the arc | fail: `CutsAtChainsAndSpikeWalls`, `LinkSymmetricDifference` | |
| J7 | a spike though the far end is not inside the arc | fail: the same two | |
| J8 | a spike though the arc ends at the wall's far end | **survives** | 134 more walls left unjoined; 45 valid symmetric differences and 89 crossing differ; no crossing group |
| J9 | each piece a wall of its own | fail: `CutsAtChainsAndSpikeWalls` | |
| J10 | a step walked backwards keeps its stops as they were | fail: `Catalogue`, `Hunt`, `Bundle` | |
| J11 | the differences' closed pieces dropped | fail: many | |
| J12 | the joined rings not split again | fail: `LinkSymmetricDifference` (survived until the plain join case was added) | |
| J15 | a step walked backwards: its interior points in walk order | fail: many | |

Two more runs were not mutations of the logic: the thickness in the two new tests larger and smaller by one
part in `1e9`. Both pass every committed test, so no pinned count sits that close to the threshold.

**Unpinned by a committed test:** C3, C6, C7, C8, C9, C12, C13, C15, C18 and J8. What the hunt says of them:

- C6 and C13 change nothing anywhere: the conditions are never the deciding ones in 110,417 cases. C13
  (`walks_chain`) is kept as the statement of what the cut assumes, and it saves the arcs' arithmetic for
  pairs along a closure; C6 is the identity rule (an arc that ends at a vertex does not pass it) and is
  kept for that.
- C7 is what ends the walk. No committed case shows it; the hunt's dense shared edges do (a hang).
- C3, C8, C9 and C15 decide only cases whose input rings cross (3 operations each).
- C12 and C18 decide valid cases of the hunt: one direction of the pair test (3 of the 10 valid cuts),
  and the thickness itself as the far points' margin.
- J8 decides 45 valid symmetric differences (whether a wall beside a sliver ring is joined), with no
  crossing group either way.

A committed case for C7, C12 and J8 would be a hunt case added to `prototype-hunt.txt` (`shared#541 seed
2`, `zero#563 seed 11`, and one of the 45); I did not add data files.

**Added afterwards (Opus, the same day):** those three cases are now in `prototype-hunt.txt` (277 cases),
with `shared#31 seed 14` for J8 (a valid case whose one wall J8 leaves unjoined: found by a scan of the
hunt's spike walls with J8 applied). The pinned counts moved accordingly (`Hunt`: 2 ring sets not
compared for a cut at a chain; `CutsAtChainsAndSpikeWalls`: the intersection cut in `zero#563` either way
round, 10 walls each way), and the three mutations rerun with `work/mutate.py`: C12 fails `Hunt` and
`CutsAtChainsAndSpikeWalls`, J8 fails `CutsAtChainsAndSpikeWalls`, and C7 hangs (no result in 180 s), as
expected. So the unpinned list is C3, C6, C8, C9, C13, C15 and C18.

## 8. Open doubts

1. **The crossing test's finer conditions are unpinned by committed tests** (section 7). They are
   exercised by the hunt, and I did not build hand cases for them. (C7, C12 and J8 were pinned
   afterwards by three hunt cases; section 7.)
2. **The spike rule looks at one arc.** A ring doubling back over a whole wall in two arcs, with a vertex
   between, would not be found, and the join would be made. None in the hunt. The symptom would be a
   crossing group, not a wrong region.
3. **Real shared edges left unjoined**: 83 walls over `5 h` long in the hunt's valid cases, where an input
   folds back on itself along the edge. The output reads the same. A sharper rule would have to tell which
   of the ring's passes along the wall is the one with the output beside it.
4. **The sliver the cut leaves** on the part cut off (section 3), under half a thickness wide. A cut at
   `w` alone would not leave it, and costs accuracy along `T`.
5. **The chain cut needs the two vertices in one unkept chain, and an arc that starts at one of them.** A
   ring twisted by an input's touch of itself elsewhere (an arc passing a vertex with no common piece
   between) is not looked for. I know of no case, and the twist scan finds none over `1 h` in the hunt.
6. **Changed outputs for inputs whose rings cross** (59 hunt cases as exported, one committed): by design
   of the rule, which cannot know why the ring crosses itself. Each is one ring more, reading the same.
7. **The sliver filter**: with a filter on, the sliver cut off in the 10 valid cases is a ring of its own
   and will be dropped where the twisted ring was kept whole. Not measured through `PolygonPartitioner`.
8. **`six#13 seed 11`** differs from the fresh export's prototype rings, on head too. Not looked into
   beyond that: the prototype's two rings there share one named point and the overlay has three rings.
9. **Not built or run**: Qt5, Linux, macOS (the arm64 FMA difference the last commit fixed could move a
   decision at a margin; the new hand tests keep their distances clear of `h`, 0.6 and 1.2 against 1, and
   the committed counts do not move with the thickness changed by one part in `1e9`, section 7). The
   `gplates` target itself was not rebuilt, only `gplates-unit-test` and pyGPlates.

## 9. Files

Changed, uncommitted (`git diff --stat`: 2,149 insertions and 93 deletions in five files, about 675 of the
insertions the scratch tests):

- `src/maths/PolygonOverlay.cc`, `PolygonOverlay.h`: `extract_rings` in parts, the cut at a chain, the join.
- `src/unit-test/PolygonOverlayTest.cc`: the exemption removed, `check_areas`, the two linking tests.
- `src/unit-test/PolygonOverlayPrototypeTest.cc`: the exemption removed, the pinned counts,
  `CutsAtChainsAndSpikeWalls`; and the scratch tests, to be removed.
- `docs/design/architecture/foundation/spherical-geometry.md`: section 1 (one row), 5.2 (the symmetric
  difference, the ring twisted at a chain, the split, grouping), 5.3, 5.7 (two entries), 6 and 7.
- This file (new).

Not written: a changelog entry (the twisted ring is a fix users of partitioning can see, in rare inputs; it
belongs with PR C's own entries), and `PLAN.md`'s status.

Scratch (the session's scratchpad, `work/`): `hunt_run.py` (the hunt with a given binary), `scan.py` (a
scratch test over every chunk, both orders), `failures.py`, `dump_diff.py`, `twist_table.py`, `mutate.py`,
`mutant_hunt.py`; `hunt-head2/` and `hunt-final-last/` (the two hunt runs); `out/` (the area, twist and
dump logs of both builds, `dump-diff-hunt.txt`, `timing-last.log`); `mutations.log`,
`mutant-hunt-1.log`, `mutant-hunt-2.log`. `final/` holds the four source files as tested, `draft/` the
draft as found, `head-*` HEAD's. The extra binaries I had put beside the build's own in `build-gplates/bin`
(head, final, draft and the mutants) are deleted; `gplates-unit-test.exe` there is the final build.
