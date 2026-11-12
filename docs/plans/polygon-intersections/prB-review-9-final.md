# PR B review 9: the final check of what the pull request says

Reviewed 2026-11-12 (Fable), adversarially, at the tip `41a2f139c` (`git status` clean before the first
change). This is not a re-review of the code. It checks that what PR B **says** is true of the code at the
tip: the foundation page's sections 5 to 7 (and what else changed on the page since the Phase 6 claim check,
`04e75e54a`), both changelogs, every docstring PR B added or changed, the two commits no review had read
(`b84512ef6`, `e7b66c3e8`), and `PLAN.md`'s lists of what is unpinned.

Read: `docs/design/architecture/foundation/spherical-geometry.md` lines 1025 to 2443 and its diff since
`04e75e54a`; `src/maths/PolygonOverlay.{h,cc}`, `PolygonPartitioner.{h,cc}`, `src/app-logic/GeometryCookieCutter.{h,cc}`,
`PartitionFeatureGeometries.{h,cc}`, `PartitionFeatureUtils.{h,cc}`, `GenericPartitionFeatureTask.cc`,
`AssignPlateIds.{h,cc}`, `src/opengl/GLReconstructedStaticPolygonMeshes.{h,cc}`, `GLCubeSubdivision.{h,cc}`,
`src/api/PyGeometriesOnSphere.cc`, `PyPlatePartitioner.cc`, `PyExceptions.cc`,
`src/qt-resources/python/api/PlatePartitioning.py`, the dialog and the command, the tests the page cites,
`src/unit-test/data/README.md`, both changelogs, `docs/pygplates/README.md`, and `PLAN.md`'s Status with
reviews 7 and 8.

