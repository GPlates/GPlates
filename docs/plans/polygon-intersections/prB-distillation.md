# PR B distillation report

Branch `feature/polygon-intersections`, worktree `C:/gplates/src/polygon-intersections`, 2026-11-08,
at `20eaea076` (84 commits past `public/main`). Edits are uncommitted; nothing under `docs/plans/`
was touched but this file. Line numbers are of the working copy after the edits. Every statement
written into the page or a comment was checked against the code as it is now; where the plan and the
code disagree, the code was followed and the disagreement is in section 5.

## 1. Changes made

### `docs/design/architecture/foundation/spherical-geometry.md`

The page grew from 1,535 to 2,259 lines. Section 5 is new (5.1 to 5.7, about 730 lines); the old
5.2 (`DateLineWrapper`) is now 5.8, unchanged but for its number.

1. **Introduction** (lines 21-28). `PolylineIntersections` replaced by the overlay and the
   partitioner; the unexposed Boolean operations named. Evidence: `src/maths/CMakeLists.txt` no
   longer lists `PolylineIntersections.*` (the diff deletes both files); `PolygonOverlay.h:113-120`
   (`Operation`); nothing in `src/api` calls `extract(UNION)` or the differences.
2. **Section 1, nine new table rows** (before the `DateLineWrapper` rows). Source: design 3.5, 3.6,
   3.7(c), 3.8 (amended), 3.9, 4.2, 4.3, 4.5; PLAN.md 2026-11-04 (`PointOnSphere::operator==`,
   point-in-polygon near zero-length edges, margins tying). Evidence per row: identity and exact
   positions, `PolygonOverlay.cc:150-160` (`is_same_position`), `:461-581`; test points threshold
   off, `:1478-1500`; margin order, `:2331-2379`; self-touch cut, `:1090-1092, 1182-1186`; cone,
   `:1255-1302`; anchorless `min_sine`, `:3062-3070`; hemisphere and grouping margins, `:185`,
   `:3897-3898, 3969-3984`; sliver widths, `GeometryCookieCutter.cc:327-331`,
   `PyPlatePartitioner.cc:64-85`, `PyGeometriesOnSphere.cc:3650-3708`, `PolygonPartitioner.h:85`;
   `get_mean_width` apex, `PolygonOverlay.cc:261-273`.
3. **Section 3.3** (the parity area note) now points at the self-overlay instead of "the
   partitioning PR". Evidence: the overlay intersects two polygons only (`PolygonOverlay.cc:313`,
   `:331`; `GeometryIntersect::intersect` has no self-intersection overload, section 7 of the page).
4. **Section 3.5** (where the threshold is off) names `Overlay::is_inside_other` and the overlay's
   other exact tests instead of the deleted
   `PolygonPartitioner::is_non_intersecting_polyline_or_polygon_fully_inside_partitioning_polygon`.
   Evidence: `PolygonOverlay.cc:1478-1500` (`false` for the threshold), `:3071` (parity entry,
   exact), `:217-218, 235` and `:3931, 3938-3939` (threshold off in the grouping and the check).
5. **Section 4.1** (users of `GeometryIntersect`): the overlay and CitcomS. Evidence:
   `PolygonOverlay.cc:313`; `GenerateVelocityDomainCitcoms.cc:272-278`.
6. **Section 5.1 The layers**: the three layers, the cascade, the rigid blocks as holes, the result
   contract and the rejected records-only rule. Source: design 1, 4.1 (as built), 5.3, 8.1, 8.2,
   8.5, 10.2 (as built after review 6), 13; survey Q4; PLAN.md 2026-10-31 (Q4), 2026-11-05 (review
   5), 2026-11-06 (review 6). Evidence: `GeometryCookieCutter.h:104-164, 209-217`,
   `GeometryCookieCutter.cc:596-660` (blocks then boundary with `include_rigid_blocks_as_interior_holes`),
   `:327-331`; `PolygonPartitioner.cc:79-96` (`is_outside_by_bounds`), `:108-160` (`overlay_polygon`,
   the result rule, the both-empty rule at `:144-149`), `:189-231` (`overlay_polyline`, collapsed
   runs), `:344-349` (a multi-point whole), `:375-378` (a point on the boundary inside),
   `:736-745`; `PolygonPartitioner.h:202-236` (same result with or without lists);
   `GLReconstructedStaticPolygonMeshes.cc:962-976`; `GenerateVelocityDomainCitcoms.cc:272-278`;
   `PolygonOverlay.cc:302-337` (one `Overlay` per call, references to the inputs).
