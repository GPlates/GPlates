# PR B claim check: `spherical-geometry.md` sections 5.1 to 5.7, the new table rows, section 6 and 7

Branch `feature/polygon-intersections` at `d7bf5c585` (clean), 2026-11-08. The page is
`docs/design/architecture/foundation/spherical-geometry.md`; line numbers are its. Every claim of
the parts written in `9bc4f3754` was read against the code as it is at this commit, by reading the
functions that apply each rule (not their comments). The plan (`phase4b-design.md`, `PLAN.md`, the
run and review reports) was used only for the measurements the page quotes, in section 3 below.

Severity: **high** wrong in a way that would mislead a reader about behaviour; **medium** wrong in a
detail, or misleading; **low** imprecise, naming, wording, or an omission with a suggested sentence.

Summary: 0 high, 2 medium, 17 low, and one note (20) needing no change. Nothing in sections 5.1 to
5.7 misdescribes what the code does on a normal path; the two medium findings concern what is counted
versus thrown, and what the prototype comparison compares as rings.

## 1. Findings

### 1. medium: "counted, not asserted" is neither, for both properties

Line 1349-1350: "Two properties hold by construction and are counted, not asserted: every input has
an even number of ends at every vertex, and every walk of the linking closes."

Evidence: no `Diagnostics` field records end parity (`src/maths/PolygonOverlay.h:516-582` lists every
counter; `max_ends` is the only one about ends, and `both_pass_twice` counts vertices with four or
more ends per input). A walk that does not return to its start increments `num_pairing_failures`
(`src/maths/PolygonOverlay.cc:3709-3713`), which is added to `pairing_failures` and *thrown* as
`InconsistentOverlayException` when `throw_if_inconsistent` (`:3853-3862`). So the first property is
not counted and the second is thrown, not merely counted.

Correction: "Two properties hold by construction and are not checked on their own: every input has
an even number of ends at every vertex (nothing counts it), and every walk of the linking closes (a
walk that did not would be a pairing failure, counted and thrown, section 5.6)."

### 2. medium: the prototype comparison compares rings for three operations only

Line 2157-2159: "output rings compare by the oracle, and as rings where the prototype's did not
depend on the tie order."

Evidence: the snapshot holds the rings of the intersection, difference and union only
(`src/unit-test/PolygonOverlayPrototypeTest.cc:149-151`, `outputs[3]`), and `compare_rings` compares
those three (`:448-450`); the reverse difference and the symmetric difference are checked by the
oracle's point samples alone (`:1098-1105`). This is exactly why section 7 (line 2219-2220) can call
the symmetric difference's pairing "unguarded by the prototype comparison" while section 6 reads as
if every operation's rings were compared; a reader cannot reconcile the two from the page.

Correction: "output rings compare by the oracle for every operation, and as rings for the
intersection, the difference and the union where the prototype's did not depend on the tie order
(the reverse difference's and the symmetric difference's rings are checked by the oracle only, which
is what leaves the symmetric difference's pairing unguarded, section 7)."

### 3. low: the polyline check counts any two classified pieces, not only tested ones

Line 1467-1468: "The one check: two *tested* pieces at an LL vertex with equal classes, a missed
intersection, counted (`ll_disagreements`) and both kept."

Evidence: `src/maths/PolygonOverlay.cc:2202-2218`: the branch `a_known && a.piece_class ==
b.piece_class` fires for any two classified pieces whatever their method; a piece deduced across a
neighbouring LL vertex can be one of them (a tested piece `a` at vertex `v1` deduces `b`; at `v2`,
`b` and a tested `c` can then be equal). The code's own comment (`:2162`) says "tested" too.

Correction: "two *classified* pieces at an LL vertex with equal classes (a missed intersection, or a
class deduced across a neighbouring crossing)".

### 4. low: the rotational order is consulted once more, for the one-passage linking order

Line 1191-1193: "The order is consulted only at six or more ends where both inputs pass more than
once (below): at four ends it is not used, and where one input passes once neither."

Evidence: at a one-passage vertex the linking order is built from the classes with the rays inside
"in their measured relative order" (`src/maths/PolygonOverlay.cc:1789-1797`), which the page itself
says at line 1254-1256. True for classification; the sentence as written contradicts line 1255.

Correction: "For classification the order is consulted only at ... ; the one-passage rule's linking
order keeps the measured relative order of the rays inside (below)."

### 5. low: a common ray's cone is omitted

Line 1185: "A common ray takes the subject's direction, or the clip's if the subject's first arc is
zero length."

