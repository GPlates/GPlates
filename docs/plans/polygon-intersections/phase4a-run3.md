# Phase 4a, run 3: the bundle rule, and polylines

**Written by Fable up to a pause at a usage limit (2026-10-31); the three runs left at the pause were
then run by the session (Opus) and folded in: the seed-2 and jittered rows of 4.2, seed 2 in 4.2a, and
the inversion-residue totals in 3.4 and 7. Section 10 records the pause.**

Run on 2026-10-31, after run 2 (`phase4a-run2.md`) and the chosen configuration (`phase4a-variants.md`
section 10): V1b identity vertices with the loop and spike rules, rule `freex`, midpoint tests in order of
margin, option 3 for anchorless chains, no contraction. This run designs and measures the rule that run 2
left open (walk-through 7.6, the *bundle rule*) on run 2's residue and wider hunts, and puts polylines
through the core (walk-through section 12). Tools, in the planning repository's
`plans/polygon-intersection-tools/`: the changed `overlay_proto.py` and `overlay_hunt.py`, the new
`overlay_polyline.py`, and `--config` on the catalogue, cascade, anchorless and cut tools (section 1).
Every statement is marked **measured** (with the command that reproduces it) or **reasoning**.

Conventions as in run 2: `h = s = 1.4142e-6` rad; a hunt *column* is a variant, a vertex rule and an
adversary. `spk` is the chosen configuration (V1b, `freex`, margin order, spike rule) and is the control
here; `bnd` is `spk` plus the bundle rule; `bndinv` adds the fair midpoint-inversion adversary (run 2,
section 4), `bndflt` the rotational order in doubles, `bndlast` the opposite tie-break among the bundle's
consistent placements. Failure cells are oracle / check / repair / unresolved / pairing counts in cases.

## 1. Changes to the prototype

All default-off: `python -I overlay_catalogue.py` reproduces run 2's section 9 table (measured, 244 s:
V0/free 0/0/0, degenerate 55, tie-dependent 90, crossings 23; V1/free 19/19/0; V1b/free 0/0/0, 77, 5, 23;
V2/free 5/5/0; V2b/free 0/0/0, 60, 0, 9; V1b/rotation 10/23/4 with tie-dependence 7 against run 2's 9, in a
failing control whose ring walk run 1 already noted as sampling-dependent).

1. **The bundle rule** (`bundle_rule`, `bundle_tiebreak`; section 3). Each ray records where its first arc
   starts, how far that is from the vertex's output position (`delta`), how long the arc is (`first_len`),
   how far its far end is (`reach`) and a cone half-angle `theta`. At a vertex with six or more ends the
   rays with pairwise separable cones are *anchored* in the exact rotational order; the others *float* and
   are placed by the sector walk's own constraints, among the orders the cones admit. `_solve_vertex` now
   also returns the order it used, and `link` pairs along that order.
2. **Ring-continuity pairing** at a vertex where only one polygon has ends (section 5): found by the spike
   rule in seven cases run 2 never ran under it.
3. **Named configurations** `overlay_proto.CONFIGS` (`default`, `spk`, `bnd`) and `--config` on
   `overlay_catalogue.py`, `overlay_cascade.py`, `overlay_anchorless.py` and `overlay_cut.py`;
   `new_stats()` so that the subclasses (`Cut`, `PolylineOverlay`) share the counters.
4. **Hunt columns** `--bundle` (`spk`, `bnd`, `bndinv`, `bndflt`, `bndlast`) and the bundle statistics:
   vertices where a ray floats, most floating and fewest anchored rays, admissible and consistent
   placements, vertices whose placement moved from the measured order, whether `bnd`'s output differs
   from `spk`'s, and for `bndlast` the region where the two tie-breaks differ (sampled round the floating
   rays' pieces, as run 2's `inversion_region` samples round midpoint-tested pieces).
5. **A fix in the spike rule**: it treated the two dangling ends of an open arc (`overlay_cut.py`, both
   `None`) as one vertex and made the whole arc internal (measured: `python -I overlay_cut.py --index 0
   --config spk` before the fix, faces 2, oracle 37 of 298; after it, clean). A prototype artefact, not a
   design point: a dangling end is not a vertex.
6. **`overlay_polyline.py`** (section 6).

## 2. The residue, diagnosed

