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
the end drops it (section 7.6) and loses nothing that was ever represented.

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

Why the `LL` rule matters (run 2, section 3): without it a long single arc between two `LL`
records, which has a reliable midpoint, stays undetermined when its neighbours are, and was
midpoint-tested in 229 of 300 random cases; worse, two arbitrary band pieces meeting at an `LL`
vertex could both be called "bounce", consistent at every vertex and wrong by `10^4 h`. With it
the random hunt's remaining midpoint tests are band pieces (arcs lying within `h` of the other
boundary over their whole length, mostly a `P` edge running along a `Q` edge that a third ring
splits), whose class is arbitrary by nature: 1 in 300 `near` cases, 24 in `zero`, 111 in `rays`.

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
  rule and adversary, in exact arithmetic (run 2, section 6). Fixing it needs a new rule for
  clusters (a parity solve over their rays), which identity vertices never need.
- It moves the output by up to the cluster's width (`2.2h` to `10.8h` in run 2's generators), and
  drops every region thinner than the cluster, including real ones.
- It removes anchors: a touch record inside a cluster is internal, so a common chain that it
  anchored becomes anchorless and needs section 5's fallback (run 2, section 7).

What contraction bought, few midpoint tests and no tie dependence, the `LL` rule and the margin
order buy as well: V1b's remaining midpoint tests are band pieces (7.4), and its output does not
depend on the tie order except for ring structure in sub-`h` spurs.

### 7.8 Ties

Records with equal keys along one segment sort in no defined order. Under identity vertices a tie
between records of one vertex is one stop (section 2); a tie between different vertices (two `LL`
crossings converging at a twist, section 8) gives two orders of a zero-length piece of `Q` whose
class is deduced, and either order is a valid output, differing only in which operation gets the
pinched ring. Run 1 and run 2 ran every tie order; the output's point set never depended on it.

### 7.9 Clean-up, last

After linking: consecutive repeated positions are removed and counted; a ring with fewer than three
distinct positions is dropped and counted; nothing else is merged or dropped. The dropped rings are
the collapsed regions of 7.2, zero area by construction. Whether to also merge regions all of whose
points lie within about `h` of both boundaries (slivers of the true answer, which contraction used
to remove by accident) is a separate, distance-based rule on the output, for the design (Phase 4b).