Evidence: `src/maths/PolygonOverlay.cc:1447-1454`: the common ray also takes the wider of its two
pieces' cones, with that piece's `delta`, `first_arc_length` and `reach`. The bundle rule (line
1318-1347) depends on this and the page never says it.

Correction: append "and the wider of its two pieces' cones (with that piece's `delta`, length and
`reach`)."

### 6. low: the rotational order is "exact" in 5.3 and "in principle rounded" in 5.2

Line 1483-1484 (the **Exact** list): "the rotational order among rays (sign tests in doubles,
measured to agree with exact arithmetic wherever used)"; line 1188-1189: "In principle that is a
rounded decision; measured in the prototype, the order in doubles agreed with the exact order".

Evidence: `sort_rays_anticlockwise` (`src/maths/PolygonOverlay.cc:1338-1361`) compares
`dot(centre, cross(a, b))` with zero in doubles, which is exact for its inputs but whose inputs are
tangents of nearby arcs: the two sentences disagree on which kind it is.

Correction: in 5.3 move it to the **Rounded** list, or keep it under Exact worded "treated as exact:
sign tests in doubles on directions that are themselves rounded, measured to agree with exact
arithmetic wherever the order is used, and consulted only where a swap of two near rays is
harmless (section 5.2)".

### 7. low: the exception does not carry the record kinds

Line 1657-1658: "a `MathematicalException` carrying the `Diagnostics` and the first failing vertex's
position and record kinds".

Evidence: `InconsistentOverlayException` stores a message, the diagnostics and the vertex position
(`src/maths/PolygonOverlay.h:595-604, 637-640`); the record kinds are written into the message
string only (`src/maths/PolygonOverlay.cc:2538-2547`).

Correction: "carrying the `Diagnostics` and the first failing vertex's position, with the vertex's
latitude, longitude and record kinds in its message".

### 8. low: `most_overlapping_plate` is not a function

Line 1589-1590: "(`GeometrySizeMetric`, and `most_overlapping_plate` in `PlatePartitioning.py`)".

Evidence: `most_overlapping_plate` is an enumerator of `PartitionMethod`; the comparison is the
`else` branch of `partition_features` (`src/qt-resources/python/api/PlatePartitioning.py:403-438`),
a tuple comparison `(area, arc_length, num_points)`.

Correction: "(`GeometrySizeMetric` in C++, and the `PartitionMethod.most_overlapping_plate` branch
of `partition_features` in `PlatePartitioning.py`)".

### 9. low: the ungrouped sort order is omitted

Line 1039-1042: "The polygons are kept in the order added, networks, then boundaries, then static
polygons, and each group can be stable-sorted by plate ID or by area, highest first".

Evidence: with `group_networks_then_boundaries_then_static_polygons = false` the cookie cutter sorts
every polygon as one group (`src/app-logic/GeometryCookieCutter.cc:168-171, 573-592`), and
pyGPlates' `SortPartitioningPlates.by_plate_id` and `by_plate_area` ask for exactly that
(`src/api/PyPlatePartitioner.cc:121-146`); only the `by_partition_type*` values group. A pyGPlates
reader would take the page's order as unconditional.

Correction: add "or, when a caller asks not to group them (pyGPlates' `SortPartitioningPlates.by_plate_id`
and `by_plate_area`), the whole list is sorted as one".

### 10. low: a grouping failure is kept and counted, and the page does not say so

Line 1419-1432 (Grouping): no mention of a hole whose parent is not a kept exterior.

Evidence: `src/maths/PolygonOverlay.cc:4110-4118`: an odd-depth ring with no parent, or whose
parent is not a polygon (dropped), is kept as an exterior of its own and counted
(`grouping_failures`); the tests require the count to be zero (`PolygonOverlayTest.cc:1538`).

Suggested sentence, after "an island inside a hole is an exterior of its own": "A hole whose parent
is not a kept exterior (a containment that is not a nesting, which rings that do not cross cannot
give) is kept as an exterior and counted (`grouping_failures`)."

### 11. low: "a feature of its own" overstates what Assign Plate IDs does

Line 1761-1763: "Each coverage piece goes in a feature of its own, as Assign Plate IDs writes them,
because two pieces with the same number of points under one property name would be an ambiguous
coverage on read-back." (Also `CHANGELOG-pyGPlates.md:43`.)

Evidence: Assign Plate IDs reuses an existing feature of the partition when it has no geometry of
that property name yet (`src/app-logic/PartitionFeatureUtils.cc:783-794`), so a coverage piece can
share a feature with other property names' pieces; a new feature is made only when the name is
already present and either side has a range (`:797-816`). pyGPlates puts every coverage piece in a
new feature and the plain geometries in another (`PlatePartitioning.py:56-67`). What both guarantee
is narrower than the sentence.

