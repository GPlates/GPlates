# Phase 4a, run 1: the overlay core on the catalogue

Run on 2026-10-29. A Python prototype of the overlay core (`overlay-walkthrough.md` sections 2-4, 6, 9
and 14) on `arc_ref.py`'s exact records, run on a catalogue of hard configurations against a
point-sampling oracle, to decide how the overlay must handle several records at or near one point.
Tools, in the planning repository's `plans/polygon-intersection-tools/`: `overlay_proto.py` (the
core), `overlay_oracle.py` (the oracle), `overlay_catalogue.py` (the catalogue and the runner). The
whole run:

```
cd <planning>/plans/polygon-intersection-tools
python -I overlay_catalogue.py --markdown catalogue.md      # 470 cases, 8 variant/rule columns, 181 s
python -I overlay_catalogue.py --case '<name>' --variant V1 --mode free --verbose   # one case, in detail
```

Every statement below is marked **measured** (from the run, with the command that reproduces it) or
**reasoning** (an interpretation of what was measured). Hand analysis of this subject has been wrong
twice; nothing here was decided by hand.

## 1. What was run

**Settings** (the decided future state of `GeometryIntersect`), measured as applied in
`overlay_proto.py`:

- Thickness `h = s = 1.4141979e-6` rad sets both the plane thickness and the coincidence radius
  (`arc_ref.T_COS_F`, `T_SINE2_F` overridden; fix F1). The zero-length bound stays `1e-6`.
- Touch records (`SL`, `LS`) are keyed along the segment by the vertex's projection onto the
  segment's great circle, exactly (fix F2); crossings keep `arc_ref.along_key`, which is already the
  projection. Equal keys stay tied.
- Every tie order was run. The largest tie group anywhere was two records (two exact ties at most, so
  at most two orders per case); the sampling path for more than 120 combinations was never taken
  (`catalogue.md`, column `ties`).
- Seven cases fell inside `arc_ref`'s ambiguity bands (a decision within rounding of a threshold)
  and were skipped: `wedge 60deg r=+2h` (straight and four bends; `2h` is exactly the wedge's reach
  at 60 degrees) and `shared neighbours d=+1h` (plain and interleaved; coincidence at exactly `h`).
  463 cases ran.

**The core**, as implemented (`overlay_proto.py`, docstring and class comments):

- Vertices by variant. **V0**: every record its own vertex. **V1**: records at the same original
  vertex of either polygon (by record type and segment index, union-find; no distance test) are one
  vertex; nothing else merged. **V2**: V1, then every maximal set of vertices joined by pieces that
  have no original vertex and are shorter than `2h` is one cluster, kept as its member list; those
  pieces are internal (not classified, not rays, not output); the cluster's rays are the remaining
  piece ends, each with its own record position and plane. **V2b** (added during the run, section
  4.6): V2, and every other single arc whose two ends are in one cluster, or at one vertex, is
  internal whatever its length. Nothing drops a record in any variant.
- Pieces: each ring split at its record stops in the sorted order; records at the same point of a
  ring that are the same overlay vertex are one stop (so V1 never has a zero-length piece between the
  two `SL` records of a wedge, while V0 does). A ring with no record is one closed piece.
- Classification: a piece with an original vertex is tested at its first one by exact parity
  (`pip_ref.parity`, threshold off; every other original vertex of the piece is also tested as a
  diagnostic, `piece_side_disagreements`); then the vertex walks deduce what they can, to a fixed
  point; a single-arc piece still unclassified is tested at its midpoint and the walks rerun; a common
  pair (one `P` piece and one `Q` piece, both single arcs between the same two vertices) is one ray
  flipping both memberships and gets `same`/`opposite` from the sector walk at either end.
- The vertex walk: rays in exact anticlockwise order about the vertex (orientation signs on the exact
  plane tangents `cross(n, v)`, never a difference of positions); the two unknowns (the memberships
  of sector 0) are solved against every constraint a classified end imposes (a `P` end fixes `in Q`
  on both its sides, a `Q` end `in P`, a classified common ray `in P xor in Q`); no solution is a
  check failure; a unique solution deduces the unclassified ends. **Rule `rotation`** uses this walk
  everywhere. **Rule `free`** replaces it at a vertex with exactly four non-common ends, two of each
  polygon, by the geometry-free rule of PLAN.md 2026-10-28 (P crosses iff its two classes differ,
  which must equal whether Q's differ; the fourth class follows from three; pairing by sector
  memberships at a crossing, same-polygon at a bounce), and uses the walk at every other vertex.
- Checks (walk-through 9.1) are the walks rerun after classification; repair (9.3) flips the least
  reliable class at a failing vertex (midpoint, then deduced, then common, then original-vertex tests
  shortest first) when a single flip makes the vertex consistent, up to twenty rounds.
- Linking: kept ends from the keep-table (walk-through 14); two kept rays pair with no geometry;
  more pair across output sectors (`free`: by memberships or same-polygon at four-end vertices);
  rings walked; the split at a repeated vertex (6.4); clean-up last: consecutive repeated positions
  removed and counted, rings with fewer than three distinct positions dropped and counted, tiny-area
  rings counted but kept. Output positions: the vertex's `P` original vertex if it holds one, else a
  `Q` one, else the record position.

**The oracle** (`overlay_oracle.py`): 300 points over the disc holding both inputs plus 300 in an
annulus (radius `M` to `100 M`, log-uniform) round every cluster of records (single linkage within
`4h`), all farther than `M` from both inputs' boundaries; each must be inside the output (exact
parity over all output rings together) exactly when `f(in P, in Q)` (exact parity on the inputs).
`M = max(10h, 2 x widest record cluster)`; the widest record cluster in the catalogue was `3h`, so
**`M = 10h` in every case** (`catalogue.md`, column `M`). The reference point is 0.37 rad off the
antipode of the inputs' centre: a symmetric case put a `Q` vertex exactly at the antipode, where
every parity path is coplanar with it (found and fixed during the run). Oracle and checks were
confirmed to fire: the `P & Q` output tested as `P \ Q` fails 339 of 900 points, and one flipped
class in the two-squares control gives 158 failures and two check failures (measured, scratch run).

## 2. Variants, in brief

Cases with at least one of each outcome, over the 463 cases, worst tie order (measured: the
`Summary` table printed by `overlay_catalogue.py`; `cross` is a proper crossing between output
edges, which the oracle cannot see).

| variant / rule | oracle fail | check fail | repairs | unresolved | degenerate dropped | tie-dependent | gf vs rotation | output crossings |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| V0 / rotation | 0 | 13 | 0 | 0 | 37 | 87 | 13 | 23 |
| V0 / free | 0 | 0 | 0 | 0 | 55 | 90 | 34 | 23 |
| V1 / rotation | **29** | 42 | 4 | 0 | 87 | 7 | 17 | 23 |
| V1 / free | **19** | 19 | 0 | 0 | 87 | 5 | 23 | 23 |
| V2 / rotation | **5** | 7 | 0 | 0 | 65 | 0 | 2 | 9 |
| V2 / free | **5** | 5 | 0 | 0 | 65 | 0 | 2 | 9 |
| V2b / rotation | 0 | 2 | 0 | 0 | 60 | 0 | 2 | 9 |
| **V2b / free** | **0** | **0** | **0** | 0 | 60 | 0 | 2 | 9 |

Per family (`O` oracle, `C` check, `D` degenerate dropped, `T` tie-dependent output, `X` output
crossings; counts of cases):

| family | n | V0/rot | V0/free | V1/rot | V1/free | V2/rot | V2/free | V2b/rot | V2b/free |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| control | 4 | clean | clean | clean | clean | clean | clean | clean | clean |
| 1 acute wedge | 115 | D20 T30 | D28 T38 | D50 | D50 | D54 | D54 | D54 | D54 |
| 2, 3 right wedge (+swapped) | 28 | D6 T12 | D12 T9 | D12 | D12 | D6 | D6 | D6 | D6 |
| 4, 5 twist (edge, vertex) | 35 | T14 X21 | T14 X21 | T5 X21 | T5 X21 | X9 | X9 | X9 | X9 |
| 6 D4 | 49 | C13 D3 T11 | D3 T11 | O8 C21 D3 | O8 C8 D3 | O5 C5 D5 | O5 C5 D5 | clean | clean |
| 7 neck | 17 | T10 | T10 | clean | clean | clean | clean | clean | clean |
| 8 touching rings | 8 | X2 | X2 | X2 | X2 | clean | clean | clean | clean |
| 9 SS clusters | 15 | T4 | T4 | O11 C11 D10 | O11 C11 D10 | clean | clean | clean | clean |
| 10 shared edges | 22 | clean | clean | clean | clean | clean | clean | clean | clean |
| 10 T-intersection | 12 | clean | clean | clean | clean | clean | clean | clean | clean |
| 11 near vertex | 128 | D8 T6 | D12 T4 | O10 C10 D12 T2 | D12 | C2 | clean | C2 | clean |
| 12 tangent rays | 30 | clean | clean | clean | clean | clean | clean | clean | clean |

