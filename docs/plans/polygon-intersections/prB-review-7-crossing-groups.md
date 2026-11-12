# PR B review 7: crossing output rings grouped into one polygon

Reviewed 2026-11-12 (Fable), adversarially: the one commit `1dc10e9da` ("Put crossing output rings into one
polygon") against its parent `a58eb0ddb`, with the commit message and `PLAN.md`'s Status entry of 2026-11-12
(at `1ce6ed73b`) treated as claims to check. Read in full around every hunk: `src/maths/PolygonOverlay.{h,cc}`
(`do_arcs_cross`, `do_ring_nodes_cross`, `is_ring_inside_ring`, `do_rings_cross`, `get_mean_width`, and
`Overlay::extract` from the hemisphere check to its end), `src/maths/PolygonPartitioner.cc`
(`overlay_polygon`, `create_polygon`, `partition_polygon`), `src/maths/PolygonOnSphere.cc` (area, bounding
circle, point-in-polygon), `PointInPolygon.cc` (`calculate_interior_reference`), `GeometryDistance.cc` (point to
polygon), `SmallCircleBounds.h` (the builders' `add`), `SphericalArea.cc` (the ring combination),
`src/app-logic/PartitionFeatureUtils.cc` (the size measure), `PartitionFeatureGeometries.cc`
(`partition_range`), `src/opengl/GLReconstructedStaticPolygonMeshes.cc` (the node test),
`src/qt-resources/python/api/PlatePartitioning.py` (most overlapping), the `partition` docstring in
`src/api/PyGeometriesOnSphere.cc`, `src/unit-test/PolygonOverlayTest.cc` (`check_oracle`, the `Group*` tests),
`PolygonOverlayPrototypeTest.cc` (the oracle block), `PolygonOverlayOracle.h`, the page's sections 5.2, 5.5,
5.6, 6 and 7 with its constants table, and both changelogs. Line numbers are at `1dc10e9da`.

Evidence is marked *run* or *reading*. *Run*, all on this machine (Windows, the `gplates` conda environment):

- the generated-case hunt (218 files, 119,198 cases) on the committed code, on two variants of it (below), and a
  second time with **the inputs swapped** (the clip as subject, intersection and difference only: 109,613
  polygon cases);
- a temporary random probe appended to `PolygonOverlayTest.cc`: polygons of one to six rings each (star-shaped
  rings of three to six points, placed and sized at random so that they cross and nest), every operation read
  as polygons (the union over the polygons, each by parity) against the parity over the rings and against
  `f(in P, in Q)`, at up to 1,400 points per case farther than the margin from every outline; in three forms:
  coordinates as generated ("generic"), snapped to a grid of 0.002 or 0.004 radians (rings then share vertices
  and edges), and with `sliver_min_width = 10 h`; with a second crossing test by 16 points an edge as a
  reference;
- sixteen variants of `PolygonOverlay.cc` (fourteen mutations, the grouping switched off, and a candidate
  fix), each rebuilt and run against the committed
  `PolygonOverlay*:PolygonPartitioner*:PartitionFeatureUtils*` tests and against the probe;
- a cost probe (a dense exterior with many holes), on the committed code and with `do_rings_cross` switched off;
- direct calls of `do_rings_cross` on shapes the committed test does not have;
- five pyGPlates scripts importing `build-pygplates/bin` (built at the tip): the visual check's box and plate,
  a three-plate cascade of random crossing polygons, and one script for each of findings 1.1, 1.2 and 1.3.

Everything temporary was removed by `git checkout`, the three touched files compared byte for byte with copies
taken before the first change, both products rebuilt, and `git status` shows only this file. Probe sources,
logs and the mutation script are in the session's scratchpad under `review7/` (`probes.cc`,
`PolygonOverlayTest.cc.with-probes`, `mutate7.py`, `mut-a.log`, `mut-b.log`, `hunt-base/`, `swapped.log`,
`py_*.py`).

Baseline and final (*run*): `gplates-unit-test --gtest_filter=PolygonOverlay*:PolygonPartitioner*:PartitionFeatureUtils*`
101 tests, 99 passed, 2 skipped (`PolygonOverlayPrototypeTest.File`, `PolygonOverlayRealDataTest.Coastlines`, by
design), before the first change and after the last restore. After the restore, GPlates `ctest` 211 of 211 (the
same two skipped) and pyGPlates `ctest` 7 of 7, as claimed.

**In short.** The grouping does what it says for crossings in general position: the argument holds as coded,
and 60,000 random crossing-input cases and a 1,500-case cascade through pyGPlates read right everywhere, where
the same probe finds half of its cases wrong with the grouping switched off. Inputs whose rings do not cross are
unchanged. Two things are wrong at the edges of it, both for invalid inputs only: the crossing test misses
crossings of any size when the rings meet at vertices they share (1.1; the hunt's three remaining failures are
this, not thin overlaps, and one of them fails in the difference with the inputs swapped), and the sliver
filter still measures a ring that crosses *itself* by its signed area, so it can drop a whole piece (1.2,
older than this commit but one ring short of the exemption the commit adds). The rest is low: area, cost,
tests, documents.

## 1. Findings, most severe first

### 1.1 Medium: a crossing through shared vertices is missed, whatever its size; the pieces overlap (run)

`do_rings_cross`, `PolygonOverlay.cc:366-431`. The vertex test (381-418) looks only at vertices farther than the
margin from the other ring, and the edge test (`do_arcs_cross`, 196-238) wants all four ends farther than the
margin from the other edge's great circle. So two rings that cross only at points where one has a vertex on the
other's outline, and whose overlap has no vertex of either ring strictly inside it, pass both tests as "not
crossing", however large the overlap. The page and the header say what is missed is "an overlap thinner than
the margin" (`PolygonOverlay.h:204-205`, section 5.2); that is not the limit.

The smallest case (*run*, `py_chord.py`, through `PolygonOnSphere.partition` at the tip): a polygon with exterior
ring `(0,0) (1,0) (1,1) (0,1)` and interior ring `(1,0) (0,1) (0,0) (1,-1)` (degrees). The interior ring's first
edge is the square's diagonal and three of its vertices are the square's, so no vertex of either ring is inside
the other and no two edges cross away from a shared vertex. By parity the polygon is the square's upper right
half and the triangle below the square. Cut by the box `(-1,-0.5) .. (2,2)` (either as the plate or as the
feature), the inside comes back as two polygons, the whole square and the cut quadrilateral, which overlap in
the square's lower left half: half a square degree that `is_point_in_polygon` puts outside the polygon (2,952 of
97,370 sample points are in both inside pieces). With the crossing polygon as the plate that half is also in
the outside piece, so inside and outside overlap, which is the thing the changelog bullets say no longer
happens.

