# PR C claim check: the distillation, the two overviews, the Boolean methods' text, the overview convention

Branch `feature/polygon-intersections` at `b4a3b6108` (clean), 2026-11-13. "The page" is
`docs/design/architecture/foundation/spherical-geometry.md`; line numbers are of the files at that
commit. Checked, against the code as it is:

- the page's text added or changed by `git diff 49d19f07b HEAD` (the distillation, and the
  exact-hemisphere bullet of section 7) and by `git diff 208792025 49d19f07b` (PR C's earlier edits);
- `polygon-overlay-overview.md` and `point-in-polygon-overview.md`, whole;
- the comments added by `git diff 49d19f07b HEAD -- src/`;
- the docstrings added by `git diff 208792025 HEAD -- src/api/`, and the `CHANGELOG-pyGPlates.md`
  bullet;
- the overview convention in `AGENTS.md`, `README.md` and `docs/design/architecture/README.md`
  (links and internal consistency only).

How. Each statement of behaviour was read against the function that makes it true (not its
comment). Where reading was not enough I ran the built products: pyGPlates `1.1.0.dev103` from
`build-pygplates/bin` (five scratch scripts, not kept: every worked example of both overviews, the
docstrings' statements, the hemisphere cases, the sliver argument), and `gplates-unit-test`
(`PolygonOverlayPrototypeTest.*`, `PolygonOverlayTest.Link*`, `ExtractPolygons`,
`UnionOverHemisphereThrows`, `AssignPlateIdsSkipsInconsistentFeature`: 20 pass, `File` skipped by
design). Measured figures were checked against `prC-symdiff-twist.md`, `PLAN.md` and the phase
reports the distillation cites (`phase2-requirements.md`, `phase3-audit.md`, `phase4a-run2.md` to
`-run4.md`, `phase4b-design.md`, `phase4b-survey.md`); none was re-run. Nothing was rebuilt, and no
file but this one was written.

Severity: **high** false, and a reader or the code would be misled; **medium** misleading or stale;
**low** imprecise, or a broken link.

Summary: 0 high, 1 medium, 11 low. Nothing in the page, the overviews or the comments misdescribes
what the code does. The medium finding is one sentence of the `union` docstring. Two figures have no
record (findings 3 and 4). Every link and anchor resolves.

## 1. Findings

### 1. medium: `union`'s docstring says a narrow gap between two polygons is dropped

`src/api/PyGeometriesOnSphere.cc:4760-4762` (the `union` docstring): "polygons apart from each
other, or touching only at points, are returned separately. A gap between them narrower than
*sliver_width_threshold_radians* is a sliver hole, and is dropped."

Evidence: read after "polygons apart from each other", "a gap between them" is the gap between two
separate polygons, and that is not closed. Run: two 10-degree squares with an open gap of `5e-6`
radians between facing edges, `union(..., sliver_width_threshold_radians=1.4142e-5)`: two polygons
(the same for `symmetric_difference`). The filter drops rings only: `is_sliver` is asked of an
exterior group, or of a hole group whose parent is an exterior (`src/maths/PolygonOverlay.cc:5454-5504`).
A gap is dropped only when the merged polygon encloses it. Run: a slot `5e-6` radians wide enclosed
between the two squares is a hole of the union at the default width (it is wider than about 9
metres) and at zero, and is dropped at `1.4142e-5`. (An open gap of `0.5e-6` radians does merge the
two, at the default width, at zero and at `1.4142e-5` alike: the edges are within the thickness and
count as shared. That is the "share an edge" case, not the filter.)

Fix: "Polygons that overlap, or share an edge, are merged into one polygon; polygons apart from each
other, or touching only at points, are returned separately. Where the merged polygon encloses a gap
between the two (a hole) narrower than *sliver_width_threshold_radians*, that sliver hole is dropped
and the polygon fills it; a gap that is not enclosed is never closed."

### 2. low: the cascade measurement, "nor the widest cluster of records grew"

Page 1130-1133: "Measured in the prototype over 157 cascade steps with shared edges jittered by up
to `2 h`, neither the identity groups nor the widest cluster of records grew from the first step to
the fourth, and no step failed."

Evidence: `phase4a-run2.md:415-421`, the table of the 157 steps (40 + 40 + 40 + 24 + 13 cascades at
steps 0 to 4). The identity groups' extent is 0 at every step. The cluster width is `1.61 h`,
`1.63 h`, `1.52 h` and `1.77 h` at steps 1 to 4: the widest is at the fourth. The record's own
prose (`:424-425`, "the widest cluster at step 4 (`1.77h`) is no wider than at step 1") contradicts
its table, and the page repeats the prose. The width is also that of the rejected contraction
variant (`V2b`, section 5.7), not of anything the design keeps. The 157 steps, the `2 h` jitter and
"no step failed" are right (also `phase4a-run3.md:301`, the final configuration: 157 steps, 0
failures).

Fix: "Measured in the prototype over 157 cascade steps with shared edges jittered by up to `2 h`, no
identity group joined two records at any step, the widest cluster of records stayed between `1.5 h`
and `1.8 h` from the first step to the fourth (the jitter's size, with no trend), and no step
failed."

### 3. low: "88 of the hunt's cases whose input rings cross" has no record

Page 1640-1641: "The cut is also made in 88 of the hunt's cases whose input rings cross (59 of them
in the intersection or the difference)".

Evidence: `prC-symdiff-twist.md` section 6 has the 59 (`:320`, "59 cases, 60 operations") and the
operations that differ as exported for crossing inputs (`:295`: 29, 31, 10 and 26 in the
intersection, the difference, the reverse difference and the union, 96 operations), but no count of
cases over every operation. 88 is in neither that report, `PLAN.md`, the code's comments nor the
tools kept in the planning repository (`plans/polygon-intersection-tools/cpp/prC/`). It is
consistent with the table (96 operations in 88 cases needs 8 cases cut in two operations) and cannot
be derived from it.

Fix: either cite the scan that gave 88 in `prC-symdiff-twist.md` before the plan is archived, or say
only what is recorded: "The cut is also made in cases whose input rings cross (96 operations of the
hunt as exported, in the intersection, the differences and the union; 59 cases in the intersection
or the difference), where a ring crosses itself because an input does".

### 4. low: "at two vertices of six ends each" has no record

Page 2709-2710: "(`six#13 seed 11`: three intersection rings against the prototype's two, at two
vertices of six ends each)".