Correction: "No two pieces with a coverage share a feature under one property name (pyGPlates gives
each its own feature; Assign Plate IDs reuses a feature only while that name is absent from it),
because two coverages with the same number of points under one name would be ambiguous on
read-back."

### 12. low: "floats" is used before the bundle rule defines it

Line 1183-1184: "A piece whose arcs all lie on zero-length segments has no direction; it bounds no
sector any order needs at two or four ends, and floats at six or more." Floating and anchored rays
are defined at line 1322-1324.

Correction: "and at six or more the bundle rule below lets it float".

### 13. low: `PartitionFeatureUtils::partition_feature` is "the same", but called differently

Line 1561-1563: "Assign Plate IDs and pyGPlates' `partition_features` partition ... through the same
`PartitionFeatureUtils::partition_feature`".

Evidence: pyGPlates passes `respect_feature_time_period = false` (`src/api/PyPlatePartitioner.cc:470`);
Assign Plate IDs passes the user's setting (`src/app-logic/GenericPartitionFeatureTask.cc:73-78`),
and a feature not defined at the reconstruction time is then skipped. The Python test
`test_partition_features_other_cases` (`pygplates/test/test_app_logic/test.py:2276-2286`) pins the
difference. The page's "the same" is true of the code and could hide this difference between the
products.

Correction: append "(pyGPlates does not apply Assign Plate IDs' check that the feature exists at
the reconstruction time; it never did)".

### 14. low: the polyline result when every run collapses

Line 1082-1083: "A polyline is `INSIDE` when no run is outside and `OUTSIDE` when none is inside".

Evidence: `src/maths/PolygonPartitioner.cc:223-229` returns `INSIDE` only if some run is inside,
else `OUTSIDE`; `Overlay::classify` likewise (`PolygonOverlay.cc:2082`). When no run of either class
remains the answer is `OUTSIDE`. An edge the code's comment calls "if one can arise".

Correction: "`INSIDE` when no run is outside and some run is inside, else `OUTSIDE`".

### 15. low: the test named for the most overlapping plate is outside the pattern given

Line 2183-2187: "The pyGPlates tests (`test_partition*` in ...) cover ... and the most overlapping
plate by area."

Evidence: that test is `test_most_overlapping_plate_by_area` (`pygplates/test/test_app_logic/test.py:2146`);
`test_partition_features_other_cases` (`:2288-2296`) also checks polygons deciding before polylines.

Correction: "(`test_partition*` and `test_most_overlapping_plate_by_area` in ...)".

### 16. low: comments cite numbered steps of the bundle rule that the page does not number

`src/maths/PolygonOverlay.cc:1886` "(the bundle rule's step 3, section 5.2)", `:2697` "(the bundle
rule's step 1, section 5.2)", `:2771` "Admissible orders (step 2)". The page's bundle-rule paragraph
(line 1318-1347) is prose without steps; only the classification is numbered.

Correction: either number the bundle rule on the page (1 anchoring, 2 the admissible orders, 3 the
solve and the tie-break), or drop "step n" from the three comments.

### 17. low: a test comment cites 5.5 for a rule the page states in 5.1

`src/unit-test/PolygonPartitionerTest.cc:1154-1155`: "the subject is whole on the side that lost the
larger area (section 5.5)". The page states this at line 1089-1090 (section 5.1); 5.5 says only "The
result contract is evaluated after it (section 5.1)" (line 1614).

Correction: cite 5.1 in the comment, or repeat the one sentence in 5.5.

### 18. low: a stale code comment the page's bundle-rule paragraph now contradicts

Not the page: `src/maths/PolygonOverlay.cc:136-140` says the cap is "never reached in Phase 4a's
3013 random cases, most was 4", but `PolygonOverlayPrototypeTest.BundleRule` pins `shared#2240 seed
12` at 7 floating rays and `capped = 1` (`src/unit-test/PolygonOverlayPrototypeTest.cc:1656`), and
the page (line 1334) describes the cap as reachable. The comment predates the `r5o` cones.

Correction (code comment): "more keep the measured order; reached by one hunt case under the `r5o`
cones, `shared#2240 seed 12`, which the `BundleRule` test pins".

### 19. low: the self-touch cuts' frequency undersells one generator (measurement)

Line 1176-1177: "The cuts fire in about a quarter to a half of the dense shared-edge cases of the
hunt and almost nowhere else."

Plan record (`PLAN.md:459-461`): `shared` 256 of 598, `rays` 226 of 598, `chain` 82 of 598 (266 of
2,099 in the chain set), `near` 2, `six` 4, the rest none, catalogue `d4` 13 of 49, bundle 7 of 15.
That is 43%, 38% and 13-14%: `chain` is below "a quarter".