This is also what the hunt's three remaining failures are (*run*). `rays#498 seed 6`, reverse difference: output
ring 1 has one vertex on ring 0 and one 1.25 h inside it, the rest outside (the debug probe of the grouping fix,
re-run), so the edge between those two vertices is a chord through ring 0, and 3 of the oracle's 368 samples,
each farther than 10 h from both inputs' outlines, are in both rings. The overlap is not thin; only its witness
vertex is within the margin. **With the inputs swapped the same case fails in the difference** (2 of 366 samples
wrong as polygons, rings right): of the swapped hunt's 109,613 cases it is the only one whose intersection or
difference reads wrong as polygons, and no swapped case groups rings of inputs that do not cross. So the
commit message's "3 after (reverse and symmetric difference, crossings thinner than the margin)" and the page's
sections 6 and 7 ("two output rings cross by less than the margin ... their polygons overlap there") describe
the cause too narrowly and place it outside partitioning, where a reverse difference is a difference with the
polygons the other way round.

How often (*run*, the probe): never in 60,000 generic cases; with vertices snapped to a grid, in 76 of 5,985
cases at 0.002 radians (49 intersection or difference operations) and 241 of 5,832 at 0.004 (155), not counting
cases with coincident rings (1.6). In 135 of the 144 and 412 of the 469 wrong operations the 16-points-an-edge
reference finds a ring pair that crosses and `do_rings_cross` does not. The grid makes shared vertices far more
common than real data has them, so read the rates as "this is the remaining failure", not as a frequency. Real
data needs two rings of one polygon digitised through the same vertices, one cutting a corner of the other.