Classification statistics, whole catalogue, every tie order (measured, scratch script over
`run_overlay`): under V2b/free, 1968 original-vertex tests, 334 deductions, **0 midpoint tests**, 0
undefined tests, 108 common pairs, 0 ambiguous common pairs (several candidate partners), 0
anchorless chains, 195 clusters, 389 internal pieces, 0 rays from a zero-length segment's plane, 0
pieces whose original vertices disagree, 0 unresolved vertices. Under V1/free: 35 midpoint tests, 4
undefined tests (a midpoint exactly on a `Q` vertex), 58 ambiguous common pairs, 32 zero-length-plane
rays, 38 unresolved vertices. Under V0/free: 84 midpoint tests.

## 3. How to read the flags

- **Degenerate dropped (`D`)** is not a failure: every dropped ring in V2b is a zero-area sliver
  within the margin. In the acute wedge it is `Q`'s tip piece `[x, v, x]` (two distinct positions);
  in the right-angle wedge `far` cases likewise; in the `LL LL` wedge cases (`P` cuts the corner,
  two crossings `1.18h` apart on one `Q` edge) it is the triangle `P & Q` that contraction removed
  (measured: `--case 'wedge 60deg r=+2.2h bend WN' --variant V2 --mode rotation --verbose`). That
  last one is contraction's known cost: the point set moves by less than the cluster width.
- **Tie-dependent (`T`)** means the cleaned output rings differed between the two tie orders. Under
  V0 and V1 every such case still passed the oracle in both orders: the difference is ring structure
  (a zero-area spur `x, v, x` inside one ring in one order and a separate dropped ring in the other;
  or, at the exact twist, which operation gets the pinched ring, section 5.4). V2 and V2b are never
  tie-dependent.
- **Output crossings (`X`)** are the bow-tie's own twist carried into the output (walk-through
  section 8), counted whenever two non-adjacent output edges cross. V0 and V1 keep it in 21 twist
  cases; V2 and V2b in 9, because within `2h` of `Q` the two crossings contract to one vertex and the
  lobes then touch instead of cross. The two `touching rings` crossings under V0/V1 are the same
  effect at a hole apex `0.3h` and `0.8h` above the exterior's vertex on `Q`'s edge, both on the
  edge; V2 contracts them.
- **Check failures (`C`)** without oracle failures are inconsistencies confined to pieces inside the
  tolerance band; with oracle failures they are the real thing.

## 4. Findings by family

### 4.1 Acute wedge (115 cases: 10, 30, 60 degrees; `r` from `0.5h` to `1.5 x h / sin(theta/2)`; straight and four bends)

