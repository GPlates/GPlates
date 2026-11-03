# Phase 4a, run 4: the `chain` failures, and the one-passage rule

Run on 2026-11-04 (Fable), after Phase 5 step 4 found that the chosen configuration (`bnd`: V1b,
rule `freex`, margin-ordered midpoint tests, the spike rule, the bundle rule; option 3 for anchorless
chains) fails about 1% of the `chain` generator's cases in the prototype itself (PLAN.md, Status
2026-11-04). This run finds the mechanism of every listed failure, designs and builds a fix, and
measures it. Tools, in the planning repository's `plans/polygon-intersection-tools/`: the changed
`overlay_proto.py` and `overlay_hunt.py` (section 9). Every statement is marked **measured** (with
the command that reproduces it) or **reasoning**.

Conventions as in runs 2 and 3: `h = s = 1.4142e-6` rad; failure cells are oracle / check / repair /
unresolved / pairing counts in cases; the hunt's codes are `O` oracle failures (points), `C` check
failures, `R` repairs, `U` unresolved pieces, `P` pairing failures, `D` degenerate rings dropped, `X`
output crossings, `T` tie-dependent output (`D`, `X` and `T` are not failures: run 1's collapsed
regions and inherited twists). The columns of this run are `bnd` (the control), `r4` (`bnd` plus the
one-passage rule of section 3), `r4n` (`r4` without the spike rule), `r4t` (`r4` with the spike rule
restricted, section 3.3), `r4inv` / `r4tinv` (the fair midpoint-inversion adversary of run 2 on `r4`
/ `r4t`) and `r4flt` / `r4tflt` (the rotational order in doubles).

## 1. What was run

All commands in the tools directory, `python -I`:

- Diagnosis: a per-vertex diagnostic (`diag.py`, session scratchpad) over `overlay_proto.Overlay`,
  logging the order in which classes are assigned, each vertex's rays with their angles, cones and
  classes, and for a failing vertex every cyclic order of its rays that would be consistent with the
  classes; a mechanism extractor (`mech.py`); an input-validity check (`selfx.py`: exact self-crossings
  of each polygon and vertices within `h` of a non-adjacent segment of their own polygon).
- The 12 listed cases and `tinyhole#123` under every column:
  `overlay_hunt.py --gen chain --seed S --index N --r4-attr` (section 7, `r4-13cases.txt`).
- `chain` at seeds 1, 2, 3 and 5, 300 cases each: `--r4` (`bnd`, `r4`, `r4n`, `r4inv`, `r4flt`) and
  `--columns bnd,r4t,r4tinv,r4tflt`; the fresh seeds 4, 6, 7, 8: `--r4-short` (`bnd`, `r4`, `r4n`) and
  `--columns bnd,r4t` (seeds 4, 6); `--variant V1b --mode freex --adv r4t` for seeds 1, 2, 3, 5.
- Every other generator at run 3's sizes: `near`, `rays`, `tinyhole`, `zero`, `six` (seed 1, 300),
  `six` (seed 7, 300), `shared` (seeds 1, 2, 3 at 300; seed 5 at 60; seed 4 at 100 with one jittered
  copy), each `--r4-short` and `--columns bnd,r4t`.
- Regression tools: `overlay_catalogue.py`, `overlay_cascade.py --n 40`, `overlay_anchorless.py`,
  `overlay_cut.py --n 200` (seeds 1, 2), each `--config bnd|r4|r4n|r4t`.
- Run 2's and run 3's residue and ring-continuity cases (`shared#28`, `#169`, `#234`, `#247` at seed 1,
  `#207`, `#215` at seed 3, `six#5`, `#188` at seed 7, `near#291`, `six#34`, `#77`, `#149`, `#182`,
  `#207`, `#219` at seed 1) under `bnd`, `r4`, `r4n` and `r4t`.
- A probe with a valid (simple) subject polygon for mechanism B (`probe_simple_gap.py`, scratchpad;
  section 2.5).
- Timing: `--gen chain --n 60 --seed 5` and `--gen six --n 60 --seed 1`, one column each, run
  concurrently.

## 2. The mechanisms

### 2.1 Where the twelve fail

**Measured** (`diag.py`; the first tie order; the same vertices fail in every order). Every failing
vertex has six or more ends, lies on a dense `Q` edge or at a `Q` vertex that `P` passes twice within
`h`, and in every one **one polygon has exactly two ends** (it passes the vertex once) while the other
has four or more:

| case | kind | failing vertex | ends (P, Q) | mechanism | fixed by |
| --- | --- | --- | --- | --- | --- |
| seed 1 `#41` | `ss-chain m=3 edge` | `v3` (`LS LS`) | 6 (4, 2) | A | along rule, one-passage rule |
| seed 1 `#107` | `zigzag-ll m=6 vertex` | `v1` (`LS` x4) | 10 (8, 2) | B | gap cone, one-passage rule |
| seed 1 `#182` | `zigzag-ll m=3 vertex` | `v1` (`LS LS`), then `v4` (`LL`) | 6 (4, 2) | B | gap cone, one-passage rule |
| seed 1 `#288` | `zigzag-ll m=3 vertex` | `v1` (`LS LS`), then `v5` (`LL`) | 6 (4, 2) | B | gap cone, one-passage rule |
| seed 2 `#28` | `along swapped m=5 vertex` | `c0` (`SS` x3, `SL`) via `v4`, `v8` | 4 (0, 4) after the spike rule | C | spike rule restricted (or removed), with the one-passage rule |
| seed 2 `#52` | `zigzag-ll m=5 vertex` | `v1` (`LS` x4), then `v2` (`LL`) | 10 (8, 2) | B | gap cone, one-passage rule |
| seed 2 `#119` | `ss-chain m=3 vertex` | `v2` (`LS LS`) | 6 (4, 2) | A | along rule, one-passage rule |
| seed 3 `#220` | `zigzag-ll m=5 vertex` | `v1` (`LS` x4), then `v2`, `v7` (`LL`) | 10 (8, 2) | B | gap cone, one-passage rule |
| seed 3 `#249` | `zigzag-ll m=9 vertex` | `v0` (`LS` x4), then `v2`, `v4` (`LL`) | 10 (8, 2) | B | gap cone, one-passage rule |
| seed 3 `#281` | `zigzag-ll m=6 vertex` | `v0` (`LS` x5) | 12 (10, 2) | B | gap cone, one-passage rule |
| seed 5 `#108` | `zigzag-ll m=7 vertex` | `v0` (`LS LS`) | 6 (4, 2) | B | gap cone, one-passage rule |
| seed 5 `#163` | `zigzag-ll m=8 vertex` | `v2` (`LS` x3) | 8 (6, 2) | B | gap cone, one-passage rule |

