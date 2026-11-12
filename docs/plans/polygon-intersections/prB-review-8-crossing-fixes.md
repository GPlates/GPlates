# PR B review 8: the fixes to review 7 (midpoints, the tree walk, bow-ties, `whole_checks`, `r5p`)

Reviewed 2026-11-12 (Fable), adversarially, at the tip `7b09dedc1`: the five commits `52bb6f4cd` ("Catch
crossings through shared vertices; keep bow-ties"), `a7fef5779` ("Walk a ring's bounding tree in the crossing
test"), `84780ca77` ("Document crossing rings as review 7 found them"), `b14bf9bb8` ("Count a whole answer's
check, so classify()'s call is pinned") and `e5b756b30` ("Compare polylines against the prototype's r5p"),
with their messages and `PLAN.md`'s two Status entries of 2026-11-12 treated as claims to check. Read in full
around every hunk: `src/maths/PolygonOverlay.{h,cc}` (`do_arcs_cross`, `do_ring_nodes_cross`,
`is_ring_inside_ring`, `do_rings_cross`, `does_ring_cross_itself`, `get_mean_width`, `classify`,
`find_reference_point`, `check_whole_result`, `apply_loop_and_spike_rules`, `classify_polyline_pieces`, and
`extract` from the hemisphere check to its end), `src/maths/PolygonPartitioner.cc` (`is_outside_by_bounds`,
`overlay_polygon`, `overlay_polyline`), `SmallCircleBounds.{h,cc}` (the distances between circles),
`PolyGreatCircleArcBoundingTree.h` (the leaf size, the node circles), the new and changed tests in
`src/unit-test/PolygonOverlayTest.cc` (`CheckWholeResult`, `GroupRingsCross`, `GroupCrossingRings`) and
`PolygonPartitionerTest.cc` (`CrossingRings`, `SliverFilterSelfCrossingRing`), the comment in
`PolygonOverlayPrototypeTest.cc`, the page's sections 5.2, 5.4, 5.5, 5.6, 6 and 7, both changelog bullets, the
`partition` docstring, and, in the planning repository at `8333d35`, `overlay_proto.py` (`CONFIGS`,
`_contract`), `overlay_polyline.py` (`PolylineOverlay`) and `overlay_export.py` (`POLYLINE_RULES`). Line
numbers are at `7b09dedc1`.

Evidence is marked *run* or *reading*. *Run*, all on this machine (Windows, the `gplates` conda environment):

- a temporary second crossing test inside `PolygonOverlay.cc`, the point test written as one loop over every
  vertex (the form `52bb6f4cd` had), with a switch for review 7's variant (every edge's midpoint), and
  `extract` made to ask it of **every pair of output rings** beside the committed test and to print any
  disagreement: over the generated-case hunt (217 chunks under `r5p`, 119,198 cases, three times), review 7's
  random probe (42,000 cases: generic, snapped to grids of 0.002 and 0.004 radians, and with the sliver
  filter; then again with the every-edge variant in place of the committed test), the committed tests and
  the Muller 2022 coastlines;
- the same hunt with each case's name printed, so that every ring taken as crossing itself could be put to a
  case whose input rings do or do not cross;
- new direct probes of `do_rings_cross` and `does_ring_cross_itself` on built shapes, and cost probes (review
  7's `Review7Cost`; two large rings in each other's bounding circles; a comb cut into strips; the
  self-crossing test on slivers of up to 200,000 points);
- thirty mutations of `PolygonOverlay.cc`, each rebuilt and run against the committed
  `PolygonOverlay*:PolygonPartitioner*:PartitionFeatureUtils*` tests;
- five pyGPlates scripts importing `build-pygplates/bin` (built at the tip before anything was touched):
  review 7's `py_chord.py` and `py_bowtie.py` again, `py_pinwheel.py` (1.1), `py_bowtie8.py` (1.2 and 1.3)
  and `py_bowtie_plate.py` (a bow-tie as the plate);
- the prototype's exporter, run again under `r5p` and under `r5o` for the committed polyline sample;
- a syntax-only compile of both precompiled-header files followed by the enum of `do_rings_cross` (1.7).