The page's claim check was split into five parts by line range (1025-1337, 1338-1438 with 5.3, 1613-1902,
5.8, and sections 6 and 7), each given to a fresh Fable subagent with the code; lines 1439-1573 (the
grouping of crossing rings, the checks, the sliver filter's exemptions, polylines) I checked myself. I
verified every medium finding of theirs against the code or by running it, and a sample of the low ones;
where a finding below rests on a subagent's reading alone it says so.

Evidence is marked *run* or *reading*. *Run*, all on this machine (Windows, the `gplates` conda environment),
with both trees built at the tip first (a no-op):

- pyGPlates scripts importing `build-pygplates/bin`: the docstrings' and changelogs' claims (`py_claims.py`),
  the committed inconsistent case with each hole removed, the feature-level cases (`part3/features.py`,
  `features2.py`), the piece counts and timings on the Müller et al. 2022 coastlines (`part5/`);
- a replay of the raster's cube quad-tree traversal over the 2,392 Merdith et al. static polygons through
  pyGPlates, down to depth 6 (`quadtree_check.py`, 197,984 partitions);
- fifteen mutations of `PolygonOverlay.cc`, each rebuilt and run against the committed
  `PolygonOverlay*:PolygonPartitioner*:PartitionFeatureUtils*` tests (`mutate9.py`, `mutations.log`), and
  one of them again for its failures' text (`m16_detail.py`);
- review 7's and review 8's cost probes appended to `PolygonOverlayTest.cc`, with the committed crossing
  test, with it answering false at once, and without its outline-distance shortcut (`cost9.py`,
  `cost9.log`);
- the plan's timing harness (`time_partition.py`) on the tip and on `main` without PR B
  (`C:/gplates/src/worktree-1/build-pygplates`, `cc1c10ea1`), alternated, and on the tip with the whole
  answer's check switched off (`timing_variant.py`, `timing9.log`).

The mutation and cost builds were started only after the five subagents had finished, since they run the
same test binary. Every temporary change was to `PolygonOverlay.cc` or `PolygonOverlayTest.cc`. The first
was restored each time from a copy taken before the first change (its working copy has LF line endings and
a checkout here writes CRLF), written back by the scripts, so with a fresh timestamp; the second by `git
checkout`, and its timestamp updated. `PolygonOverlay.cc` has the SHA-1 it had before the first change
(`b25a2b8d`), `git diff` is empty, both products were rebuilt from the restored sources, and `git status`
shows only this file. Nothing was committed, and the stub, the dependency matrix and the planning
repository were not touched. Scripts and logs are in the session's scratchpad under `review9/` (the
subagents' in `part1/`, `part3/` and `part5/`).

Baseline (*run*, before the first change) and final (after the last restore and rebuild): GPlates `ctest`
214 of 214 (`PolygonOverlayPrototypeTest.File` and `PolygonOverlayRealDataTest.Coastlines` skipped, by
design), pyGPlates 7 of 7.

**In short.** Nothing found changes what the code does for a user, and no changelog bullet is false. The
documentation commit `e7b66c3e8` is right: the one inconsistent input needs a crossing hole in both
polygons, and the cut succeeds with either removed. The other unreviewed commit, `b84512ef6`, is harmless
for a reason nobody recorded: the function it changes is **not reachable** (it serves static polygons drawn
as fans, which have been compiled out since 2012), so the raster the developer looked at could not show it
either way (1.1). The page has two false statements about the two products agreeing (1.2, 1.3), two stale
measurements (1.4), a tolerance whose stated value is not the one that runs (1.5), a section that says the
overlay adds no distance threshold after three commits added several (1.6), and a list of unpinned fixes
that a later commit shortened (1.7). The rest is low: sentences left stale by a later edit, cross-references
that land nowhere, and about thirty citations of reviews, runs and pull requests that will mean nothing once
`docs/plans/` is deleted, in the page and in code comments alike (1.11).

## 1. Findings, most severe first

### 1.1 Medium: `b84512ef6` changes a function nothing calls, so no raster shows what it answers (reading and run)

`GLReconstructedStaticPolygonMeshes.cc:969-987`. The commit removes the test that, after the partitioner
answered *outside* for a quad-tree node, still recorded the node if it held the static polygon's first
vertex. Its message, the comment it leaves (`:978-983`) and `PLAN.md:942-947` give two reasons: the
developer's raster looked right, and over 55,000 boxes the overlay never answered outside where the test
would have overridden it.

**The code is unreachable.** This overload is called only from `:698`, for a drawable that holds a
`PolygonFan` and not a `PolygonMesh` (`:685-702`). The only place a fan drawable is made is `:543-611`, which
is under `#if 0 // Temporarily comment polygon fan's until we getting it all working...`, and has been since
`11af71161` (2012). A geometry that `PolygonMesh::create` cannot mesh gets no drawable and is skipped
(`:670-674`); a meshed one finds its nodes from its triangles against each node's frustum (`:708-899`). So
`polygon_partitioner.partition_polygon` at `:973` never runs in GPlates, with or without the removed test.
The page already says so in section 3.3's table (`spherical-geometry.md:345`: `PolygonFan::create`'s "only
caller ... is under `#if 0`"), but section 5.1 lists the raster meshes as one of the partitioner's two other
callers (`:1060-1063`), 5.1's result contract cites how "the raster meshes call it" (`:1087`), 5.5 says "so
the raster meshes stay exact" (`:1769`) and 5.6 says what they do with an inconsistent cut (`:1814-1815`).

So the change is safe, but the evidence for it is not evidence: the visual check could not have seen this
code, and decision 4 of 2026-11-04 ("the Phase 6 visual check decides") had no way to decide. **There is
nothing for the developer to check visually for this commit.** A raster's static polygons are unaffected by
anything PR B does to the partitioner.

**Were the fan path enabled, is the change right?** By reading, yes. The node's polygon is the subject and
the static polygon the clip. A node surrounding the polygon has a clip ring inside the subject, classified
inside by its own vertex, so both extracts keep a ring and the answer is intersecting
(`PolygonOverlay.cc:3224-3227`, `PolygonPartitioner.cc:144-159`); the bounding circles then overlap, so the
shortcut does not answer first. A node inside a hole of the polygon is outside, rightly, and the removed
test agreed (the first exterior vertex is not in such a node). A node wholly inside the polygon is inside.
An exception is taken as intersecting (`:971-977`). An exact touch is outside, by the result contract.

*Run*, on what the quad tree really asks (`quadtree_check.py`: the six cube faces' four-arc node polygons
of `GLCubeSubdivision::create_bounding_polygon`, descended to depth 6 as the C++ does, stopping at an
outside node, for all 2,392 polygons of the Merdith et al. static polygons through
`PolygonOnSphere.partition` with the filter off): 197,984 partitions, 100,022 outside, 88,379 intersecting,
9,583 inside, no exception. Every polygon is recorded in some node, and 6,693 recorded nodes hold their
whole polygon. In no outside answer is a vertex of the polygon or of the node more than `1.5e-6` radians
inside the other. But the commit's "could never fire" is not what this shows: in **147** outside answers the
node holds the polygon's first exterior vertex by pyGPlates' point-in-polygon (which counts a point on the
outline as inside; the removed test had the threshold off, so there it would have gone either way by
rounding). All 147 are at depth 1, where the node edges are the planes `x = 0`, `y = 0` and `z = 0`: 148 of
the polygons start at a vertex on longitude 180 (`first_vertex.py`), where the file's polygons are cut. The
earlier experiment's random boxes (`redundant_test_check.py`) had no such edge, which is why it saw three
touches and the quad tree has 165. Each is a touch, where the polygon has no area in the node, so dropping
the node is right; the test would only have added rendering work.

The earlier experiment also used only polygons valid at 0 Ma (2,389), boxes of 0.3 to 40 degrees along
parallels and meridians (a root node is a cube face 90 degrees across), and surrounding boxes only for
polygons under 170 degrees of longitude and within 85 degrees of latitude, so not for a polygon across the
dateline or a pole. None of that changes the answer; the replay covers them.

Confidence: high (the preprocessor block, the call graph, and the page's own table).

**Recommended fix** (documents only; the code can stay):

- `GLReconstructedStaticPolygonMeshes.cc:978-983`, replace the comment with:
  ```
  // A node surrounding the test polygon is intersecting, not outside: the test polygon is then a
  // clip ring inside the subject, which the overlay reports as intersecting. So no test of the
  // polygon's vertices against the node is needed. (This overload serves a polygon drawn as a fan,
  // which create_polygon_mesh_drawables compiles out: nothing calls it today, and no rendering
  // shows what it answers.)
  ```
- `spherical-geometry.md:1060-1063`, after "for a quad-tree node's bounding polygon." add: "That overload
  serves a static polygon drawn as a fan, and the fan fallback is under `#if 0` (section 3.3), so nothing
  reaches it today: a meshed polygon's nodes are found from its triangles."
- `:1769`, replace "so the raster meshes stay exact" with "so a caller that asks for no filter (the raster
  meshes' unreached path among them) gets an exact cut".
- `PLAN.md:942-947`: add that the function is unreachable (the fan fallback is under `#if 0`), so the raster
  could not show it, and that a replay of the real quad tree found 147 touches at a polygon's first vertex
  (on longitude 180) and no overlap missed.

### 1.2 Medium: "the two products cannot disagree" is false under the most overlapping plate (run and reading)

`spherical-geometry.md:1720-1721`: "Which properties are partitioned is decided once, in that visitor:
pyGPlates' `partition_features` removes exactly the properties it reports as partitioned, so the two
products cannot disagree."

That holds when splitting. Under the most overlapping plate, Assign Plate IDs removes every reported
property before it looks at the method (`GenericPartitionFeatureTask.cc:90-93`, ahead of the switch at
`:139`) and then puts each geometry back whole (`:195`, `add_unpartitioned_geometry_to_feature`), while
pyGPlates returns `feature.clone()` with nothing removed (`PlatePartitioning.py:428-489`). Two differences
follow (*run* for pyGPlates, `part3/features.py` cases 1, 2 and 4; *reading* and
`PartitionFeatureUtilsTest.cc:1163-1191` for GPlates):

- a constant range that matches no geometry is removed by GPlates and kept by pyGPlates
  (`unclassifiedGeometryCoverage ... scalars=[1.0, 2.0, 3.0]` is still on the feature);
- a single-window geometry and its single-window range come back constant from GPlates and stay
  `GpmlPiecewiseAggregation` in pyGPlates.

Neither changelog claims otherwise (the pyGPlates bullet on time-dependent geometry says "with
`PartitionMethod.split_into_plates`"). Confidence: high.

**Recommended fix**, replacing the sentence:

> Which properties are partitioned is decided once, in that visitor: when splitting, pyGPlates'
> `partition_features` removes exactly the properties it reports as partitioned, so the two products split
> alike. With the most overlapping plate they differ. Assign Plate IDs removes the same properties and puts
> each geometry back whole: a single-window geometry and its range come back constant, and an unmatched
> constant range (below) does not come back. pyGPlates returns the feature unchanged but for the copied
> properties.

Or make pyGPlates do the same, in the `partition_features` rework already decided (`PLAN.md:1031`).

### 1.3 Medium: a time-dependent range that matches no geometry is kept, and a piece can read it back (run)

`spherical-geometry.md:1725-1727`: "A time-dependent one is kept: it belongs with a time-dependent geometry,
which is kept (it is unmatched only for having no value at the reconstruction time)."

The code keeps any time-dependent range under the name, whatever the reason it is unmatched
(`PartitionFeatureGeometries.cc:939-950`; its comment makes the same claim). So the hazard the sentence
before it says is closed ("it could be read back as the coverage of a piece that happened to have its number
of points") is open for this case. *Run* (`part3/features2.py` case A): a constant triangle with a
single-window range of 4 values, its window containing the reconstruction time, has no coverage before the
cut (`Feature.get_geometry(..., CoverageReturn.geometry_and_scalars)` gives `None`). After
`partition_features` both pieces' features carry the range, and the 4-point piece reads it back as its
coverage, `[7.0, 8.0, 9.0, 10.0]`. GPlates keeps the property the same way (*reading*). The input is rare
(a range in a time window beside a constant geometry, with the wrong number of values). Confidence: high.

**Recommended fix**: either remove such a range too (in `partition_feature`, when the range has a value at
the reconstruction time), or say it, replacing the sentence:

> A time-dependent one is kept: it normally belongs with a time-dependent geometry, which is kept, and is
> unmatched for having no value at the reconstruction time. One that has a value then, but a number of
> values that is no geometry's, is kept too, and a piece with that number of points reads it back as its
> coverage.

and the comment at `PartitionFeatureGeometries.cc:939-941` likewise.

### 1.4 Medium: section 6's two real-data comparisons with pyGPlates are stale (run)

`spherical-geometry.md:2364-2367`: "the piece counts equal pyGPlates' at each time (431 polygons at 0 Ma
against 703 polylines before), and pyGPlates' `partition_features` over the 2,136 features takes about 30%
longer than before (0.28 s against 0.20 s at 0 Ma, resolving the topologies about 0.06 s of either)."

**The counts.** The harness cuts with the filter off; pyGPlates' `PlatePartitioner` has since been given the
default width of `10 h`. *Run* (`part5/count_pieces.py`): with `sliver_width_threshold_radians=0` pyGPlates
gives the harness's 149 polygons cut into 431 at 0 Ma, 83 into 212 at 100 Ma and 50 into 133 at 200 Ma; at
its default, 146 into 415, 82 into 210 and 50 into 131. The plan's own later timing logs print "146 polygons
cut into 415 pieces". The 16 fewer are section 5.5's 16 rings under `10 h`.

**The timing.** *Run*, the plan's own harness (`time_partition.py`, best of 5, the two builds alternated
twice; `partition_features` alone, resolving the topologies timed apart at 0.05 to 0.07 s for either):

| Time | Tip | `main` without PR B | |
| --- | --- | --- | --- |
| 0 Ma | 0.331 to 0.348 s | 0.196 to 0.197 s | about 70% longer |
| 100 Ma | 0.234 to 0.238 s | 0.143 to 0.145 s | about 64% |
| 200 Ma | 0.199 to 0.201 s | 0.119 to 0.120 s | about 67% |

So the page's "before" figure reproduces to the digit, which rules out the machine, and its "after" does
not: 0.28 s was measured on 2026-11-07 (`PLAN.md:730-737`), and the plan's later records of the same call
are 0.280 s (`:918`), then 0.31 s in `56952f9b5`'s message (which put the rise down to the machine), then
0.33 to 0.34 s (`:972`, `:996`, `:1056`), each compared only with the one before. A subagent's plain script
over the same features (`part5/time_partition3.py`) gives 0.47 to 0.49 s against 0.31 to
0.33 s, about half as long again, and `PLAN.md:1027` has 0.49 s for that form. Either way "about 30%" is
too low, and it was 40% even for the pair quoted. Where the 0.05 s since went: not the whole answer's check
(*run*, with `check_whole_result` returning at once the tip takes 0.324 to 0.325 s at 0 Ma, so the check
costs about 0.01 to 0.02 s); by reading, most likely the Python that puts kept properties back on the plain
geometry path, which `56952f9b5` added for every feature. `PLAN.md:1027-1032` already measures the Python
assembly at about 0.31 of 0.49 s and decides its rework.

Also, "resolving the topologies about 0.06 s of either" reads as if the 0.06 s were inside the figures; the
harness times it apart.

Confidence: high (two harnesses, two builds alternated).

**Recommended fix**, replacing from "the piece counts" to the end of the paragraph:

> the piece counts equal pyGPlates' at each time with its sliver filter off (431 polygons from the 149 cut
> at 0 Ma, against 703 polylines before; at pyGPlates' default width the 16 rings under `10 h` of section
> 5.5 are dropped, 415 at 0 Ma), and pyGPlates' `partition_features` over the 2,136 features takes about
> 70% longer than before (0.34 s against 0.20 s at 0 Ma, after about 0.06 s resolving the topologies for
> either), most of it in the Python that assembles the features.

The developer may want the 70% known before the pull request is opened, since the plan and the page both
say 30%.

### 1.5 Medium: `get_mean_width`'s apex threshold is `1e-12`, not `1e-18`, and the code depends on that (reading and run)

`PolygonOverlay.cc:751-755`; the page at `spherical-geometry.md:79` (section 1's table) and `:1755-1756`
("where that is rounding (`magSqrd()` under `1e-18`, a ring of no area whichever apex is used)").

`vector_area.magSqrd() > 1e-18` compares a `real_t`, whose `>` is `r1 - r2 > EPSILON` with `EPSILON = 1e-12`
(`Real.h:274-280`; section 1's own first rows say so). So the vector area is the apex only when its
`magSqrd()` exceeds about `1e-12`: a ring of more than about `5e-7` steradians, some 20 square kilometres.
Every smaller ring, which is every sliver the filter exists for, takes the vertices' mean direction, and the
second test is in effect `> 2e-12`. That is harmless: the mean direction of a small ring is at its centre,
and a ring on a whole great circle has a vector area of about 6.

It is also **load-bearing**. *Run* (`m16_detail.py`): with the test written as the comment means it,
`vector_area.magSqrd().dval() > 1e-18`, seven committed tests fail (`PolygonOverlayPrototypeTest.Catalogue`,
`.Hunt`, `.Anchorless`, `.Bundle`, `PolygonOverlayTest.LinkCatalogueFamilies`,
`PolygonPartitionerTest.SliverFilterEmptiesBoth`, `.SliverFilterMeanWidth`), each with
`UnableToNormaliseZeroVectorException`: `Vector3D::get_normalisation` takes a vector with `magSqrd()` under
the same `real_t` tolerance for zero and throws. The comparison works because it is the same test
`get_normalisation` makes. So do not "correct" the code with `.dval()`.

Confidence: high.

**Recommended fix.** In the code, say what runs (a comment, and a literal that does not mislead):

```
	// The same test get_normalisation makes (real_t's: magSqrd() over about 1e-12, a vector area longer
	// than about 1e-6, so a ring of more than about 5e-7 steradians). A smaller ring is far from a
	// great circle, and its vertices' mean direction is near its centre.
	const Vector3D apex =
			(vector_area.magSqrd() > 0) ? Vector3D(vector_area.get_normalisation())
			: (sum.magSqrd() > 0) ? Vector3D(sum.get_normalisation())
			: Vector3D(ring.front().position_vector());
```

(`> 0` as a `real_t` is `> 1e-12`; the second test is then `1e-12` where it is `2e-12` today, which is
still what `get_normalisation` needs. If the literals are kept, change only the comment.) In the page:

- `:79`, first two cells: "`get_mean_width` apex, `magSqrd()` within `real_t`'s tolerance of zero (about
  `1e-12`) | `PolygonOverlay::get_mean_width`: the fan's apex is the ring's vector area, normalised, unless
  its `magSqrd()` is within `real_t`'s tolerance of zero (a vector area shorter than about `1e-6`, a ring
  under about `5e-7` steradians, which is also where `get_normalisation` refuses it); then the vertices'
  mean direction, and if that cancels too the first vertex".
- `:1755-1756`, replace "(`magSqrd()` under `1e-18`, a ring of no area whichever apex is used)" with
  "(`magSqrd()` within `real_t`'s tolerance of zero, about `1e-12`: a ring under about `5e-7` steradians,
  whose mean direction is near its centre)".

