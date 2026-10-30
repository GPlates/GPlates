# PR A distillation report

Branch `feature/polygon-intersections`, worktree `C:/gplates/src/polygon-intersections`, 2026-10-30.
Edits are uncommitted. Nothing under `docs/plans/` was touched. Line numbers below are of the
working copy after the edits (code) or before them where noted.

## 1. Changes made

### `docs/design/architecture/foundation/spherical-geometry.md`

1. **Section 4.1, the `PointInPolygon` bullet.** Added that its sign tests are exact with no
   thickness, parity is exact, and the on-outline test is a separate distance pass with the
   threshold that is `GeometryIntersect`'s thickness, so a vertex on the outline there is a
   contact here. Source: audit 3.8 (and PLAN.md Phase 3, "Also `PointInPolygon.cc:173`'s own
   arc-plane intersection, for consistency"). Evidence: `PointInPolygon.cc` sign tests as the doc's
   own section 3.4 describes them (exact `< 0`); `PointInPolygon::get_point_on_polygon_threshold_cosine`
   equals `GeometryIntersect::Intersection::get_on_segment_start_threshold_cosine()` (`GeometryIntersect.cc:1321`
   returns `1.0 - EPSILON`; `GeometryIntersectTest.cc:950-952` pins the equality).

2. **Section 4.2, "A vertex shared by the two geometries is therefore reported once".** Added that a
   vertex shared by two rings of one polygon is reported once per ring (a hole touching its exterior
   ring at a vertex gives two intersections against a vertex of the other geometry there). Source:
   audit R6 ("one record per ring at that vertex"). Evidence: the SS is made per segment pair at
   `GeometryIntersect.cc:437-450`, and the two rings' segments starting at the shared vertex are
   distinct segments of the polygon's arc sequence (`PolygonOnSphere::get_segment` numbering, which
   the `Graph` comment at `GeometryIntersect.h:201-207` describes); `GeometryIntersectTest.cc:590-606`
   ("hole touching the exterior": two SS, segment indices 2 and 4).

3. **Section 4.2, "Pruning".** Replaced the one-sentence "the circles are expanded by the thickness,
   so a pruned pair of arcs cannot touch" with the soundness argument. Source: audit 3.4 (R11).
   Evidence: leaf circle centred on the arcs' centroid with the farthest point of any arc as radius
   (`PolyGreatCircleArcBoundingTree.h:697-702`; `BoundingSmallCircleBuilder::add(const GreatCircleArc &)`
   at `SmallCircleBounds.h:511-519` uses `maximum_distance(centre, gca)`), expanded by the default
   expansion in `get_bounding_small_circle()` (`SmallCircleBounds.h:607-613`), which is the thickness
   cosine (`SmallCircleBounds.cc:443-453`); internal nodes use `create_optimal_bounding_small_circle`
   of the children's circles (`PolyGreatCircleArcBoundingTree.h:671-679`; `SmallCircleBounds.cc:243+`);
   the node test is `angle_between_centres < extent1 + extent2` as an exact cosine comparison
   (`SmallCircleBounds.h:287-296`, `is_precisely_less_than`), called from
   `GeometryIntersect.cc:1025-1030`. The thick crossing conditions (`> THICKNESS_THRESHOLD_SINE`,
   `< -THICKNESS_THRESHOLD_SINE`, `GeometryIntersect.cc:572-582, 612-622`) imply exact opposite
   sides. The triangle-inequality margin is arithmetic on those facts.

4. **Section 4.4, the corner bullet.** Added why a corner contact is two intersections: only the
   coincidence disc about the corner counts as the corner (step 1); beyond it the two thick planes
   still overlap. Source: audit 3.2, last paragraph. Evidence: coincidence is `dot >= THICKNESS_THRESHOLD_COSINE`
   (`GeometryIntersect.cc:348-356`, a disc); touches are plane-plus-lune tests per arc
   (`:830-894`).

5. **Section 4.4, "Ordering".** Added that the key only orders: topology resolving sorts by the same
   key and takes its interpolation ratio from the position, so a change of key moves no point.
   Source: audit R5 caveat 1 and fix F2's risk statement. Evidence:
   `ResolvedSubSegmentRangeInSection.h:155-161` (`operator<`: segment index, then
   `angle_in_segment.is_precisely_less_than`); `ResolvedSubSegmentRangeInSection.cc:742-775`
   (`get_interpolate_ratio_in_segment`: `acos(dot(position, start)) / arc_length`).

