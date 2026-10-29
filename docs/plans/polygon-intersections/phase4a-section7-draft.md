# Draft: `overlay-walkthrough.md` section 7, "One point, several records"

Written 2026-10-29 after Phase 4a run 2 (`phase4a-run2.md`), for the recommended variant: identity
vertices with the loop rule (V1b), the geometry-free rule at four-end vertices with the `LL`
crossing known (`freex`), stalled single arcs midpoint-tested in order of margin, and the
rotational order only at vertices with six or more ends. It replaces the current section 7
("Contracting short pieces"), whose premise, that short pieces must be contracted by distance, run 1
and run 2 refuted. Section numbers below refer to the walk-through. Notation as there: `h` is the
thickness, now `s`; a *record* is one contact `GeometryIntersect` reports.

## 7. One point, several records

### 7.1 What a record names

Splitting at every record (section 2) assumed the records are at distinct points. They often are
not. Each record names zero, one or two original vertices:

| record | meaning | names | its position |
| --- | --- | --- | --- |
| `LL` | two segment interiors cross | nothing | computed, where the planes meet |
| `SL`, `LS` | a vertex of one polygon lies on a segment of the other | that vertex | the vertex itself |
| `SS` | a vertex of each polygon coincide | both | `P`'s vertex |

A touch record sits *at the touching vertex*, not at its projection onto the other segment. The
projection only orders the record along that segment.

Where several records land on one point, they do so because they name the same vertex. The acute
wedge: `Q` has a corner at `v`, opening downwards, and `P` passes horizontally through a vertex `x`
just below `v`, within `h` of both of `Q`'s edges:

```
            v
           / \
   ---------x---------   P
         /     \
        /   Q   \
```

`GeometryIntersect` reports `x` on `Q`'s left edge and `x` on `Q`'s right edge: two `SL` records, at
the same point, both naming `x`. Farther down the two would be two `LL` crossings `P` makes with the
corner; here they have collapsed onto `x`. Neither can be dropped (run 1 dropped one and `Q`'s ring
came out as one piece with one class, consistent and wrong in 15 of 25 cases); and they are not two
places.

### 7.2 Vertices are identities

**An overlay vertex is the set of records that name a common original vertex**, closed
transitively: an `SS` record joins a vertex of `P` to a vertex of `Q`, so `SS(x, y1)` and
`SS(x, y2)` make `{y1, x, y2}` one vertex although `y1` and `y2` are distinct vertices of `Q`. An
`LL` record names nothing and is always a vertex of its own. No distance is measured: two records
are one vertex because they say so, and the only merging by distance in the system is
`GeometryIntersect`'s own coincidence test, which is what put them at one point.

The wedge's two `SL` records are one vertex, with six ends: `P` in and out, `Q`'s left edge in
both directions from the touch, `Q`'s right edge likewise. Nothing has zero length. The piece of
`Q` from the left touch up round `v` and down to the right touch has an original vertex, `v`, so it
is classified there (section 3) and kept or not like any piece. Its output ring is `x, v, x`: the
positions of a region thinner than `h`, which the records had already collapsed; the clean-up at
the end drops it (7.9) and loses nothing that was ever represented.

Each vertex is output at one position: its `P` original vertex if it holds one (so `P`'s vertices
stay exact, which scalar coverages want), else its `Q` one, else the record's position.

**The cost of identity.** A vertex built from a chain of `SS` records, `x1 ≡ y1 ≡ x2 ≡ y2 ≡ …`,
can be as wide as the chain: two boundaries digitised along each other with vertices every `0.45h`
make one vertex `28.8h` wide for 65 records, output at one point (run 2, `probe_group_growth.py`).
The oracle still passes, because the whole chain lies within `h` of both boundaries, but the output
has moved by the chain's extent. This bound is set by the data, not by the overlay; a boundary with
vertices a few metres apart along another is the only way to reach it.

### 7.3 The loop rule

Identity alone fails in one way (run 1: every V1 failure). `Q` has two vertices `y1`, `y2`, `0.8h`
apart, both within `h` of `P`'s vertex `x`:

```
          y1 ---- y2        Q's short edge
       ----- x -----        P
```

Both are `SS` records with `x`, so `{y1, x, y2}` is one vertex, and `Q`'s edge `y1`-`y2` starts and
ends at it: a loop. It has no original vertex inside and no useful midpoint (it lies on `P`), and
its two rays run almost along `Q`'s through rays, so the walk round the vertex cannot order them.

**A single arc that starts and ends at one vertex is internal**: not classified, not a ray, not
output. `Q` then passes through the vertex once, arriving at `y1` and leaving at `y2`. The arc is a
there-and-back inside the vertex's extent, so it bounds nothing. The rule tests identity, not
length, so it needs no threshold; the arc's length is bounded by the extent of 7.2.

A loop *with* an original vertex is not a loop in this sense: the wedge's tip piece `x, v, x`
is classified at `v` and kept.

### 7.4 Four ends: no geometry

At a vertex where each polygon passes once there are four ends: `p1`, `p2` of `P` and `q1`, `q2`
of `Q`. Section 4.3 said the rotational order is needed to tell a crossing (`p q p q`) from a
bounce (`p p q q`). It is not, and it is wrong exactly where it matters: the records already
encode the snapped topology, and the rays describe the unsnapped one. With a sub-`h` piece at the
vertex the two can disagree, and every rotational-order failure of runs 1 and 2 was of that kind.

**The rule.** `P` crosses `Q` at the vertex iff `p1` and `p2` have different classes; `Q` crosses
`P` iff `q1` and `q2` do; and the two must agree. So:

- three known classes fix the fourth (`q2 = not q1` if `P`'s differ, `q2 = q1` if not);
- four known classes that disagree are a check failure (section 9);
- **at a vertex that is one `LL` record the crossing is known from the record**, since each
  segment crossed the other's thick plane: both polygons' classes must differ, one known class fixes
  the other three, and "both the same" is a check failure. A touch record can be either, so only
  the first two rules apply there.

**Pairing** (section 6) at a crossing: the sector `(in P, in Q) = (a, b)` is bounded by the `P`
piece whose class is `b` and the `Q` piece whose class is `a`, so each output sector names its two
rays; at a bounce, `P`'s two rays pair and `Q`'s two pair (the pinch pairing, which Q11's split then
separates if it has to).

**Why the `LL` rule matters.** Without it two things go wrong (run 2, section 3).

*It deduces too little.* The first two rules need three known classes to find the fourth. Take a
`Q` edge crossed by two edges of `P`, at `LL` records `u` and `w`:

```
            |           |
   Q -------u-----------w------- Q
            |           |
            P           P
```

`Q`'s piece from `u` to `w` is a single arc, with no original vertex to test. At `u` the other
three pieces (`P`'s two, and `Q`'s piece before `u`) may be single arcs too, with nothing known
yet, so the piece `u`-`w` stays undetermined and goes to a midpoint test. That test is reliable
(the midpoint is far from `P`), but the design wants midpoint tests rare, and without the rule 229
of 300 random cases needed one. The record at `u` already says that `P` and `Q` cross there, so
each polygon's two classes at `u` must differ, and any *one* known class at `u` fixes all four.
Deduction then runs along a chain of crossings from a single tested piece, and the 229 cases fell
to 4.

*It can accept a contradiction.* At a touch, "`P`'s two classes are the same, and so are `Q`'s"
is a bounce, which a touch can be. At an `LL` record it is impossible, since the segments cross.
Run 2's `rays#81`: `P`'s edge runs within `h` of a `Q` edge for thousands of `h`, and a hole of `P`
crosses that `Q` edge (an `LL` record), splitting it, so the two run-together pieces are not a
common pair. Both are band pieces (they lie within `h` of the other boundary all along), both were
midpoint-tested with arbitrary answers, and the answers happened to make the `LL` vertex a bounce
for both polygons. Every check passed, because a bounce satisfies the first two rules, and the
pieces deduced from these two came out wrong: 138 of 900 oracle points. With the rule, a bounce at
an `LL` vertex is a check failure. With the margin order of 7.5 the case does not arise at all:
the long pieces nearby are tested first and the band pieces are deduced from them.

*Why only `LL`.* A touch record can stand for a crossing or a bounce (a vertex of `P` on `Q`'s
edge may continue to the other side or come back), so only the classes can say which. A crossing
record can only be a crossing. The rule reads that from the record kind, which `GeometryIntersect`
decided with its thick planes, so the overlay still makes no geometric decision.

With the rule, the random hunt's remaining midpoint tests are band pieces (arcs lying within `h`
of the other boundary over their whole length, mostly a `P` edge running along a `Q` edge that a
third ring splits), whose class is arbitrary by nature: 1 in 300 `near` cases, 24 in `zero`, 111
in `rays`.