Evidence: `prC-symdiff-twist.md:264` ("3 intersection rings against the prototype's 2") and `:457-458`
("Not looked into beyond that: the prototype's two rings there share one named point and the overlay
has three rings"); `PLAN.md:1147`. Nothing recorded says where the rings differ or how many ends the
vertices have, and the case is not in the committed data, so it cannot be run here.

Fix: drop the clause, or record its source: "(`six#13 seed 11`: three intersection rings against the
prototype's two, which share one point)".

### 5. low: 28 of 35 mutations "fail one", but one of the three added only hangs

Page 2773-2777: "28 of the mutations fail one: 25 at first, and three more since three of the hunt's
cases were committed to hold them (the walk's stop at the other passage, without which the walk does
not end in a dense shared edge; ...)".

Evidence: `prC-symdiff-twist.md:433-435`: "C12 fails `Hunt` and `CutsAtChainsAndSpikeWalls`, J8 fails
`CutsAtChainsAndSpikeWalls`, and C7 hangs (no result in 180 s), as expected." The planning
repository's `mutations.log` has the reruns of C12 and J8 only. So 27 mutations fail a test, and
with the 28th (C7) the suite does not finish. The page's own parenthesis says as much; "fail one"
does not.

Fix: "and 28 of the mutations no longer pass: 25 failed a test at first, and three more since three
of the hunt's cases were committed to hold them (two fail a test; with the third, the walk's stop
at the other passage removed, the walk does not end in a dense shared edge and the test never
finishes; ...)".

### 6. low: one of the four dateline defects is not in section 5.8's "dateline-only" list

Page 2864-2867: "The port would cure what section 5.8 lists as dateline-only (a hole that crosses the
dateline merged into an exterior, the guess when both poles are touched, the ring run backwards at a
touch, the grouping of the other rings by an epsilon distance)".