Correction: "in 14% to 43% of the cases of the dense shared-edge generators (`shared`, `rays`,
`chain`) and almost nowhere else".

### 20. low: the real-data section does not say what a width of `h` means for the hemisphere ring

Line 1624-1626 and 2190-2201: fine as written; noted only that the great-circle-ring failure
(line 1623-1626) was found by the pyGPlates test `test_partition_sliver_width_threshold`
(`pygplates/test/test_maths/test_geometries_on_sphere.py:1900-1910`), which the section-6 list of
pyGPlates tests ("the sliver argument") covers without naming it. No change needed.

## 2. Checked and found correct

By subsection; the code read is named so that a later reader can re-check.

**Introduction (21-28), 3.3, 3.5, 4.1.** `PolylineIntersections` is gone from `src/maths`; the five
operations are `PolygonOverlay::Operation` (`PolygonOverlay.h:113-120`) and nothing under `src/api`
names `UNION`, `REVERSE_DIFFERENCE` or `SYMMETRIC_DIFFERENCE`. The velocity layers partition points
through `GeometryCookieCutter::partition_point` (`PlateVelocityUtils.cc:300-307`). 3.5's list of
where the threshold is off matches `is_inside_other` (`PolygonOverlay.cc:1492-1500`), the anchorless
parities (`:3072`), `is_ring_inside_ring` (`:217-218, 235`) and the hemisphere check (`:3932-3940`).
4.1: the overlay calls `GeometryIntersect::intersect` (`:313, 331`), CitcomS takes
`unordered_intersections[0]` (`GenerateVelocityDomainCitcoms.cc:272-278`).

**Section 1, the nine new rows.** Identity grouping by union-find keyed (side, global segment)
(`PolygonOverlay.cc:476-509`); `is_same_position` at the cone (`:1285`), clean-up (`:3813, 3824,
3840`) and the polyline joins (`:4189`). Test points threshold off (`:1492-1500`). Midpoint margin
order, largest first (`:2347-2362`). Self-touch cut: `h` as the thickness sine, `ALONG_ROUNDING =
1e-12`, `MAX_CHAIN = 16` (`:1090-1092, 1154, 1182-1184`). Bundle cone: `0` when `delta == 0`, `pi`
when `L <= delta`, else `atan2(delta, L - delta)`; offsets only for a stop not at a vertex whose
first record is not `SEGMENTS_CROSS`, or with no record (`:1272-1302`). Anchorless `min_sine = 1e-6`
against the axis and against `cross(R, X)` (`:3063-3071`). `HEMISPHERE_CHECK_MARGIN = 10` times the
thickness (`:185, 3898-3899`); the grouping margin `max(10 h, 2 * cluster_width)` (`:3970-3985`).
Sliver widths `10 h` (`GeometryCookieCutter.cc:327-331`; `PyPlatePartitioner.cc:67-85`, `None`
default, `0` off, negative `ValueError`), `h` (`PyGeometriesOnSphere.cc:3654-3661, 3701-3708`), none
(`PolygonPartitioner.h:85`). `get_mean_width` apex thresholds `1e-18` and `1e-12` (`:269-273`).
`h` is `1.4142e-6` radians, 9.0 m; `10 h` 90 m.

**5.1.** `GeometryCookieCutter` holds a `PolygonPartitioner` per polygon
(`GeometryCookieCutter.h:308-319`); rigid blocks then the boundary with
`include_rigid_blocks_as_interior_holes` (`GeometryCookieCutter.cc:619-655`); per-group stable sort
by plate ID or area (`:739-767, 783-816`); Assign Plate IDs passes `SORT_BY_PLATE_ID`, grouped, high
speed (`AssignPlateIds.cc:83-93, 173-183`). `partition_point` threshold on
(`PolygonPartitioner.cc:736-745`); a point on the boundary inside (`:325-327, 375-378`). One
`Overlay` per call (`:124, 204`). `is_outside_by_bounds` (`:79-96`). The other callers: `src/api/
PyGeometriesOnSphere.cc` and `GLReconstructedStaticPolygonMeshes.cc` only (`TopologyUtils.h:52` is an
include with no use). The cascade (`GeometryCookieCutter.cc:457-527`). The result contract:
`get_result_from_classes` (`PolygonOverlay.cc:2560-2612`) then extraction and the emptiness rule
(`PolygonPartitioner.cc:124-159`), the both-empty rule by dropped area with outside on a tie
(`:144-149`); whole geometries returned as the same object (`:665-672, 706-716`, `add_whole`
`:515-535`); the multi-point whole (`:338-349`); the result-only path through the same
`overlay_polygon` (`:386-391, 688-704`); the polyline rule and the collapsed run (`:211-229`).

