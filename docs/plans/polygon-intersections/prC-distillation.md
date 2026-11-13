# PR C distillation report: the final distillation, the two overviews, the overview convention

Branch `feature/polygon-intersections`, worktree `C:/gplates/src/polygon-intersections`, 2026-11-13,
at `49d19f07b`. Every edit is uncommitted. Nothing under `docs/plans/` was touched but this file.
Line numbers are of the working copy after the edits. "The page" is
`docs/design/architecture/foundation/spherical-geometry.md`.

How it was done. I read in full: `PLAN.md`, `overlay-walkthrough.md`, `prC-symdiff-twist.md`,
`prA-distillation.md`, `prB-distillation.md`, `prB-claim-check.md`, the nine `prB-review-*.md` (their
findings, test gaps, "page statements to amend" and unpinned lists; their "checked and found
correct" lists skimmed), the page's sections 1, 3, 5, 6 and 7, `PolygonOverlay.h`, the overlay's
construction, classification, linking, split, join and grouping in `PolygonOverlay.cc`,
`PointInPolygon.{h,cc}`, `PolygonOnSphere::is_point_in_polygon`, and the planning repository's two
point-in-polygon plans (read only). The other 17 plan files (requirements, audit, survey, the four
runs and the variants, the design, the eight Phase 5 reviews, the PR A review) were read in full by
four read-only Opus helpers, each told what PR A and PR B had already placed or dropped and asked
for lasting content found in no destination; I then read every passage they cited and checked each
candidate against the code before using it (about half were used). Statements of behaviour were
checked by reading the code, and the overviews' worked examples, and three statements of the page,
by running the built pyGPlates (scripts in the session scratchpad, not kept).

Verification. Both products rebuilt after the last source edit and both suites run: GPlates
`ctest` 218 of 218 (`PolygonOverlayPrototypeTest.File` and `PolygonOverlayRealDataTest.Coastlines`
skipped by design), pyGPlates 7 of 7 (the stub test among them). Every relative Markdown link and
anchor in the six files with new or changed links resolves (50 relative links, checked by GitHub's
slug rule). Not run: Linux, macOS, Qt5 (comments only), the docs build (no docstring changed).

## 1. What moved where

### `spherical-geometry.md` (2,726 to 2,907 lines; no section renumbered, none added)

| # | What | Where (page line) | Source in the plan | Code evidence |
| --- | --- | --- | --- | --- |
| 1 | The page is the reference; the two overviews; sections are never renumbered because the code cites them by number | intro, 44 | `PLAN.md` *PR C* step 9 ("Not now: splitting section 5 ... repoint the code's 270-odd citations") | `grep -rnE "[Ss]ections? 5(\.[0-9]+)?" src pygplates`: 185 lines (see 3.3) |
| 2 | Links to the overviews | 182 (section 3), 1049 (section 5) | *PR C* step 9 | the two new files |
| 3 | The Boolean methods' names (Python's `set`, Shapely), an empty list for no area, no `& \| - ^` operators (a result is a list; no multi-polygon geometry), and what "itself" means in Python | 5.1, 1094-1102 | `PLAN.md` *PR C* decision 1; Decisions 2026-10-25; `phase5-review-5.md:207-209` | `PyGeometriesOnSphere.cc:3777-3853` (no operator bound; `to_polygon_list`); run: `P.difference(Q)[0] is P` is `False`, `== P` is `True` |
| 4 | Why feeding rounded pieces back is safe (the thickness against Hobby's spurious crossings), and that nothing accumulates along the cascade | 5.1, 1123-1133 | `phase3-audit.md:453-457`; `phase2-requirements.md:219-223`; `phase4a-run2.md:422-438`; `phase4a-run3.md` section 7 (157 steps) | one `Overlay` per call (`PolygonPartitioner.cc:124`, `:204`); thick planes (page 4.2); the `2.4e-11` bound is the page's own (5.2). The 157 steps are the prototype's |
| 5 | The result contract cannot be read from the classes: a vertex-tested piece can still collapse | 5.1, 1138-1142 | `phase4b-design.md:1686-1692`; `PLAN.md` 2026-11-05 (review 5, the `2e-6` tip) | `PolygonOverlay.h:969-971`; clean-up's three-position rule (`PolygonOverlay.cc:5213-5227`); `PolygonPartitioner.cc:140-158` |
| 6 | Common pairs: `h / cos(L / 2)` has no bound near a half circle, and nothing tests the length | 5.2, 1220-1223 | `phase2-requirements.md:333-335` (R8); `prB-review-1` 1.3 | `find_common_pairs`, `PolygonOverlay.cc:1506-1566` (no length test) |
| 7 | The one-passage rule: the order within a half is ring structure only; it deduces less than the walk; valid inputs reach its three failures | 5.2, 1331-1340 | `phase4a-run4.md:232-234`, `:252-254`, `:174-188` | `PolygonOverlay.cc:2292-2310` (the built orders keep the measured relative order) |
| 8 | Step 3: inverting the band pieces' midpoint tests was *not* harmless in dense shared edges (10 of 1,155), and why rounding cannot do it (64 of 69 midpoints `0.05 h` to `0.8 h` off) | 5.2, 1369-1377 | `phase4a-run3.md:424-427`, `:486-506`; `PLAN.md` 2026-10-31 | prototype measurement; corrects the page (3.4) |
| 9 | Anchorless chains: `X` is the subject's midpoint, so its projection can still pass the end of a short clip segment; `R2` is not re-tested and no test was built for it | 5.2, 1397-1405 | `prB-review-6-fixes.md` 1.4; `phase5-review-2.md:118-131`; `prB-review-1` 1.3 | `PolygonOverlay.cc:3708-3710` (the midpoint), `:3741-3749` (the fallback, no further test) |
| 10 | The bundle rule: the cap bounds floating rays only (`(k + m - 1)! / (k - 1)!` orders); the gap between a common ray's two tangents is not covered where both inputs pass more than once | 5.2, 1442-1445, 1457-1462 | `phase5-review-2.md:289-291`; `phase4a-run4.md:470-472`, 2.6 | `PolygonOverlay.cc:3402-3406`, the enumeration `:3486-3550`; `build_rays` (a common ray takes the subject's direction) |
| 11 | Repair: the whole-chain candidate only at a chain's end | 5.2, 1477-1480 | `phase5-review-2.md:149-155` | `PolygonOverlay.cc:3930` (`if (has_non_common_ray)`) |
| 12 | Linking: a bounce's ring can still come back and be split; no operation reaches the crossing rule with four kept ends since the symmetric difference is joined | 5.2, 1505-1506, 1516-1520 | `prB-review-2` 1.3 (the four-kept branch); `prC-symdiff-twist.md` section 4 | `walk_rings`, `PolygonOverlay.cc:4444-4450` (`pair_and_walk_rings` is never given `SYMMETRIC_DIFFERENCE`); the free rule's block `:4128-4168`; run: the hourglass's difference is two polygons |
| 13 | The join's cost (2 to 2.8 times for the rings, 10% to 26% for `extract`; 2% to 5% for partitioning) | 5.2, 1561-1566 | `prC-symdiff-twist.md` section 6, *Timing* | the structure is `walk_rings`'; the figures are that report's |
| 14 | The hemisphere check reads the rings as walked, so a ring at the tie tolerance is read by an arbitrary direction | 5.2, 1727-1730 | `phase4b-design.md:909-911` (said of the rejected check) | `PolygonOverlay.cc:5262`, `:5278-5290` (before `oriented`); `PointInPolygon.cc:2203-2205`; run: the union of two quarter spheres returns one polygon or raises, by order and direction |
| 15 | A vertex holding several subject vertices outputs the first named; the others' coverage values are not carried | 5.4, 1863-1867 | `phase4b-design.md:197-200`; `phase5-review-4.md:150-154` | `PolygonOverlay.cc:1034-1057`, `:5739-5745`; `PolygonOverlay.h:476` |
| 16 | The nearest outline value's rejected alternatives (drop the coverage; NaN) | 5.4, 1916-1918 | `phase4b-design.md:1917-1921` | the rule itself is `PartitionFeatureGeometries.cc`'s step 3 |
| 17 | Most overlapping: the outline-length alternative; the size metric is not a strict weak ordering | 5.4, 1935-1939 | `phase4b-survey.md:189-196`; `phase4b-design.md:1422-1425`; `phase5-review-6.md:57-63` | `PartitionFeatureUtils.cc:901-920`, `.h:442-444` (`real_t` members) |
| 18 | The symmetric difference throws wherever either difference would | 5.6, 2039-2041 | `prC-symdiff-twist.md` section 6 | `PolygonOverlay.cc:4444-4445` (one failure count for both), `:5232-5241` |
| 19 | Greiner-Hormann entry: Kim and Kim's cost and the overlay's; Martínez, Rueda and Feito as the nearest relative and the sector rule's origin; tracing with the overlay as a check | 5.7, 2100-2111 | `phase2-requirements.md:159-161`, `:205-212`, `:598-611`, `:639-641` | none (literature) |
| 20 | Exact arithmetic rejected | 5.7, 2113-2116 | `phase2-requirements.md:214-218`, `:636-637` | none (a decision) |
| 21 | Of the three local rules, the "silent vertex" one was wrong, not narrow | 5.7, 2143-2149 | `phase4a-run4.md:169-171`, `:198-203` | `solve_vertex` returns early only for no rays (`PolygonOverlay.cc:2108`); `deduce_to_fixed_point` visits every vertex (`:2531`) |
| 22 | Symmetric difference: the untried third repair (order parallel rays by ring continuity) | 5.7, 2173-2177 | `prC-symdiff-twist.md` section 4, *Rejected* | none (not built) |
| 23 | The twisted ring: another margin for the far points; a cut at the passed vertex alone | 5.7, 2197-2205 | `prC-symdiff-twist.md` section 3, *Rejected*; mutation C18 | `PolygonOverlay.cc:5028` (the margin is `h`), `:5113-5129` (cut at both vertices, each part closed along the chain) |
| 24 | Why not every plate independently, or one arrangement | 5.7, 2220-2227 | `phase2-requirements.md:623-634` | none (a decision); the cascade is `GeometryCookieCutter::partition` |
| 25 | A side rule for a polyline piece along the outline | 5.7, 2229-2234 | `phase4a-run3.md:398-411` | `PolygonOverlay.cc:2746` (one fixed class) |
| 26 | Asking once whether each input's rings cross | 5.7, 2236-2242 | `PLAN.md` 2026-11-12 ("the cheaper option"); the planning repository's inbox | `PolygonOverlay.cc:5352-5361` (every pair, whatever the inputs) |
| 27 | The prototype comparison accepts a common difference in any case with a band difference | 6, 2671-2673 | `phase5-review-2.md:182-189` | `PolygonOverlayPrototypeTest.cc:1081-1086` |
| 28 | Stale counts corrected: two ring sets not compared; two of the sample's cases cut; 13 walls and 10 | 6, 2677-2679, 2686-2687 | `prC-symdiff-twist.md` section 7 (the addendum); `PLAN.md` 2026-11-13 | `PolygonOverlayPrototypeTest.cc:1531-1533`, `:1588-1598` |
| 29 | The comparison needs a deterministic overlay: no unordered order in a result, no FMA contraction | 6, 2687-2692 | `PLAN.md` 2026-11-13 (PR B's merge, `e69656d17`); `phase5-review-3.md:333-334` | `src/CMakeLists.txt:874-882`; `PolygonOverlay.cc:1523-1525` (the only unordered container, read back in first-seen order) |
| 30 | Mutations: 28 of 35 fail; the three since pinned; of the seven left only the margin changes a valid output | 6, 2773-2784 | `prC-symdiff-twist.md` section 7 and its addendum | the test's own comments, `PolygonOverlayPrototypeTest.cc:1524-1527`, `:1568-1578` |
| 31 | No pyGPlates test raises `PartitionError` through `PlatePartitioner` or `partition_features` | 6, 2784-2786 | `phase5-review-7.md:219-221` | `grep PartitionError pygplates/test`: `test_geometries_on_sphere.py:1859-1861`, `:2014-2025` only |
| 32 | Clipping by an open arc: how the prototype linked, what it did not run (a polygon round a pole; six ends), what a port would cure, keep and change | 7, 2853-2868 | `phase4a-run2.md:475-528`; `phase4b-survey.md:205-208`; `phase4b-design.md:1122-1131`; `PLAN.md` *Phase 4* (the `DateLineWrapper` bullet) | not built; the defects are the page's 5.8; `DateLineWrapper`'s users are `PyDateLineWrapper.cc`, `OgrWriter.cc`, `MapRenderedGeometryLayerPainter.cc`; the pairing is `partner[vertex][ray]` in `pair_and_walk_rings` |
| 33 | A corner's lost subject location: the fallback not built | 7, 2827-2830 | `phase4b-design.md:1212-1214` | `Options::original_subject`, `PolygonOverlay.h:834-849` (the originals are there for the wrap-around) |
| 34 | The chain cut looks only between two vertices of one unkept chain | 7, 2881-2883 | `prC-symdiff-twist.md` section 8, doubt 5 | `PolygonOverlay.cc:4960-4967` (the ring's vertex points grouped by chain), `:5090-5112` |

### Comments at the code (comments only)

| # | Where | What | Source |
| --- | --- | --- | --- |
| 35 | `PolygonOverlay.cc:4139-4144` (`pair_and_walk_rings`) | No operation reaches the crossing rule with four kept ends; with three output sectors the loop would pair a ray twice | `prB-review-2` 1.3; PR C |
| 36 | `PolygonOverlay.cc:3656-3659`, `:3725-3730` (`classify_anchorless_chain`) | The fallback counts the clip's parity at the subject's projection; the two limits of item 9 | `prB-review-6-fixes.md` 1.4; `phase5-review-2.md` 1.5 |
| 37 | `PolygonOverlay.cc:412` | `NEAR` (and `FAR`) are macros of `<windows.h>`: why the enumerators are `SIDE_*` | `prB-review-8-crossing-fixes.md` 1.7 |
| 38 | `PolygonOverlay.cc:141-143` (`BUNDLE_CAP`) | The enumeration's size | `phase5-review-2.md:289-291` |
| 39 | `PolygonOverlay.cc:2137-2140` (`solve_vertex`) | A vertex with one input's ends still constrains; skipping it was tried and is wrong | `phase4a-run4.md:198-203` |
| 40 | `PartitionFeatureUtils.cc:907-910` (`GeometrySizeMetric::operator<`) | Not a strict weak ordering; never sort with it | `phase5-review-6.md:57-63` |
| 41 | `PlatePartitioning.py:412-414` | The partitioned positions index the clone because it iterates in the same order | `phase5-review-8.md:224-231` |

### The overviews and the convention

| # | What | Source |
| --- | --- | --- |
| 42 | `foundation/polygon-overlay-overview.md` (370 lines): the five steps; pieces (two squares); classifying (vertex test, deduction across crossings, the check); common pieces and the sector rule (two shared-edge pictures); the keep table read against the three examples; pairing, the walk and the split (an hourglass: a touch and two crossings); grouping by depth parity, orientation, the hemisphere check; the three layers and the pyGPlates methods; twelve one-sentence pointers into the reference | `overlay-walkthrough.md` sections 1 to 4, 6, 10, 11, 14, rewritten from the code |
| 43 | `foundation/point-in-polygon-overview.md` (282 lines): the smaller side and parity; the outside point; counting crossings and the vertex case; the outline counts as inside; three speeds; what consumers rely on; seven pointers into the reference | the code, the page's section 3, the planning repository's `plans/point-in-polygon.md` and `point-in-polygon-interior-rule.md` |
| 44 | `AGENTS.md` (*Architecture pages*: one paragraph; *Planning documents*: one paragraph); `docs/design/architecture/README.md` (one new section, *How to read these documents*, inserted whole before *Overview*, no existing line changed); `README.md` (one line after the `BUILD-*.md` list) | `PLAN.md` *PR C* step 9 |

How the overviews were checked. Every example's output was run through the built module: the two
squares (intersection 4 points, difference 6, union 8, symmetric difference two polygons); the
neighbours (intersection empty, `Q.partition(P)` outside with `P` whole, union and symmetric
difference one polygon of 8 points); `Q` against `P`'s edge inside (4, 8 and 6 points); the
hourglass (triangle; two polygons of 4 points meeting at the apex; a union of 9 points); `Q` in `P`
(a hole, clockwise, in an anticlockwise exterior); an island in a hole (two polygons); the caps and
the equator ring for "the smaller side" and the tie; three nested rings, a ring poking out and a
bow-tie for parity; the outline threshold between `1.3e-6` and `1.6e-6` radians. The pieces and
classes of each example were derived by reading `build_stops_and_pieces`, `classify_pieces`,
`solve_four_end_vertex`, `is_kept` and `pair_and_walk_rings`, not observed (pyGPlates does not expose
them). The changelogs were read against the code and left as they are (two remarks in section 4).

## 2. What was deliberately dropped, and why

History and progress (the code, the tests and git are the record):
1. `PLAN.md`'s Status entries as such: dates, commit hashes, suite counts, the Linux runs, who ran
   what, the CI failures and their two fixing commits (`are_exactly_equal` is explained at
   `MathsUtils.h:113-126`; the FMA flag at `src/CMakeLists.txt:874-882`; the `NotificationGuard`
   constructor is code).
2. `prC-symdiff-twist.md`: the verdict on the draft, the scratch files' archaeology, the per-case
   lists of changed hunt cases, the full mutation tables (the page keeps the totals and the
   unpinned conditions), the session's file list.
3. The nine `prB-review-*.md`: every finding was fixed and pinned, or is on the page; their "checked
   and found correct" lists; their probes.
4. The decision that `feature/architecture-diagrams` follows pyGPlates 1.1, and that its
   `foundation` page links the overviews: the roadmap's, in the planning repository.

Plans, not design (they stay with the archive and the roadmap):
5. How to port `DateLineWrapper` (a point oracle on the present code first; 1.1 if time allows).
   Item 32 keeps what the port would cure, keep and change.
6. How to drive the prototype, the exporter and the hunt; the nine known hunt differences' case
   names; `six#13 seed 11`'s investigation. The tools are in the planning repository.
7. The `partition_features` rework's breakdown (0.49 s, of which 0.31 s Python assembly): the
   figures disagree with section 6's later 0.29 s and were taken before a fix; section 7 already
   has the item.

Superseded or unmeasured:
8. The 255 grouping failures of 2026-11-08 (before crossing rings were grouped), the "four kept ends
   pair crosswise" hypothesis, `r5o` against `r5p`.
9. `phase4b-design.md` 2.1 (a reusable `Workspace`) and 4.1 (skipping clip rings by their own
   circles): optimisation ideas never measured.
10. `phase3-audit.md:382-384` (point-in-polygon could share the chord interpolation helper): a tidy-up
    with different fallbacks either side.

Too small to earn a sentence (each is true of the code and harmless):
11. `trusted_flipped` can count one piece twice; a bundle vertex with every ray floating is counted
    nowhere; a polyline of internal pieces only comes out outside by default; two runs of one class
    either side of a dropped one-point run stay two polylines; the nearest-outline clamp's
    unreachable wrong case; `piece_side_disagreements` is empty in release builds; the returned
    features' order follows property names; the put-back property loses XML attributes;
    `expect_symmetric` relies on discovery order; an `InformationModelError` can escape the coverage
    path of `partition_features` for a range its feature type disallows.

About 41 items distilled (34 on the page, 7 comments), two overviews, the convention in three
files; about 25 kinds of thing dropped, in the 11 groups above.

## 3. Disagreements found

Between the page and the code (all corrected on the page):
1. **Section 6's mutation paragraph and counts were stale.** It said 25 of 35 mutations fail and
   listed the walk's stop, the pair's direction and the join's far-end condition as unpinned; that
   one ring set per file is not compared; that one of the sample's cases is cut; "13 walls and 8".
   The tests have had three more hunt cases since the fix was committed
   (`PolygonOverlayPrototypeTest.cc:1524-1527`): `num_chain_cut_ring_sets` is 2 (`:1533`), the hunt
   file's cut cases are `{1, 1, 0, 0, 1}` and `{1, 0, 1, 0, 1}` and its walls `{10, 10}`
   (`:1594-1598`), and the report's addendum has 28 of 35 failing. Items 28 and 30.
2. **Section 7 named `extract_rings`** as where the pairing is keyed by ray; since PR C it is
   `pair_and_walk_rings` (`PolygonOverlay.cc:4067`).
3. **Section 6 on the comparison's exemption** said a common pair's class may differ "where a band
   piece's class reaches it through the sector walk"; the test accepts any common difference in a
   case with a band difference, without tracing it (`PolygonOverlayPrototypeTest.cc:1081-1086`).
   Item 27.
4. **Section 5.2 step 3** read as if inverting the band pieces' midpoint tests were harmless in
   dense shared edges too. The record is 10 wrong of 1,155 there (`phase4a-run3.md:424-427`).
   Item 8.
5. **Section 5.1's "the input object"**: true of the C++ (`partition_polygon` returns the same
   `PolygonOnSphere`), not of Python's `is`. Item 3. The docstrings say "this polygon itself is
   returned", which is the same looseness and was left.
6. **Section 5.2's free rule** described a crossing's pairing of four kept ends as live. Nothing
   reaches it now. Items 12 and 35.

Between the plan and the code:
7. **"About 270 code comments cite section 5.x"** (`PLAN.md` 2026-11-08 and *PR C* step 9, and this
   task's brief). I count 185 lines in `src/` and `pygplates/` that cite section 5, and 220 that
   cite any numbered section (about 15 of the 220, in eight files, cite something else). The page says "about 180".
8. **`PLAN.md` 2026-11-13, "7 of 35 mutations survive"** against `prC-symdiff-twist.md` section 7's
   first count of 10: both are right at their dates; the page had the earlier one (1 above).
9. **`phase4b-design.md:1606-1607`**, repairs "in debug builds, logged with the vertex": the overlay
   logs nothing (no `qDebug`, `qWarning` or stream in `PolygonOverlay.cc`); repairs are counted.
10. **`phase4a-run4.md:443-444, 464`**, a bundle "built only when both polygons have four or more
    ends": the code builds one at six or more ends unless one input passes once
    (`PolygonOverlay.cc:1965-1970`), which includes a vertex with one input's ends only. The page
    already says so.
11. **`phase4a-run3.md:152-153`**, "the C++ compares vertex indices, not coordinates": it compares
    coordinates (`is_same_position`, `PolygonOverlay.cc:156-166`).
12. **`overlay-walkthrough.md`** against the code, where the overview follows the code: 11 (exteriors
    and holes "need no test", the output side from the sector memberships: the code groups by
    containment and orients by `PolygonOrientation`); 14 (a hemisphere failure "detected by the
    grouping": it is one point test); 9.3 ("never fail the call": the overlay throws); 6.4 (the
    pinch is paired directly now, so the overview's split example is a ring that returns by way of
    two crossings); 12 (a polyline's hairpin is not made internal); and everything listed in the
    brief as later (repair, the bundle and one-passage rules, crossing groups, the sliver filter,
    the cut at a chain, the join).
13. **`phase2-requirements.md`** and **`phase4b-survey.md`** statements overtaken by later decisions
    (a hole goes to "the smallest exterior containing it"; "never fail the call"; the result-only
    path stops early; contraction): superseded, and the page has the decisions.

Between the changelog and the code (not rewritten, as asked):
14. `CHANGELOG-pyGPlates.md`, "`union()` and `symmetric_difference()` ... raise `PartitionError` if
    the result would enclose more than a hemisphere": true of a result whose *ring* would (the
    docstring says "an outer ring"). Two caps of 80 degrees about opposite poles unite without
    error (two rings, each its smaller side); and a union that is exactly a hemisphere is returned
    or refused by the inputs' order (item 14).

## 4. What I was unsure of

1. **Item 14 is an edge I found by running, not a recorded decision.** Whether an exact hemisphere
   should be accepted, refused, or made deterministic is the developer's call; the page only says
   what happens. It could equally be a section 7 bullet.
2. **Prototype measurements I could not check**: 157 cascade steps (item 4); 10 of 1,155 and 64 of
   69 (item 8); the timings (item 13); 13 of 600 cuts (item 32); "no case seen" (item 10). Each is
   attributed as measured, and none decides a statement about what the code does.
3. **"No committed test was built to reach" the `R2` fallback** (item 9) rests on two reviews and a
   grep of the tests; nothing counts the fallback, so I did not prove it unreached.
4. **`phase4b-design.md:310-313`**, "a chord fraction differs by `theta^2 / 8` of the segment, `2 h`
   on a `0.03` rad segment": my series for the difference between a chord fraction and an angle
   fraction gives at most `0.016 theta^2` of the segment (`0.3 h` at `0.03` radians). Not placed.
5. **The oracle-first plan for the `DateLineWrapper` port** (dropped, 2.5) is the most useful thing
   I left out on the "no plans in design documents" rule. If the roadmap's entry does not have it,
   it should.
6. **`Hobby 1999`, `Kim and Kim`, `Martínez, Rueda and Feito 2009`** are cited from
   `phase2-requirements.md`'s reading of them; I did not open the papers.
7. **The overlay overview's hourglass** replaces the walk-through's pinch as the split's example,
   because the pinch no longer needs a split. Its walk order is one valid order, not necessarily
   the one the code takes; the two output rings are what the run gives.
8. **The point-in-polygon overview** says "about 6%" of shared plate vertices are not bit-identical
   (the page's 94%), and that the interior centroid changed "for polygons more than about a degree
   across" (the switch is a bounding radius of `0.01` radians).
9. **`PlatePartitioning.py`'s new comment** (item 41) states the dependency (a clone iterates in the
   feature's order) without my having read the revision's clone; the committed
   `test_partition_features_removes_partitioned_properties` is what holds it.
10. **`docs/design/architecture/README.md`** now has a section *How to read these documents* directly
    above the existing *Overview* (the area diagram). The two uses of "overview" sit close together;
    the other branch's rewrite may want to rename one.

## 5. Checklist over the plan files

"Clean" means I found nothing lasting that is not now in the page, at the code or in the
changelogs. "Helper" means an Opus helper read the file in full and I verified its candidates.

| File | Read by | Undistilled after this pass |
| --- | --- | --- |
| `PLAN.md` | me | Clean. What stays in the archive only: the status history, the roadmap decisions (2.4, 2.5), the tools' pointers (2.6) |
| `overlay-walkthrough.md` | me | Clean: rewritten as the overlay overview; its sections 5, 7 to 9, 12 and 13 are the page's 5.2 to 5.4 |
| `phase2-requirements.md` | helper, then me | Clean after items 6, 19, 20, 24. Not placed: the cost arithmetic at `N = 10^4` |
| `phase3-audit.md` | helper, then me | Clean after item 4. Not placed: 2.10 |
| `phase4a-run1.md` | helper | Clean |
| `phase4a-run2.md` | helper, then me | Clean after items 4 and 32 |
| `phase4a-run3.md` | helper, then me | Clean after items 8 and 25 |
| `phase4a-run4.md` | helper, then me | Clean after items 7, 10, 21 |
| `phase4a-variants.md` | helper | Clean |
| `phase4b-design.md` | helper, then me | Clean after items 5, 14 to 17, 32, 33. Not placed: 2.9, 4.4 above |
| `phase4b-survey.md` | helper, then me | Clean after items 17 and 32 |
| `phase5-review-1.md` | helper | Clean (its margin-order gap was closed on 2026-11-12; two small items in 2.11) |
| `phase5-review-2.md` | helper, then me | Clean after items 9 to 11, 27, 38 |
| `phase5-review-3.md` | helper | Clean after item 29 |
| `phase5-review-4.md` | helper, then me | Clean after item 15 |
| `phase5-review-5.md` | helper, then me | Clean after item 3 |
| `phase5-review-6.md` | helper, then me | Clean after items 17 and 40 |
| `phase5-review-7.md` | helper, then me | Clean after item 31 |
| `phase5-review-8.md` | helper, then me | Clean after item 41 |
| `prA-distillation.md` | me | Clean (a report) |
| `prA-review.md` | helper | Clean (one small item in 2.11) |
| `prB-claim-check.md` | me | Clean: its 20 findings are applied (spot-checked 5, 7, 10, 16, 18) |
| `prB-distillation.md` | me | Clean: its "stays until PR C" list (section 4) is the page's 5.1, 5.6, 5.7 and 7 now, and 2.6 above |
| `prB-review-1-overlay-classification.md` | me | Clean after items 6, 9, 36 |
| `prB-review-2-overlay-extraction.md` | me | Clean after items 12 and 35 |
| `prB-review-3-partition-layer.md` | me | Clean |
| `prB-review-4-feature-level.md` | me | Clean |
| `prB-review-5-pygplates.md` | me | Clean |
| `prB-review-6-fixes.md` | me | Clean after items 9 and 36 |
| `prB-review-7-crossing-groups.md` | me | Clean (its grouping argument is the page's, compressed; its section 2 is the page's section 7) |
| `prB-review-8-crossing-fixes.md` | me | Clean after item 37 |
| `prB-review-9-final.md` | me | Clean: its unpinned list is the page's section 6 |
| `prC-symdiff-twist.md` | me | Clean after items 13, 18, 22, 23, 28, 30, 34 |