**Reproduced** (measured; `python -I overlay_hunt.py --gen shared --seed 1 --index N --variant V1b --mode
freex --adv spk --no-measure`, and `--adv mrg` for the spike-less control): `shared#28` (`O473 C1 U1 P8`
under `mrg`, `O600 C1 U1 P7` under `spk`; the oracle count of an inconsistent case varies between runs
with the ring walk's start), `shared#169` (`O152 C1 U1 P7`, both), `shared#234` and `shared#247`
(`C1 U1 P5` and `O240 C1 U1 P6` under `mrg`, clean under `spk`), and `six#5` at seed 7 (`O172 C1 U1 P6`).

**What the vertices look like** (measured, a diagnostic script over `run_overlay`; the walk-through's
account in 7.6 said "several rays leave along one thick plane, so their order by angle is noise"):

| case | vertex | records | extent | ends | the misplaced ray | its first arc | its start off the output position |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `shared#28` | `c1` | `LS SS SS SS SS` (P11, P12, Q4, Q5, Q6) | `1.45h` | 6 (5 rays) | the common ray to `v8` | `1.01h` | `0.41h` |
| `shared#28` | `c0` | `LS SS×8 LS` (P6-P9, Q7-Q11) | `3.31h` | 8 (6 rays) | three P rays, `1.2h` to `2.0h` | `1.24h`, `1.42h`, `2.01h` | `0.6h` to `2.9h` |
| `shared#169` | `c0` | `SS SS SS LS` | `1.40h` | 6 | Q's `0.96h` arc to `v2`, P's `0.96h` arc | `0.96h` | `1.40h` |
| `shared#247` | `v4` | `SS SS` (P5 with Q6 and Q7) | `0` by records | 6 | Q's three sub-`1.3h` arcs to `v2` | `1.12h` to `1.22h` | `0.7h` (Q6, Q7 are `0.7h` from P5) |
| `six#5` (seed 7) | `v2` | `SS LS` | `0.36h` | 6 | the common ray to `v4` | `1.01h` | `0.36h` |

At every failing vertex the adjacent-ray angles are **15° to 180°, not small**: the rays are not nearly
parallel. What they have in common is that the vertex is a *chained identity group whose records sit at
several positions*, up to `3.3h` apart, and the misplaced rays are arcs about `1h` long that leave from a
position `0.4h` to `2.9h` away from the output position the order is computed about. Seen from that
position an arc shorter than its own offset can point anywhere, so its place in the rotational order is
noise; long rays from the same group (`450h` to `8000h`) are placed to within a fraction of a degree
and are fine. In `shared#247` the group's extent *by record positions* is zero (both `SS` records sit at
P's vertex), but Q's pieces leave from Q6 and Q7, `0.7h` away: the record position is not where the ray
starts, which is why the rule below measures the start of each arc and not the group's extent.

**What the walk did wrong** (measured, `shared#28 c1`, before any change): the five rays in measured
order were P (`False`), P long (`True`), Q long (`True`), common, P (`False`). Only the Q ray and the
common ray flip Q, so the three P rays all lie in sectors with one value of `in Q`, and their classes
`False, True, False` cannot all hold: check 1 fails, the piece stays unresolved, pairing fails. The two
consistent cyclic orders both put the common ray between the P `False` rays and the Q ray, on the other
side from where its `1.01h` arc, starting `0.41h` off, was measured. So the diagnosis "the order is
noise" is right; "along one thick plane" is not the mechanism. **Reasoning**: the diagnosis corrects the
walk-through's 7.6 and 7.7 wording, which should say "rays whose first arc is short compared with the
distance from its start to the vertex's output position", and the rule is built on that.

## 3. The bundle rule

### 3.1 Definition

At an overlay vertex `v` with six or more ends (a common ray counting two), for each ray `r`:

- `s_r` is the exact position its first arc starts from (the record position or original vertex at that
  end of its piece), `c_v` the vertex's output position (walk-through 7.2), `delta_r = |s_r - c_v|`,
  `L_r` the length of the first arc (to the piece's next stop: its first original vertex, or its other
  end), and the **cone** `theta_r` is `0` if `s_r` *is* `c_v` (exact comparison of positions), `pi` if
  `L_r <= delta_r`, else `atan(delta_r / (L_r - delta_r))`. For a common ray both arcs are measured and
  the wider cone kept.
- Two rays are **separable** if the angle between their directions is at least `theta_i + theta_j`.
- Rays are taken in order of certainty (smallest cone, then largest reach); a ray is **anchored** if
  separable from every ray already anchored, else it **floats**. At a vertex whose rays all start at `c_v`
  (one record, or records at one point) every cone is `0`, every ray is anchored and nothing changes.
- The **admissible orders** are the cyclic orders that keep the anchored rays in the exact rotational
  order (about `c_v`, exact orientation signs, as before) and place each floating ray where its cone
  allows: there is an assignment of angles, one per ray, within `theta_r + theta_ref` of its measured
  angle (`ref` the most certain anchored ray, which fixes the frame and whose own cone widens the
  others') and non-decreasing round the turn. The measured order is always admissible.
- The sector walk of 4.3 runs over every admissible order. A class is **deduced** only if every
  consistent order (admissible, and agreeing with every known class) gives it. The order used for
  pairing is the consistent order with the **fewest inversions against the measured order** (ties by
  enumeration order). No consistent order is a check failure, as before.
- More than six floating rays: the measured order is kept and the vertex counted (`capped`; never
  happened).

### 3.2 Why this shape

**Reasoning.** The brief asked for combinatorial criteria first. Three were tried on paper and rejected:

- *Single arcs float, pieces with an original vertex are anchored.* Wrong both ways: `shared#28 c1`'s
  Q ray is a single arc `1260h` long whose order against the `8090h` P ray is real geometry (the two bound
  a 70° wedge of `P ∩ Q`), and `Q0[c0->c0]` in `shared#247` has an original vertex but a first arc of
  `1.7h` starting `0.8h` off. Length is what matters, and only relative to the start's offset.
- *Rays to the same neighbouring vertex form one bundle.* Right for the two P arcs `c0->c1` in
  `shared#28`, but it says nothing about the common ray to `v8`, which is the one misplaced.
- *Bundle everything at a chained group, and solve by parity alone.* Without the long rays' order the
  solve has two frames (`shared#28 c1`: `P ∩ Q` in the 70° wedge or in its 290° complement), and only the
  accident that the two short P rays are interchangeable made both give the same pairing there.

So the criterion is geometric but **threshold-free**: the cone is the bound on how far the arc's direction
seen from `c_v` can differ from its exact tangent at `s_r` (`|far - c_v|` has a component of at least
`L - delta` along the tangent and at most `delta` across it). No constant is chosen; `h` enters only
through the data (`delta` is at most the group's extent, which identity sets). Just either side of
separability nothing jumps: a pair of rays that is barely separable keeps its measured order; barely
inseparable, the less certain one floats and the parity solve either confirms the measured order (one
consistent placement, the usual case: section 4) or offers the swap, which the tie-break takes only if
the measured order is inconsistent. The swap moves a ray across a neighbour whose direction it cannot be
told from, so what changes is confined to the sector between two arcs that start within the group's
extent of each other and run within their cones of each other: a sliver no wider than about the group's
extent (`3.3h` here). Section 4 measures it.

**It leaves four-end vertices alone** by construction (`ends >= 6`), and the free rule still handles them.
**What it changes where run 2 was right**: slivers only, within `0.42h` of a boundary, in 12 of 898
`shared` cases and nowhere else (measured, 4.2a); the point set beyond that is unchanged.

### 3.3 Exact and rounded

**Reasoning**, following run 2's sections 5 and 6:

- *Exact* (predicates on the records): identity (union-find on record indices), whether `s_r` is `c_v`
  (the same original vertex: the C++ compares vertex indices, not coordinates), the record kinds (`LL`
  rule), the rotational order among anchored rays (exact orientation signs in the prototype; in doubles,
  run 2 measured no disagreement at any used vertex, and the `bndflt` column repeats that here: identical
  failure and measurement tables, section 4).
- *Rounded* (doubles, tolerances not predicates): `delta`, `L`, `theta`, the angle between two
  directions, the separability test and the admissibility test. A rounding flip moves a ray between
  anchored and floating, or adds or removes one admissible order, only when the ray's cone touches a
  neighbour's within rounding, about `1e-16` rad on cones of `1e-2` to `pi`. Flipping to floating costs
  nothing when the parity solve has one consistent placement (the common case), and otherwise offers a
  swap across a neighbour the ray cannot be distinguished from, which touches the sliver of 3.2.
  Flipping to anchored at the margin trusts an order that is right or wrong by that same swap. Neither
  can make the walk inconsistent where exact arithmetic finds it consistent, because the measured order
  is always admissible and the parity solve is exact combinatorics.
- *Combinatorial*: the enumeration is `k^m` placements times the permutations within gaps, filtered by
  admissibility; the most floating rays seen was 3 and the most admissible orders 24 (section 4), so a C++
  port can enumerate with a small cap and fall back to the measured order beyond it.

### 3.4 Interaction with the rest of the configuration

**Reasoning**, confirmed by the runs of section 4 where noted.

- *Checks and repair (section 9).* The check at a bundled vertex is "some admissible order is consistent
  with the classes"; a vertex with none fails as before and repair flips classes as before (repair does
  not move rays). Fewer vertices fail, and no repair was needed in any `bnd` run without the adversary.
  Under the fair inversion adversary the dense shared-edge residue of run 2 stays (`shared#0`, `#23`,
  `#47`: check 2, repair hitting its twenty-round cap): that mechanism is an inverted band piece inside a
  dense common chain, found by check 2 and not repairable by single flips, and the bundle rule does not
  touch it (its vertices are four-end chain vertices, so no bundle fires there: measured, `bundles=0` in
  `shared#0`).
- *Margin order (7.5).* Unchanged: the bundle affects only what a vertex deduces, and it deduces less
  (a class must hold in every consistent placement), so more arcs reach the midpoint test. Measured: the
  midpoint-test totals of `bnd` equal `spk`'s in every generator but `shared`, where they rise by 8 to 10
  per 300 cases (227 to 237 at seed 1, 243 to 251 at seed 2, 236 to 242 at seed 3), all band pieces
  (4.2a).
- *Chained identity groups whose rays leave from different positions* are exactly the vertices the rule
  is for: `theta > 0` only there. The group's extent bounds `delta`; a long group (the `28.8h` chain of
  `probe_group_growth.py`) makes more rays float but changes nothing in principle.
- *Loop and spike rules.* They remove ends before the bundle sees the vertex; the bundle then has fewer
  rays. Section 5 is the case where they remove all of one polygon's ends.

## 4. Results

### 4.1 The residue (task 1, step 4)

Measured (`python -I overlay_hunt.py --gen shared --seed 1 --index N --variant V1b --mode freex --adv bnd
--no-measure`, and `--gen six --seed 7 --index 5`):

| case | `spk` | `bnd` | what the bundle did (`diag`, measured) |
| --- | --- | --- | --- |
| `shared#28` | `O600 C1 U1 P7` | clean | `c0` (8 ends): 3 anchored, 3 floating, 4 consistent placements, moved; `c1`: 4 anchored, 1 floating, 2 placements |
| `shared#169` | `O152 C1 U1 P7` | clean | `c0`: 2 anchored, 4 floating (one of them the `451h` Q ray, `1.2e-5` rad from the `6950h` P ray), 12 placements, moved |
| `shared#234` | clean (spike rule) | clean | `v3`: 4 anchored, 2 floating, 8 placements, moved |
| `shared#247` | clean (spike rule) | clean | `c0`: 2 anchored, 2 floating, 4 placements, measured order kept |
| `six#5` (seed 7) | `O172 C1 U1 P6` | clean | `v2`: 4 anchored, 1 floating (the common ray), 1 placement, moved |

The `D` and `X` codes that remain (degenerate rings dropped, output crossings) are run 1's collapsed
regions and inherited twists, not failures.

### 4.2 The hunts

Measured (`python -I overlay_hunt.py --gen G --n N --seed S --bundle --list-failures`; cells are
oracle / check / repair / unresolved / pairing cases; "fires" is vertices where a ray floated (cases);
"largest" the most floating rays at one vertex / the most consistent placements). The `near` and `six`
seed-1 rows are after the ring-continuity pairing of section 5 (their first run is in section 5).

| generator | seed | cases | `spk` | `bnd` | `bndinv` | `bndflt` | `bndlast` | fires | largest | `bnd` output differs from `spk` (where `spk` clean) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| near | 1 | 300 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 0 | - | 0 |
| rays | 1 | 299 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 0 | - | 0 |
| tinyhole | 1 | 300 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 0 | - | 0 |
| zero | 1 | 299 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 0 | - | 0 |
| six | 1 | 300 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 7 (7) | 2 / 12 | 0 (placement never moved) |
| six | 7 | 300 | **2**/2/0/2/2 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 6 (6) | 2 / 5 | 2 (0) |
| chain | 5 | 60 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 1 (1) | 1 / 2 | 0 |
| shared | 5 | 60 | 0/0/0/0/0 | 0/0/0/0/0 | **1**/2/2/0/1 | 0/0/0/0/0 | 0/0/0/0/0 | 1 (1) | 3 / 4 | 0 |
| shared | 1 | 299 | **2**/2/0/2/2 | 0/0/0/0/0 | **2**/9/9/0/2 | 0/0/0/0/0 | 0/0/0/0/0 | 12 (10) | 4 / 12 | see 4.2a |
| shared | 2 | 300 | 0/0/0/0/0 (a) | 0/0/0/0/0 (a) | **2**/18/18/0/2 | 0/0/0/0/0 | 0/0/0/0/0 | 10 (10) | 3 / 4 | see 4.2a |
| shared | 3 | 299 | **2**/2/0/2/2 | 0/0/0/0/0 | **1**/12/12/0/1 | 0/0/0/0/0 | 0/0/0/0/0 | 10 (10) | 4 / 4 | see 4.2a |
| shared, +1 jittered copy | 4 | 197 | **2**/4/0/4/4 | 0/0/0/0/0 | **4**/7/7/0/4 | 0/0/0/0/0 | 0/0/0/0/0 | 19 (13) | 2 / 4 | 6 (2) |

(a) The three full `shared` runs were started before the ring-continuity pairing of section 5 existed.
Seed 2's one failure then, `shared#262` (`O1004 U2 P11`, no check failure, under both `spk` and `bnd`),
is that mechanism; the seed-2 row is the full rerun with the pairing (1838 s), in which it passes. The
jittered row (3 of 200 skipped as ambiguous, 1135 s) is 100 cases and one jittered copy of each: the
`spk` failures are `#41`, `#59` and their copies, the mechanism of section 2, and `bnd` clears them. The pairing can only turn such failures
into passes (it applies where linking previously always failed), so seeds 1 and 3 are unaffected.

The other `spk` failures are the mechanism of section 2 throughout, and `bnd` clears every one: `shared` seed 1
`#28` and `#169` (run 2's residue), seed 3 `#207` and `#215` (new, both dense), `six` seed 7 `#5` and
`#188` (both `touching-vertex`: P's exterior and hole touching at a vertex on a Q vertex). The `bndinv`
failures are run 2's inversion residue (its section 2: "the repair policy runs out"): in dense shared
edges the fair adversary flips band pieces inside common chains, check 2 fires at the chain's ends
(`C2`), and repair either clears it (`R5` to `R18`, oracle 0: 7 cases at seed 1, 11 at seed 3) or hits
its round cap (`R20`, oracle failures: `shared#0` and `#288` at seed 1, `#174` at seed 3, `#23` at
seed 5). Run 2 had 6 oracle / 13 check / 9 repair cases at seed 1 under the margin order; with the spike
and bundle rules it is 2 / 9 / 9. Not a bundle mechanism (3.4): the vertices involved are chain vertices
with four ends. Under `bndflt` nothing changes anywhere (every cell equals `bnd`'s).

#### 4.2a Where the bundle rule changes a clean output

Measured (a comparison script over `run_overlay`, `spk` against `bnd` on every case where a placement
moved; `shared` seeds 1, 2 and 3): the bundle moved a placement in 6, 5 and 5 cases, 2, 0 and 2 of them the
residue; in the other 4, 5 and 3 (`shared#49`, `#93`, `#215`, `#234` at seed 1; `#13`, `#68`, `#127`,
`#182`, `#204` at seed 2; `#173`, `#192`, `#249` at seed 3) `spk` was clean and `bnd`'s output differs.
Sampling 300 to 600 points round the floating rays' pieces: the differing regions are `0` to `3.0e3 h^2`
and every differing point lies **within `0.42h` of the nearer boundary** (seed 2: within `0.08h`, two of
its five with no differing sample at all); `bnd` passes the oracle in all twelve.
Mechanism: `bnd` deduces a class at a bundled vertex only if every consistent placement gives it, so at a
vertex with two or more consistent placements it deduces fewer band pieces than `spk` (which reads them
off the measured order, noise included), those pieces go to a midpoint test instead (`bnd` made 237
midpoint tests against `spk`'s 227 at seed 1, 242 against 236 at seed 3), and a test's arbitrary answer
can differ from the order's arbitrary implication. Both are band-piece classes, and the region between
them is the sliver run 2 measured for the inversion adversary (under `0.5h` wide). The alternative,
deducing from the tie-break's placement alone (`bundle_deduce='nearest'`, hunt adversary `bndn`), was
measured and **rejected**: it changes no clean output (0 of 299 at seed 1) but leaves `shared#28` and
`#169` failing exactly as `spk` does, because committing early to what a noisy order implies is what
made those vertices inconsistent in the first place. So the brief's "no change to any output run 2 got
right" holds for the point set beyond `0.42h` of a boundary (12 of 898 `shared` cases differ at all, 0 in
the other generators), and not for slivers inside the tolerance;
**reasoning**: a band piece classified by a test at its own midpoint is the better-founded of the two
arbitrary answers, and the sliver merge of walk-through 7.9, if adopted, removes the difference.

**The tie-break** (`bndlast` against `bnd`, measured in the same runs): the opposite choice among
consistent placements changed the output rings in 1 `six` seed-1 case, 1 `shared` seed-1 case and 1
`shared` seed-3 case (and in 7 `six` seed-1 cases before the ring-continuity pairing); sampling 300
points within `6h` of each floating ray's piece in those cases found **no point whose membership differed
in any operation** (`area_max=0`, `far_max=0h`). So the choice changed how the same points were grouped
into rings, or slivers below the sampling's resolution (about `0.3 h^2` per sample for a `1.5h` piece),
and nothing beyond. **Reasoning**: this is 3.2's argument measured; the sliver merge of walk-through 7.9
would remove even that.

**How often the rule fires, and the largest bundle** (measured, the `bnd` rows above): never in `near`,
`rays`, `tinyhole` and `zero` (no chained group there has a short arc leaving off-centre), 7 vertices in
300 `six` cases (seed 1; 6 at seed 7), 1 in 60 `chain`, 1 in 60 `shared` at seed 5 and 10 to 12 in 299
at seeds 1 and 3 (about 3% of cases, all but two dense). The largest: 4 floating rays at a vertex with 1
anchored (`shared`), 120 admissible orders, 12 consistent placements; no vertex reached the cap of 6
floating rays. Among vertices where the rule fired, the placement actually moved from the measured order
in 0 of 7 (`six` seed 1), 2 of 6 (`six` seed 7), 6 of 12 and 5 of 10 (`shared` seeds 1 and 3), the rest
being confirmed by the parity solve.

### 4.3 Regression (task 1, step 5)

Measured, under `--config bnd` and `--config spk` (identical results in every tool):

- Catalogue: `python -I overlay_catalogue.py --config bnd` (and `spk`), 463 cases, V1b/freex: 0 oracle, 0
  check, 0 repairs, 0 unresolved, 0 pairing; degenerate 48 (77 under run 1's rules: the spike rule drops
  fewer), tie-dependent 5, crossings 23. The bundle never fired (no chained group in the catalogue has a
  short arc leaving off-centre).
- Cascade: `python -I overlay_cascade.py --n 40 --config bnd`, 40 cascades, 157 steps, 0 failures, 0
  checks, 0 repairs; midpoint tests 27 over the run (as run 2).
- Anchorless: `python -I overlay_anchorless.py --config bnd`, 16 cases, fallback `crossings`: every
  anchorless chain resolved, 0 oracle failures.
- Cut: `python -I overlay_cut.py --n 200 --config bnd` (seeds 1 and 2): 7 and 1 oracle failures, every
  one a case whose `P` the generator made invalid (run 2's section 10 finding: 13 of 13 over three
  seeds), the same as under `--config default`; the bundle never fired.

## 5. A finding of its own: one polygon's ends all gone

**Measured** (`python -I overlay_hunt.py --gen near --seed 1 --index 291 --variant V1b --mode freex --adv
mrg|spk`, and `--gen six --seed 1 --index 34|77|149|182|207|219`): seven cases that run 2 never ran
under the spike rule (`--spike` was run at seed 9 and on the residue) fail under `spk` and pass under
`mrg`, with pairing failures and an unresolved vertex but **no check failure** (`near#291`: `O301 U1 P7`;
`six#34`: `O435 U1 P9`, all six `three-rings`). Mechanism (`near#291`): Q's hole has its apex `y` within
`h` of a P vertex (`SS`), P's ring passes `y` twice (an `LS` too), and P's vertex `x` lies on both hole
edges `1.61h` from `y` (`SL SL`). The hole's two arcs `x -> y -> x` are a spike and become internal, so
at `y` the hole has no ends left and P has four: the sector walk has no Q ray to fix `in P`, and
`in P` alternates round four P rays in two equally consistent ways, so pairing stalls (`U1 P7`). Nothing
of Q's boundary is at `y` any more, so `y` is not an intersection for linking: **each arriving end of
the remaining polygon pairs with the end leaving from the same stop, the ring passing straight through**.
No geometry; the natural pairing of a self-touch of P. Implemented in `link` (the vertex has ends of one
polygon only); all seven pass under `spk` and `bnd` (measured, 4.2). **Reasoning**: the loop rule can
leave the same situation (a Q loop at a vertex P passes twice), which run 2's 1500 cases did not produce;
the rule covers it too. It should go into walk-through 7.3 beside the loop and spike rules.

## 6. Polylines (task 2)

### 6.1 What was built

`overlay_polyline.py`: `PolylineOverlay(Overlay)` with P a polyline (walk-through section 12). The core
supplies records (`arc_ref` with `is_last`, so `SS`/`SL` at the polyline's last vertex), identity
vertices, the loop and spike rules and the common pairs. New: the polyline's pieces (no wrap; an end
point that is not a record is an original vertex of its piece), classification (a piece with an original
vertex tested there; a piece along Q's boundary, a common pair with a Q single arc, given the Q2 class
`along` in `inside | outside | left | right`, the last two the class of a point `3h` to that side of its
midpoint; a single arc deduced across a vertex that is one `LL` record from the polyline's other piece
there, never across a touch and never from an along-piece; the rest midpoint-tested in order of margin),
and the output: consecutive pieces of one class joined, an internal piece (a loop or spike inside an
identity vertex) taking the class of the piece before it so that the output covers P without a gap. The
one check is "the two pieces at an `LL` vertex differ when both were tested" (counted, not repaired).

**The oracle** samples points along P beyond `M = max(10h, 2 x widest record cluster)` from Q's boundary;
each must lie in exactly one output polyline (by its along-coordinate on P) whose class is its parity
against Q. A second pass samples every output polyline regardless of `M` and records how far from Q's
boundary a point whose class disagrees with its polyline gets (`far`), the measure the brief asked for.
(Its first version interpolated samples by chord fraction while the records' coordinates are angle
fractions, which on a `0.03` rad segment differs by `2h` and put a sample of a `0.8h` neck piece `2.8h`
outside it: a false `far` of `2.79h` in `opened-six#40`, gone with spherical interpolation. The record
of it stays here because it is the kind of mistake a C++ test along a polyline could make too.)

**Generators**: `opened` (a hunt generator's P ring, exterior 60%, cut at a vertex or at a point on an
edge, reversed 30%), `ends` (an end point on a Q edge or vertex, or `0` to `2h` either side of one; both
ends in 30%), `along` (a polyline along a Q edge, jittered and interleaved as `shared`, dense 40%, starting
on the edge 30%), `touch` (a vertex on Q's edge with both neighbours on one side; a segment passing `0` to
`2h` from a Q vertex outside its corner), `pinch` (through a vertex where Q's exterior and a hole touch,
with or without a P vertex there, starting there 30%).

### 6.2 Results

Measured (`python -I overlay_polyline.py --gen G --n 300 --seed 2 --list-failures`; columns are the
along-class / spike rule / midpoint order; cells are cases with oracle failures (wrong class beyond `M`),
coverage failures (a sample in no or two output polylines), `LL` disagreements; `far` the farthest a
misclassified point got from Q's boundary; midpoint tests as tests/cases; along-pieces as pieces/cases):

| generator | cases | `inside`/spike/margin | `left`/spike/margin | `inside`/no spike | `inside`/spike/first | `far` | midpoint | along | Q2 leak | end records |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ends | 300 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 | `0.76h` | 14/14 | 7/7 | 0 | 236 |
| along | 300 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 | `1.07h` | 130/100 | 1047/261 | 0 | 70 |
| touch | 300 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 | `0.99h` | 0/0 | 14/14 | 0 | 0 |
| pinch | 299 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 | `0.97h` | 90/88 | 7/7 | 0 | 77 |
| opened `near` | 200 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 | `0.27h` | 9/9 | 1/1 | 0 | 26 |
| opened `chain` | 199 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 | `0.87h` | 28/26 | 191/56 | 0 | 36 |
| opened `tinyhole` | 200 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 | `0.96h` | 3/2 | 66/47 | 0 | 18 |
| opened `rays` | 200 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 | `0.89h` | 18/18 | 56/51 | 0 | 30 |
| opened `six` | 200 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 | `1.09h` | 19/16 | 63/57 | 0 | 47 |
| opened `zero` | 199 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 | `0.92h` | 14/14 | 76/64 | 0 | 41 |
| opened `shared` | 200 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 | `0.99h` | 140/80 | 693/123 | 0 | 96 |
| all eleven at once (`--all --n 200 --seed 1`) | 2196 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 | `1.09h` | 381/292 | 1864/600 | 0 | 539 |

(The opened rows are `python -I overlay_polyline.py --gen opened --base B --n 200 --seed 1`, all rerun
with the spherical sampler of 6.1.)

- **No failure** in 3395 polyline cases (1199 of the four specific generators at seed 2, 2196 of all
  eleven at seed 1): no sample beyond `M` in a wrong or in two output polylines, no `LL` disagreement.
- **The misclassified reach is within `h`** (`far` at most `1.07h`, `along`): a wrong class never got
  farther from Q's boundary than the thickness. **Reasoning**: a piece with an original vertex is right
  by its test; a single arc deduced across an `LL` crossing is right if its neighbour is; so the only
  arbitrary classes are midpoint-tested band pieces and along-pieces, which lie within `h` of Q by
  construction. The measure confirms it and would catch a propagated error.
- **Midpoint tests** are where single arcs between touch records have no `LL` to deduce across:
  none in `touch`, 14 in 300 `ends`, 90 in 299 `pinch` (the polyline through the pinch meets the
  exterior and the hole within `h`, so its arcs between those records are band pieces), 130 in 300
  `along` (the arcs between interleaved touches that are not common pairs, because a Q original vertex
  lies between). The spike rule removes a few (`along`: 148 without it) by making there-and-back arcs
  internal; the margin order changes nothing here (no deduction flows from a midpoint test but across an
  `LL`, and none of those arcs reaches one).
- **The Q2 choice changes nothing beyond the along-pieces**: 0 cases in 1199 where `inside` and `left`
  differ in the class of any non-along piece (`Q2 leak`), as the design guarantees (no deduction uses an
  along-piece). Along-pieces are common in `along` (1047 pieces in 261 cases, up to the whole shared
  stretch) and rare elsewhere (7 to 14 cases per 300). The `left` test is a point `3h` to one side: in a
  thin face it is the test option 1 failed for polygons (run 2, section 7), so if a side rule is wanted
  it should be option 3's crossing count, not an offset point.

### 6.3 Recommendation on Q2

**Reasoning.** `inside` (the proposed answer) is a fixed class with no test and no failure mode, matches
the inclusive threshold of a user's point query, and in the cascade sends a polyline along a boundary
shared by two plates to the first plate, as walk-through 12 wanted. A side rule costs a test per
along-piece that is fragile exactly where along-pieces occur (thin faces, dense shared edges) and buys a
distinction nobody asked for. Measured: the choice touches only the along-pieces themselves. Recommend
`inside`, with `outside` available as the same parameter for a caller that wants the complement
(`P \ Q` of a polyline should then use `outside` so that the two outputs partition P, which the parameter
gives without a second pass).

## 7. Recommendation

**Adopt the bundle rule as defined in 3.1**, in the chosen configuration, and the ring-continuity pairing
of section 5 beside the loop and spike rules. Measured: with them the configuration has 0 oracle, check,
repair, unresolved and pairing failures in 3013 random polygon cases of seven generators at seeds 1 to
5 and 7, one run with jittered copies (where run 2's residue was 4 of 299 `shared` and `six#5`), the 463-case catalogue, 157
cascade steps, 16 anchorless cases and 400 cuts (the cut failures being invalid inputs), the same under
the float rotational order, and its tie-break never changed a sampled point's membership. What is left is
run 2's inversion residue in dense shared edges under the fair adversary: wrong output in 10 of 1155
`shared` cases (2 at seed 1, 2 at seed 2, 1 at seed 3, 1 at seed 5, 4 in the jittered run; up to 1261
of the oracle's points wrong), and check failures repaired to a correct output in 38 more. It is a
repair-policy question and not a vertex rule (open question 4). For polylines, section 12's design works as written with
the two refinements of 6.1 (internal pieces absorbed, no deduction from an along-piece); recommend Q2 =
`inside`.

**On slivers** (the developer's question): the bundle rule adds no sliver of its own that sampling could
see (4.2), and the slivers of the true answer (run 2, section 7: faces between a P edge and the Q edge it
runs along, up to `8.8e3 h^2`) are unchanged. The decision is therefore the one run 2 left: whether to
merge output regions all of whose points lie within about `h` of both boundaries. **Reasoning**: nothing
in this run argues for it; the oracle is defined to ignore them, every variant produces them, and a
merge is a distance rule on the output that would need its own threshold and its own test against the
inputs (a region thinner than `h` between two boundaries is the true answer, not an artefact). Recommend
leaving them in the output and documenting them, unless the cascade's users see them.

## 8. Open questions

1. The walk-through's 7.6 and 7.7 should be reworded from "along one thick plane" to the start-offset
   mechanism of section 2, and 7.3 should gain the ring-continuity pairing of section 5.
2. The cone uses `atan(delta / (L - delta))`, the planar bound; on the sphere at these scales the
   difference is far below rounding, but the C++ should say so.
3. The `bndlast` measurement found ring-structure differences only; the sliver merge of 7.9 (if adopted)
   would make the tie-break irrelevant to the output. Without it, the fewest-inversions tie-break is the
   recommendation, and the C++ needs a deterministic enumeration order for its ties.
4. The inversion residue (`shared#0`, `#23`, `#47`): whether repair should be allowed to flip a whole
   common chain at once (check 2 names it), rather than piece by piece into its round cap.
5. Polylines: a polyline whose end point is a record (236 of 300 `ends` cases have one) has a one-ended
   piece there; section 12's "needs no pairing" held, but the C++ output of such an end (does the output
   polyline start exactly at the end point or at the vertex's output position?) is a provenance choice.

## 9. What did not finish, and what was cut down

Every run reported in sections 4 and 6 finished, except as section 10 lists; no process was left
running. What was sized down against run 2's intentions: `chain` at 60 cases (seed 5, as run 2 finally ran it; its 300-case run never finished in
run 2 and was not attempted); `shared` at seed 5 at 60 cases (run 2's comparison size) beside the three
full seeds; the jittered copies as one copy of 100 `shared` cases at seed 4 rather than two copies of
every generator; no `--controls` columns (the controls, V0/V1/V2 and `rotation`, were settled by runs 1
and 2 and do not use the bundle rule). The `shared` 300-case runs took 1300 to 1700 s each with five
columns; `six` 700 s; the polyline suites 25 to 230 s. Timings are on the planning worktree's machine,
single-threaded, several runs in parallel.

## 10. Status at the pause, and where to pick up

**Done and measured**: the diagnosis (section 2); the bundle rule as defined in 3.1, built in
`overlay_proto.py` (`bundle_rule`, `bundle_tiebreak`, `bundle_deduce='all'`; the `nearest` deduction
variant exists only for the comparison of 4.2a and is rejected); the ring-continuity pairing (section
5); the hunt columns and statistics; `--config spk|bnd` on the regression tools; every row of 4.2 but
the jittered one; 4.2a on seeds 1 and 3; 4.3 in full; `overlay_polyline.py` and every row of 6.2; the
README in the tools directory. All tool changes are uncommitted in the planning worktree
(`plans/polygon-intersection-tools/`), for review. Scratch scripts (the per-vertex diagnostic, the
`bnd`-against-`spk` comparison, the admissibility check, the polyline far-point finder) live in the
session's scratchpad and are not deliverables; the comparison script's results are quoted in 4.2a.

**Finished after the pause** (by the session, 2026-10-31; outputs in its scratchpad as
`run3-shared-s2.txt`, `run3-shared-jitter-s4.txt` and `run3-compare-shared-s2.txt`): the seed-2 rerun and
the jittered run (4.2's rows), and the seed-2 comparison (4.2a). Left undone: Fable's own reread of
sections 3 and 7 against the code; the session's review takes its place. Nothing in the design is open on
the run's side: the decisions left for the developer are in section 7 (adopt the rule; Q2 = `inside`;
leave slivers), with 4.2a's trade-off (sliver changes in 12 of 898 `shared` cases against the residue
fixed) stated for them to weigh, and open question 4 (the inversion residue).