**5.2 Records and vertices; stops and pieces.** Subject first (`:313`); `build_vertices` (`:461-581`:
output position subject, else clip, else first record); one stop per ring vertex with the assertion
(`:667-690`), `DANGLING_END` (`:693-695`), records inside a segment in ordered-list order with
`ratio_along_segment` by projection (`:79-113, 706-724`); pieces between boundaries, one record stop
giving a piece round to itself (`:767-771`), closed pieces (`:815-821, 831-848`), `single_arc_segment`
(`:783-788`).

**Loop and spike rules; common pairs; self-touch cuts.** `:909-1008` (the tip's other ends `> 2`
rejects, `:976-988`); `:1012-1072` (`common_ambiguous` when both sides have pieces, `:1066-1070`);
`:1076-1208` (chain of the other input's non-common single arcs, in order along the segment, within
`h` of its plane in doubles; rebuilt once, `:424-441`).

**Rays and their order.** Direction from the first non-zero-length arc's start (`:1240-1253`); no
direction when all are zero length (`:1306-1313`); `sort_rays_anticlockwise` (`:1338-1361`), rays
with no direction last (`:1386`); the bundle built at six or more ends where neither passes once
(`:1469-1472`).

**Classification.** Step 1 (`:2276-2323`, debug count `:2313-2322`; clip at `clip_speed`, subject at
`ADAPTIVE`, `:1492-1500`). Step 2: the free rule and the LL rule (`:1504-1596`, one crossing record
`:1536-1538`); the one-passage rule (`one_passage_side` `:2933-2959`; `:1689-1808`, mirror order
`:1798-1807`); the sector walk over every order with four starts (`:1819-1872`), deductions only
where every solution agrees (`:1943-1983`), memberships where the chosen order's solutions agree
(`:1985-2014`). Step 3 (`:2332-2380`). Step 4 (`get_common_chain` `:2962-2997`;
`classify_anchorless_chain` `:3000-3098`; `get_crossing_parity_excluding_segment`
`PointInPolygon.cc:2289-2345`, one global segment index). Step 5 (`:2427-2436`).

**The bundle rule.** Certainty order and separability (`:2704-2750`); never anchors without a
direction or at a cone of `pi` (`:2736`); `BUNDLE_CAP = 6`, `> 6` capped (`:140, 2759-2763`); no
frame (`:2764-2768`); admissible orders by greedy earliest placement, gaps in anchored order,
floating rays in certainty order, lexicographic permutations (`:2779-2904`); measured order first
(`:2906-2928`); fewest inversions, ties by enumeration order (`:1888-1926`).

**Checks and repair.** `check_and_repair` (`:2440-2557`); the ranks (`:3150-3168`), whole-chain
candidates ranked with the sector classes and before single pieces (`:3245-3276`), the counters
(`:3287-3301`), `max_repair_rounds` 20 (`PolygonOverlay.h:732`).

**Linking, split, clean-up.** `is_kept` (`:3318-3343`); the table was checked cell by cell against
`is_in_output` (`PolygonOverlay.h:125-144`). Pairing rules in order (`:3400-3622`): unresolved and
odd counted and failing, two kept, the free rule's crossing and bounce, ring continuity (one side has
ends, none common), all-common in either phase, the sector walk with `keep_mismatches`. The walk
(`:3649-3716`), closed pieces (`:3719-3733`), split by identity into the other non-crossing pairing
(`:3740-3772`), clean-up exact and counted (`:3809-3848`), the throw (`:3853-3862`). Along a common
pair the walk follows the subject piece (`ray_end`, `:3378-3382`).

**Grouping, orientation, hemisphere check, sliver filter.** `is_ring_inside_ring` (`:191-236`);
containers and parent of greatest depth (`:3986-4017`); orientation by
`PolygonOrientation::calculate_polygon_orientation` (`:4027-4039`); the hemisphere check's candidate
points, parity reading, `f`, both messages, and `hemisphere_check_skipped` (`:3900-3961`); the filter
outermost first with dropped parents (`:4044-4089`).

**Polylines.** `classify_polyline_pieces` (`:2091-2260`): along pieces inside by
`polyline_along_is_inside` (`:2101-2111`), test points (`:2123-2142`), deduction across one LL record
with two non-common polyline pieces (`:2175-2213`), midpoints by margin (`:2222-2258`);
`extract_polylines` (`:4125-4199`), internal pieces taking the neighbour's class, stop positions.