### 1.6 Medium: section 5.3 says the overlay adds no distance threshold; section 1's table omits the newest (reading)

`spherical-geometry.md:1576-1577`: "The overlay adds no distance threshold of its own to section 1's
system." And `:1587-1588`, the end of the *Exact* list: "the containment tests of the grouping (with the
margin above) and the hemisphere check's point tests (with `10 h`)."

Section 5.3 is unchanged since `04e75e54a`; the code beside it is not. At the tip the overlay has, of its
own: `HEMISPHERE_CHECK_MARGIN = 10` thicknesses (`PolygonOverlay.cc:188`, applied in `find_reference_point`,
`:2591`, for `extract` and for `check_whole_result`, whose candidates are `20 h` off the subject's edges,
`:2631`); the grouping margin `max(10 h, 2 x the widest record cluster)` (`:4624-4625`), used by
`is_ring_inside_ring`, three ways by `do_rings_cross` (`:420`, `:530`, `:552`) and by
`does_ring_cross_itself` (`:4738`); `PROBE_DISTANCE_IN_MARGINS = 3` (`:198`); and the two-margin arms of the
self-crossing test (`:699-702`). Section 5.2 itself says the margin decides what the crossing tests see
(`:1472`). None of 5.3's three lists places the crossing tests, the whole answer's check or the self-touch
cuts.

Section 1's row for these (`:77`, edited in `1dc10e9da`) names `Overlay::extract` for the `10 h` (it is
`find_reference_point`) and omits the probe distance, the two-margin arms, the whole answer's check and
`does_ring_cross_itself`; that table is introduced as "the one table of every tolerance in this code". The
table's other overlay rows hold (a subagent's reading, which I sampled), but row `:76` still describes the
anchorless test before `3180ea66e`: each input's parity is now counted on its own segment's great circle, to
its own test point (`PolygonOverlay.cc:3707-3727`).