6. **Section 4.4, after "What is exact".** Two paragraphs added:
   - the same vertex is decided as the end of one segment and the start of the next bit for bit (the
     "complementary tests"). Source: audit R10 and 3.2. Evidence: consecutive arcs are created from
     the same `PointOnSphere` (`GreatCircleArc::create`, `GreatCircleArc.cc:351-388`, stores the
     points); the vertex tests `GeometryIntersect.cc:348-356` and the plane dots `:572, :578, :612,
     :618` take the arc's cached `rotation_axis()` (`GreatCircleArc.cc:434-447`), so the operands are
     the same `double`s in the pair `(A_prev, B)` (end point) and `(A, B)` (start point);
   - symmetry: swapping the geometries mirrors the types, swaps the orders, keeps positions and
     angles except an SS, placed at the first geometry's vertex. Source: audit R12 and 3.7. Evidence:
     the vertex and plane tests are symmetric in the two segments (`:348-356`, `:568-636`); the
     crossing branches `:661-750` are mirror images; the crossing position is
     `cross(plane1, plane2)` normalised with the sign from `segment1_start_point_on_positive_side_of_segment2_plane`
     (`:245-256`), and swapping negates both the cross product (exactly) and the sign choice (a
     crossing has the two start points on opposite sides, `:672-698` and `:716-742`); SS positions
     `:447, :496, :516, :535`; the GoogleTest `symmetry_of_polygon_and_polyline`
     (`GeometryIntersectTest.cc:860`).

7. **Section 4.4, new paragraph "Near-antipodal arcs".** `GreatCircleArc::create` refuses end points
   antipodal as a `real_t`, so every arc's plane is accurate to about `1e-10` radians and the thick
   classification is sound for every constructible arc; the half-circle guard is only in the
   unreachable fallback. Source: audit R8 and 3.5 (the requirements' "known gap" refuted). Evidence:
   `GreatCircleArc.cc:361-384` and `:574-596` (`evaluate_construction_parameter_validity`,
   `dot_p1_p2 <= -1.0` on a `real_t`); `UnitVector3D.h:318-323`; the axis is the normalised cross
   product (`GreatCircleArc.cc:665-681`); the guard is in `interpolate_plane_crossing`
   (`GreatCircleArc.cc:801-808`), reached only from the fallback at `GeometryIntersect.cc:276-284`;
   `GeometryIntersectTest.cc:614` (`near_antipodal_edge`).

8. **Section 4.5, "Porting `GreatCircleArc::intersect`'s refinements, or one primitive for both
   callers".** Added the concrete cost of unifying: `minimum_distance(arc, arc)` returns zero when
   `intersect()` says the arcs meet, so thick planes there would return zero for arcs passing within
   the thickness of each other, for every C++ caller. Source: audit 3.1, "Should the two be
   unified? No." Evidence: `GreatCircleArc.cc:1041-1056`.

9. **Section 7, new bullet "A self-intersection mode of `GeometryIntersect`".** What such a mode
   needs: no segment against itself (an SS), adjacent segments need no skip except that a
   zero-length segment and its successor give an SS, the tree descended against itself, and the two
   orders becoming one with each record twice. Source: audit 3.10 (requirements section 3.7; PLAN.md
   Decisions 2026-10-25, the simple-polygon split filed as a possible PR after B). Evidence: a
   segment against itself has `dot(start, start) = 1 >= THICKNESS_THRESHOLD_COSINE` at
   `GeometryIntersect.cc:348-349` so `:437-450` adds an SS; for adjacent segments the shared vertex
   is bit-identical, so `segment1_end_point_on_segment2_start_point` is true, which excludes the
   crossing (`:650-653`) and the LS of the later start (`:873-874`), and the SL at `:840-848` needs
   the earlier start on the later segment's plane and in its lune; a zero-length segment's end
   points have `dot >= 1 - 5e-13` (`GreatCircleArc.cc:672`, `magSqrd() <= 0` as a `real_t`), above
   `1 - EPSILON`, so the pair (zero-length segment, its successor) has coincident start points and
   `:437-450` adds an SS (**this caveat is mine, found while verifying the audit's point 2, which
   did not mention it**); the descent structure `:1032-1138` takes two trees and visits ordered
   pairs. The last point (two orders become one) is a statement about `Graph`
   (`GeometryIntersect.h:254-255`), not code behaviour.

### `src/maths/GeometryIntersect.h`

10. **The `angle_in_segment1/2` field comment (`:179-188`).** Added that for a touch the angle along
    the touched segment is to the vertex's projection onto that segment's great circle, and that
    the angles only order (no position is derived from them). Source: fix F2 as applied, audit R5.
    Evidence: `GeometryIntersect.cc:154-166` (`get_angle_along_touched_segment`), `:197-206`.

### `src/maths/GeometryIntersect.cc`

11. **`sort_geometry_intersections` comment (`:117-123`).** Added that keys also tie within the
    angle's resolution near a segment's start (about `1.5e-8` radians, since the angle is compared
    as a cosine), and that consumers must accept tied intersections in either order. Source: audit
    R5 caveat 2 and the hunt's observation (PLAN.md status 2026-10-30, step 5: real-data near-ties
    sort in discovery order). Evidence: `SortGeometryIntersection::operator()` at `:104` uses
    `is_precisely_less_than`, which compares cosines (`AngularDistance.h:201-206`); the resolution
    is the doc's section 1 "cosine resolution" row (`acos(1 - 2^-53)`).

