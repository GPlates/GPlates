# Finish polygon partitioning (pyGPlates 1.1, Phase 1)

Areas: foundation, plate partitioning (app-logic `GeometryCookieCutter`), python-bindings

## Status

- 2026-10-23: plan approved. Phase 0 done: branch restarted off `main` (`601ca9e45`); the 2021
  branch is `archive/polygon-intersections-2021` on `private`. Next: Phase 1.
- 2026-10-23: Phase 1 drafted. `spherical-geometry.md` has new §4 Intersection (4.1 to 4.3)
  and §5.1 Partitioning today; Testing and Deferred work are now §6 and §7. DateLineWrapper
  becomes §5.2 (Phase 2). Next: the Fable claim check of §4 and §5.1, then the developer reads them.
- **Found in Phase 1** (measured with pyGPlates at `601ca9e45`; now in doc §5.1). These are bugs in
  the no-intersection shortcut, separate from the polyline output, and PR B must fix them:
  - a partitioning polygon wholly inside the input polygon puts the whole input outside;
  - an input crossing only a partitioning polygon's hole is decided wholesale by one vertex (a
    polygon wholly outside, a polyline wholly inside);
  - an input polygon with a hole is wholly outside a partitioner that crosses only that hole's
    boundary.

  `GLReconstructedStaticPolygonMeshes` works around the first itself. Each case becomes a
  regression test in PR B.
- 2026-10-23: the Fable claim check of §4 and §5.1 found 16 issues, all applied. One is a fourth bug
  for PR B. A piece overlapping the partitioning boundary is classified by the polygon's
  *orientation*: the bend test's triple products are about zero, and `> 0` is exact. So it is
  outside for a counter-clockwise polygon and inside for a clockwise one, although the merger's
  comment assumes overlaps are inside. This is the cascade's normal case once the output is
  polygons. Next: the developer reads §4 and §5.1, then Phase 2 (Fable), after checking the usage
  limit.

## Context

`PolygonOnSphere.partition`, `PlatePartitioner.partition_geometry` and GPlates' Assign Plate IDs
all return an intersecting polygon as **polylines**. The goal is true partitioning: a polygon with
holes cuts an input polygon with holes into output polygons with holes, robustly. The 1.1 checklist
item is *Finish polygon partitioning, from `feature/polygon-intersections`*. The developer wants to
re-learn the old code before designing, and to read the design doc stage by stage as the work goes.

### What the survey found (2026-10-23)

**Who uses which intersection code:**
- `GeometryIntersect` (`src/maths/GeometryIntersect.{h,cc}`) is the workhorse.
  - It uses thick planes (about 1e-6 rad), tests coincident endpoints first, and prunes with a
    bounding tree.
  - It sorts intersections along both geometries, in four types (`SEGMENTS_CROSS`, plus three
    "start on start/segment" touch types).
  - Callers: `PolylineIntersections`, topology resolving (`TopologyIntersections.cc`,
    `ResolvedSubSegmentRangeInSection.cc`), and `GeometryCrossing` (none of the latter's code is
    called).
- `PolylineIntersections` (`src/maths/PolylineIntersections.{h,cc}`) wraps it.
  - For polygons it uses **exterior rings only** (the interior-ring flag is passed as `false`).
  - It doesn't wrap the cut pieces round a ring.
  - Callers: `PolygonPartitioner`, and `GenerateVelocityDomainCitcoms`.
- `PolygonPartitioner` (`src/maths/PolygonPartitioner.{h,cc}`) is where a polygon becomes
  polylines: `GeometryPartitioner::visit_polygon_on_sphere`, the `GEOMETRY_INTERSECTING` branch
  (~cc:168).
  - It classifies each piece with a local "bend" test.
  - `GeometryCookieCutter` (`src/app-logic/GeometryCookieCutter.cc:379-443`) cascades plates.
    Whatever lies outside plate N is fed to plate N+1, so after the first cut every later plate
    sees only polylines.