**5.3.** Each entry traced to the code named under 5.2; `PointInPolygon.cc:357-380` for the
zero-length edge's end point standing in for its crossing.

**5.4.** `Provenance`, `Location`, `Options` (`PolygonOverlay.h:644-682, 747-776`);
`get_vertex_provenance` (`PolygonOverlay.cc:4216-4296`); provenance after the split
(`:3781-3807`); the polyline stop's own record (`:4321-4362`); `compose_subject_location`
(`:4366-4547`: wrap-around only with the original or a cascade clip, the two mismatch counters).
`CascadeProvenance`, `create_identity_provenance` (`PolygonPartitioner.h:155-173, 310-313`;
`.cc:480-535, 827-834`). The cookie cutter's `want_provenance` on `partition_geometry` only
(`GeometryCookieCutter.h:242-266`; `.cc:355-385, 417-431, 464`). Coverages: steps 1 to 3
(`PartitionFeatureGeometries.cc:641-746, 756-840`; the nearest outline by projection, clamped,
`:331-396`), provenance asked for only with a coverage (`:564-568`), `exterior_ring_only`
(`:646-671`), only `subject` locations read (`:699-700`). Time-dependent geometry
(`:152-179, 467-512`); `partition_features` removing the reported properties
(`PlatePartitioning.py:369-382`; `PyPlatePartitioner.cc:460-479`). `GeometrySizeMetric::operator<`
(`PartitionFeatureUtils.cc:898-913`). A task partitions before it modifies
(`GenericPartitionFeatureTask.cc:66-93`).

**5.5.** `Options::sliver_min_width` default none (`PolygonOverlay.h:789`); `get_mean_width`
(`PolygonOverlay.cc:239-299`: Van Oosterom and Strackee fan, modulo `4 pi`, smaller side, perimeter
by `atan2`, apex fallbacks); the three widths as under section 1; `partition_into_plates` takes the
argument (`PlatePartitioning.py:480-490, 712-740`).

**5.6.** The throw sites (`:2522-2554, 3853-3862, 3941-3956`); `throw_if_inconsistent` default true
(`PolygonOverlay.h:738`); `PartitionError` as a `MathematicalError` naming the four entry points
(`PyExceptions.cc:482-494`); `AssignPlateIds` catching per feature (`AssignPlateIds.cc:290-306`;
`.h:308-319`); the dialog's one warning, ten names, also after cancelling
(`AssignReconstructionPlateIdsDialog.cc:307-316, 540-568`); the CLI's `qWarning` per feature
(`CliAssignPlateIdsCommand.cc:392-398`); the raster meshes catching and taking intersecting
(`GLReconstructedStaticPolygonMeshes.cc:966-977`). `inconsistent.txt` exists and is the input of
`AssignPlateIdsSkipsInconsistentFeature` and `test_partition_error`.

**5.7.** Each "what the code does instead" traced as above; `_partition_feature_geometries`
(`PyPlatePartitioner.cc:942-943`; `PlatePartitioning.py:344-345`).