### 7.5 Single arcs: deduce first, test by margin

A piece with an original vertex is tested there (section 3). A single arc between two records is
first deduced from its end vertices (7.4) to a fixed point. One that is still undetermined is
tested at its midpoint, and **the stalled arc whose midpoint is farthest from the other polygon's
boundary is tested first**, then deduction runs again.

The order matters, and run 2 shows why. In a neck of `Q` narrower than `2h`, with `P`'s edge
running along it:

```
     Q ---------+         +---------- Q
                 \  y1   /
   P --------------=====--------------   P's edge, y1 and y2 on it (LS LS)
                 /  y2   \
     Q ---------+         +---------- Q
```

`P`'s piece between `y1` and `y2` is `1.2h` long and lies inside the band: its class is arbitrary,
and either answer is within tolerance *for that piece*. Tested first, its arbitrary class reached
the long piece of `P` beyond `y2` through the four-end rule at `y2` (where `Q`'s classes are
known), and the long piece came out wrong by `10^4 h` in 9 of 300 cases. Tested in order of
margin, the long piece is tested first (its midpoint is `5000h` from `Q`) and the band piece is
deduced from it. The principle: a class decided inside the band must never be the source of a
deduction for a piece that has a margin. Length is a proxy for margin; margin is the rule.

Under this order, negating every midpoint test whose arc lies wholly within `2h` of the other
boundary (run 2's standing adversary) changed only slivers: regions whose points lie within
`0.5h` of the nearer boundary, of at most a few thousand `h^2` (a band piece thousands of `h`
long and under `h` wide), in 1500 random cases, with no oracle or check failure.

### 7.6 Six or more ends: the rotational order, and what is left of it

A vertex with six or more ends (the wedge, a neck, three rings of `Q` within `h` of one point,
`P`'s exterior and hole touching on `Q`'s edge) is resolved by the sector walk of section 4.3, in
the exact rotational order of its rays: the classes alone do not say which of `Q`'s two passes `P`
threads between. Three things keep this safe:

- The rays are exact tangents of the segments' planes at the vertex's position, ordered with exact
  orientation signs, never by a difference of two nearby positions. In doubles the order agreed with
  the exact order at every six-end vertex in 1500 random cases, down to an adjacent-ray angle of
  `1.8e-15` rad (run 2, section 6).
- Two exactly parallel rays (the neck's two `Q` edges, one from each side) can only be swapped with
  each other, and swapping adjacent rays changes neither the memberships between them nor the pairing
  across the zero-width sector, which pairs the same two rays either way.
- Every class at the vertex comes from an original-vertex test, a deduction anchored on one, or a
  margin-ordered midpoint test (7.5), so the walk has reliable constraints; disagreement is a check
  failure (section 9), not a silent error.

Run 2 found no failure of this walk under identity vertices in 1500 random cases of five
generators and the 463-case catalogue. It did find one, in two settings: under contraction (7.7),
and under identity in dense jittered shared edges, 4 of 299 cases (two boundaries digitised along
each other with vertices `0.4h` to `1.2h` apart and offsets up to `2h`, so that a ring's
consecutive vertices coincide with the other's in the order `A, B, A` and it revisits a vertex),
plus one case of two rings of `P` touching at a `Q` corner. In all of them several rays leave the
vertex along the same thick plane, and their order by angle is noise, not geometry. The rule that
is missing, and that Phase 4b should design, treats such rays as one *bundle*: the pieces between
bundled rays are band pieces whose classes are solved from the vertex's parity constraints, not
read off an order. Until then those cases fail the checks of section 9, visibly.

A two-arc there-and-back of one ring between two vertices (`A, B, A` with no other vertex) is the
two-arc case of the loop rule and is made internal likewise (the *spike rule*); it fixed half of
the dense shared-edge failures and is part of the recommended configuration.

### 7.7 Why not contract by distance

The earlier section 7 proposed contracting every piece without an original vertex shorter than
`2h`, merging its ends into one cluster. Run 1 and run 2 measured it (variant V2b):

- It fails. `P`'s edge running down a neck of `Q` with the neck's two vertices on it (7.5's
  picture) is two four-end vertices `0.4h` to `1.3h` apart under identity, each resolved without
  geometry; contraction merges them into one six-end cluster whose rays leave from two different
  points, nearly parallel, and the rotational walk round it is inconsistent: 19 of 300 cases, every
  rule and adversary, in exact arithmetic (run 2, section 6). Contraction makes it worse in two
  ways. It *creates* six-end vertices: identity keeps this configuration as two four-end vertices,
  which need no geometry at all (7.4), and contraction joins them into one that needs the
  rotational order. And it *spoils* that order: a cluster's rays leave from different member
  positions, up to the cluster's width apart, so their order round the cluster is not the order at
  any one point and can be wrong even in exact arithmetic, whereas every ray at an identity vertex
  is a tangent at one position. Identity vertices are not free of the problem: when several of
  their rays leave along one thick plane, the order is noise there too, and they need 7.6's bundle
  rule. But that took dense coincident boundaries (4 of 299 cases), where contraction's failure
  came from an ordinary neck (19 of 300). Fixing contraction would need the bundle rule at every
  cluster, on top of contraction.
- It moves the output by up to the cluster's width (up to `20h` in run 2's generators, in dense
  shared edges), and drops every region thinner than the cluster, including real ones.
- It removes anchors: a touch record inside a cluster is internal, so a common chain that it
  anchored becomes anchorless and needs section 5's fallback (run 2, section 7).

What contraction bought, few midpoint tests and no tie dependence, the `LL` rule and the margin
order buy as well: V1b's remaining midpoint tests are band pieces (7.4), and its output does not
depend on the tie order except for ring structure in sub-`h` spurs.

### 7.8 A twist on `Q`, and ties

`P`'s ring is `A, D, C, B`: the edges `A`-`D` and `C`-`B` cross at `X`, so `P` is a bow-tie, and
`X` lies on `Q`'s top edge, with `Q` below:

```
  A               C
  | \           / |
  |   \       /   |
  Y_L---- X ----Y_R      Q's top edge (Q below)
  |   /       \   |
  | /           \ |
  B               D
```

`Q`'s edge is crossed twice at `X`, by `A`-`D` and by `C`-`B`. Both are `LL` records, which name no
original vertex, so they are two vertices, `E` (on `A`-`D`) and `F` (on `C`-`B`), even though here
they have equal keys along `Q` and the same position (run 1, section 4.3). No record is ever *at*
`X`: it is a vertex of neither polygon. As `Q`'s edge rises towards `X` from below (section 8), `F`
and `E` converge, coincide when the edge passes through `X`, and swap order along `Q` above it.
This is FHP's one failure (their Fig. 21): they label each intersection crossing or bouncing and
alternate entry and exit along each ring, and two intersections at one point have no order to
alternate in.

**Seen as one point**, `X` has six rays: `P`'s four, towards `A`, `B`, `C` and `D`, and `Q`'s two,
along its edge towards `Y_L` and `Y_R`. Going anticlockwise from `Q`'s edge on the right, each
sector's (in `P`, in `Q`):

| Sector | Where | (in `P`, in `Q`) |
| --- | --- | --- |
| `Y_R` to `C` | upper part of the right lobe | (1, 0) |
| `C` to `A` | above `X`, between the lobes | (0, 0) |
| `A` to `Y_L` | upper part of the left lobe | (1, 0) |
| `Y_L` to `B` | lower part of the left lobe | (1, 1) |
| `B` to `D` | below `X`, between the lobes | (0, 1) |
| `D` to `Y_R` | lower part of the right lobe | (1, 1) |

Each ray flips one membership (`P`'s rays flip `P`, `Q`'s flip `Q`), as it must. `P ∩ Q` is the two
`(1, 1)` sectors, the lower parts of the lobes: two triangles touching at `X`, which is the result
FHP's Fig. 21 expects. `P \ Q` is the two `(1, 0)` sectors, the upper parts.

**What the overlay sees** is two four-end vertices with a zero-length piece of `Q` between them,
in whichever order the tie gives. Say `F` comes first along `Q`, as it would if `Q`'s edge were a
hair below `X`:

```
  A               C
  | \           / |
  |   \       /   |
  |      \ /      |      X a hair above Q's edge
  |      / \      |
  Y_L---F---E---Y_R      Q's top edge (Q below)
  |   /       \   |
  | /           \ |
  B               D
```

Each vertex has its own four rays and its own walk:

| Vertex | Sector | Where | (in `P`, in `Q`) |
| --- | --- | --- | --- |
| `F` | `E` to `C` | between the lobes, above `Q`'s edge | (0, 0) |
| `F` | `C` to `Y_L` | left lobe, above | (1, 0) |
| `F` | `Y_L` to `B` | left lobe, below | (1, 1) |
| `F` | `B` to `E` | between the lobes, below | (0, 1) |
| `E` | `Y_R` to `A` | right lobe, above | (1, 0) |
| `E` | `A` to `F` | between the lobes, above | (0, 0) |
| `E` | `F` to `D` | between the lobes, below | (0, 1) |
| `E` | `D` to `Y_R` | right lobe, below | (1, 1) |

Together they are the six sectors round `X`, with the two between the lobes each split across the
zero-length piece `F`-`E`. Each vertex is an ordinary crossing, which the `LL` rule of 7.4 knows
from the record. `P`'s pieces at each have original vertices (`C` and `B` at `F`, `A` and `D` at
`E`), so they are tested. The zero-length piece is deduced at either end: it is outside `P`,
bounding `(0, 0)` above and `(0, 1)` below. Its direction is `Q`'s edge, from `Q`'s plane, never
`E` minus `F`, so its zero length costs nothing.

The outputs, read off the tables: `P ∩ Q` pairs `Y_L` with `B` at `F` and `D` with `Y_R` at `E`,
giving the two triangles `Y_L, F, B` and `E, Y_R, D`. `P \ Q` pairs `C` with `Y_L` at `F` and `Y_R`
with `A` at `E`, giving one ring, `Y_L, A, E, Y_R, C, F`: section 8's ring, its twist shrunk to the
point `X`, through which it passes twice. The other tie order, `E` first, is `Q`'s edge a hair
above `X`, and gives the mirror image: `P ∩ Q` one ring through `X` twice, `P \ Q` two triangles.
Both orders describe the same point set and differ only in ring structure. Run 1 ran both, and the
oracle passed both. The ring through `X` twice passes there at two different vertices, `E` and
`F`, so section 6.4's split (which looks for a repeated vertex) does not see it: it is the input's
twist carried through (section 8), not something the overlay made.

**Ties in general.** Records with equal keys along one segment sort in no defined order. Under
identity vertices a tie between records of one vertex is one stop (section 2). A tie between
different vertices, as here, gives two orders of a zero-length piece whose class is deduced, and
either order is a valid output. Run 1 and run 2 ran every tie order, and the output's point set
never depended on it.

### 7.9 Clean-up, last

After linking: consecutive repeated positions are removed and counted; a ring with fewer than three
distinct positions is dropped and counted; nothing else is merged or dropped. The dropped rings are
the collapsed regions of 7.2, zero area by construction. Whether to also merge regions all of whose
points lie within about `h` of both boundaries (slivers of the true answer, which contraction used
to remove by accident) is a separate, distance-based rule on the output, for the design (Phase 4b).
