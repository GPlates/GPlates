# Phase 4a, run 2: breaking the clean variants

Run on 2026-10-29, after run 1 (`phase4a-run1.md`) and its explanation (`phase4a-variants.md`). The
candidates were V0/free, V1b/free and V2b/free; this run tried to break them with a random cluster
hunt, perturbations for rounding, a standing midpoint-inversion adversary, six-end vertices, a short
cascade, anchorless chains and a cut by an open arc, and measured what each costs. Tools, in the
planning repository's `plans/polygon-intersection-tools/`: `overlay_hunt.py`, `overlay_cascade.py`,
`overlay_anchorless.py`, `overlay_cut.py`, and the changed `overlay_proto.py` (section 1). Every
statement is marked **measured** (with the command that reproduces it) or **reasoning**.

Conventions: `h = s = 1.4142e-6` rad. A *column* is a variant, a vertex rule and an adversary:
rule `free` is run 1's geometry-free rule at four-end vertices; `freex` is section 3's extension;
adversary `inv` negates midpoint tests (section 4), `flt` computes the rotational order in doubles
(section 5), `lng` midpoint-tests the longest stalled single arc first (section 2). Counts are cases
with at least one of the thing, worst tie order, unless stated.

## 1. Changes to the prototype

All default-off, so `python -I overlay_catalogue.py` still reproduces run 1 (measured: section 9).

1. `invert_midpoints`, with `invert_band`: every midpoint test's answer is negated; with a band, only
   when the whole single arc lies within the band of the other polygon's boundary (its ends, midpoint
   and quarter points). Run 1's inversion had no band. The hunt's `inv` column uses `2h`.
2. `float_order`: the rotational order of the rays at a vertex computed in double arithmetic on
   normalised float directions (no exact predicate, ties where the sort leaves them). The exact order
   is always computed too and a vertex whose two orders differ is counted
   (`order_disagreements`, and `order_disagreements_used` for vertices where the order is used, that
   is not four-end under `free`).
3. `midpoint_order='longest'`: when deduction stalls, midpoint-test the longest stalled single arc,
   not the first in piece order (section 2 says why).
4. `anchorless='offset'|'crossings'`: walk-through section 5's options 1 and 3 (section 7). Option 3
   needed one correction found here: the path from `R` to the chain point must stay on `R`'s side of
   the chain arc's great circle, so the parity's random waypoint is restricted to that side
   (`_parity_on_side`). Without it the path arrived from the inner side in about half the runs and
   both memberships described the wrong side.
5. Rule `freex`: the free rule, and a four-end vertex that is a single `LL` record is a known
   crossing (section 3).
6. `spike_rule` (added last, section 2 finding 7): two consecutive single arcs of one ring from a
   vertex to another and straight back are internal, the two-arc case of the loop rule. Hunt column
   `spk` (with `freex` and the margin order).
7. Statistics: identity-group extents and record counts, displacement of each record from its
   vertex's output position, vertices with six or more ends and their smallest adjacent-ray angle;
   output rings carry the pieces they were walked along (`clean_rings_tagged`, for the sliver
   classification); `run_overlay` can share records between runs on the same inputs.

## 2. The random cluster hunt (task 1)

`overlay_hunt.py`: seven generators, each case reproducible from (generator, seed, index), every
tie order, the oracle of run 1 with 200 global and 200 annulus samples, margin
`M = max(10h, 2 x widest record cluster)`. Every polygon has a hole. The generators and what they
put at one point are in the file's docstring; in brief: `near` (vertices of one polygon 0 to `3h`
from the other's edges and vertices, up to three per case, both directions), `chain` (3 to 9 short
segments, `0.3h` to `1.9h`, across, along or zigzagging an edge, and `SS` chains along a dense edge,
roles swapped half the time), `tinyhole` (a hole of radius `0.3h` to `1.5h` within `2h` of the
boundary; two clusters `2h` to `4h` apart), `rays` (a `P` vertex 0 to `2h` from a `Q` vertex with a
`P` edge leaving `1e-7` to `1e-2` rad from `Q`'s edge), `six` (necks `0.2h` to `1.9h` wide of `Q`'s
exterior, exterior-and-hole, two holes, three rings within `h` of one point, `P`'s exterior and hole
touching on `Q`'s edge or vertex; roles swapped 30%), `zero` (a `P` segment of `1e-9` to `9e-7` rad
at a `Q` corner or edge, with extra vertices within `2h`), `shared` (neighbouring plates, offsets to
`2h`, interleaved, dense spacing `0.4h` to `1.2h` in 40%, a hole within `2h` of the shared edge in
40%).

**Failures** (measured; `python -I overlay_hunt.py --gen G --n 300 --seed 1` for run 1's
configuration, rule `free` and `first` order, with `--freex` and `--margin` for the two
improvements of sections 3 and 6; `chain` and `shared` are `--n 60 --seed 5 --margin`, the 300-case
run of `chain` being still in progress when this was written, see the end of this section; the
299-case runs of `shared` finished, 3000 to 4700 s each). Cells are oracle / check / repair counts
(cases); `-` not run; "spike rule" is the `spk` column, section 1 item 6, run on the failing cases
one by one at 299 cases and as a column at 60 and 100.

| generator | cases | V0 `free` | V1b `free` | V2b `free` | V0 `freex` | V1b `freex` | V2b `freex` | V0 margin | V1b margin | V2b margin |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| near | 300 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 |
| rays | 299 | 0/0/0 | 0/1/1 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 |
| tinyhole | 300 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 |
| zero | 299 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 |
| six | 300 | **33**/1/1 | **9**/0/0 | **19**/17/0 | 0/0/0 | 0/0/0 | **19**/19/2 | 0/0/0 | 0/0/0 | **19**/19/2 |
| six | 100 (seed 9, `--spike`) | - | - | - | - | - | - | 0/0/0 | 0/0/0 (spike rule: 0/0/0) | **7**/7/2 |
| near | 100 (seed 9, `--spike`) | - | - | - | - | - | - | 0/0/0 | 0/0/0 (spike rule: 0/0/0) | **1**/1/0 |
| chain | 60 | - | - | - | - | - | - | 0/1/1 | 0/0/0 (spike rule: 0/0/0) | **3**/3/0 (spike rule: **2**/2/0) |
| shared | 60 | - | - | - | - | - | - | 0/0/0 | 0/0/0 (spike rule: 0/0/0) | 0/1/1 (spike rule: 0/1/1) |
| shared | 299 (seed 1) | **5**/0/0 | **7**/4/0 | **18**/18/3 | 0/0/0 | **2**/4/0 | **10**/18/3 | **1**/1/1 | **3**/4/0 (spike rule: 2 cases) | **10**/18/3 |