**Section 6.** Every test the page describes exists under the family named, and does what the page
says: the 53 `PolygonOverlayTest` cases (structure, rays, rules, classification, repair, linking,
grouping, orientation, hemisphere, self-touch, polylines, provenance), the 18 `PolygonPartitionerTest`
cases, the 11 `PartitionFeatureUtilsTest` cases, the 9 prototype tests (`Catalogue`, `Hunt`,
`Polyline`, `Anchorless`, `Bundle`, `File` behind `GPLATES_POLYGON_OVERLAY_PROTOTYPE_CASES`,
`OnePassageRule`, `PartitionerCollapsedRegion`, `BundleRule` pinning floating, admissible and
placement counts), `PolygonOverlayRealDataTest.Coastlines` behind the two environment variables with
"0 100 200" as the default times. The oracle's items (`PolygonOverlayOracle.h:46-53, 410-416,
707-754, 757-797, 847-880`); local tangent frame at a centre on no axis (`PolygonOverlayTest.cc:50-53,
112-113`); band-piece exemption and common-pair rule (`PolygonOverlayPrototypeTest.cc:77-85,
986-1067`); no repair on generated data (`:837-847`); the configuration `r5o` (`:63`). The pyGPlates
tests named are at `test_geometries_on_sphere.py:1793-1910` and `test.py:2146-2443`.

**Section 7.** The pairing is `partner[vertex][ray]` (`PolygonOverlay.cc:3387`); `RingView::closed`
and `Stop::DANGLING_END` exist (`PolygonOverlay.h:237, 278`); `intersect_segments` exists
(`GeometryIntersect.cc:340`); a self-crossing ring's orientation remark matches the code's comment
(`PolygonOverlay.cc:4024-4025`); the time-dependent bullet matches 5.4.

## 3. Measurements that cannot be read from the code, and their consistency with the plan

Each is attributed on the page as a measurement or a count; none decides a statement about
behaviour. Found in the plan unless marked:

- `28.8 h` for 65 records (line 1121): `phase4a-run2.md`, `phase4a-variants.md`.
- Rays `1.8e-15` radians apart (1190): `phase4a-run2.md`, `phase4b-design.md`.
- 30 of 824 rays, `2.4e-11` (1211-1213): `phase4b-design.md`, `phase5-review-8.md`.
- Self-touch cuts "a quarter to a half" (1177): `PLAN.md:459-461` gives 43%, 38%, 14% (finding 19).
- 218 midpoint tests against 1 (1245); 1 / 24 / 250 and `0.5 h` (1288-1289); 9 of 300 (1287):
  `phase4a-run2.md`, `phase4b-design.md`.
- 19 of 2,393 `chain` cases (1258): `phase4a-run4.md`, `phase4b-design.md`.
- "about half the prototype's runs" for the waypoint (1313-1314): `phase4b-design.md:1887`.
- `0.42 h` (1341): `phase4a-run3.md`, `PLAN.md:259`. One vertex in 3,500 (1347): `phase4b-design.md:622`,
  `PLAN.md:382`; the `Bundle` test's comment says "about 1 vertex in 900 dense shared-edge cases"
  (`PolygonOverlayPrototypeTest.cc:1504-1505`) and `PLAN.md:405` "15 cases from about 15,900
  `shared`": different denominators (all cases against the dense generator), not a contradiction.
- `1.6 h`, 5 of 119,198, "about a quarter" (1427-1429): `phase4b-design.md:836-837`, `PLAN.md:833`.
- 78 of 300 sampled points (1472): `phase4a-run4.md`, `phase4b-design.md`.
- `0.019 h` (1497): `PLAN.md:314`.
- 611 of 4,192 (1530), `12 h` (1532), `0.03` radians (1546), 0.2% and 3% (1551): `phase4b-design.md`,
  `phase5-review-4.md`, `PLAN.md:517-518`.
- About 320 GPlates-only files (1564): `phase4b-design.md`, `PLAN.md`.
- The coastline histogram (1640-1647): `PLAN.md:743-752` (783 rings, 282 polygons, 5 / 11 / 57 by
  decade giving 5, 16, 73 cumulative; 0.47, 4.5, 373 km²; the named slivers and 71 km x 5 m).
- `0.15 h` to `0.35 h`, `1 h` to `1.7 h`, 67,639 of 119,198 (1683-1688): `phase4b-design.md:1546-1552`.
- 33 of 300; 19 catalogue cases; 19 of 300, 3 of 60, 18 of 299; `20 h`; 10 catalogue cases; 77
  against 48, 10 to 40; `0.3 h` to `1.5 h` (section 5.7): `phase4a-run1.md` to `run4.md`,
  `phase4a-variants.md`, `phase4b-design.md` section 13.
- Section 6: 14,000 cases and 319 mismatches (pre-existing text, `PLAN.md:184`); 119,198 "about
  fourteen times" (`PLAN.md:826`); the 13 differences as 4 + 4 + 4 + 1 (`PLAN.md:849-854, 901`);
  15,255 / 313 / 11 / 2 and the timing 0.28 s against 0.20 s, 0.06 s, 431 against 703, 2,136
  features (`PLAN.md:733-737` and the 2026-11-07 status).
- `1.0 h` and `1.63 h` are the prototype test's tolerances (`PolygonOverlayPrototypeTest.cc:104-111`),
  not on the page; consistent.

History the code cannot show, checked against the plan: the early stop that "answered `INTERSECTING`
where the overlay would have raised" (1086-1089) is `PLAN.md:654-660` (review 6; the inconsistent
input `shared#1434 seed 12`), and `PLAN.md:586-594` records the other defect (intersecting where the
lists returned the subject whole, pinned by `PartitionerCollapsedRegion`). The great-circle ring
measuring `0` (1624-1626) is review 8 and the pyGPlates test at `test_geometries_on_sphere.py:1900-1910`.
"The reviews' fixes are each pinned by a test, most of them checked by reverting the fix" (2187-2188)
is a process claim (`PLAN.md:574` records the mutations); not checkable here.

## 4. Comment spot-check

30 comments citing "section 5.x", across 17 files; "holds" means the cited section states what the
comment relies on.

