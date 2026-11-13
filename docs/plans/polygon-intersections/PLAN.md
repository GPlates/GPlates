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

- 2026-10-30: **Phase 4a finished.** The Fable claim check of `overlay-walkthrough.md` found 21
  issues (3 high: the `LL` rule fixes only its own polygon's other class; a chained identity group's
  rays leave from several positions; same-polygon pairing at a bounce is the split pairing), all
  applied, with the `LL` correction carried into `phase4a-variants.md`, `phase4a-run2.md` and the
  prototype. Next: PR A, starting with the fixes (step 3 below).

- 2026-10-30: **PR A's fixes done** (F7 f076e6ec9, F4 baf701134, F1 39aac6ba7, F2 72a69a0b4, F5
  ddf997c35, F6 9c20957e2), both test suites green (pyGPlates 7/7, GPlates 92/92). F6 is the
  documentation on `Graph` (ring structure, and which vertices each record names); the
  `PolygonOnSphere` helper waits for its first caller in PR B. **Step 6 done**: topology resolving
  before (the build at 6afb8aa76, kept in the session scratchpad) and after, on Muller et al. 2022
  every 5 Myr from 1000 Ma (`topology_diff.py` in the planning repository): 3 of 3799 resolved
  topologies changed, each a junction moving onto an original vertex by 1.01 to 1.08 s; changelog
  entries added in both products with F1. **Step 2 done**: the audit's section 7 corrections are in
  the foundation page (4 commits, describing the code after the fixes) and in the requirements.
  Left for PR A: steps 4 (GoogleTests), 5 (the hunt), 7 (doc 4.4 to 4.5) and 8.
- 2026-10-30: **Step 4 done**: `src/unit-test/GeometryIntersectTest.cc`, the audit's 16 cases, all
  passing (GPlates 108/108). Reverting F2 fails the ordering case and the touch-key check. Left for
  PR A: steps 5, 7 and 8.
- 2026-10-30: **Step 5 done** (planning repository 924eb4e): the hunt against the fixed build, with
  `arc_ref.configure('fixed')` and four new generators (`tiny_polar`, `long_edges`, `refeed`,
  `shared_real` from the Muller et al. 2022 boundaries). 8000 polyline and 6000 ring cases, 36559
  records, **no mismatch**; the old code modelled gives 319 mismatches in 2000 cases. Three
  corrections to the observation, none to the C++: several records at an end vertex, ties below
  the cosine resolution near a segment's start (about 1.5e-8: real-data near-ties sort in
  discovery order, which the overlay's tie handling must allow), and recurring positions
  (unobservable, skipped). Interior rings are not observable through `partition()` (it intersects
  exterior rings only); the GoogleTests cover them, and PR B's polygon output will make them
  observable. Left for PR A: steps 7 and 8.
- 2026-10-30: **Step 7 done**: `spherical-geometry.md` section 4.4 (tolerances) and 4.5 (rejected
  alternatives), and section 6's account of the GoogleTests and the hunt (0903c5b28); the Fable
  claim check (1 high, 5 medium, 6 low) applied. Left for PR A: step 8 (distil, Linux Docker run,
  push, the PR when the developer asks).

- 2026-10-30: **PR A distilled** (Fable; the foundation page and code comments). The map of what
  went where, and the list of plan content now fully distilled for PR A, is
  **`prA-distillation.md`** beside this file: all of `phase3-audit.md` as it bears on PR A, and
  PLAN's Context, Phase 3 and PR A status entries. The requirements' Phase 4a amendments, the
  walk-through and the `phase4a-*` files are PR B's to distil.

- 2026-10-31: **PR A code review** (Fable, high effort, given the diff and scope but not the
  hunt results or the distillation's claims; report `prA-review.md` beside this file). No defect
  in the production code; it independently confirmed that no consumer uses the angle fields as a
  distance. Two test gaps, both fixed: the "zero-length segment at a vertex" was 1.2e-6 radians
  long, above the zero-length bound (shrunk, and both zero-length cases now assert it); and no
  test covered the bounding circles' expansion (all 16 passed with it set to zero), so the
  pruning test gained an end-to-end case that fails without it.

- 2026-10-31: **Linux Docker run** (Ubuntu 22.04, GCC 11, Qt 5.15, at b9131a7db): GPlates 108 of
  108; pyGPlates 6 of 7, the failure `pygplates-source-closure-test` reporting the dependency
  matrix stale since the GoogleTests commit (unit-test includes of maths headers). Regenerated
  (a doc-only, platform-independent change); the Windows pyGPlates suite then passes 7 of 7.

- 2026-10-31: **PR A opened as GPlates/GPlates#108** (base `main`, from this branch at
  `385a679c6`; branch pushed to `public` and `private`). Later commits go to `private` only until
  it merges, so as not to change the PR.

- 2026-10-31: **PR A merged** as #108 (merge `6b6472850`, CI green on the first run); `main`
  merged back into this branch. What PR A completed is distilled (`prA-distillation.md`).

- 2026-10-31: **Phase 4b started**, split into four steps (see *Phases*): prototype run 3 (the
  bundle rule, and polylines), a consumer survey, the written design, and its review.
- 2026-10-31: **Step 2 done**: `phase4b-survey.md` (Opus). Beyond doc 5.1: provenance must compose
  through the cascade; a result-only path for `GLReconstructedStaticPolygonMeshes`;
  `PolylineIntersections` has one other caller (CitcomS, one crossing). Found: pyGPlates cannot make
  a readable coverage on a polygon with holes (measured: writes count all rings, reads count the
  exterior); pyGPlates' `partition_features` drops coverages; `partition_range` reads one past the
  end for an intersection on a polygon's closing segment; "most overlapping" sums arc length, which
  polygon pieces inflate by their cuts. Eight checkpoint questions end the file (5 and 6 wait for
  run 3).
- 2026-10-31: **Decided (developer), survey questions 1 to 3.** (1) A cut point's coverage values
  are interpolated along the cut between its two ends. (2) A polygon coverage covers all its rings,
  as the `src/api` audit already decided (planning repository, `plans/src-api-audit.md`, triage
  G1-1: reader falls back to exterior-only counts in 1.1). That change is G1-1's, not this branch's;
  PR B needs it only by its Phase 5 coverage step, and it is Opus work. Partition-specific coverage
  bugs (the `partition_range` read past the end, pyGPlates' `partition_features` dropping
  coverages) are fixed in PR B. (3) "Most overlapping" uses area for polygons.
  Measured for question 4 (`touch.py` in the session scratchpad): today a polygon that only touches
  the partitioner, from either side, comes back `intersecting` as its outline in polylines (one
  closed polyline, or two where it shares part of an edge), and a polyline touching from outside is
  split in two at the touch.
- 2026-10-31: **Decided (developer), survey questions 4, 7 and 8.** (4) A polyline or polygon whose
  pieces would all have one class is not split: the original geometry goes whole to the inside or
  outside list, and the result is `INSIDE` or `OUTSIDE`; `INTERSECTING` (with pieces) only when
  pieces differ in class. (7) The design keeps run 2's three additions for cutting by an open arc;
  the `DateLineWrapper` port is its own PR after B. (8) The core goes in a new
  `src/maths/PolygonOverlay.{h,cc}`, `PolygonPartitioner` keeps its interface on top, and
  `PolylineIntersections` is deleted. G1-1 and G1-4 go out as their own PR ahead of the `src/api`
  audit's (planning repository `b9c5180`). Questions 5 and 6 wait for run 3.
- 2026-10-31: **Step 1 done: prototype run 3** (Fable, paused at the usage limit with three runs left;
  the session ran them and folded them in; report `phase4a-run3.md`, tools at planning `e3ae846`).
  The residue's mechanism was not parallel rays but short arcs leaving a chained identity group from
  off its output position. **Bundle rule**: each ray gets a cone from its first arc's offset and
  length (no new constant); inseparable rays float, a class is deduced only if every consistent
  placement agrees, the fewest-inversions placement is used for pairing. Also ring-continuity pairing
  where a spike leaves one polygon's ends only. 0 failures in 3013 random polygon cases (run 2's
  residue all cleared), the catalogue, cascade, anchorless and cut runs; clean outputs change only in
  slivers within `0.42h` (12 of 898 `shared`). Polylines: 3395 cases clean, misclassification within
  `1.09h`. Left open: under the fair midpoint-inversion adversary, wrong output in 10 of 1155 dense
  shared-edge cases (repair hits its round cap; run 3's open question 4). Session review: the code
  matches the report's definitions; no defect found. Fable recommends Q2 = `inside` and leaving
  slivers in the output.
- 2026-10-31: **Decided (developer): Q2 = `inside`** (a polyline piece along the partitioner's
  boundary is inside). **Slivers (developer):** an optional output filter (drop rings whose mean width,
  2 area / perimeter, is below a width), in the design; whether to enable it, and the width, decided
  in Phase 5 by measuring thin rings when partitioning real data by a real topology model.
  **Inversion residue:** measured by the session (run 3 report, section 11): of 69 midpoints the
  adversary inverted in the 8 broken cases, 64 lie `0.05h` to `0.8h` from the other boundary, where a
  robust test cannot flip; inverting only the 5 within rounding (`3e-15` rad) breaks nothing. So no
  run 4; the design keeps whole-chain repair as a safety net, and Phase 5 compares classes with the
  prototype.
- 2026-10-31: **Steps 3 and 4 done: `phase4b-design.md`** (Fable, then the session's review, its
  section 15). Notable from the code: the overlay passes the subject first to `GeometryIntersect`;
  option 3 needs a new `PointInPolygon` entry and one outside point per polygon; grouping by parity
  depth, with the output side derived from the keep-rule (not the rotational order); zero-length arcs
  float with a cone of pi; failure is an `InconsistentOverlayException`, never a silent answer; fifteen
  commits for Phase 5. **Checkpoint**: the developer approves the design and answers its section 14
  (output ring orientation; a coverage value for a ring of partitioner points only; failure
  reporting and the Python exception's name; the redundant containment test in
  `GLReconstructedStaticPolygonMeshes`; the coverage form of `partition_geometry`).
  That PR was started 2026-10-31 in worktree-2 (another session). Merge `main` into this branch
  once it lands, before Phase 5's coverage step; expect a small conflict in `PartitionFeatureUtils.cc`
  (it switches the exterior-point calls that PR B rewrites).

- 2026-11-04: **Design approved; section 14 answered (developer).** (1) Output rings have the output
  on their left. (2) A ring of partitioner points only takes its coverage values from the nearest
  point of the input's outline, interpolated along that segment. (3) Python raises `PartitionError`;
  GPlates' Assign Plate IDs catches per feature, leaves it unpartitioned and reports the skipped
  features once. (4) `GLReconstructedStaticPolygonMeshes` keeps its containment test in PR B; the
  Phase 6 visual check decides. (5) PR B carries coverages at the feature level only (the private
  helper `partition_features` calls); whether any geometry-level entry point (`partition_geometry`,
  `PolygonOnSphere.partition`, PR C's operations, `DateLineWrapper.wrap`, `to_tessellated`) gains a
  coverage form is decided in PR C, with some merit seen in keeping coverages at the feature level
  only. **Phase 5 started.**

- 2026-11-04: **Phase 5 steps 1 to 3 done** (`phase4b-design.md` section 12): `src/maths/PolygonOverlay.{h,cc}`
  (GPlates-only until the partitioner includes it), with `PolygonOverlayTest` (23 cases) and
  `PolygonOverlayPrototypeTest`. Step 1 `996d48f29` (vertices, stops, pieces), step 2 `56d2a62f2` (loop
  and spike rules, common pairs, rays), step 3 `da06e8537` (classification and checks). The prototype
  comparison (design 11.2) runs now, a step early: the planning repository's `overlay_export.py` writes
  the catalogue under `bnd` with option 3, and **all 463 cases agree record for record, piece for piece
  and class for class**. Found on the way:
  - `PointOnSphere::operator==` is not exact: it compares the dot product with `real_t`'s `1e-12`, so
    points about `1.4e-6` rad (the thickness) apart compare equal. The overlay compares coordinates
    exactly (the cone's "starts at the output position", and later clean-up's repeated positions).
  - **The tie orders the prototype did not have.** `GeometryIntersect` sorts by a cosine in doubles, so
    records whose cosines agree within a few ulps come out in either order where the prototype's exact
    keys see no tie (`d4 phi=90 lone`: two touches projecting to one point; `twist edge` at `d = -1` and `-4` ulp). The
    exporter groups those as ties; both orders pass.
  - **Point-in-polygon is not exact near a zero-length edge.** `GreatCircleArc` calls an arc shorter
    than about `1e-6` rad zero length (`magSqrd() <= 0` in `real_t`), and `PointInPolygon.cc:367`
    approximates such an edge's crossing by its end point, so the answer within about the edge's
    length of it can be wrong (measured: a point `0.019h` inside `P` called outside at every speed).
    `minimum_distance` is likewise off there (`0.13h` against `0.019h`). A vertex in no record is more
    than `h` from the outline, outside that zone, so only band pieces are affected; but walk-through
    section 3's "exact, a flip needs the input within rounding" is false near such edges. For the design
    doc's tolerances section.
  - **Margins tie at the cosine resolution.** `minimum_distance` resolves about `1e-8` rad (`0.01h`)
    near an outline, so two band pieces' margins can tie and the first in piece order is tested. Both
    are band pieces, as the design argued. A sine-based distance would order them; only if the hunt
    shows it matters.
  - 39 band pieces, all in `d4 hole-between` (13 cases), take the other class from the prototype. The
    comparison accepts a band piece (a single arc within `2h` of the other outline) whatever its
    method, since one deduced from another band piece is arbitrary too. Step 7's output oracle is what
    must confirm these.
  - **Decided (developer):** the prototype, the exporter and the hunt tools stay in the private
    planning repository; the committed test and data README describe the snapshot without naming it
    (`db0322e79`). Moving them was judged not worth it: the trimmed prototype would be a second
    implementation nothing keeps in step, and only the developer runs it.
  - **Fable review 1** of steps 1 to 3 (`phase5-review-1.md`, with the session's check as its section
    4): the comparison never checked a common pair's class (fixed; still 0 differ, and a mutation now
    fails 108 cases), the spike rule wrapped round a polyline (fixed), an unreachable branch (now an
    assertion; design 3.3 corrected), three new unit tests for its test gaps.
  Next: step 4, the bundle rule.

- 2026-11-04: **Phase 5 step 4 done: the bundle rule** (`8b6128884`). With it disabled the C++ fails a
  vertex check on run 2's residue (`shared#28`, `#169`, `six#5` and `#188` at seed 7), as the prototype
  did; with it all are consistent, and the anchored, floating and placement counts are run 3's (section
  4.1), except `shared#234`: run 3's table says 8 placements, the prototype itself gives 4 in all 16 tie
  orders (a slip in the run 3 report). The comparison now also covers 233 committed generated cases
  (`prototype-hunt.txt`: the residue and 30 to 50 of each generator) and any export named by
  `GPLATES_POLYGON_OVERLAY_PROTOTYPE_CASES`; on 5091 uncommitted ones (300 of each generator at seed 1
  with a jittered copy each, and `chain` at seeds 2, 3 and 5) it agrees wherever the prototype is clean.
  A common pair's class may differ only where a band piece's does (`chain#66` seed 2: a 2h common chain
  follows a 1.9h band piece through the sector walk).
  - **New: the chosen configuration fails about 1% of `chain` cases, in the prototype itself.** Runs 2
    and 3 ran `chain` only at seed 5, cases 0 to 59 (its 300-case run never finished). At 300 cases per
    seed: seed 1 `#41`, `#107` (one check failure, which repair finds no flip for), `#182`, `#288` (two
    check failures, repair at its 20-round cap, `O324` oracle failures for `#182`); seed 2 `#28`, `#52`,
    `#119`; seed 3 `#220`, `#249`, `#281`; seed 5 `#108`, `#163`. The C++ reproduces them (it fails the
    same checks); design 9.2 would throw `InconsistentOverlayException` there, never a silent answer,
    but step 15 requires zero failures on generated data. `chain#41` is `ss-chain m=3 edge`, `#182`
    `zigzag-ll m=3 vertex`, no bundle at either. **Needs a prototype investigation (run 4, Fable)**
    before the core is final; whether before step 6 (repair) is the developer's call.