7. **Section 5.2 The overlay**, stage by stage. Source: design 2, 3 (every subsection, as
   amended), 4.2, 4.3 (as built), 5.1, 6.1 (as built); `overlay-walkthrough.md` 3, 4, 5, 6, 7.3 to
   7.8, 9, 10, 11, 12, 14; `phase4a-run3.md` 3; `phase4a-run4.md` 3; `phase4a-variants.md` 10;
   reviews 3 (self-touch cuts, the all-common vertex), 4 (provenance after the split), 8 (the cone
   by stop type, the great-circle ring). Evidence: records and vertices `PolygonOverlay.cc:311-313,
   461-581`; stops and pieces `:585-856` (one stop per ring vertex asserted at `:684-686`; the
   ratio by projection `:79-113`); loop and spike rules `:909-1008` (the tip condition
   `:971-988`); common pairs `:1012-1072`; self-touch cuts `:1076-1208`; rays `:1212-1314`
   (`measure_ray`: direction `:1251-1253`, offsets by stop type `:1272-1283`, cone `:1291-1302`,
   no direction `:1306-1313`), `:1318-1388` (the order), `:1392-1474` (common ray's direction
   `:1440-1445`, wider cone `:1448-1454`, bundle at six or more ends where neither passes once
   `:1469-1472`); classification `:2263-2436` (vertex tests `:2275-2322` with the debug count
   `:2312-2321`; margin order `:2331-2379`; anchorless `:2389-2424`; clip partners `:2427-2435`);
   the free rule and the LL rule `:1505-1596`; the one-passage rule `:1676-1808`; the sector walk
   `:1810-2017` (the four starts `:1839-1872`; deduce only if every solution agrees `:1943-1983`;
   the tie-break `:1883-1926`); the bundle rule `:2682-2929` (certainty order `:2703-2714`, never
   anchors `:2733-2735`, cap `:2758-2761`, no frame `:2763-2767`, admissibility `:2803-2839`,
   enumeration `:2841-2903`, measured first `:2905-2927`); `one_passage_side` `:2933-2958`;
   anchorless chains `:2962-3097` (score `:3011-3033`, second reference point `:3062-3070`, flip
   `:3089-3092`); `PointInPolygon.h:109-130`, `PointInPolygon.cc:2289-2345`; checks `:2440-2556`
   (counters, the message `:2521-2553`); repair `:3137-3314` (rank `:3149-3167`, whole chain
   `:3244-3259`, order `:3261-3275`); keep-rule `:3318-3342`; pairing `:3384-3621` (free rule
   `:3448-3482`, two kept `:3484-3490`, ring continuity `:3492-3532`, all common `:3551-3575`,
   sector walk `:3540-3620`); the walk `:3648-3715`; closed pieces `:3718-3732`; split
   `:3739-3771`; clean-up `:3778-3850`; pairing failure throws `:3852-3861`; hemisphere check
   `:3886-3960`; grouping `:3963-4016` and `is_ring_inside_ring` `:192-236`; orientation
   `:4026-4038`; filter order `:4050-4088`; polylines `:2091-2259`, `:4124-4198`.
8. **Section 5.3 Tolerances in the overlay**: exact, rounded and combinatorial decisions; why
   identity. Source: design 3 (the Decisions column), 3.6, 3.8, 13; walk-through 7.2, 7.4, 7.7;
   `phase4a-run3.md` 3.3; `phase4a-run4.md` 3.4; PLAN.md 2026-11-04 (step 1-3 findings).
   Evidence: as item 7, and `PointInPolygon.cc:355-375` (a zero-length edge's crossing stands in by
   its end point).