| Comment | Cites | Result |
| --- | --- | --- |
| `pygplates/test/test_app_logic/test.py:2250` one feature per coverage piece | 5.7 | holds (1761-1763; see finding 11 on the wording) |
| `test.py:2300` time-dependent geometry kept unchanged | 5.4 | holds (1595-1600) |
| `src/api/PyGeometriesOnSphere.cc:3652` a ring narrower than `h` has collapsed; self-overlapping input has no area | 5.5 | holds (1634-1637) |
| `src/api/PyPlatePartitioner.cc:447` same code as Assign Plate IDs | 5.4 | holds (1561-1563) |
| `src/app-logic/AssignPlateIds.cc:303` leave the feature, go on | 5.6 | holds (1672-1677) |
| `AssignPlateIds.h:312` reported once at the end | 5.6 | holds (1674-1676) |
| `GeometryCookieCutter.cc:417` tables composed per cut, clip index | 5.4 | holds (1534-1559) |
| `GeometryCookieCutter.h:208` default width ten thicknesses, why | 5.5 | holds (1631-1648) |
| `GeometryCookieCutter.h:248` every plate must get an answer | 5.6 | holds (1667-1668) |
| `PartitionFeatureGeometries.cc:328` nearest outline, step 3 | 5.4 | holds (1572-1577) |
| `PartitionFeatureGeometries.cc:627` replacing the epsilon map | 5.4 | holds (1581-1583) |
| `PartitionFeatureGeometries.h:49` the file split for pyGPlates | 5.4 | holds (1563-1565) |
| `PartitionFeatureUtils.cc:903` area, then arc length, then points | 5.4 | holds (1589-1593) |
| `src/maths/PolygonPartitioner.cc:74` the bounding-circle shortcut's condition | 5.1 | holds (1047-1051) |
| `PolygonPartitioner.cc:183-186` polyline runs; a side with no run makes it whole | 5.2, 5.1 | holds (1456-1473; 1082-1083) |
| `PolygonPartitioner.h:210-215` same result with or without lists; throws either way | 5.1, 5.6 | holds (1084-1086; 1667) |
| `src/opengl/GLReconstructedStaticPolygonMeshes.cc:967` inconsistent taken as intersecting | 5.6 | holds (1678-1679) |
| `src/maths/PolygonOverlay.h:110` `f(in subject, in clip)` per operation | 5.2 | holds (1368-1379) |
| `PolygonOverlay.h:887` polyline: no sector walk, one check counted, no repair | 5.2 | holds (1456-1468) |
| `PolygonOverlay.h:941` complement means over a hemisphere or wrong classes | 5.2 | holds (1446-1449) |
| `PolygonOverlay.cc:974` the spike rule's tip condition | 5.2 | holds (1144-1148) |
| `PolygonOverlay.cc:1327` near-ties in the sort measured harmless | 5.2 | holds (1188-1191) |
| `PolygonOverlay.cc:1886` "the bundle rule's step 3" | 5.2 | does not hold as cited: the page's bundle rule has no steps (finding 16) |
| `PolygonOverlay.cc:2697` "the bundle rule's step 1" | 5.2 | same (finding 16) |
| `PolygonOverlay.cc:3497` ring continuity, no valid input shows it | 5.2 | holds (1387-1391) |
| `PolygonOverlay.cc:4020` orientation the developer's decision, output on the left | 5.2 | holds (1432-1435) |
| `PolygonOverlay.cc:4540-4542` the induction's two failures; a coverage interpolates | 5.4 | holds (1546-1552) |
| `src/unit-test/PolygonOverlayOracle.h:619` grouping and orientation assume rings that do not cross | 5.2 | holds (1419-1421, 1438) |
| `PolygonOverlayTest.cc:756` the cone was once zero at a clip ray off its edge's line | 5.2 | holds (1195-1213) |
| `PolygonOverlayTest.cc:2317` near-zero output edges | 5.5 | holds loosely: 5.5 (1650-1651) says the overlay added none on real data and no filter was built; the comment's point is that two vertices at one point can make one, which the page does not say |
| `PolygonPartitionerTest.cc:1155` whole on the side that lost the larger area | 5.5 | the rule is in 5.1 (1089-1090), not 5.5 (finding 17) |
| `PolygonPartitionerTest.cc:356` a polyline along the edge is inside, both orientations | 5.2 | holds (1459-1461) |
| `PolygonOverlayPrototypeTest.cc:1584` the result contract's emptiness rule | 5.1 | holds (1076-1081) |

Of 33 checked: 29 hold, 1 holds loosely, 3 cite a section or a step the page does not have
(findings 16 and 17).