- 2026-11-04: **Phase 4a run 4 done** (Fable, high effort; `phase4a-run4.md`; tools at planning
  `d48d252`). The `chain` failures (19 of 2393 cases over eight seeds under `bnd`) all fail at a vertex
  of six or more ends where **one polygon passes once** and the other two or more times within `h`, and
  the rotational order is trusted where it carries no information: a crossing counted at both ends of a
  band arc (A), a long ray inside a `2h` common ray's `11` to `32` degree direction gap (B; also with a
  simple, non-self-crossing subject), a corner cut off by the spike rule where the other polygon crosses
  the tip (C). **Proposed (`CONFIGS['r4t']`): the one-passage rule** (run 4 section 3.1: at such a
  vertex the order is not consulted; `m_in(b) = m_in(a) xor` the number of the other polygon's rays
  inside, mod 2; the linking order is built from the classes), **and the spike rule only where the
  other polygon passes the tip at most once** (3.3). The bundle rule then applies only where both pass
  more than once. Measured clean over every generator at run 3's sizes, the catalogue (output
  identical to `bnd`), cascade, anchorless and cut; clean outputs change in slivers within `0.52h`.
  **Session check:** the twelve listed cases fail under `bnd` and pass under `r4t` (by the exporter, a
  different code path from the hunt's); on fresh seeds 9 to 12 `bnd` fails one (`chain#30` seed 11,
  `O695` by the hunt's oracle) and `r4t` none; `bnd` re-exports byte-identical to the committed
  snapshots. **Checkpoint:** the developer adopts `r4t` or `r4n` (no spike rule; run 4 open question 1),
  then the design (run 4 section 7) and the C++ change, before step 5.

- 2026-11-04: **Decided (developer): `r4t`** (the one-passage rule and the restricted spike rule; run
  4's recommendation, over `r4n`). The chosen configuration is now `CONFIGS['r4t']`; `phase4b-design.md`
  is amended (its header, 3.4, 3.6, 3.7.1 case 1b, 3.8, 9.1, 11.2, 13). **Phase 5 step 4b done**
  (`8b643ba6e`): the C++ has the rule, the snapshots are re-exported under `r4t` (the catalogue's is
  byte-identical to `bnd`'s), and the comparison agrees in every committed case and in 6291 uncommitted
  ones, including `chain` at seven seeds (no prototype failure left). Within a half at a one-passage
  vertex the linking order keeps the measured relative order of the rays, as the prototype does (run 4
  open question 2: ring structure only). The bundle rule is now reached in about 1 vertex in 3500 cases,
  so its step 4 verification (against `bnd`, before 4b) is what covers it.

- 2026-11-04: **Phase 5 step 5 done: anchorless chains** (option 3). A new
  `PointInPolygon::get_crossing_parity_excluding_segment` (the crossing counter was file-local); each
  polygon's own outside point and a direct arc, rather than the prototype's one reference point and a
  waypoint kept on its side; the second reference point where the outside point is within about `1e-6`
  rad of the chain's great circle. The 48 cases of `overlay_anchorless.py` (three seeds, exported by
  `overlay_export.py --anchorless`, planning `c5d4ad4`) agree with the prototype; inverting the class
  fails 47. No hunt generator produces an anchorless chain (every hunt export had none).

- 2026-11-04: **Phase 5 step 6 done: repair and failure** (`e9b54acac`). Repair as the prototype's
  (first failing vertex, least reliable class by method then length, 20 rounds), plus the design's
  whole-chain candidate (no prototype counterpart), tried before single flips of its rank: the test
  chain needs 6 single flips without it, 1 with it. What repair cannot fix (or an unresolved piece)
  throws `InconsistentOverlayException` with the diagnostics and the failing vertex;
  `Options::max_repair_rounds` and `Options::throw_if_inconsistent` (count instead) are for tests and
  hunts. The prototype comparison now fails a case on any C++ repair (generated data must need none):
  still 0 in all 6291 uncommitted cases. Not yet done from design 9: the debug-only all-vertices
  assertion (counted only), and the debug dump's comparison form (9.3); the partitioner's and the
  products' handling of the exception (steps 11 to 13).

- 2026-11-04: **Fable review 2** of steps 4 to 6 (`phase5-review-2.md`): no defect in the translation.
  Fixed: nothing committed reached the bundle rule after run 4 (15 cases from about 15,900 `shared`
  now committed, their counts pinned); the anchorless loop could overwrite a class and spin on
  inconsistent data; one undirected chain stopped later chains; repair counters accumulated; the cap's
  order. **Open, for step 15 (or a run 5):** three dense shared-edge cases fail in the prototype and the
  C++ alike under `r4t`, all at a bundle vertex (both inputs passing more than once): `shared#6` seed
  11, `#1434` seed 12, `#2587` seed 13 (16 to 20 repairs, 2 vertices left failing; the C++ repairs
  `#6` with the whole-chain candidate). In `#2587` the bundle's single consistent placement deduces
  classes that fail downstream; the review asks whether the cone bound is too tight for a wedge tip
  whose apex is within the band. About 1 in 5000 `shared` cases; design 9.1's "none seen" no longer
  holds. Also noted: the second reference point's fallback is reached by no committed case.

- 2026-11-04: **Phase 5 step 7 done: linking, split, clean-up** (`extract_rings`, the five operations).
  The tests gain the output oracle (design 11.3, `src/unit-test/PolygonOverlayOracle.h`): all 772
  committed and 6291 uncommitted cases pass under all five operations. Parity cannot see pairing, so the
  snapshots also carry the prototype's rings for `and`, `diff`, `or` (planning `4fa0852`), compared where
  the decomposition is unique; rings touching at two vertices split either way depending on the walk's
  start (the prototype's is arbitrary), and are skipped (103 ring sets of about 7000 cases). A pairing
  that is wrong at one vertex but non-crossing is undone by the split (walk-through 6.4), so only the
  symmetric difference's crossing pairing is unguarded by the comparison; `LinkTwoSquares` pins it.
  Found: two distinct vertices can lie within a computed crossing's rounding (6e-16 and 2e-12 rad
  apart), where the prototype's floats coincide and its clean-up drops one; the C++ keeps both (no
  merging by distance), so its output can hold near-zero edges. **For step 14's output filters**: whether
  to drop such points. Deviation from design 9.1: a pairing failure throws `InconsistentOverlayException`
  (counted when not throwing), rather than asserting.

- 2026-11-04: **Phase 5 step 8 done: grouping, orientation, the hemisphere check** (`extract`, output
  polygons). **Design 4.3's per-ring output side is rejected** (amended there, and in section 13): the
  side of each kept piece from its class is wrong for a band piece about `h` long, since the output ring
  runs between the vertices' output positions and not along the piece (the `d4` catalogue cases); a
  length-weighted vote fixed those but not the `tinyhole` hunt's holes, all of whose pieces are band
  pieces (a union of two small polygons then threw as over a hemisphere); and a ring of an input whose
  rings cross has no one side (every twist's union and symmetric difference threw). As built: rings are
  grouped by parity depth (4.2), oriented by their role (`PolygonOrientation`, since `SphericalArea`'s
  signed area is zero for a ring the thickness across: every edge is zero length to `GreatCircleArc`),
  and the hemisphere check is the oracle at one reference point clear of both inputs by `10h` (a
  complement reading flips every point; parity reads crossing inputs right too). Containment found two
  more degenerate shapes (4.2): a ring thinner than `s` along another (the farthest vertex decides) and a
  triangle of three shared vertices (an edge midpoint decides). The oracle now also reads each output
  polygon by parity, and checks every ring's orientation, where no input ring crosses another; most
  hunt cases have crossing inputs (the generators move vertices onto the other polygon and place holes
  near the vertex centroid: 2642 of the 4192 large-set cases), so the polygon checks cover about 2300
  generated cases plus the hand tests (holes in both, islands in holes, a hole three deep, the inputs
  reversed, a union and a shared-edge symmetric difference over a hemisphere throwing). All 774
  committed and 6291 uncommitted cases pass. **Open, for PR C**: output rings that cross themselves
  (where the input's do) have no orientation with the output on the left all round.

- 2026-11-05: **Fable review 3** of steps 7 and 8 (`phase5-review-3.md`), and its fixes, all done (the
  developer chose the root cause for 1.2, in the prototype too, and every test gap):
  - **1.2, a silent wrong answer** (upstream, step 2): where an input touches itself on a stretch it
    shares with the other (a hole's tip on its own exterior's edge), `GeometryIntersect`, which never
    intersects an input with itself, cuts only the other's segment, so nothing pairs; the band
    classes then let the sector walk deduce wrong common classes that every check accepted. Fixed by
    **self-touch cuts** (design 3.5; prototype configuration `r5c`, planning `bc564e1`): an unpaired
    single arc whose ends the other input joins by a chain within the thickness of its segment is cut
    at the chain's vertices, and the pieces built again. Measured (cases with a cut): `shared` 256 of
    598, `rays` 226 of 598, `chain` 82 of 598 (266 of 2099 in the chain set), `near` 2, `six` 4,
    `tinyhole`, `zero` and `holes` none; catalogue `d4` 13 of 49; bundle 7 of 15. The C++ and the
    prototype agree on every case of every set. Getting them to agree found three traps, each now a
    comment at the code: the prototype's `T_SINE` is the old thickness (PR A made it `s`, `S_SINE`,
    larger by sqrt 2); a segment `GreatCircleArc` calls zero length has no rotation axis (its own
    plane, in doubles: `Vector3D` will not normalise so short a normal); a chain vertex at a segment's
    end needs a margin of `1e-12` rad or doubles and the prototype disagree. And one in the tools:
    `CONFIGS` already had an `r4s`, and a later duplicate key silently won.
  - **The guard**: a hemisphere-check failure now throws for every operation under
    `throw_if_inconsistent` (counted otherwise), with a message that names both causes it cannot tell
    apart (an output over a hemisphere, or wrong classes).
  - **1.1, a false throw**: at a vertex whose rays are all common (identical inputs that touch
    themselves) the sector walk knows the memberships only up to swapping both; consecutive kept rays
    pair, either phase, and the split decomposes.
  - **Tests**: identical self-touching inputs, the self-touch on a shared edge (both inputs, on and
    `0.5h` off), a four-kept bounce, a vertex both inputs pass twice, the wedge's split pinned, the
    area-sum oracle (design 11.3: robust ring areas, parity by depth, a tolerance of `2h` times the
    perimeters over the area; skipped for degenerate input rings, isoperimetric ratio under `1e-3`),
    the orientation checked by the vector area rather than `PolygonOrientation`, review 3's four hunt
    cases, `chain#146` seed 2, and a new hunt generator `holes` (non-crossing inputs with holes and
    islands; 300 pass, 20 committed). The ring comparison now skips cases whose prototype rings depend
    on the tie order (design 11.2 item 4; 14 catalogue, 8 hunt, 2 bundle cases). Every mutation of the
    new code fails a committed test. All 798 committed and 6591 uncommitted cases pass.
  - **Now unreachable**: a non-common ray without a direction. Its one case (`d4 hole-between`) is
    now a self-touch cut, and no committed or constructed case reaches one; `RaysWithoutDirection` was
    replaced by `ShortSegmentSelfTouch`. The code stays (real data can have short segments).
  - **For step 9**: the walk would index a polyline's dangling end (review 3 1.4).

- 2026-11-05: **Phase 5 step 9 done: polylines, CitcomS** (design 6.1, 6.2). A polyline subject is
  classified by its own rules (`classify_polyline_pieces`: the along class of Q2, inside unless
  `Options::polyline_along_is_inside` says otherwise; vertex tests at interior vertices and dangling
  ends; deduction only across a vertex that is one crossing; midpoints by margin) and output as runs of
  one class (`extract_polylines`); `extract_rings` and `extract` now assert a polygon subject (review 3
  1.4). **Deviation from design 6.1**: a run's points are the stops' own positions, not the vertices'
  output positions (an identity vertex along a dense run of touches can put its position far off the
  polyline; the oracle found 78 of 300 samples uncovered). The prototype's polyline overlay now runs
  `r5c`'s rules (the restricted spike rule and the self-touch cuts) and the exporter writes polyline
  cases (`--polyline GEN:SEED:INDICES`, `opened-<base>` for opened hunt rings; planning repository);
  `prototype-polyline.txt` (189 cases: 30 of each of `ends`, `along`, `touch`, `pinch`, 10 opened rings of
  each hunt generator) and 1899 uncommitted cases agree with the prototype in pieces, classes and runs,
  and pass the new oracle along the polyline (`count_polyline_failures`: within `2h` of a run of the
  right class, as many runs as the polyline passes the point). Hand tests: across, no record, along
  (both settings of the option), a touch and an end on the edge, a pinch. Mutations: each of the along
  class, deduction across touches, deduction from along pieces, untested dangling ends, vertex
  positions and internal pieces' class fails a committed test; **reversing the midpoint margin order
  changes no polyline case in either set** (not pinned). CitcomS uses `GeometryIntersect::intersect`;
  `citcoms_meshes` covers it (verified: it fails if the intersection's point is replaced).
  **`PolylineIntersections` stays until step 11**: `PolygonPartitioner` still calls it (design 12
  amended). Trap: a source restored by copying can keep an older time than its object, so ninja does
  not rebuild it (a stale CitcomS object failed ctest after a mutation check); touch it.

- 2026-11-05: **Phase 5 step 10 done: provenance** (design 7.1, 7.2, both amended "as built"). Every
  output point of a ring or a polyline can carry a `Provenance` (`Options::want_provenance`): its
  location on the subject (a vertex, or a segment and ratio; none for a cut point) and on the clip,
  with the clip's index; through a cascade it is composed with the subject's own table
  (`Options::subject_provenance`, with the original's and the earlier clips' ring sizes for the
  wrap-around). Found: the composition's induction fails where an identity vertex holds two vertices
  of one input (a piece's edge spans the group): counted, never a wrong value (0.2% of second-step
  points in the large set, 3% in the chain set); and guessing the wrap-around without the ring's size
  gave a wrong segment (now an option, or a mismatch); and a vertex's clip location must be set
  before composing. Checked: every committed and generated case's points resolve to them, polygons and
  polylines; a second cascade step on every polygon case (the difference's pieces against the clip
  again); hand tests of each kind of point, a two-plate cascade beside and across plate 1, and a
  polyline cascade. Six mutations each fail a committed test. **Moved to step 11**: provenance through
  `PolygonPartitioner` and `GeometryCookieCutter` (`PartitionedGeometry`, 8.2), with the partition
  layer itself.

- 2026-11-05: **Fable review 4** of steps 9 and 10 (`phase5-review-4.md`), and its fixes. The polyline
  path is a faithful translation of the prototype; CitcomS is right. Fixed: **provenance was filled
  before the split** (3.11), so a split ring's vertex could lose its subject location or take the
  other passage's (wrong at 611 of 4192 large-set cases; the oracle accepted a point with only a clip
  location): now filled per ring after the split, from stored stops (the leaving stop and its piece's
  far stop), which also removes the guess at a loop piece's end. **A polyline stop's clip location**
  was its identity vertex's (up to `12h` off along dense touches): now its own record's. A **second
  cause of composition mismatches** (an input's corner within `h` of both segments, naming one),
  found by a second step against a moved clip; mismatches now counted apart, subject (a location
  lost) and clip (nothing lost). Tests: tolerances `2h` (first step) and `3h` (second; measured `1.0h`
  and `1.63h`, against `M >= 10h` before), a check that a point beside an edge clearly along the
  subject has a subject location (the old computation and three mutations fail committed tests),
  the second step also against the clip moved by 0.37 of its radius (clip index 1, so the clip
  locations are checked against the right clip), a split at a vertex holding no subject vertex
  (`ProvenanceAtSplit`), and a polyline's run points against its pieces' stops exactly. All 987
  committed and 8490 uncommitted cases pass. **Open, for step 11**: the review suggests handing the
  composition the original geometries rather than ring sizes (design 7.2 as built); to decide there.

- 2026-11-05: **Decided (developer): the composition gets the original geometries** (review 4's
  suggestion): `Options::original_subject` and `Options::cascade_clips` replace the ring sizes. The
  developer's questions first established what the clip side is for: a coverage uses the subject
  location alone (7.3), and no earlier clip's location is ever output (every point at an overlay vertex
  lies on this clip), so composing an earlier clip's location only keeps a cut edge between two points
  of the subject's outline from being counted as a lost location; provenance is offered by the cookie
  cutter's single-geometry `partition_geometry` only (its several-geometry form mixes pieces whose
  tables would not say which geometry they refer to). Design 7.2 amended.

- 2026-11-05: **Phase 5 step 11 done: the partition layer** (design 8.1, 8.2, 5.3, 10.2, all amended
  "as built"). `PolygonPartitioner` cuts polygons and polylines by the overlay: a polygon into polygons
  with holes, both inputs' holes taking part; a geometry whose pieces would all be on one side (a touch
  from either side, along the boundary, identical) comes back itself, multi-points included; with
  neither list, the result-only path (`Overlay::classify(stop_when_result_known)`, stopping after the
  vertex tests when a subject piece is on each side or a clip piece is inside the subject, and
  answering `GEOMETRY_INTERSECTING` where the overlay throws). The disjoint-bounding-circle shortcut
  of design 4.1 is built in the partitioner (it was not before). `PartitionedGeometry` carries a
  table per piece; `GeometryCookieCutter::partition_geometry(..., want_provenance)` composes them
  through the cascade, a clip location's index being the plate's position. `PolylineIntersections`,
  the bend test and the polyline merger are deleted; `PolygonOverlay` joined the pyGPlates sources.
  Trap: Windows' `WinUser.h` defines `DIFFERENCE`, breaking the `Operation` enumerator wherever
  `<windows.h>` came first (undefined in the header, with a comment). Tests: `PolygonPartitionerTest`
  (doc 5.1's four bugs: a partitioner inside the input, an input across a partitioner's hole, a
  partitioner across the input's hole, along the boundary for both orientations; the contract's
  touches and identical inputs returned as the same object; the result-only path; a multi-point's
  provenance; and a two-plate cascade through `GeometryCookieCutter` built from features, with a hole
  across the shared edge and the input's closing segments across both plates, every point's
  provenance resolving and every point beside an edge along the input keeping its location). The two
  pyGPlates tests that asserted polylines now assert polygons (design 8.4; the second's input crosses
  itself). Mutations: seven of eight fail a committed test (the input itself returned as a copy; the bounding-circle shortcut always taken; the early stop on one subject class; the cascade's clip index unset; the piece's table, the original geometry, or a whole piece's table dropped); the eighth, skipping the classes' early return, is equivalent (the emptiness check after extraction gives the same answer). The whole piece's table was unpinned until a far plate was put between the two in the cascade, which returns the first plate's outside piece whole. All 987 committed and
  8490 uncommitted prototype cases pass; GPlates and pyGPlates suites green. **For step 12**: the products'
  handling of `InconsistentOverlayException` (Assign Plate IDs skips and reports; pyGPlates
  `PartitionError` is step 13), and coverages through the new tables. **Before PR B merges**: the
  foundation page (`spherical-geometry.md` section 5.1 and its mentions of `PolylineIntersections`)
  describes partitioning as it was; Phase 6's distillation rewrites it.

- 2026-11-05: **Fable review 5** of step 11 (`phase5-review-5.md`), and its fixes (the developer: all
  of them). No silent wrong answer: run through the partitioner on every uncommitted case, every table
  the right length and every location resolving (25303 polygon pieces, 13689 cut again, 5340 runs).
  - **1.1, one contract (decided by the developer, after asking why the no-list path did not raise
    and how the collapsed cases differed):** pyGPlates `PolygonOnSphere.partition` without lists was
    a second caller of the result-only path, so one polygon could raise with lists and say
    "intersecting" without, or say "intersecting" where the lists returned it whole (4 large-set
    cases, the intersection's rings all collapsed). Now the result is 5.3's with or without lists
    (the no-list form finishes, extracts and looks, unless the early stop or the bounding circles
    settle it), the partitioner raises either way, and `GLReconstructedStaticPolygonMeshes` catches
    the exception itself. **Found on the way: the early stop was not exact.** The review and the session
    had both argued that a vertex-tested piece's region cannot collapse (its vertex is more than `h`
    from the other outline); a probe of `shared#247 seed 1` showed a piece leaving an overlay vertex
    and coming back to it, a tip `2e-6` rad long, whose ring has two positions. The stop now counts only
    a piece with three distinct positions of its own in its ring (clean-up's own rule). Design 5.3,
    8.5, 9.2 and 10.2 amended.
  - **1.2**: the pyGPlates docstrings' polyline warnings are assigned to step 13 (design 8.4).
  - **1.3**: the partitioner's point-in-polygon speed now reaches the overlay
    (`Options::clip_speed`); a polyline run at one position, if one can arise, is dropped and a side
    left empty makes the polyline whole; the test helper compares pieces point for point (and the
    one-list forms); added lines within 100 columns.
  - **Test gaps**: other geometry types through the cookie cutter with provenance (a multi-point over
    three plates, a polyline cut by two, a point, a cutter with no plates), a piece keeping two holes,
    a whole multi-point and point, the collapsed case (`PartitionerCollapsedRegion`, from the committed
    hunt file), the helix band (an interior past its bounding circle) both ways, and in the prototype
    comparison, on every polygon case, the partitioner's no-list and list results agree and a whole
    result is the subject itself (all 987 committed and 8490 uncommitted cases pass). Mutations: 16 of
    17 fail a committed test (review 5's nine, step 11's eight less the equivalent one, and the early
    stop's new guard); the survivor, the cascade's partitioning polygons reversed, changes only the
    clip-mismatch diagnostic. Not pinned: the renderer's catch (no rendering tests) and the one-point
    run (no input makes one). GPlates 179 of 179, pyGPlates 7 of 7.

- 2026-11-05: **Line length (developer)**: a new rule in `AGENTS.md`, committed straight to `main`
  (`a46c71a01`, the developer's request) after the developer noticed wraps at 100 columns that looked
  out of place among longer lines: tab = 4; a new or changed line may match the longest code line
  within 30 lines either side, up to 130 (measured over `src/`: 4% of lines exceed 100, 0.4% exceed
  130); pyGPlates docstring literals neither count as neighbours nor are bound. `main` merged into this
  branch with G1-1/G1-4 (#109, `cb01ccbea`; no conflict in `PartitionFeatureUtils.cc`, only the
  generated matrix), and the step-11 wraps the rule allows undone (`c40a212a0`).

- 2026-11-06: **Phase 5 step 12 done: coverages, area, and Assign Plate IDs' failures** (design 7.3,
  8.3, 9.2, each amended "as built").
  - **Coverages from provenance**: `PartitionFeatureUtils` asks the cookie cutter for provenance when
    a geometry property has a coverage, and each point takes its scalars from its location on the
    original (a vertex's; a segment's ends' interpolated, a ring's last segment wrapping to its first
    vertex, which fixes the old read past the end); runs of points with none (cuts; a hole's points
    under an exterior-ring-only coverage) are interpolated by arc length between their valued ends;
    a ring with none takes the nearest outline point's value, by the point's projection onto the
    segment (the old fallback's ratio was the distance to the point itself). The epsilon map and the
    0.5-degree search are gone.
  - **Area**: "most overlapping" compares area (polygons), then arc length (polylines), then points.
  - **Assign Plate IDs** catches `InconsistentOverlayException` per feature, leaves the feature as it
    was (a task partitions before it modifies) and reports the skipped features once: the dialog in
    one warning (up to ten by name and ID, the rest counted), the CLI a `qWarning` line each. Not
    tested: no committed input is inconsistent, and the dialog is GUI.
  - Tests: `PartitionFeatureUtilsTest` (a polygon with a hole cut by a plate whose corner is inside
    it: vertices exact, the hole's too, a crossing on the closing segment, the plate's corner by arc
    length; an exterior-ring-only coverage, with the hole opened into exterior rings and kept whole;
    a plate inside the input, the nearest outline value; the size metric). Mutations:
    seven, each failing a committed test (the closing segment's wrap, arc-length interpolation along a cut, the nearest-outline value and its projection, the exterior-ring-only domain, the rings' offsets, area in the size metric); the exterior-ring-only mutation survived until a piece keeping its hole was added. GPlates 185 of 185, pyGPlates 7 of 7.
  - Trap (tools): a rewrap script took a continuation line `++i)` for four indentation markers and
    dropped the loop's increment; the test hung (150 s) rather than failed. Rerun the tests after
    any scripted rewrap.

- 2026-11-06: **Fable review 6** of step 12 (`phase5-review-6.md`), and its fixes (the developer:
  all of them).
  - **1.1, a silent wrong answer**: the coverage interpolation's loops for a polyline's end runs ran
    for closed rings too, so on a ring whose first point has no value, the points before its first
    valued point all took that point's value. A ring starts so whenever a piece is cut a second time
    (the second overlay's first stop is on the first plate's cut edge): every feature spanning three
    plates. The committed tests' rings all started at a crossing. Fixed with one guard;
    `RunAcrossRingStart` checks every run of every ring against an independent arc-length
    interpolation, in both plate orders.
  - **The early stop dropped (decided by the developer)**: a new test on a committed inconsistent
    input (`polygon-overlay/inconsistent.txt`, the bundle failure `shared#1434 seed 12`) showed the
    contract decided after review 5 unmet: without lists, the overlay's early stop answered
    "intersecting" before the checks that raise. The developer asked whether dropping it also
    removes `stop_when_result_known`; it does, with `stopped_early` and the review-5 guard. Now the
    result-only path is the full path less building the output geometries (design 10.2), and the
    partitioner raises with or without lists.
  - **Assign Plate IDs' catch tested** on that input: the feature keeps its properties and plate ID,
    and is reported; the dialog reports also on a cancelled run.
  - **Test gaps**: two scalar types, the nearest outline on a hole, the nearest-outline clamp (an
    L-shaped input), a polyline, a multi-point, a point and a whole polygon through
    `partition_feature`, and the size metric's point-count tie-break. Mutations: 14, every one
    failing a committed test (review 6's six survivors, the fix reverted, the catch's record removed,
    and step 12's seven).
  - Minor: doc-comment lines the rewrap script had stripped of their leading space, two odd rewraps,
    an unused include, and lines over the line-length rule. The developer's note on that rule: it
    counts any code line within the window as a neighbour (the session's checker had counted only
    existing ones, which was stricter than the rule says).
  - All 987 committed and 8490 uncommitted cases pass, the no-list and list forms agreeing on every
    polygon case; GPlates 191 of 191, pyGPlates 7 of 7.

- 2026-11-06: **Phase 5 step 13a done** (`27cbfea1f`): `PartitionError` (a `MathematicalError`, from
  `InconsistentOverlayException`, listed in the reference's exceptions), the docstrings of
  `PolygonOnSphere.partition` and `PlatePartitioner.partition_geometry` (the polyline warning gone; what
  partitioning now does; `:raises PartitionError:`; `versionchanged:: 1.1`), the `:raises:` of
  `partition_features` and `partition_into_plates`, `PartitionMethod.most_overlapping_plate` by area
  (the enum's docstring and `PlatePartitioning.py`'s measure, `(area, arc length, points)`), the stub
  regenerated, the docs built from scratch with no warnings, and the changelog entries of both products
  (GPlates' Assign Plate IDs set complete; pyGPlates' coverage entry comes with 13b). New pyGPlates
  tests: polygons with holes, touches returned whole, `PartitionError` on the committed inconsistent
  input (read from `src/unit-test/data/polygon-overlay/inconsistent.txt`), the area overlap with an
  L-shaped feature.
- 2026-11-06: **Phase 5 step 13b done** (`295cc3270`): coverages through pyGPlates'
  `partition_features` (design 8.4 as built). A private `PlatePartitioner._partition_feature_geometries`
  calls the same `PartitionFeatureUtils::partition_feature` as Assign Plate IDs, and the Python side
  removes the range properties with the geometries and sets the pieces back. **Found**: including
  `PartitionFeatureUtils.h` pulled about 320 GPlates-only files (data mining among them) into the
  pyGPlates closure, through `AssignPlateIds.h` and reconstructions; the feature-geometry partitioning
  (`PartitionedFeature`, `partition_feature` and its coverage code) moved to
  `app-logic/PartitionFeatureGeometries.{h,cc}`, same namespace, and the closure grew by those two files
  only. **Decided (developer)**: a piece of an exterior-ring-only coverage keeps that form, as Assign
  Plate IDs writes it (the geometry set alone and the range added as a `GmlDataBlock`, after checking its
  count is the exterior ring's, since `set_geometry` needs every ring's); the developer noted, and the
  session confirmed, that a hole cut open then gains values while a hole left whole gets none, inherent
  in that form. Tests: a polygon with a hole across two plates (original values exact, one per point,
  one range per feature) and the exterior-ring-only form kept. Not run as mutations (each needs a
  pyGPlates rebuild); by reading, losing the coverage or the two-step write fails the new test. **For
  the PR**: the architecture page of the plate-partitioning area, and the foundation page, describe
  partitioning as it was (Phase 6's distillation).

- 2026-11-06: **Fable review 7** of step 13 (`phase5-review-7.md`). Decided (developer): **each coverage
  piece in its own feature, as Assign Plate IDs does** (`PartitionedFeatureManager::get_feature_for_partition`
  starts a new feature for a piece with a coverage), which fixes review 7 1.1 (`AmbiguousGeometryCoverageError`
  from two same-count pieces under one name), 1.3 (read back as no coverage) and 1.5 (a mixed list
  refused); and **all the other fixes** (the lost `def test_partition_point`, tests pinning the three
  surviving mutations, unused includes, coverages in `partition_features`' docstring, the range added
  without information-model verification): `851c0850f`, each of the three surviving mutations now failing
  a test. **Review 7 1.2, decided (developer)**: time-dependent geometry is not really supported yet, and
  partitioning one window would leave the others undefined, so a time-dependent geometry property is
  not partitioned but kept, unchanged, on every feature returned, as Assign Plate IDs already does (only
  pyGPlates' removal loop had dropped it; a feature with nothing else to partition is returned whole):
  `7c89317bb`, with a test. Before 1.1, pyGPlates partitioned the window at present day and lost the
  others. Supporting time-dependent geometry, and what partitioning should do with it, is an inbox
  item (planning repository `db47f6c`). GPlates 191 of 191, pyGPlates 7 of 7.

- 2026-11-07: **Phase 5 step 14a done: the sliver filter, off, and the real-data measurements** (`c156cd7c3`; design
  4.5, 10.3 and 11.4, each "as built"). The filter is `Options::sliver_min_width`, set through
  `PolygonPartitioner::create` and the `GeometryCookieCutter` constructors; it is off by default, and not
  in pyGPlates. **Found**: `SphericalArea` can't measure a sliver, since its fan drops every edge under
  about `1e-6` radians, and cuts make such edges. The measure is the new `PolygonOverlay::get_mean_width`
  (exact triangles, modulo `4 pi`, the smaller side). **Decided (session; the design had left it open)**:
  a filter that empties both outputs leaves the subject whole on the side that lost more area. Tests:
  the filter's four cases and the measure; nine mutations of the filter and three of the measure, each
  caught.
  - **Real data**: Muller et al. 2022, the coastlines (2136 features, 2937 polygons, 100,000 points),
    reconstructed by their own plate IDs and partitioned by the topologies at 0, 100 and 200 Ma.
  - **Timing**, pyGPlates `partition_features`, best of three, against `main` before PR B:
    | Time | Before PR B | After PR B |
    | --- | --- | --- |
    | 0 Ma | 0.20 s | 0.28 s |
    | 100 Ma | 0.15 s | 0.19 s |
    | 200 Ma | 0.13 s | 0.16 s |

    That is about +30%, and resolving the topologies is about 0.06 s either way. Pieces at 0 Ma: 431
    polygons, against 703 polylines before.
  - **Checks** (`PolygonOverlayRealDataTest`): 15,255 overlays past the bounding circles, 313 of them
    intersecting. No `InconsistentOverlayException`, no oracle failure, no check failure even before
    repair, no bundle vertex (at most six ends at a vertex), 11 midpoint tests, and 2 collapsed rings
    dropped. The piece counts equal pyGPlates' at each time.
  - **Slivers**: 783 output rings of the 282 polygons cut. Their mean widths, in `h` (about 9 m):
    | Mean width | Rings |
    | --- | --- |
    | under 1 `h` | 5 |
    | 1 to 10 `h` | 11 |
    | 10 to 100 `h` | 57 |
    | 100 to 1000 `h` | 235 |
    | 1000 `h` or more | 475 |

    Under `h`, 10 `h` and 100 `h`, that is 5, 16 and 73 rings: 0.47 km², 4.5 km² and 373 km² (`7e-6`
    of the cut polygons' area). The thinnest are triangles, where a continental outline was digitised
    along a plate boundary: South New Zealand and the Campbell Plateau at 0 Ma, eight of the sixteen
    under `10 h`; the San Jorge plate; Western Cameroon at 100 Ma, 71 km long and 5 m wide.
  - **Near-zero output edges** (the open item from step 7): on real data the overlay adds none. Of the
    output's edges, the 8 under `h / 10` and 29 under `h` are exactly the inputs' own, and no edge on
    either side is under `1e-9` radians. Nothing found calls for a filter of them.
  - **Next, for the developer**: the default and the width (design 4.5), then wiring it through (step
    14b; decided and done, the next entry but one).

- 2026-11-07: **Two items for PR B from the `src/api` audit triage** (finding G8-1, decided with the
  developer in the audit session on `main`; recorded in the planning repository's
  `plans/src-api-audit.md`, *Triage*, group 8). Both are in `partition_features`' `split_into_plates`
  block, which PR B rewrites, so they go here and not in the audit's fix PR.
  1. **A feature with nothing to partition is still dropped.** After review 7's 1.2 (`7c89317bb`),
     the block returns a feature whole only when
     `not partitioned_geometries and not unpartitioned_geometries and has_time_dependent_geometry`.
     A feature with **no geometry at all** fails the last test, falls through, and ends up in
     neither returned list. `most_overlapping_plate` and the VGP path return it as an unpartitioned
     clone, as documented. **Fix:** drop `and has_time_dependent_geometry`, so any feature with
     nothing partitioned comes back unpartitioned and whole. Test: a feature with no geometry is in
     the unpartitioned list.
  2. **Partition a time-dependent geometry with exactly one window that contains the reconstruction
     time** (developer's idea, 2026-11-07): this refines 1.2 without reopening it.
     - **The rule.** A geometry property whose value is a `GpmlPiecewiseAggregation` with exactly one
       time window is partitioned like a constant geometry, and its pieces are written as constant
       properties, if that window contains the partitioner's reconstruction time (with
       `GeoTimeInstant`'s epsilon at the window's ends).
     - **Everything else time-dependent keeps 1.2,** unchanged on every returned feature: more than
       one window, one window not containing the time, or a `GpmlIrregularSampling` of geometries. If
       such a geometry is the feature's only one, item 1 returns the feature whole.
     - **Why it's safe.** With one window there are no other windows to leave undefined, and its
       begin and end are redundant: `gml:validTime` defines when the feature appears and
       disappears. `partition_features` already treats a feature's geometry as being at the
       reconstruction time (its docstring: "effectively contain geometry at the reconstruction time").
     - **Why it's useful.** It supports a single geometry wrapped in a piecewise aggregation by
       mistake, which very early GPlates builds wrote into early topology files.
     - **GPlates must do the same.** 1.2 was justified by matching Assign Plate IDs, so the rule goes
       in the shared partitioning code both products use, or in both paths. Otherwise pyGPlates and
       GPlates diverge again. Tests in both products; changelog lines for both.

- 2026-11-07: **Phase 5 step 14b done: the sliver filter on at `10 h` in the cascade** (`d771f769c`; design 4.5,
  "Decided"). **Decided (developer)**: 10 h, about 90 m, as `GeometryCookieCutter`'s default
  (`get_default_sliver_min_width`), which reaches Assign Plate IDs and pyGPlates' `PlatePartitioner`;
  `PolygonPartitioner` and `PolygonOnSphere.partition` stay off; pyGPlates exposes it. The argument is
  `sliver_width_threshold_radians` (named after `distance_threshold_radians`), on both `PlatePartitioner`
  constructors and on `partition_into_plates`. `None` gives the default, as `anchor_plate_id=None` does,
  zero turns the filter off, and a negative value raises `ValueError`. Tests: the cookie cutter's
  default and none (the test helper now constructs with the defaults, so it tests the constructor's own
  default, which Assign Plate IDs relies on), and pyGPlates' argument through both constructors and
  `partition_into_plates`. Four mutations, each caught. Changelog entries in both products; stub
  regenerated, docs built from scratch. GPlates 197 of 197, pyGPlates 7 of 7. **Then (developer)**:
  `PolygonOnSphere.partition` takes the same argument, defaulting to zero (off), since filtering the
  pieces afterwards is not the same: it can't drop a sliver hole, return the geometry whole when a
  side empties, or measure short edges exactly (`get_area` is `SphericalArea`'s). (Its default became
  `h` in step 15: Status, 2026-11-08.)

- 2026-11-07: **The two `partition_features` items from the `src/api` audit done** (G8-1; the entry
  "Two items for PR B" above; design 8.4 "as built"). **Item 1**: a feature with nothing partitioned is
  returned unpartitioned, whole (`split_into_plates` dropped one with no geometry, on `main` too: a
  changelog bug fix). **Item 2**: a one-window `GpmlPiecewiseAggregation` containing the reconstruction
  time is partitioned as a constant geometry, in the visitor both products share, its clone a constant
  property, so both of GPlates' methods and pyGPlates write constant geometry back. pyGPlates now
  removes exactly the properties the shared code partitioned, by position, instead of deciding with its
  own rule. **Found**: `Feature.set_geometry` replaces every property of the name it sets, which hid a
  missing removal from every existing test. The new test is a feature whose two geometries have
  different names and go to different plates. Tests in both products; seven mutations, each caught;
  changelog entries in both. GPlates 198 of 198, pyGPlates 7 of 7. **Left for the audit's session**:
  mark G8-1 done in the planning repository's `plans/src-api-audit.md` (not edited here, since that
  session has its triage of group 8 in progress).

- 2026-11-08: **Phase 5 step 15: the hunt at scale** (design 11.2), in four commits: `626544744`
  (the comparison's harness), `8bea9c67a` (the bundle cone), `09c1f7cb6` (the grouping margin),
  `3f7bac4b0` (`PolygonOnSphere.partition`'s default); design 3.8, 4.2, 4.5 and 9.1 amended.
  - **The hunt**: 119,198 new prototype cases, about fourteen times the earlier sets, over the eight
    generators at new seeds, plus jittered `shared` copies and polyline cases (planning repository
    tools, driven from the session's scratchpad). First reported as all passing: my driver's bug, since
    gtest prints `[  PASSED  ] 0 tests.` on a failure. 93 differed, and three more cases failed in the
    prototype too.
  - **Found and fixed**:
    - The grouping margin. Output rings cross within a vertex by up to its cluster's width, and a
      containment test with a margin of `h` put a hole outside its exterior: 5 cases, the
      intersection's area wrong by about a quarter.
    - **The bundle cone (decided, developer)**. A subagent diagnosed the residue in exact arithmetic:
      the cone assumed a ray's first arc on its segment, which a touch record at the other polygon's
      vertex is not, so in four cases the true order was not admissible. The prototype's `r5o`
      (planning `82825cd`) and the C++ add those offsets. Clean: `#2587` seed 13, `#1434` seed 12,
      `#4938` seed 3, `#2557` seed 8, all committed. The snapshots are re-exported under `r5o`
      (a scratchpad script rebuilds them from their case names).
  - **The test, not the code**: the provenance tolerance (37 cases; a point at a vertex can be a
    cluster's width from its record), and rings of no area whose orientation the oracle judged (35;
    an input overlapping itself along a stretch bounds them).
  - **Decided (developer)**: `#2288` seed 2 is left as the residue (band classes along a dense
    shared edge, no bundle vertex; it throws, and is now the failure tests' input).
    `PolygonOnSphere.partition` defaults to `h` (design 4.5): the three widths are `h` (the overlay's
    resolution), `10 h` (global data's), and zero (exact).
  - **After the fixes**: 13 of 119,198 cases differ, none in a partition's geometry.
    - 4 are the symmetric difference's crossing pairing (rings overlapping where a vertex's ends are
      paired across): PR C.
    - 4 are coincident rings, at least two of them the zero-area rings' cases (likely all four).
    - 4 are a cascade's second step losing a subject location (no point's provenance wrong):
      uninvestigated.
    - 1 is a structural difference: two crossings 1e-16 radians apart are two vertices in the C++, one
      in the prototype. The output is right (0 wrong of 19,982 samples).
  - **Linux**: three prototype tests failed, from my archive: `core.autocrlf=true` makes `git
    archive` write CRLF, and the case names kept a `\r`. Fixed in the reader and in the recipe
    (`gplates-local.md`, planning `39ce8f4`). The working tree passed all 198, and pyGPlates 7 of 7, on
    Ubuntu 22.04.
  - GPlates 198 of 198, pyGPlates 7 of 7; mutations reverting either fix are caught by the committed
    cases.

- 2026-11-08: **Review 8 (Fable) of steps 14 and 15, and its fixes** (`phase5-review-8.md`, committed
  `820e9757d`), in five code commits, one per finding, and design 3.8, 4.5, 8.4, 9.2 and 11.4 amended.
  - **1.1, a silent wrong answer**: `get_mean_width` fanned its triangles from the vertices' mean
    direction, which for a ring on one great circle lies in the ring's plane, so a 36-point equator
    ring measured 0 and the default filter reported a hemisphere wholly inside a 10-degree box (all
    three public paths). The apex is now the ring's vector area. Tested on the equator and meridian
    rings and a hemisphere partitioned by a box, in both products.
  - **1.2**: the cone's offsets are decided by the stop's type (only a touch record inside a segment
    and a self-touch cut are measured), not by the `1e-12` threshold, which crossings in dense shared
    edges exceeded. Pinned by `RaysCrossingsOnTheirSegments` (whose first version, at one place on the
    sphere, had crossings off their circles by only `3.5e-13`, so it now samples ten) and the far end's
    offset by `RaysFarEndOffset` (it was pinned only by `BundleRule`'s counts).
  - **1.3**: a ring inside a dropped ring is dropped with it (an island of a filled hole once stayed, as
    a polygon overlapping it).
  - **1.4**: a single-window coverage's range is written back constant with its domain by "most
    overlapping plate".
  - **1.5 and the test gaps**: `:type ...: float, or None`; `is_ring_inside_ring` moved into the
    `PolygonOverlay` namespace and tested directly (`GroupRingInsideRing`); the real-data harness's
    area cut subtracts holes.
  - Each fix reverted in turn fails its new test (a scratchpad mutation script); the hunt's 119,198
    cases give the same 13 differences as before. GPlates 203 of 203, pyGPlates 7 of 7; docs built
    from scratch with no warnings. Linux (Ubuntu 22.04, the tip `1a2f2ddac`): the same.

- 2026-11-08: **Phase 6: PR B distilled** (Fable), in three commits.
  - `9bc4f3754`: `spherical-geometry.md` section 5 rewritten from the code (5.1 the layers to 5.7 the
    rejected alternatives; `DateLineWrapper` is now 5.8), with the tolerance table, Testing and
    Deferred work; traps not yet at the code put in comments; two changelog details. The report,
    with what moved where, what may be dropped at PR C and where the plan disagreed with the code:
    `prB-distillation.md`. My review corrected a few overclaims and historical phrasings.
  - `d7bf5c585`: about 270 comment citations of the plan repointed to the page, from the report's
    mapping (an Opus subagent; reviewed: comments only, both trees build).
  - The Fable claim check of the new text (`prB-claim-check.md`): 0 high, 2 medium, 17 low, all
    applied (the commit after this entry). The measurements quoted are consistent with the plan's
    records; 29 of 33 sampled comment citations held, and the other 4 are fixed.

- 2026-11-08: **The full PR B review, by area** (Fable, five reviews one at a time:
  `prB-review-1-overlay-classification.md` to `prB-review-5-pygplates.md`), and its fixes (the
  developer approved the consolidated list), in seven code commits from `3180ea66e` to `bb28c7824`
  and the page's amendments.
  - **Three wrong answers fixed**: an anchorless chain's clip parity counted off the clip's great
    circle (a thin polygon partitioned by a near-copy of itself came back wholly outside; review 1),
    with a new hemisphere check for whole answers, which extract never saw; a polyline's hairpin made
    internal by the spike rule (review 2); `partition_features` dropping an unpartitioned property
    that shares a partitioned one's name (review 5).
  - **Decided (developer)**: a hairpin along the clip's edge is along (inside); an unmatched coverage
    range is removed with its geometry (both products now agree); a single-window geometry with a
    multi-window coverage is not partitioned; pyGPlates' most overlapping measures what splitting
    partitions; the cookie cutter without lists keeps its early return (the page says so).
  - **A decision reversed by evidence**: review 2's grouping failure was to throw (review 2 found no
    input reaching it). Made to throw, the hunt threw in 255 cases, every one an input whose rings
    cross, which partitioning has always accepted, so it stays counted (comment and page say why).
  - Small fixes: zero-measure most overlapping, `NaN` widths refused, self-touch cuts keyed by stop,
    stale comments, both changelogs. Tests for each and for the reviews' gaps (the LL rule, the cookie
    cutter's options on a plate with a hole, coverage pieces per feature, the exception in a second
    property); eight mutations reverting the fixes each fail their test.
  - GPlates 209 of 209, pyGPlates 7 of 7, docs from scratch clean; real-data timing unchanged (0.280 s
    against 0.277 s at 0 Ma) and nothing refused. The hunt: the same 13 polygon differences, plus about
    35 polyline cases where the prototype (`r5o`) still makes a hairpin internal, all passing the
    polyline oracle. Linux (Ubuntu 22.04, `b41aa8223`): pyGPlates 7 of 7, GPlates 209 of 209. A Fable
    review of the fixes (`prB-review-6-fixes.md`) found three medium gaps in them, fixed in the next
    commit (the hunt unchanged, both suites green). Still unpinned: the self-touch key (no constructed
    input reaches it) and classify() calling the whole answer's check.
    **Open**: mirror the polyline rule in the prototype (`r5p`) so the hunt is clean
    again; the margin order still has no hand-built test (the prototype comparison pins it).

- 2026-11-12: **The developer's visual check of Phase 6 is done** (data and checklist outside the repository,
  in `C:/gplates/visual-check-prB`: synthetic plates, the inconsistent case, the Merdith static
  polygons, and a reconstructed raster). Everything looked right. Two observations, both explained:
  - **The inconsistent case's inputs are invalid.** In the skipped-features check, the plate-5 piece
    of a box cut by `Q` was two polygons, not one with a hole. Both `P` and `Q` of `shared#2288 seed
    2` have a hole crossing their exterior ring, and the overlay's output is right by parity, as
    designed for crossing rings. With either hole removed, the cut succeeds, so the only known
    inconsistent input needs crossing rings on both sides. The page (5.6 and section 7), the test
    comment, the data README and both changelogs now say so; the changelogs had described it as
    nearby outlines alone.
  - **Piece counts** matched pyGPlates exactly (the feature files the developer saved). Adjacent
    pieces with one plate ID are separate static polygons: Merdith's North America is age strips, all
    plate 101. A geometry with a coverage gets its own feature when its feature already has a
    geometry of that property name, so the ranges cannot be mismatched. That predates PR B.
  - **`GLReconstructedStaticPolygonMeshes`' containment test removed** (decision 4 of 2026-11-04). The
    raster looked right with it, and over 2,389 static polygons and about 55,000 boxes standing in for
    quad-tree nodes (those surrounding each polygon among them), the overlay never answered outside
    where the test would have overridden it. Three answers of outside were exact touches, which is
    correct. Script: scratchpad `redundant_test_check.py`. The page no longer lists it as a caller
    of point-in-polygon. **Corrected by review 9**: the function is unreachable (it serves polygons
    drawn as fans, whose creation is under `#if 0` since 2012), so the raster could not show it
    either way; a replay of the real quad tree (197,984 partitions) found 147 touches at a polygon's
    first vertex (on longitude 180) and no overlap missed, so the removal stands.

- 2026-11-12: **Crossing output rings grouped into one polygon** (developer's decision, after the
  visual check showed a plate-5 piece of two polygons overlapping the plate-100 piece). Where an
  input's rings cross, output rings cross at points the overlay never computes, and in separate
  polygons, read as a union, they overlap. Parity over all a side's rings was always right (the
  hunt's oracle), but as polygons it was never checked for crossing inputs.
  - **Measured first**: with the polygon check switched on for crossing inputs and the old
    grouping, 11,212 of the hunt's 70,291 crossing-input cases were wrong as polygons (against 255
    counted grouping failures). After: 3, all in the reverse or symmetric difference, crossings
    thinner than the 10 h margin (now in the page's section 7). The hunt otherwise unchanged:
    the known 13 polygon differences and the polyline hairpin cases (`r5o`).
  - **The test** (`do_rings_cross`): first the edge test alone, by the rings' bounding trees (the
    developer's pointer to `GeometryDistance.cc`); it missed the dense shared-edge crossings
    (every edge end within 10 h of the other ring: `shared#31 seed 1` crosses only at a 0.1 h
    margin), so a vertex test came first: a vertex beyond the margin inside, another outside. Both
    are needed (mutations: without either, committed tests fail).
  - **The symmetric difference's rings can cross at a vertex** whose four kept ends pair crosswise
    (13 catalogue cases, `d4 ... hole-between`): grouped too, which reads right; the tests'
    false-positive check (a crossing group from inputs whose rings do not cross) excludes that
    operation. PR C's pairing item.
  - **Decision (6) of 2026-11-08 resolved**: a grouping failure now means a hole group whose
    nesting fails (a crossing thinner than the margin), kept as its own polygon and counted;
    unpinned (the mutation always placing it in its parent survives; no committed input reaches
    it).
  - **Timing**: A/B against `a58eb0ddb`, alternating, partition_features 0.331/0.338 s before and
    0.342/0.340 s after at 0 Ma, the same at 100 and 200 Ma. So the cheaper option (check each
    input once) is not needed; its notes are in the planning repository's inbox (`777b9c3`, under
    "Notes: split a self-crossing polygon into simple polygons").
  - The visual check's box now gives one plate-5 polygon with the plate-100 piece's hole ring as
    its interior ring. GPlates 211 of 211, pyGPlates 7 of 7.

- 2026-11-12: **Review 7 (Fable High) of the grouping, and its fixes** (`prB-review-7-crossing-groups.md`;
  the developer approved every recommendation). The grouping argument holds as coded (60,000
  random crossing-input cases read right); inputs whose rings do not cross are unchanged.
  - **Fixed, `52bb6f4cd`**: rings meeting only at vertices on each other's outline can overlap by
    any amount with no vertex inside (1.1): the crossing test also classifies an edge's midpoint
    when both its ends are within the margin. The hunt's crossing-input cases wrong as polygons 3 to
    0, and the 4 "symmetric difference's pairing" cases were this too: 44 cases differ (35 polyline
    `r5o`, 9 polygon: 4 coincident, 4 second-step, 1 tie order). A ring crossing itself measured
    as a sliver by its signed area (a bow-tie three quarters inside a plate came back wholly
    outside, 1.2, older than the grouping): a sliver candidate that crosses itself is kept
    (`does_ring_cross_itself`). Tests: the chord case (overlay, both ways round, and its closing
    edge), groups three deep (a hole group in a hole parent, depth by rings), the partition layer
    with crossing rings either way round, the bow-tie; each of those mutations now fails a test.
  - **Fixed, `a7fef5779`**: the cost (1.4): the vertex test walks the ring's bounding tree, a node
    clear of the other ring's bounding circle counting as outside at once. Review 7's probe:
    extract(INTERSECTION) 115 to 22 ms (341 rings) and 2,280 to 215 ms (1,481 rings), against 17
    and 171 ms with the test off. Hunt identical; coastline partition_features within a few
    percent of `a58eb0ddb` (0.345/0.333 s against 0.332/0.332 s at 0 Ma).
  - **Documented, `84780ca77`**: 1.3 (a crossing piece's area is exterior less interiors, for most
    overlapping), 1.7 (exterior-ring-only coverages), 1.8 (the page, comments, changelogs, the
    `partition` docstring; stub regenerated, docs from scratch clean with the links resolved).
    Deferred to section 7: 1.6 (coincident rings with area become two polygons) and the review's
    section 2 (grid-snapped inputs with rings in common and crossing give wrong ring parity
    without throwing, 5 to 52 in about 6,000).
  - **Still unpinned**: the vertex test in one argument order (every pair built is found both
    ways), the nesting check's fallback, the sliver exemption for groups, `do_arcs_cross`'s
    same-crossing sign test, hole order. GPlates 213 of 213, pyGPlates 7 of 7; Linux (Ubuntu
    22.04, `b60b21e2d`): pyGPlates 7 of 7, GPlates 213 of 213.

- 2026-11-12: **Before PR B: the prototype at `r5p`, the whole answer's check pinned, and the
  `partition_features` question measured** (the developer's decisions).
  - **`r5p`** (planning repository `8333d35`; `e5b756b30`): the prototype mirrors the C++'s two
    polyline rules (the spike rule on polygon rings only; a hairpin along the clip's edge along).
    `r5p` is `r5o` for polygons (the catalogue's records, tie orders and classes identical; output
    rings vary between any two runs, hash randomisation). The hunt: 9 cases differ, the known
    polygon ones, none in a partition's pieces; the 35 polyline cases are gone. The committed
    `prototype-polyline.txt`, re-exported, agrees to the band piece.
  - **`whole_checks`** (`b14bf9bb8`): classify()'s call of check_whole_result was unpinned (no
    counter of checks made); now counted and expected, and the mutation dropping the call fails.
  - **The margin order needs no hand-built test**: both orderings' mutations (smallest first, and
    first found, in both loops) fail `BundleRule`, `OnePassageRule` and
    `AssignPlateIdsSkipsInconsistentFeature`. Dropped from the open items.
  - **The branch history stays as it is** (confirmed by the developer, 2026-11-12): two
    merges of `main` inside it, code commits that each record a step or a review's fixes, plan
    commits that touch only this directory, and commit hashes cited throughout this plan and the
    reports.
  - **`PlatePartitioning.py`** (the release checklist's "Audit the plate partitioning"): its
    correctness review is PR B's (area reviews 3 to 5, then 6 and 7, and the visual check).
    Measured on the Muller 2022 coastlines at 0 Ma: `partition_features` 0.49 s, of which the C++
    partitioning (`_partition_feature_geometries`) is 0.15 s and reverse reconstruction 0.03 s, so
    about 0.31 s is the Python feature assembly; GPlates' Assign Plate IDs does the same assembly in
    C++ (`PartitionFeatureTask` and its two tasks), and PR B's reviews found three places where the
    two differed. **Decided**: route pyGPlates' assembly through the C++ tasks in a PR of its own,
    after PR C (likely for 1.1, not required for it).

- 2026-11-12: **Review 8 (Fable High) of review 7's fixes, and its fixes** (`prB-review-8-crossing-fixes.md`;
  the developer approved the recommendations, 1.2 as the local test). The commits did what they
  said (the tree walk equal to the loop on 5.3 million ring pairs; the hunt clean; no false crossing).
  - **Fixed, `401859aa9`**: the pinwheel (1.1: rings each with vertices on the other's edges,
    missed at any size): the point test now looks three margins along, and at the midpoints of,
    both edges of every vertex near the other ring. A bow-tie with a vertex at its crossing (1.2):
    a ring also crosses itself where, at a vertex near a non-adjacent part, its path passes
    decisively from one side of the path there to the other (each probe clear of the other path
    by the margin, and that path's arms at least two margins long; else undecided, not crossing).
    The cost (1.4): rings whose outlines are farther apart than the margin do not cross, settled by
    one distance query first: 100 long pieces side by side 2.7 times the old grouping (was 25),
    50 pieces 1.4, an annulus 1.1, review 7's probe 1.1. `NEAR` renamed (1.7). The point test is
    the public `has_points_on_both_sides`, compared with plain loops (each way round, and the whole
    test) over 400 generated pairs; mutations of the walk's descent, of looking along both edges,
    of the filter, of the self-crossing test's sides and of its undecided rule (a probe within the
    margin of the path) each fail a test; the rule's two-margin arms are unpinned (review 9).
    Not pinnable: the far-node shortcut's count (an inside point implies an exit, where a near node
    gives an outside point).
  - **Measured**: pyGPlates, the pinwheel 0 samples wrong (1,687 before) and the bow-tie with a
    vertex at its crossing cut at every width; the hunt identical (9 known); review 7's random
    probe 0 wrong with a missed pair generic, with the filter and on the 0.002 grid, 2 operations
    on the 0.004 grid (5 before); no output ring of an intersection or difference from the hunt's
    valid inputs taken as crossing itself (every ring asked, a temporary switch); coastline
    `partition_features` unchanged (0.334/0.343 s against 0.332/0.327 s at 0 Ma).
  - **Documented, `05620ce97`**: the limit (an overlap, or a self-crossing, within a few margins of
    where the rings meet) in the page, the changelogs and the docstring, and as deferred work in
    section 7 with the self-intersection mode as the cure; "not dropped itself"; a bow-tie's area
    (1.3) in section 5.4 and the changelogs. GPlates 214 of 214, pyGPlates 7 of 7, docs from
    scratch clean; Linux (Ubuntu 22.04, `555a2f2b0`): pyGPlates 7 of 7, GPlates 214 of 214.

- 2026-11-12: **Review 9 (Fable High): the final check of what PR B says** (`prB-review-9-final.md`;
  the developer approved the recommendations). Nothing changes what the code does for a user, and no
  changelog bullet was false; `e7b66c3e8` is right.
  - **`b84512ef6` changed unreachable code** (1.1): see the visual-check entry above. Comment, page
    (four places) and this plan corrected; the code stays.
  - **Timing** (1.4): `partition_features` was about 70% slower than `main` (0.33 s against 0.20 s
    at 0 Ma, both builds alternated), not the 30% recorded on 2026-11-07; the records had drifted,
    each A/B against the commit before. Profiled: about 0.04 s was `56952f9b5`'s Python scanning
    every property of every returned feature for the names `set_geometry` replaces. **Fixed**: looked
    up by name (`Feature.get` per name), identical output on all 2,306 features; now about 45%. The
    rest is the overlay and the Python assembly, for the rework.
  - **Decided (developer)**: the documents only for 1.2 (pyGPlates' most overlapping plate returns
    the feature unchanged where Assign Plate IDs rewrites it; for the rework, and section 7 now lists
    the rework), 1.3 (an unmatched time-dependent range with a value at the reconstruction time is
    kept and can be read back by a piece of that point count; removing it could take a kept
    time-dependent geometry's coverage) and 1.5 (`get_mean_width`'s apex tests are `real_t`
    comparisons, so `1e-12`; load-bearing, seven tests throw as doubles).
  - **Documents**: section 5.3 and section 1's table now list the overlay's margins (1.6); about
    thirty stale sentences in sections 5 to 7 (1.8 to 1.10, each checked against the code where
    the reviewer's evidence was a subagent's reading); citations of reviews, runs, "PR C" and "the
    visual check" removed from the page, code comments and tests (1.11); `PartitionError`'s
    docstring as `e7b66c3e8` worded it elsewhere, and a `versionchanged:: 1.1` on
    `partition_features` and `partition_into_plates` (1.12); this plan's lists (1.13).
  - **Still unpinned** (review 9 section 3, by mutation at the tip): the point test in one argument
    order, the nesting check's fallback, the sliver exemption for groups, the filter's
    hole-of-an-exterior condition (new), `do_arcs_cross`'s same-crossing test, hole order, the
    far-node count (not pinnable), the self-crossing test's two-margin arms (new), the self-touch
    cuts' key; speed only: the outline-distance shortcut. Pinned since review 8's list: classify()'s
    call of the whole answer's check and a skipped check's count, the filter's self-crossing test
    for a hole, the probe distance, the largest ring as exterior.
  - Commits `3acb8ff41` (the lookup), `9ff8e530a` (documents), `2836b4b58` (plan). GPlates 214 of 214,
    pyGPlates 7 of 7, stub regenerated, docs from scratch clean; `partition_features` 0.29/0.30 s
    against 0.20 s on `main` at 0 Ma (alternated); Linux (Ubuntu 22.04, `2836b4b58`): pyGPlates 7 of
    7, GPlates 214 of 214. The PR description is drafted in the session's scratchpad
    (`prB-description.md`), for the developer.

- 2026-11-13: **PR B merged as #110** (`aeb027de7`, a merge commit; the developer approves their own
  PRs). Opened 2026-11-12 with the description approved by the developer. CI's first run: macOS
  GPlates failed six tests, and Linux and macOS showed compile warnings (some from this PR, some
  from recent PRs). Fixed in two commits:
  - `79c4ed951`: `-Wfloat-equal` and one `-Wshadow`; deliberate exact comparisons go through the new
    `GPlatesMaths::are_exactly_equal` (`==` via `std::equal_to`, which neither GCC nor Clang warns
    about in a system header).
  - `e69656d17`: three Assign Plate IDs tests segfaulted on arm64, from `NotificationGuard` built
    from `*model_ptr()`, NULL for a feature collection outside a `Model` (UBSan; old code, first
    reached by PR B's tests); the guard gains a `Model *` constructor. The other three (`Hunt`,
    `OnePassageRule`, `PolylinePinch`) were FMA contraction on arm64, reproduced on x86 with clang
    `-march=haswell`; `-ffp-contract=off` for GCC and Clang (**decided, developer**: kept, no
    changelog line).
  - Second CI run green on all six jobs. The planning repository's roadmap has the *Done* row, and
    `DateLineWrapper` onto the overlay is a roadmap entry and a 1.1 checklist item after the
    `partition_features` rework (1.1 if time allows).

- 2026-11-13: **PR C planned** (developer): four methods (`intersection`, `union`, `difference`,
  `symmetric_difference`), each returning a list of polygons; `PartitionError` for every failure,
  including a union or symmetric difference over a hemisphere; no coverage form at the geometry
  level; the sliver argument as `partition`'s; intersection and difference through the partitioner,
  so that they equal `partition`'s lists. The symmetric difference's pairing is fixed first. Section
  *PR C* below.

- 2026-11-13: **PR C step 1 done: two overlay defects, both fixed** (report `prC-symdiff-twist.md`).
  - Diagnosis (Opus): the "four kept ends pair crosswise" explanation was wrong; the free rule pairs
    the 13 `d4 hole-between` crossings correctly. (1) **A twisted ring**, silent: a hole's tip on its
    own exterior, where that exterior runs along the other input's edge, leaves a ring of the reverse
    difference (swapped, partitioning's difference) crossing itself within `h` at an unkept common
    chain, a lobe wound backwards (parity right, `get_area` off by twice the lobe). (2) **The
    symmetric difference's crossing rings**: in those 13, and in the hunt (39,313 valid polygon cases)
    37 as exported and 57 swapped, from the `six` and `zero` generators, with both differences clean.
    Found by an area-identity scan of every operation (i+d=P, i+r=Q, u+i=P+Q, x=d+r), which no
    committed check made (the area-sum oracle checks only the subject's i+d).
  - **Decided (developer)**: try to make the overlay's own symmetric difference sound (merged
    across shared edges, raising over a hemisphere) rather than return the two differences
    separately; with Fable, at xHigh (an agent first launched with no effort set was stopped and
    its draft taken over).
  - Fix (Fable, xHigh; reviewed by Opus against the code): `extract_rings` in parts
    (`pair_and_walk_rings`, `split_walked_rings`, `walk_rings`); a ring twisted between two vertices
    of an unkept chain is cut there (`rings_cut_at_chains`; geometric, but no new constant: the
    self-touch cut's `h` and `1e-12`); the symmetric difference is the two differences' rings joined
    across the walls between them, but for a wall walked as a spike (`spike_walls`). Hunt: crossing
    groups from valid inputs 37 / 57 to 0 / 0, area identities off 26 / 41 to 0 / 0, 10 cases
    differing from the prototype (the 9 known and `six#13 seed 11`, on HEAD too). Partitioning
    changes only where a twisted ring is cut (10 valid hunt cases, slivers of about `1.5 h^2`).
    Timing: partitioning's `extract` 2% to 5% slower, the symmetric difference's 10% to 26%.
  - Opus's checks: 217 of 217 GPlates CTests; the real-data test on Müller 2022 unchanged against PR
    B's record (282 polygons cut, 783 output rings, 5 under `h`); three hunt cases added to
    `prototype-hunt.txt` to pin mutations C7 (the crossing test's walk must stop: a hang without
    it), C12 and J8 (`shared#541 seed 2`, `zero#563 seed 11`, `shared#31 seed 14`); 7 of 35
    mutations survive, unpinned (report section 7). The session's scripts and scratch tests are in
    the planning repository's `plans/polygon-intersection-tools/cpp/prC/`.
  - **Decided (developer)**: at the end, archive this whole directory in the planning repository's
    `plans/done/` rather than only deleting it (step 9 of *PR C*).

- **Resume here** (updated 2026-11-13): **PR C under way**, in the steps of section *PR C* below
  (steps 0 to 2 done; next, step 3, from the session's drafts of the docstrings and the Python
  tests, which steps 3 and 5 still have to check against the built methods). PR C ends with the final Fable distillation of this
  directory, then archives it in the planning repository and deletes it here (step 9).
  After PR C: the `partition_features` rework (its own PR; it should also make pyGPlates' most
  overlapping plate match Assign Plate IDs, review 9's 1.2), then `DateLineWrapper` onto the
  overlay. **Tools**: the PR B session's scripts for the C++ (the hunt at scale and run comparison,
  mutation testing, `partition_features` timing, the Linux Docker builds including clang with FMA
  and UBSan) are in the planning repository's `plans/polygon-intersection-tools/cpp/` (README
  there, section `cpp/`); earlier entries here that cite "scratchpad" scripts mean that session's
  copies.
- *Resume here as of 2026-11-08:*
  - **PR A** is merged (#108). The branch carries PRs B and C; do not delete it.
  - **PR B**: Phase 5 under way, in the fifteen steps of `phase4b-design.md` section 12; steps 1 to 4
    and 4b (run 4's rule) 5 (anchorless chains) and 6 (repair, `InconsistentOverlayException`) are done (Status,
    2026-11-04). Review 2 done; steps 7 (linking) and 8 (grouping, orientation, hemisphere check) done.
    Review 3 (steps 7 and 8) and its fixes done, 2026-11-05; steps 9 (polylines, CitcomS) and 10
    (provenance) done, and review 4 with its fixes; step 11 (the partition layer) and review 5 with
    its fixes done; step 12 (the merge of `main`, coverages, area, Assign Plate IDs' failures)
    done, and review 6 with its fixes (the overlay's early stop dropped); step 13 (13a and 13b) and
    review 7 with its fixes done; step 14 (the sliver filter, on at `10 h` in the cascade, and the
    real-data measurements) done, 2026-11-07, and the two `partition_features` items from the
    `src/api` audit (Status, 2026-11-07); step 15's hunt at scale and its fixes done, 2026-11-08
    (Status), and a clean Linux run of the tip (`ce3324a79`: pyGPlates 7 of 7, GPlates 198 of 198, Ubuntu
    22.04); review 8 and its fixes done, and the distillation with its claim check (Phase 6; Status,
    2026-11-08); the full review by area and its fixes (Status, 2026-11-08). Next: a Fable review
    of those fixes, a Linux run, and the developer's visual check of Assign Plate IDs on polygons in
    a running GPlates (and its skipped-features dialog). Open: the prototype's polyline rule (`r5p`);
    the
    residue `shared#2288` seed 2 (design 9.1); the 4 second-step and 4 coincident-ring differences
    of the hunt (Status, 2026-11-08); for PR C, the symmetric difference's crossing pairing. The
    prototype configuration is now `r5o`. The prototype comparison is
    `PolygonOverlayPrototypeTest`; re-export with `overlay_export.py` when a step changes what the
    prototype is compared on.
  - *History (2026-10-27), kept for the record; steps 1 to 7 below are done (see the dated entries
    above).* Phases 0 to 2 are done and claim-checked. **Algorithm chosen: (b), the overlay**
    (developer, 2026-10-25). The walk-through of requirements §3.3 is complete
    (`overlay-walkthrough.md`); the developer reads it, then answers the 11 questions in
    `phase2-requirements.md` §4 (Q3 partly and Q11 already decided). Only **Q1** (thickness `t` or
    `s`) and **Q9** (derive chains in the partitioner) bear on PR A; the rest are for PR B.
  - **What is left for PR A** (Q1 = `s` and Q9 = derive, both decided 2026-10-28):
    1. The developer reviews `phase3-audit.md`.
    2. Apply its section 7 corrections to the design doc (§1, §4.2, §4.3) and the requirements, and
       record the decisions: `GeometryIntersect` is the primitive; `GreatCircleArc::intersect`
       stays separate for distance, with nothing ported (D5).
    3. Fixes F1, F2 and F4 to F7 (Opus; F3 withdrawn 2026-10-30; the developer reviewed the audit
       and approved these fixes on 2026-10-30, so step 1 is done): F1 the thickness and its
       comments; F2 order touch records by projection; F4 delete the dead code and state the
       `sin(phi) > t` invariant; F5 document the record positions; F6 ring structure for consumers,
       and which original vertices each record names (audit F6, 2026-10-30); F7 `std::stable_sort`.
    4. GoogleTests for `GeometryIntersect` (the audit's 16 cases, §6.1; `cpp-test` skill).
    5. The hunt: extend `plans/polygon-intersection-tools/` in the planning repository (re-fed
       pieces, interior rings, real-model shared edges, tiny and high-latitude geometries; §6.2),
       and run it to zero mismatches after the fixes.
    6. Topology resolving on a real model, before and after: every change under `s`, and no rise
       in multiple-intersection sections (Q1 raised the thickness); changelog entries in both
       products.
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
from the prototype. It goes in the plan, not the doc. Four steps (2026-10-31):
1. **Prototype run 3 (Fable):** the bundle rule, designed and built in the prototype, run to zero
   failures on run 2's residue and a wider dense shared-edge hunt; and polylines through the core,
   which the prototype has not yet run. Report `phase4a-run3.md`.
2. **Consumer survey (Opus, alongside step 1):** the code facts the design needs, checked against
   the code: every exterior-only path and coverage site below, the cascade, `partition()`'s
   contract in both products, `GLReconstructedStaticPolygonMeshes`, networks' rigid-block holes.
   Report `phase4b-survey.md`, ending in the developer's checkpoint questions (coverage policy and
   scope, slivers, `DateLineWrapper`).
3. **The design (Fable),** from the prototype, runs 1 to 3, the walk-through and the survey, with
   the developer's answers: `phase4b-design.md`, covering the bullets below.
4. **Opus review against the code**, then the developer's checkpoint.

The design covers:
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

PR B (#110) built the overlay. It computes all five operations (`PolygonOverlay::Operation`), and
the C++ oracle already checks each one: `check_oracle` in `PolygonOverlayTest.cc`, and
`UnionOverHemisphereThrows` for the union and the symmetric difference over a hemisphere. Only
partitioning (the intersection and the difference) is exposed. PR C exposes the four set
operations in pyGPlates, closes the symmetric difference's open pairing item (design page section
7), and ends the branch's plan with the final distillation. The next checklist item, *Audit the
plate partitioning*, follows; so does the roadmap's "revisit the exact parity area".

#### Decisions (developer, 2026-11-13)

1. There are four methods: `intersection`, `union`, `difference` and `symmetric_difference`. Each
   is `(other_polygon, sliver_width_threshold_radians=None)` and returns a `list` of
   `PolygonOnSphere`, which is empty when the result has no area. There are no `& | - ^`
   operators, since the result is a list.
2. Every failure raises **`PartitionError`**, including a union or symmetric difference that would
   exceed a hemisphere. Its docstring is broadened to say so.
3. **No coverage form** for any geometry-level entry point. Coverages stay with
   `partition_features` and Assign Plate IDs. This goes in the design page with its reason.
4. **`sliver_width_threshold_radians`** behaves as in `partition`: it defaults to `h` (about 9 m),
   zero keeps every piece, and a negative value or NaN raises `ValueError`.
5. (Proposed in the plan, approved with it.) `p.intersection(q)` and `p.difference(q)` **are**
   partitioning's inside and outside: `PolygonPartitioner::create(q, ADAPTIVE, sliver)` and then
   `partition_polygon(p, ...)`. So they return exactly the lists that `q.partition(p, inside,
   outside)` fills, including `p` itself when it only touches `q`, and the whole of `p` on one side
   when the sliver filter empties the other. A plain `extract` would leave `p` with a notch where
   the intersection had been dropped as a sliver, and the two methods would then disagree.
   `union` and `symmetric_difference` use `PolygonOverlay::Overlay` directly (`classify`, then
   `extract`).

#### Steps

0. This plan into PLAN.md (done, with this section). Then fast-forward the branch:
   `git merge --ff-only public/main` (`main` is this branch plus the #110 merge commit). (Done, by
   a rebase instead: the two plan commits after #110 were on `private` only, so the branch could not
   fast-forward; they were rebased onto `public/main` and force-pushed to `private`.)
1. **The symmetric difference's pairing** (`PolygonOverlay.cc`, `extract_rings`, the four-ray
   free-rule block at about `:4114-4150`). **Done 2026-11-13, otherwise than planned** (Status,
   2026-11-13; `prC-symdiff-twist.md`): the hypothesis below was wrong, and the fix is the cut at a
   chain and the join of the two differences. The symmetric difference uses the overlay's own
   operation (decision 1 stands, and so does its hemisphere error, decision 2). The plan as written:
   - Diagnose with `dump()` on the 13 `d4 ... hole-between` catalogue cases. My working
     hypothesis: the symmetric difference is the only operation that keeps all four ends at a
     crossing; in these cases the classes say "crossing" while the order used places each input's
     two rays next to each other; so the class-based pairing crosses in that order.
   - Likely fix: where all four ends are kept, pair consecutive kept rays in the order used (with
     `solve_vertex`, as the all-common branch already does). Of the three pairings at a vertex,
     both non-crossing ones give the same rings after the split (section 5.2).
   - Then drop the symmetric difference's exemption from the no-crossing-group checks
     (`PolygonOverlayTest.cc` `check_oracle`, about `:1660`; `PolygonOverlayPrototypeTest.cc`,
     about `:1166`), and the "can cross at a vertex" wording: `PolygonOverlay.cc` about `:4608`,
     and the design page section 5.2 (about lines 1441 and 1458).
   - Add a hand test that pins it. A mutation check: reverting the fix must fail a committed test.
     A hunt run (planning repo `plans/polygon-intersection-tools/cpp/`) must find 0 symmetric
     difference crossing groups from inputs whose rings don't cross.
   - If the diagnosis shows that the order itself is unreliable there, stop and bring the
     findings to the developer. The grouping already reads these cases correctly, so leaving them
     documented is a valid fallback.
2. **C++ entry point.** (Done 2026-11-13: `PolygonOverlay::create_polygon` and
   `extract_polygons`, also used by `PolygonPartitioner` and the real-data test, which each had a
   copy; test `ExtractPolygons`.) Move `create_polygon(const OutputPolygon &)` out of
   `PolygonPartitioner.cc`'s anonymous namespace into `PolygonOverlay.{h,cc}` as a public
   function, and use it in both places. Add `PolygonOverlay::extract_polygons(subject, clip,
   operation, options)`, which builds the overlay, classifies it, extracts the polygons and
   returns `std::vector<PolygonOnSphere::non_null_ptr_to_const_type>`. Add a short GoogleTest in
   `PolygonOverlayTest.cc` for each operation, including disjoint and nested inputs.
3. **pyGPlates methods** (`src/api/PyGeometriesOnSphere.cc`).
   - Factor the sliver-argument validation out of `polygon_on_sphere_partition` and share it.
   - Write one binding function per operation (or one parameterised by operation) and four
     `.def`s with `bp::arg("other_polygon")`.
   - Write the docstrings with the `api-docstring` skill. They cover: holes in the output, an
     empty list, touching inputs, the parity reading for inputs whose rings cross (as
     `partition`'s does), the hemisphere limit for union and symmetric difference (each ring means
     its smaller side), slivers, `.. versionadded:: 1.1`, and an example.
   - Broaden `PartitionError`'s docstring (`PyExceptions.cc:482`). In `partition`'s docstring, add
     one sentence pointing to `intersection` and `difference`.
4. **Stub.** Regenerate it with `generate_stub.py --output ... --check ...`, and confirm that
   `pygplates-stub-test` passes.
5. **Python tests** in `PolygonOnSphereCase` (`pygplates/test/test_maths/test_geometries_on_sphere.py`):
   - a point oracle for each method, on boxes with holes (grid samples away from the outlines,
     `is_point_in_polygon` of the outputs against `f(in p, in q)`);
   - disjoint, nested, identical, and sharing an edge (the union is one polygon, the intersection
     `[]`);
   - intersection and difference equal `partition`'s lists;
   - `PartitionError` for the union of the two 80-degree caps, and for the symmetric difference of
     the shared-edge pair (both from `UnionOverHemisphereThrows`);
   - the sliver argument (`ValueError`, zero keeps every piece);
   - `TypeError` for an argument that isn't a polygon.
6. **Changelog.** One pyGPlates bullet. There is no GPlates entry: Assign Plate IDs uses only
   the intersection and the difference, and step 1's cut at a chain changes those only in rare
   degenerate inputs of a feature not yet released (polygon partitioning is new in 1.1), so it
   needs no entry in either product.
7. **Design page** (`spherical-geometry.md`).
   - Section 5.1: the Boolean methods as a consumer layer, and decision 5.
   - Section 5.2: the pairing.
   - Section 5.6: `PartitionError` for the hemisphere case.
   - Section 5.7: no coverage form, with its reason.
   - Section 7: remove "The Boolean operations in pyGPlates". The self-crossing output ring's
     orientation stays deferred, under the self-intersection mode item.
   - If any include changes, regenerate the dependency matrix (`pygplates-source-closure-test`
     says so).
8. **Verify.**
   - Build both trees and run `ctest` on both.
   - Build `docs-pygplates` from scratch (`-W`), to check that the new docstrings render.
   - Do a Linux Docker run (the `gplates-local.md` recipe).
   - Push to `private`.
9. **Final distillation** of `docs/plans/polygon-intersections/`.
   - Run a fresh Fable agent (`model: "fable"`) with a self-contained prompt: what PR A and PR B
     already distilled (`prA-distillation.md`, `prB-distillation.md`), where each kind of content
     goes, and the rule that every statement must be checked against the code.
   - **Two overviews** (developer, 2026-11-13), part of the same distillation, written by Fable
     from the code, a few hundred lines each, beside their reference in
     `docs/design/architecture/foundation/`:
     - `polygon-overlay-overview.md`, for `spherical-geometry.md` section 5. Rewritten from
       `overlay-walkthrough.md`, which predates the code and is out of date (repair, the bundle
       and one-passage rules, the grouping of crossing rings, the sliver filter, the cut at a chain
       and the symmetric difference's join all came later), keeping its approach: the five steps,
       pieces, classifying a piece, common pieces and the sector rule, the keep table that gives
       every Boolean operation, pairing, the split and the grouping, each with an ASCII diagram.
       The generic case only: each part of the degenerate machinery (the thickness band, record
       identity, bundles, repair, the cut at a chain, spike walls) is named in a sentence, with a
       link to the reference section that has it.
     - `point-in-polygon-overview.md`, for section 3, the same way. Sources: the code, section 3,
       and the planning repository's point-in-polygon plan in `plans/done/` (#106, #107).
     - Each opens by naming its reference and saying that the reference and the code govern where
       they differ; the reference's section links back to it. Claim-checked with the rest. Not a
       simplified copy of the reference: an explainer may leave things out and give a page to one
       worked example, which the reference (complete and exact, for agents and maintainers) must
       not.
   - **The overview convention**, in this PR, since it brings the first overviews:
     - `AGENTS.md`, *Planning documents*: an explainer written for the developer while a branch is
       in progress (as `overlay-walkthrough.md` was) is a plan document. At distillation it is
       rewritten from the code into the design's overview where the reference has become too
       detailed to learn the design from (most of it degenerate-case handling, or a core idea that
       needs worked examples), and is otherwise archived with the plan. The developer may also ask
       for an overview of existing code, written from the code and claim-checked like an area
       page. *Architecture pages*: a pull request that changes a design with an overview checks
       the overview in the same pull request (the overview's narrow scope keeps that cheap).
     - `docs/design/architecture/README.md`: a short *How to read these documents* near the top
       (an area's page first; then a design's overview, where it has one, before its reference;
       the reference and the code are the authority); the three kinds of document (area pages,
       overviews, references); the naming (`<design>-overview.md` beside its reference, never a
       separate tree, so that whoever edits the reference sees it); and a list of the overviews.
     - The root `README.md`: one line pointing developers at `docs/design/architecture/README.md`
       (today it links only the versioning design).
     - Each area page lists its design documents with their kind. `foundation/README.md` exists
       only on `feature/architecture-diagrams`, so its links are made on that branch, which
       reaches `main` after this one (below).
     - Not now: splitting section 5 into its own `polygon-overlay.md`, which would pair the
       overview one to one but repoint the code's 270-odd citations of "section 5.x" again.
       Revisit if the page grows (eg `DateLineWrapper` onto the overlay).
   - **Decided (developer, 2026-11-13): `feature/architecture-diagrams` after pyGPlates 1.1**, so
     after this PR; the `foundation` page's links to the two overviews are made there. Its pages
     (every area's but the model's) await the developer's review, and it last merged `main` at #103:
     #104 to #110 changed `maths/` (39 files), `app-logic/` (24, plate partitioning), `api/` (8) and
     `opengl/` (6), so its `foundation` page (2026-10-18) still describes `PolylineIntersections`
     and polygons partitioned into polylines, and its `topologies`, `python-bindings` and
     `opengl` pages need checking. Never merge it into this branch directly: this PR would then
     carry its pages.
   - I review its output. Then a Fable claim check of the new text runs, serially, with a pause
     for the 5-hour limit between Fable agents.
   - **Archive, then delete** (developer, 2026-11-13): copy the whole of
     `docs/plans/polygon-intersections/` (every plan, report and review file, as it stands after
     the distillation) into the planning repository as `plans/done/polygon-intersections/`, and move
     its tools beside it (`plans/polygon-intersection-tools/`, which holds the prototype, the hunts'
     scripts and `cpp/`, including `cpp/prC/`) to `plans/done/polygon-intersection-tools/`, fixing
     any path that points at the old place (its README, and `plans/README.md` or the roadmap if they
     cite it). Commit and push there first (the planning repository's update sequence). Then delete
     the directory from the branch in its own commit, whose message names the archive. The GPlates
     repository keeps only the distilled content; the archive is the complete record of the effort.
   - Update the roadmap row in the planning repository. Also note, against the docs E PR
     (primer, partitioning section), that it should mention the Boolean methods.
10. **The PR** when the developer asks. Then comes the `partition_features` rework.

Commits: the overlay fix (the cut at a chain and the symmetric difference's join, with their tests
and the design page's account of them, step 1); the entry point; the Python methods (docstrings,
stub, tests, changelog); the design page; the distillation; the plan's deletion (after the
archive).

#### Verification

- GPlates `gplates-unit-test`: `PolygonOverlayTest` (all operations, now with no exemption for
  the symmetric difference), `PolygonOverlayPrototypeTest` and the new entry-point test.
- pyGPlates: `pygplates-test` (the new cases), `pygplates-stub-test`,
  `pygplates-source-closure-test` and `pygplates-linkage-test`.
- The hunt: 0 symmetric difference crossing groups from non-crossing inputs, and no new
  differences elsewhere.
- Linux Docker: both suites green. The docs build cleanly from scratch.

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