The seven failures of the fresh seeds (section 5.2) are the same two mechanisms: A in `#22` (seed 4)
and `#221` (seed 7), B in `#168` (seed 4), `#249` (seed 6), `#114` (seed 7), `#125` and `#250`
(seed 8).

**Not the cause: self-crossing input.** All 12 cases have a self-crossing polygon (the one the
generator builds: the zigzag's closing edge `C->D` runs through the chain). But so do 121 to 160 of
300 `chain` cases at every seed, 178 of 300 `near`, 261 of 300 `rays`, 146 of 300 `shared` (measured,
`selfx.py`), and they pass: the overlay reads polygons by parity and the design carries self-crossing
input through (design section 1). What the mechanisms need is a polygon passing **twice within `h`
of one point of the other polygon's boundary**: a self-touch within tolerance, or a neck narrower
than about `2h`. Section 2.5 shows mechanism B with a simple polygon.

**Ruled out** in every case (measured, `diag.py`'s "consistent cyclic orders"): a missing or spurious
record (the records are what the specification gives, and the C++ agrees record for record), a
rounding flip (every direction and order is exact in the prototype; `r4flt` changes nothing), the
tie order (the same vertex fails in every order). In the B cases with a `then v2 (LL)` the `LL`
vertex fails only because a class deduced at the first vertex contradicts the `LL` rule there.

### 2.2 Mechanism A: a crossing inside a single arc, counted at both ends

**Measured** (`chain#41` seed 1; `#119` seed 2 is the mirror image; `mech.py`). `Q`'s dense edge has
vertices `Q5`, `Q6` two steps (`1.99h`) apart; `P`'s chain `p0, p1, p2` runs along it within `0.5h`
(`SL` records at the `p`s, `LS` at the `Q`s: the common chain `v1..v6`); `P`'s closing edge `C->D`
(segment 5) crosses the same `Q` edge at `71.6` degrees between `Q5` and `Q6`, passing `0.94h` from
each. Both `Q` vertices are in segment 5's thick plane, so the primitive reports `LS(5,5)` and
`LS(5,6)` and no `LL` for the pair (segment 5, `Q` segment 5): the crossing is absorbed into the two
touches, as the specification says it should be. The overlay then has a single arc `P0[v5->v3]`
(`1.99h`, segment 5 between the two touches) whose stored arc runs from `Q6` to `Q5`, along `Q`'s
edge, while its ray directions are segment 5's exact tangents at `Q5` and `Q6`, `71.6` degrees off
that edge. At `v3` (`= Q5`) the rotational order is `C(v2) 0, P(v5) 89.6, C(v4) 180, P(v7) 269.6`
degrees: alternating, a crossing; at `v5` (`= Q6`) likewise. One crossing is thus claimed at both
vertices. With `P0[v6->v5] = False` (outside `Q`, from the `LL` at `v5`'s far side) and
`P0[v3->v7] = True` (from the `LL` at `v7`), `P` crosses `Q` an odd number of times between `v6` and
`v7`; the sector walk at `v5` deduces `P0[v5->v3] = True` from its order, and `v3`'s order then has
both `P` rays inside `Q` across an alternating order: check 1 fails, and no single flip repairs it
(flipping `P0[v5->v3]` fails `v5` instead). The four orders consistent with the classes all put the
two `P` rays side by side (a bounce at `v3`).

**Reasoning.** The class of `P0[v5->v3]` is a band class either way (the arc crosses `Q` in its
interior; its midpoint is on `Q`'s edge); the inconsistency is between two rotational orders, each
exact, each right for an infinitely thin line, and jointly wrong because the overlay has split one
crossing into two vertices `2h` apart. The band piece cannot be given a class that satisfies both
orders. Classification: the orders are exact; the classes `False` and `True` at the far ends are
exact (`LL` rule from vertex tests); the deduction at `v5` is combinatorial on an order that is not
informative there. The configuration needs `Q`'s path between the two touches to be split by another
vertex of `P` (here `v4 = p1`), that is `P` passing twice within `h` of the same `Q` edge.

### 2.3 Mechanism B: a long ray inside a common ray's direction gap

**Measured** (`chain#182` seed 1; the other eight B cases differ only in numbers, `mech.py`).
`Q`'s hole vertex `Q11` lies on two segments of `P`'s zigzag, within `0.13h` of the closing edge
(segment 5) and exactly on segment 2 (`LS(2,11)`, `LS(5,11)`), so `v1` is `Q11` with `P` passing
twice. Segment 2 runs along the hole edge `Q10->Q11` for `2.0h` (`SL(2,10)` at the zigzag vertex
`p1`): the common pair `P0[v0->v1] / Q1[v0->v1]`, whose `P` arc leaves `v1` at `130.3` degrees and
whose `Q` arc (the hole's edge) at `150.4`: a **direction gap** of `20.1` degrees, the usual
`atan(h / L)` of a `2h` common arc. Segment 5's ray toward `v4` leaves at `131.3` degrees: inside
the gap. The common ray's direction is `P`'s (design 3.6), so the order at `v1` is
`C 0, P(v4) 0.98, P(v2) 180, P(v3) 181, Q(v6) 252.5`: both `Q` boundary rays (`C` and `Q(v6)`) are
adjacent, every `P` ray is on one side of `Q`, and from `P0[v1->v2] = True` (vertex-tested, exact)
the walk deduces `P0[v1->v4] = True` and `P0[v3->v1] = True` at its first visit. But `Q`'s edge is
at `150.4` degrees, and segment 5's ray at `131.3` is inside the hole: `P0[v1->v4]` is `False` (its
exact midpoint parity, and the `LL` rule at `v4`). The `LL` vertex `v4` then fails, and `v1` fails
once `Q1[v1->v6]` arrives from `v6`. Repair hits its cap (`O248 C2 R20`). The gaps in the nine B
cases are `11` to `32` degrees; the misplaced ray is `1` to `11` degrees from the common ray's `P`
direction and is long (`2750h` to `8940h`) in seven, a `1.7h` loop piece in `#107`, and in `#281`,
`#163`, `#114` it is itself vertex-tested (exact) and the wrong deduction flows to the other rays.

**Reasoning.** A common ray stands for two arcs whose tangents differ by up to `atan(2h / L)`; a ray
inside that gap is on `P`'s side of one and `Q`'s side of the other, and which matters depends on
whose membership is asked. Walk-through 7.6's "every ray leaves from that point" holds for the
positions; it does not make a common ray's direction unique. The decision is rounded only in the
sense that `gap` is a float; the order itself was exact and exactly misleading. The run-3 cone is
`0` here (`delta = 0`: every arc starts at the output position), which is why the bundle rule did not
fire (`bundles=0` in every B case).

### 2.4 Mechanism C: a corner removed by the spike rule

**Measured** (`chain#28` seed 2, the only C case; `geom.py`, `mech.py`). `P`'s hole has a corner at
`P5` (`112` degrees). `Q` is the swapped chain polygon: three of its vertices `Q2, Q3, Q4` are within
`h` of `P5` (`SS` x3), its vertex `Q5` sits in the corner's wedge `1.04h` from `P5`, within `h` of
both edges (the audit's D3 wedge: `LS(4,5)`, `LS(5,5)`), and its closing edge `Q` segment 7 passes
`0.17h` from `P5` (`SL(5,7)`), `3.1` degrees from `P`'s edge segment 4. `Q` is self-crossing there
(segment 7 crosses `Q` segment 4 between `P5` and `Q5`). The overlay has `c0 = {P5, Q2, Q3, Q4}` and
`v4 = Q5`; `P`'s two edges from `v4` to `c0` and back are a there-and-back by identity, so the **spike
rule makes them internal**: `P`'s corner is cut off and `P` re-routed through `Q5`. Then `c0` has four
`Q` ends and no `P` end, the sector walk there says `in P` is constant round it, and
`Q0[v1->c0] = True` (vertex-tested at `Q1`, `1.36h` above the corner) is carried to `Q0[c0->v8]`,
the `2090h` run of segment 7, which is inside the hole (`False`: its exact midpoint, and the `LL`
rule at `v8`). Check failures at `v4` and `v8`, repair at the cap (`O547 C2 R20`).

**Reasoning.** Segment 7 crosses `P`'s edge segment 5 `0.18h` from the corner: in the true geometry
`Q` crosses `P` *at the spike's tip*. Re-routing `P` by `1.04h` moves that crossing, for a segment
`3` degrees from `P`'s edge, by `1.04h / sin 3` degrees `= 14h` along it (measured on the
coordinates), beyond the band: no single class fits the piece. The spike rule's premise (walk-through
7.6, design 3.4: the hairpin bounds nothing and nothing of the other polygon crosses it) fails when
the other polygon crosses the hairpin at its tip. What distinguishes this spike from the ones the rule
was made for (run 2's `A, B, A` along `P`'s edge, where the tip is passed once by `P`): the other
polygon passes the tip **twice** (`Q`'s chain and `Q` segment 7), that is the tip is also a self-touch
of `Q`. A combinatorial restriction follows (3.3). The sector walk's "in `P` constant round a vertex
with no `P` ends" is otherwise right and needed: a one-polygon vertex is where the ring-continuity
pairing of run 3 section 5 applies, and silencing it breaks `tinyhole#123` (section 2.6).

### 2.5 Mechanism B with a valid input

**Measured** (`probe_simple_gap.py`, 40 trials, `probe-simple-gap.txt`): `Q` a square with a hole
whose vertex `y` has edges at `150` and `23` degrees; `P` a thin U whose two sides are a `4h` segment
through `y` at `130` degrees (common with the hole's edge for `2h`) and a long segment at `130.3` to
`138` degrees passing `y` at `0.02h` to `0.9h`, the two sides not crossing (their lines meet beyond
the short side's ends); the rest of `P` far away. `P` is simple in 35 of the 40 trials (the other
five, with the offset below about `0.1h`, cross within the short side). **`bnd` fails all 40**
(`O58` to `O87`, `C2`, `R20`); `r4`, `r4n` and `r4t` pass all 40. So the mechanism needs `P` within `h`
of itself at `Q`'s vertex (a neck narrower than `h`), not a self-crossing. **Reasoning**: mechanism
A needs `Q`'s path between two touches of one `P` segment split by a vertex of another `P` ring or
passage, which again is `P` within `h` of itself; a valid polygon with holes can do it (a hole within
`h` of the exterior at a `Q` edge).

### 2.6 What the first build found: three local rules, one of them wrong

**Measured** (`--r4-attr`: columns `r4x`, `r4a`, `r4g`, `r4s`; `r4-13cases.txt`). The first build was
the smallest local rule per mechanism, all three on top of `bnd`:

- `r4a`, the **along rule**: a non-common single arc whose two end records lie on one segment of the
  other polygon (by kind and indices: an interior record lies on its segment, a vertex record on the
  two segments at that vertex) runs along that segment; its ray floats with a cone of `pi`. Fixes
  the two A cases and nothing else.
- `r4g`, the **gap cone**: a common ray's cone is at least the angle between its `P` and `Q`
  tangents. Fixes the nine B cases and nothing else.
- `r4s`, **silent one-polygon vertices**: a vertex where only one polygon has ends deduces nothing
  and is not checked. Fixes `#28` and **breaks `tinyhole#123`** (`bnd` clean; `r4s` `O194 P11`): `Q`'s
  tiny hole (`r = 0.56h`) inside `P`'s band is removed whole by the loop and spike rules, leaving two
  one-polygon vertices on `P`'s ring; without the walk's "classes equal" there, the `1.68h` band piece
  between them takes an arbitrary midpoint class that its neighbours (both exact) contradict, and the
  ring-continuity pairing finds one kept end. Rejected.

`r4x` (all three) therefore fails `#123` too. Every failing vertex having one polygon pass once
pointed to the rule below, which covers A and B at once and is the four-end rule's own principle.

## 3. The one-passage rule

### 3.1 Definition

At a vertex `v` with six or more ends where one polygon (`X`) has **exactly two ends** (counting a
common ray as an end of both) and the other (`Y`) has four or more:

- `X`'s boundary passes `v` once and divides its neighbourhood into an inside-`X` half and an
  outside half. Each non-common `Y` ray is in one half, by its class alone (its class is "inside
  `X`"). **Any assignment of classes to the `Y` rays is realisable by some rotational order** (put
  the inside rays in the inside half in any order, the outside rays in the other), so the measured
  order adds no constraint that the classes do not already carry, and is not consulted.
- For each `X` end `e`, let `m_in(e)` be `Y`'s membership on the inside-`X` side of `e`: the end's
  class if it is a plain ray (the same on both sides of a plain ray); `1 - c` if it is a common ray
  with class `c` (`same = 0`, `opposite = 1`; on the inside side `in X = 1`, so `in Y = c xor 1`).
  Walking the inside half from one end to the other crosses every `Y` ray inside `X`, each flipping
  `Y`'s membership, so the one relation is
  **`m_in(b) = m_in(a) xor (number of Y rays inside X, mod 2)`**.
  (Walking the outside half gives the same relation, by the parity of the total number of `Y` ends.)
- **Deduction**: if exactly one of the terms is unknown (one `X` end's class, or one `Y` ray's
  class), it follows. **Check**: all known and the relation false is a check failure, as before.
- **Linking** needs memberships per sector, so an order is built from the classes: `a`, then the `Y`
  rays inside `X` in their measured relative order, then `b`, then the rays outside, and the sector
  walk runs on it (if that walk has no solution, the mirror order with the halves exchanged is the
  other realisation; the parity relation guarantees one of the two). The relative order within a
  half is `Y`'s own self-arrangement, which touches only which kept rays pair with which (ring
  structure), never the point set.
- Vertices with six or more ends where **both** polygons pass more than once keep the bundle rule
  of run 3 unchanged. Four-end vertices keep the free and `LL` rules; the one-passage rule at a
  four-end vertex would reduce to the free rule (`Q`'s classes differ iff exactly one of `P`'s two
  rays is inside `Q`).

Built in `overlay_proto.py`: `_is_once` (`:1032`), `_solve_once` (`:1045`), called from
`_solve_vertex` before the bundle logic (`:955`); option `once_rule`; `CONFIGS['r4']`.

### 3.2 Why this shape

**Reasoning.** Walk-through 7.4 argued that at a four-end vertex "the records already encode the
snapped topology and the rays describe the unsnapped one, and with a piece inside the band at the
vertex the two can disagree". Every failure here is that disagreement at a six-or-more-end vertex:
the crossing counted at both ends of a band arc (A), a long ray between the two tangents of a `2h`
common arc (B), a corner's crossing moved `14h` by a `1h` re-routing (C). The rotational order is
only ever needed to tell *which* of one polygon's passes another polygon's ray threads between
(7.6). When one polygon passes once there is nothing to thread between, and the classes say
everything the order could. The rule is combinatorial (counting ends, xor), needs no constant, and
deduces strictly less than the sector walk did at those vertices, so it can never introduce a false
inconsistency: whatever it deduces, the walk on some order would have deduced too.

What it costs: the walk's extra deductions at such vertices, which were exactly the unreliable ones.
Pieces that the walk used to deduce there are now classified elsewhere (the `LL` rule, a vertex
test) or midpoint-tested; the tests are band pieces (measured, section 5.4), so the margin principle
is untouched. The bundle rule now applies only where both polygons pass more than once, which the
hunts reach in 1 vertex of 2393 `chain` and 1155 `shared` cases (`both_twice`, section 5).

### 3.3 The spike rule, restricted

**Reasoning**, from 2.4. A spike (two consecutive single arcs of one ring from `A` to `B` and straight
back) is made internal only if **the other polygon has at most two ends at the spike's tip `B`**
(passes it at most once, counting a common ray as an end). With two passages of the other polygon
at the tip, the tip is a self-touch of that polygon and the hairpin can lie between its passages
(`#28`: `Q`'s chain and `Q`'s segment 7); removing it then removes a crossing. Run 2's spikes (`Q`'s
`A, B, A` along `P`'s edge, `P` passing `B` once) and `tinyhole#123` (`P` passing the hole's vertex
once) are untouched. Combinatorial; built in `_contract` (`:669`, option `spike_tip_once`);
`CONFIGS['r4t']`. The alternative, no spike rule at all (`r4n`), also passes every case (section 5)
but changes more: 77 degenerate rings dropped in the catalogue against 48 (run 1's count before the
spike rule), and 10 to 40 more midpoint tests per 300 `shared` cases.

### 3.4 Exact, rounded, combinatorial

**Reasoning**, in the terms of run 2's sections 5 and 6 and design 3.6:

- *Exact*: nothing new is computed from coordinates. The end counts, the classes and the xor are
  combinatorial; the linking order is built from the classes and the measured cyclic order (exact
  orientation signs, as before).
- *Rounded*: nothing. The rule removes the one place where a rounded quantity (the run-3 cone) was
  the guard against an order's misuse at these vertices.
- *Combinatorial*: `O(k)` per vertex; no enumeration (the bundle enumeration remains only for
  both-pass-twice vertices).

The first build's gap cone is rounded (a float angle used as a tolerance) and the along rule
combinatorial; both are kept in the prototype for comparison (`r4g`, `r4a`) and are not recommended:
each covers one mechanism, and the gap cone relies on the bundle enumeration at every dense chain end.

## 4. Results on the listed cases

**Measured** (`--r4-attr`, `r4-13cases.txt`; `bnd | r4 | r4n | r4x | r4a | r4g | r4s`, with the oracle):

| case | `bnd` | `r4` | `r4n` | `r4t` | `r4x` | `r4a` | `r4g` | `r4s` |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `chain#41` s1 | `C1` | clean | clean | clean | clean | clean | `C1` | `C1` |
| `chain#107` s1 | `O82 C1 U3 P5` | clean | clean | clean | clean | `O82 C1 U3 P7` | clean | `O82 C1 U3 P6` |
| `chain#182` s1 | `O269 C2 R20 P19` | clean | clean | clean | clean | `O248 C2 R20` | clean | `O269 C2 R20` |
| `chain#288` s1 | `O547 C2 R20 P18` | clean | clean | clean | clean | `O524 C2 R20` | clean | `O450 C2 R20` |
| `chain#28` s2 | `O547 C2 R20 P27` | **`O547 C2 R20 P28`** | clean | clean | clean | `O547 C2 R20` | `O547 C2 R20` | clean |
| `chain#52` s2 | `O708 C2 R20 P16` | clean | clean | clean | clean | `O708 C2 R20` | clean | `O708 C2 R20` |
| `chain#119` s2 | `O29 C1 U1 P6` | clean | clean | clean | clean | clean | `O29 C1 U1` | `O29 C1 U1` |
| `chain#220` s3 | `O883 C2 R20 P26` | clean | clean | clean | clean | `O984 C2 R20` | clean | `O984 C2 R20` |
| `chain#249` s3 | `O696 C2 R20 P21` | clean | clean | clean | clean | `O662 C2 R20` | clean | `O696 C2 R20` |
| `chain#281` s3 | `O261 C1 U3 P9` | clean | clean | clean | clean | `O261 C1 U3` | clean | `O261 C1 U3` |
| `chain#108` s5 | `C1 U2 P8` | clean | clean | clean | clean | `O115 C1 U2` | clean | `O375 C1 U2` |
| `chain#163` s5 | `O589 C1 U3 P11` | clean | clean | clean | clean | `O589 C1 U3` | clean | `O663 C1 U3` |
| `tinyhole#123` s1 | clean | clean | clean (`D1`) | clean | **`O194 P11`** | clean | clean | **`O194 P10`** |

("clean" may carry `D` and `X` codes, as `bnd` does on these cases.) The residue of runs 2 and 3
(`shared#28`, `#169`, `#234`, `#247`, `#207`, `#215`, `six#5`, `#188`) and the ring-continuity cases
(`near#291`, `six#34`, `#77`, `#149`, `#182`, `#207`, `#219`) pass under `bnd`, `r4`, `r4n` and `r4t`
(measured, `r4-residue.txt`; `r4n` and `r4t` drop one degenerate ring more in four of them).

## 5. The hunts

### 5.1 `chain`, the four seeds

**Measured** (`--gen chain --n 300 --seed S --r4 --list-failures`, 1815 to 2510 s each with five
columns; `r4t` from `--columns bnd,r4t,r4tinv,r4tflt`, section 5.5 note). Cells: oracle / check /
repair / unresolved / pairing cases; `midpt` midpoint tests / cases; `once` vertices solved by the
one-passage rule (cases); `changed` cases whose output differs from `bnd`'s where `bnd` was clean,
with the largest differing region (sampled round every piece under `50h`, in `h^2`) and the farthest
differing point's distance from the nearer boundary.

| seed | cases | `bnd` | `r4` | `r4n` | `r4inv` | `r4flt` | `r4` midpt (`bnd`) | `r4` once | `r4` changed (clean), area, far |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | 299 | **3**/4/2/1/3 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 57/57 (54/54) | 77 (75) | 4 (1), `6.1 h^2`, `0.23h` |
| 2 | 300 | **3**/3/2/1/3 | **1**/1/1/0/1 (`#28`) | 0/0/0/0/0 | **1**/1/1/0/1 (`#28`) | **1**/1/1/0/1 (`#28`) | 62/62 (61/61) | 80 (78) | 3 (0) |
| 3 | 299 | **3**/3/2/1/3 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 40/40 (39/39) | 78 (78) | 4 (1), `8.5 h^2`, `0.38h` |
| 5 | 300 | **2**/2/0/2/2 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 60/60 (58/58) | 67 (66) | 2 (0) |

`r4n`'s midpoint tests are 75, 82, 58 and 78 (the spike rule's absence); `r4inv` inverted 57, 62, 40
and 60 midpoints per seed and changed the output in every such case, within `1.4 h^2` and `0.11h` to
`0.32h` of a boundary (run 2's sliver measure); `r4flt` equals `r4` cell for cell. The bundle rule
fired in 13 to 15 `bnd` cases per seed and in 0 under `r4` (`both_twice = 0`).

`r4t` on the same seeds, single column (`--adv r4t`, 459 to 611 s): **0/0/0/0/0 at every seed**, 61,
66, 44 and 62 midpoint tests, 81, 84, 82 and 69 one-passage vertices, `both_twice` 0. The comparison
columns (`bnd,r4t,r4tinv,r4tflt`) are in section 5.5.

### 5.2 `chain`, four seeds nobody had looked at

**Measured** (`--seed 4|6|7|8 --r4-short`, 1251 to 1415 s):

| seed | cases | `bnd` | failing (`bnd`) | mechanism | `r4` | `r4n` | `r4t` |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 4 | 298 | **2**/2/2/0/2 | `#22` (`ss-chain swapped m=7`), `#168` (`zigzag-ll m=5`) | A, B | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 |
| 6 | 298 | **1**/1/1/0/1 | `#249` (`zigzag-ll m=7`) | B | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 |
| 7 | 300 | **2**/2/0/2/2 | `#114` (`zigzag-ll m=8`), `#221` (`ss-chain swapped m=7`) | B, A | 0/0/0/0/0 | 0/0/0/0/0 | (not run; section 8, question 4) |
| 8 | 299 | **2**/2/2/0/2 | `#125`, `#250` (`zigzag-ll`) | B, B | 0/0/0/0/0 | 0/0/0/0/0 | (not run; section 8, question 4) |

So `bnd` fails 19 of 2393 `chain` cases over eight seeds (0.8%), the fix covers the seven cases it
was not designed on, and the mechanisms are the two already found. Output changes against `bnd` in
clean cases: 4, 0, 1, 2 (within `0.31h`).

### 5.3 The other generators

**Measured** (`--r4-short --list-failures`, and `--columns bnd,r4t`):

| generator | seed | cases | `bnd` | `r4` | `r4n` | `r4t` | once (`r4`) | midpt `bnd` / `r4` / `r4n` | `r4` changed (clean), area, far |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| near | 1 | 300 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 13 | 1 / 1 / 3 | 0 |
| rays | 1 | 299 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 3 | 111 / 113 / 119 | 2 (2), no differing sample |
| tinyhole | 1 | 300 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 0 | 20 / 20 / 24 | 0 |
| zero | 1 | 299 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 0 | 24 / 24 / 32 | 0 |
| six | 1 | 300 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 200 (177) | 2 / 20 / 28 | 17 (17), `11.7 h^2`, `0.39h` |
| six | 7 | 300 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 203 (187) | 3 / 24 / 32 | 17 (17), `11.9 h^2`, `0.41h` |
| shared | 1 | 299 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 21 (14) | 237 / 245 / 280 | 3 (3), `7.9 h^2`, `0.27h` |
| shared | 2 | 300 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 16 (15) | 251 / 254 / 292 | 2 (2), no differing sample |
| shared | 3 | 299 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 18 (14) | 242 / 249 / 281 | 3 (3), `11.8 h^2`, `0.22h` |
| shared | 5 | 60 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | (not run) | 1 | 48 / 48 / 57 | 0 |
| shared, +1 jittered | 4 | 197 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 27 (15) | 137 / 157 / 161 | 8 (8), `17.5 h^2`, `0.52h` |

`r4t`'s counts equal `r4`'s in every cell above except where the restricted spike rule leaves a spike
in place (`six` seed 1: 212 one-passage vertices, 26 midpoint tests, 22 changed outputs; `near`: 15
and 3). The `six` generator is where the rule fires most (necks, touching rings: one polygon passes
once at every such vertex) and where it costs most: 18 to 21 midpoint tests per 300 cases that the
sector walk used to deduce, all band pieces, and the changed outputs are the slivers those tests
decide (`far` at most `0.41h`, as run 3's 4.2a measured for the bundle rule, `0.42h`). Tie-dependent
output rises from 0 to 3 or 4 `six` cases (ring structure in sub-`h` spurs, as run 2 saw for V1b).

### 5.4 Regression tools

**Measured** (`--config r4|r4n|r4t`; outputs in the scratchpad):

- Catalogue: `r4` and `r4t` are **identical to `bnd` line for line** (463 cases: 0 oracle, 0 check, 0
  repairs, 0 unresolved, 0 pairing; degenerate 48, tie-dependent 5, crossings 23, `gf!rot` 13; 67 to
  79 s). `r4n`: 0 failures, degenerate 77, `gf!rot` 22.
- Cascade (`--n 40`): identical to `bnd` under all three (157 steps, 0 failures, 0 checks, 0
  repairs, 26 midpoint tests).
- Anchorless: identical (16 cases, 17 chains resolved, 0 failures).
- Cut (`--n 200`, seeds 1 and 2): identical to `bnd` (7 and 1 oracle failures, every one an invalid
  `P`, as run 2 section 10 found; the bundle rule never fired).
- Polylines: `overlay_polyline.py` builds its overlay by hand and does not use the sector walk at
  six-end vertices (run 3 section 6: deduction across `LL` vertices only), so the rule does not reach
  it; `--all --n 200 --seed 1` was run once under the first build and was unchanged.

### 5.5 Adversaries and timing on `r4t`

**Measured** (`--columns bnd,r4t,r4tinv,r4tflt` on `chain` seeds 1, 2, 3, 5; `--columns bnd,r4t` on
seeds 4, 6 and the generators of 5.3; 860 to 1941 s each):

| `chain` seed | cases | `bnd` | `r4t` | `r4tinv` | `r4tflt` | `r4t` midpt | once | `r4t` changed (clean), area, far | `r4tinv` inverted, changed, area, far |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | 299 | **3**/4/2/1/3 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 61/59 | 81 (77) | 4 (1), `6.1 h^2`, `0.23h` | 61, 57, `1.1 h^2`, `0.25h` |
| 2 | 300 | **3**/3/2/1/3 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 66/64 | 84 (80) | 3 (0) | 66, 63, `1.2 h^2`, `0.32h` |
| 3 | 299 | **3**/3/2/1/3 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 44/42 | 82 (80) | 4 (1), `8.5 h^2`, `0.38h` | 44, 40, `1.2 h^2`, `0.11h` |
| 5 | 300 | **2**/2/0/2/2 | 0/0/0/0/0 | 0/0/0/0/0 | 0/0/0/0/0 | 62/61 | 69 (67) | 2 (0) | 62, 60, `1.4 h^2`, `0.23h` |
| 4 | 298 | **2**/2/2/0/2 | 0/0/0/0/0 | - | - | 61/59 | 81 (76) | 5 (3), `3.6 h^2`, `0.30h` | - |
| 6 | 298 | **1**/1/1/0/1 | 0/0/0/0/0 | - | - | 64/62 | 85 (83) | 1 (0) | - |

`r4tflt` equals `r4t` cell for cell; `r4tinv` inverts every midpoint test (they are all band pieces)
and changes the output only within `0.32h` of a boundary. On `shared` seeds 2 and 3 `r4t` is clean
with 262 and 249 midpoint tests (`bnd` 251, 242) and 3 and 2 changed clean outputs within `0.22h`;
`both_twice` is 1 vertex (seed 3), the only place the bundle rule still fires.

Timing (`--gen chain --n 60 --seed 5`, one column, three runs concurrent): `bnd` 93.8 s, `r4t` 93.9 s,
`r4n` 94.3 s; `six` 60 cases: `bnd` 54.3 s, `r4t` 54.4 s. The hunt's time is the oracle's; the rule
replaces an enumeration with a count.

## 6. Recommendation

**Adopt the one-passage rule (3.1) and restrict the spike rule as in 3.3** (`CONFIGS['r4t']`).
Measured: 0 oracle, check, repair, unresolved or pairing failures in 2393 `chain` cases over eight
seeds (where `bnd` fails 19), 1155 `shared` and 197 jittered, 600 `six`, 300 each of `near`, `rays`,
`tinyhole`, `zero`, the 463-case catalogue, 157 cascade steps, 16 anchorless cases and 400 cuts; the
fair inversion adversary and the float order change nothing (chain, four seeds); run 2's and run 3's
residue stays clean; the catalogue output is identical to `bnd`'s. What changes in clean outputs is
slivers within `0.52h` of a boundary (jittered `shared`) and `0.41h` elsewhere, in 3 to 22 cases per
300 (`six` most), from band pieces now midpoint-tested rather than read off an order. The bundle rule
stays in the design for vertices where both polygons pass more than once (1 vertex in 3548 cases).

Second choice: `r4n` (no spike rule), equally clean, with more degenerate rings dropped and more
midpoint tests in dense shared edges; it would be the simpler design (one rule out, one in) if the
restriction of 3.3 is judged too ad hoc.

Not recommended: the three local rules of 2.6 (one mechanism each; one of them wrong).

## 7. What changes for the C++ design (`phase4b-design.md`)

- **2.7 Rays, `Bundle`**: no new ray fields. The `Bundle` per vertex is built only when both polygons
  have four or more ends. A per-vertex `OnePassage` record (or a flag on the vertex) for the
  diagnostics: which polygon passes once, the deduced classes, whether the linking order differs from
  the measured one.
- **3.4 The loop rule and the spike rule**: the spike condition gains "and the other polygon has at
  most two ends at the tip `B`" (ends of non-internal pieces, counted at that point of `_contract`,
  a common ray's `Q` end counting once). Cite 2.4.
- **3.6 Rays and the rotational order**: "The order is used only at six or more ends" becomes "only
  at six or more ends where both polygons pass more than once (3.8); where one passes once, 3.7.1
  case 1b". The cone quantities stay (the bundle rule still needs them).
- **3.7.1 `solve_vertex`**, new case between the four-end rule and the sector walk:

  > **1b. Six or more ends, one polygon with exactly two ends** (`_is_once`, `:1032`): the
  > one-passage rule (`_solve_once`, `:1045`), combinatorial. Let `X` be that polygon and `a`, `b` its
  > two ends; for an end that is a plain ray, `m_in = ` its class; for a common ray with class `c`,
  > `m_in = c xor 1` (`same = 0`, `opposite = 1`). The constraint is `m_in(b) = m_in(a) xor n`, `n` the
  > number of `Y`'s non-common rays classified inside `X`, mod 2. With exactly one of these unknown,
  > deduce it (an `X` end's class, or the one unclassified `Y` ray's); with all known, a false
  > relation is a check failure. Memberships for linking: the sector walk on the order
  > `a, [Y rays inside X, measured relative order], b, [Y rays outside]`, or its mirror if that has no
  > solution (one of the two always has).
- **3.8 The bundle rule**: applies at six or more ends where both polygons have four or more ends.
  Run 3's residue cases (its 4.1 table) are all one-passage vertices and are now solved by 1b
  (measured, section 4).
- **3.9 Checks**: the one-passage relation is check 1 at such a vertex; nothing else changes. Repair
  unchanged.
- **3.10 Linking**: unchanged; the order it receives at a one-passage vertex is the built one.
- **9.1 What can fail**: add the three mechanisms as what the sector walk could get wrong at a
  six-end vertex where one polygon passes once, now excluded by construction; what remains is a
  vertex where both pass more than once with a noisy order (the bundle rule's residue, none seen).
- **11.2 The prototype comparison**: re-export under `r4t` (`overlay_export.py` uses
  `CONFIGS['bnd']` at `:185`; make the configuration a parameter) and regenerate
  `PolygonOverlayPrototypeTest`'s data; Phase 5 step 4's comparison must then show the C++'s
  one-passage deductions and built orders agreeing with the prototype's.
- **13 Rejected alternatives**, new rows: the along rule and the gap cone (one mechanism each; the
  gap cone needs the bundle enumeration at every dense chain end: run 4 section 2.6); silent
  one-polygon vertices (breaks `tinyhole#123`: 2.6); no spike rule (`r4n`: clean, but 77 against 48
  degenerate rings and more midpoint tests: 3.3); chord directions for the rotational order (the
  stored arcs' chords instead of the tangents: an embedding, but the snapped chords can cross without
  a record within the band, so the same class of failure elsewhere, and a change to every vertex;
  reasoning only, not built); a one-sided interval cone for common rays (needs an exact symbolic tie
  when a ray's chord *is* the other polygon's segment, as in mechanism A; not built).
- **Walk-through 7.4 and 7.6**: 7.4's rule is the two-end case of the one-passage rule; 7.6's "the
  classes alone do not say which of `Q`'s two passes `P` threads between" is exactly the condition
  for needing the order, and its "every ray leaves from that point" does not give a common ray one
  direction. 7.3 (spike rule) gains the tip condition. PLAN.md's chosen configuration gains the rule.

## 8. Open questions

1. `r4t` against `r4n`: whether the spike rule's restriction (a combinatorial condition found on one
   case) is preferable to dropping the spike rule, which the one-passage rule may have made
   unnecessary (measured: `r4n` is as clean; its cost is degenerate drops and midpoint tests, not
   failures). A developer decision.
2. The linking order at a one-passage vertex uses the measured relative order of the `Y` rays within
   a half. Measured: it changes ring structure only (`tie-dependent` 3 or 4 `six` cases); whether the
   C++ should prefer the fewest-inversions order from the whole measured order instead is a
   determinism choice, as run 3's open question 3 was.
3. The `six` generator's 18 to 21 extra midpoint tests per 300 cases are band pieces the walk used to
   deduce from a trusted order at necks. If Phase 5's real-data measurement (design 4.5) shows thin
   rings from them, the sliver filter is where to act, not the rule.
4. The fresh seeds 7 and 8 were measured under `r4` and `r4n` but not `r4t` (time); `r4t` differs
   from `r4` only where a spike is left in place, and matched `r4` on seeds 1 to 6 where run.

## 9. Files changed in the planning worktree (uncommitted)

`plans/polygon-intersection-tools/`:

- `overlay_proto.py`: options `once_rule` (`_is_once`, `_solve_once`, the call in `_solve_vertex`),
  `spike_tip_once` (in `_contract`), and the first build's `along_rule` (`_prev_seg`, `_segments_on`,
  `_is_along`, cone `pi` in `_build_rays`), `gap_cone` (`Ray.gap`, cone widening in `_build_rays`) and
  `one_polygon_silent` (in `_solve_vertex`); `_bundle_split` never anchors a cone of `pi` and records
  whether a gap cone caused a float; `_bundle_orders` keeps the measured order when nothing is
  anchored; class defaults for the subclasses; counters `once_vertices`, `once_deduced`,
  `once_moved`, `both_twice`, `along_rays`, `along_vertices`, `gap_floats`, `silent_vertices`,
  `bundle_unanchored` in `new_stats`; `CONFIGS` `r4`, `r4n`, `r4t`, `r4x`, `r4a`, `r4g`, `r4s`;
  `run_overlay` passes the options. `CONFIGS['bnd']` is unchanged (the catalogue under it reproduces
  run 3's table, measured).
- `overlay_hunt.py`: adversaries `r4`, `r4n`, `r4t`, `r4inv`, `r4tinv`, `r4flt`, `r4tflt`, `r4x`, `r4a`,
  `r4g`, `r4s` in `run_options` (`R4_RULES`); columns `--r4`, `--r4-short`, `--r4-attr`, and
  `--columns A,B,...` (any adversaries as V1b/freex columns); the output comparison against a
  column's base (`CHANGE_BASE`), sampling the differing region round every short piece where the base
  was clean; the run-4 counters in the per-case summary, the totals and a report line.

Scratch (session scratchpad, not deliverables): `diag.py`, `mech.py`, `geom.py`, `selfx.py`,
`probe_simple_gap.py`, `summ.py`, the patch scripts, and the hunt outputs (`r4-*.txt`, `r4t-*.txt`,
`r4tc-*.txt`, `probe-simple-gap.txt`, `time-*.txt`). No process was left running.