12. **`get_angle_along_touched_segment` comment (`:144-160`).** Added that the key only orders, with
    topology resolving's `operator<` and position-derived ratio named. Source and evidence as item 5.

13. **`intersect_bounding_tree_nodes` comment (`:1019-1030`).** Added why the expansion needs to be at
    least the thickness and no more: the centre-distance margin, and that a thick crossing is an
    exact crossing. Source: audit 3.4. Evidence as item 3.

### `src/maths/SmallCircleBounds.cc`

14. **`get_default_angular_expansion` comment (`:445-456`).** Added that the expansion must not be
    less than the threshold because `GeometryIntersect` prunes node pairs whose expanded circles do
    not intersect. Source: audit 3.4 and 4.1 ("The expansion is derived from `GeometryIntersect`'s
    cosine, so it follows any change of thickness"). Evidence: `SmallCircleBounds.cc:449-450`
    derives it from `get_on_segment_start_threshold_cosine()`; `GeometryIntersect.cc:1025-1030`.

### Changelogs

No entry added. F1's entries exist in both (`CHANGELOG-GPlates.md:43`, `CHANGELOG-pyGPlates.md:39`)
and describe the measured effect. F2 reorders only records within about `sqrt(2)` thicknesses of
each other on one segment and moves no point (item 5); the topology comparison of PLAN.md's
2026-10-30 entry, run with all the fixes in, found only F1's three junction moves, so there is
nothing user-visible to attribute to F2. F4 removed unused code, F5 and F6 are comments, F7 makes
an unspecified order deterministic. None is visible to users.

## 2. PR A plan content now fully distilled (may be dropped from the plan)

From `phase3-audit.md`:
- Section 0's decisions as they bear on PR A: Q1 = `s` (doc 4.4, 4.5; `GeometryIntersect.cc:1306-1321`);
  Q9 = derive in the consumer, `Graph` unchanged (doc 4.5 "Classifying contacts inside the
  primitive", doc 7 "Polygon partitioning"); F3 withdrawn and why (doc 4.5 "Reporting a corner
  contact once"; `GeometryIntersect.h:214-217`); D4 at shallow angles (doc 4.4); what the overlay
  needs from the `Graph` (`GeometryIntersect.h:201-217`).
- Section 1.1 constants: doc section 1 rows (`EPSILON`, zero-length, `s`, thickness, cosine
  resolution) carry the computed values; `GeometryIntersectTest.cc:937-953` pins the two
  inequalities.
- Section 1.2 and 2, the R1-R12 verdicts: R1, R2 (per-ring, corner wedge), R4 (overlaps), R5
  (ordering, ties, key), R6, R7 (closed by `s`), R8 (zero length; near-antipodal, item 7), R9 (the
  refuted gap, doc 4.5; cascade re-feeding in doc 6's hunt description), R10 (item 6), R11 (item
  3), R12 (item 6) are all in doc 4.2, 4.4, 4.5 or 6. R3 (not provided, by design) is doc 4.2
  "Overlap is not a type" and 4.5.
- Section 1.3 defects: D1 (doc 4.5 "Ordering a touch by the angle to its vertex"), D2 (4.5 "Keeping
  the thickness at the zero-length bound"), D3 (4.4, 4.5), D4 (4.4), D5 (4.2 "Where the arcs
  cross", 4.5, `GeometryIntersect.cc:267-275`), D6 (4.2 step 1, `GeometryIntersect.h:72-77`).
- Section 3.1 (thick-plane decisions against `GreatCircleArc::intersect`, unify or not): doc 4.2,
  4.3, 4.5, and item 8.
- Section 3.2 (tunnelling argument): doc 4.2 "Why vertices come first", item 6, item 4.
- Section 3.3 (end-point suppression, fictitious segment, wrap-around, global indices): doc 4.2 and
  `GeometryIntersect.h:52-63, 162-171, 201-207`.
- Section 3.4 (pruning): item 3, 13, 14.
- Section 3.5, 3.6, 3.7: items 7, 6; doc 6 (re-feeding in the hunt).
- Section 3.8 (`PointInPolygon.cc` consistency): item 1, doc section 1 half-circle-guard row.
- Section 3.9 (dead code): removed by F4 (commit `baf701134`); the commit message is the record. Not
  design, so not in the doc.
- Section 3.10 (self-intersection mode): item 9.
- Section 4.1 (Q1): doc 4.4 "One thickness, and it is `s`", 4.5, the measured topology change in
  4.4 and the changelogs; `GeometryIntersect.cc:1306-1321`.
- Section 4.2 (Q9): doc 4.5 and 7.
- Section 5, fixes F1, F2, F4-F7: each is code (`39aac6ba7`, `72a69a0b4`, `baf701134`, `ddf997c35`,
  `9c20957e2`, `f076e6ec9`), with its rationale in doc 4.4/4.5 and its comment. F3 withdrawn: doc
  4.5.
- Section 6.1 (the 16 test cases): `GeometryIntersectTest.cc` and doc 6.
- Section 6.2 (the exact reference and the hunt, including what the observation cannot see):
  doc 6. The tool itself lives in the planning repository.
- Section 7 (corrections): applied earlier (PLAN.md status 2026-10-30, step 2).

From `PLAN.md`:
- Context, "What the survey found": the call graph and consumers (doc 4.1, 5.1), the "lacks its last
  two refinements" claim corrected (doc 4.5), `GeometryCrossing` (deleted; doc 4.2 and 4.5 record
  it), `PolylineIntersections` exterior-only and the bend test (doc 5.1), `PartitionFeatureUtils`
  (doc 5.1), no tests (doc 6 now describes them).
- Decisions 2026-10-23 that bear on PR A (the doc's structure, sections 4 and 5): done.
- Phase 3 as planned: every bullet is covered above.
- Status entries 2026-10-28 to 2026-10-30 for PR A: the fixes, the topology comparison (doc 4.4
  and the changelogs), the GoogleTests (doc 6), the hunt and its three observation corrections
  (doc 6, item 11).
- Verification, PR A: doc 6 (0 mismatches; the GoogleTests). The original "topology-resolving
  output is unchanged" was replaced by the measured bound (doc 4.4).

From `phase2-requirements.md` section 2: the R-list's lasting properties are covered as listed
above; the Phase 4a amendments at its end (R2 relaxed to identity grouping, R3 not needed, R10's
`LL` qualification, R4 confirmed, F2 as a premise of Phase 4a) belong to the overlay and are PR B's
to distil, except that `LL` meaning a real crossing is already stated in doc 4.2 step 3 and 4.4
"What is exact".

Judged not lasting, and left in the plan: the probe scripts' names and per-probe measurements
(the doc keeps the results that matter: the misordering example, the corner-wedge bounds, the
topology numbers, the hunt totals); the hand-counted table of records in the right-angle wedge
(doc 4.4 states the behaviour and the acute-corner bound; test 12 pins the numbers); the audit's
"risk to topology resolving" per fix, superseded by the measurement; the plan's step bookkeeping.

## 3. Not placed, or contradicted

- **Audit 3.2, "The only way to get no record while an exact crossing exists is for `x` to fall
  within about `1e-16` of three thresholds at once."** Not placed. It is a claim about rounding
  that cannot be checked against the code by reading, and the doc's 4.2 "Why vertices come first"
  already states the verified part (the vertex and plane tests differ at the threshold; the order
  removes the one inconsistency that matters).
- **Audit 3.10, point 2 ("adjacent segments need no special skip")** is incomplete: a zero-length
  segment and its successor have coincident start points, so `intersect_segments` reports an SS for
  that adjacent pair (evidence in item 9). The doc's deferred-work entry says so. The audit's
  statement about the pair across the zero-length segment (`seg_i`, `seg_i+2`) is right.
- **Audit 6.2's sensitivity figure** ("a deliberately wrong threshold gives 27 mismatches in 300
  cases") is from the first hunt; doc 6 carries the later and stronger figure from the extended
  hunt (319 mismatches in 2,000 cases with the old code modelled), so the earlier one was not
  added.
- **The `PolygonOnSphere` ring helper** proposed in F6 waits for its first caller in PR B (PLAN.md
  2026-10-30); the `Graph` comment gives the rule instead. Nothing to place now.
- Nothing in the PR A sources was found contradicted by the code beyond the 3.10 caveat above.