Confidence: high.

**Recommended fix.** `:1576-1577`:

> The overlay builds and classifies with no distance threshold of its own: vertices are grouped by
> identity, and the one distance is `GeometryIntersect`'s thickness `h` (in the records, and again in the
> self-touch cuts). What it adds are margins for its checks and for reading its output, all in section 1's
> table: `10 h` for the hemisphere check and a whole answer's check, and the grouping margin, `max(10 h,
> twice the widest record cluster)`, for containment and for the crossing tests, which look three margins
> along an edge. A margin keeps an exact test clear of the band. It also bounds what the crossing tests
> see: an overlap or a self-crossing reached only within a few margins is missed (section 5.2). Its
> decisions are of section 5.2's three kinds:

`:1587-1588`, the end of the *Exact* list:

> the containment tests of the grouping, and the point and edge tests of its crossing tests and of the
> sliver filter's self-crossing test (point-in-polygon with the threshold off and sign tests, at points
> farther than the margin above from what they are tested against); and the point tests of the hemisphere
> check and of a whole answer's check (with `10 h` from the inputs' outlines).

Add to the *Rounded* list, before "the orientation of an output ring":

> the self-touch cuts (a vertex within `h` of a piece's own plane and further along it by more than `1e-12`
> radians, section 5.2); which points the crossing tests take as near (a distance compared with the margin,
> where a flip changes only whether a crossing within a few margins is seen);

`:77`, second and third cells:

> `HEMISPHERE_CHECK_MARGIN` in `PolygonOverlay.cc`. `Overlay::find_reference_point`: the reference point of
> the hemisphere check (`extract`) and of a whole answer's check (`check_whole_result`) must be farther than
> `10 h` from both inputs' outlines; the whole answer's candidates beside the subject's edges are `20 h` off
> them. `Overlay::extract`: the grouping margin, for `is_ring_inside_ring`'s point that decides containment,
> for `do_rings_cross` (outlines farther apart than it do not cross; a point within it decides nothing) and
> for `does_ring_cross_itself`. `PROBE_DISTANCE_IN_MARGINS = 3`: both crossing tests look three margins
> along the edges of a vertex within the margin, and `does_ring_cross_itself` asks for arms two margins long
> | Whether an output's rings, or a whole answer, read as the output or its complement; which ring contains
> which, which rings cross, and which ring crosses itself (section 5.2)

`:76`: "...when within that of the great circle of the input's own segment along the chain (as a sine), or
of the input's test point on it or its antipode"; and "The crossing arc must meet that great circle only at
the test point, ...".

### 1.7 Medium: section 6's "all but two" is out of date, and `PLAN.md`'s lists need three corrections (run)

`spherical-geometry.md:2351-2354`: "The reviews' fixes are pinned by tests, all but two (the self-touch
cuts' key, which no constructed input reaches, and the whole answer's check being called from `classify()`)
checked by reverting the fix."

`b14bf9bb8` pinned the second (`PolygonOverlayTest.cc:1438`, `whole_checks == 1`), and the page's own test
list at `:2269` describes that test. What is unpinned at the tip, by mutation (*run*, `mutations.log`; each
against the committed `PolygonOverlay*:PolygonPartitioner*:PartitionFeatureUtils*` tests, 102 passing and 2
skipped unmutated):

| Mutation | Committed tests that fail |
|---|---|
| the point test one way round only (`do_rings_cross`) | **none fails** (as the plan says) |
| the nesting check always true (no grouping failure) | **none fails** (as the plan says) |
| the sliver filter applied to groups | **none fails** (as the plan says) |
| `do_arcs_cross` without its same-crossing test | **none fails** (as the plan says) |
| hole groups appended in reverse order | **none fails** (as the plan says) |
| a far node not counted as outside | **none fails** (the plan's "not pinnable") |
| self-touch cuts keyed by vertex, not by stop | **none fails** (as the plan says) |
| the outline-distance shortcut removed | none fails, rightly: speed only |
| a hole of a hole group filtered too | **none fails** (in no list) |
| the self-crossing test's arms of any length (the two-margin rule off) | **none fails** (in no list) |
| the filter does not ask the self-crossing test of a hole | `SliverFilterSelfCrossingRing` (none in review 8) |
| `whole_checks` counted for a skipped check too | `CheckWholeResult` (none in review 8) |
| `PROBE_DISTANCE_IN_MARGINS` 1 in place of 3 | `GroupRingsCross`, `GroupRingsCrossAgainstLoops`, `SliverFilterSelfCrossingRing` |
| the largest ring not chosen as a group's exterior | `GroupCrossingRings` |
| the apex thresholds compared as doubles | seven tests (1.5) |

So the plan's lists are right for what they name, `401859aa9`'s claims hold (the filter's hole path and a
skipped whole check are now pinned), and two things are in no list: the filter's condition that a hole is
filled only when its parent is an exterior (`PolygonOverlay.cc:4779`), and the self-crossing test's
two-margin arms (`:696-705`; the plan's "undecided rule" mutation was the other one, a probe within the
margin of the path). `GroupRingsCrossAgainstLoops` compares `do_rings_cross` with a reference that is itself
both ways round, and no generated pair is seen one way only, which is why the first row survives.

Confidence: high.

**Recommended fix**, replacing the sentence at `:2351-2354`:

> The fixes made after review are pinned by tests, each checked by reverting the fix, all but one (the
> self-touch cuts' key, which no constructed input reaches). In the grouping, no test holds the point test
> in one argument order (every crossing built is seen both ways), the nesting check's fallback, the
> exemption of a group from the sliver filter, the same-crossing test of two arcs, the order of holes, or
> the count of a node far from the other ring as outside (a ring with a point inside the other has a point
> outside it in a near node too).

Section 3 has the list for `PLAN.md`.

### 1.8 Low: statements in sections 5.1 and 5.2 made stale by a later edit, or that two places now disagree on (reading; the cost by run)

- **`:1495`**, "(rings whose overlap the crossing test misses, reached only within the margin)". `05620ce97`
  changed the limit to "within a few margins" at `:1472`, in `PolygonOverlay.h:1040-1041` and at
  `PolygonOverlay.cc:4840-4841`, and missed this one. Fix: "reached only within a few margins of the
  outlines". The test comment at `PolygonOverlayPrototypeTest.cc:1339-1340` ("a grouping failure is a
  crossing thinner than the margin") has the same stale limit.
- **`:1497-1498`**, "For inputs whose rings do not cross, every group is one ring and this is plain
  containment with parity depth." Not for the symmetric difference, whose rings can cross at a vertex
  (`:1444-1446`), are grouped, and are excluded from the tests' no-crossing-group check for that reason
  (`PolygonOverlayPrototypeTest.cc:1159-1169`). Fix: "For inputs whose rings do not cross, every group is
  one ring (but in the symmetric difference, above) and this is plain containment with parity depth."
- **`:1426-1427`**, "The cut is always the other *non-crossing* pairing of the four ends at `v`: of the
  three pairings, the original crosses nothing ...". The symmetric difference's pairing can cross (the same
  lines `:1444-1446`, and section 7 at `:2386`). A subagent's reading; I did not reproduce a crosswise
  pairing. Fix: "Where the pairing at `v` crosses nothing, the cut is the other *non-crossing* pairing of
  the four ends at `v`: of the three pairings, exactly one of the other two crosses;" and after the
  sentence: "Every pairing rule above crosses nothing, but for the symmetric difference's at a vertex whose
  four kept ends pair crosswise (below), where the rings cross and are grouped."
- **`:1344-1345`**, "At a vertex whose rays all start at its output position every cone is `0`, every ray
  is anchored and nothing changes." Stale since the cone gained the offsets off the segment
  (`PolygonOverlay.cc:1763-1778`): a ray from a touch record inside a segment, or to one, has a `delta` of
  up to `h` though it starts at the output position, as `:1204-1215` and section 1's row `:75` say. Fix:
  "At a vertex whose rays all start at its output position, with no touch inside a segment and no
  self-touch cut at either end of their first arcs (so no offset off the segment, above), every cone is
  `0`, every ray with a direction is anchored and nothing changes."
- **`:1466-1470`, the cost**: "about 1.1 times for an exterior with 1,481 holes and 100,000 points, 1.2
  times for an annulus of two 100,000-point rings, 1.4 times for ... 50 long pieces side by side, and 2.7
  times for 100 of them ... (without it, 25 times ...)". *Run* (`cost9.py`: `extract` with the committed
  test against `do_rings_cross` answering false at once, each four to six times): 1,481 rings, 201 to 206 ms
  against 175 to 188 ms, 1.1; the annulus of two 100,000-point rings, 80 to 92 ms against 72 to 85 ms, 1.1
  (of 20,000 points, 17 to 20 ms against 15 to 18 ms, 1.1); 50 pieces, 26 to 29 ms against 19 to 22 ms,
  1.3; 100 pieces, 103 to 108 ms against 40 to 46 ms, 2.3 to 2.7; and without the outline-distance
  shortcut, 100 pieces take 1,107 to 1,111 ms, 24 to 28 times. So every figure holds but the annulus's,
  which is 1.1 here, as `401859aa9`'s message and `PLAN.md:1045` have it. Fix: "1.1 times for an annulus".
- **`:1317-1319`**, "the clip's parity is counted to the subject's projection, as before." At `04e75e54a`
  both parities were counted to the midpoint itself, unprojected, so "as before" names nothing a reader can
  see and is not what the old code did. Fix: "...is counted to the subject's projection instead."
- **`:1309-1310`**, "(`PointInPolygon::get_crossing_parity_excluding_segment`, the new entry, since the
  crossing counter was file-local)". The counter is still file-local. Fix: "an entry of its own, since the
  crossing counter is file-local".
- **`:1152-1153`**, "the count of ends at a vertex stays even for each input". Not for a polyline subject,
  one of whose ends can be at a record (`PolygonOverlay.cc:1276-1296`). Fix: "for each polygon input".
- **`:1186-1187`**, "the exact tangent of the piece's first arc of non-zero length". The code skips an arc
  whose *segment* is zero length (`:1737`), as `:1189` says. Fix: "of the piece's first arc that lies on a
  segment of non-zero length". `PolygonOverlay.h:401-402` has the same wording.
- **`:1189-1191`, `:1199-1200` and `:1338-1339`**, the bundle rule's condition. The code builds a bundle at
  six or more ends unless one input passes exactly once (`:1960`), so "where both inputs pass more than
  once" omits a vertex where only one input has ends left, and at a one-passage vertex a ray with no
  direction does not float: it is placed by its class. Fix at `:1338-1339`: "where neither input passes
  exactly once (both pass more than once, or only one has ends left)"; at `:1190-1191`: "at six or more it
  floats under the bundle rule below, or, where one input passes once, is placed by its class (the
  one-passage rule)".
- **`:1349-1350`**, "The measured order is always admissible and is placed first." The code adds it if the
  enumeration did not produce it (`:3547-3555`); the admissibility test uses `atan2` angles where the order
  uses sign tests. Fix: "The measured order is always among them, and first: it is admissible in principle,
  and is added if rounding keeps it out of the enumeration."
- **`:1409-1410`**, "pairs with the end of its ring's next non-internal piece at the same stop". The code
  asks for the same vertex (`:4178-4180`); with an internal piece between, it is another stop of it. Fix:
  "pairs with the end, at this vertex, of its ring's next non-internal piece (a neighbour that is elsewhere
  or not kept is a pairing failure)".
- **`:1420-1422`**, "so the output's positions there are the subject's (the clip's can differ by up to
  `h`)". A common pair has no interior stops (`:1522-1527`), so the walk emits only the two vertices' output
  positions, the same whichever piece is followed; what the choice gives is the subject piece's stops for
  provenance. Fix: "Along a common pair the walk follows the subject's piece. A common pair has no interior
  stops, so the output there is its two vertices' output positions whichever piece is walked; what the
  choice gives is the subject piece's stops, for provenance (section 5.4)."
- **Section 3.3's caller list (`:364-377`)**, which `:1050` cites, omits `is_outside_by_bounds`
  (`PolygonPartitioner.cc:91-95`), a sixth caller of `is_outside_bounding_small_circle_outside_polygon`. Fix:
  add "- `is_outside_by_bounds` in `PolygonPartitioner.cc` (section 5.1), which answers *outside* without an
  overlay when the two bounding circles are disjoint, only if each polygon's interior lies within its own
  circle."

Checked in my own lines and right: see section 2.

### 1.9 Low: sections 5.4 to 5.7 (run where said; otherwise a subagent's reading, sampled)

- **`:1707-1709`**, "a bow-tie's piece has an area near zero, so a bow-tie three quarters inside one plate
  goes to the plate holding the other quarter". *Run* (`part3/bowtie.py`, review 8's geometry): the plate's
  piece measures zero and the other plate's two triangles 0.1251 square degrees each of the bow-tie's 2.0,
  so the split is seven eighths against one eighth by area; "three quarters" is the plate's share of the
  bow-tie's height. And a piece measures zero only when its lobes are equal. Fix: "A ring that crosses
  itself is measured by its signed area, in which lobes wound opposite ways cancel. A piece with two equal
  lobes measures zero: a bow-tie with seven eighths of its area in one plate, cut across both lobes, goes to
  the plate holding the other eighth." The same "three quarters inside a plate" is at `:1537`,
  `PolygonOverlay.cc:4729` and `PolygonPartitionerTest.cc:1548`; there "three quarters of its height" would
  do.
- **`:1682-1684`**, "(`Coverage::exterior_ring_only`, section 3.3's note) is partitioned as the exterior
  ring alone". Section 3.3 has no such note (the page mentions the form nowhere else), and the polygon is
  cut whole, holes and all; only the coverage is limited (*run*, `features2.py` case C: a piece keeps its
  hole, with 4 scalars for 8 points). Fix: "(`Coverage::exterior_ring_only`,
  `ScalarCoverageFeatureProperties.h`) stays in that form. The polygon is cut whole, and each piece has
  scalars for its exterior ring alone: a hole the cut opens into an exterior ring gets values by step 2,
  and a hole left whole gets none."
- **`:1791`**, "coincident rings (rings of no area that clean-up missed, `coincident_rings`)", against
  section 7 at `:2395-2397`, "two coincident rings with area ... come out as two polygons". Section 7 is
  right (*run*, `part5/sec7b.py` case A2: two identical four-point polygons outside). Fix: "coincident rings
  (`coincident_rings`: rings of no area that clean-up missed, or a ring an invalid input repeats, section
  7)".
- **`:1787-1790`**, the exception "carrying the `Diagnostics` and the first failing vertex's position".
  That is `classify()`'s throw; the two hemisphere throws carry the reference point
  (`PolygonOverlay.cc:2691`, `:4587`) and the pairing throw none (`:4520`). Fix: "carrying the `Diagnostics`
  and a position: the first failing vertex's, or a hemisphere check's reference point".
- **`:1796-1798`**, "whose measure was the one place a wrong number silently changed a result (the
  great-circle ring above, since fixed)". The page now records a second (a ring that crosses itself,
  `:1535-1538`), and section 7 a remaining one. Fix: "The sliver filter is outside this guarantee: it is a
  distance rule, and its measure has twice changed a result silently (the great-circle ring above, and a
  ring that crosses itself, section 5.2). A ring that crosses itself only within a few margins can still be
  dropped (section 7)."
- **`:1695-1698`**, "lexicographically with `real_t`'s comparisons". C++ only
  (`PartitionFeatureUtils.cc:901-915`); pyGPlates compares a tuple of floats exactly
  (`PlatePartitioning.py:468-469`), so two plates whose areas differ by under `1e-12` tie in GPlates and not
  in pyGPlates. Fix: "lexicographically (in C++ with `real_t`'s comparisons; pyGPlates compares the floats
  exactly)".
- **`:1615-1618`**, "every output point carries ... its location on the clip that made it". An original
  subject vertex in no record has no clip location (`PolygonOverlay.h:748`). Fix: "and, for a point on a
  clip's outline, its location on that clip, with the clip's index in a cascade".
- **`:1763-1764`**, "raises `ValueError` if negative". Also for NaN (*run*). Fix: "if negative or NaN".
- **`:1837-1838`**, "Its phase-1 case table remains the specification `GeometryIntersect` was audited
  against (section 4)". Section 4 says no such thing (it names the paper once, as a rejected alternative).
  Fix: drop "(section 4)", or "Its case table for the intersection phase was the starting specification of
  what partitioning needs from `GeometryIntersect`".
- **`:1652-1654`**: a sentence inserted before "A coverage then interpolates them along their run" changed
  what "them" refers to. Fix: move that sentence up to follow "... 3% in the dense chain set." and write
  "such points" for "them".

### 1.10 Low: sections 5.8, 6 and 7

- **`:1908-1909`**, "It is the only polygon-by-polygon clipper in the code." False since the overlay. Fix:
  "It is separate from the overlay (section 5.2) and shares no code with it or with `GeometryIntersect`
  (section 4.1): it finds its own crossings, against its own tolerance." Section 5.8 is otherwise unchanged
  since `04e75e54a` and PR B does not touch `DateLineWrapper`; a sample of 24 of its statements held. Its
  closing block (`:2114-2143`, "what generalises") still reads as a study for a general clipper that now
  exists and works differently; one sentence would say so: "The overlay takes nothing from this code:
  tracing by entry and exit flags was rejected for a general clip polygon (section 5.7), and putting
  `DateLineWrapper` onto the overlay is deferred (section 7)." Smaller, by a subagent's reading of
  `DateLineWrapper.cc`: `:2096-2097` "whether each ratio is above `0.5`" (only the end point's is tested,
  `:1208`, `:1223`); `:2111-2112` "The tolerance is one global number" (the section itself lists `1e-4` and
  `real_t` tests too).
- **`:2315-2319`**, the hunt's 9 cases "(section 7)". Section 7 lists the 4 + 4 but not the ninth (two
  crossings `1e-16` apart, whose output is right), and section 6 never mentions the one case that fails in
  the prototype and the C++ alike, though `src/unit-test/data/README.md:39` sends the reader to section 6
  for it. Fix: "...are two vertices in the C++ and one in the prototype, the output right (the first eight
  are in section 7). One more case fails in both, the inconsistent cut of section 5.6."
- **`:2317-2318` and `:2394-2395`**, "4 coincident rings of no area". The record is "at least two of them
  the zero-area rings' cases (likely all four)" (`PLAN.md:851`). Fix in both: "4 with coincident rings (at
  least two of them rings of no area)".
- **`:2404-2405`**, "Both tests decide at sampled points clear of the outlines". Both also have an edge
  test (`PolygonOverlay.cc:542-552`, `:561-574`). Fix: "Both tests decide only from points, and from edges
  whose ends are, farther than the margin from the other outline".
- **`:2239-2240`**, "`PolygonOnSphere.partition()`, whose pieces end at the intersections along the
  polyline". True of the code PR A tested, not of PR B's. Fix: "whose pieces then ended at ...".
- **`:2330-2332`**, "provenance through the cookie cutter (a multi-point, two holes, ...)". The first two
  call `PolygonPartitioner::partition_geometry` (`PolygonPartitionerTest.cc:535-`, `:570-`), and the cascade
  has a third, far plate. Fix: "provenance (a multi-point and a piece keeping two holes through the
  partitioner's own entry; through the cookie cutter a cascade of two plates with a hole across their
  shared edge and the input's closing segments across both, a far plate between them returning a piece
  whole, and the other geometry types)".
- **`:2356-2361`**, the real-data test: `GPLATES_POLYGON_OVERLAY_REAL_DATA_TIMES` is optional (0, 100 and
  200 by default, `PolygonOverlayRealDataTest.cc:67`), and cuts whose input rings cross are read by the
  oracle as rings, not as polygons (`:321-348`; the run prints 6 such cuts, all at 0 Ma). So the only
  real-data cuts that reach the grouping of crossing rings are not checked as polygons. Fix: "and optionally
  the times in `GPLATES_POLYGON_OVERLAY_REAL_DATA_TIMES`, 0, 100 and 200 Ma by default)"; and after "no
  check failure even before repair," add "(the 6 cuts at 0 Ma with crossing input rings read by the oracle
  as rings, not as polygons),". Or have the harness read them as polygons, as the prototype comparison now
  does.
- **`:2304-2306`**, rings compared "where the prototype's did not depend on the tie order". Also only where
  no band piece differs and the rings do not touch at two vertices
  (`PolygonOverlayPrototypeTest.cc:1393`, `:1485`). Fix: add "no band piece differs and the rings do not
  touch at two vertices (which decompose either way)".
- **`:2387-2388`**, "(...the 4 generated cases once put down to it were rings meeting at shared vertices,
  section 5.2)". Section 5.2 does not mention them; section 6 does. Fix: end at "which the grouping of
  crossing rings reads right, section 5.2)".
- **`:2432-2433`**, the parity-area item names only interior rings; section 5.4 now gives a ring that
  crosses itself the same limit. Fix: add "and for a ring that crosses itself (section 5.4: its lobes
  cancel, which misleads the most overlapping plate)".
- **`:2343-2345`**: `AssignPlateIdsFeatureCases` also holds a time-dependent range kept beside a cut
  constant geometry. Fix: after "a range matching no geometry" add "(removed if constant, kept if
  time-dependent)".
- Section 7's third item (`:2390-2399`) is titled for the inconsistent cut but also holds the hunt's eight
  open cases and two invalid-input limits. A separate item, "**Invalid inputs with rings in common**", from
  "two coincident rings with area", would read better.
- Not in section 7: the `partition_features` rework decided at `PLAN.md:1031-1032` (pyGPlates' feature
  assembly through the C++ tasks). It would also settle 1.2. Whether it belongs there is the developer's
  call.

### 1.11 Low: citations that will not survive `docs/plans/`, and progress told as history (reading)

By `AGENTS.md` a design document does not record progress, and nothing in it should need the plan to be
understood. The page was clean of this at `04e75e54a`; the eight commits since brought some back, and the
code and tests carry the same names. None of these names is in the committed test data.

**Review numbers.** The page: `:1458` ("review 7's square"), `:1460` ("review 8's pinwheel"), `:1473`
("review 8's random probe"), `:1538` ("(review 7)"), `:1542` ("review 8"), `:1706`, `:1709`, `:2395` ("From
review 7's random probes"), `:2402`. The code: `PolygonOverlay.h:201`, `:204`, `:209`;
`PolygonOverlay.cc:459`, `:529`, `:577`. The tests: `PolygonOverlayTest.cc:2059`, `:2077`, `:2367`
("review 7's finding 1.1"); `PolygonPartitionerTest.cc:1502`, `:1548` ("review 7's finding 1.2"), `:1568`.
In every case the shape is described beside the citation, so the fix is to drop the citation: "a square
with an interior ring through three of its corners", "a pinwheel", "a random probe of polygons with several
crossing rings, snapped to a grid of 0.004 radians".

**Runs, phases, pull requests and other plan names.** The page: `:1154-1155` ("the prototype's first run"),
`:1339` ("The residue of the prototype's second run"), `:1385-1386` ("under the chosen configuration"),
`:2311-2312` ("live in the planning repository"; the tests and the data README say "kept outside this
repository"), `:2382` ("are the next pull request, which also decides"; say "are deferred, with the
decisions of ..."), `:2315-2316` ("about fourteen times the earlier sets", which the page does not name).
The code: `PolygonOverlay.cc:3806-3807` ("run 3's inversion residue", "any normal Phase 4a run"), `:1752`
("prototype configuration 'r5o'", now `r5p`). The tests: `PolygonOverlayPrototypeTest.cc:1161` ("PR C"),
`PolygonOverlayTest.cc:2308` and `PolygonPartitionerTest.cc:1475` ("the visual check's case"),
`PartitionFeatureUtilsTest.cc:1099` ("the src/api audit's G8-1, decided 2026-11-07"). `r5p`, "the hunt",
"Phase 4a prototype" and "run 2's residue" are used consistently by `PolygonOverlayPrototypeTest.cc` and the
data README, which describe the frozen snapshot they name, so those can stand; `PolygonOverlayTest.cc:868`
still says `r5o`.

**Progress told as history** (each records a trap, so recast rather than delete): `:1446-1447` ("the hunt
found 11,212 of its 70,291 cases ... against 255 counted as grouping failures") and `:2319-2324` ("11,212
wrong ... before crossing rings were grouped, 3 with the first crossing test ..., and none once edges'
midpoints were tested (... which were also the 4 cases first put down to the symmetric difference's
pairing)"): say what the grouping and each point of the test are for, with the count, once. `:1706-1707`
("and before such rings were grouped, as separate polygons, they summed too large instead"). `:1689-1693`
("Step 2's loop once ran its polyline end rule for closed rings too"): a bug of this branch fixed before any
release; it belongs as the comment already at `PartitionFeatureGeometries.cc:861-863`. `:1751-1753` ("Built
first with ... The apex is now ..."), `:1798` ("since fixed"), `:1064-1066` ("The one other user of the old
polyline-cutting code ... wanted ... and now takes"). And `GLReconstructedStaticPolygonMeshes.cc:980-983`,
the comment of 1.1.

"The developer's decision" and "found by review" appear about ten times (`:1090`, `:1173`, `:1237`,
`:1430`, `:1500`, `:1557`, `:1676`, `:1890`, `PolygonOverlay.cc:4705`); they were there at `04e75e54a` and
are borderline. Listed once, not ranked.

### 1.12 Low: docstrings (run and reading)

- **`PartitionError`** (`PyExceptions.cc:485-492`): "This can happen where the two outlines run within
  about 1e-6 radians of each other along many closely spaced vertices." `e7b66c3e8` corrected exactly this
  description in both changelogs, the page, the test comment and the data README ("which overstated how a
  valid polygon could hit it") and missed the docstring. Fix: "The only case found is two polygons that
  each have an interior ring crossing their exterior ring, with outlines running within about 1.4e-6
  radians (9 metres on the Earth) of each other along many closely spaced vertices." The stub carries
  docstrings, so regenerate it with the change.
- **`PlatePartitioner.partition_features`** (`PlatePartitioning.py:156-313`) has no `.. versionchanged::
  1.1`, though it is where most of 1.1's changes to partitioning arrive (polygon pieces, coverages carried,
  time-dependent geometry, a feature with no geometry returned, `PartitionError`); `partition`,
  `partition_geometry`, `PlatePartitioner.__init__` and `partition_into_plates` each have one, and
  `partition_into_plates`' names only the two new arguments. Suggested, in both:
  ```
      .. versionchanged:: 1.1
         A polygon is partitioned into polygons (previously polylines). With *PartitionMethod.split_into_plates*
         a geometry's scalar coverage is carried to its pieces, a time-dependent geometry is partitioned only in a
         single time window containing the reconstruction time, and a feature with no geometry is returned
         unpartitioned. *PartitionMethod.most_overlapping_plate* measures polygons by area.
         :class:`PartitionError` is raised if a cut is inconsistent.
  ```
- The exception's message is the overlay's own: "The overlay's classes are inconsistent after 20 repairs: 2
  vertices have no consistent arrangement and 0 pieces are unresolved. The first is at latitude 40.046,
  longitude -117.547, with records LS(9,4)." The latitude and longitude are what a user needs; "classes",
  "repairs" and "records LS(9,4)" are the implementation's words. Not a claim, so not a finding against any
  document; noted because the dialog deliberately does not show it.

Everything else in the docstrings holds, and they follow `docs/pygplates/README.md` (type fields
markup-free, `, or` unions, no single backquotes, `:returns:` before `:rtype:`). `pygplates-stub-test`
passes, so no docstring at the tip leaves the stub stale.

### 1.13 Low: `PLAN.md`

- "Resume here" (`PLAN.md:1063-1068`) ends "Open as before: the prototype's polyline rule (`r5p`); the
  margin order's hand-built test." Both were closed by the entry above it (`:1010-1020`: the prototype is at
  `r5p`; the margin order "Dropped from the open items"), and the same sentence says "the prototype is at
  `r5p`".
- `PLAN.md:1048` says mutations "of the self-crossing test's sides and of its undecided rule each fail a
  test". True of the two mutations run (`grouping-fix/mutate_r8.py`); the other half of the undecided rule,
  the two-margin arms, is held by nothing (1.7).
- `PLAN.md:942-947` and decision 4 at `:290`: see 1.1.
- The timing records drift without saying so: 0.280 s at 0 Ma on 2026-11-08 (`:918`), 0.33 to 0.34 s on
  2026-11-12 (`:972`, `:996`, `:1056`), with only `56952f9b5` and `b84512ef6` between, and each A/B taken
  against the commit before, never against `main`. See 1.4.

## 2. Checked and found sound

- **`e7b66c3e8`** (*run*, `py_claims.py`). `inconsistent.txt` is two polygons of two rings each. Each hole
  has vertices on both sides of its own exterior (2 of 4 inside for `P`, 3 of 4 for `Q`) and an outline
  distance of zero from it. `Q.partition(P)` raises `PartitionError` at the default width, at zero and at
  `10 h`; with `P`'s hole removed, with `Q`'s removed, or with both, it succeeds. (The other way round,
  `P.partition(Q)`, succeeds with both holes: the failure is of one order only.) The page's 5.6 and section
  7, the test comment (`PartitionFeatureUtilsTest.cc`), the data README and both changelogs say this
  consistently; the one place it missed is 1.12.
- **The changelogs against the user-visible diff** (*reading*, and *run* where a claim could be run). Every
  user-visible change has a bullet in the product it shows in: polygons cut into polygons, holes taking
  part, touches not split, crossing rings, coverages on pieces, the sliver filter and its argument,
  single-window geometry, the unmatched coverage (GPlates only: pyGPlates dropped every coverage before, so
  nothing changed there), the most overlapping plate by area, the skipped features and `PartitionError`,
  and the feature with no geometry. No bullet describes an intermediate state. GPlates' bullets speak of
  plates, holes and the dialog, pyGPlates' of API names. *Run*: a polyline along a boundary shared by two
  plates goes to the first in the sort order; a geometry touching from either side, along the outline, or
  identical comes back whole and is the object passed in; a surrounding polygon gives the plate's region
  inside and a polygon with one hole outside; `ValueError` for a negative or NaN width from all three entry
  points; strips up to `4 h` wide are dropped at `10 h` and the subject is then returned whole outside; a
  bow-tie's `get_area` is `3.7e-7` against `3.0e-4` for one lobe; `PartitionError` derives from
  `MathematicalError`, is raised with and without lists, leaves the caller's lists untouched, and fails the
  whole of `partition_features` under both methods. The dialog lists up to ten skipped features and counts
  the rest, also after a cancel; the command writes one warning each. One nit: the GPlates bullet on
  crossing rings says "Such a piece is not dropped as a sliver" two bullets before slivers are introduced.
- **The numbers in the docstrings and changelogs** (*reading*): `h = sin(acos(1 - 1e-12)) = 1.41421e-6`
  radians, 9.0 metres; `PlatePartitioner`'s default `10 h = 1.4142e-5`, 90 metres
  (`GeometryCookieCutter.cc:328-331`); `partition`'s default `h` (`PyGeometriesOnSphere.cc:3654-3656`).
- **Lines 1439-1573 of the page** (*reading*, against `PolygonOverlay.cc`), apart from 1.8: the outline
  distance settled first (`:530`), after the bounding circles (`:519`); the point test's points (every
  vertex; at a near vertex the midpoints of both its edges and the points three margins along them,
  `:436-506`); a far node counted outside at once (`:469-479`); the edge test with all four ends clear of
  the other's great circle by the margin's sine (`:255-268`, `:552`); containment by the first vertex
  farther than the margin, else the farthest of the vertices and edge midpoints, none if all are on the
  other ring (`:356-401`); the margin `max(10 h, 2 x the widest cluster)`, the oracle's
  (`PolygonOverlayOracle.h:169-178`); depth, parent, the largest ring as exterior, a hole group in its
  parent's polygon, the nesting condition and the counted failure (`:4669-4846`); orientation by
  `PolygonOrientation`; the hemisphere check's candidates in order and its three tests with the threshold
  off (`:2587-2659`, `:4558-4594`); the whole answer's candidates (`:2611-2636`) and its skip; the filter
  outermost first, a group not filtered itself but dropped with its parent, the self-crossing test asked
  only of a ring that measures as a sliver (`:4734-4785`); the self-crossing test's two parts, its probes
  three margins along both paths, each farther than the margin from the other path, and the two-margin arms
  (`:556-718`); polylines (the along class, the test points, deduction across one LL record only, the one
  check, `:2697-2887`). The counts quoted agree with the plan: 11,212 of 70,291 and 255 (`PLAN.md:955`), 5
  of 119,198, 2 in 11,668 and none generic or at 0.002 (`:1053-1054`), 39,320 valid cases (`:1054-1055`,
  review 8).
- **Lines 1025-1337** (a subagent's reading, with a 19-case pyGPlates probe of the result contract, the
  same with and without lists; I sampled its findings): no medium finding; every name, caller, order,
  condition and constant matches. The measurements agree with the plan's records.
- **Lines 1338-1438** (a subagent's reading): the bundle rule, checks and repair, linking (all 30 cells of
  the table worked from `f`), split and clean-up, apart from 1.8.
- **Sections 5.4 to 5.7** (a subagent's reading and runs; I re-ran `features.py` and `features2.py`):
  provenance and its composition, the three coverage steps (a crossing gets 10 between 0 and 20; a plate
  inside the subject gets the nearest-outline values), the most overlapping measure (0.0121 against 0.0303
  sampled), the sliver widths by entry point, the four throws, the coastline figures.
- **Section 6** (a subagent's runs): every test, fixture, data file and environment variable named exists
  and does what is said; the data files hold 463, 274, 189, 48 and 19 cases; the eight generators are those
  in `prototype-hunt.txt`; `PolygonOverlayRealDataTest.Coastlines`, run for real, reproduces 15,255
  overlays, 313 intersecting, no repair, 11 midpoint tests, 2 collapsed rings, 783 rings with 5, 16 and 73
  under `h`, `10 h` and `100 h`. No superseded hunt figure (13 cases, 35 polyline cases, `r5o`) is left in
  sections 6 and 7. All eleven tests added since `04e75e54a` are mentioned; none was renamed.
- **Section 7** (a subagent's runs): nothing listed is done at the tip, and every reference to section 7
  from elsewhere lands on an item but for the ninth hunt case (1.10). A second ring overlapping the exterior
  by a strip up to `9 h` wide comes out as two overlapping polygons, and one polygon from `11 h`; a bow-tie
  of half-size up to `10 h` is returned whole outside at the default width and cut from `15 h`: the limits
  section 7 states.
- **Section 5.8** (a subagent's sample of 24 statements against `DateLineWrapper.{h,cc}`): its account of
  that code holds; see 1.10 for what is stale beside it.
- **Qt5** (*reading*). PR B touches `src/qt-widgets/AssignReconstructionPlateIdsDialog.{h,cc}` (a
  `QStringList`, a `QMessageBox`, `tr()` with `%n`), nothing version-conditional. The plan's Linux run
  (Ubuntu 22.04, the apt route) is a Qt 5.15 build, and passed at `555a2f2b0`; the tip differs from it by
  the plan alone.
- **Leftovers** (*reading* of the diff's added lines): no `TODO` of this branch's, no debug output outside
  the two opt-in test harnesses, no test named for behaviour it no longer checks (`GroupRingsCross`'s
  header comment lists less than the test now holds, but nothing false).
- **The suites** (*run*): counts in the header.

## 3. What remains unpinned

The corrected list for `PLAN.md`, by mutation at the tip (1.7's table):

- The point test of `do_rings_cross` in one argument order (every crossing built is seen both ways, and the
  reference of `GroupRingsCrossAgainstLoops` is itself both ways round).
- The nesting check's fallback (a hole group kept as a polygon of its own, `grouping_failures`): no
  committed input reaches it.
- The sliver filter's exemption of a group of crossing rings.
- **New:** the filter's condition that a hole is filled only when its parent is an exterior group.
- `do_arcs_cross`'s same-crossing sign test.
- The order of holes in a polygon.
- The far-node shortcut's count as outside (not pinnable: a ring with a point inside the other has a point
  outside it in a near node too).
- **New:** the self-crossing test's two-margin arms (the plan's "undecided rule" mutation was the probe
  within the margin of the path, which is pinned).
- The self-touch cuts' key (no constructed input has a vertex with two stops on one segment).
- Speed only, so not pinnable by a result: the outline-distance shortcut, and the bounding-circle return
  before it. Nothing committed times `extract`.

No longer unpinned, against review 8's table: `classify()`'s call of the whole answer's check and
`whole_checks` for a skipped check (`CheckWholeResult`), and the filter's self-crossing test for a hole
(`SliverFilterSelfCrossingRing`). Pinned and not listed before: the probe distance of three margins, and
the largest ring as a group's exterior.

Not pinned by any test, from this review: the load-bearing `real_t` comparison of 1.5 is pinned only by
accident (seven tests throw without it); `GLReconstructedStaticPolygonMeshes`' partitioner path (1.1) is
unreachable, so untested and untestable; the two differences of 1.2 and the kept range of 1.3 have no test
either way; the real-data harness does not read a cut with crossing input rings as polygons (1.10).

Not looked at: a Linux build of the tip (it differs from `555a2f2b0`, which passed, by `docs/plans/` alone);
the documentation build (no docstring changed since `05620ce97`'s clean build from scratch).