9. **Section 5.4 Provenance and scalar coverages**: provenance per point and its composition
   through the cascade, coverages from provenance, the most overlapping plate, time-dependent
   geometry. Source: design 2.8, 7.1 to 7.3 (as built), 8.3, 8.4 (as built), 14 (question 2);
   reviews 4, 6, 7, 8 (1.4); PLAN.md 2026-11-05, 2026-11-06, 2026-11-07 (G8-1). Evidence:
   `PolygonOverlay.h:644-682, 750-776`; `PolygonOverlay.cc:4215-4295` (`get_vertex_provenance`),
   `:4299-4316`, `:4320-4361` (a polyline stop's own record), `:4365-4546`
   (`compose_subject_location`: wrap-around from the original `:4441-4447`, subject then earlier
   clip `:4493-4534`, mismatches `:4536-4545`), `:3780-3806` (after the split);
   `PolygonPartitioner.h:108-173`; `PolygonPartitioner.cc:480-535, 827-834`;
   `GeometryCookieCutter.h:243-257, 260-276`; `GeometryCookieCutter.cc:396-495`;
   `PartitionFeatureGeometries.h:47-51, 182-185`; `PartitionFeatureGeometries.cc:150-178` (the
   single time window), `:238-260` (`Range`), `:327-399` (nearest outline by projection),
   `:503-512` (the range's clone constant), `:627-720` (`partition_range`), `:750-845`
   (`interpolate_ring_values`, the closed-ring guard `:830-845`); `PartitionFeatureUtils.cc:84-121,
   899-912`, `PartitionFeatureUtils.h:405-444`; `PlatePartitioning.py:46-61` (one feature per
   coverage), `:158-168, 341-382, 416-438`; `PyPlatePartitioner.cc:447-470, 942-943`.
10. **Section 5.5 The sliver filter**: the rule, the measure, the widths and why. Source: design
    4.5 (as built and decided); PLAN.md 2026-11-07 (step 14a measurements, 14b), 2026-11-08 (step
    15, review 8 1.1 and 1.3). Evidence: `PolygonOverlay.h:778-789`; `PolygonOverlay.cc:240-299`,
    `:4043-4088`; `GeometryCookieCutter.h:148-150, 209-217`, `GeometryCookieCutter.cc:327-331`;
    `PolygonPartitioner.h:74-78, 85`; `PyPlatePartitioner.cc:64-85, 702-710, 740-746`;
    `PyGeometriesOnSphere.cc:3650-3708, 4515-4525`. The histogram and the three widths' reasons are
    measurements (PLAN.md 2026-11-07, design 4.5), stated as such; the "no near-zero edges" finding
    is PLAN.md 2026-11-07, with its code consequence (no filter of them) verified by its absence.
11. **Section 5.6 Failure**: the exception, each product's handling, the residue case. Source:
    design 9.1 (as measured), 9.2 (as built); PLAN.md 2026-11-06 (review 6), 2026-11-08. Evidence:
    `PolygonOverlay.h:585-641, 734-738`; `PolygonOverlay.cc:2521-2553, 3852-3861, 3940-3955`;
    `PolygonPartitioner.h:214-215`; `PyExceptions.cc:482-494`; `AssignPlateIds.cc:293-307`,
    `AssignPlateIds.h:309-329`; `AssignReconstructionPlateIdsDialog.cc:304-318, 540-568`;
    `CliAssignPlateIdsCommand.cc:392-398`; `GLReconstructedStaticPolygonMeshes.cc:962-976`;
    `src/unit-test/data/polygon-overlay/inconsistent.txt` (`shared#2288 seed 2`) and
    `data/README.md`.
12. **Section 5.7 Rejected alternatives**. Source: design 13 (every row), 4.3 (the rejected
    per-ring output side), 8.4 (question 5); requirements 3.2, 3.5, 3.6; variants 2 to 4, 10, 11;
    walk-through 5, 7.4, 7.7; runs 1 to 4. Evidence for what the code does instead: as items 6 to
    9; one feature per coverage piece, `PlatePartitioning.py:46-61` and
    `PartitionFeatureUtils.cc:764+` (`get_feature_for_partition`). The measurements quoted (33 of
    300, 19 catalogue cases, 19 of 300 / 3 of 60 / 18 of 299, `20 h`, 10 catalogue cases, 218 to 1,
    9 of 300, 19 of 2,393, 77 against 48, 15 of 25) are the prototype's, from the run reports, and
    cannot be checked by reading; they are kept because each justifies a rule.
13. **Section 6**: the overlay tests, the oracle, the prototype comparison and its snapshots, the
    partition-layer and coverage tests, the Python tests, the real-data harness and its
    measurements; the hunt's observation corrected ("saw nothing on interior rings"). Source: design
    11 (as built), 12 step 15; PLAN.md 2026-11-07, 2026-11-08. Evidence: the `TEST` names in
    `PolygonOverlayTest.cc`, `PolygonPartitionerTest.cc`, `PartitionFeatureUtilsTest.cc`,
    `PolygonOverlayPrototypeTest.cc`, `PolygonOverlayRealDataTest.cc` (listed by `grep ^TEST`);
    `PolygonOverlayTest.cc:46-52, 111-115`; `PolygonOverlayOracle.h:47-53, 88-93, 411-440,
    758-800, 846-880`; `PolygonOverlayPrototypeTest.cc:57-84, 836, 1467-1531, 1585-1660`;
    `PolygonOverlayRealDataTest.cc:64-78`; `data/README.md`; the new `def test_` names in the
    pygplates test diff. The hunt's and the real-data numbers are PLAN.md's measurements.
14. **Section 7**: the "Polygon partitioning" bullet replaced by the Boolean operations in
    pyGPlates (PR C) with the two known unfinished items; new bullets for the residue and the open
    hunt differences, clipping by an open arc, and time-dependent geometry; the self-intersection
    and parity-area bullets repointed at the self-overlay. Source: design 5.2, 6.3, 9.1; PLAN.md
    "Resume here", 2026-11-04 (step 8 open item), 2026-11-07 (inbox item), 2026-11-08; requirements
    3.7. Evidence: `PolygonOverlay.cc:3386` (`partner` keyed by ray, so the `Node {ray, side}` of
    design 6.3 is not built); `PolygonOverlay.h:237, 278` (`RingView::closed`,
    `Stop::DANGLING_END`); `PolygonOverlay.cc:4023-4024` (a self-crossing ring's orientation).

### `src/maths/PolygonOverlay.cc`

15. **`build_stops_and_pieces` comment** (`:622-632`): two records of different vertices tied along
    a segment come in either order; the piece between them is a zero-length single arc whose class
    is deduced, and either order is valid. Source: walk-through 7.8; design 3.1; PLAN.md 2026-10-30
    (step 5, near-ties in discovery order) and 2026-11-04 (the tie orders the prototype did not
    have). Evidence: `GeometryIntersect.cc` sorts by `is_precisely_less_than` on a cosine with
    `std::stable_sort` (the page's section 4.4); two consecutive record stops in one segment with
    no `ORIGINAL_VERTEX` between give `single_arc_segment` (`PolygonOverlay.cc:783-788`) and no
    interior vertex, so the piece is deduced or midpoint-tested (`:2331-2379`); the split cuts by
    vertex identity (`:3745-3759`), so two vertices at one position are not cut.
16. **`is_inside_other` comment** (`:1486-1491`): point-in-polygon is not exact within about a
    zero-length edge's length of it, so only a band piece's midpoint test can be affected. Source:
    PLAN.md 2026-11-04 ("Point-in-polygon is not exact near a zero-length edge", measured `0.019h`).
    Evidence: `PointInPolygon.cc:355-375` (the end point stands in for the crossing); a vertex in no
    record is more than `h` from the outline (`GeometryIntersect.cc`'s touch test, section 4.2 of
    the page).

### `src/app-logic/GeometryCookieCutter.h`

17. **Constructor comment** (`:110-113`): since the overlay intersects interior rings, a network's
    partitioning polygon never claims a rigid block's area in either order; the order still decides
    which plate ID a block's own polygon assigns. Source: design 8.2 ("Networks' rigid blocks").
    Evidence: `GeometryCookieCutter.cc:648-655` (the boundary polygon with
    `include_rigid_blocks_as_interior_holes = true`), `PolygonOverlay.cc:341-375` (every interior
    ring is a `RingView`), `PolygonPartitioner.cc:740-744` (`partition_point` counts holes through
    `is_point_in_polygon`).

### `src/opengl/GLReconstructedStaticPolygonMeshes.cc`

18. **Comment at the surrounding-node test** (`:985-989`): redundant since the overlay (a clip ring
    inside the subject makes the partitioner report intersecting; the shortcut answers outside only
    for disjoint circles containing the interiors), kept until a visual check retires it. Source:
    design 8.5, 14 (question 4, the developer: keep it in PR B). Evidence:
    `PolygonOverlay.cc:2596-2599` (a clip piece inside the subject keeps both outputs),
    `PolygonPartitioner.cc:79-96, 116-122`.

### `src/unit-test/PolygonOverlayPrototypeTest.cc`

19. **Header comment** (`:61-66`): the snapshots' configuration is `r5o` (cones counting a ray's
    offsets off its segment), not `r5c`. Source: PLAN.md 2026-11-08 (the snapshots re-exported
    under `r5o`; "the prototype configuration is now `r5o`"). Evidence: the `BundleRule` test's own
    comment and expectations (`:1631-1660`, "`r5o`"); `PolygonOverlay.cc:1255-1283` (the offsets).

### Changelogs

20. `CHANGELOG-GPlates.md:47`: added that a polyline along a boundary shared by two plates goes to
    the first of them. Evidence: `PolygonOverlay.cc:2101-2110` (`METHOD_ALONG` inside by default),
    `PolygonOverlay.h:740-745`; the cascade gives the piece to the first plate
    (`GeometryCookieCutter.cc:454-500`). The other seven entries were checked against the code
    (items 6, 9, 10, 11) and are accurate.
21. `CHANGELOG-pyGPlates.md:43`: added that each piece with a coverage is returned in a feature of
    its own, as Assign Plate IDs does. Evidence: `PlatePartitioning.py:46-61`;
    `PartitionFeatureUtils.cc:764+`. The entries at `:38-46` and `:75` were checked against
    `PyGeometriesOnSphere.cc:4473-4553`, `PyPlatePartitioner.cc:702-746, 844-887`,
    `PyExceptions.cc:482-494`, `PlatePartitioning.py:158-168, 363-367, 416-438` and are accurate.

### `docs/design/architecture/reconstruction/README.md`

Unchanged. Its one mention (`:205-207`: `ReconstructUtils::reconstruct` is the door used by
`GeometryCookieCutter`) is still true (`GeometryCookieCutter.cc:192`).

## 2. Mapping of the plan references in `src/` and `pygplates/` to the page

Every `phase4b-design.md`, `phase4a-*`, `phase5-review-*`, `PLAN.md` and walk-through citation in
a code or test comment (`git grep -n "phase4b-design\|phase4a\|PLAN.md\|phase5-review\|review [0-9]\|walk-through"
-- src pygplates`), and where the content now is. "Page" is `spherical-geometry.md`.

| Plan reference | Cited from | Now |
| --- | --- | --- |
| design 2.8 (provenance structures, ratios by projection) | `PolygonOverlay.h:645, 664, 686`; `PartitionFeatureGeometries.cc:377` | page 5.4 (first paragraph) |
| design 3.5 (self-touch cuts) | `PolygonOverlayTest.cc:1278` | page 5.2 "Self-touch cuts" |
| design 3.6 (rays, the order, the cone quantities) | `PolygonOverlay.h:344`; `PolygonOverlay.cc:1327` | page 5.2 "Rays and their order"; §1 "Bundle cone" |
| design 3.7, 3.7.1 cases 1, 1b, 2 | `PolygonOverlay.h:472, 885, 1094`; `PolygonOverlay.cc:1510, 1635, 1677`; `PolygonOverlayTest.cc:1020` | page 5.2 "Classification" steps 1 to 5 |
| design 3.8 (the bundle rule; amended: the cone's offsets) | `PolygonOverlay.h:365`; `PolygonOverlay.cc:1255, 1885, 2696`; `PolygonOverlayTest.cc:756, 808, 866` | page 5.2 "The bundle rule" and "Rays and their order"; §1 "Bundle cone" |
| design 3.9 (anchorless chains, checks, repair) | `PolygonOverlay.h:731`; `PolygonOverlay.cc:2382, 3140`; `PolygonOverlayTest.cc:1297, 1346` | page 5.2 step 4 and "Checks and repair"; §1 "Anchorless reference point" |
| design 3.10 (keep-rule, pairing table) | `PolygonOverlay.h:935, 955`; `PolygonOverlay.cc:3323, 3384`; `PolygonOverlayTest.cc:1478` | page 5.2 "Linking" |
| design 3.11 (split, clean-up) | `PolygonOverlay.cc:3735` | page 5.2 "Split and clean-up" |
| design 4.1 as built (bounding circles) | `PolygonPartitioner.cc:74`; `PolygonPartitionerTest.cc:494`; `PolygonOverlayRealDataTest.cc:121` | page 5.1, the `PolygonPartitioner` bullet |
| design 4.2 (grouping) | `PolygonOverlay.h:172, 714`; `PolygonOverlayTest.cc:1658, 1850`; `PolygonOverlayOracle.h:619` | page 5.2 "Grouping, orientation and the hemisphere check"; §1 "Hemisphere check and grouping margins" |
| design 4.3 (orientation; the hemisphere check; the rejected output side) | `PolygonOverlay.cc:3886`; `PolygonOverlayTest.cc:1658` | page 5.2 (same), 5.7 |
| design 4.5 (the sliver filter) | `PolygonOverlay.h:157, 779`; `PolygonPartitioner.h:75`; `GeometryCookieCutter.h:209`; `PolygonPartitionerTest.cc:1033, 1155, 1190` | page 5.5; §1 "Sliver widths", "`get_mean_width` apex" |
| design 5.1 (`f` per operation) | `PolygonOverlay.h:110`; `PolygonOverlay.cc:2563` | page 5.2 "Linking" (the table) |
| design 5.2 (the hemisphere failure) | `PolygonOverlay.cc:3886`; `PolygonOverlayTest.cc:1925` | page 5.2 "The hemisphere check", 5.6 |
| design 5.3 (the result contract) | `PolygonPartitioner.h:55, 210`; `PolygonPartitioner.cc:101`; `PolygonOverlay.h:895`; `PolygonPartitionerTest.cc:397`; `PolygonOverlayPrototypeTest.cc:1270, 1585` | page 5.1 "The result contract" |
| design 6.1 (polylines) | `PolygonOverlay.h:700, 971, 1098`; `PolygonOverlay.cc:2094, 2265`; `PolygonPartitioner.cc:183`; `PolygonOverlayTest.cc:2158`; `PolygonOverlayPrototypeTest.cc:555, 1486`; `PolygonOverlayOracle.h:411` | page 5.2 "Polylines"; page 6 (the oracle along a polyline) |
| design 7, 7.1, 7.2 (provenance, composition) | `PolygonOverlay.h:645, 664, 747`; `PolygonOverlay.cc:4222`; `PolygonPartitioner.h:108, 152`; `GeometryCookieCutter.h:244`; `GeometryCookieCutter.cc:417`; `PartitionFeatureGeometries.cc:562`; `PolygonOverlayTest.cc:2333`; `PolygonOverlayPrototypeTest.cc:524, 1111, 1174`; `PolygonOverlayOracle.h:552`; `PolygonPartitionerTest.cc:765` | page 5.4 "Provenance", "Composition through the cascade" |
| design 7.3 (coverages) | `PyPlatePartitioner.cc:447`; `PartitionFeatureGeometries.h:49`; `PartitionFeatureGeometries.cc:204, 628` | page 5.4 "Scalar coverages" |
| design 8, 8.1, 8.2 (the partition layer, the cascade) | `PolygonPartitioner.cc:100`; `PolygonPartitioner.h:114`; `PolygonPartitionerTest.cc:62, 765` | page 5.1 |
| design 8.3 (most overlapping by area; `PartitionFeatureUtils`) | `PartitionFeatureUtils.cc:86, 903`; `PartitionFeatureGeometries.h:49`; `PartitionFeatureUtilsTest.cc:71` | page 5.4 "Most overlapping plate", "Scalar coverages" (the file split) |
| design 9.2 (never a silent wrong answer) | `PolygonOverlay.h:587`; `PolygonOverlay.cc:2521`; `PolygonPartitioner.h:215`; `GeometryCookieCutter.h:249`; `AssignPlateIds.h:311`; `AssignPlateIds.cc:302`; `GLReconstructedStaticPolygonMeshes.cc:967` | page 5.6 |
| design 10.2 (the result-only path, as amended after review 6) | `PolygonPartitioner.cc:105`; `PolygonPartitionerTest.cc:467`; `PolygonOverlayPrototypeTest.cc:1270` | page 5.1 "The result contract" |
| design 10.3, 11.4 (real data) | `PolygonOverlayRealDataTest.cc:64` | page 6 "Real data"; 5.5 |
| design 11.2 (the prototype comparison) | `PolygonOverlayPrototypeTest.cc:57` | page 6 "The prototype comparison" |
| design 11.3 (the oracle) | `PolygonOverlayOracle.h:47, 411, 758`; `PolygonOverlayPrototypeTest.cc:1085` | page 6 (the oracle) |
| design 12, step 15 (no repair on generated data) | `PolygonOverlayPrototypeTest.cc:836` | page 6 ("Any repair of generated data fails the case"), 5.2 "Checks and repair" |
| design 14, question 2 (the nearest outline value) | `PartitionFeatureGeometries.cc:329` | page 5.4 "Scalar coverages", step 3 |
| `phase4a-run4.md` 2.4, 3.3 (the spike rule's tip condition) | `PolygonOverlay.cc:974` | page 5.2 "The loop rule and the spike rule" |
| `phase4a-run4.md` 3.1 (the one-passage rule); a cone of `pi` never anchors | `PolygonOverlay.cc:1677, 2734`; `PolygonOverlayPrototypeTest.cc:1525` | page 5.2 "Classification" step 2, "The bundle rule" |
| `phase4a-run3.md` 4.1 (run 2's residue cleared by the bundle rule) | `PolygonOverlayPrototypeTest.cc:1474` | page 6 "The prototype comparison" ("the second run's residue") |
| `phase5-review-3.md` 1.1 (the all-common vertex) | `PolygonOverlay.cc:3555`; `PolygonOverlayTest.cc:2020, 2055` | page 5.2 "Linking" |
| `phase5-review-3.md` 1.2 (self-touch cuts) | `PolygonOverlay.cc:1083`; `PolygonOverlayTest.cc:2020, 2102` | page 5.2 "Self-touch cuts" |
| review 3 1.4 (a dangling end has no ray) | `PolygonOverlay.cc:3350` | page 5.2 "Polylines" (no linking) |
| review 4 1.1 (provenance after the split) | `PolygonOverlay.cc:3780`; `PolygonOverlayTest.cc:2552`; `PolygonOverlayOracle.h:90` | page 5.4 "Provenance" |
| review 4 1.2 (a polyline stop's clip location) | `PolygonOverlay.cc:4331` | page 5.4 "Provenance" |
| review 4 1.3 (the corner's composition mismatch) | `PolygonOverlay.cc:4539`; `PolygonOverlayPrototypeTest.cc:1253` | page 5.4 "Composition through the cascade" |
| review 4 (provenance tolerances `1.0h`, `1.63h`) | `PolygonOverlayPrototypeTest.cc:104` | not on the page (a test tolerance); keep the comment, drop the reference |
| review 5 1.1 (one contract with or without lists) | `PartitionFeatureUtilsTest.cc:867`; `PolygonOverlayPrototypeTest.cc:1271, 1588` | page 5.1 "The result contract" |
| review 6 1.1 (a run across a ring's start) | `PartitionFeatureGeometries.cc:830`; `PartitionFeatureUtilsTest.cc:331, 656` | page 5.4 "Scalar coverages" (last sentences) |
| review 7 1.1, 1.2, mutations M1, M2, M4 | `pygplates/test/test_app_logic/test.py:2249-2299` | page 5.7 (one feature per coverage piece), 5.4 (time-dependent geometry) |
| Phase 5 review 2 1.1 (the bundle counts pinned) | `PolygonOverlayPrototypeTest.cc:1506, 1631` | page 6 "The prototype comparison" |
| PLAN.md 2026-11-04 (two vertices within a crossing's rounding) | `PolygonOverlayRealDataTest.cc:425` | page 6 (the hunt's structural difference), 5.5 (no near-zero edges on real data) |
| PLAN.md step 14 (near-zero output edges) | `PolygonOverlayTest.cc:2318` | page 5.5 (last sentence) |
| walk-through 3 (the T-intersection) | `PolygonOverlayTest.cc:1191` | page 5.2 "Classification" step 1 |
| walk-through 6.4 (the pinch, the split pairing, Q11) | `PolygonOverlay.cc:3477, 3560, 3735`; `PolygonOverlayTest.cc:1090` | page 5.2 "Linking", "Split and clean-up" |
| walk-through 7.5 (the margin principle) | `PolygonOverlay.cc:2328` | page 5.2 "Classification" step 3 |
| walk-through 9.3 (repair) | `PolygonOverlay.cc:3140` | page 5.2 "Checks and repair" |
| `phase3-audit.md` | `GeometryIntersectTest.cc:44` | PR A's (`prA-distillation.md`); page 6, the `GeometryIntersect` tests |

## 3. Plan content now fully distilled

From `phase4b-design.md`:
- 1 (scope, what it fixes and does not): page 5.1, 5.2, 7, and the changelogs.
- 2 (data structures, as sketched): superseded by the code; the page describes the code's names
  (`RingView`, `Stop`, `Piece`, `Vertex`, `Ray`, `Bundle`, `Location`, `Provenance`, `Options`).
  2.1 (one call, nothing cached): 5.1. 2.2 (subject first): 5.2. 2.3 (what a record names): 4.2
  and 5.2. 2.4 to 2.7: 5.2. 2.8 (ratios, provenance): 5.4. 2.9 (the public class): the header.
- 3 (every step), 3.1 to 3.12 with their amendments: page 5.2 and 5.3; the measurements that
  justify each rule are kept there (3.6's `1.8e-15`; 3.7(c)'s 1/24/250 and `0.5 h`; 3.8's cases and
  the 30 of 824 rays; 3.9's "no repair"), the rest judged not lasting.
- 4.1 (rings with no records, the bugs table): 5.1, 5.2 (closed pieces), 6 (the tests).
- 4.2, 4.3 as built, 4.4: 5.2; the rejected per-ring output side: 5.7.
- 4.5 (the filter, as built and decided): 5.5, §1.
- 5.1 to 5.3: 5.2 (the table), 5.2 (the hemisphere check), 5.1.
- 6.1 as built: 5.2 "Polylines". 6.2 (CitcomS): 4.1, 5.1. 6.3 (the three hooks): page 7, with the
  one not built noted.
- 7.1 to 7.3 as built: 5.4.
- 8.1 to 8.5 as built: 5.1, 5.4, 5.6; the `DIFFERENCE` macro trap is the header's comment
  (`PolygonOverlay.h:46-52`). 8.6: the changelogs (checked, two clauses added).
- 9.1 to 9.3: 5.2 "Checks and repair", 5.6; the debug count: 5.2 step 1.
- 10.1 (costs): not placed, see section 5. 10.2 (as built after review 6): 5.1. 10.3: 6.
- 11.1 to 11.5: 6.
- 12 (the implementation order): bookkeeping, not lasting.
- 13 (rejected alternatives): 5.7 (every row; "dropping one of a corner's two records" was already
  4.5).
- 14 (the five questions and their answers): the decisions are in 5.2 (orientation), 5.4 (the
  nearest outline value), 5.6 (`PartitionError`, Assign Plate IDs skips and reports), the comment
  at `GLReconstructedStaticPolygonMeshes.cc` (the containment test kept), 5.7 (no public coverage
  form).
- 15 (the session review): bookkeeping.

From `PLAN.md`:
- Context, "What the survey found" as it bears on PR B (the four bugs, `PartitionFeatureUtils`):
  5.1, 5.2, 5.4, 6.
- Decisions 2026-10-23 and 2026-10-25 that bear on PR B (the Boolean operations supported; the
  simple-polygon split as a later PR; the inbox notes): 5.2, 7.
- Phases 4, 5 and 6, and PR C as planned: done or page 7.
- Status 2026-10-28 (tolerances reopened), 2026-10-29 to 2026-10-30 (runs 1 and 2, the chosen
  configuration, the walk-through): 5.2, 5.3, 5.7.
- Status 2026-10-31 (survey decisions, run 3, Q2, slivers, the design): 5.1, 5.2, 5.5.
- Status 2026-11-04 to 2026-11-08 (steps 1 to 15 and reviews 1 to 8): every finding and decision
  is in 5.1 to 5.7 or 6, or is a comment at the code (the `DIFFERENCE` macro; the stale object
  file after a copy and the rewrap script's `++i)` are tool traps, not lasting); the measurements
  that decided something (the sliver histogram, the timing, the hunt's size and residue, the
  real-data checks) are in 5.5, 5.6 and 6.
- "Resume here": the open items are page 7.
- Verification: page 6.

From `phase4a-run1.md` to `phase4a-run4.md`, `phase4a-variants.md`, `overlay-walkthrough.md`:
the algorithm's derivation, each rule's reason and each rejected variant with its failure are in
5.2, 5.3 and 5.7 (run 1: the catalogue families and V1's failure; run 2: V0, V2b, the LL rule, the
margin order, option 3, the order in doubles, the cascade; run 3: the bundle rule and its rejected
criteria, ring continuity, polylines, Q2; run 4: the one-passage rule, the spike rule's condition,
`r4n`, the local rules). The walk-through's sections 3, 4, 5, 6, 7.3 to 7.9, 8, 9, 10, 11, 12, 13 and
14 are in 5.2 and 5.4 (7.1 and 7.2's account of records is 4.2; 15 points at the requirements).

From `phase4b-survey.md`: 1 (who calls what): 5.1. 2 (the result contract): 5.1. 3 (the cascade):
5.1, 5.4. 4 (coverages, the exterior-only assumption, `partition_features` dropping coverages, which
points have values): 5.4 and the changelogs. 5, 6: decisions, all made.

From `phase2-requirements.md`: 3.1 to 3.6 (the candidates, the overlay's steps, the assessment and
the recommendation): 5.7 and 5.2. 3.7 (the self-overlay): page 7. 4 (the eleven questions): all
answered, in 4.4 to 4.5 (Q1, Q9) and 5.x (Q2 polylines along, Q3 the sector rule and option 3, Q4
the result contract, Q6 orientation, Q7 OGC style and the open arc, Q8 the layers, Q11 the split;
Q5 and Q10 are the audit's). The Phase 4a amendments at its end (identity grouping, R3 not needed,
R4 confirmed by use, F2 a premise of the overlay): 5.2.

From `phase5-review-1.md` to `phase5-review-8.md`: each finding's fix is in the code with its
comment, and its design consequence is on the page (review 1: one stop per ring vertex, 5.2; review
2: the bundle counts pinned, 6; review 3: 1.1 and 1.2, 5.2; review 4: provenance after the split and
the clip location, 5.4; review 5: one contract, 5.1; review 6: the closed-ring run and the early stop
dropped, 5.4 and 5.1; review 7: one feature per coverage, time-dependent geometry, 5.7 and 5.4;
review 8: the great-circle ring, the cone by stop type, the dropped ring's contents, the constant
range, 5.5, 5.2, 5.4). Their test-gap lists are the tests named in 6.

## 4. What stays in the plan, and what was judged not lasting

Stays until PR C deletes the plan:
- PR C's own content: design 5.2's exposure of the operations and the Python error's name; design
  14 question 5 (a geometry-level coverage form); PLAN.md "PR C" and its Decisions 2026-10-25; the
  symmetric difference's crossing pairing (PLAN.md 2026-11-08) and the self-crossing ring's
  orientation (2026-11-04, step 8). All summarised in page 7.
- The open residue and hunt differences (design 9.1 as measured; PLAN.md 2026-11-08): page 7 names
  them; the case names (`shared#2288 seed 2`, the 4 + 4 + 1 differences) stay in the plan and the
  committed data file's name.
- How to drive the prototype, the exporter and the hunt (PLAN.md "Resume here": `overlay_export.py`,
  the configuration `r5o`, re-export when a step changes what is compared; the planning repository
  paths), since the tools live outside this repository.

Judged not lasting, and not placed: the per-step commit hashes and dates; the per-case names of
the prototype's runs except where a test names them; the Diagnostics counts case by case (the
`BundleRule` test pins what matters); design 2's sketches with `d_` names; design 10.1's cost table
(unmeasured estimates); design 12's order of commits; design 11.1's table of families (the tests
carry them); the review reports' "checked and found correct" lists; the tool traps (a copied
source keeping an older time than its object, the rewrap script eating `++i)`, `CONFIGS` duplicate
keys, `gtest` printing `PASSED 0 tests` on a failure, the CRLF archive); the session's timing of
individual mutations.

## 5. Not placed, or contradicted by the code

- **Design 6.3, "the pairing map is keyed by a `Node { ray, side }` ... the one structural choice
  PR B makes for the port"**: not built. The pairing is `partner[vertex][ray]`
  (`PolygonOverlay.cc:3386-3397`). Page 7 says the pairing is keyed by ray today.
- **Design 9.3, the all-vertices check "asserted in debug builds"**: the code only counts it
  (`piece_side_disagreements`, `PolygonOverlay.cc:2312-2321` and `:2146-2155` under
  `GPLATES_DEBUG`); PLAN.md 2026-11-04 records "counted only". The page says counted.
- **Design 9.3, "a `GPLATES_DEBUG`-only dump"**: `Overlay::dump` is unconditional
  (`PolygonOverlay.h:879-881`). Not on the page (a debugging aid).
- **Design 3.9's `crossing_parity_excluding_segment(polygon, from, to, excluded_ring,
  excluded_segment)`**: the code takes one global segment index
  (`PointInPolygon.h:125-130`). The page names the function without its parameters.
- **Design 3.5 omits the chain cap**: `MAX_CHAIN = 16` (`PolygonOverlay.cc:1091`). The page and
  §1 include it.
- **Design 3.6, "a piece whose arcs are all zero length ... at six or more ends it floats with a
  cone of `pi`"**: true, but `sort_rays_anticlockwise` also puts such rays last, in end order
  (`:1385`); only stated in the code.
- **Design 10.1 (costs)**: estimates, not measured; the only measured cost is the real-data timing
  (page 6). Not placed otherwise.
- **PLAN.md 2026-11-04, "A sine-based distance would order [tying margins]; only if the hunt shows
  it matters"**: the hunt did not show it; nothing to place (the page says a tie is harmless).
- **`PolygonOverlayPrototypeTest.cc`'s header named `r5c`** where the snapshots are `r5o`: fixed
  (item 19).
- **Measurements** (the prototype's failure counts, the hunt's 119,198 cases and its 13 differences,
  the sliver histogram, the real-data counts and the timing) cannot be verified by reading; the page
  keeps those that justify a decision and attributes them as measurements. Nothing on the page that
  describes code behaviour rests on them.
- No statement of the design about the code's behaviour was found wrong beyond the above; the
  design's "as built" passages match the code everywhere else checked.