Under the fair inversion adversary (section 4) the margin columns stay the same except: `shared`
V0 **5**/7/7 (oracle up to 1592 of 900, pairing failures: a band piece's class flipped breaks V0's
pairing of its zero-length pieces), V1b 0/1/1 (`shared#47`: an inverted band piece inside a dense
common chain makes the chain's two ends disagree, check 2 of section 9 fires, repair restores the
oracle), V2b 0/1/1; with the spike rule V1b under inversion is **1**/2/2 there (`shared#23`, a
dense case where the repair of the inverted band piece's consequences hits its twenty-round cap).
In the 299-case `shared` run under the margin order the fair adversary gives V0 **23**/37/37,
V1b **6**/13/9 and V2b **8**/21/8: dense shared edges hold many band pieces, and flipping them
one by one is where the repair policy of section 9.3 runs out (`R20`, the round cap, in most of
the failing cases). Under float order (section 5) nothing changes in any generator, spike rule
included.

Tie dependence (output rings differing between tie orders, ring structure only, oracle always
passed): V0 in 9 (`near`), 2 (`rays`), 5 (`tinyhole`), 135 (`zero`), 131 (`six`), 19 (`chain`), 15
(`shared`) cases; V1b 0 everywhere but one `six` case under some adversaries; V2b 0 except `six`
(14 to 17, its failing cases) and `chain` (1).

**Measurements** (V1b under `freex`; V0 and V2b where they differ; the columns are the hunt's
`grpext`, `clwidth`, `displ`, `sixcase`/`six`, `minang6`, `ordis/u`, `slivers`/`arb`):

| generator | V1b widest identity group (records) | V2b widest cluster | displacement V1b / V2b | V1b six-end vertices (cases) | smallest adjacent angle at them | float order disagreements used, any variant | V1b sliver rings (arbitrary) | V0 slivers (arbitrary) | V2b slivers (arbitrary) | midpoint tests V0 / V1b / V2b |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| near | 0 (2) | `1.85h` | 0 / `1.85h` | 15 (14) | `2.6e-5` | 0 | 55 (0) | 55 (0) | 27 (0) | 4 / 1 / 1 |
| rays | 0 (2) | `1.93h` | 0 / `1.93h` | 6 (6) | `1.0e-6` | 0 | 17 (0) | 17 (0) | 16 (0) | 119 / 111 / 111 |
| tinyhole | `1.14h` (4) | `3.69h` | `1.14h` / `3.69h` | 1 (1) | 0.12 | 0 | 157 (12) | 194 (46) | 152 (0) | 30 / 22 / 2 |
| zero | `1.88h` (6) | `2.23h` | `1.67h` / `2.23h` | 4 (4) | 0.17 | 0 | 28 (6) | 225 (212) | 21 (0) | 130 / 24 / 0 |
| six | `1.3h` (3) | `2.37h` | `1.3h` / `2.1h` | 213 (183) | `1.8e-15` | 1 (V0) | 28 (0) | 51 (16) | 24 (0) | 116 / 2 / 0 |
| chain (60) | `13.7h` (19) | `12.1h` | `13.7h` / `13.7h` | 15 (15) | `1.3e-8` | 0 | 85 (0) | 305 (258) | 19 (0) | 131 / 16 / 0 |
| shared (60) | `4.49h` (8) | `20.4h` | `4.49h` / `20.4h` | 6 (5) | `2.8e-14` | 0 | 417 (30) | 658 (383) | 236 (6) | 150 / 46 / 39 |

`M` was `10h` in every generator but `chain` (`28.5h`) and `shared` (`40.8h`), where the record
clusters reach `14h` and `20h`. The largest sliver areas (`7.2e3` to `8.8e3 h^2`) are slivers of
the true answer: the face between a `P` edge and the `Q` edge it runs along, or between two
jittered copies of a shared edge.

**Findings.**

1. **No candidate fails outside `six`, `chain` and `shared` in any configuration** (1198 cases
   of `near`, `rays`, `tinyhole`, `zero`): the clusters, tiny holes, nearly parallel rays and
   zero-length segments of the brief are handled by all three. **Every candidate fails somewhere
   in `six`, `chain` or `shared`** (findings 2 to 4 and 7).
2. **V0 fails `six` under run 1's configuration (33 of 300)** and is fixed by the test order.
   Mechanism (`python -I overlay_hunt.py --gen six --seed 1 --index 0 --variant V0 --verbose`, two
   holes of `Q` `1.36h` apart with `P`'s vertex between them): V0's two `SL` records at `x` are two
   vertices with a zero-length `P` piece between them; that piece is the first stalled single arc,
   its midpoint is `x` itself, inside the band of both hole edges, its exact parity is "inside `Q`",
   and the free rule deduces the long `P` pieces on either side from it, as a bounce, although `P`
   crosses the hole's boundary within `h` of `x` (the touch stands for the crossing). Every local
   check passes and 894 of 900 oracle points are wrong. Under `lng`, margin or `freex` the long
   pieces are decided first and V0 passes all 300. Reasoning: a zero-length piece can never be
   tested, only deduced, and V0 makes one at every multi-record point.
3. **V1b fails `six` under run 1's configuration (9 of 300)**, by the same ordering mechanism on
   a `1.2h` piece in a neck (section 6), and passes all 300 under `lng`, margin or `freex`.
4. **V2b fails `six` (19 of 300) and `chain` (3 of 60) under every configuration**: a cluster with
   six or more ends whose rays leave from different members (section 6; in `chain`, the `zigzag-ll`
   cases: `LL` crossings under `2h` apart contract into clusters up to `12h` wide). The margin
   order and `freex` do not touch it, since it is the rotational walk at a cluster that fails. With
   another seed it appears in `near` too (`--gen near --n 100 --seed 9 --spike`: `near#40`, two `P`
   vertices `0.07h` and `1.99h` from `Q` vertices, V2b oracle 59, check and pairing failures; V0 and
   V1b clean on all 100).
5. **Group extents and cluster widths are unbounded, with identical maxima under V1b and V2b**
   (`13.7h` in `chain`, 19 records; `28.8h` for 65 records in `probe_group_growth.py`'s dense
   shared edge at `0.45h` spacing), because V2b's clusters start from V1b's identity groups and the
   chain of `SS` records is what grows. V2b's clusters are wider where they merge by distance
   (`20.4h` in `shared`, `12.1h` in `chain`, against V1b's `4.49h` and `13.7h`), and its
   displacement equals its cluster width. Reasoning: the bound is the extent of the data's own
   coincidences; no fixed bound exists for either, and contraction adds to it.
6. **Repairs**: V1b needed none in any normal run of the five main generators; one under the
   inversion adversary (`shared#47`, caught by the chain check); V0 one in `chain#42` (an `SS`
   chain, 12 repairs, oracle passed) and 7 under inversion in `shared` (5 of them still failing the
   oracle); V2b one in `shared#8`.
7. **Dense jittered shared edges break every variant, V1b least.** In the 299-case `shared` run
   (40% of its cases *dense*: vertices `0.4h` to `1.2h` apart along the shared edge, offsets up to
   `2h`, interleaved) V0 fails 5, V1b 7 and V2b 18 under run 1's configuration; under `freex` with
   the margin order V1b still fails 4 of those 7 (`python -I overlay_hunt.py --gen shared --seed 1
   --index 28 --variant V1b --mode freex --adv mrg`; also 169, 234, 247), all dense, with check
   failures, unresolved pieces and pairing failures, 152 to 600 oracle points. Mechanism
   (`--index 247 --verbose`): the two rings' vertices coincide pairwise in interleaved order, so the
   identity groups make a ring visit the groups in the order `A, B, A` (`Q`'s three consecutive
   vertices within `h` of `P`'s at `A`, `B`, `A`): two sub-`h` single arcs there and back, which the
   loop rule (one arc) does not cover; the three pieces between `A` and `B` (one of `P`, two of `Q`)
   are not a common pair; and the sector walk at the six-end vertices `A` and `B`, with three
   nearly parallel rays along the edge, is inconsistent. The **spike rule** (section 1, item 6: two
   consecutive single arcs there and back are internal) fixes `shared#234` and `#247` but not `#28`
   and `#169`, where a ring revisits a group three times (`A, B, A, ...`) through a loop and two
   arcs. Reasoning: this is the same gap as V2b's cluster failure, seen from the other side: at a
   vertex with six or more ends the overlay still orders rays by angle, and along a shared edge
   within `h` the angles between the rays are noise. V1b meets it only when identity makes a ring
   revisit a vertex (dense coincidences), V2b whenever contraction joins vertices. Section 11 says
   what follows.

The 300-case runs of `chain` (`--n 300 --seed 1`, in every configuration) and the `--jitter 2`
and `--controls` runs at `--all --n 60` were still running after six hours (their cases have record
clusters up to `25h` wide, so the oracle's annulus sampling and up to 24 tie orders per case make
them an order of magnitude slower); the smaller runs above (`chain --n 60 --seed 5`, `--all --n 12
--jitter 2`, `--all --n 12 --controls`) stand in for them.

## 3. The free rule's blind spot at an `LL` vertex, and rule `freex`

**The blind spot** (measured: `python -I overlay_hunt.py --gen rays --seed 1 --index 81 --variant
V1b --adv lng --verbose`, and the same with `--mode freex`; reasoning for the mechanism). Run 1's
free rule at a four-end vertex is "P crosses iff its two classes differ, which must equal whether
Q's differ". At a vertex that is one `LL` record, the record itself says the two segments cross
(each crosses the other's thick plane with its ends on opposite sides), so each polygon's two
classes must differ; the rule as run in run 1 did not use that, and accepted "both the same" at an
`LL` vertex as a bounce. Two consequences:

1. **It deduces too little.** From one known class at an `LL` vertex the rotational walk deduces
   the other three; the free rule needs three known to deduce the fourth. So a long single arc
   between two `LL` records (a `Q` edge crossed twice by one `P` edge, say) stays undetermined when
   its neighbours are, and is midpoint-tested although it has a margin: V0 229, V1b 218 and V2b 197
   midpoint tests in 300 `near` cases (run 1's catalogue never showed this, because every `LL`
   vertex there had a vertex-tested piece). Such a test is reliable (the midpoint is far from the
   boundary), but it is a test the design said would be rare.
2. **It can be consistent and wrong.** `rays#81` (`P`'s vertex `x` at `Q`'s hole vertex `v`, `P`'s
   edge leaving `1e-5` rad from the hole's edge, so `P`'s edge `x`-`w` lies within `h` of the hole's
   edge `v`-`w` for `3160h`, and `P`'s own hole crosses the hole's edge at `v7`, `14.5h` before `w`,
   so the `Q` edge is split there and the two are not a common pair): under `lng` both band pieces,
   `P`'s `x`-`w` and `Q`'s `v`-`v7`, are midpoint-tested independently (each arbitrary), the free
   rule then accepts a bounce at the `LL` vertex `v7` for both polygons, every local check passes, and
   138 of 900 oracle points are wrong. (Under `first` order the tests happened to agree.)

**Rule `freex`**: at a four-end vertex that is a single `LL` record, each polygon's two classes must
differ; one known class deduces the other three; both the same is a check failure. Every other
four-end vertex (touch records, several records) keeps run 1's rule, since a touch can be a bounce
or a crossing. Reasoning: the crossing is read from the record kind, which `GeometryIntersect`
decided with the thick planes, so the rule stays geometry-free in the overlay.

Measured (`python -I overlay_hunt.py --gen G --n 300 --seed 1 --freex`; the summary rows are
labelled `fre` for `freex` by the printer): on `near`, `rays`, `tinyhole` and `zero` every candidate
is clean under every adversary, and the midpoint tests fall from 229 to 4 (V0), 218 to 1 (V1b) and
197 to 1 (V2b) on `near`; from 279 to 119, 271 to 111 and 270 to 111 on `rays`; from 42 to 30, 34
to 22 and 14 to 2 on `tinyhole`. The tests left are band pieces (on `rays`, all 111 of V1b's were
inverted by the banded adversary, section 4). The V1b check failure of `rays#96` under `free` (a
six-end identity vertex with three `1.27h` pieces, repaired to a passing output) does not occur
under `freex`. On the run-1 catalogue `freex` changes nothing (section 9).

## 4. The midpoint-inversion adversary (task 2c, task 4)

Run 1 negated every midpoint test and saw no change beyond the margin; run 2 keeps the adversary
on and measures the region it changes (dense samples within `6h` across and `3h` beyond every
midpoint-tested piece, classified against the normal and the inverted output for all three
operations; the area is the differing fraction of the sampled box, the distance is the farthest
differing sample from the nearer input boundary).

**Three forms of the adversary, and which is fair.** Measured on the hunt (`inv` columns of
`--freex` and `--margin` runs, `invall` in `--controls`, and the first batch's midpoint-only band):

1. *Every midpoint test negated* (run 1's form): fails the oracle in 5 of 8 `near` smoke cases
   (`O416`, region reaching the `6h` sampling limit, area `1.4e5 h^2`). Reasoning: under the free
   rule a long single arc with a reliable midpoint can be midpoint-tested (section 3), and negating
   a reliable test is simply a wrong class, which the checks then cannot see because the vertex was
   underdetermined. Run 1's catalogue had no such arc, so its conclusion ("either class is within
   tolerance") held there by accident of the catalogue.
2. *Negated when the midpoint is within `2h` of the other boundary*: still fails (`rays#153`,
   `O563`, all three variants): a long arc whose midpoint happens to pass near a `Q` vertex.
3. *Negated when the whole arc lies within `2h` of the other boundary* (ends, midpoint, quarter
   points): the fair form, since only then is either class within tolerance. This is the `inv`
   column of every `freex` and `margin` run.

**Under the fair adversary, with `freex` or the margin order** (measured, per generator, V1b;
V0 and V2b in the same runs): no oracle, check, repair, pairing or unresolved failure in `near`,
`rays`, `tinyhole`, `zero` or `six`, 1500 cases. The region changed:

| generator | midpoint tests negated, V1b | cases whose output changed | largest region (`h^2`) | farthest point from the nearer boundary |
| --- | --- | --- | --- | --- |
| near | 1 in 1 case | 1 | `1.9e3` (margin: 216) | `0.17h` (`0.28h`) |
| rays | 111 in 111 | 111 | `3.9e3` (`1.7e3`) | `0.45h` (`0.46h`) |
| tinyhole | 22 in 15 | 14 | 8.8 (9.2) | `0.43h` (`0.35h`) |
| zero | 24 in 24 | 24 | 0.94 | `0.21h` |
| six | 2 in 2 | 2 | 0 (none sampled differing) | 0 |

So an arbitrary-class piece decides a region of width under `0.5h` beside the band piece, as long
as that piece is (a band piece thousands of `h` long along a `Q` edge gives a few thousand `h^2`).
Under V0 the counts are larger (`six`: 113 negated in 87 cases, `9.7e3 h^2`, `0.41h`) because its
zero-length pieces are all band pieces. Under V2b almost nothing is negated (`six`, `zero`: 0;
`rays` 111, the same long band pieces, since a `3000h` arc does not contract).

**The slivers of the output** (the `slivers` / `arb` columns: output rings all of whose vertices
and edge midpoints lie within `3h` of both inputs' boundaries; *arbitrary* if walked along a
midpoint-tested piece): over the five generators under `freex`, V1b output 17 to 158 sliver rings
per 300 cases (most in `tinyhole`, where the tiny hole itself is a sliver of the true answer), of
which 0 to 13 arbitrary, with areas up to `7.2e3 h^2` (`rays`: a region between a `P` edge and the
`Q` edge it runs along) and arbitrary ones up to `1.1 h^2`; V2b 16 to 152, 0 to 3 arbitrary; V0 17
to 225, 0 to 212 arbitrary (`zero`: its zero-length pieces bound them). Under the margin order the
arbitrary counts are the same as under `freex`. Reasoning: slivers of the true answer dominate and
no variant removes them; arbitrary-class slivers are a V0 phenomenon, rare under V1b and absent
under V2b, and in every case thinner than `h`.

## 5. Rounding: jitter and the rotational order in doubles (task 2a, 2b)

**The rotational order in doubles (2b).** Every hunt column computes the exact order and the
float order at every vertex and counts the vertices where they differ (`ordis`), and among those
the vertices where the order is used (`ordis/u`: not a four-end vertex under `free`/`freex`); the
`flt` columns then run the whole pipeline on the float order. Measured (section 2's table, column
"float order disagreements used"): **0 disagreements at any used vertex under V1b or V2b in every
generator** (1618 cases), one under V0 (`six`, a six-end vertex V0 does not have, so it was at a
four-end vertex and unused). The `flt` columns' failure counts equal the exact columns' in every
generator and variant. Reasoning: the exact tangents of the two planes at one point are far apart
in angle unless the planes nearly coincide, and when they do (the neck, `1.8e-15` rad) the two
rays are adjacent and swapping them changes nothing (section 6). The C++ computes the tangents
`cross(n, v)` in doubles from a normal already rounded, which this float order does not model;
but a wrong sign needs two rays within about `1e-16` of each other, where the swap is harmless.

**Controls** (`python -I overlay_hunt.py --all --n 12 --seed 7 --controls --margin`, 84 cases,
every generator): V1b/rotation fails 4 (9 check failures), V2b/rotation 2 (7), against V1b/`freex`
margin 2 and V2b 0, so the rotational order at four-end vertices is still worse than the free rule
(run 1's result stands); V1/free 16, V2/free 2 (run 1's failures); `invall` (every midpoint test
negated, run 1's adversary) V0 16 and V1b 15 oracle failures of 84, which section 4 explains.

**Jitter (2a).** Measured (`python -I overlay_hunt.py --all --n 12 --seed 6 --jitter 2 --margin`:
84 base cases of every generator and two copies of each with every input coordinate moved by -4 to
+4 ulps and renormalised, 248 cases ran, 4 ambiguous): V0 and V1b 0 oracle, check or repair
failures in every column; V2b fails one `six neck` case and both its jittered copies alike (the
cluster mechanism of section 6, stable under jitter). No jittered copy failed where its base case
passed, in any variant. Reasoning: the exact records change under jitter only when a decision sits
within rounding of its threshold, which `arc_ref` reports as ambiguous and the hunt skips (4 of
252 here); away from the thresholds the overlay's decisions are exact functions of the records.

## 6. Six or more ends (task 3)

**How often, and how parallel.** Measured (the `sixcase` / `six` / `minang6` columns of the hunt
summaries, V1b; a vertex counts when it has six or more ends, a common ray counting two): `six`
213 vertices in 183 of 300 cases, smallest adjacent-ray angle `1.8e-15` rad; `near` 15 in 14,
`2.6e-5`; `rays` 6 in 6, `1.0e-6`; `zero` 4 in 4, `0.17`; `tinyhole` 1 in 1, `0.12`; `shared` and
`chain`: section 2's table. Under V2b the counts are higher (`six` 334 in 260, `near` 51 in 48),
since contraction joins four-end vertices into clusters; under V0 there are none (every record is
its own four-end vertex), which is why V0 had the most midpoint tests and the zero-length-piece
failures of section 2. The most ends at one vertex was 8 under V1b. The `1.8e-15` is run 1's
narrow neck: a `P` vertex between two parallel `Q` edges under `2h` apart, one identity vertex with
six ends and two rays that are parallel but for rounding.

**Float order at six-end vertices.** Measured (`ordis/u`, every hunt column): the rotational order
computed in doubles differed from the exact order at **no vertex where it was used** under V1b or
V2b in any generator (0 of all vertices in 1500 cases), and at one V0 vertex (`six`, no failure).
Reasoning: at exactly parallel rays the float and exact orders can only swap two adjacent rays,
and swapping two adjacent rays changes neither the sector memberships between them (the walk crosses
both either way) nor the pairing across the zero-width sector (it pairs the same two rays), so even
a disagreement there would not change the output; a disagreement that matters needs a third ray
between two nearly parallel ones, which the generators did not produce. Run 1's worry (the neck's
order "is a coin toss in doubles") is therefore harmless as well as unobserved.

**The failures at six-end vertices are not float failures.**

- **V2b fails 19 of 300 `six` cases** under every rule and adversary (oracle 69 to 1035 of 900,
  check 17 to 19, unresolved 16 to 17, pairing 17 to 19; `python -I overlay_hunt.py --gen six --seed 1
  --index 18 --variant V2b --verbose`, also indices 20, 26, 27, 29, 55, 82, 120, 135, 137, 141, 154,
  200, 206, 242, 245, 246, 248). The configuration: `P`'s edge runs down the middle of a neck of `Q`
  narrower than `2h` (two holes, or the exterior and a hole, or the neck form) for thousands of `h`,
  and the two `Q` vertices at the neck's end both lie on `P`'s edge (`LS LS`, `0.4h` to `1.3h`
  apart) while `P`'s vertex at the other end lies on both `Q` edges (`SL SL`). Under V1b the two
  `LS` records are two four-end vertices with a sub-`2h` `P` piece between them, each resolved by
  the free rule; under V2b that piece contracts them into one six-end cluster whose rays leave from
  two member positions, the two `Q` rays along the neck nearly parallel to `P`'s, and the rotational
  walk round the cluster is inconsistent (`C1`), leaves a piece unresolved and cannot pair. This is
  run 1's section 6 case ("a six-end cluster with the same geometry has no geometry-free rule yet"),
  and it occurs in 6% of the `six` generator's cases. It is exact-arithmetic failure: the exact
  order is wrong for the snapped topology, not rounded.
- **V1b fails one `six touching-vertex` case under `freex` with the margin order** (`--all
  --n 12 --seed 7 --controls`: `six#5`, `[LL LS LS SL SL SS SS]`, oracle 247, one unresolved piece,
  pairing failure; `P`'s exterior and hole touching at a vertex that lies on a `Q` vertex, with the
  hole's apex within `h` as well), not seen in the 300 seed-1 cases: the same six-end mechanism as
  finding 7 of section 2, at an identity vertex of `P`'s two rings and `Q`'s corner.
- **V1b fails 9 of 300 `six` cases under `free` with `first` order** (indices 20, 135, 141, 154,
  200, 242, 245, 246, 248; oracle 102 to 703) and **0 under `lng`, `margin` or `freex`** with any
  adversary. Mechanism (`--index 135 --variant V1b --verbose` against `--adv lng`): `P`'s edge runs
  nearly along the neck, the two `Q` neck vertices touch it (`LS LS`), and the `1.17h` piece of `P`
  between them is the first stalled single arc in piece order, so it is midpoint-tested (an
  arbitrary answer inside the band) and the free rule at the `LS` vertex, whose `Q` classes are
  known, deduces the long `P` piece beyond from it: wrong by `10^4 h`. Testing the piece with the
  largest margin first (the long one) and deducing the band piece from it is right every time.
  Under `freex` the long piece is already deduced through an `LL` vertex before any midpoint test.
- **V0 fails 33 of 300 under `free` with `first` order**, 0 under `lng`, `margin` or `freex`: the
  same ordering mechanism with its zero-length pieces (section 2).

**V3.** Reasoning: float ordering never caused a failure, so the brief's trigger for V3 did not
fire; but the exact rotational order itself fails at six-end vertices whose rays run together
within `h` (V2b's clusters, 19 of 300 `six` and 3 of 60 `chain`; V1b's dense shared edges, 4 of
299, and `six#5` of seed 7). The extension needed is the same for both: at a vertex with six or
more ends, rays that are within the tolerance of each other (leaving along the same thick plane)
form one bundle whose internal order is not geometric; the bundle's pieces are band pieces and
their classes should be solved from the parity constraints of the vertex (run 1's V3 idea) rather
than from the order. It was not built in this run; it is the first item for Phase 4b or the Phase 5
hunt (section 11).

## 7. Anchorless chains (task 6, Q3)

`overlay_anchorless.py` builds whole-ring common chains: `Q`'s ring identical to `P`'s (and reversed),
every `Q` vertex jittered within `0.9h` of `P`'s, a hole of `P` equal to `Q`'s ring (identical and
jittered), `P`'s ring equal to a hole of `Q`, a thin face (`P` with a second hole `2h` or `1.5h`
inside its ring, or `Q` likewise, so the face beside the chain is thinner than option 1's `3h`
offset), and both rings of `P` coinciding with both rings of `Q`. Every polygon has an ordinary hole
too. Each case under V0, V1b and V2b with fallback `none`, `offset` (option 1) and `crossings`
(option 3), every tie order, against the oracle.

Measured (`python -I overlay_anchorless.py`, 20 cases, 180 rows):

- With no fallback every case has 7 (two-rings: 11) anchorless common `P` pieces, 14 (22)
  unresolved pieces and 245 to 630 oracle failures of 900: the chain is dropped. The run-1 count of
  0 anchorless chains was a property of its catalogue, not of the overlay.
- **Option 3 (`crossings`) is right in every case under every variant**: 0 oracle failures, 0
  unresolved, 0 check failures, including the thin faces and the reversed rings. One chain arc is
  classified and the sector walks carry the class round the chain (`anchorless_arcs_max = 7`).
- **Option 1 (`offset`) fails every thin-face case** (303 of 900 oracle failures, all three
  variants, both `2h` and `1.5h`): the point `3h` to the side lands in the face beyond the thin one,
  as walk-through section 5 predicted. It is right on the other cases.
- Option 3 as written in the walk-through needed one correction (section 1, item 4): the
  crossing count must follow a path that stays on `R`'s side of the chain arc's great circle, which a
  parity test through a random waypoint does not guarantee. With the waypoint restricted to `R`'s
  side it is exact; the C++ entry that "leaves out given edges" must also fix the side it approaches
  from (a direct arc from `R`, or a waypoint on `R`'s side).
- **V2b loses anchors that V0 and V1b keep**: in `thin-face-P jittered` one `Q` vertex lies on a
  `P` edge (`SL`), which anchors the chain under V0 and V1b (0 anchorless), but under V2b the `SL`
  vertex and its neighbouring `SS` vertex are `1.1h` apart, the pieces between them are contracted,
  and the chain leaves the cluster as common pairs only: 7 anchorless pieces, and the fallback is
  needed (both options then behave as above). Reasoning: contraction makes the fallback more
  frequent, since any anchor inside a cluster is internal.

Decision input for Q3: option 3, with the side rule.

## 8. The cascade (task 5, Q8)

`overlay_cascade.py`: three to five plates in a row, each with its own copy of every shared edge
(1 to 12 vertices at random positions along it, each offset 0, up to `h`, or up to `2h` sideways,
so the two copies interleave), each with a hole; a star-shaped subject `P` with a hole is partitioned
by the first plate, its remainder (`P \ Q1`, the output rings as they come) by the second, and so
on, with an oracle at every step against the original inputs beyond `M` from every boundary seen.

Measured (`python -I overlay_cascade.py --n 40 --seed 1` and `--mode freex`; 40 cascades, 157
steps, one tie order per step), maxima over the cascades per step:

| step | cascades | records | remainder vertices | V0/V1b group extent | V2b cluster width | V2b displacement | oracle fail (any variant) | midpoint tests V0, V1b, V2b (`free`) | same (`freex`) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | 40 | 6 | 34 | 0 | 0 | 0 | 0 | 0, 0, 0 | 0, 0, 0 |
| 1 | 40 | 25 | 55 | 0 | `1.61h` | `1.61h` | 0 | 10, 10, 0 | 8, 8, 0 |
| 2 | 40 | 26 | 64 | 0 | `1.63h` | `1.63h` | 0 | 12, 12, 1 | 10, 10, 0 |
| 3 | 24 | 20 | 64 | 0 | `1.52h` | `1.52h` | 0 | 5, 5, 0 | 5, 5, 0 |
| 4 | 13 | 19 | 73 | 0 | `1.77h` | `1.77h` | 0 | 3, 3, 0 | 3, 3, 0 |

No oracle, check, repair, pairing or unresolved failure at any step under any variant or rule;
`M = 10h` throughout. Neither identity groups nor clusters grow along the cascade: the widest cluster
at step 4 (`1.77h`) is no wider than at step 1, and no identity group joins two records (extent 0:
the copies' vertices are at random positions along the edge, so `SS` coincidences are rare; the
`chain ss-chain` generator of section 2 is where groups chain). The remainder's vertex count grows
by the records (34 to 73 over four steps), as it must. Reasoning: a cluster is bounded by the
geometry at one step (the sub-`2h` pieces between records of the two boundaries present), and the
next step sees the previous output's vertices as ordinary vertices, so nothing accumulates unless a
later plate's boundary is itself within `2h` of an earlier cut, which the shared-edge jitter here
(`<= 2h`) produces and still did not widen.

The midpoint tests under V0 and V1b (about 10 per step at steps 1 and 2) are pieces of the jittered
shared edge between touch records, within the band, that are not common pairs because the two
copies' records interleave (section 3, the `rays#81` mechanism); V2b contracts them. Reasoning for
Q8: the sequential cascade is safe for the overlay as measured; the all-plates-at-once question is
about slivers (section 4), not correctness.

## 9. The run-1 catalogue rerun

Measured (`python -I overlay_catalogue.py`, 463 cases, every tie order, 329 s; and
`--mode freex`, 184 s), with the changed prototype at its defaults:

| variant / rule | oracle | check | repairs | degenerate dropped | tie-dependent | output crossings |
| --- | --- | --- | --- | --- | --- | --- |
| V0 / free | 0 | 0 | 0 | 55 | 90 | 23 |
| V0 / freex | 0 | 0 | 0 | 55 | 90 | 23 |
| V1 / free | 19 | 19 | 0 | 87 | 5 | 23 |
| V1b / free | 0 | 0 | 0 | 77 | 5 | 23 |
| V1b / freex | 0 | 0 | 0 | 77 | 5 | 23 |
| V1b / rotation | 10 | 23 | 4 | 77 | 9 | 23 |
| V2 / free | 5 | 5 | 0 | 65 | 0 | 9 |
| V2b / free | 0 | 0 | 0 | 60 | 0 | 9 |
| V2b / freex | 0 | 0 | 0 | 60 | 0 | 9 |
| V2b / rotation | 0 | 2 | 0 | 60 | 0 | 9 |

The same as run 1's section 2 and its addendum (V1/rotation 28 oracle against 29, the sampling
difference the addendum already noted), so the run-2 options at their defaults change nothing, and
`freex` changes nothing on the catalogue either: every `LL` four-end vertex there was already
determined by its vertex-tested pieces.

## 10. Cutting by an open arc (task 7)

`overlay_cut.py`: a star `P` with a hole, cut by one open great-circle arc (one segment, or a
two-segment polyline) whose ends lie outside `P`, in six kinds: plain crossings; a `P` vertex 0 to
`2h` from the arc; a `P` edge running along the arc within `h` (both vertices on it); a hole of `P`
touching the arc at a vertex; a reflex vertex of `P` touching the arc from one side (a notch, two
sectors of one face at the vertex); and the polyline. The output is the faces of `P` on either side:
every piece of `P` kept once, every piece of the arc inside `P` kept twice, as the front and back
edges. The oracle: a sample point beyond `M` from `P`'s boundary and from the arc is in exactly one
face if it is in `P` and in none otherwise; two points `1.5M` either side of the arc at the same
point along it, both inside `P`, are in different faces.

**What the core supports as it is** (measured: the `Cut` class subclasses `Overlay` and reuses its
records, projection keys, identity vertices, loop rule, contraction, `Piece`, `Vertex`, `Ray`, exact
ray directions and the exact rotational sort unchanged): everything up to and including the rays.

**What it needed** (the three additions in `overlay_cut.py`; reasoning for each, measured that it
works):

1. *Pieces of an open geometry*: the arc's first and last pieces dangle (one end is the arc's end
   point, not a vertex) and the arc does not wrap round. The polygon core builds pieces ring by
   ring; an open geometry is a second case of the same builder.
2. *Classification of the arc's pieces without classes for `P`'s pieces.* The overlay classifies
   `P`'s pieces as inside or outside `Q`; an arc has no inside. The cut classifies only the arc's
   pieces (inside `P` or not): a piece with an original arc vertex is tested there; a dangling piece
   at the arc's end (outside `P` by construction); and the rest by parity along the arc, flipping at
   each vertex where an odd number of `P`'s non-common rays leave on the positive side of the arc
   segment's plane. The side of a ray is `arc_ref.side` of its far vertex against that plane, the
   same thick decision `GeometryIntersect` made for its records, so a far vertex within the thickness
   counts nothing (a `P` edge running along the arc). The parity and the vertex tests agree in every
   valid case (`parity_disagreements = 0`).
3. *Linking by faces, with pieces used in both directions.* The pairing is the overlay's (the two
   rays bounding each sector inside `P` are paired), but an arc ray inside `P` bounds a face on each
   side, so it carries two nodes, one per side, and its piece is walked once per node; a walker
   leaving on a ray's anticlockwise side arrives at the other end on that ray's clockwise side. A `P`
   piece needs no orientation: on arrival the walker takes whichever side of the `P` ray is inside
   `P`, which is what lets a self-crossing `P` (walk-through section 8) through. Rings of `P` with
   no record are attached to the face that contains them. The first version walked "leave along the
   previous ray", which assumes `P` has a consistent left side and failed on the twisted inputs the
   generator makes; the node pairing does not.

The vertex rule: the sector memberships inside `P` at a vertex are anchored by any arc ray (its
class holds on both its sides) and flip at every `P` ray, so the pairing needs the rotational order
only to know which rays are adjacent, exactly as the polygon overlay does at six or more ends. At a
four-end vertex where `P` passes once and the arc once, the pairs follow from the sides alone
(the `P` ray on side `+` pairs with the arc piece inside `P` on side `+`, and likewise `-`; at a
bounce from one side with the arc inside `P`, same-polygon pairs, as Q11), so the geometry-free
rule carries over with "side of the arc's plane" in place of "class".

Measured (`python -I overlay_cut.py --n 200 --seed S` for `S` = 1, 2, 3; 600 cases, V1b; and seed 1
under V0 and V2b):

| variant, seeds | cases | oracle fail | straddle fail | check | parity disagreement | unresolved | of the failing cases, `P` invalid |
| --- | --- | --- | --- | --- | --- | --- | --- |
| V1b, 1+2+3 | 600 | 13 | 0 | 1 | 0 | 0 | 13 of 13 |
| V0, 1 | 200 | 7 | 0 | 0 | 0 | 0 | 7 of 7 |
| V2b, 1 | 200 | 7 | 0 | 0 | 0 | 0 | 7 of 7 |

Every failing case is one where the generator's moved vertices made `P` invalid (its exterior ring
self-crossing, or its hole outside the exterior; flagged `P-INVALID` in the output), with 1 to 2
sample points wrong of 300, in the island or the twisted lobe; the one check failure (seed 3,
index 24) is a `P` edge along the arc crossed by another edge of `P`'s own ring. No valid case
failed, and no straddling pair was ever in one face. Six-end vertices did not occur (`max ends 4`),
since the arc is simple and `P` was built with one ring near it. Reasoning for the
`DateLineWrapper` decision: the core carries over; the port needs the three additions above and the
pole-to-pole arc's ends outside `P` (a polygon round a pole is the case not run here).

## 11. Recommendation

**Recommended: V1b, rule `freex`, stalled single arcs midpoint-tested in order of margin, and
option 3 for anchorless chains.** That is: an overlay vertex is the set of records naming a common
original vertex (identity, no distance); a single arc from a vertex back to itself is internal (the
loop rule); at a four-end vertex the geometry-free rule, with the crossing known at an `LL` record;
the rotational order (exact) only at six or more ends; a piece with an original vertex tested
there, a single arc deduced first and, if still undetermined, tested at its midpoint, the one with
the largest margin first; a whole-ring common chain classified by crossings from `R` on `R`'s
side; clean-up last. Measured, in this configuration: 0 oracle failures, 0 check failures and 0
repairs in 1498 random cases of `near`, `rays`, `tinyhole`, `zero` and `six` (seed 1), 60 `chain`
and 60 `shared`, the 463-case catalogue, 157 cascade steps, 20 anchorless cases and 600 open-arc
cuts; under the fair midpoint-inversion adversary 0 oracle failures in those generators and one
repaired check failure in 60 `shared` cases (`shared#47`); under the float rotational order no
change; tie-independent output in every case but one (ring structure). The spike rule (section 1,
item 6) belongs with it: it is the loop rule's two-arc case and removed half of the residue below.
**Residual failures**: 4 of 299 dense `shared` cases (seed 1; 2 with the spike rule), 6 of 299
under the fair inversion adversary there, and 1 of 84 mixed cases (`six#5`, seed 7), all at
identity vertices with six or more ends whose rays run together within `h` (section 2, finding 7;
section 6), which is the one mechanism left in every variant and the first thing for Phase 4b to
design out (a bundle rule at such vertices, section 6 "V3"). Section 7 draft:
`phase4a-section7-draft.md` (since moved into `overlay-walkthrough.md` as section 7).

The two improvements are needed, and they are rules, not thresholds: without `freex` or the margin
order V1b fails 9 of 300 `six` cases (a band piece's arbitrary class propagated to a long piece
through the four-end rule), and either one alone fixes all 9; both are recommended, `freex` because
it also removes nearly every midpoint test on long arcs (229 to 4 in `near`) and the margin order
because it is the principle (never deduce a piece with a margin from one without) that `freex`
happens to enforce at `LL` vertices only.

**Rejected: V2b (identity, loop rule, then contraction of sub-`2h` pieces into clusters, and any
single arc with both ends in one cluster internal).** It fails: 19 of 300 `six` cases, 3 of 60
`chain` cases and 18 of 299 `shared` cases, in exact arithmetic, under every rule, test order and
adversary. The failure is the
cluster with six or more ends whose rays leave from different member positions (`P`'s edge along a
neck of `Q` narrower than `2h` with the neck's vertices on it; `LL` crossings under `2h` apart in a
zigzag), where the rotational walk is inconsistent and leaves pieces unpaired; V1b keeps these as
four-end vertices that the geometry-free rule resolves. Fixing it would need a parity-solve rule
for clusters (run 1's V3), on top of contraction. Its costs even where it works: the output moves
by the cluster width (`20.4h` in dense shared edges, `12.1h` in zigzag chains, against V1b's
identity-group extents of `4.5h` and `13.7h` in the same cases, which both share); every region
thinner than a cluster is dropped, real ones included (46 degenerate rings in `near`, V1b 12);
touch records inside clusters stop anchoring common chains, so the anchorless fallback is needed
more often (section 7); and it is the only variant that merges by distance, with a threshold (`2h`)
that must stay consistent with `GeometryIntersect`'s. What it bought, no midpoint tests and no tie
dependence, `freex` and the margin order give V1b as well (section 3). The developer's acceptance
of an early merge if it tested better does not apply: it tested worse.

**Rejected: V0 (every record its own vertex).** Under run 1's configuration it fails 33 of 300
`six` cases: a zero-length piece between two records at one point is the first stalled single arc,
its midpoint is the record position inside the band, and the free rule deduces the long pieces from
it. The margin order or `freex` fix that (0 of 300), but what remains is structural: zero-length
pieces that can only be deduced, never tested, at every multi-record point; the most midpoint tests
of any variant (116 to 150 per generator run, against V1b's 2 to 46, in `six`, `zero`, `chain`,
`shared`); the only variant whose output depends on the tie order (9 to 135 cases per generator,
ring structure only); the most arbitrary-class slivers (212 of 225 in `zero`, V1b 6 of 28); and under
the fair inversion adversary 5 of 60 dense shared-edge cases fail the oracle outright, with pairing
failures (V1b 0). It is also the variant the walk-through's own argument (7.2 of the draft) says
is wrong in principle: the wedge's two records are one point, not two.

**Rejected: the rotational order at four-end vertices** (run 1, confirmed: V1b/rotation fails 10
catalogue cases) and **V1, V2** (run 1: 19 and 5 catalogue cases, reproduced in section 9).

**Not needed: V3.** No float disagreement was ever used by V1b, and its six-end vertices never
failed; the one six-end failure mode found belongs to contraction.

**Open for Phase 4b** (not decided here): the distance-based merge of output slivers (section 4:
slivers of the true answer up to `8.8e3 h^2` are output by every variant); the identity-group
extent, unbounded in principle (`28.8h` in a `0.45h`-spaced shared edge), which contraction would
not reduce; whether the C++ keeps the all-vertices diagnostic (run 1's suggestion; unchanged here).

## 12. Review addendum (2026-10-29, the session's review of this run)

- **Reproduced** (measured): `python -I overlay_hunt.py --gen six --n 60 --seed 1 --margin
  --no-measure --list-failures` fails V2b in six cases (`six#18`, `#20`, `#26`, `#27`, `#29`,
  `#55`), as section 6 lists, and nothing else; V1b's residual failures `shared#28` and `shared#247`
  (`--gen shared --seed 1 --index N --variant V1b --mode freex --adv mrg`) fail as section 2,
  finding 7, says.
- **An inconsistency in section 11**: "Not needed: V3 ... its six-end vertices never failed"
  contradicts the residual V1b failures, which section 2 (finding 7) and section 6 place at six-end
  identity vertices whose rays run together within `h`, and section 6's own conclusion that a bundle
  rule is needed for both variants. Read section 11's "Not needed" as: V3 is not needed *for float
  rounding*; the bundle rule (a parity solve at a six-end vertex over rays that leave along one
  thick plane) is needed, and is the first open item for Phase 4b.
- **How realistic the residue is**: the failing `shared` cases are *dense* (vertices of both copies
  of a shared edge `0.4h` to `1.2h` apart along it, offsets up to `2h`, interleaved), about 4 to 11 m
  between vertices. Real plate boundaries are rarely sampled that densely, but duplicated or
  near-duplicated vertices are common in data, so the bundle rule must still be designed rather than
  dismissed.
- **The longer runs** (300-case `chain`, the 60-per-generator jitter and controls runs) did not
  finish; no process was left running.