Evidence: the list headed "Dateline-only" (page 2471-2486) has the merged hole, the fall-back to a
guess and the epsilon grouping. The ring run backwards at a touch is in the paragraph before it,
*Degeneracies* (page 2461-2464: "it runs the geometry backwards, so the output's orientation is not
preserved"), and is not in the list.

Fix: "The port would cure four defects of section 5.8 (a hole that crosses the dateline merged into
an exterior, the guess when both poles are touched, the grouping of the other rings by an epsilon
distance, and the ring run backwards at a touch)".

### 7. low: "the hunt has none wider than a thickness" holds for valid inputs only

Page 2883-2885: "a ring twisted where an arc passes a vertex of its own input with no common piece
between them is not looked for, and the hunt has none wider than a thickness."

Evidence: the twist scan covers the 39,320 cases with valid inputs (`prC-symdiff-twist.md:272-281`,
and `:450-452`, doubt 5). For the hunt's 70,291 cases whose input rings cross, rings cross
themselves wherever an input does (section 5.2 says so, page 1640-1642), and no scan of them is
recorded. Section 5.2's own sentence (page 1638-1640) has the qualifier.

Fix: "and no valid input of the hunt gives one wider than a thickness."

### 8. low: the *Linking* paragraph still names `extract_rings` for the pairing and the walk

Page 1486: "**Linking** (`is_kept`, `extract_rings`)", heading the paragraph both diffs edit.

Evidence: since PR C the pairing and the walk that the paragraph describes are
`Overlay::pair_and_walk_rings` (`src/maths/PolygonOverlay.cc:4031`), called through `walk_rings`
(`:4416`); `extract_rings` (`:5140`) calls `walk_rings`, cleans up and throws. The distillation
corrected the same name in section 7 (page 2855, now `pair_and_walk_rings`) and left this one. Not
false, since `extract_rings` is still the entry, but it no longer holds the code described.

Fix: "**Linking** (`is_kept`, `pair_and_walk_rings`, from `extract_rings`)".

### 9. low: the `intersection` docstring's example needs the second polygon inside the first

`src/api/PyGeometriesOnSphere.cc:4711-4713`: "for example, the intersection of a polygon with a hole
and a polygon covering that hole is the second polygon with the same hole."

Evidence: true only when the second polygon lies within the first's exterior ring. Run: a box of 10
degrees with a hole from 3 to 7, and a box from 2 to 8: one polygon, the second box's four points
with the hole's four. A polygon that covers the hole and reaches past the first's exterior is cut by
that exterior. Run: with the second box from 2 to 12 degrees one way, the result has about half the
second polygon's area.

Fix: "for example, the intersection of a polygon with a hole and a smaller polygon inside it that
covers the hole is the smaller polygon with the same hole."

### 10. low: `AGENTS.md` says an explainer is "archived with the plan", and the same section deletes plans

`AGENTS.md:385-388`: "At distillation it becomes the design's overview, ...; otherwise it is archived
with the plan."

Evidence: the section has no archive. A long-lived branch's plan is deleted (`AGENTS.md:404-407`:
"then delete `docs/plans/<name>/`", "the last one deletes the plan") and `:415`: "Once a plan is
deleted, anything it held that was not distilled survives only in git history". Nothing in
`AGENTS.md`, the root `README.md` or the architecture README says where a plan is archived.

Fix: "otherwise it goes the way of the plan: what lasts in it is distilled, and it is deleted with
the plan."

### 11. low: "claim-checked like an area page" refers to a procedure the repository does not describe

`AGENTS.md:388-389`: "The developer may also ask for an overview of existing code: it is written from
the code and claim-checked like an area page."

Evidence: "claim" occurs nowhere else in `AGENTS.md`, the root `README.md` or
`docs/design/architecture/README.md`. The architecture README says only that "A page describes the
code as it is" (`:33`).

Fix: "it is written from the code, and every statement in it is checked against the code, as an
area page's are."

### 12. low: the architecture README's new section against the rest of that README

`docs/design/architecture/README.md:39-42`: "There are three kinds of document here, read in this
order: 1. **An area's page** (its `README.md`) says what the area is for and how its parts fit
together. Start there."