Confidence: high (three independent runs agree).

Fix (*run* as a variant, `fix-midpoints` in `mutate7.py`): classify each edge's midpoint as well as its start in
`has_vertices_on_both_sides`. A chord between two outline points has its midpoint well inside. With it: the
committed tests pass (99 of 99); the hunt goes from 51 differing cases to 44, the crossing-input cases wrong as
polygons from 3 to 0, with no new difference and no crossing group from inputs whose rings do not cross; the
grid probe's wrong operations with a missed pair go from 212 to 2 at 0.004 and to 0 at 0.002 (3,000 cases each); the
generic probe stays at 0. The four cases that also stop differing are the page's "4 at the symmetric
difference's crossing pairing" (`six#243 seed 10`, `six#219`, `#346`, `#571 seed 9`): their rings cross at a
vertex with no vertex beyond the margin (rings cross only at a margin of 0 in the debug probe), so they are the
same miss, and section 7's pairing item is smaller than it reads. The variant as run triples the vertex test's
cost (1.4), so test midpoints only for edges whose two ends are both undecided (within the margin), which is the
only place a chord can hide. Then correct the three statements above, and pin it (the square and the
quadrilateral, both ways round, and `rays#498 seed 6` swapped).

### 1.2 Medium: a ring that crosses itself is measured by its signed area, and dropped whole when its lobes cancel (run; older than the commit)

`Overlay::extract`, `PolygonOverlay.cc:4441-4446` and `4466-4492`; `get_mean_width`, `434-494`. The commit
exempts a group of two or more rings from the sliver filter because "its region is no one ring's" (4452-4453,
4473-4476). A single output ring that crosses itself is in the same position and is not exempt: its mean width is
`2 * area / perimeter` with the fan's signed area, which weights each lobe by its winding number, so two lobes
wound opposite ways subtract. `is_sliver` and the single-ring branches are unchanged by this commit (the diff
changes only their index), and the drop is the same with the grouping switched off (*run*, the mutation
`no-crossing-groups` on three of the cases below), so this is PR B's sliver filter, not this commit; it is here
because the commit decided which crossing rings the filter must leave alone and stopped one ring short.

*Run* through pyGPlates (`py_bowtie.py`): the bow-tie `(0,0) (2,2) (2,0) (0,2)` (a rectangle with two vertices
swapped, the classic invalid polygon), three quarters of it inside the plate `(-1,0.5) .. (3,3)`. The inside
piece is one ring with two equal lobes, signed area zero. `PolygonOnSphere.partition` at its default width (h)
returns `outside` with the whole bow-tie in the outside list; `PlatePartitioner.partition_features` leaves the
feature whole with plate ID 0. With `sliver_width_threshold_radians=0` the same call returns one inside piece
and two outside pieces, correctly. The whole-feature answer comes from `PolygonPartitioner.cc:150-154`: the
filter emptied the inside, so the subject is "whole, outside".

*Run*, the probe with `sliver_min_width = 10 h` on generic inputs: in 100 of 6,000 cases (55 intersection or
difference operations) the polygons lack a region the rings have, at points farther than 10 h from every
outline, with no point in two polygons. Case 210's reverse difference is one 13-point ring of signed area 0.21
square units against lobes of some hundreds (mean width 2.3 h): all of it is dropped and the operation returns
no polygon, 112 of 1,394 samples lost. Lobes need not be equal by design: any ring whose lobes cancel to within
`width * perimeter / 2` goes. (Case 210 is the one examined ring by ring. The other 99 are put down to the same
mechanism because no point of any is in two polygons, and nothing else in the filter drops or fills a region
that wide; they were not examined one by one.)