- `GreatCircleArc::intersect` (`GreatCircleArc.cc:858`) has exact sign tests.
  - It interpolates along the better-conditioned arc, and checks the point lies in both lunes
    (from #104/#106).
  - It is used **only** by `minimum_distance(arc, arc)`.
  - GeometryIntersect lacks its last two refinements.
  - It has no touch or overlap classification, which partitioning needs.
- `DateLineWrapper` (`src/maths/DateLineWrapper.{h,cc}`) is a real Greiner-Hormann clipper. It
  uses its own thick-dateline math, not GeometryIntersect. Shortcuts that hold only for the
  dateline:
  - two vertices per crossing;
  - entry/exit read from the longitude sign;
  - inside/outside of the clip polygon from point-in-polygon at a pole, assumed arbitrarily when
    both poles are touched;
  - a clipped hole always merges into an exterior;
  - a ring that doesn't cross is grouped by `minimum_distance == 0` (the epsilon test already in
    the roadmap).

  Parts that generalise: linked lists with twin links, a shared graph for all rings, the phase-2
  toggle walk, and the per-point provenance (which `PartitionFeatureUtils.cc:182,487` asks for).
- `GeometryCrossing` (`src/maths/GeometryCrossing.{h,cc}`) is on main as an unused stub.
  - The branch's "second cut" adds overloads, plus a `find_vertex_crossing` skeleton with empty
    branches, no return, and undefined `next`/`prev`.
  - Only the intent is worth keeping: reduce the touch/overlap intersections to true crossings.
- **Partitioning features with scalar coverages** (`src/app-logic/PartitionFeatureUtils.cc`). It
  partitions features (Assign Plate IDs, `partition_features`) and carries each scalar coverage
  over to the partitioned geometries. It deals only with the polygon *exterior*:
  - the coverage's domain is the exterior points only (`:213`, `:237`, `:438`), because
    `ScalarCoverageFeatureProperties.cc:223,274` extracts it that way (TODO at `:235`);
  - a partitioned point is mapped back to its original index through an epsilon
    `std::map` (`:180-194`), which can merge two close points;
  - a new point (an intersection) gets its value from the nearest original segment, searched
    within 0.5 degrees and interpolated (`:473-555`). That relies on every new point lying on
    the input's outline, which is true only while the output is polylines (TODO `:488-492`).

  Polygon output breaks that assumption. A partitioned polygon's ring follows the
  partitioner's boundary *across the input's interior*, and those points have no coverage value
  at all. The exterior-only coverage assumption also reaches well beyond partitioning:
  `get_geometry_exterior_points` is used in `ReconstructedScalarCoverage.cc`,
  `TopologyReconstruct.cc`, the `RenderedColoured*OnSphere.h` rendering, and elsewhere.
- **Tests:** no C++ tests for GeometryIntersect, PolylineIntersections, PolygonPartitioner or
  GeometryCookieCutter. The pyGPlates partition tests assert today's polylines ("will be fixed in
  future").

**The papers** (read; both are text PDFs):
- Greiner-Hormann 1998:
  - two linked copies of each intersection;
  - entry/exit by a point-in-polygon test at a start vertex, then toggling at each intersection;
  - even-odd interior.
- Liu et al. 2007: one shared intersection node; holes as oppositely oriented rings kept in a
  separate list; entry/exit computed once and then assumed to alternate.
- Both handle degeneracies only by **perturbation**.
- Also relevant, and in the same folder:
  - Schettino 1999, "Polygon intersections in spherical topology: application to plate
    tectonics", which is spherical and about plates;
  - "Arrangements of Geodesic Arcs on the Sphere" (CGAL).
