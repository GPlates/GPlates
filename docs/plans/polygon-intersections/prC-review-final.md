# PR C final review

Branch `feature/polygon-intersections` at `a1a08e529` (clean), 2026-11-14, against the merge base
`aeb027de7` (PR #110). An independent read of the whole diff outside `docs/plans/`, most of the
effort on `src/maths/PolygonOverlay.{h,cc}` and the pyGPlates methods. No file but this one was
written in the worktree; nothing was committed or pushed.

Severity, as the brief defines it: **high** wrong results, a crash or undefined behaviour, or a
blocker; **medium** a real defect in a rare case, a test gap that hides one, or a misleading
promise; **low** style, clarity, small risks.

**Summary: 0 high, 6 medium, 5 low.**

The PR's own code is sound as far as I could read and test it: the join, the cut at a chain, the
entry point and the bindings. I found no error in them, their loops end, and nothing I ran went
wrong for inputs none of whose rings touch another ring of the same polygon (no failure in about
134,000 operations checked against an independent overlay, section 3).

What I did find is in code the PR does not change, but which its new methods expose and its text
vouches for. A polygon that **touches itself** (a hole touching its exterior at a point, which
OGC allows and the overlay's own outputs have) gives wrong ring structure, and so a wrong
`get_area()`, when the point of touch lies on the other polygon's outline. Every point is still
read right by parity, which is why the oracle and the hunt do not see it. There are three causes
(findings 1 to 3). All three are on `main` already, in `partition()`. The first has a two-line
candidate fix that I tested.

**Recommendation.** Ready to open once the three `fixup!` commits are folded (finding 9) and the
texts this PR writes or extends stop promising what findings 1 to 3 break (finding 4). Findings 1
to 3 need not hold this PR, since it does not make them worse, but they should be fixed or recorded
as known limitations before pyGPlates 1.1 ships `partition()` and these methods. Finding 1 is
small enough for a pull request straight after this one.

## 1. How I checked

- **Reading.** `pair_and_walk_rings`, `walk_rings`, `join_difference_rings`, `get_first_arc`,
  `FirstArc`, `split_walked_rings`, `extract_rings`, `extract`, `create_polygon`,
  `extract_polygons`, with `build_stops_and_pieces`, `apply_loop_and_spike_rules`,
  `find_common_pairs`, `find_self_touch_cuts`, `build_rays`, `solve_vertex`,
  `deduce_to_fixed_point` and `classify_pieces` for what they rely on; `PolygonPartitioner.cc`'s
  polygon path; the bindings and docstrings in `PyGeometriesOnSphere.cc` and `PyExceptions.cc`;
  the tests.
- **The committed tests.** `gplates-unit-test --gtest_filter='PolygonOverlay*:PolygonPartition*'`:
  94 pass, 2 skipped by design. `test_geometries_on_sphere.PolygonOnSphereCase`: 44 pass.
  `generate_stub.py --check`: the stub is current.
- **The hunt.** `PolygonOverlayPrototypeTest.File` over all 217 chunks of the scratchpad's export.
  10 cases differ, the same 10 that `prC-symdiff-twist.md` section 6 records (4 coincident rings,
  4 second-step provenance, `shared#2932 seed 2`, `six#13 seed 11`). So the plan's "after" numbers
  reproduce.
- **A fuzz against an independent overlay.** Polygons with integer vertices on a small plane grid
  (3 to 5 units), mapped to the sphere by a gnomonic projection at 0.02 radians per unit, so that
  straight lines are great circles: shared vertices, shared edges, edges overlapping in part and
  vertices on edges are all common, and every feature is thousands of thicknesses across. Shapely
  (2.2, in another conda environment, exchanging JSON) gives the exact answer in the plane. For
  each pair and each of the four methods I compared the total area with shapely's polygons mapped
  the same way (to `1e-10`), tested every method at a few hundred sample points per pair
  (membership, and no point in two polygons), and compared the numbers of holes. Seven families:
  star-shaped
  polygons, polyominoes, polygons with one hole, and each against a shifted, rotated or mirrored
  copy of itself. 24,000 pairs with the sliver filter off, 12,000 of them also with the operands
  swapped, 12,000 with the default filter; and 10,000 triples for two-step cascades (`op1(A, B)`,
  then each piece with `C`).
- **Scratch builds outside the worktree.** To see diagnostics and try changes without touching
  the worktree or its build trees, I compiled a copy of `PolygonOverlay.cc` (HEAD plus switches
  read from the environment, off by default) with the build trees' own compiler commands and
  linked a scratch `pygplates.pyd` and `gplates-unit-test.exe` from the trees' other objects. With
  no switch set both behave as the stock builds do (same test results, the same fuzz failures,
  case for case). The switches: print `Diagnostics` and `dump()` at each `extract`; skip the cut at
  a chain; choose the one-passage rule's order nearest the measured one (finding 1); count
  `deduce_to_fixed_point`'s sweeps (finding 5).

Everything is in the session scratchpad under `final-review/` (scripts, the 34,000 generated
cases, logs). It is not kept with the repository, so the reproductions and the candidate change
are also written out in section 5.

## 2. Findings

### 1. medium: the one-passage rule's linking order reverses rays, leaving rings that cross

`src/maths/PolygonOverlay.cc:2290-2311` (the two built orders) and `2416-2418` (the choice);
used by the pairing at `4228-4308`. On `main` since PR B. It is the cause of 191 of the fuzz's
193 intersections with a wrong area, so of `partition()`'s inside list as well.

**What is wrong.** At a vertex of six or more ends where input `X` passes once, the order for the
linking is built as `a`, the other input's rays inside `X` "in measured relative order", `b`, the
rays outside; then its mirror "if the walk finds no placement on the first". As far as I can see
both orders have a placement whenever one does (the relation is symmetric in the two halves, and
the other input has an even number of ends), so the first is the one used:
`chosen = consistent_orders.front()`, and the test at 2417 that prefers the order nearest the
measured one excludes `once`.

The first order is right only if the inside rays lie anticlockwise from `a` to `b`. If they lie
from `b` round to `a`, the true order is `a, b, i1, i2` and the built one is `a, i1, i2, b`. Read
the other way round that is `a, b, i2, i1`: the true order with the rays inside the half reversed.
With one ray in a half nothing changes. With two or more (the other input passing twice on one
side of `X`: a hole's tip on its own exterior, on `X`'s outline), consecutive kept rays pair
crosswise: `a` with `i1` and `i2` with `b`, where the output's sectors are `(b, i1)` and
`(i2, a)`.

One such vertex heals: its ring passes it twice, and the split's cut there is a non-crossing
pairing. Two such vertices on the same two rings do not: each ring passes each vertex once, the
rings cross at both, and `extract` puts them into one polygon as a crossing group. The design
page says the order within a half "decides only which kept rays pair with which (the ring
structure), never the point set" (`spherical-geometry.md:1331-1333`); that is so, but the ring
structure it gives here is two rings that cross.

**Evidence.**

- Reproduction 1 (section 5): two L-shaped polygons round a square hole that touches each one's
  own exterior at a corner. `P.intersection(Q)` should be two pieces of three grid squares each,
  `0.0024` steradians. It returns **one polygon, two rings that cross, `get_area()` `4.4e-16`**.
  `Q.partition(P)` puts the same polygon inside.
- The diagnostics of `holes-transform-1-40` (another such pair): `one_passage_vertices 2`,
  `one_passage_moved 2`, `rings_split 0`, `rings_cut_at_chains 0`, `crossing_ring_groups 1`,
  no repair, no check failure.
- With the order nearest the measured one chosen (the candidate change in section 5):
  `one_passage_moved 0`, no crossing group, and the two expected polygons with the expected areas
  to rounding.
- Over the fuzz's 6,075 pairs in which a polygon touches itself, wrong areas in the intersection
  fall from 193 to 2; the other methods barely move (their failures are findings 2 and 3). In the
  10,000 cascades, which start from polygons that do not touch themselves, 5 of 129,472
  second-step operations give a wrong area (all intersections of a first-step piece that touches
  itself), and none with the change.
- The cut at a chain plays no part: with it switched off the failing set is the same, case for
  case, in all four methods.

**Suggested fix.** Let the one-passage rule choose between its two built orders as the bundle
rule chooses between its admissible ones, by fewest inversions against the measured order (the
candidate in section 5), or build the order from the measured halves directly and fall back on
the classes only where the measured halves contradict them. With the candidate:

- the overlay tests: 92 of 94 pass. `OnePassageRule` fails on its pinned `one_passage_moved`
  counts (fewer vertices now move, as intended), and `Hunt` on `six#207 seed 1` (three
  intersection rings against the prototype's two);
- the hunt: 13 cases differ against the stock build's 10. The three new ones (`shared#276 seed
  27`, `shared#4906 seed 10`, `shared#492 seed 5`) are each one ring more than the prototype's,
  with no oracle failure, no crossing group and no area off;
- the fuzz and the cascades: no new failure.

So it looks safe, but the prototype has the same rule and the snapshots pin it, which is a reason
to make the change in its own pull request (with the prototype changed to match, as the test
file's header asks) and not in this one. A test for it: reproduction 1 with `check_areas` and no
crossing group, both ways round.

The new overview repeats the assumption (`polygon-overlay-overview.md:341`): "Where one polygon
passes once the order is not needed at all". It is not needed for the classes. It is needed for
the pairs.

### 2. medium: an input's touch of itself inside the other's edge is two overlay vertices

`src/maths/PolygonOverlay.cc`: `build_vertices` (identity only), the tie comment at `1112-1118`,
`find_common_pairs` (`1506-1566`, a pair is two single arcs between the same two vertices) and
the repeated-vertex cut in `split_walked_rings` (`4895-4919`, by identity). The rules are PR B's;
`union()` and `symmetric_difference()` are where most of it shows (53 of the fuzz's 55 wrong
unions).

**What is wrong.** Where two rings of one input meet at a point (each has a vertex there, or one
has a vertex on the other's edge), and the other input's edge passes through that point between
its own vertices, the two records name no common original vertex. They become two overlay
vertices at one position, with a zero-length piece of the other input between them. Three things
follow.

- A ring that passes both is not cut, since the cut is by identity. The output ring passes one
  position twice. In a union the hole is then a lobe of the exterior ring wound the same way, and
  `get_area()` counts it twice.
- Two pieces that lie on each other, one ending at the first of the two vertices and the other
  starting at the second, are not a common pair. Each is classified on its own, by a midpoint
  test of a point that lies on the other input's outline, so its class is rounding. The comment
  at 1116-1118 says either tie order "is a valid output, differing only in ring structure"; here
  the order decides whether a shared stretch pairs at all.
- The same happens with no record at all where an input touches itself away from the other
  input: the two passages are two original vertices inside two pieces, and a ring that walks
  both pieces passes the point twice. (`holes-transform-1-770`'s difference passes one such
  point and one pair of coincident vertices, and comes back as one twisted ring of area
  `0.00057` where three polygons of `0.00157` are expected.) The design page defers a ring
  twisted at a vertex of its own input with no chain there (`spherical-geometry.md:2884-2886`),
  with "no valid input of the hunt gives one wider than a thickness". The fuzz gives them whole
  polygons wide.

**Evidence.**

- Reproduction 2: `P` with a hole that touches its exterior at a shared vertex position, and a
  triangle `Q` one of whose edges passes through that point. `P.union(Q)` returns **one ring
  that passes the point twice, `get_area()` `0.01007` against `0.00487`**. The dump: records
  `SL(6,0)` and `SL(10,0)` (the exterior's vertex and the hole's vertex, both on `Q`'s segment
  0) are vertices `v0` and `v3`, and `Q`'s piece between them has length `0h`.
- Reproduction 3: `P.symmetric_difference(Q)` returns a ring with a **spike one grid unit long**
  (0.02 radians, about 127 km on the Earth), `(3,2), (3,1), (3,2)`, grouped as a hole of a
  polygon it lies outside, total area `0.00240` against `0.00280`. Both differences are right
  here. The dump: `P`'s piece `v7->v2` and `Q`'s hole piece `v6->v7` lie on each other, `v2` and
  `v6` are the two vertices at `(3,2)`, and neither piece is common. The same two polygons
  mapped about another point of the sphere give a different answer (the spike, but the right
  grouping and area), because the midpoint class of the unpaired piece flips from outside to
  inside. So the output of such inputs depends on rounding, and can differ between compilers.
- Fuzz, 6,075 self-touching pairs, sliver filter off: a ring that passes one position twice in
  53 unions, 14 differences and 21 symmetric differences, each with a wrong area. In 10 failing
  symmetric differences both differences are right. In the one I traced (reproduction 3) the
  pieces that should be a wall are not common, so the join does not see them.

**Suggested fix.** This needs a design decision, so I only name directions, none tested. The
least change: treat two overlay vertices joined by a zero-length piece (two stops tied on one
segment with nothing between them) as one vertex, for the vertex-pair key of `find_common_pairs`
and for the repeated-vertex cut. The fuller one is the self-intersection mode section 7 already
defers, which would give every arc its stop where it passes a vertex of its own input. Until
either, say so where users read it (finding 4).

### 3. medium: two valid polygons with holes' tips on a shared edge raise `PartitionError`

`src/maths/PolygonOverlay.cc:1570-1706` (`find_self_touch_cuts`). On `main` since PR B;
`partition()` raises too.

**What is wrong.** The self-touch cut looks for a chain of the other input's pieces from the
piece's start vertex **to its end vertex**. Where both inputs touch themselves on a stretch they
share, at different points, each side is cut only at the other's tip: one side's pieces are
`[0,2] [2,3]`, the other's `[3,1] [1,0]`. No chain of one side ends at the other side's cut, so
nothing is cut, nothing pairs, and the whole stretch is band pieces classified by midpoints that
lie on the other outline. The classes then describe no arrangement, or describe one with
zero-width rings.

**Evidence.**

- Reproduction 4: the same square twice, one with a hole whose tip is at `(1,0)` on the bottom
  edge and one with its mirror image at `(2,0)`. `intersection`, `union`, `difference` and
  `Q.partition(P)` all raise `PartitionError` ("The output's rings would mean its complement:
  the classes are wrong"), in either order of the operands. `symmetric_difference` returns. The
  dump: `self_touch_cuts 0`, `midpoint_tests 2`, `P`'s bottom pieces `v0->v9` and `v9->v1`
  against `Q`'s `v1->v8` and `v8->v0`, none common.
- With the sliver filter off the same cause gives a ring of collinear points returned as a
  polygon of zero area: `[(4,5), (5,5), (3,5), (1,5)]` in `holes-transform-1-109`'s difference,
  whose dump shows the same staggered, unpaired pieces along the shared top edge. The fuzz has 15
  differences and 30 symmetric differences with such a polygon (I traced only that one). The
  areas are right and the default filter drops them, but "zero keeps every piece" then returns
  geometry that is not a polygon.

**Suggested fix.** Let the cut take every vertex of the other input's chain that lies within the
thickness of the piece's segment and strictly inside its arc, stopping when the chain leaves the
arc's extent, not only a chain that arrives at the piece's far vertex. Both sides then get both
cuts and pair. Untested. At the least, the exception's docstring should not say what it says now
(finding 4).

### 4. medium: the docstrings and the page vouch for results that findings 1 to 3 break

These are texts this PR writes or extends, so they are this PR's to correct, whatever is decided
about the code.

- `src/api/PyExceptions.cc:493-495`: "This is rare: the only case found is two polygons that
  each have an interior ring crossing their exterior ring, with outlines running within about
  1.4e-6 radians...". The PR extends this sentence to the four new methods. Reproduction 4 is a
  second case, with valid polygons a few vertices each.
- `src/api/PyGeometriesOnSphere.cc:4710-4725` (`intersection`, which the other three methods
  refer to): the returned polygons "have interior rings where the region needs them", and
  crossing rings and an unreliable `get_area()` are promised only "where either polygon's rings
  cross each other or themselves (an invalid polygon...)". Reproductions 1 to 3 get both from
  polygons whose rings only touch. The docstring's own example sums `get_area()` over
  `continental_polygon.intersection(plate_polygon)`, which is what reproduction 1 gets wrong.
  The same sentence is in `partition`'s docstring (4637).
- `docs/design/architecture/foundation/polygon-overlay-overview.md:341`, the sentence quoted in
  finding 1.
- `spherical-geometry.md`, section 7's deferred item (2884-2886): true of the hunt, but it reads
  as a bound on the defect, and the hunt has no generator that could reach it (finding 6).

**Suggested fix.** If findings 1 to 3 are not fixed in this PR: one sentence in `intersection`'s
docstring (and `partition`'s) that the same holds, for now, where a polygon's rings touch each
other at a point on the other polygon's outline; drop "the only case found" from `PartitionError`;
and record findings 1 to 3 in section 7 with their reproductions.

### 5. medium (performance): classification is quadratic in the length of a shared edge

`src/maths/PolygonOverlay.cc:2523-2564` (`deduce_to_fixed_point`). Not changed by this PR, and
`partition()` pays it too, but merging polygons that share an edge is what `union()` is for.

**What is wrong.** Each sweep visits every vertex and calls `solve_vertex`, and sweeps repeat
until one changes nothing. Along a common chain the number of sweeps grows with the chain's
length, so a chain of `m` pieces costs in proportion to `m` sweeps of `m` vertices. (I take it
that a class travels only a few vertices per sweep where the chain runs against the vertex
order; I measured the sweeps, not the reason.)

**Evidence.** Two polygons sharing an edge of `m` identical vertices along a meridian, any of
the four methods or `partition()`, either way round:

| `m` | time |
| --- | --- |
| 2,000 | 0.2 s |
| 5,000 | 2.0 s |
| 10,000 | 8.0 s |
| 20,000 | 33 s |

The scratch build's counter: 93 sweeps for 1,001 vertices, 191 for 2,001. For comparison, two
identical 20,000-point rings (an anchorless chain, where the class happens to travel with the
vertex order) take 0.1 s, and two overlapping 20,000-point caps 0.02 s.

**Suggested fix.** A work list: after a deduction at a vertex, revisit only the vertices at the
other ends of the pieces just classified. Or classify a common chain as a whole once one of its
pieces has a class (`get_common_chain` already walks one).

### 6. medium (test gap): no test has an input touching itself on the other's outline at scale

Every one of the fuzz's 337 failing operations (sliver filter off, operands as generated) comes
from a pair in which a polygon touches itself; there are none in the 17,925 pairs without. The
hunt reports no crossing group and no area off for valid inputs because its self-touching cases
are at the thickness scale (`d4`, `zero`, `six`), and the prototype shares the one-passage
rule's order, so the comparison cannot see finding 1 either. `check_areas` and the
crossing-group expectation in `PolygonOverlayTest.cc` would have caught all of findings 1 to 3
had such an input been among their cases.

**Suggested fix.** Add reproductions 1 to 4 as tests (expecting today's behaviour only if it is
recorded as a known limitation). For the hunt, a generator of small grid polygons under a
gnomonic projection, with holes that share a vertex or put a vertex on an edge, checked by the
area identities and the crossing-group count, needs no outside oracle.

### 7. low: a test comment says the opposite of what the test shows

`pygplates/test/test_maths/test_geometries_on_sphere.py:2046-2047`: "the gap is a sliver hole,
dropped by default". The gap is `5e-6` radians wide, over three times the default width, and
`p.union(q)` keeps the hole (I ran it: `[1]` by default and at zero, `[0]` at `1.4142e-5`). The
assertions are right: they pass `0` and `1.4142e-5`. Say "dropped at ten times the default
width", as the intersection case above it does.

### 8. low: three new test lines are wider than 100 columns

`src/unit-test/PolygonOverlayPrototypeTest.cc:1626` (106), `PolygonOverlayTest.cc:2892` (105)
and `2904` (103), counting a tab as 4. All in code the PR adds, so no older neighbour licenses
them. Otherwise the added C++ matches `AGENTS.md`: hard tabs, Allman braces, one parameter per
line, `d_` members, pointer and reference tokens on the name.

### 9. low: commits to tidy before the pull request opens

- Three `fixup!` commits are still unfolded: `f01694b34`, `e0a676d8b` (both into `26351dba6`)
  and `aa9cede39` (into `f1a545b5e`).
- `3ea862783`'s body ends "The evidence is in
  docs/plans/polygon-intersections/prC-symdiff-twist.md" (also its one line over 72 columns,
  at 73). The plan directory is deleted before the merge, so the permanent record will point at
  a file that `main` never has. Point at section 5.2 and section 6 of the design page, where the
  evidence was distilled.
- Otherwise the messages meet `AGENTS.md`: imperative subjects of 26 to 67 characters, bodies
  that say why and what was rejected.

### 10. low: the grouping is quadratic in the number of output rings, and a union can have many

`src/maths/PolygonOverlay.cc:5352-5409` ("The number of rings is small: every pair is tested").
True of partitioning's outputs. A union of two polygons whose shared boundary was digitised
twice has a hole for every enclosed gap: a comb of 5,000 teeth against a straight edge takes
2.0 s in `union()` and `symmetric_difference()` (5,000 holes) against 0.05 s for the
intersection and the difference; 500 teeth take 0.02 s. A bounding-circle prefilter before
`do_rings_cross` and `is_ring_inside_ring` would do.

### 11. low: `walks_chain` is linear, inside a loop over pairs

`src/maths/PolygonOverlay.cc:4972-4982` and `5090-5106`. For a ring that walks `m` consecutive
pieces of an unkept chain (after a cut at a long chain, or along an unjoined wall in the
symmetric difference), every pair of its points on the chain calls `walks_chain`, which walks
from one to the other before it answers: about `m^3 / 6` steps. I could not make a case with
large `m` (the cut needs thickness-scale geometry, and the fuzz's walked chains are a few pieces
long), so this is a hazard on reading only. A prefix count of chain steps per ring would make
the test constant.

## 3. Checked and found sound

- **`join_difference_rings`.** Each kept piece is walked once by each difference (the pairing is
  an involution, so a walk is a cycle), so a wall piece has one step in each output; the chain
  paths a cut adds are same-side pieces, never walls, so overwriting `piece_steps` for them is
  harmless. A common piece has distinct end vertices and no interior stop (`find_common_pairs`),
  so `step_end_at` is well defined and a chain path needs no interior points. Each join takes two
  ends out of the linking and links their partners, which keeps `link` an involution on the ends
  that remain, in any order of the joins and where two joined pieces meet; so the final walk's
  `do ... while` returns to its start. A wall with other than two ends is joined without the
  spike test, which is the safe side.
- **`split_walked_rings`.** `chain_path` cannot miss: the breadth-first search walks the same
  graph the union-find joined, and the two vertices differ (the repeated-vertex cut ran first).
  The split ends: a cut at a chain needs a non-chain step on both sides and leaves each part
  with fewer, and a ring that walks the chain between the two points is not tested again.
  `crosses_arc`'s walk stops at the other passage, so it ends; its `beyond` values are always
  set before use; the sector test's algebra is right (the sign of `[a, b, corner]` is the turn
  from one tangent to the other). A zero-length other arc gets an arbitrary `leaving_sign`, which
  can only make or miss a cut, and either reads the same by parity.
- **The tests pin the cut.** With the cut at a chain switched off, `LinkTwistAtChain`,
  `CutsAtChainsAndSpikeWalls`, `Catalogue` and `Hunt` fail.
- **The entry point and bindings.** `extract_polygons`' overlay outlives nothing it refers to;
  the bindings hold both polygons for the call, and the temporary partitioner lives to the end of
  its expression. Arguments: `None`, `0`, a negative value, NaN, infinity, a non-polygon, `None`
  for the polygon, keywords. `intersection()` and `difference()` equal `partition()`'s lists,
  right or wrong, in all 32,000 comparisons I made (8,000 fuzz pairs, both ways round, default
  filter and filter off; the one pair that raises does so in both). A whole result is the input
  itself.
- **Inputs that do not touch themselves.** 17,925 fuzz pairs, four methods, 8,913 of them also
  with the operands swapped and 9,012 with the default sliver filter: no wrong area, no sample
  point misread, no point in two polygons, no exception. The numbers of holes differ from
  shapely's in a few hundred, all with the right area and membership (a region touching itself
  at a point has more than one valid description).
- **Hemisphere cases.** Two half belts round the equator: the union and the symmetric difference
  raise, the intersection and the difference return. Two 89-degree caps opposite each other, an
  85-degree cap with a far small one, two 60-degree caps 100 degrees apart: all return the right
  number of polygons.
- **Partitioning is otherwise unchanged.** The restructuring moves the pairing and the walk
  without altering them; with the cut switched off, no fuzz result changes in any method.
- **Platform.** The new decisions compare with the thickness (`1.4e-6`) or with `1e-12` radians
  quantities whose rounding is `1e-16`, and the new tests keep their constructed margins at 4% of
  the thickness or more, so I see no decision of the PR's own that a contraction or another
  `atan2` would flip. (The exception is not new: finding 2's midpoint classes.) Line endings: LF
  in the index for every file touched; `git diff --check` is clean.

## 4. Not checked

- Linux, macOS, arm64, GCC and Clang: nothing was built but with MSVC. The Qt5 build. The docs
  build.
- `PolygonOverlayRealDataTest` (its data variable is not set here).
- The base of the comparison by a build of `aeb027de7`: the `gplates` worktree's pyGPlates build
  is from September. "On `main` already" rests on the diff (classification, pairing and the
  self-touch cuts are untouched but for comments) and on the switch that turns the cut off.
- The prototype, which is outside the repository: that it shares finding 1 is inferred from the
  comparison passing on `one_passage_moved` and failing with the candidate change.
- Whether the suggested fixes for findings 2 and 3 work. Only finding 1's was built and run.
- The documentation beyond what the code change could make untrue (the claim check covered it).

## 5. Reproductions and the candidate change

Run with `PYTHONPATH=build-pygplates/bin`. Polygons are given on a plane grid and mapped by a
gnomonic projection, so that a vertex on a grid line through two others lies on their arc.

```python
import math
import pygplates

CENTRE = 1.5  # the grid point mapped to (1, 0, 0)

def point(x, y):
    v = (1.0, 0.02 * (x - CENTRE), 0.02 * (y - CENTRE))
    n = math.sqrt(sum(c * c for c in v))
    return pygplates.PointOnSphere(v[0] / n, v[1] / n, v[2] / n)

def polygon(exterior, *holes):
    return pygplates.PolygonOnSphere(
            [point(*p) for p in exterior], [[point(*p) for p in h] for h in holes])

def area(polygons):
    return sum(p.get_area() for p in polygons)

unit = polygon([(0, 0), (1, 0), (1, 1), (0, 1)]).get_area()

# 1. Finding 1. Expected: two pieces, 6 * unit = 0.0024. Got: one polygon of two crossing rings,
#    area 4.4e-16. Q.partition(P) puts the same polygon inside.
P = polygon([(0, 1), (0, 2), (0, 3), (1, 3), (2, 3), (3, 3), (3, 2), (3, 1), (2, 1), (2, 0),
             (1, 0), (0, 0)], [(2, 2), (1, 2), (1, 1), (2, 1)])
Q = polygon([(3, 2), (3, 1), (3, 0), (2, 0), (1, 0), (0, 0), (0, 1), (0, 2), (1, 2), (1, 3),
             (2, 3), (3, 3)], [(1, 1), (2, 1), (2, 2), (1, 2)])
print(1, len(P.intersection(Q)), area(P.intersection(Q)), 6 * unit)

# 2. Finding 2. Expected area 0.00487. Got: one ring through (1, 3) twice, area 0.01007.
CENTRE = 2.5
P = polygon([(0, 0), (1, 0), (5, 0), (5, 1), (4, 5), (3, 5), (1, 3)],
            [(1, 1), (4, 1), (3, 4), (1, 3)])
Q = polygon([(1, 1), (1, 4), (0, 5)])
print(2, area(P.union(Q)), P.get_area() + Q.get_area() - area(P.intersection(Q)))

# 3. Finding 2. Expected: four polygons, area 0.00280. Got: three, one with the ring
#    (3,2), (3,1), (3,2), (3,3), (4,3) as a hole, area 0.00240. With CENTRE = 0.0: four polygons
#    of the right area, one with the spike (3,2), (3,1), (3,2).
CENTRE = 1.5
P = polygon([(0, 1), (1, 0), (3, 0), (2, 1), (3, 1), (3, 3), (0, 3), (0, 2)],
            [(2, 1), (2, 2), (1, 2)])
Q = polygon([(1, 2), (2, 3), (4, 3), (3, 2), (4, 2), (4, 0), (1, 0), (1, 1)],
            [(3, 2), (3, 1), (2, 1)])
print(3, area(P.symmetric_difference(Q)), area(P.difference(Q)) + area(Q.difference(P)))

# 4. Finding 3. intersection, union, difference and Q.partition(P) raise PartitionError.
P = polygon([(0, 0), (3, 0), (3, 3), (2, 3), (0, 3), (0, 2)], [(1, 0), (2, 1), (2, 2)])
Q = polygon([(3, 0), (0, 0), (0, 3), (1, 3), (3, 3), (3, 2)], [(2, 0), (1, 1), (1, 2)])
P.intersection(Q)
```

The candidate change for finding 1, as built and run (`solve_vertex`, at line 2417):

```diff
 	unsigned int chosen = consistent_orders.front();
-	if (!once && vertex.bundle && consistent_orders.size() > 1)
+	if ((once || vertex.bundle) && consistent_orders.size() > 1)
 	{
 		unsigned int fewest = inversions(orders[chosen]);
```

The fuzz's counts, for the record (24,000 pairs, sliver filter off, operands as generated;
6,075 pairs have a polygon that touches itself, and every failure is among them):

| | intersection | union | difference | symmetric difference |
| --- | --- | --- | --- | --- |
| wrong total area | 193 | 55 | 15 | 26 |
| right area, a polygon of zero area returned | 0 | 0 | 15 | 30 |
| `PartitionError` | 1 | 1 | 1 | 0 |
| wrong total area, with finding 1's candidate | 2 | 55 | 14 | 24 |
| sample point misread, or in two polygons | 0 | 0 | 0 | 0 |