Confidence: high for the mechanism and both runs; the frequency in real data is low (a kink's small loop does
not cancel a large lobe; a symmetric bow-tie cut across its axis does).

Fix: leave a ring that crosses itself out of the filter, as a group is: the edge test of `do_rings_cross` run on
the ring against itself (skipping an edge's neighbours) finds it with the same margin. Or measure such a ring
by something that cannot cancel. Pin the bow-tie.

### 1.3 Low: a piece with crossing rings has no usable area, and "most overlapping" can now pick the other way (run)

`add_polygon`, `PolygonOverlay.cc:4494-4516`, puts a group's largest ring first and the rest as interior rings;
`PolygonOnSphere::get_area` is the exterior's area less the interiors' (`SphericalArea.cc:205-216`), which
section 3 of the page already says is not the parity area when rings cross. The consumers that add up pieces'
areas are `PartitionFeatureUtils.cc:123` (GPlates) and `PlatePartitioning.py:460` (pyGPlates), for the most
overlapping plate.

*Run* (`py_area.py`): a plate with exterior `(0,0) .. (10,10)` and an interior ring `(5,2) .. (15,8)` half outside
it. Its piece of a subject covering it is one polygon with one interior ring, `get_area` 0.0121 steradians where
sampling gives 0.0304. Against a second plate overlapping the subject by 0.0212, `most_overlapping_plate` assigns
the second plate; the crossing plate holds the larger part. Before this commit the two rings were two polygons
and summed to about 0.049 (too large by twice the overlap), so that feature went to the crossing plate. Neither
number is the area. For the common case (a hole poking a little out of its exterior) the new number is the
closer one, so this is not a regression on balance, but the direction of the error has changed and nothing says
so.

Confidence: high (run) for the numbers; the "before" figure is by reading. Fix: none cheap (the parity area
needs the rings intersected); say in the changelog bullet or the page's most-overlapping paragraph that a piece
whose rings cross is measured as its exterior less its interiors. An interior ring of such a piece can lie
partly or wholly outside its exterior ring (a group of three where the smallest does not touch the largest),
which the `partition` docstring (`PyGeometriesOnSphere.cc:4517-4518`, "with interior rings where the pieces need
them") does not prepare a caller for.

### 1.4 Low: the pairwise test costs every vertex of the larger ring for every ring inside its bounding circle (run)

`has_vertices_on_both_sides`, `PolygonOverlay.cc:381-413`, returns early only once it has both an inside and an
outside vertex. For a hole inside an exterior it never does: every hole vertex is tested against the exterior
(a distance query and a point-in-polygon each), and every exterior vertex against the hole's bounding circle.
With `m` rings inside one large ring of `N` vertices that is `m * N` circle distances, where the old grouping
did one point test a pair.

*Run* (`Review7Cost`: a wavy square exterior and a grid of 12-point holes, cut by a box): 20,000 exterior points
and 341 output rings, `extract(INTERSECTION)` 104 ms against 16.6 ms with `do_rings_cross` returning false at
once (classify is 5 ms); 100,000 points and 1,481 rings, 2,125 ms against 171 ms (classify 29 ms). The
difference's extract (fewer rings) goes from 5.2 to 14 ms and from 23 to 98 ms. So "partitioning the Muller 2022
coastlines takes the same time" (not re-run here) is true of polygons with few rings a piece, and the code
comment and page's "the number of rings is small: every pair is tested" is an assumption about the data, not a
bound. A cascade that leaves one hole a plate in a large remainder is where it would show.

Confidence: high for the measurements; low that any current data set reaches it. Fix if wanted: when every
vertex of the smaller ring is decided and on one side, skip the larger ring's vertex loop (a crossing then has
to pass between decided vertices, which the edge test finds); or take the larger ring's candidates from its
bounding tree against the smaller ring's bounding circle.

### 1.5 Low: the hand-built tests pin less of the commit than its message suggests (run)

Mutations (*run*, `mut-b.log`; "probe" is the temporary random probe, 3,000 generic cases):

| Mutation | Committed tests | Probe |
|---|---|---|
| vertex test removed | fail, but only `PolygonOverlayPrototypeTest.Bundle` and `.Hunt` | 1 case wrong |
| edge test removed | fail (`GroupRingsCross`, `GroupCrossingRings`, `.Hunt`) | 46 cases wrong |
| vertex test one way round only | **none fails** | 1 case wrong |
| a vertex outside the bounding circle not counted as outside | fail, only `.Bundle` and `.Hunt` | none |
| exterior not the largest ring | fails (`GroupCrossingRings`) | none (reads the same) |
| nesting check always true | **none fails** (as the plan says) | none; on the grid 118 cases wrong against 120 with the check |
| a hole group refused its hole parent's polygon | **none fails** | 4 cases wrong (the path was taken 13 times) |
| depth counted by groups, not rings | **none fails** | 53 cases wrong |
| the sliver filter applied to groups | **none fails** | not measured (the probe's filter run has no reference) |
| `do_arcs_cross` without its same-crossing test | **none fails** | none |
| holes appended in reverse order | **none fails** | none (reads the same) |
| the margin zero in the edge test, the vertex test, or `extract`'s call | fail (8 to 11 tests each) | none wrong: more is grouped, and it still reads right |

So the message's "mutations removing either test or the exterior choice fail committed tests" is true, but the
vertex test is held only by the committed prototype samples: `GroupRingsCross`
(`PolygonOverlayTest.cc:2002-2031`) has no pair that only the vertex test finds (its hole across the exterior
is also an edge crossing), and nothing calls the test with the arguments in one order only where it matters.
`GroupCrossingRings` (2041-2091) has two groups of two rings at depth 0. Nothing committed has a group inside
another group's ring, a ring in a stretch a group covers twice, a group of three, a hole group with a hole
parent, or a crossing group under the sliver filter. The probe shows those paths are both reached and needed
(rows 7 and 8), so they are right but unguarded. The partition layer, the feature level and pyGPlates have no
test with crossing rings at all, though both changelogs make a claim about them.

The prototype test's changes are sound: a crossing-input case wrong as polygons is reported and fails the case
(`PolygonOverlayPrototypeTest.cc:1169-1183`), so dropping the grouping-failure and coincident-ring counts for
those cases (1340-1345) hides nothing the oracle can see. Its limits: `rings_cross` (the oracle's test of the
inputs) has the same blind spot as 1.1 (segments meeting at a vertex are skipped), so the square and
quadrilateral would be checked as an input whose rings do not cross; an uncomputed crossing gets only the 300
global samples, not the 12 a vertex gets, so a small wrong region away from every overlay vertex can pass; and
the second cascade step still runs only for inputs whose rings do not cross (1204).

Suggested pins, in order of worth: 1.1's pair and the bow-tie; one overlay case with a group inside the
doubly covered stretch of another and a hole group inside that (three levels); a `PolygonPartitioner` or
pyGPlates test that a plate whose hole crosses its exterior cuts a box into pieces that sample right.

### 1.6 Low: two coincident rings are not grouped either (run; older than the commit)

`extract`, `PolygonOverlay.cc:4391-4396`. A polygon whose interior ring repeats its exterior ring is empty by
parity. Its two output rings do not cross, `is_ring_inside_ring` answers none (counted as `coincident_rings`) or,
when rounding leaves a vertex 1e-14 off, "inside" both ways, and they become two polygons covering the same
area. *Run*, grid probe case 2611 (0.002): `P` is a unit square twice, the difference is two identical polygons,
20 of 1,396 samples in both. The comment calls coincident rings "a degenerate output clean-up did not catch" and
the header "rings of no area"; these have area. Same cure as crossing rings (one polygon, which parity then
cancels), if it is wanted at all: a duplicated ring is rarer than a crossing one.

### 1.7 Low: under an exterior-ring-only coverage, a group's smaller rings lose their values (reading)

`PartitionFeatureGeometries.cc:681-705` gives a piece of an exterior-ring-only coverage (an older file's form)
values on its exterior ring alone. Before this commit every crossing ring was the exterior of its own polygon;
now only the largest ring of a group is, so a ring made from the subject's own exterior that is not the largest
of its group comes out as an interior ring without values. It needs the old coverage form, crossing rings, and
two crossing output rings that both run along the subject's exterior. Not run. Worth a sentence in section 5.4
rather than code.

### 1.8 Low: documents (reading, against the runs above)

- Section 5.2, `PolygonOverlay.h:204-205` and `989-990`, the comment at `PolygonOverlay.cc:4547-4548`, section
  6's last sentence, section 7's item and the commit message all describe the missed crossings as thinner than
  the margin or within the margin of an edge's end. See 1.1: the overlap can be any size.
- Both changelog bullets end "so the pieces ... don't overlap". True in general position, not for 1.1; and
  neither says that such a piece's area is not its area (1.3) or that the sliver filter leaves these pieces
  alone.
- `PolygonOverlay.cc:4318-4319` and section 5.2, "the number of rings is small: every pair is tested": see 1.4.
- Section 7 lists "4 at the symmetric difference's crossing pairing" as open for PR C; with the midpoint
  variant all four read right (1.1), so they are a grouping miss, and the commit's "its rings are grouped too"
  holds only for the catalogue's 13.
- The `partition` docstring and the `PlatePartitioner` text on slivers were not touched: a caller is not told
  that a piece may have an interior ring across or outside its exterior ring, nor that a piece whose rings cross
  is never dropped as a sliver.

Checked and right: the constants table's row; section 5.2's account of the two tests, the depth, the parent, the
exterior choice, the nesting formula and the fallback; section 5.5's sentence in 5.2 on groups; section 5.6;
section 6's description of what the prototype test now checks; `Diagnostics`' two comments; the header comments
of `is_ring_inside_ring` and `extract`; the regenerated matrix (one more include within `maths`, and
`pygplates-source-closure-test` passes).

## 2. Outside the commit, seen on the way

With vertices snapped to a grid the overlay itself answers wrongly without throwing in a few cases: the parity
over the output **rings** differs from `f(in P, in Q)` in 5 of 5,985 cases at 0.002 radians and 52 of 5,832 at
0.004, run as production runs (`throw_if_inconsistent`), in the intersection and difference among others. Case
1157 (0.002): `P` and `Q` each hold the same unit square as an interior ring, and each square crosses another
ring of its own polygon. These are inputs with whole rings in common and crossing rings at once, far from
anything real, and nothing in this commit touches classification; noted so the grid probe's other numbers are
read with it in mind (1.1's counts are of operations whose polygons differ from their own rings). The pyGPlates
cascade repeated on a 0.25-degree grid mixes the two: 12 of 1,500 cases have a sample in two pieces, in none or
in the wrong plate's, and I did not separate 1.1 from this in them.

## 3. Checked and found sound

- **The grouping argument, as coded** (*reading*, then *run*). Take a point `x` and the groups with a ring
  containing it. Groups are connected and do not cross, so those groups are totally ordered by nesting, a
  group's depth is the number of rings of the groups outside it that contain `x`, its parent is the next one
  out, and the nesting check holds. Cut the chain where the depth is even: each stretch is one polygon, the
  rings of a stretch that is not the last sum to an even number (the difference of two even depths), and the
  last stretch has the parity of the whole. So the union over polygons of each polygon's parity is the parity
  over all rings. A hole group in a hole parent (an odd depth plus an even number of the parent's rings) is one
  more link of the same stretch, and reads right; a group inside a ring that crosses itself is handled by the
  same count, since containment is asked by parity. Containment on the group's first ring only is enough for
  the same reason (the count is constant over a connected group). *Run*: 60,000 generic cases (seeds 1 to 3,
  20,000 each, the first 6,000 again as production runs), every operation, 0 cases where the polygons differ from the rings and
  0 where the rings differ from the truth; no grouping failure in any. With `do_rings_cross` switched off the
  same probe has 1,548 of 3,000 cases wrong, so it sees what the commit fixes.
- **Through the product** (*run*). A cascade of a random crossing polygon through three random plates by
  `PolygonOnSphere.partition` (1,500 cases, 865,373 samples away from the outlines): every sample in exactly
  the piece it should be in, so a piece with crossing rings (an interior ring across or outside its exterior)
  is a sound subject for the next plate. The visual check's box and plate 100 through `PlatePartitioner`: one
  plate-100 piece and one plate-5 piece, each with the plate's 4-point hole ring as its interior ring, 0 of
  199,464 samples wrong, as the plan says. (The other feature of `one_skipped.gpml` raises `PartitionError`, the
  known inconsistent cut.)
- **Inputs whose rings do not cross are unchanged** (*reading* and *run*). With every group a single ring the
  new code computes the same containers, parents, sliver decisions and polygon order as the old, and the new
  nesting check (`depth == parent's depth + 1`) cannot fail for a nesting. The hunt reports no crossing group
  from inputs whose rings do not cross in any operation but the symmetric difference, in either role (the
  39,322 polygon cases whose input rings do not cross, as exported and swapped).
- **The hunt's numbers** (*run*): 119,198 cases, 70,291 with crossing inputs, 3 of them wrong as polygons
  (`rays#498 seed 6`, `six#431 seed 2`, `six#502 seed 4`), 51 cases differing in all. With `do_rings_cross`
  switched off (the nesting check kept, so not quite the old code) 11,596 are wrong, against the 11,212
  claimed for the old grouping.
- **`do_rings_cross` on other shapes** (*run*): large rings (caps of 1.4 to 1.7 radians, one over a
  hemisphere, a small ring at its centre and at its antipode; three-point rings of 120-degree edges crossing
  like a star) answer as they should and the same in both argument orders; a hole's corner or edge beyond its
  exterior flips from "no" to "yes" between 9 h and 11 h; duplicate points and edges of 1e-13 do not disturb
  it. The bounding-circle shortcut is consistent with point-in-polygon (a point outside a ring's bounding
  circle is outside it by parity too). `do_arcs_cross`'s sign test is right for arcs under half a circle; the
  descent prunes only pairs of nodes whose circles are disjoint.
- **False positives are harmless to the reading** (*run*): with the margin zero, thousands of touching pairs
  are grouped and the generic probe still reads right everywhere. A wrongly grouped pair costs the polygon
  structure (and the committed tests), not the answer.
- **Consumers** (*reading*). A polygon's bounding circle and both bounding-circle builders take every ring, so
  the bounds shortcut in `overlay_polygon` is safe for a piece whose interior ring leaves its exterior.
  Point-in-polygon's outside point is the antipode of the exterior's centroid whenever every ring is in that
  hemisphere, which holds for any piece of ordinary size. Provenance follows each ring through `oriented` and
  `create_polygon` in the same order as the points. `GLReconstructedStaticPolygonMeshes` uses only the result.
- **The sliver filter with groups** (*reading*): a group is never dropped itself, is dropped with a dropped
  parent, and a single-ring hole in an even crossing parent is still filled; no path lets a kept group sit in a
  dropped polygon. No scenario found in which the exemption makes inside and outside overlap.
- **The suites** (*run*): counts in the header.

## 4. What remains unpinned

- Everything marked "none fails" in 1.5's table: the vertex test in one order, the nesting check (still, as
  the plan says), a hole group in a hole parent, depth by rings, the filter's exemption, the same-crossing sign
  test, hole order.
- Any crossing-ring input above the overlay: `PolygonPartitioner`, `GeometryCookieCutter`, Assign Plate IDs,
  `PlatePartitioner`, `PolygonOnSphere.partition`. The cascade and the visual-check case pass by this review's
  scripts only.
- The cost with many rings (1.4), and the partition timing claim, which was not re-run.
- The crossing test on rings with edges near half a circle (the sign test), which no committed input and no
  probe of mine reaches inside `extract`.
- 1.7, by reading only.
- Not looked at: a Linux build of the tip; Qt5 (nothing here is Qt-sensitive).