- The modern answer to degeneracies in Greiner-Hormann: Foster, Hormann & Popa 2019, "Clipping
  simple polygons with degenerate intersections". The developer added it to the same folder as
  `Clipping_Simple_Polygons_with_Degenerate_Intersect.pdf`.
  - It labels each intersection as crossing or bouncing, and delayed crossing or bouncing for
    overlapping chains. That is what GeometryCrossing set out to do.
  - It cites Kim & Kim's alternative, which classifies the midpoints of the edges next to each
    intersection as inside or outside. That is close to candidate (b) below. FHP reject it as
    costly.
  - Its title restricts it to *simple* polygons, so whether it covers self-crossing rings (which
    #107 allows) is for Phase 2 to establish.

**Two consequences that shape the design:**
1. #107's interior is a parity: a point's winding number differs by an odd number from the
   ring's rounded mean, combined by parity across rings. Toggling at each crossing from the cached
   outside point is therefore exactly Greiner-Hormann's phase 2, so partitioning can agree with
   `is_point_in_polygon` *by construction*. Every output piece is a subset of the input polygon,
   so the smaller-side rule holds for the output too.
2. **The plate cascade makes overlapping boundaries the normal case, not an edge case.** A piece
   cut by plate N has edges lying on the boundary that plate N shares with plate N+1. So handling
   overlap (collinear shared edges) robustly is mandatory, and perturbation is not acceptable.

## Decisions (developer, 2026-10-23)

- **A new long-lived branch, fresh off `main`, in the `polygon-intersections` named worktree.** The
  old 2021 branch is archived as `archive/polygon-intersections-2021` on `private` (its only
  content is the GeometryCrossing "second cut", tip `6d6935893`), and the new branch keeps the
  name `feature/polygon-intersections`, so the worktree stays named after its branch.
  - Its plan is this file, `docs/plans/polygon-intersections/PLAN.md`.
- **Two PRs.**
  - PR A: the intersection audit and fixes, plus the design-doc sections on current state.
  - PR B: polygon partitioning and its consumers.
  - The plan travels with both. PR A distils what it completes; PR B deletes the plan.
- **The design doc extends `docs/design/architecture/foundation/spherical-geometry.md`:**
  - new §4 Intersection and §5 Partitioning;
  - Testing and Deferred work renumbered;
  - retitled (for example "Distance, point-in-polygon, intersection and partitioning on the
    sphere").

  Sections describing **existing** code are written as each stage completes, which is what the
  developer reads before each stage. The **new** design lives in the plan until it is implemented
  (AGENTS.md: a design doc must not describe a moving target).

## Phases

The order changes from the developer's first sketch: requirements come **before** the audit. Then
the audit asks "does GeometryIntersect give partitioning what it needs?" rather than auditing in
the abstract. This also settles the GeometryIntersect-versus-GreatCircleArc question with the
requirements in hand, as the developer anticipated.

### Phase 0: set-up (Opus) — done 2026-10-23
- Archive the old branch, create the new one, and move this plan into
  `docs/plans/polygon-intersections/PLAN.md`.
- Configure both build trees.
- Update the roadmap and the 1.1 checklist pointer, and push to `private`.

### Phase 1: map of the current intersection code (Opus writes, Fable claim-checks)
- Doc §4.1 to §4.3:
  - the call graph (GeometryIntersect, then PolylineIntersections, then PolygonPartitioner and
    GeometryCookieCutter; topology resolving; GreatCircleArc::intersect for distance;
    DateLineWrapper on its own);
  - GeometryIntersect's algorithm: thick planes, ordering of coincident endpoints, the four types,
    end-point suppression, the bounding tree;
  - where a polygon becomes polylines today, and why: exterior rings only, the bend test, the
    cascade.
- Built from this survey. Every statement is checked against the code by a Fable claim check, as
  for the area pages.
- **The developer reads it, then the code.**

### Phase 2: DateLineWrapper, the papers, and requirements (Fable)
- A Fable agent reads:
  - DateLineWrapper in full;
  - Greiner-Hormann, Liu, Schettino 1999, and Foster-Hormann-Popa 2019 (all now in
    `C:\Users\jcann\Development\Usyd\documentation\Papers\Graphics\Intersection\`);
  - the GeometryCrossing stub.
- It writes:
  1. **Doc §5.2, "Clipping by DateLineWrapper"**: how its Greiner-Hormann works, and which
     shortcuts are dateline-only. This describes existing code, so it is fine for the design doc.
  2. **Requirements for the intersection primitive**, in the plan. Partitioning needs:
     - crossing, touch and overlap classified consistently along *both* geometries;
     - a shared vertex reported once;
     - an overlap reported as a chain, with its endpoints;
     - ordering along each ring, including interior rings;
     - consistency with point-in-polygon's on-outline threshold;
     - behaviour on zero-length and near-antipodal arcs.
  3. **Two candidate algorithms**, assessed against the cascade's overlap problem:
     - (a) Greiner-Hormann tracing with Foster-Hormann-Popa labelling;
     - (b) an overlay/arrangement: split both boundaries at every intersection, classify each
       sub-edge on its own (inside, outside, shared same-direction, shared opposite-direction)
       by its midpoint's parity and the direction of any overlap, then link the kept sub-edges
       into rings.

     **My lean is (b).** Greiner-Hormann's alternation spreads one misclassified crossing to every
     later piece, whereas (b) confines an error to one sub-edge, and it handles holes and
     self-crossing rings uniformly. Fable should argue the case either way. Both share
     Greiner-Hormann's start: the parity from the outside point.
- **Checkpoint:** the developer reads §5.2 and the candidates (and the papers), and chooses.

### Phase 3: intersection audit and fixes (Fable audit; Opus fixes) → PR A
- A Fable audit of GeometryIntersect against the phase 2 requirements, and against
  `GreatCircleArc::intersect`:
  - the exact signs versus the thick-plane decision;
  - interpolation along the better-conditioned arc;
  - the check that the point lies in both lunes;
  - the tunnelling argument (GeometryIntersect.cc:479-535);
  - the end-point suppression;
  - pruning soundness in the bounding tree.

  Also `PointInPolygon.cc:173`'s own arc-plane intersection, for consistency.
- Decide and record which primitive partitioning uses. Probably GeometryIntersect, given the
  touch/overlap types, with GreatCircleArc's two refinements ported. Then decide whether
  `GreatCircleArc::intersect` stays separate for distance (its needs differ: a crossing is a
  crossing) or is unified. Delete the dead `arcs_are_near_each_other`, the arc-equivalence
  predicates and `SmallCircle::intersection` if confirmed unused.
- A brute-force hunt against an exact reference. Extend #106's exact-predicate tool
  (`gplates-planning/plans/point-in-polygon-tools/pip_ref.py`) to arc-arc intersection, with
  degenerate generators: shared vertices, collinear overlaps, near-parallel planes, near-antipodal
  arcs, and ring edges cut and re-fed (as the cascade does).
- Add C++ GoogleTest coverage for GeometryIntersect. There is none today. Follow the `cpp-test`
  skill.
- Doc §4.4 to §4.5: the tolerances of intersection within §1's system, and the decisions with
  their rejected alternatives.
- **PR A**: fixes, tests, doc §4 and §5.1 to §5.2, and changelog entries if anything visible changes
  (topology resolving uses GeometryIntersect, so a fix there can be visible in both products).

### Phase 4: partitioning design (Fable designs; Opus reviews against the code)
- A detailed design of the chosen algorithm. It goes in the plan, not the doc.
  - Data structures, with provenance per point.
  - Holes in both polygons.
  - Rings that don't intersect. These are classified by point-in-polygon, never by
    `minimum_distance == 0`.
  - Grouping output rings into polygons, by containment or parity.
  - Self-crossing input.
  - Inside **and** outside output, both of which partitioning needs.
  - Polylines through the same machinery, so `PolylineIntersections` and the bend test can go.
  - The cascade in GeometryCookieCutter, which must now pass polygons on.
  - The `partition()` return contract in both products.
- **Provenance for every output point**, on every ring (exterior and interior), in the style of
  `DateLineWrapper::LatLonPolygon::InterpolateOriginalSegment`. Each point is one of:
  - an original vertex, with its ring and index;
  - on an original segment, with its ring, segment and interpolation ratio;
  - on the partitioner's boundary inside the input.

  This replaces `PartitionFeatureUtils`' epsilon map and its nearest-segment search.
- **Scalar coverages on partitioned polygons.** These decisions are for the developer at the
  checkpoint:
  - **A value for the points on the partitioner's boundary**, which lie inside the input where
    the coverage has no value. Some options: interpolate along the cut between the values at its
    two end intersections; no value (and drop the coverage); nearest point on the outline.
  - **Coverages on interior rings.** Today a coverage's domain is the exterior points only, and
    that assumption runs through `ScalarCoverageFeatureProperties`,
    `ReconstructedScalarCoverage`, `TopologyReconstruct` and the coloured rendering.
    - At minimum, partitioning must handle input with holes. A polygon whose coverage covers only
      its exterior gives its pieces values on the exterior-derived points only.
    - The developer said interior rings need handling, coverages included. Phase 4 surveys every
      exterior-only coverage site and proposes the scope: this PR, or a follow-up.
- **Every exterior-only path in the partition code is moved to all rings.** That covers
  `PartitionFeatureUtils` (`:213`, `:237`, `:438`), `PolygonPartitioner.cc:484` (the inside test
  for a polygon that doesn't intersect: one exterior vertex, which misses a partitioner hole
  lying inside the input), and the cookie cutter's handling of networks' rigid-block holes
  (`GeometryCookieCutter.cc:563-568`).
- Decide where it lives: a new `src/maths/PolygonClipping` (or similar) replacing the internals
  of `PolygonPartitioner`. Decide `GeometryCrossing`'s fate: absorbed or deleted.
- **Checkpoint:** the developer approves the design, the coverage policy and the coverage scope.

### Phase 5: implementation (Opus implements; Fable reviews each major commit)
- Build it in the order of the design, with GoogleTest unit tests per stage.
- **The oracle** needs no new exact reference, because point-in-polygon was audited in
  #106/#107. For random points `p`, `p` is inside an inside piece exactly when it is inside both
  the input and the partitioner, and the pieces are disjoint. Also, the areas of the inside and
  outside pieces sum to the input's area (with the parity-area caveat for crossing rings).
- A Fable adversarial hunt for degenerate inputs: shared edges, cascaded plates from a real
  topology model, holes that touch the boundary, self-crossing rings, polygons near antipodal.
- Switch the consumers:
  - `PolygonPartitioner`/`GeometryCookieCutter` emit polygons;
  - `PolygonOnSphere.partition` and `PlatePartitioner.partition_geometry` lose their warnings
    (docstrings via the `api-docstring` skill; regenerate the stub);
  - `PartitionFeatureUtils`: provenance-driven mapping of coverages, all rings, and the
    coverage policy agreed in Phase 4. Test: partition a feature that has a scalar coverage, and
    check the values at original vertices, at intersections, and along cuts;
  - `GLReconstructedStaticPolygonMeshes.cc:917-964` (classification only);
  - the pyGPlates tests that assert polylines.
- Optional, with the developer's say-so: DateLineWrapper's ring grouping moved to the new
  containment code, which closes the roadmap's epsilon "touching" item.

### Phase 6: finish → PR B
- Write doc §5.3 onwards from the implemented code.
- Changelog entries in both products: Assign Plate IDs (GPlates) and
  `partition`/`PlatePartitioner` (pyGPlates).
- The developer's visual check of Assign Plate IDs on polygons in GPlates. There is no rendering
  coverage.
- **A Fable distillation of `docs/plans/polygon-intersections/`** (per CLAUDE.md, with a
  self-contained prompt and every statement checked against the code), then delete the plan.
- The next checklist item, *Audit the plate partitioning*, follows. Also the roadmap's "revisit
  the exact parity area", which this PR's ring-intersection code unblocks.

## Who does what

| Work | Model | Why |
|---|---|---|
| Survey, doc §4.1 to §4.3 drafting, set-up, fixes, implementation, tests | Opus (session) | mechanical or code-heavy work, checked by tests |
| Claim checks on every doc section | Fable | errors in the developer's reading material compound |
| Papers, DateLineWrapper, requirements, candidate algorithms | Fable | the hardest reasoning in the project, and where the choices are made |
| Intersection audit; adversarial degenerate-case hunts | Fable | as for #104 and #106 |
| Partitioning design | Fable, reviewed by Opus against the code | |
| Commit reviews in Phase 5; final distillation | Fable | CLAUDE.md requires Fable for distillation |

Fable agents run one at a time, with a pause for the developer to check the 5-hour limit between
them (memory: fable-one-page-at-a-time).

## Verification
- `ctest` for both products, on both build trees (Windows), at every PR. A Linux Docker run before
  each PR (the `gplates-local.md` recipe).
- PR A: the brute-force arc-arc hunt shows 0 mismatches against the exact reference, and the new
  GeometryIntersect GoogleTests pass. Topology-resolving output is unchanged on a real model (diff
  the resolved boundaries before and after).
- PR B:
  - the point-in-polygon oracle and the area-sum oracle hold over the hunt;
  - the updated pyGPlates partition tests expect polygons;
  - a partitioned feature with a scalar coverage has values at original vertices, at
    intersections and along cuts, as the agreed policy says (`partition_features` and Assign Plate
    IDs);
  - `pygplates-stub-test` passes;
  - the developer's GPlates visual check.