Everything temporary was removed and the files' timestamps updated: the two test files by `git checkout`,
and `PolygonOverlay.cc` from a copy taken before the first change, since its working copy has LF line endings
and a checkout here writes CRLF (`git diff` is empty either way). The three touched files
(`PolygonOverlay.cc`, `PolygonOverlayTest.cc`, `PolygonOverlayPrototypeTest.cc`) have the SHA-1 they had
before the first change, both products were rebuilt, and `git status` shows only this file. Probe sources,
logs and scripts are in the session's scratchpad under `review8/` (`instrument.py` holds the reference test,
the switches and the mutations; `probes8.cc`; `hunt8.py`, `attribute.py`; `run_random.ps1`,
`run_mutations.ps1`, `mutations.log`; `py_pinwheel.py`, `py_bowtie8.py`, `py_bowtie_plate.py`; `cmp_cases.py`,
`pch_near.py`).

Baseline and final (*run*): `gplates-unit-test --gtest_filter=PolygonOverlay*:PolygonPartitioner*:PartitionFeatureUtils*`
103 tests, 101 passed, 2 skipped (`PolygonOverlayPrototypeTest.File`, `PolygonOverlayRealDataTest.Coastlines`, by
design), before the first change and after the last restore. After the restore, GPlates `ctest` 213 of 213
(the same two skipped) and pyGPlates `ctest` 7 of 7, as claimed.

**In short.** The commits do what they say for the cases they name, and every number I re-measured holds: the
hunt has 9 differing cases and none wrong as polygons, the tree walk answers exactly as the loop it replaced
(5.3 million ring pairs, no disagreement), review 7's cost probe is back to 1.25 times the old grouping,
review 7's square-and-chord and bow-tie now come out right through pyGPlates, the whole answer's check is
pinned, and the re-exported polyline sample is the committed file case for case. What is left is at the edges
of the two fixes, for invalid inputs only. The midpoint rule looks only at edges with **both** ends near the
other ring, so a crossing in which each ring has a vertex on the other's *edge* (not on its vertex) is still
missed at any size, and the pieces still overlap (1.1). The self-crossing test is by edges only, so a bow-tie
with a vertex at its crossing is still dropped whole (1.2); the header says so, but the page, both changelogs
and the docstring say such a piece is never dropped. The rest is low: a kept bow-tie's area; tests;
documents; and one thing that reaches valid inputs, the cost of the crossing test for rings that lie in each
other's bounding circles, which the walk cannot prune (a subject cut into 100 long strips side by side spends
1.1 s in `extract` against 0.05 s without the test, 1.4).

## 1. Findings, most severe first

### 1.1 Medium: a crossing through vertices that lie on the other ring's edges is still missed, whatever its size (run)

`do_rings_cross`, `PolygonOverlay.cc:366-485`; the midpoint condition at `:460`. A midpoint is classified only
for an edge whose two ends are both within the margin of the other ring. An edge that starts on the other
ring's outline, passes through the other ring, leaves it **through one of its vertices**, and ends far
outside has one end near and one end far: neither its ends nor its midpoint give a point inside. The edge test
does not see it either, since the vertex it passes through is an end of the other ring's edges there
(`do_arcs_cross` wants all four ends clear of the other's great circle, `:217-230`). So the documents' limit,
"an overlap reached only within the margin of the outlines (thinner than the margin, or a chord shorter than
about twice the margin)" (`PolygonOverlay.h:208-209`, section 5.2 at `spherical-geometry.md:1463`, the comment
at `PolygonOverlay.cc:4624-4625`), is not the limit.

The smallest case built (*run*, `Review8Shapes` and `py_pinwheel.py`): a **pinwheel** of two four-point rings
(degrees), `a = (0,0) (1.5,0) (1,1) (-0.5,1)` and `b = (1,0) (1,1.5) (0,1) (0,-0.5)`. Each of `a`'s vertices
`(0,0)` and `(1,1)` lies on an edge of `b`, each of `b`'s `(1,0)` and `(0,1)` on an edge of `a`; each of the
four long edges runs from its own on-edge vertex, through the other ring's on-edge vertex, to a far vertex. No
vertex of either is inside the other, no edge has both ends near the other ring, and no two edges cross clear
of their ends. The rings overlap in the whole unit square. `do_rings_cross` answers false both ways round;
review 7's variant (every edge's midpoint) answers true while the edges extend less than the square's side
beyond it (the midpoint is then inside the square) and false beyond that; 16 samples an edge answer true
always. The vertices need not be exactly on the edges: moved 15 h along the square's diagonals into the other
ring (about 10 h from its edge) the answer is still false; at 30 h the vertex test sees them.