Measured records: `SS` for `r <= 0.8h` (coincident with `v`); two `SL` at `x` for `r` from `1.2h` up
to the reach (`3.48h` at 30 degrees, `10.3h` at 10 degrees); `LL LL` beyond it (`P` cuts the corner).
The bends towards `v` (`WN`, `NW`) add an `LS` (`v` on `P`'s edge) at small `r` and the bends away
(`WS`, `SW`) an `LL` on `Q`'s far edge. No oracle failure in any variant, rule or order; no check
failure; no repair (`overlay_catalogue.py --family wedge`).

At the two-`SL` vertex under V1 (one vertex, six ends: `P` in, `P` out, `Q`'s tip piece twice, `Q`'s
outer piece twice), the sector walk is fully determined and consistent, the tip piece is classified
at `v`, and `P & Q` comes out as `Q` minus its tip with the sliver `x, v, x` dropped
(`--case 'wedge 30deg r=+3h straight' --variant V1 --mode free --verbose`).

### 4.2 Right-angle wedge, both roles (28 cases: `a` from `0.5h` to `1.2h`, `near` and `far` arrival)

Measured records: `SS` for `a <= 0.6h`; **`SL SL LS`** (the third record, `v` on `P`'s arriving
segment) for `a = 0.72h` to `0.95h` with `near` arrival, `SL SL` with `far`; `LS` alone from
`a = 1.05h` (plus an `LL` on `Q`'s far edge in every case). No oracle failure in any variant. Under V1
the three short pieces between the `x` vertex and the `v` vertex (two of `Q` round the corner, one of
`P`, each under `1.4h`) are classified by deduction with no check failure; under V2/V2b they are one
cluster of width `a sqrt 2`. V0 is tie-dependent in 12 cases (ring structure only).

### 4.3 Twist near `Q` (35 cases: edge offsets `-10h` to `+10h`, ulps either side of 0, and 0; a `Q` corner within `3h` of `X`)

Measured: **no record ever appears at `X`** in any case; the records are `LL LL LL LL` for the edge
(the two middle crossings converging on `X`, separation `2|d|`), and `LL LS LS` when `Q`'s corner is
within `0.5h` of `X` (the corner on both diagonals, an identity vertex of four `P` ends under V1). At
`d = 0` the two crossings have exactly equal keys along `Q` (an exact tie, two orders) and exactly
coincident positions; at `+-1 ulp` and `+-4 ulp` they are distinct and already swapped. No oracle
failure in any variant, rule or order. V0 and V1 keep the two vertices and the zero-length `Q` piece
between them, whose class is deduced (not midpoint-tested), and each order gives a valid output:
one order has `P & Q` as one pinched ring (six positions, through `X` twice) and `P \ Q` as two
triangles, the other the reverse (`--case 'twist edge d=+0h' --variant V1 --mode free --verbose`).
V2/V2b contract the pair into one six-end vertex whenever `2|d| < 2h` and give the two-triangles
output for both operations, independent of order.

### 4.4 D4 (49 cases: segment `0.5h` to `3h` at 90, 45 and 20 degrees; lone; a `P` hole apex between the foots; a `Q` vertex between the foots)

Measured records: lone and hole cases `SL SL` (+`LL`s elsewhere) for every length up to `3h`: the
segment's end points are within `h` of `Q`'s plane at 20 degrees even at `3h`, so a short segment
crossing an edge is two touches and no crossing well beyond `2h` at shallow angles. The `Q`-vertex
case gives `SS SS` for length `<= 1.9h` (both foots coincident with `Q`'s bend vertex) and
`LS LS SL SL` beyond.

- **Lone**: clean everywhere (a common pair, the sector rule).
- **`Q` vertex between, `SS SS`**: V1 fails the oracle in all 8 cases (698 of 900 points), with check
  failures the repair cannot fix. Mechanism, section 4.6. V0 and V2/V2b pass.
- **Hole between**: V0/rotation and V1/rotation report a check failure in all 13 cases, with no
  oracle failure and no repair found; V0/free and V1/free are clean. Measured at
  `--case 'd4 len=+1h phi=45 hole-between' --variant V0 --mode rotation --verbose`: at the lower foot
  the rotational order is `P Q P Q` (a crossing) while both `P` classes there are `inside Q` (the
  deduced short piece and the continuing piece whose vertex is inside `Q`) and both `Q` classes
  `outside P`: a bounce. No single class flip makes that vertex consistent, because the sub-tolerance
  piece crosses `Q`'s edge in its middle and has no class that satisfies the rotational order at both
  of its ends. Reasoning: this is the "snapped topology against unsnapped directions" case of the
  brief, at an ordinary four-end vertex; the geometry-free rule accepts the snapped topology.
- **Hole between, length `2.1h` to `3h`, V2**: the oracle fails (514-516 points) in all 5 cases. The
  two `Q` pieces foot-apex-foot are each under `2h`, so the three vertices are one cluster of width
  `2.1h` to `3h`, but `P`'s segment between the foots is longer than `2h`, so under V2's literal rule
  it is a loop leaving the cluster and returning, with rays collinear with `P`'s through-going rays
  (`--case 'd4 len=+2.5h phi=45 hole-between' --variant V2 --mode free --verbose`: eight rays, check
  failure, empty `P & Q`). V2b makes it internal and is clean.

### 4.5 Narrow neck (17), touching rings (8), shared edges (22), T-intersection (12), tangent rays (30)

All clean in every variant and rule (measured, the families `neck`, `touching-rings`, `shared`,
`shared` (T-intersection rows), `tangent`). Notes:

- Neck: a `P` vertex in a neck narrower than `2h` gives two `SL` on different `Q` edges, an identity
  vertex of two `P` ends and four `Q` ends; the exterior-and-hole neck gives two `SL` on different
  `Q` rings. Both resolved by the sector walk (V1) or as a cluster (V2).
- Shared edges: interleaved chains of up to seven records are chains of common pairs; every chain
  got the same class from both ends; the interior chain vertices (two common rays) pair their two
  kept rays without memberships. Offsets `1.5h` and above give no records (the copies are separate
  pieces, which is right).
- The T-intersection at `y_C = 1.1h`, `delta` from `-0.9h` to `+0.9h`: one `SL` at `B`; `A`-`B` is
  tested at `A`; the four-end vertex at `B` is consistent under both rules in all 12 cases.
- Tangent rays: see section 5.12.

### 4.6 SS clusters (15) and the V1 failure mechanism

Measured records: `LL SS SS` for two `Q` vertices `0.3h` to `1.5h` apart near a `P` vertex and for two
`P` vertices `0.8h` or `1.5h` apart at a `Q` vertex; a zero-length `P` segment at a `Q` vertex gives
`SS SS` and on a `Q` edge `SL SL` (plus a spurious-looking `LS`: `Q`'s corner on `P`'s segment
arriving at the zero-length pair).

**V1 fails** 11 of the 15 (oracle 222-294 of 900; check failure; pairing failure), and all 19 of its
failures in the catalogue are `LL SS SS` cases. Mechanism, measured at
`--case 'ss two Q vertices d=+0.8h x=(+0.4h,+0.3h)' --variant V1 --mode free --verbose`: the two `SS`
records share `P`'s vertex `x`, so they are one identity vertex; `Q`'s `0.8h` edge between its two
coincident vertices becomes a closed single-arc piece with both ends at that vertex and no original
vertex. Its two rays (along the short edge, from the two ends) are `1.13e-6` rad from `Q`'s through
rays, its class can only come from a midpoint test (its midpoint lies on `P`), and the six-ray walk
is inconsistent. V0 instead has two vertices with a zero-length `P` piece and `Q`'s short edge
between the same two: a common pair, decided by the sector rule, clean in both orders (tie-dependent
ring structure only). V2/V2b contract the loop (any single arc from a vertex to itself is internal)
and are clean.

### 4.7 Near vertex (128 cases: `P` vertex `1.05h` to `2h` from `Q`'s corner, eight directions, four edge pairs)

Measured records: from `LL LS SL` (a touch each way) to `LL LS SL SL SL` depending on direction;
the `psi = 315` cases with `P` arriving from the north or west put a `P` edge parallel to a `Q` edge
within `h`: `LS` of `v` on it, two `SL` of `x` on `Q`'s edges, three short pieces, two `Q` pieces
between the same two vertices (so no common pair: `common_ambiguous`), and two rays whose exact
angle is `1.1e-20` rad at the four-end vertex `v`.

- **V1/rotation fails the oracle in 10 cases** (`rho = 1.05h` and `1.2h`, `psi = 45, 225, 315`),
  with check failures; four of them hit the twenty-round repair cap without ever flipping an
  original-vertex test, and still failed. **V1/free passes all 128**: at the four-end vertex the class rule does not
  look at the two collinear rays, and the deduction through the three short pieces is consistent.
- **V2 and V2b** pass the oracle in all 128. Under the rotational rule the two `rho = 2h` cases with
  `W-S` and `N-E` edges report a check failure: `P` passes along `Q`'s edge for `2h` and turns south
  `2h` inside it; the `LS` at `v` and the `SL` at `x` form a cluster (the `2h` common pair between
  them is internal) whose four rays leave two different points, `P`'s southward ray from `x` and
  `Q`'s southward ray from `v` parallel; the order about the centroid comes out `P P Q Q` (a
  bounce) while the classes say a crossing (`--case 'near vertex rho=+2h psi=0 W-S' --variant V2
  --mode rotation --verbose`). The free rule is consistent, and the output is right under both
  (two kept rays pair without geometry).

## 5. The claims

1. **Acute wedge: the two `LS`/`SL` records at `x` are the two crossings `P` makes at larger `r`,
   collapsed; they must not be dropped; V1 handles them; V0 fails when `P` bends at `x` in one tie
   order.** Collapsed crossings: **confirmed** (measured, section 4.1: `SL SL` up to the reach,
   `LL LL` beyond it, at all three angles). Must not be dropped: **confirmed** by experiment
   (measured, scratch script monkey-patching `intersect_rekeyed` to delete one of the pair, V1/free):
   dropping the first-sorted record gives 6 to 157 oracle failures of 900 in 15 of 25 cases, the other
   record 0; the checks report nothing in every case (`Q`'s ring is then one piece, classified at
   one vertex, consistent and wrong), while the all-vertices diagnostic flags the piece in three of
   four runs looked at. V1 handles them: **confirmed** (no failure in 115 cases). **V0 fails when `P`
   bends: refuted**: V0 passed the oracle and the checks in every wedge case under both rules and
   both orders; its only order dependence is where the zero-area spur `x, v, x` ends up (section 3).
   Reasoning: the hand analysis presumably used the fictitious direction of the zero-length piece in
   the rotational order; with the geometry-free rule the zero-length piece's class is deduced and
   never ordered, and under the rotational rule the exact sort happened to be consistent in these
   cases (it is not in the D4 hole cases, section 4.4).
2. **Right-angle wedge with the third record is the hard case** (snapped topology against unsnapped
   directions). **Not confirmed as a failure**: no variant failed it (section 4.2). The suspected
   mechanism does bite, but elsewhere: in the parallel-edge near-vertex cases under V1/rotation
   (section 4.7) and the D4 hole cases under both V0 and V1 with the rotational rule (section 4.4).
3. **Roles swapped**: same result (measured, `right-wedge-swapped`, 14 cases, clean).
4. **Twist near an edge: no record at `X`; two `LL` converge, coincide and swap; without contraction
   each is an ordinary crossing and either order gives a valid output with the stated topologies.**
   **Confirmed** in full (measured, section 4.3). The two outputs at the exact tie are exactly the
   two topologies described.
5. **Twist near a vertex of `Q`**: confirmed likewise, including the corner exactly on `X` (two `LS`
   at one `Q` vertex, a four-`P`-end identity vertex, clean in all variants).
6. **D4**: a lone short segment is a common chain and the sector rule decides it (confirmed, all 21
   lone cases clean); with a record between the foots it is not common, and then: V1 fails when the
   foots coincide with a `Q` vertex (the loop mechanism), the rotational rule is inconsistent at the
   foot with a hole apex between (no oracle failure), V2's `2h` threshold fails when the cluster is
   wider than `2h` and V2b fixes it (measured, section 4.4).
7. **Narrow neck**: no variant fails (measured).
8. **Exterior and hole touching at a vertex on a `Q` edge**: no variant fails; V0/V1 keep the hole
   apex's crossing of the exterior as an output crossing when the apex is `0.3h` or `0.8h` off the
   exterior vertex (measured, section 3).
9. **SS clusters**: V1 fails, V0 and V2/V2b pass (measured, section 4.6).
10. **Shared edges and the T-intersection**: all clean (measured, section 4.5).
11. **`P` vertex `h` to `2h` from a `Q` vertex**: V1 with the rotational rule fails 10 of 128; every
    other variant/rule passes the oracle (measured, section 4.7).
12. **Tangent rays: nearly tangent rays occur only along common chains, so the rotational order
    outside clusters is well conditioned.** Measured (`tangent` family, and a scratch script
    printing `min_ray_angle_plain` for every case): for `|alpha| <= 0.8 h/L` the `P` edge is a
    common pair with `Q`'s edge (one ray; the `P`/`Q` direction gap is `alpha`); for
    `|alpha| >= 1.2 h/L` it is a separate ray at angle `alpha`. So in this family the claim holds and
    the smallest angle ordered between separate rays is `1.2 h/L`: `1.7e-4` rad at `L = 0.01`,
    `1.7e-3` at `L = 0.001`. **Across the whole catalogue the claim is refuted for V0 and V1**: the
    smallest plain (non-common, non-cluster) angle is `1.1e-20` rad (`near vertex rho=+1.05h psi=315
    N-E`, a `P` edge parallel to a `Q` edge `0.74h` away, the two pieces between `v` and `x` not a
    common pair because `Q` has two pieces there), then `1.5e-7` to `2.7e-6` in the `ss` and D4
    `Q`-vertex cases, then `2.8e-5` (`T-intersection yC=+1.1h delta=-0.9h`). All of those below
    `2.8e-5` are inside clusters under V2/V2b, where the rays are excluded from "plain" by
    definition, **except the narrow neck**: with a `P` vertex between two exactly parallel `Q` edges
    under `2h` apart (ten `neck` cases, both the neck and the exterior-against-hole form), the
    identity vertex has six ends and two `Q` rays at exactly zero angle (one from each side of the
    neck, pointing the same way), and no cluster forms because there is one vertex. The exact sort
    breaks the tie by its half-plane rule, the walk was consistent and the oracle passed in all ten,
    but in doubles that order is a coin toss. After those, the smallest angle V2b orders outside a
    cluster or a common ray is `2.8e-5` rad (the T-intersection, `A`-`B` against `Q`'s top edge at
    `B`), then `1.6e-4` (T-intersection `y_C = 2h`) and `1.7e-4` (tangent). Reasoning: `2.8e-5` rad
    between exact plane tangents is far above double rounding, so the order is well conditioned
    there; the ill-conditioned orders sit inside clusters, or at a six-end vertex where two `Q`
    edges pass within `2h` of each other, which contraction by piece length does not see (a case
    for run 2, section 6).

**Geometry-free rule against the rotational order.** The two disagree (a four-end vertex where the
rotational order's alternation differs from whether `P`'s classes differ) in 34 cases under V0, 23
under V1 and 2 under V2/V2b (measured, `gf_vs_rotation`). Wherever they disagree the geometry-free
rule is the one consistent with the oracle: every V1/rotation oracle failure and every rotational
check failure without an oracle failure (D4 hole, near-vertex clusters) is at such a vertex, and the
free rule never failed a check at a four-end vertex (`gf_check_failures = 0` under V0 and V2b).
Reasoning: at a four-end vertex the classes come from original vertices or from deductions anchored
on them, and the records already encode the snapped topology; the ray directions describe the
unsnapped one and can contradict it when two rays are within the tolerance of each other.

**Deduce-first or test-first.** Under V2b no midpoint test was needed in 463 cases: every single-arc
piece outside a cluster was deduced from an end vertex. Under V1, 35 midpoint tests, four of them
undefined (the midpoint exactly a `Q` vertex), and under V0, 84 (measured, section 2). Reasoning:
deduce first; a midpoint test is only ever reached when a vertex is underdetermined, which did not
happen once clusters absorb the sub-tolerance pieces.

**Repairs.** V2b needed none. V1/rotation repaired in four cases (to the twenty-round cap, never
flipping an original-vertex test) and still failed them; the rotational-rule D4 hole failures found
no single flip that would help (measured). Reasoning: the repair policy of 9.3 is not a safety net
for a wrong vertex rule, and single flips cannot fix a sub-tolerance piece that has no consistent
class.

## 6. What run 2 should focus on

- **V2b/free is the candidate**: zero oracle failures, zero check failures, zero repairs, no
  order dependence, no midpoint test, in 463 cases. Run 2 should try to break it with the random
  cluster generator and perturbation runs (rounding is not in `arc_ref.py`): clusters wider than
  `3h` (chains of sub-`2h` pieces), clusters that swallow a ring (a tiny hole entirely within `2h` of
  a boundary), two clusters within `2h` of each other, and rays leaving a cluster nearly parallel
  from different member points, which the rotational order got wrong twice here (section 4.7) at
  vertices the free rule covered because they had four ends; a six-end cluster with the same geometry
  has no geometry-free rule yet.
- **Contraction's cost**: the `LL LL` wedge cases lose a triangle of width `1.18h`; measure the
  largest cluster width in a cascade (Q8), and whether a cluster can be wider than any fixed bound.
- **The neck**: two edges of one polygon within `2h` of each other with a vertex of the other
  between them give exactly parallel rays at one six-end vertex (section 5.12); the order between
  them is undefined in doubles. Decide whether such a vertex needs a geometry-free treatment (the
  zero-width sector between the two rays is the neck itself) or a merge of the two `Q` passes.
- **The rotational order inside clusters** is still used for six or more ends. Either extend the
  geometry-free reasoning (a parity solve over the cluster's rays, V3) or measure how often a
  cluster has six or more rays and whether its rays are ever within `1e-3` rad.
- **Zero-length segments**: a `P` segment under `1e-6` on a `Q` edge produced an unexpected `LS`
  (`Q`'s corner on the arriving segment) and four rays from zero-length planes under V1; V2b
  absorbed it. Check the generator covers both vertices of such a segment landing in different
  clusters.
- **Anchorless chains (Q3) and cutting by an open arc**: not exercised (0 anchorless chains).
- **The all-vertices diagnostic** (every original vertex of a piece tested) caught a dropped record
  the checks missed; decide whether the C++ keeps it as a debug assertion.
- Rewrite walk-through section 7 from these results: a twist on `Q` needs no contraction (4.3); what
  needs it is a single arc from a vertex or cluster back to itself (4.6) and any sub-`2h` piece whose
  rays would otherwise be ordered against a parallel ray (4.7); the geometry-free rule at four-end
  vertices (section 5, last paragraphs).

## 7. Review addendum (2026-10-29, the session's review of this run)

- **Reproduced**: `python -I overlay_catalogue.py` gives the summary table of section 2 (one
  difference: V1/rotation tie-dependent in 9 cases on the rerun, 7 above).
- **V1b, added in review**: V1, and a single arc (no original vertex) from a vertex back to itself
  is internal; no contraction by length. Every V1 failure is such a loop (section 4.6), and an arc
  between two records of one identity vertex is at most `2h` long by construction (both lie within
  `h` of the same original vertex), so the rule needs no distance threshold. Measured
  (`python -I overlay_catalogue.py --variant V1b`): **V1b/free 0 oracle, 0 check, 0 repairs, 0
  unresolved, 5 tie-dependent (ring structure only), 23 output crossings (the twists, as V0/V1)**;
  V1b/rotation fails the oracle in 10 (the parallel-ray near-vertex cases, as V1/rotation).
- **Midpoint tests** (measured, a scratch script over `run_overlay`, rule `free`, every tie order):
  V0 84 in 42 cases; V1b 13 in 13 cases, all `d4 ... hole-between`; V2b 0.
- **Midpoint tests inverted** (measured, every midpoint result negated, as if rounding had decided
  it the other way; `free` rule over the whole catalogue): V0 and V1b still 0 oracle, 0 check, 0
  repairs. The pieces that reach a midpoint test are underdetermined by their vertices, and either
  class is within the tolerance.
- So in this catalogue **no contraction by distance is needed**: V0/free and V1b/free are clean
  without one, and V2b/free is clean with one. The geometry-free four-end rule is what every clean
  variant has in common; the rotational order fails somewhere under every variant but V2b. Run 2
  should compare V0/free, V1b/free and V2b/free under the random generator and perturbations, with
  the midpoint inversion as a standing adversary, and prefer the simplest that survives.

## Appendix: every case

Columns: records found (kinds); number of tie orders; `M`; one cell per variant/rule. `.` is clean.
`O` oracle failures (worst order, all three operations summed), `C` check failures, `R` repairs, `U`
unresolved pieces, `D` degenerate rings dropped, `P` pairing failures, `X` output edge crossings, `M`
undefined midpoint tests, `T` output differs between tie orders. Generated by
`python -I overlay_catalogue.py --markdown catalogue.md`.

| case | records | ties | M | V0/rot | V0/fre | V1/rot | V1/fre | V2/rot | V2/fre | V2b/rot | V2b/fre |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| control two squares | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| control Q inside P |  | 1 | 10h | . | . | . | . | . | . | . | . |
| control hole crossed only | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| control apex on edge (6.4) | LS | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 10deg r=+0.5h straight | SS | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 10deg r=+0.5h bend WS | LL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 10deg r=+0.5h bend SW | LL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 10deg r=+0.5h bend WN | SS | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 10deg r=+0.5h bend NW | SS | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 10deg r=+0.8h straight | SS | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 10deg r=+0.8h bend WS | LL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 10deg r=+0.8h bend SW | LL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 10deg r=+0.8h bend WN | SS | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 10deg r=+0.8h bend NW | SS | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 10deg r=+1.2h straight | SL SL | 2 | 10h | T | T | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 10deg r=+1.2h bend WS | LL SL SL | 2 | 10h | D1 T | D1 T | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 10deg r=+1.2h bend SW | LL SL SL | 2 | 10h | D1 T | D1 T | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 10deg r=+1.2h bend WN | LS SL SL | 2 | 10h | . | D1 T | D1 | D1 | . | . | . | . |
| wedge 10deg r=+1.2h bend NW | LS SL SL | 2 | 10h | . | D1 T | D1 | D1 | . | . | . | . |
| wedge 10deg r=+2h straight | SL SL | 2 | 10h | T | T | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 10deg r=+2h bend WS | LL SL SL | 2 | 10h | D1 T | D1 T | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 10deg r=+2h bend SW | LL SL SL | 2 | 10h | D1 T | D1 T | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 10deg r=+2h bend WN | SL SL | 2 | 10h | . | . | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 10deg r=+2h bend NW | SL SL | 2 | 10h | . | . | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 10deg r=+3h straight | SL SL | 2 | 10h | T | T | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 10deg r=+3h bend WS | LL SL SL | 2 | 10h | D1 T | D1 T | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 10deg r=+3h bend SW | LL SL SL | 2 | 10h | D1 T | D1 T | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 10deg r=+3h bend WN | SL SL | 2 | 10h | . | . | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 10deg r=+3h bend NW | SL SL | 2 | 10h | . | . | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 10deg r=+10.3h straight | SL SL | 2 | 10h | T | T | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 10deg r=+10.3h bend WS | LL SL SL | 2 | 10h | D1 T | D1 T | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 10deg r=+10.3h bend SW | LL SL SL | 2 | 10h | D1 T | D1 T | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 10deg r=+10.3h bend WN | SL SL | 2 | 10h | . | . | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 10deg r=+10.3h bend NW | SL SL | 2 | 10h | . | . | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 10deg r=+12.6h straight | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 10deg r=+12.6h bend WS | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 10deg r=+12.6h bend SW | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 10deg r=+12.6h bend WN | LL LL | 1 | 10h | . | . | . | . | D1 | D1 | D1 | D1 |
| wedge 10deg r=+12.6h bend NW | LL LL | 1 | 10h | . | . | . | . | D1 | D1 | D1 | D1 |
| wedge 10deg r=+17.2h straight | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 10deg r=+17.2h bend WS | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 10deg r=+17.2h bend SW | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 10deg r=+17.2h bend WN | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 10deg r=+17.2h bend NW | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 30deg r=+0.5h straight | SS | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 30deg r=+0.5h bend WS | LL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 30deg r=+0.5h bend SW | LL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 30deg r=+0.5h bend WN | SS | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 30deg r=+0.5h bend NW | SS | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 30deg r=+0.8h straight | SS | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 30deg r=+0.8h bend WS | LL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 30deg r=+0.8h bend SW | LL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 30deg r=+0.8h bend WN | SS | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 30deg r=+0.8h bend NW | SS | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 30deg r=+1.2h straight | SL SL | 2 | 10h | T | T | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 30deg r=+1.2h bend WS | LL SL SL | 2 | 10h | D1 T | D1 T | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 30deg r=+1.2h bend SW | LL SL SL | 2 | 10h | D1 T | D1 T | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 30deg r=+1.2h bend WN | LS SL SL | 2 | 10h | . | D1 T | D1 | D1 | . | . | . | . |
| wedge 30deg r=+1.2h bend NW | LS SL SL | 2 | 10h | . | D1 T | D1 | D1 | . | . | . | . |
| wedge 30deg r=+2h straight | SL SL | 2 | 10h | T | T | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 30deg r=+2h bend WS | LL SL SL | 2 | 10h | D1 T | D1 T | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 30deg r=+2h bend SW | LL SL SL | 2 | 10h | D1 T | D1 T | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 30deg r=+2h bend WN | SL SL | 2 | 10h | . | . | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 30deg r=+2h bend NW | SL SL | 2 | 10h | . | . | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 30deg r=+3h straight | SL SL | 2 | 10h | T | T | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 30deg r=+3h bend WS | LL SL SL | 2 | 10h | D1 T | D1 T | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 30deg r=+3h bend SW | LL SL SL | 2 | 10h | D1 T | D1 T | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 30deg r=+3h bend WN | SL SL | 2 | 10h | . | . | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 30deg r=+3h bend NW | SL SL | 2 | 10h | . | . | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 30deg r=+3.48h straight | SL SL | 2 | 10h | T | T | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 30deg r=+3.48h bend WS | LL SL SL | 2 | 10h | D1 T | D1 T | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 30deg r=+3.48h bend SW | LL SL SL | 2 | 10h | D1 T | D1 T | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 30deg r=+3.48h bend WN | SL SL | 2 | 10h | . | . | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 30deg r=+3.48h bend NW | SL SL | 2 | 10h | . | . | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 30deg r=+4.25h straight | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 30deg r=+4.25h bend WS | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 30deg r=+4.25h bend SW | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 30deg r=+4.25h bend WN | LL LL | 1 | 10h | . | . | . | . | D1 | D1 | D1 | D1 |
| wedge 30deg r=+4.25h bend NW | LL LL | 1 | 10h | . | . | . | . | D1 | D1 | D1 | D1 |
| wedge 30deg r=+5.8h straight | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 30deg r=+5.8h bend WS | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 30deg r=+5.8h bend SW | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 30deg r=+5.8h bend WN | LL LL | 1 | 10h | . | . | . | . | D1 | D1 | D1 | D1 |
| wedge 30deg r=+5.8h bend NW | LL LL | 1 | 10h | . | . | . | . | D1 | D1 | D1 | D1 |
| wedge 60deg r=+0.5h straight | SS | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 60deg r=+0.5h bend WS | LL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 60deg r=+0.5h bend SW | LL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 60deg r=+0.5h bend WN | SS | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 60deg r=+0.5h bend NW | SS | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 60deg r=+0.8h straight | SS | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 60deg r=+0.8h bend WS | LL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 60deg r=+0.8h bend SW | LL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 60deg r=+0.8h bend WN | SS | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 60deg r=+0.8h bend NW | SS | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 60deg r=+1.2h straight | SL SL | 2 | 10h | T | T | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 60deg r=+1.2h bend WS | LL SL SL | 2 | 10h | D1 T | D1 T | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 60deg r=+1.2h bend SW | LL SL SL | 2 | 10h | D1 T | D1 T | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 60deg r=+1.2h bend WN | LS SL SL | 2 | 10h | . | D1 T | D1 | D1 | . | . | . | . |
| wedge 60deg r=+1.2h bend NW | LS SL SL | 2 | 10h | . | D1 T | D1 | D1 | . | . | . | . |
| wedge 60deg r=+1.8h straight | SL SL | 2 | 10h | T | T | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 60deg r=+1.8h bend WS | LL SL SL | 2 | 10h | D1 T | D1 T | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 60deg r=+1.8h bend SW | LL SL SL | 2 | 10h | D1 T | D1 T | D1 | D1 | D1 | D1 | D1 | D1 |
| wedge 60deg r=+1.8h bend WN | LS SL SL | 2 | 10h | . | D1 T | D1 | D1 | . | . | . | . |
| wedge 60deg r=+1.8h bend NW | LS SL SL | 2 | 10h | . | D1 T | D1 | D1 | . | . | . | . |
| wedge 60deg r=+2h straight | ambiguous: height at threshold | | |  |  |  |  |  |  |  |  |
| wedge 60deg r=+2h bend WS | ambiguous: height at threshold | | |  |  |  |  |  |  |  |  |
| wedge 60deg r=+2h bend SW | ambiguous: height at threshold | | |  |  |  |  |  |  |  |  |
| wedge 60deg r=+2h bend WN | ambiguous: height at threshold | | |  |  |  |  |  |  |  |  |
| wedge 60deg r=+2h bend NW | ambiguous: height at threshold | | |  |  |  |  |  |  |  |  |
| wedge 60deg r=+2.2h straight | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 60deg r=+2.2h bend WS | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 60deg r=+2.2h bend SW | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 60deg r=+2.2h bend WN | LL LL | 1 | 10h | . | . | . | . | D1 | D1 | D1 | D1 |
| wedge 60deg r=+2.2h bend NW | LL LL | 1 | 10h | . | . | . | . | D1 | D1 | D1 | D1 |
| wedge 60deg r=+3h straight | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 60deg r=+3h bend WS | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 60deg r=+3h bend SW | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 60deg r=+3h bend WN | LL LL | 1 | 10h | . | . | . | . | D1 | D1 | D1 | D1 |
| wedge 60deg r=+3h bend NW | LL LL | 1 | 10h | . | . | . | . | D1 | D1 | D1 | D1 |
| wedge 60deg r=+3h straight | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 60deg r=+3h bend WS | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 60deg r=+3h bend SW | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| wedge 60deg r=+3h bend WN | LL LL | 1 | 10h | . | . | . | . | D1 | D1 | D1 | D1 |
| wedge 60deg r=+3h bend NW | LL LL | 1 | 10h | . | . | . | . | D1 | D1 | D1 | D1 |
| right wedge a=+0.5h near | LL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| right wedge a=+0.5h far | LL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| right wedge a=+0.6h near | LL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| right wedge a=+0.6h far | LL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| right wedge a=+0.72h near | LL LS SL SL | 2 | 10h | T | D1 | D1 | D1 | . | . | . | . |
| right wedge a=+0.72h far | LL SL SL | 2 | 10h | D1 T | D1 T | D1 | D1 | D1 | D1 | D1 | D1 |
| right wedge a=+0.85h near | LL LS SL SL | 2 | 10h | T | D1 | D1 | D1 | . | . | . | . |
| right wedge a=+0.85h far | LL SL SL | 2 | 10h | D1 T | D1 T | D1 | D1 | D1 | D1 | D1 | D1 |
| right wedge a=+0.95h near | LL LS SL SL | 2 | 10h | T | D1 | D1 | D1 | . | . | . | . |
| right wedge a=+0.95h far | LL SL SL | 2 | 10h | D1 T | D1 T | D1 | D1 | D1 | D1 | D1 | D1 |
| right wedge a=+1.05h near | LL LS | 1 | 10h | . | . | . | . | . | . | . | . |
| right wedge a=+1.05h far | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| right wedge a=+1.2h near | LL LS | 1 | 10h | . | . | . | . | . | . | . | . |
| right wedge a=+1.2h far | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| right wedge a=+0.5h near swapped | LL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| right wedge a=+0.5h far swapped | LL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| right wedge a=+0.6h near swapped | LL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| right wedge a=+0.6h far swapped | LL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| right wedge a=+0.72h near swapped | LL LS LS SL | 2 | 10h | T | D1 T | D1 | D1 | . | . | . | . |
| right wedge a=+0.72h far swapped | LL LS LS | 2 | 10h | D1 T | D1 T | D1 | D1 | D1 | D1 | D1 | D1 |
| right wedge a=+0.85h near swapped | LL LS LS SL | 2 | 10h | T | D1 T | D1 | D1 | . | . | . | . |
| right wedge a=+0.85h far swapped | LL LS LS | 2 | 10h | D1 T | D1 T | D1 | D1 | D1 | D1 | D1 | D1 |
| right wedge a=+0.95h near swapped | LL LS LS SL | 2 | 10h | T | D1 T | D1 | D1 | . | . | . | . |
| right wedge a=+0.95h far swapped | LL LS LS | 2 | 10h | D1 T | D1 T | D1 | D1 | D1 | D1 | D1 | D1 |
| right wedge a=+1.05h near swapped | LL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| right wedge a=+1.05h far swapped | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| right wedge a=+1.2h near swapped | LL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| right wedge a=+1.2h far swapped | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| twist edge d=-10h | LL LL LL LL | 1 | 10h | X1 | X1 | X1 | X1 | X1 | X1 | X1 | X1 |
| twist edge d=-3h | LL LL LL LL | 1 | 10h | X1 | X1 | X1 | X1 | X1 | X1 | X1 | X1 |
| twist edge d=-1h | LL LL LL LL | 1 | 10h | X1 | X1 | X1 | X1 | . | . | . | . |
| twist edge d=-0.3h | LL LL LL LL | 1 | 10h | X1 | X1 | X1 | X1 | . | . | . | . |
| twist edge d=-0.1h | LL LL LL LL | 1 | 10h | X1 | X1 | X1 | X1 | . | . | . | . |
| twist edge d=-0.000707h | LL LL LL LL | 1 | 10h | X1 | X1 | X1 | X1 | . | . | . | . |
| twist edge d=-4ulp | LL LL LL LL | 1 | 10h | X1 | X1 | X1 | X1 | . | . | . | . |
| twist edge d=-1ulp | LL LL LL LL | 1 | 10h | X1 | X1 | X1 | X1 | . | . | . | . |
| twist edge d=+0h | LL LL LL LL | 2 | 10h | T | T | T | T | . | . | . | . |
| twist edge d=+1ulp | LL LL LL LL | 1 | 10h | X1 | X1 | X1 | X1 | . | . | . | . |
| twist edge d=+4ulp | LL LL LL LL | 1 | 10h | X1 | X1 | X1 | X1 | . | . | . | . |
| twist edge d=+0.000707h | LL LL LL LL | 1 | 10h | X1 | X1 | X1 | X1 | . | . | . | . |
| twist edge d=+0.1h | LL LL LL LL | 1 | 10h | X1 | X1 | X1 | X1 | . | . | . | . |
| twist edge d=+0.3h | LL LL LL LL | 1 | 10h | X1 | X1 | X1 | X1 | . | . | . | . |
| twist edge d=+1h | LL LL LL LL | 1 | 10h | X1 | X1 | X1 | X1 | . | . | . | . |
| twist edge d=+3h | LL LL LL LL | 1 | 10h | X1 | X1 | X1 | X1 | X1 | X1 | X1 | X1 |
| twist edge d=+10h | LL LL LL LL | 1 | 10h | X1 | X1 | X1 | X1 | X1 | X1 | X1 | X1 |
| twist vertex w=(+0h,+0h) | LL LS LS | 2 | 10h | T | T | . | . | . | . | . | . |
| twist vertex w=(+7.85e-11h,+0h) | LL LS LS | 2 | 10h | T | T | . | . | . | . | . | . |
| twist vertex w=(+0h,-7.85e-11h) | LL LS LS | 2 | 10h | T | T | . | . | . | . | . | . |
| twist vertex w=(+0.5h,+0h) | LL LS LS | 2 | 10h | T | T | . | . | . | . | . | . |
| twist vertex w=(+0h,+0.5h) | LL LS LS | 2 | 10h | T | T | . | . | . | . | . | . |
| twist vertex w=(+0h,-0.5h) | LL LS LS | 2 | 10h | T | T | . | . | . | . | . | . |
| twist vertex w=(-0.5h,+0h) | LL LS LS | 2 | 10h | T | T | . | . | . | . | . | . |
| twist vertex w=(+0.5h,+0.5h) | LL LS LS | 2 | 10h | T | T | . | . | . | . | . | . |
| twist vertex w=(-0.5h,-0.5h) | LL LS LS | 2 | 10h | T | T | . | . | . | . | . | . |
| twist vertex w=(+1.5h,+0h) | LL LL | 1 | 10h | X1 | X1 | X1 | X1 | X1 | X1 | X1 | X1 |
| twist vertex w=(+0h,+1.5h) | LL LL LL LL | 2 | 10h | T | T | T | T | . | . | . | . |
| twist vertex w=(+0h,-1.5h) | LL LL | 1 | 10h | X1 | X1 | X1 | X1 | X1 | X1 | X1 | X1 |
| twist vertex w=(-1.5h,+0h) | LL LL LL LL | 2 | 10h | T | T | T | T | . | . | . | . |
| twist vertex w=(+1.5h,-0.3h) | LL LS | 1 | 10h | X1 | X1 | X1 | X1 | X1 | X1 | X1 | X1 |
| twist vertex w=(+3h,+0h) | LL LL | 1 | 10h | X1 | X1 | X1 | X1 | X1 | X1 | X1 | X1 |
| twist vertex w=(+0h,+3h) | LL LL LL LL | 2 | 10h | T | T | T | T | . | . | . | . |
| twist vertex w=(-3h,+0h) | LL LL LL LL | 2 | 10h | T | T | T | T | . | . | . | . |
| twist vertex w=(+0h,-3h) | LL LL | 1 | 10h | X1 | X1 | X1 | X1 | X1 | X1 | X1 | X1 |
| d4 len=+0.5h phi=90 lone | LL SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| d4 len=+0.5h phi=45 lone | LL SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| d4 len=+0.5h phi=45 hole-between | LL LL SL SL SL | 1 | 10h | C1 | . | C1 | . | . | . | . | . |
| d4 len=+0.5h phi=45 Qvertex-between | LL SS SS | 2 | 10h | T | T | O698 C2 P6 | O698 C2 P6 | . | . | . | . |
| d4 len=+0.5h phi=20 lone | LL SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| d4 len=+0.5h phi=20 hole-between | LL LL SL SL SL | 1 | 10h | C1 | . | C1 | . | . | . | . | . |
| d4 len=+0.5h phi=20 Qvertex-between | LL SS SS | 2 | 10h | T | T | O698 C2 P6 | O698 C2 P6 | . | . | . | . |
| d4 len=+1h phi=90 lone | LL SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| d4 len=+1h phi=45 lone | LL SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| d4 len=+1h phi=45 hole-between | LL LL SL SL SL | 1 | 10h | C1 | . | C1 | . | . | . | . | . |
| d4 len=+1h phi=45 Qvertex-between | LL SS SS | 2 | 10h | T | T | O698 C2 P6 | O698 C2 P6 | . | . | . | . |
| d4 len=+1h phi=20 lone | LL SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| d4 len=+1h phi=20 hole-between | LL LL SL SL SL | 1 | 10h | C1 | . | C1 | . | . | . | . | . |
| d4 len=+1h phi=20 Qvertex-between | LL SS SS | 2 | 10h | T | T | O698 C2 P6 | O698 C2 P6 | . | . | . | . |
| d4 len=+1.5h phi=90 lone | LL SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| d4 len=+1.5h phi=45 lone | LL SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| d4 len=+1.5h phi=45 hole-between | LL LL SL SL SL | 1 | 10h | C1 | . | C1 | . | . | . | . | . |
| d4 len=+1.5h phi=45 Qvertex-between | LL SS SS | 2 | 10h | T | T | O698 C2 P6 | O698 C2 P6 | . | . | . | . |
| d4 len=+1.5h phi=20 lone | LL SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| d4 len=+1.5h phi=20 hole-between | LL LL SL SL SL | 1 | 10h | C1 | . | C1 | . | . | . | . | . |
| d4 len=+1.5h phi=20 Qvertex-between | LL SS SS | 2 | 10h | T | T | O698 C2 P6 | O698 C2 P6 | . | . | . | . |
| d4 len=+1.9h phi=90 lone | LL SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| d4 len=+1.9h phi=45 lone | LL SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| d4 len=+1.9h phi=45 hole-between | LL LL SL SL SL | 1 | 10h | C1 | . | C1 | . | . | . | . | . |
| d4 len=+1.9h phi=45 Qvertex-between | LL SS SS | 2 | 10h | T | T | O698 C2 P6 | O698 C2 P6 | . | . | . | . |
| d4 len=+1.9h phi=20 lone | LL SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| d4 len=+1.9h phi=20 hole-between | LL LL SL SL SL | 1 | 10h | C1 | . | C1 | . | . | . | . | . |
| d4 len=+1.9h phi=20 Qvertex-between | LL SS SS | 2 | 10h | T | T | O697 C2 P6 | O697 C2 P6 | . | . | . | . |
| d4 len=+2.1h phi=90 lone | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| d4 len=+2.1h phi=45 lone | LL SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| d4 len=+2.1h phi=45 hole-between | LL LL SL SL SL | 1 | 10h | C1 | . | C1 | . | O515 C2 D2 P12 | O515 C2 D2 P12 | . | . |
| d4 len=+2.1h phi=45 Qvertex-between | LL LS LS SL SL | 2 | 10h | D1 T | D1 T | D1 | D1 | . | . | . | . |
| d4 len=+2.1h phi=20 lone | LL SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| d4 len=+2.1h phi=20 hole-between | LL LL SL SL SL | 1 | 10h | C1 | . | C1 | . | O515 C2 D2 P12 | O515 C2 D2 P12 | . | . |
| d4 len=+2.1h phi=20 Qvertex-between | LL LS LS SL SL | 2 | 10h | D1 T | D1 T | D1 | D1 | . | . | . | . |
| d4 len=+2.5h phi=90 lone | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| d4 len=+2.5h phi=45 lone | LL SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| d4 len=+2.5h phi=45 hole-between | LL LL SL SL SL | 1 | 10h | C1 | . | C1 | . | O516 C2 D2 P12 | O516 C2 D2 P12 | . | . |
| d4 len=+2.5h phi=45 Qvertex-between | LL LS SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| d4 len=+2.5h phi=20 lone | LL SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| d4 len=+2.5h phi=20 hole-between | LL LL SL SL SL | 1 | 10h | C1 | . | C1 | . | O515 C2 D2 P12 | O515 C2 D2 P12 | . | . |
| d4 len=+2.5h phi=20 Qvertex-between | LL LS LS SL SL | 2 | 10h | D1 T | D1 T | D1 | D1 | . | . | . | . |
| d4 len=+3h phi=90 lone | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| d4 len=+3h phi=45 lone | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| d4 len=+3h phi=45 hole-between | LL LL LL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| d4 len=+3h phi=45 Qvertex-between | LL LS | 1 | 10h | . | . | . | . | . | . | . | . |
| d4 len=+3h phi=20 lone | LL SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| d4 len=+3h phi=20 hole-between | LL LL SL SL SL | 1 | 10h | C1 | . | C1 | . | O514 C2 D2 P12 | O514 C2 D2 P12 | . | . |
| d4 len=+3h phi=20 Qvertex-between | LL LS SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| neck w=+0.5h apex | LL LL SL SL | 2 | 10h | T | T | . | . | . | . | . | . |
| neck w=+0.5h apex-offset | LL LL SL SL | 2 | 10h | T | T | . | . | . | . | . | . |
| neck w=+1h apex | LL LL SL SL | 2 | 10h | T | T | . | . | . | . | . | . |
| neck w=+1h apex-offset | LL LL SL SL | 2 | 10h | T | T | . | . | . | . | . | . |
| neck w=+1.5h apex | LL LL SL SL | 2 | 10h | T | T | . | . | . | . | . | . |
| neck w=+1.5h apex-offset | LL LL LL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| neck w=+1.9h apex | LL LL SL SL | 2 | 10h | T | T | . | . | . | . | . | . |
| neck w=+1.9h apex-offset | LL LL LL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| neck w=+2.5h apex | LL LL LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| neck w=+2.5h apex-offset | LL LL LL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| neck w=+4h apex | LL LL LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| neck w=+4h apex-offset | LL LL LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| neck hole d=+0.5h | LL LL SL SL | 2 | 10h | T | T | . | . | . | . | . | . |
| neck hole d=+1h | LL LL SL SL | 2 | 10h | T | T | . | . | . | . | . | . |
| neck hole d=+1.5h | LL LL SL SL | 2 | 10h | T | T | . | . | . | . | . | . |
| neck hole d=+2h | LL LL SL SL | 2 | 10h | T | T | . | . | . | . | . | . |
| neck hole d=+3h | LL LL LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| touching rings eps=+0h | LL LL SL SL | 2 | 10h | . | . | . | . | . | . | . | . |
| touching rings eps=+0h hole-above | LL LL SL SL | 2 | 10h | . | . | . | . | . | . | . | . |
| touching rings eps=+0.3h | LL LL SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| touching rings eps=+0.3h hole-above | LL LL SL SL | 1 | 10h | X1 | X1 | X1 | X1 | . | . | . | . |
| touching rings eps=+0.8h | LL LL SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| touching rings eps=+0.8h hole-above | LL LL SL SL | 1 | 10h | X1 | X1 | X1 | X1 | . | . | . | . |
| touching rings eps=+1.2h | LL LL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| touching rings eps=+1.2h hole-above | LL LL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| ss two Q vertices d=+0.3h x=(+0.15h,+0.3h) | LL SS SS | 2 | 10h | . | . | O292 C1 D2 P4 | O292 C1 D2 P4 | . | . | . | . |
| ss two Q vertices d=+0.3h x=(+0.15h,-0.3h) | LL SS SS | 2 | 10h | . | . | O222 C1 P2 | O222 C1 P2 | . | . | . | . |
| ss two Q vertices d=+0.3h x=(+0.15h,+0h) | LL SS SS | 2 | 10h | . | . | O292 C1 D2 P4 | O292 C1 D2 P4 | . | . | . | . |
| ss two Q vertices d=+0.8h x=(+0.4h,+0.3h) | LL SS SS | 2 | 10h | . | . | O293 C1 D2 P4 | O293 C1 D2 P4 | . | . | . | . |
| ss two Q vertices d=+0.8h x=(+0.4h,-0.3h) | LL SS SS | 2 | 10h | . | . | O222 C1 P2 | O222 C1 P2 | . | . | . | . |
| ss two Q vertices d=+0.8h x=(+0.4h,+0h) | LL SS SS | 2 | 10h | . | . | O293 C1 D2 P4 | O293 C1 D2 P4 | . | . | . | . |
| ss two Q vertices d=+1.5h x=(+0.75h,+0.3h) | LL SS SS | 2 | 10h | . | . | O294 C1 D2 P4 | O294 C1 D2 P4 | . | . | . | . |
| ss two Q vertices d=+1.5h x=(+0.75h,-0.3h) | LL SS SS | 2 | 10h | . | . | O223 C1 P2 | O223 C1 P2 | . | . | . | . |
| ss two Q vertices d=+1.5h x=(+0.75h,+0h) | LL SS SS | 2 | 10h | . | . | O294 C1 D2 P4 | O294 C1 D2 P4 | . | . | . | . |
| ss zero-length P seg 0.2h at Q vertex | LL SS SS | 2 | 10h | T | T | D1 | D1 | . | . | . | . |
| ss zero-length P seg 0.2h on Q edge | LS SL SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| ss zero-length P seg 0.6h at Q vertex | LL SS SS | 2 | 10h | T | T | D1 | D1 | . | . | . | . |
| ss zero-length P seg 0.6h on Q edge | LS SL SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| ss two P vertices +0.8h apart at Q vertex | LL SS SS | 2 | 10h | T | T | O355 C1 D1 P4 M1 | O355 C1 D1 P4 M1 | . | . | . | . |
| ss two P vertices +1.5h apart at Q vertex | LL SS SS | 2 | 10h | T | T | O355 C1 D1 P4 M1 | O355 C1 D1 P4 M1 | . | . | . | . |
| shared neighbours d=+0h | SS SS | 1 | 10h | . | . | . | . | . | . | . | . |
| shared inside d=+0h | LS LS | 1 | 10h | . | . | . | . | . | . | . | . |
| shared neighbours d=+0h interleaved | LS LS LS SL SL SS SS | 1 | 10h | . | . | . | . | . | . | . | . |
| shared inside d=+0h interleaved | LS LS LS SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| shared neighbours d=+0.5h | SS SS | 1 | 10h | . | . | . | . | . | . | . | . |
| shared inside d=+0.5h | LS LS | 1 | 10h | . | . | . | . | . | . | . | . |
| shared neighbours d=+0.5h interleaved | LS LS LS SL SL SS SS | 1 | 10h | . | . | . | . | . | . | . | . |
| shared inside d=+0.5h interleaved | LS LS LS SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| shared neighbours d=+1h | ambiguous: coincidence at threshold | | |  |  |  |  |  |  |  |  |
| shared inside d=+1h | LS LS | 1 | 10h | . | . | . | . | . | . | . | . |
| shared neighbours d=+1h interleaved | ambiguous: coincidence at threshold | | |  |  |  |  |  |  |  |  |
| shared inside d=+1h interleaved | LS LS LS SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| shared neighbours d=+1.5h |  | 1 | 10h | . | . | . | . | . | . | . | . |
| shared inside d=+1.5h |  | 1 | 10h | . | . | . | . | . | . | . | . |
| shared neighbours d=+1.5h interleaved |  | 1 | 10h | . | . | . | . | . | . | . | . |
| shared inside d=+1.5h interleaved |  | 1 | 10h | . | . | . | . | . | . | . | . |
| shared neighbours d=+2h |  | 1 | 10h | . | . | . | . | . | . | . | . |
| shared inside d=+2h |  | 1 | 10h | . | . | . | . | . | . | . | . |
| shared neighbours d=+2h interleaved |  | 1 | 10h | . | . | . | . | . | . | . | . |
| shared inside d=+2h interleaved |  | 1 | 10h | . | . | . | . | . | . | . | . |
| shared neighbours d=+3h |  | 1 | 10h | . | . | . | . | . | . | . | . |
| shared inside d=+3h |  | 1 | 10h | . | . | . | . | . | . | . | . |
| shared neighbours d=+3h interleaved |  | 1 | 10h | . | . | . | . | . | . | . | . |
| shared inside d=+3h interleaved |  | 1 | 10h | . | . | . | . | . | . | . | . |
| T-intersection yC=+1.1h delta=-0.9h | SL | 1 | 10h | . | . | . | . | . | . | . | . |
| T-intersection yC=+1.1h delta=+0h | SL | 1 | 10h | . | . | . | . | . | . | . | . |
| T-intersection yC=+1.1h delta=+0.5h | SL | 1 | 10h | . | . | . | . | . | . | . | . |
| T-intersection yC=+1.1h delta=+0.9h | SL | 1 | 10h | . | . | . | . | . | . | . | . |
| T-intersection yC=+2h delta=-0.9h | SL | 1 | 10h | . | . | . | . | . | . | . | . |
| T-intersection yC=+2h delta=+0h | SL | 1 | 10h | . | . | . | . | . | . | . | . |
| T-intersection yC=+2h delta=+0.5h | SL | 1 | 10h | . | . | . | . | . | . | . | . |
| T-intersection yC=+2h delta=+0.9h | SL | 1 | 10h | . | . | . | . | . | . | . | . |
| T-intersection yC=+10h delta=-0.9h | SL | 1 | 10h | . | . | . | . | . | . | . | . |
| T-intersection yC=+10h delta=+0h | SL | 1 | 10h | . | . | . | . | . | . | . | . |
| T-intersection yC=+10h delta=+0.5h | SL | 1 | 10h | . | . | . | . | . | . | . | . |
| T-intersection yC=+10h delta=+0.9h | SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.05h psi=0 W-E | LL LS SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.05h psi=0 W-S | LL LS SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.05h psi=0 NW-SE | LL LS SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.05h psi=0 N-E | SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.05h psi=45 W-E | LL LS SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.05h psi=45 W-S | LS LS SL SL | 2 | 10h | D1 T | D1 T | O140 C1 D3 P5 | D1 | . | . | . | . |
| near vertex rho=+1.05h psi=45 NW-SE | SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.05h psi=45 N-E | SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.05h psi=90 W-E | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.05h psi=90 W-S | LS SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.05h psi=90 NW-SE | LS SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.05h psi=90 N-E |  | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.05h psi=135 W-E | LL LS SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.05h psi=135 W-S | LS SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.05h psi=135 NW-SE | LS SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.05h psi=135 N-E | LS SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.05h psi=180 W-E | LL LS SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.05h psi=180 W-S |  | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.05h psi=180 NW-SE | LL LS | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.05h psi=180 N-E | LS SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.05h psi=225 W-E | LL LS SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.05h psi=225 W-S | SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.05h psi=225 NW-SE | SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.05h psi=225 N-E | LS LS SL SL | 2 | 10h | D1 T | D1 T | O128 C1 D3 P4 T | D1 | . | . | . | . |
| near vertex rho=+1.05h psi=270 W-E | LL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.05h psi=270 W-S | SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.05h psi=270 NW-SE | LS SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.05h psi=270 N-E | LL LS SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.05h psi=315 W-E | LL LS SL SL SL | 2 | 10h | . | D1 | O151 C1 D4 P5 | D1 | . | . | . | . |
| near vertex rho=+1.05h psi=315 W-S | LS SL SL SL | 2 | 10h | D1 | D1 | O374 C2 R20 D3 P12 | D1 | . | . | . | . |
| near vertex rho=+1.05h psi=315 NW-SE | LS SL SL SL | 2 | 10h | T | D1 | D1 | D1 | . | . | . | . |
| near vertex rho=+1.05h psi=315 N-E | LS SL SL SL | 2 | 10h | D1 | D1 | O422 C2 R20 D3 P12 | D1 | . | . | . | . |
| near vertex rho=+1.2h psi=0 W-E | LL LS SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.2h psi=0 W-S | LL LS SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.2h psi=0 NW-SE | LL LS SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.2h psi=0 N-E | SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.2h psi=45 W-E | LL LS SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.2h psi=45 W-S | LS LS SL SL | 2 | 10h | D1 T | D1 T | O154 C1 D3 P5 | D1 | . | . | . | . |
| near vertex rho=+1.2h psi=45 NW-SE | SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.2h psi=45 N-E | SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.2h psi=90 W-E | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.2h psi=90 W-S | LS SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.2h psi=90 NW-SE | LS SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.2h psi=90 N-E |  | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.2h psi=135 W-E | LL LS SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.2h psi=135 W-S | LS SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.2h psi=135 NW-SE | LS SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.2h psi=135 N-E | LS SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.2h psi=180 W-E | LL LS SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.2h psi=180 W-S |  | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.2h psi=180 NW-SE | LL LS | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.2h psi=180 N-E | LS SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.2h psi=225 W-E | LL LS SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.2h psi=225 W-S | SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.2h psi=225 NW-SE | SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.2h psi=225 N-E | LS LS SL SL | 2 | 10h | D1 T | D1 T | O149 C1 D3 P5 T | D1 | . | . | . | . |
| near vertex rho=+1.2h psi=270 W-E | LL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.2h psi=270 W-S | SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.2h psi=270 NW-SE | LS SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.2h psi=270 N-E | LL LS SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.2h psi=315 W-E | LL LS SL SL SL | 2 | 10h | . | D1 | O151 C1 D4 P5 | D1 | . | . | . | . |
| near vertex rho=+1.2h psi=315 W-S | LS SL SL SL | 2 | 10h | D1 | D1 | O374 C2 R20 D3 P12 | D1 | . | . | . | . |
| near vertex rho=+1.2h psi=315 NW-SE | LS SL SL SL | 2 | 10h | T | D1 | D1 | D1 | . | . | . | . |
| near vertex rho=+1.2h psi=315 N-E | LS SL SL SL | 2 | 10h | D1 | D1 | O422 C2 R20 D3 P12 | D1 | . | . | . | . |
| near vertex rho=+1.5h psi=0 W-E | LL LS SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.5h psi=0 W-S | LL LS SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.5h psi=0 NW-SE | LL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.5h psi=0 N-E | SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.5h psi=45 W-E | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.5h psi=45 W-S | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.5h psi=45 NW-SE | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.5h psi=45 N-E |  | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.5h psi=90 W-E | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.5h psi=90 W-S | LS SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.5h psi=90 NW-SE | LL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.5h psi=90 N-E |  | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.5h psi=135 W-E | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.5h psi=135 W-S |  | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.5h psi=135 NW-SE | LL LS | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.5h psi=135 N-E |  | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.5h psi=180 W-E | LL LS SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.5h psi=180 W-S |  | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.5h psi=180 NW-SE | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.5h psi=180 N-E | LS SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.5h psi=225 W-E | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.5h psi=225 W-S |  | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.5h psi=225 NW-SE | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.5h psi=225 N-E | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.5h psi=270 W-E | LL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.5h psi=270 W-S | SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.5h psi=270 NW-SE | SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.5h psi=270 N-E | LL LS SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.5h psi=315 W-E | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.5h psi=315 W-S | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.5h psi=315 NW-SE | LL LS | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+1.5h psi=315 N-E | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+2h psi=0 W-E | LL LS SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+2h psi=0 W-S | LL LS SL | 1 | 10h | . | . | . | . | C1 | . | C1 | . |
| near vertex rho=+2h psi=0 NW-SE | LL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+2h psi=0 N-E | SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+2h psi=45 W-E | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+2h psi=45 W-S | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+2h psi=45 NW-SE | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+2h psi=45 N-E |  | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+2h psi=90 W-E | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+2h psi=90 W-S | LS SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+2h psi=90 NW-SE | LL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+2h psi=90 N-E |  | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+2h psi=135 W-E | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+2h psi=135 W-S |  | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+2h psi=135 NW-SE | LL LS | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+2h psi=135 N-E |  | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+2h psi=180 W-E | LL LS SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+2h psi=180 W-S |  | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+2h psi=180 NW-SE | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+2h psi=180 N-E | LS SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+2h psi=225 W-E | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+2h psi=225 W-S |  | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+2h psi=225 NW-SE | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+2h psi=225 N-E | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+2h psi=270 W-E | LL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+2h psi=270 W-S | SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+2h psi=270 NW-SE | SL SL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+2h psi=270 N-E | LL LS SL | 1 | 10h | . | . | . | . | C1 | . | C1 | . |
| near vertex rho=+2h psi=315 W-E | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+2h psi=315 W-S | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+2h psi=315 NW-SE | LL LS | 1 | 10h | . | . | . | . | . | . | . | . |
| near vertex rho=+2h psi=315 N-E | LL LL | 1 | 10h | . | . | . | . | . | . | . | . |
| tangent L=0.01 alpha=-5h/L | LL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| tangent L=0.01 alpha=-2h/L | LL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| tangent L=0.01 alpha=-1.2h/L | LL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| tangent L=0.01 alpha=-0.8h/L | SL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| tangent L=0.01 alpha=-0.5h/L | SL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| tangent L=0.01 alpha=-0.1h/L | SL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| tangent L=0.01 alpha=+0h/L | SL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| tangent L=0.01 alpha=+0.1h/L | SL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| tangent L=0.01 alpha=+0.5h/L | SL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| tangent L=0.01 alpha=+0.8h/L | SL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| tangent L=0.01 alpha=+1.2h/L | SS | 1 | 10h | . | . | . | . | . | . | . | . |
| tangent L=0.01 alpha=+2h/L | SS | 1 | 10h | . | . | . | . | . | . | . | . |
| tangent L=0.01 alpha=+5h/L | SS | 1 | 10h | . | . | . | . | . | . | . | . |
| tangent L=0.01 alpha=+20h/L | SS | 1 | 10h | . | . | . | . | . | . | . | . |
| tangent L=0.01 alpha=+200h/L | SS | 1 | 10h | . | . | . | . | . | . | . | . |
| tangent L=0.001 alpha=-5h/L | LL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| tangent L=0.001 alpha=-2h/L | LL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| tangent L=0.001 alpha=-1.2h/L | LL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| tangent L=0.001 alpha=-0.8h/L | SL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| tangent L=0.001 alpha=-0.5h/L | SL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| tangent L=0.001 alpha=-0.1h/L | SL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| tangent L=0.001 alpha=+0h/L | SL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| tangent L=0.001 alpha=+0.1h/L | SL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| tangent L=0.001 alpha=+0.5h/L | SL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| tangent L=0.001 alpha=+0.8h/L | SL SS | 1 | 10h | . | . | . | . | . | . | . | . |
| tangent L=0.001 alpha=+1.2h/L | SS | 1 | 10h | . | . | . | . | . | . | . | . |
| tangent L=0.001 alpha=+2h/L | SS | 1 | 10h | . | . | . | . | . | . | . | . |
| tangent L=0.001 alpha=+5h/L | SS | 1 | 10h | . | . | . | . | . | . | . | . |
| tangent L=0.001 alpha=+20h/L | SS | 1 | 10h | . | . | . | . | . | . | . | . |
| tangent L=0.001 alpha=+200h/L | SS | 1 | 10h | . | . | . | . | . | . | . | . |