Evidence, three small inconsistencies:
- The only two overviews the section lists are in `foundation/`, an area with no page: the *Areas*
  table has `Foundation` without a link (`:162`), and `:138-139` says "An area without a link has no
  page yet". A reader of either overview cannot "start there".
- *What is here*, directly above (`:12-27`), lists four kinds (this README, the generated
  `dependency-matrix.md`, the area pages, the design documents); "three kinds of document here"
  leaves two of them out and adds one.
- "Overview" now has three senses in one file: `:5` ("This document is the overview of `src/`"),
  the heading `## Overview` at `:58` (the area diagram), and the new kind of document. `AGENTS.md:124`
  ("Its `README.md` is the overview") and `:130` ("Some designs also have an overview") sit six
  lines apart in the same way. (The distillation report notes this, section 4, item 10.)

Fix: "Three kinds of document describe an area, read in this order: 1. **The area's page** (its
`README.md`, where the area has one yet) says ..."; and rename the heading at `:58` to
`## The areas at a glance`, or call the README "the map of `src/`" at `:5` and in `AGENTS.md:124`.

## 2. Checked and found right

**The conventions' links.** All 50 relative links and anchors in the six files resolve by GitHub's
slug rule (4 in the page, 13 and 17 in the overviews, 10 in the architecture README, 6 in the root
README, none in `AGENTS.md`), and so do the overviews' references by paragraph name (*Records and
vertices*, *Classification* step 4, *Checks and repair*, *A ring twisted at a chain*, *The symmetric
difference*, *Grouping*) and by section number between the two overviews. `AGENTS.md` and the two
READMEs agree on the name (`<design>-overview.md`), the place (beside the reference) and the
authority (the reference and the code).

**The page's introduction.** 186 lines of `src/` and `pygplates/` cite section 5 by number ("about
180").

**Section 1's row for an output ring's arc passing a vertex**, against `FirstArc`
(`PolygonOverlay.h:1274-1313`), `get_first_arc` (`.cc:4732-4775`), the spike test (`:4634-4654`) and
`crosses_arc` (`:4988-5077`): the thickness sine, `ALONG_ROUNDING = 1e-12`, the far vertex by
identity, the first points beyond the thickness each way, and the three cases that decide nothing.

**Section 5.1.** The four bindings (`PyGeometriesOnSphere.cc:3772-3853`): `intersection` and
`difference` through `partition_polygon` (identical to `partition()`'s lists; run: equal lists);
`union` and `symmetric_difference` through `extract_polygons` (`PolygonOverlay.cc:6044-6062`: one
overlay, no bounds shortcut, `create_polygon`, which the partitioner uses too). No operator is
bound. Run: `p.difference(q)[0] is p` is false and `== p` true; a clockwise input apart from the
other comes back from `union` anticlockwise and from `difference` as itself. The result contract's
new sentences (`PolygonOverlay.h:969-971`, `PolygonPartitioner.cc:124-158`, `phase4b-design.md:1686-1692`).
The cascade's argument from the thickness (but finding 2).

**Section 5.2.** Common pairs have no length test (`:1506-1566`), and `h / cos(45 degrees)` is
`1.4 h`. The one-passage rule's three added sentences (`:2191-2310`, `phase4a-run4.md:172-188`,
`:232-254`). Step 3's inversion figures (10 of 1,155; 69, 64 and 5; `3e-15`: `phase4a-run3.md:424-427`,
`:486-500`). The anchorless chain's two limits (`:3700-3752`; `R2` is 60 to 120 degrees from `R`,
`PointInPolygon.cc:2161`). The bundle's enumeration (`:3486-3548`: `k (k + 1) ... (k + m - 1)`
orders, 5,040 for six among two) and the common ray's direction and cone (`:1923-1953`). The
whole-chain repair candidate only at a vertex with a non-common ray (`:3930`). The free rule: with
four kept ends only a bounce is reached (`:4128-4168`; the intersection, the union and both
differences keep two ends of a crossing, and `walk_rings`, `:4421-4447`, never pairs the symmetric
difference). The symmetric difference as two walks, two splits, a join and a third split
(`:4443-4453`), the walls and the spike rule (`:4527-4663`), the join (`:4670-4725`). The split and
the cut at a chain (`:4813-5136`): unkept subject chain pieces, `walks_chain` both ways, the pair
tested both ways round, the walk's stop at the other passage, the sectors of the passage, each part
closed along the chain. The hemisphere check on the rings as walked (`:5259-5310`); run: the union of
two quarter spheres is returned or raises `PartitionError` by the inputs' order and directions (of
eight combinations, two return). Every hunt figure of the section but the 88 is in
`prC-symdiff-twist.md` sections 3, 4 and 6; the timings are its *Timing* table (2.1, 2.8 and 2.0
times; 16%, 26% and 10%; 3%, 5% and 2%).