Through pyGPlates at the tip (*run*, `py_pinwheel.py`): a polygon with `a` as exterior and `b` as interior
ring (the unit square is outside it by parity, being in both rings), cut by a box that only takes the tip off
one blade. Either way round the inside comes back as **two polygons**, the two rings, which overlap in the
square: 1,687 of 58,872 sample points are in two inside pieces, and outside the pinwheel polygon by parity.
With the pinwheel as the plate the same points are also in the outside piece, so inside and outside overlap,
which is what both changelog bullets and the docstring say does not happen. The sliver width makes no
difference.

How often (*run*, review 7's random probe, 6,000 cases each at two seeds, counting as review 7 did the
operations whose polygons differ from their own rings' parity and in which the 16-samples-an-edge reference
finds a pair that `do_rings_cross` does not): never on generic inputs (0 of 12,000 cases, 644,833 ring
pairs). Snapped to a grid of 0.002 radians, none in 11,973 cases (135 in review 7's 5,985); at 0.004, 5
operations in 3 of 11,668 cases (412 operations in review 7's 5,832), 3 of them differences (seed 1 cases
1927 and 4429, seed 2 case 3283). In those the rings also share whole stretches and pass through their own
vertices (case 3283's difference has a ring that crosses itself at a vertex it shares with a square it
overlaps). The operations wrong for other reasons on the grids (coincident rings, review 7's 1.6; or no
missed pair, 9 at 0.002 and 62 at 0.004 with seed 1, against review 7's 9 and 57) are as before and were not
looked at again. The every-edge variant finds 8 more crossing pairs than the committed rule in the 24,000
grid cases, and 46 in the hunt (all in the symmetric difference of `shared` cases whose inputs cross), but
the same 5 operations stay wrong under it, and the hunt's intersections and differences are the same: **on
everything generated here the implementer's cheaper rule loses no operation that review 7's variant got
right**; it loses the pinwheel with short blades, which neither generator makes.

For two simple rings, real data needs four vertex-on-edge coincidences alternating between two rings of one
polygon (with two of the four corners shared vertices instead, the "half pinwheel" probe, the both-ends-near
rule catches it; by reading, one shared corner is enough). That is much rarer than review 7's shared
vertices, so the likelihood is low; it is ranked medium because the overlap is of any size and the documented
limit is wrong.

Confidence: high (direct calls, pyGPlates, and the reference test agree).

Fix, if wanted: an edge crosses the other ring unseen only by passing through one of its vertices, so, for an
edge of a near node with at least one near end, collect the other ring's vertices within the margin of the
edge (its bounding tree against the edge), and classify the midpoint of each stretch between them (and the
edge's ends) that is longer than twice the margin. That finds the pinwheel at any blade length and a chord
whose own midpoint lies on a vertex of the other ring. Cheaper and partial: the midpoint of every edge with
*one* near end too (finds it only while the blade is shorter than the overlap). Otherwise say it: correct
the three statements above, the changelogs' "don't overlap" and the docstring's "no two pieces overlap", and
put the miss in section 7, which no longer lists any miss of the crossing test.

### 1.2 Medium: a bow-tie with a vertex at its crossing is still dropped whole (run)

`does_ring_cross_itself`, `PolygonOverlay.cc:488-502`; `is_sliver`, `:4518-4523`. The test is the edge test
alone, so it misses a ring that crosses itself at one of its own vertices, or with a vertex within the margin
of the other edge at the crossing. The header says so (`PolygonOverlay.h:222-223`). The sliver filter then
measures the ring by its signed area as before, and review 7's finding 1.2 stands for it unchanged.

*Run* (`Review8SelfCross`, the bow-tie `(0,0) (2,2) (2,0) (0,2)` scaled, margin 10 h): with a vertex at the
crossing on both diagonals, false; on one diagonal, false; with a vertex 5 h along one diagonal from the
crossing, false; 20 h along, true. Through pyGPlates (`py_bowtie8.py`, the plate `(-1,0.5) .. (3,3)` holding
three quarters of the bow-tie): with a vertex at the crossing on one diagonal, `PolygonOnSphere.partition` at
its default width returns `outside` with the whole bow-tie in the outside list, as review 7 found for the
plain bow-tie; with a vertex on both diagonals it is cut at the default width (the lobes of that ring do not
cancel exactly) and returns `outside`, whole, at `10 h`, which is `PlatePartitioner`'s and Assign Plate IDs'
width. With `sliver_width_threshold_radians=0` both are cut correctly.

A figure of eight drawn in one stroke through a vertex is as ordinary an invalid polygon as two swapped
vertices, and like them it is dropped only where its lobes nearly cancel; so this is the defect the commit
fixed, at the same severity, for the part of it that remains. And the documents written after the fix
promise more than the code does: the page ("Nor is a ring that crosses itself", section 5.2's paragraph on
the filter, `spherical-geometry.md:1521`; "such a ring ... are never dropped", section 5.5, `:1733`), both
changelog bullets ("or a bow-tie ... Such a piece is never dropped as a sliver"), and the docstring ("it is
never dropped as a sliver", `PyGeometriesOnSphere.cc:4527`).

Confidence: high (three runs).

Fix: at a vertex of the ring within the margin of another of its vertices, or of the inside of a
non-adjacent edge, compare the cyclic order of the four directions leaving the point (the two edges at each
vertex, or the vertex's two edges and the edge's two halves): alternating means a crossing, and a touch (a
figure of eight with lobes wound alike, which must stay "not crossing": its area does not cancel) does not
alternate. Or keep the edge test and say in the page, the changelogs and the docstring what the header says.
Pin whichever: a bow-tie with a vertex at its crossing is not in any test.

### 1.3 Low: a kept bow-tie has no area, so "most overlapping" gives it to the plate holding a quarter of it (run)

`PolygonOnSphere::get_area` of a ring that crosses itself is the signed sum, so the inside piece the fix now
keeps (the bow-tie's upper three quarters, two equal lobes) has area zero. *Run* (`py_bowtie8.py`): with plate
7 over the upper three quarters and plate 9 over the lower quarter, `PartitionMethod.split_into_plates` gives
plate 7 a six-point piece of area 0.0000 square degrees and plate 9 two triangles of 0.1251 each, and
`most_overlapping_plate` assigns the bow-tie to **plate 9**. A rectangle of the same extent goes to plate 7.
Before the fix the piece was dropped whole (review 7), so plate 7 held nothing of it then either, and this is
not a regression; but the changelog bullets now say such a piece's area "is taken as its outer boundary's
less its holes'" (GPlates) and "is its exterior ring's less its interior rings'" (pyGPlates), which for a
bow-tie reads as a usable number and is zero. Section 5.4's paragraph (`spherical-geometry.md:1680-1684`)
speaks only of rings crossing each other.

Confidence: high. Fix: none cheap (the parity area needs the ring intersected with itself); add to section
5.4 and the two bullets that a ring crossing itself is measured by its signed area, in which lobes wound
opposite ways cancel.

### 1.4 Low: the walk prunes by the other ring's bounding circle only, so rings in each other's circles still cost every vertex of both, for every pair (run)

`has_points_on_both_sides`, `PolygonOverlay.cc:428-465`. A node is skipped when its circle is farther than the
margin from the other ring's *bounding circle*. That bounds the cost of a large ring against a small one,
which is what review 7 measured and what the commit fixes. *Run*, `Review7Cost` in one binary with the
committed test, with `do_rings_cross` returning false at once, and with the one-loop point test: 20,000
points and 341 rings, `extract(INTERSECTION)` 21 to 23 ms, 16 to 18 ms and 120 to 143 ms; 100,000 points and
1,481 rings, 221 to 233 ms, 177 to 183 ms and 2,733 to 2,842 ms. So the commit's numbers hold, and "now about
1.3 times" (section 5.2) is 1.25 here.

It does not bound a pair of rings each inside the other's bounding circle. Every node is then "near", and
every vertex of both rings gets a distance query and a point-in-polygon test, as in the loop. *Run*:

- Two rings (`Review8WalkCost`): two wavy squares of 20,000 points each, one 0.95 the size of the other (an
  annulus's rings): 14 ms a call warm, the same as the one-loop test, and 30 ms the first time, when the
  rings' structures are built, which is every time in `extract` (its ring polygons are new). The old
  grouping's test of the pair, `is_ring_inside_ring`, takes 0.14 ms. As one cut, that annulus with a corner
  taken off by a box: `extract(DIFFERENCE)` 45 to 48 ms against 17 to 20 ms with the crossing test off; with
  100,000 points a ring, 241 to 245 ms against 78 to 90 ms.
- Many rings (`Review8Strips`): a comb of long fingers of 1,000 points each, cut from its spine by a box, so
  that the inside is that many long rings side by side, each within the others' bounding circles.
  `extract(INTERSECTION)`: 20 fingers, 46 to 47 ms against 8 to 9 ms with the test off; 50 fingers, 268 to
  273 ms against 19 to 22 ms (279 to 286 ms with the one-loop test: the walk gains nothing here); 100
  fingers, 1,085 to 1,088 ms against 41 to 47 ms, where `classify()` takes 19 to 30 ms; 50 fingers of 4,000
  points, 1,071 to 1,079 ms against 79 to 88 ms. Four times the cost for twice the rings, and for four times
  the points.

So the test is linear in the points of every pair of rings that lie in each other's bounding circles, and
quadratic in the number of such rings: at 100 strips `extract` is 25 times its old cost, a worse ratio than
the one the commit fixed, on a valid input. Section 5.2's "a node of one ring farther than the margin from the
other's bounding circle is outside it at once, so a large exterior is not walked vertex by vertex for each of
its many holes" is true as far as it goes. Holes close to a large ring's outline are no trouble (*reading*:
only the arcs under each hole's circle are visited). The Muller 2022 coastlines do not reach any of this
(*run*, `PolygonOverlayRealDataTest.Coastlines` at 0, 100 and 200 Ma: 912 ring pairs in all, none crossing;
the test takes 5.4 s with the crossing test on, off or as one loop).

Confidence: high for the measurements; low that current data reaches the quadratic case (it takes one cut
that leaves many long pieces side by side). Fix if wanted: descend both trees, as the edge test does. A node
of `a` whose circle is farther than the margin from every leaf circle of `b` has all its vertices on one side
of `b` and clear of its outline, so one point-in-polygon test stands for all of them; the cost is then that
of the stretches where the two outlines really run near each other.

### 1.5 Low: what the tests pin, by mutation (run)

Thirty mutations of `PolygonOverlay.cc` (*run*, `mutations.log`), each against the committed tests:

| Mutation | Committed tests that fail |
|---|---|
| midpoints not classified | `GroupRingsCross`, `GroupCrossingRings`, `PolygonPartitionerTest.CrossingRings` |
| a midpoint classified only where the edge's end is *not* near | the same three |
| the edge's start classified in place of its midpoint | the same three |
| every edge's midpoint classified (review 7's variant) | **none fails** |
| the point test one way round only | **none fails** (as the plan says) |
| the point test removed | those three, `PolygonOverlayPrototypeTest.Bundle` and `.Hunt` |
| a far node not counted as outside | **none fails** |
| a far node counted as inside | `.Bundle`, `.Hunt`, `UnionOverHemisphereThrows` |
| a node far at any distance from the bounding circle (the margin zero) | **none fails** |
| no node ever far (the shortcut off) | none fails, rightly: speed only |
| the walk descends into a node's first child only | `.Hunt` only |
| a vertex outside the bounding circle not counted as outside | `.Bundle` only |
| the point test's margin zero | 8 tests |
| the self-crossing test always false | `GroupRingsCross`, `SliverFilterSelfCrossingRing` |
| the self-crossing test always true | `GroupRingsCross` and five `SliverFilter*` tests |
| the self-crossing test's margin zero | `GroupRingsCross`, `SliverFilterHoleWithIsland` |
| the filter does not ask it of an exterior | `SliverFilterSelfCrossingRing` |
| the filter does not ask it of a hole | **none fails** |
| the sliver filter applied to groups | **none fails** (as the plan says) |
| the nesting check always true | **none fails** (as the plan says) |
| a hole group refused its hole parent's polygon | `GroupCrossingRings` (none in review 7) |
| depth counted by groups, not rings | `GroupCrossingRings` (none in review 7) |
| holes appended in reverse order | **none fails** (as the plan says) |
| `do_arcs_cross` without its same-crossing test | **none fails** (as the plan says) |
| the edge test removed | `GroupRingsCross`, `GroupCrossingRings`, `.Hunt` |
| `classify()` does not call `check_whole_result` | `CheckWholeResult` |
| it calls it for an inside answer only, or for an outside answer only | `CheckWholeResult` |
| it calls it twice | `CheckWholeResult` |
| `whole_checks` counted for a skipped check too | **none fails** |

So `52bb6f4cd`'s "each mutation fails one" holds for what it lists (the midpoints, groups three deep, the
partition layer, the bow-tie), `b14bf9bb8`'s for the call, and the plan's list of what is still unpinned is
right. Besides that list:

- **The walk is held only by the prototype samples, and its shortcut by nothing.** The hand-built rings are
  small (a leaf holds eight edges), and no hand-built test fails when the walk descends into one child only,
  so the descent and the far-node test are held only by `.Hunt` and `.Bundle`. A far node not counted as
  outside passes every test, yet the count is needed: the walk adds an edge's *start* only, so where a
  leaf's last edge runs from a near vertex to a far one that begins a far leaf, that vertex is counted
  nowhere else. Its correctness rests on section 2's comparison. A test that keeps the one-loop form (thirty
  lines) and compares it with `do_rings_cross` over a few hundred generated pairs of rings of more than eight
  edges would hold it.
- **The exemption's hole path**: a bow-tie as the partitioning polygon inside a subject, whose outside piece
  has the bow-tie as a hole that measures as a sliver. Right at the tip (section 2), not in a test; filling
  that hole would put the bow-tie in both the inside and the outside.
- **A skipped whole check**: no test has a subject with no candidate point (`whole_check_skipped` is only
  ever expected to be 0), so nothing holds `whole_checks` against counting skips.
- The both-ends-near rule cannot be told from every edge's midpoint by any committed test (nor need it be,
  unless 1.1 is settled by sampling more).
- "The closing edge's midpoint" of `52bb6f4cd`'s message and of `GroupRingsCross`'s comment
  (`PolygonOverlayTest.cc:2034-2036`) was a code path for one commit only: the walk has no closing-edge case.
  The assertions are still worth having; the comment could say they are the same rings started elsewhere.

Suggested pins, in order of worth: the one-loop comparison above; a pinwheel and a bow-tie with a vertex at
its crossing, whichever way 1.1 and 1.2 are settled (as crossings, or as the documented misses); the bow-tie
as the partitioning polygon; a subject too thin for a whole check (`whole_checks == 0`,
`whole_check_skipped == 1`).

### 1.6 Low: documents (reading, against the runs above)

- `PolygonOverlay.h:208-209`, section 5.2 (`spherical-geometry.md:1463-1464`), `PolygonOverlay.cc:4624-4625`,
  `PolygonOverlay.h:1007-1008`: the crossing test's miss is put at overlaps reached only within the margin.
  See 1.1.
- Both changelog bullets and the docstring (`PyGeometriesOnSphere.cc:4522-4527`): "don't overlap" / "no two
  pieces overlap" (1.1), "never dropped as a sliver" (1.2), the area (1.3). Besides 1.2, a group of crossing
  rings *is* dropped when the ring it lies in is dropped as a sliver (`PolygonOverlay.cc:4546-4549`, and
  section 5.2's own "a group inside a dropped group is dropped with it"), so "never" needs "itself", as the
  code comment has it. `spherical-geometry.md:1733` has the same "never dropped".
- Section 7 (`spherical-geometry.md:2360-2369`) now lists no miss of the crossing test: the three hunt cases
  are gone, rightly, but the limit that section 5.2 states (and 1.1's) is not carried there as deferred work.
- Section 5.2, `spherical-geometry.md:1474-1475`: "Every pair of rings whose bounding circles meet is tested,
  by containment one point each." Two things are run together: every pair not already in one group is given
  to the crossing test, which returns at once if the circles do not meet; and containment is asked of each
  group's first ring against every other ring, whatever their circles, at one point. Nothing says what a
  pair whose circles do meet costs (1.4).
- Section 5.2, `:1458`: "Both tests descend the rings' bounding trees". The point test walks one ring's
  tree against the other ring's bounding circle, not against its tree; the edge test descends both. The
  difference is 1.4.

Checked and right: section 5.2's account of the points classified (vertices, and midpoints of edges with both
ends near) and of the node shortcut; "twelve times ... now about 1.3 times"; section 5.4's two sentences
(exterior-ring-only coverages by reading only, as in review 7; the 0.0121 against 0.0304 steradians are review
7's); the filter paragraph's "asked only of a ring that measures as a sliver" (`&&` short-circuits at
`PolygonOverlay.cc:4520-4522`; counted: the committed tests ask 65 times, of which 12 are true, all the
bow-tie's; the hunt's unfiltered extracts never ask); section 5.6; section 6's
test lists (every test named exists and does what is said) and its numbers (9 cases, in the three kinds
given; 70,291; none wrong as polygons); section 6's and the test comment's account of `r5p`; section 7's
two new invalid-input items (5 of 5,985 and 52 of 5,832 reproduce exactly); the header comments of
`does_ring_cross_itself`, `check_whole_result` and `Diagnostics::whole_checks`; the commit messages'
numbers.

### 1.7 Low: `NEAR` is a macro wherever `<windows.h>` is included (run for today's builds)

`PolygonOverlay.cc:386`, `enum PointSide { NEAR, INSIDE, OUTSIDE };`. `<minwindef.h>` defines `NEAR` (and
`near`, `FAR`, `far`) as empty macros, so the enum does not compile after `<windows.h>`. It compiles today
because nothing this file includes brings that header in: *run*, a syntax-only compile of each
precompiled-header file (`gplates-lib_pch.h`, `pygplates_pch.h`, for a build with
`GPLATES_USE_PRECOMPILED_HEADERS`) followed by the enum passes under Qt6, and the same with `<windows.h>`
included first fails. Not checked under Qt5, whose OpenGL headers include `<windows.h>` in some versions. A
later include, or a Qt5 precompiled-header build, would break it with an error far from its cause. Rename the
enumerators (`SIDE_NEAR`, say).

## 2. Checked and found sound

- **The tree walk is the loop** (*reading*, then *run*). The points classified are the same set: the leaves
  hold every arc of the ring once, the closing arc among them (`PolygonOnSphere.cc:960`), and each arc
  carries both its ends, so an edge whose end lies in another node, or in a far node, is handled as any
  other; a far node's circle bounds its arcs (`PolyGreatCircleArcBoundingTree.h:698-702`), so each of its
  vertices is one the loop's own bounding-circle test called outside, and none of its edges has a near end;
  the answer is "an inside point and an outside point exist", so the order of visiting does not matter.
  *Run*: the committed test against the one-loop test on every pair of output rings: 3,180,263 pairs in the
  hunt (116,960 crossing), 2,128,166 in the random probe (generic, both grids, the sliver run), 11,933 in the
  committed tests, 912 in the coastlines: no disagreement. The mutation that descends into one child only
  fails the committed `PolygonOverlayPrototypeTest.Hunt` (1.5), so rings of more than one leaf are reached.
- **The midpoint rule gives no false crossing** (*reading* and *run*). Two rings that do not cross have every
  point of one, off the other's outline, on one side of it, so a point farther than the margin on the wrong
  side needs an overlap deeper than the margin, whatever point is sampled. *Run*: the hunt reports no
  crossing group from inputs whose rings do not cross outside the symmetric difference (39,320 polygon
  cases), and its 9 differing cases are the 9 known; direct probes: a hole along its exterior's edge with
  vertices of its own on it, a hole on three of its exterior's corners, corners overlapping by 1.6 h, and two
  edges weaving within 1 h of each other all answer false, a hole's chord across a notch of its exterior
  true; review 7's square and chord flips between a side of 19 h and 21 h, as "a chord shorter than about
  twice the margin" says (the midpoint is half a side from the outline).
- **Review 7's two cases through pyGPlates** (*run*): `py_chord.py`, one inside piece with one interior ring
  either way round, 0 of 97,370 samples wrong or in two pieces; `py_bowtie.py`, cut at every width, and
  `PlatePartitioner` gives plate 7 the six-point piece. A bow-tie as the *plate* (`py_bowtie_plate.py`, inside
  a box and across one): the outside piece keeps the bow-tie as its hole at every width up to 0.001 radians,
  and no sample of about 39,000 is in both or neither, so the filter's hole path takes the same decision as
  its exterior path.
- **The hunt** (*run*, three times): 217 chunks, 119,198 cases, 9 differ (4 with coincident rings, 4 at the
  second step with no point's provenance wrong, `shared#2932 seed 2` in every tie order), 70,291 with crossing
  inputs, none wrong as polygons. No polyline chunk differs.
- **The sliver filter with the self-crossing test** (*run*). Review 7's probe with `sliver_min_width = 10 h`:
  0 of 6,000 cases where the polygons lack a region the rings have (100 before), the test asked 816 times and
  true 206. Over the hunt, 379,097 output rings measure under 10 h, 964 of them cross themselves by the test,
  **all in cases whose input rings cross**: no ring of the 39,320 polygon cases with valid inputs is taken as
  crossing itself, so the filter is unchanged for them. Direct probes: a square, a triangle of 120-degree
  edges, a figure of eight touching at a vertex with lobes wound alike, a spike out and back, a weaving
  sliver 0.8 h wide and a lobe 5 h tall answer false; a bow-tie with duplicate points, a pentagram of radius
  1.5 and a heptagram with edges near 170 degrees true. (A spike that runs out through the ring's far edge and
  back answers true: two of its edges do cross that edge.) No way found for inside and outside to overlap or
  leave a gap through the exemption: both extracts ask the same ring.
- **The self-crossing test's cost** (*run*, `Review8SelfCrossCost`): a straight sliver 0.5 h wide, 124 ms at
  200,000 points and 17 ms at 20,000 (the mean width itself takes 9 and 1.4 ms); along a random walk 180 and
  16 ms; folded back and forth into a square, the worst built, 1,072 and 70 ms. About linear, ten times the
  measurement, and paid only by a ring that already measures as a sliver.
- **`whole_checks`** (*reading* and *run*). Counted after the skip and before the comparison
  (`PolygonOverlay.cc:2463`), so it is the number of checks made. `classify()` is the only caller outside the
  tests, and has one return for a polygon subject (`:2360-2366`); a polyline returns earlier by design (the
  check needs a polygon subject). The whole answers that `overlay_polygon` makes itself are a different
  kind: outside by bounds without an overlay (`PolygonPartitioner.cc:116-121`), and a side left empty by
  clean-up or the filter after both extracts (`:144-158`), whose hemisphere check is `extract`'s (at a point
  that need not be in the subject). Mutations: without the call, or with it for one answer only,
  `CheckWholeResult` fails (1.5).
- **`r5p`** (*reading* and *run*). The spike rule: `_contract` skips the rings of a polygon named in
  `spike_rule_skip_polys`, which `PolylineOverlay` sets to the polyline alone; the C++ gives a ring that is
  not closed no pairs (`PolygonOverlay.cc:1216-1218`). The along rule: both collect the clip's pieces that
  are not closed, not internal, without an interior vertex and with a vertex at each end, by their unordered
  pair of vertices, and make along a polyline piece that is not internal, has no interior vertex, joins two
  different vertices and matches one (`:2496-2521`; `PolylineOverlay.classify`); both then deduce nothing
  from or to an along piece. A polygon subject is untouched (`r5p`'s options are `r5o`'s, word for word).
  *Run*: the exporter under `r5p` on the committed sample's 189 cases writes the committed file case for case
  (the cases come in another order, as given on the command line); under `r5o` it writes the file as it was
  before `e5b756b30`; the two differ in two cases, `along#14` and `opened-shared#5`, where a piece tested at
  its midpoint is now along.
- **The suites** (*run*): counts in the header.

## 3. What remains unpinned

- Everything marked "none fails" in 1.5's table.
- The crossing test on anything like 1.1's pinwheel, and the self-crossing test on a ring that crosses itself
  at a vertex (1.2): no test has either, and the committed tests cannot tell the both-ends-near rule from
  every edge's midpoint.
- The walk's far-node shortcut: only its equivalence run here says it is right; no committed input has a
  crossing whose only outside points are in far nodes.
- The cost (1.4): nothing committed times `extract`, and the Muller 2022 timing in the plan was not re-run as
  an A/B of two builds (the coastline test's total is the same with the crossing test off).
- 1.3, and the exterior-ring-only coverage sentence of section 5.4 (by reading only, as in review 7).
- Not looked at: a Linux build of the tip; Qt5 (1.7 is the only Qt-sensitive thing seen).
