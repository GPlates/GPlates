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
- 2026-10-23: Phase 2 written by Fable.
  - Doc §5.2, "Clipping by DateLineWrapper", plus two §1 table rows. Reviewed by Opus; one fix
    (the ring-grouping test also takes a ring that *contains* the exterior). Its own Fable claim
    check found 19 issues, all applied (2026-10-23).
  - `phase2-requirements.md`: the papers, requirements R1 to R12, the candidates and 11 open
    questions. It recommends **(b), the overlay**: split both geometries at every intersection,
    classify each piece by one threshold-off `is_point_in_polygon` of its longest arc's midpoint,
    classify common chains by a "sector rule" at a chain end, and link the kept pieces by
    rotational pairing at vertices, with local consistency checks.
  - New gap found (R7): a vertex `1.1e-6` to `1.3e-6` rad off an edge is "on the outline" for
    point-in-polygon, but `GeometryIntersect` finds no contact (thickness `1e-6` against
    `s = 1.414e-6`).
  - **Checkpoint:** the developer reads §5.2, the requirements file and the papers, chooses the
    algorithm, and answers its section 4 questions.

- 2026-10-25: the developer's questions at the checkpoint, before choosing.
  - `P`/`Q` are named the other way round from Foster-Hormann-Popa's section 2. Kept; a note in
    `phase2-requirements.md` says so.
  - A twist of `P` lying on `Q` (Foster-Hormann-Popa's one failure). The overlay handles it only if
    the two records at the twist become one vertex, which the requirements did not say. Added:
    *contract pieces shorter than the tolerance* (requirements §3.3; also the proposed answer to
    question 4), and recommendation reason 5.
  - **Decided: support union and difference as well as intersection.** The overlay's keep-rule
    generalises (requirements §3.3, "Union and the other Boolean operations"). New pyGPlates
    methods on `PolygonOnSphere`, in a PR C (see *Decisions*).
  - Splitting a self-crossing polygon into simple ones: the same machinery (requirements §3.7), a
    possible PR after B.
  - Three inbox notes filed: a spatial index for operations on many polygons; multi-polygon and
    multi-polyline properties; the simple-polygon split.

- 2026-10-25: **Phase 3 audit done early, by Fable** (`phase3-audit.md`; scripts in the planning
  repository's `plans/polygon-intersection-tools/`), assuming the recommended answers except Q1
  and Q9. Not yet reviewed by the developer. Main findings: misordered touch records near a
  segment start (D1), the R7 gap (D2; recommends raising the thickness to `s`), duplicate records
  in a corner's wedge (D3), short-segment touches up to `2t` apart, so contraction needs `>= 2t`
  (D4), unreachable chord interpolation, so porting `GreatCircleArc::intersect`'s refinements is
  moot (D5). Q9: derive chains in the partitioner. Its section 7 lists corrections to the design
  doc and the requirements, not yet applied.

- 2026-10-26: the walk-through so far is written up in **`overlay-walkthrough.md`** (the
  developer's request: the expanded explanations, to become the basis of doc §5.3 onwards, with
  requirements §3.3 as the overview). Corrections from it, applied to the requirements:
  - test a piece at an original vertex, not the longest arc's midpoint, which can lie within
    rounding of `Q` (the developer's T-intersection example);
  - the linking needs no direction; pairing can pinch a ring, and the split is a cut of the
    ring's vertex sequence at a repeated vertex.

  Decided: the sector rule is primary (Q3, partly); always split pinched rings (Q11). Proposed for
  the anchorless fallback: the developer's idea, crossings from `R` leaving out the chain's own arc.

- 2026-10-28: **Q1 decided: raise the thickness to `s`** (developer; for the R7 agreement with
  point-in-polygon and one meaning of "the same point"). **Q9 decided: derive chains in the
  partitioner**; the `Graph` is unchanged apart from F2 and F6. The audit's claim that `s` also
  removes the right-angle corner wedge (D3) was wrong and is corrected in `phase3-audit.md`: the
  thickness and the coincidence radius are one constant, so the wedge scales with it. F3 remains
  the fix at every angle; the third (SL) record of the right-angle case is left to contraction.

- 2026-10-28: **Tolerances in the overlay reopened** (the developer). Found while discussing D3,
  not yet written into `overlay-walkthrough.md` (its section 7 is under review):
  - The wedge's two records both sit at `P`'s vertex `x`, so they are one overlay vertex by
    identity, not by distance. Treating them as two vertices with a zero-length piece between them
    is wrong when `P` bends at `x`: the piece's direction is fictitious, and the result depends on
    the unspecified tie order (one order passes the checks, the other fails; worked by hand).
  - A twist near `Q` never produces a record at `X`, which is no vertex of either polygon. The two
    crossings stay two LL vertices, converge as `Q`'s edge rises, coincide within rounding, and swap
    order along `Q`. Kept as two vertices with a zero-length piece of `Q` between them, each is an
    ordinary crossing: the piece's direction is `Q`'s plane and its class is deduced. Either tie
    order gives a valid output. So section 7.1's two reasons for contracting (no test point, no
    direction) fail for the twist, and 7.2's "without contraction the walk fails" is wrong.
  - At a vertex where each polygon passes once, nothing needs the rotational order: `P` crosses
    iff its two pieces differ in class (the check: so do `Q`'s), a missing class follows, and
    pairing is by the sector memberships (crossing) or same-polygon (bounce, with Q11's split).
  - What is left for contraction: clusters of distinct vertices within about `2s`, where the
    records encode a snapped topology and the planes' directions describe the unsnapped one (the
    right-angle wedge with its SL record; D4 when not a common chain).
  - Proposed principle: never drop a record (withdraw F3); vertices are identities; nothing merged
    by distance in the core; if anything is merged, at linking, as a list of member vertices;
    output clean-up (zero-area rings, repeated positions) last. Proposed investigation: a Python
    prototype of the overlay core on `arc_ref.py`, with a point-sampling oracle, run on a catalogue
    of cluster configurations in both tie orders and on a random cluster generator.

- 2026-10-29: **Phase 4 split into 4a (the overlay core, by a Python prototype and hunt) and 4b
  (the written design)**, developer agreed; see *Phases*. Next: the Phase 4a Fable brief, launched
  after the developer checks the usage limit. PR A's F1, F2, F4, F5 and F7 do not depend on it;
  F3 is to be withdrawn; F6 waits for 4a.

- 2026-10-29: **Phase 4a run 1 done** (Fable, high effort; `phase4a-run1.md`, tools in the planning
  repository at 82fccd6). 463 catalogue cases, every tie order, against the oracle. Clean (0 oracle,
  0 check, 0 repairs): V0/free (no grouping, no merge), V1b/free (identity vertices, a loop arc
  internal, no contraction by distance; added in review), V2b/free (contraction at `2h` plus
  same-cluster arcs). Failing: V1 (identity alone: two SS at one vertex make a loop arc), V2 (a
  cluster wider than `2h`), and the rotational order somewhere under every variant but V2b. The
  geometry-free rule at four-end vertices is what every clean variant has in common. Refuted: the
  hand claim that V0 fails when `P` bends at `x`; the tangent-ray claim outside common chains (a
  narrow neck gives exactly parallel rays at one six-end vertex). Inverting every midpoint test
  changes nothing for V0 or V1b. Run 2: compare V0/free, V1b/free and V2b/free under the random
  generator and perturbations, prefer the simplest that survives. **`phase4a-variants.md`**
  explains the variants (identity vertices, the loop rule, clusters, zero area, midpoint tests,
  slivers, six-end vertices), from the developer's questions on run 1; it feeds walk-through
  section 7 and the design doc's rejected alternatives once a variant is chosen.

- 2026-10-29: **Phase 4a run 2 done** (Fable, high effort; `phase4a-run2.md`, with the review's
  addendum as section 12; `phase4a-section7-draft.md`; tools at 115471b in the planning
  repository). All three run-1 candidates broke somewhere. **Recommended: V1b** (identity
  vertices, loop rule, plus the spike rule, its two-arc case) **with rule `freex`** (at a four-end
  vertex that is one LL record the crossing is known, so one class fixes the other three) **and
  midpoint tests ordered by margin**; option 3 for anchorless chains (Q3), with the path kept on
  `R`'s side. V2b rejected: fails 19 of 300 `six`, 3 of 60 `chain`, 18 of 299 `shared`, in exact
  arithmetic, at six-end clusters; moves output by up to `20h`. V0 rejected: zero-length pieces,
  most midpoint tests, tie-dependent output. Float ordering never mattered. Residue: V1b fails 4
  of 299 dense jittered shared edges (2 with the spike rule) and one six-end case, at six-end
  identity vertices whose rays run together within `h`; a **bundle rule** there is the first open
  item for Phase 4b. Cascade clean with no growth; the open-arc cut works with three additions.
  Awaiting the developer's decision on the variant.

- 2026-10-30: **Decided (developer): the overlay core is V1b with the spike rule, the `LL` rule
  (`freex`) and margin-ordered midpoint tests; Q3's fallback is option 3** (crossings from `R`,
  path on `R`'s side). The bundle rule is Phase 4b's first design item. `phase4a-variants.md`
  sections 10 and 11 explain the chosen configuration and run 2's corrections. The section 7
  draft gained the twist on `Q` as a worked sector example (7.8), a fuller account of the `LL`
  rule (7.4), and a corrected 7.7 (identity vertices also need the bundle rule, but contraction
  creates six-end vertices where identity has four-end ones); it waits for the developer's read
  before replacing walk-through section 7.

- 2026-10-30: **Walk-through section 7 replaced** by the draft (developer approved; the draft file
  is deleted). The rest of the walk-through was checked against runs 1 and 2 and corrected:
  overview step 3 (deduction), thickness wording and the T-intersection's variable (`h` is now the
  thickness), option 3 decided with the side rule (section 5), the 6.4 example now paired
  directly by the bounce rule, the `LL` bounce check and the all-vertices debug check (9.1),
  repair's measured role (9.3), inherited twists as the OGC exceptions (11), and polylines
  (deduction across crossings; not yet run by the prototype). `phase2-requirements.md` section 3.3
  and questions 3 and 4 updated to match.

- **Resume here** (2026-10-27):
  - Phases 0 to 2 are done and claim-checked. **Algorithm chosen: (b), the overlay** (developer,
    2026-10-25). The walk-through of requirements §3.3 is complete (`overlay-walkthrough.md`,
    sections 1 to 15); the developer reads it, then answers the 11 questions in
    `phase2-requirements.md` §4 (Q3 partly and Q11 already decided). Only **Q1** (thickness `t` or
    `s`) and **Q9** (derive chains in the partitioner) bear on PR A; the rest are for PR B.
  - **What is left for PR A**, once Q1 and Q9 are answered:
    1. The developer reviews `phase3-audit.md`.
    2. Apply its section 7 corrections to the design doc (§1, §4.2, §4.3) and the requirements, and
       record the decisions: `GeometryIntersect` is the primitive; `GreatCircleArc::intersect`
       stays separate for distance, with nothing ported (D5).
    3. Fixes F1 to F7 (Opus): F1 the thickness and its comments; F2 order touch records by
       projection; F3 report a corner contact once; F4 delete the dead code and state the
       `sin(phi) > t` invariant; F5 document the record positions; F6 ring structure for consumers;
       F7 `std::stable_sort`.
    4. GoogleTests for `GeometryIntersect` (the audit's 16 cases, §6.1; `cpp-test` skill).
    5. The hunt: extend `plans/polygon-intersection-tools/` in the planning repository (re-fed
       pieces, interior rings, real-model shared edges, tiny and high-latitude geometries; §6.2),
       and run it to zero mismatches after the fixes.
    6. Topology resolving on a real model, before and after: unchanged if Q1 keeps `t`; every
       change under `s` if Q1 raises it, which also needs changelog entries in both products.
    7. Doc §4.4 to §4.5 (the tolerances within §1's system; the decisions and rejected
       alternatives), with a Fable claim check.
    8. Distil what PR A completes; `ctest` on both trees, a Linux Docker run, push to `private`,
       and the PR when the developer asks.
  - `DateLineWrapper` is not checked before the Phase 4 decision.
  - Build trees: `build-gplates` and `build-pygplates` in this worktree, configured and green
    (GPlates 92/92, pyGPlates 7/7).
  - The planning repository's worktree used so far is `gplates-planning-wt/worktree-2`.

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

## Decisions (developer, 2026-10-25)

- **Boolean operations: intersection, union and difference**, not only partitioning's inside and
  outside. `PolygonOnSphere` gains methods for them (names proposed: `intersection`, `union`,
  `difference`, perhaps `symmetric_difference`, as in Python's `set` and Shapely), each taking a
  polygon and returning a list of polygons. **Three PRs now:** the overlay supports every
  operation in PR B, and the methods come in a small **PR C** (API, docstrings, stub, changelogs, a
  union oracle), so that PR B's review stays on partitioning (confirmed by the developer). PR C
  then deletes the plan instead of PR B.
- Not in this branch; filed as `Notes:` entries in the planning repository's `requests/inbox.md`
  (`c4d0055`): a spatial index for operations on many polygons (the union of a sequence, a plate
  model's gaps and overlaps); multi-polygon and multi-polyline geometry properties; and splitting a
  self-crossing polygon into simple polygons (requirements §3.7, a possible PR after B).

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

Split in two (developer, 2026-10-29). The tolerance handling is settled by experiment first, and the
written design follows it.

**Phase 4a: the core, by prototype (Fable).** A Python prototype of the overlay core in the planning
repository's `plans/polygon-intersection-tools/`, on `arc_ref.py`'s exact records: vertices,
pieces, classification, deduction, pairing, linking and the checks. Not a second implementation:
the core and an oracle only.
- **The oracle:** sample points well away from both boundaries; each is in the output exactly when
  `f(in P, in Q)`, with the output read by parity over its rings (no grouping needed).
- **The runs:** a catalogue of configurations with several records at or near one point (wedges,
  a twist near `Q`, narrow necks, touching rings, D4 segments, SS clusters), then a random cluster
  generator (vertices within 0 to `3s` of edges, vertices and each other). Every case in both tie
  orders, and under small perturbations, since the C++'s rounding is not in `arc_ref.py`.
- **The variants:** identity vertices with deduction only; plus contraction at linking, a cluster
  kept as the list of its vertices; possibly a parity solve over a cluster's internal pieces.
- **Decides:** Q4 (contraction, from the winning variant); Q5 (from the repair counts); Q3's
  fallback (options 1 and 3 on identical rings and thin faces); deduce-first or test-first for
  single-arc pieces; Q8 informed by cluster widths in a short cascade; the core data structures
  (vertex, cluster, piece, ray) and so the provenance model; what `GeometryIntersect` must provide
  (feeds PR A's F6). One run cutting by an open arc, for the `DateLineWrapper` decision.
- **Delivers:** the results (which variant has zero oracle failures and zero repairs, and what the
  failures of the others looked like); `overlay-walkthrough.md` section 7 rewritten as "One point,
  several records"; the catalogue and generator, which become Phase 5's GoogleTest cases and the
  start of its hunt; the prototype itself, as the reference to check the C++ against record for
  record, piece for piece and class for class.

**Phase 4b: the written design.** A detailed design of the chosen algorithm, the core translated
from the prototype. It goes in the plan, not the doc.
  - Data structures, with provenance per point.
  - Holes in both polygons.
  - Rings that don't intersect. These are classified by point-in-polygon, never by
    `minimum_distance == 0`.
  - Grouping output rings into polygons, by containment or parity.
  - Self-crossing input, including a twist lying on the other polygon, and every other case of
    several records at or near one point: as Phase 4a decides. (This bullet said "contract pieces
    shorter than the tolerance"; reopened 2026-10-28, see *Status*.)
  - Inside **and** outside output, both of which partitioning needs, and the other Boolean
    operations by the same keep-rule. A union (or symmetric difference) larger than a hemisphere
    must fail, not come back as its complement.
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
- **`DateLineWrapper` as a layer over the overlay?** (decided 2026-10-23 to settle it here.)
  Clipping to the dateline is cutting by one open arc, from pole to pole. In the overlay, that
  means splitting at the intersections, keeping the input's pieces, and keeping the arc's pieces
  that lie inside the input *in both directions*, so they become the front and back edges. Then
  the pieces are linked.
  - The overlay would fix `DateLineWrapper`'s known defects: holes merged into exteriors, the
    both-poles guess, reversed rings at a touch, and the epsilon grouping.
  - What stays: lat/lon output, snapping to ±180/±90, the central meridian, tessellation, and
    provenance.
  - So the design should support cutting by an open arc (pieces used in both directions), not
    only by a polygon.
  - If yes: port `DateLineWrapper` in its own PR after PR B. It is part of the pyGPlates API, so
    it can go in 1.1 if time allows, or after 1.1 (decided 2026-10-23: either is fine, since it has
    long served well as it is). The port changes map display, export and pyGPlates'
    `DateLineWrapper` output: holes stay holes, and rings keep their direction. So it needs the
    developer's visual check and changelog entries in both products.
    - The port PR starts with a brute-force oracle on the current code. Wrap random polygons
      (holes, rings round a pole, touches, vertices on the dateline, non-zero central meridians),
      then check that a random point is inside some output piece exactly when it is inside the
      input, outside a narrow band along the dateline. The same oracle then checks the port.
    - Only the thin layer that stays needs an audit.
  - If no: `DateLineWrapper` stays as it is for 1.1, without an audit (it has served well). A
    later audit, with the same oracle, is the roadmap's call.
- **Checkpoint:** the developer approves the design, with Phase 4a's results behind its core, the
  coverage policy and the coverage scope.

### Phase 5: implementation (Opus implements; Fable reviews each major commit)
- Build it in the order of the design, with GoogleTest unit tests per stage. Phase 4a's catalogue
  supplies many of the cases, and its prototype is the reference the C++ is checked against.
- **The oracle** needs no new exact reference, because point-in-polygon was audited in
  #106/#107. For random points `p`, `p` is inside an inside piece exactly when it is inside both
  the input and the partitioner, and the pieces are disjoint. Also, the areas of the inside and
  outside pieces sum to the input's area (with the parity-area caveat for crossing rings). For
  each Boolean operation `f`, `p` is inside the output exactly when `f(p in P, p in Q)`.
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
- `DateLineWrapper` is not touched in PR B. See Phase 4's decision about it.

### Phase 6: finish → PR B
- Write doc §5.3 onwards from the implemented code.
- Changelog entries in both products: Assign Plate IDs (GPlates) and
  `partition`/`PlatePartitioner` (pyGPlates).
- The developer's visual check of Assign Plate IDs on polygons in GPlates. There is no rendering
  coverage.
- **A Fable distillation of what PR B completed** (per CLAUDE.md, with a self-contained prompt and
  every statement checked against the code).

### PR C: Boolean operations in pyGPlates (Opus)
- `PolygonOnSphere` methods for intersection, union and difference (names in *Decisions*, 2026-10-25),
  on PR B's overlay. Docstrings via the `api-docstring` skill; regenerate the stub; changelog.
- The oracle for every operation (Phase 5), and the error for a union over a hemisphere.
- The final Fable distillation of `docs/plans/polygon-intersections/`, then delete the plan.
- The next checklist item, *Audit the plate partitioning*, follows. Also the roadmap's "revisit
  the exact parity area", which this PR's ring-intersection code unblocks.

## Who does what

| Work | Model | Why |
|---|---|---|
| Survey, doc §4.1 to §4.3 drafting, set-up, fixes, implementation, tests | Opus (session) | mechanical or code-heavy work, checked by tests |
| Claim checks on every doc section | Fable | errors in the developer's reading material compound |
| Papers, DateLineWrapper, requirements, candidate algorithms | Fable | the hardest reasoning in the project, and where the choices are made |
| Intersection audit; adversarial degenerate-case hunts | Fable | as for #104 and #106 |
| Overlay-core prototype and its hunt (Phase 4a) | Fable | the tolerance handling, where hand analysis has gone wrong twice |
| Partitioning design (Phase 4b) | Fable, reviewed by Opus against the code | |
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