**Sections 5.3, 5.4 and 5.6.** `Vertex::subject_vertex` is the first named, and the output position
(`:1030-1059`, `:5739-5745`). The size metric's comparison and its one use, a running maximum
(`PartitionFeatureUtils.cc:496-538`, `:901-920`; `real_t`'s `<` is "less by more than epsilon",
`Real.h:274-280`). One failure count serves both differences (`:4444-4445`, `:5232-5241`).
`PartitionError` from all four methods, and the hemisphere case (run: two 80-degree caps 60 degrees
apart raise from `union`; two such caps about opposite poles return two polygons).

**Section 5.7.** Each new or extended entry against its record: Kim and Kim, Martínez et al. and the
rejected check (`phase2-requirements.md:159-161`, `:205-212`, `:598-611`, `:639-641`); exact
arithmetic (`:214-218`); the third local rule (`phase4a-run4.md:198-203`; `solve_vertex` returns
early only for no rays, `:2108`, and `deduce_to_fixed_point` visits every vertex, `:2531`); the
symmetric difference's three entries and the twisted ring's (the report's sections 3 and 4, with
C18's `0.77 h^2`); all plates at once (`phase2-requirements.md:623-634`); the polyline side rule
(`phase4a-run3.md:398-411`, one fixed class at `:2746`); asking once (`PLAN.md:975-978`: 0.331 and
0.338 s before, 0.342 and 0.340 s after); the coverage form and the developer's decisions of
2026-11-13 (`PLAN.md:1602-1622`, `:1137-1140`).

**Section 6.** `LinkTwistAtChain` (the chain is `0.37 h` and `140 h` long by its construction),
`LinkSymmetricDifference`, `check_areas` (four identities, `4 h` times both perimeters),
`ExtractPolygons` (all five operations), the Python `test_boolean_operations*`. The prototype
comparison's exemption (`PolygonOverlayPrototypeTest.cc:1081-1086`), the pinned counts (0, 2, 0, 0;
`{1, 1, 0, 0, 1}` and `{1, 0, 1, 0, 1}`; 13 and 10 walls: run, pass; the symmetric difference's count
is the differences', so the cut cases are two). The determinism paragraph (`:1523-1525`, the only
unordered container; `src/CMakeLists.txt:874-882`). The hunt paragraph and its three scans (the
report's section 6), but finding 4. The seven unpinned conditions and what the hunt says of each
(the report's section 7 and its addendum), but finding 5. No Python test raises `PartitionError`
through `PlatePartitioner` or `partition_features` (`grep`).

**Section 7.** The exact hemisphere (run, above). The walls (the report's sections 4 and 8). The
corner's fallback (`phase4b-design.md:1208-1214`). The open arc (`phase4a-run2.md:475-528`: 13 of 600,
all invalid inputs; no six-end vertex; a polygon round a pole not run; `DateLineWrapper`'s three
users), but finding 6.

**The overlay overview.** Every example was run. The two squares: 4, 6 and 8 points, the symmetric
difference two polygons. The neighbours: no intersection, `Q.partition(P)` outside with `P` whole,
the union and the symmetric difference one polygon of 8 points. `Q` inside `P` against its edge: 4,
8 and 6 points. The hourglass: a triangle, two polygons of 4 points meeting at the apex, a union of
9 points. A hole comes out clockwise in an anticlockwise exterior; an island in a hole gives two
polygons. The sector rule's two worked cases follow from `solve_vertex`'s constraints, and the keep
table is `is_kept` (`:4003-4027`) for all six kinds of piece and five operations. The three layers,
the bounds shortcut, the polyline's along class (`polyline_along_is_inside`, true by default), and
Assign Plate IDs reporting the features not partitioned (`AssignReconstructionPlateIdsDialog.cc:311-315`).

**The point-in-polygon overview.** Run: caps of 30 and 120 degrees either way round; the equator
ring drawn eastwards has the northern hemisphere, westwards the southern; three nested rings, a ring
poking out and a bow-tie by parity; the outline threshold between `1.40e-6` and `1.42e-6` radians.
Against the code and the page's section 3: the boundary centroid is the exterior ring's
(`PolygonOnSphere.cc:720-735`); 34 candidates; the fan sum from the candidate's antipode; the
policy's 8, 20 and 1000 (`PolygonOnSphere.cc:634-636`); `1e-8` steradians (`PointInPolygon.cc:2203`,
0.41 square kilometres); the two circles and the lunes; `use_point_on_polygon_threshold`; the
consumers' table; the four rejected alternatives named.

**The comments.** All seven say what the code beside them does: `BUNDLE_CAP`; `SIDE_*`
(`minwindef.h` defines `NEAR` as `near` and `near` as nothing); `solve_vertex`; the two in
`classify_anchorless_chain`; the unreachable crossing branch (with three output sectors the loop
would pair one subject ray twice: worked through); `GeometrySizeMetric::operator<`; and
`PlatePartitioning.py` (run: a feature's clone iterates its properties in the same order).

**The docstrings and the changelog.** Run for each method: the empty list, touching along an edge
and at a corner, one polygon inside the other, identical inputs, "this polygon itself" (equal, as
`partition()`'s docstring uses the word), the hole of a difference, the merge across a shared edge
by `union` and `symmetric_difference`, `ValueError` for a negative width and NaN, `None` as the
default, the hemisphere error, the example (it runs), and `partition()`'s new sentence. The stub
agrees with the built module (`generate_stub.py --check`, exit 0). The changelog bullet says what
the docstrings say.

## 3. What I could not verify

1. **The hunt and the prototype were not re-run.** Every figure was checked against a record, and
   two have none (findings 3 and 4). The timings (`ScratchTiming`) are the report's alone.
2. **The three papers** (Hobby 1999; Kim and Kim as Foster, Hormann and Popa report them; Martínez,
   Rueda and Feito 2009). The page matches `phase2-requirements.md`'s reading of them; the two
   sources I tried for Foster, Hormann and Popa refused an automated fetch.
3. **"`O(n + k log n)`, the order of the intersection phase"** (page 2103-2104). It is
   `phase2-requirements.md:600-604`'s statement; neither the page's section 4 nor the code states
   the intersection phase's order.
4. **Negative statements**: "no such input has been built" (page 1400), "no committed test was built
   to reach the fallback" (page 1403-1405; no overlay test names the second reference point, but
   nothing counts the fallback, so I did not prove it unreached), "no case of it has been seen"
   (page 1462).
5. **"`symmetric_difference()` can then return two polygons along an edge where `union()` returns
   one"** (page 2819-2820). Two hand-built fold-backs along a shared edge gave one polygon from both
   methods; the hunt's `shared` cases that show an unjoined wall are not in the committed data.
6. **Decisions attributed to the developer.** Those of 2026-11-13 are in `PLAN.md`; "accepted as a
   tie" is `b4a3b6108`'s message; "chosen (by the developer)" for the nearest outline value (page
   1916) is recorded only as the design's recommendation (`phase4b-design.md:1917-1921`).
7. **The docs build, Linux, macOS and Qt5** were not run (no rebuild was allowed). The docstrings'
   `:meth:`, `:class:` and `:func:` targets all name existing entities, by reading.
8. **The point-in-polygon overview's "about 6%"** and "No tectonic plate of the models measured
   comes to half the sphere" restate the page's sections 3.5 and 7, which I did not re-measure.
