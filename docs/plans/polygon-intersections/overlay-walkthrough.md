# The overlay, explained

Written 2026-10-25 and 26, while walking the developer through `phase2-requirements.md`
section 3.3 one paragraph at a time. Section 3.3 is the summary, good for coming back to once the
algorithm is understood but hard to follow on a first reading. This file explains it from the
start, with examples, and records what the walk-through corrected. When partitioning is
implemented, this file becomes the basis of the design doc's section 5.3 onwards (Phase 6),
rewritten against the code, with section 3.3 as the overview. Until then it describes a design,
not code, so it stays in the plan (AGENTS.md, *Planning documents*). Section 7 was replaced on
2026-10-30 from Phase 4a's prototype runs (`phase4a-run1.md`, `phase4a-run2.md`, explained in
`phase4a-variants.md`): the old section 7 proposed contracting short pieces by distance, which the
runs refuted.

Notation: `P` is the geometry being cut (the subject), `Q` the polygon cutting it (the clip), `t`
the thickness of `GeometryIntersect` today (about `1e-6` radians), and `s` the point-on-outline
threshold (about `1.414e-6` radians). Audit fix F1 raises the thickness to `s` (open question 1,
decided); from section 3 on it is called `h`. FHP is Foster, Hormann and Popa 2019 (requirements section 1.3).

## 1. Overview

The overlay computes `P ∩ Q` and `P \ Q` (partitioning's two outputs), or any other Boolean
operation, in five steps:

1. Intersect all the rings of both polygons with `GeometryIntersect`.
2. Split every ring of both polygons at every intersection, into *pieces*.
3. Classify each piece as inside or outside the other polygon: with one point-in-polygon test at
   an original vertex, or, for a piece with none, by deduction from its end vertices (section 7).
   Pieces where `P` and `Q` run together (*common pieces*) cannot be tested; the *sector rule*
   classifies them instead.
4. Keep the pieces that bound the output.
5. Link the kept pieces into rings, and group the rings into polygons with holes.

A piece with an original vertex is classified on its own, by one exact test with a margin; a
piece with none is deduced from its neighbours, and the order of section 7.5 confines what a wrong
class can reach. The checks compare the classes meeting at every vertex, so a disagreement is
reported where it is, not carried along a ring; they cannot see every error (sections 7.4 and
9.1 give the cases). FHP labels each intersection vertex
instead and alternates entry and exit along each ring, so one wrong label inverts the rest of the
ring, and nothing in the output shows it.

## 2. Pieces

Take every ring of both polygons: `P`'s exterior and holes, and `Q`'s exterior and holes. Mark on
each ring every point where `GeometryIntersect` reported a contact with the other polygon: a
crossing, a touch, or a vertex of a stretch where the two run together. Those are the
*intersection vertices*. A *piece* is the stretch of one ring from one intersection vertex to the
next, with any original vertices between. A ring with no intersection vertex is one closed piece.

Example: `P` is the lower-left square and `Q` the upper-right one, crossing at the two `x`s.

```
            +-----------+ Q
            |           |
  P +-------x---+       |
    |       |   |       |
    |       +---x-------+
    |           |
    +-----------+
```

`P`'s ring has two pieces: `p1`, round `P`'s top-right corner between the `x`s, and `p2`, the long
way round. `Q`'s ring has two: `q1`, round `Q`'s bottom-left corner, and `q2`, the rest. Here
`p1` is inside `Q`, `p2` outside, `q1` inside `P` and `q2` outside, so `P ∩ Q` is `p1` with `q1`,
and `P \ Q` is `p2` with `q1`.

This is FHP's structure after its first phase: the parts of each list between consecutive
intersection vertices. The difference is what gets labelled. FHP labels the vertices (crossing or
bouncing, entry or exit); the overlay labels the pieces, and FHP's labels follow from them. A
vertex where the pieces on either side have the same class is a bounce: a touch from outside just
splits an outside stretch in two. A vertex where their classes differ is a crossing.

Some vocabulary used below:

- A piece has two *ends*, one at each of its vertices. A closed piece that leaves a vertex and
  comes back to it (a ring with one intersection vertex) has both ends at that vertex.
- A *common piece* is a piece of `P` and a piece of `Q` lying together. It counts as two ends at
  each of its vertices, one of each polygon, but leaves the vertex as one *ray*.

## 3. Classifying a piece

A piece has no contact with the other polygon in its interior: every contact became a vertex, so
it is an end of some piece. (This relies on `GeometryIntersect` missing no contact, R11.) So a
piece lies on one side of the other polygon's boundary, apart from within the tolerance of that
boundary, and the class of any one of its points well away from that boundary is the class of the
whole piece. A twist of `P` inside a piece does not matter: the piece is still on one side of `Q`.

**The test is `is_point_in_polygon` with the threshold off**: strict parity, without "within `s` of
the outline counts as inside". The inclusive threshold suits a user's query, but here it would
call a piece running just outside `Q`, within `s` of it, inside. The pieces must divide the sphere
exactly, and agree with the oracle of PLAN.md Phase 5. (Open question 1.)

**The test point is an original vertex of the piece, when it has one.** A vertex of `P` that is in
no intersection record is farther than the thickness (`t` today, `s` after fix F1) from `Q`'s
boundary, or `GeometryIntersect` would have reported it as a touch. So the test has a margin of at
least the thickness, about `10^10` times point-in-polygon's rounding.

The first proposal was the midpoint of the piece's longest arc, on the reasoning that the ends are
the only places where `Q` is known to be near. That is wrong, as the developer showed
(2026-10-26). `GeometryIntersect` reports a *vertex* within `t` of the other geometry; it says
nothing about an arc that leaves an intersection vertex at a small angle, and such an arc stays
close to `Q` for much of its length. The developer's example, a T-intersection:

```
  +----\
    o---+-o
    |
    |
    o
```

`P`'s edge runs from its vertex `A` (top left) down to its vertex `B`, which touches `Q`'s top
edge. Moving `A` down makes the edge nearly parallel to `Q`'s top edge, passing just above `Q`'s
corner `C` (more than `t` from it, so `C` is not a touch). Take `Q`'s top edge as `y = 0`, with `C`
at `x = 0`, `B` at `(b, -δ)` and `A` at `(a, y_A)`, `a < 0 < b`. `B` is a touch, so `|δ| <= t`; with
`δ > 0` it lies inside `Q`. The edge passes `C` at height `y_C > t`. When `A` is just left of `C`
(`a` near 0), the midpoint is at height `(y_A - δ)/2`, about `(y_C - δ)/2`. With `B` on the outside
(`δ <= 0`) that is more than `t/2`: safe. With `B` inside `Q` it is positive but can be arbitrarily
small, when `y_C` is just above `t` and `δ` just below it: then the edge crosses `Q`'s top edge
near its middle with no record (the touch at `B` stands for the contact), and the midpoint test is
decided by rounding. The vertex `A` is more than `t` from `Q` whatever happens, so it is the point
to test.

Had the midpoint been tested, the result would not have been silent: a wrong class for this piece
contradicts the tests of `Q`'s pieces at `B` (the four-end rule, section 7.4), and the checks
find it.

**A piece with no original vertex** is a single arc between two intersection vertices, and there
the midpoint is the only point, without a guaranteed margin. If the arc's two ends lie on the same
edge of `Q`, with no vertex of either between, it is a common piece, and section 4 applies. If
not, it is deduced from its end vertices first, and tested at its midpoint only if still
undetermined, the one farthest from the other boundary first (section 7.5). The audit's D3, a
vertex in a corner's wedge reported once per edge, is one vertex here (sections 7.1 and 7.2).

## 4. Common pieces: the sector rule

### 4.1 The problem

A *common piece* is one where `P` and `Q` run together within the tolerance, as two plates do
along a shared boundary in the cascade. Its points lie on the other polygon's boundary, so a
point-in-polygon test of any of them means nothing. Today's code decides this case by the ring's
orientation, which is the fourth bug of PLAN.md's Status.

Each side of a common piece has two memberships, (in `P`, in `Q`). Crossing a common piece crosses
both boundaries, so both memberships flip, and only two cases are possible:

- The sides are `(1,1)` and `(0,0)`. The interiors are on the *same* side, and the piece bounds
  `P ∩ Q` (and the union).
- The sides are `(1,0)` and `(0,1)`. The interiors are on *opposite* sides, like neighbouring
  plates, and the piece bounds `P \ Q` (and `Q \ P`).

So the only question is which, and it is answered at an end of the chain, without testing any
point on it. (A chain is a run of common pieces; vertices inside it, where an original vertex of
either polygon lies along the other, have just the two common ends, and the memberships carry
through them unchanged.)

### 4.2 At a chain end

At a vertex `v`, `P` and `Q` stop running together: `P`'s ring leaves along a piece `p`, `Q`'s
ring along a piece `q`, and `c` is the chain. Three rays leave `v`, making three sectors. Let
`a = [q inside P]` and `b = [p inside Q]`; `p` and `q` are ordinary pieces, so these are reliable
tests.

- The sector between `p` and `q` borders both, so it is `(in P, in Q) = (a, b)`.
- Crossing `p` flips `P`: the sector between `p` and `c` is `(not a, b)`.
- Crossing `q` flips `Q`: the sector between `q` and `c` is `(a, not b)`.

The two sides of `c` are `(not a, b)` and `(a, not b)`, and one of them is `(1,1)` exactly when
`a != b`. So **`c` is in the intersection if and only if `[q inside P] != [p inside Q]`**, and in
`P \ Q` otherwise. The two sides flip into each other across `c`, which checks the arithmetic.

**Neighbouring plates.** `Q` is to the right of `P` and shares part of `P`'s right edge, from `u`
to `v`:

```
        +-------+
        |   Q   |
  +-----u       |
  |  P  |c      |
  |     v-------+
  +-----+
```

At `u`, `p` is `P`'s top edge going left, outside `Q`, so `b = 0`; `q` is `Q`'s left edge going up,
outside `P`, so `a = 0`. At `v`, `p` is `P`'s right edge going down and `q` is `Q`'s bottom edge
going right, and again `a = b = 0`. So `c` goes to `P \ Q`, which is right: the interiors are on
opposite sides, and `P` keeps `c` as boundary.

**`Q` inside `P`, sharing part of `P`'s right edge:**

```
  +-----------+
  |     P     |
  |     +-----u
  |     |  Q  |c
  |     +-----v
  |           |
  +-----------+
```

At `u`, `p` is `P`'s right edge going up, outside `Q`, so `b = 0`; `q` is `Q`'s top edge going left
into `P`, so `a = 1`. At `v` likewise. So `c` goes to `P ∩ Q`, which is right: `P ∩ Q = Q`, whose
boundary includes `c`. `P \ Q` is `P` with a notch cut out; its boundary includes `Q`'s other three
edges but not `c`.

Ring orientation is never used, which is what fixes the fourth bug.

### 4.3 What it needs, and how it differs from FHP

**No geometry at a three-ended vertex.** With three rays, every pair of rays is adjacent: the
three sectors are the three pairs, whatever the angular order. So the rule above needs no order,
and it does not matter if `p` and `q` leave `v` nearly tangent.

**Why four or more ends need the rotational order.** With four rays there are four sectors but
six pairs, and two of the pairs (the opposite rays) bound no sector. Which pairs those are depends
on the angular order. At a vertex where `P` meets `Q` with no common piece, with ends `p1`, `p2`
of `P` and `q1`, `q2` of `Q`, the order is either `p q p q` (`P` crosses `Q`) or `p p q q` (`P`
bounces off `Q`), and the sectors, and so the memberships the walk assigns them, differ. The order
is exactly FHP's crossing-or-bouncing information, read from the geometry. Two things limit the
need for it:

- It is needed only to deduce a common chain at a vertex with four or more ends, or to pair four
  or more kept ends (section 6). Ordinary pieces are classified by their own tests and need
  nothing at the vertex.
- The pieces' own tests check it: `p1` and `p2` have the same class exactly when `P` bounces,
  which must agree with the order `p p q q`.

(Section 7.4 goes further: where each polygon passes a vertex once, the classes alone decide
crossing or bounce, and the order is not used at all. It is needed only at six or more ends.)

**The general vertex.** Sort the ends by angle round `v` (the tangent of each piece's first arc).
Start from a sector that a non-common piece fixes, and go round, flipping `P` at a `P` end, `Q` at
a `Q` end, and both at a common ray. Every other non-common piece fixes a membership too, and they
must agree: a disagreement is a detected error.

**Against FHP.** FHP decides a chain (delayed crossing or bouncing) from side tests at its ends:
on which side of `P`'s chain `(P-, I, P+)` the neighbour `Q-` (or `Q+`) lies, and likewise at the
other end. The signed areas have the chain itself as their base, and in this data the chain's two
copies lie within the tolerance of each other, which makes those the fragile decisions. The sector
rule uses only
the tests of `p` and `q`, made away from the chain, and either end of a chain decides it alone, so
the other end is an independent check.

## 5. A chain with no anchor

A chain that is a whole ring of `P` and a whole ring of `Q` (the two coincide within the tolerance
all the way round) meets no non-common piece anywhere, so nothing fixes it. This may be less rare
than section 3.3 implies: partitioning a polygon by an identical one (assigning plate IDs to static
polygons using the same polygons, or topological plates built along them), a hole of `P` equal to
a ring of `Q`, and the cascade, when what is left of `P` after the earlier plates is exactly the
next plate.

The options:

1. **A point offset sideways from the ring** by more than the tolerance, tested against both
   polygons (requirements section 3.3). It fails when the face beside the ring is thinner than the
   offset.
2. **FHP's rule**: keep the ring in the intersection exactly when both rings are holes or both are
   not. Under the parity rule a ring's role is the parity of its containment by its polygon's other
   rings, decided by point tests of one of its vertices against them, which is fragile when rings
   touch.
3. **Crossings from `R`, leaving out the chain's own arc** (the developer's idea, 2026-10-26). Pick
   a point `X` on the chain: the midpoint of the chain arc whose great circle is farthest from `R`
   (the largest `|dot(R, axis)|`). Count the crossings of the arc from `R` to `X` with `P`'s edges,
   and separately with `Q`'s, exactly as `is_point_in_polygon` does, but leaving out the edge of
   `P` and the edge of `Q` that the arc containing `X` lies on. The arc from `R` to `X` meets that
   arc's great circle only at `X`, so the points just before `X` lie on `R`'s side of it, and the two parities
   are the memberships `(in P, in Q)` on that side; the other side has both flipped. Nothing is
   offset, so no thin face can defeat it, and the crossing tests are point-in-polygon's own exact
   sign tests (audited in #106). The developer's worry, the arc from `R` arriving parallel to the
   chain at `X`, is why `X` is chosen on the arc farthest from `R`'s great circles: it fails only if
   `R` lies on the great circle of every arc of the ring, which no ring does. Cost: one test per
   polygon at the slow speed, `O(N)`, only for anchorless rings. It needs a point-in-polygon entry
   that leaves out given edges, and whose path to `X` stays on `R`'s side of the left-out arc's
   great circle: a direct arc from `R`, or a waypoint on `R`'s side. (Phase 4a run 2: a path
   through an unrestricted random waypoint arrived from the wrong side in about half the runs, and
   both memberships then described the other side.)
4. **An offset arc, intersected with both polygons**, testing the midpoint between the chain and
   the first intersection along it (also the developer's). It works, but `GeometryIntersect` cannot
   see a boundary within `t` of the arc's start, which is on the chain, so it needs care; option 3
   does the same with exact tests that already exist.

**Decided: option 3** (developer, 2026-10-30; the sector rule is the primary rule, 2026-10-26).
Phase 4a run 2 measured both on whole-ring chains, identical and jittered within `h`, holes equal
to rings and thin faces: option 3 was right in every case, and option 1 failed every thin face, as
predicted.

## 6. Linking the kept pieces into rings

### 6.1 Which pieces are kept

For `P ∩ Q`: `P`'s pieces inside `Q`, `Q`'s pieces inside `P`, and the common chains whose sides
are `(1,1)` and `(0,0)`. For `P \ Q`: `P`'s pieces outside `Q`, `Q`'s pieces inside `P`, and the
common chains whose sides are `(1,0)` and `(0,1)`. A piece of `Q` inside `P` is kept by both, as
`q1` is in the two-squares example. Other operations: requirements section 3.3, "Union and the
other Boolean operations".

### 6.2 Ends come in even numbers

At every intersection vertex the number of ends is even, counting a common piece as two. Each pass
of a ring of `P` through the vertex arrives and leaves, so `P` has an even number of ends there,
and so does `Q`. (The number of *rays* can be odd: a chain end has four ends, `p`, `q` and the
chain's two, but three rays.) An identity vertex (section 7.2) adds the ends of its records, and
the loop and spike rules (7.3, 7.6) remove two ends of one ring at a time, so the count stays
even. A polyline's end point can have one end, but polylines are not linked.

The kept ends are even too. Going round a vertex, output membership flips at every kept end and
nowhere else, and it must come back to where it started.

### 6.3 Pairing

At each vertex the kept ends are paired, and a ring is a walk that arrives along one end of a pair
and leaves along the other.

- **Two kept ends (the usual case):** one pairing, no geometry. The kept pieces form cycles, and
  walking them closes the rings. No direction is needed either: GPlates never normalises ring
  orientation, so a `Q` piece may run either way relative to the `P` pieces it joins, and the walk
  just follows the pieces.
- **Four or more:** the two ends bounding each output sector are paired, so the rings touch at
  the vertex but never cross. Where each polygon passes once, the sectors follow from the classes
  (section 7.4); at six or more ends, from the ends sorted by angle round the vertex (7.6).

### 6.4 A ring that passes through a vertex twice

Pairing across output sectors can still give one ring that passes through a vertex twice. `P` is a
square, and `Q` a triangle inside it with its apex `v` on `P`'s top edge:

```
  +------v------+
  |     / \     |
  |    / Q \    |
  |   +-----+   |
  |      P      |
  +-------------+
```

There is one intersection vertex, `v`, so each ring is one closed piece: `P`'s from `v` round the
square back to `v`, and `Q`'s from `v` round the triangle back to `v`. Each has both its ends at
`v`, so `v` has four ends and four rays: `P`'s top edge to the left (`L`) and to the right (`R`),
and `Q`'s left (`a`) and right (`b`) sides. For `P \ Q` both pieces are kept, so all four ends are
kept. In angular order the rays are `L`, `a`, `b`, `R`, and the sectors between them are: `L` to
`a`, in `P` and not in `Q` (output); `a` to `b`, inside `Q` (not output); `b` to `R`, output again;
and `R` to `L`, above `P` (not output). Pairing across output sectors gives `{L, a}` and `{b, R}`,
and the walk is: leave along `R`, round `P`, back along `L`, out along `a`, round `Q`, back along
`b`, out along `R`. One ring, through `v` twice, pinched there. The other non-crossing pairing,
`{R, L}` and `{a, b}`, across the sectors that are not output, gives `P`'s ring and `Q`'s ring
separately: `P` with `Q` as a hole touching it at `v`.

(This example is a vertex where each polygon passes once, and it is a bounce: `P`'s two pieces have
the same class. So section 7.4's rule pairs `P`'s two ends together and `Q`'s two together, which
is the split pairing below, directly. The cut below is still needed where the pairing is made from
the rotational order, at six or more ends.)

Both use the same kept pieces, so under the parity rule both describe the same points; only the
grouping differs. **Decided (developer, 2026-10-26, open question 11): always split**, since OGC
simple features (and so most export formats) forbid a ring that touches itself but allow a hole
that touches its exterior at a point.

**How the split is made.** After linking, walk each ring, keeping the intersection vertices it has
passed. When it reaches one a second time, the ring is `v, A, v, B`, for two paths `A` and `B` that
each return to `v`. Cut it into two rings, `v, A` and `v, B`. That is the other pairing of the
ring's four ends at `v`, made without sorting anything. Repeat until no ring passes any vertex
twice; each cut adds a ring, so it stops.

**Why the cut never makes a crossing.** Of the three pairings of the four ends, the original
crosses nothing, and exactly one of the other two crosses. If the cut were the crossing one, the
rings `v, A` and `v, B` would cross at `v` and nowhere else: they never cross elsewhere, since the
pairing everywhere else only lets rings touch. But two closed curves on a sphere cross an even
number of times (Jordan), and touches do not count. So the cut is the other non-crossing pairing,
the one across the sectors that are not output. This is FHP's split-candidate clean-up (their
Fig. 14), found without its labels.

Pinches inherited from the input (a ring of `P` that touches itself at one of its own vertices) are
not intersection vertices, and the cut does not see them. Splitting a polygon into simple rings in
general is the overlay of a polygon with itself (requirements section 3.7).

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
corner; here they have collapsed onto `x`. Neither can be dropped: both stand for real crossings,
and run 1, dropping the first-sorted one, got `Q`'s ring as one piece with one class, consistent and
wrong in 15 of 25 cases. (Dropping the other one passed every case; run 1 did not establish why, so
that is not evidence that it is safe.) And they are not two places.

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
can be as wide as the chain: two boundaries digitised along each other, with the two polygons'
vertices interleaved `0.45h` apart, make one vertex `28.8h` wide for 66 records, output at one point (run 2, `probe_group_growth.py`).
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
encode the snapped topology, and the rays describe the unsnapped one. With a piece inside the
tolerance band at the vertex (under about `2h`) the two can disagree, and every rotational-order
failure whose mechanism run 1 traced was of that kind.

**The rule.** `P` crosses `Q` at the vertex iff `p1` and `p2` have different classes; `Q` crosses
`P` iff `q1` and `q2` do; and the two must agree. So:

- three known classes fix the fourth (`q2 = not q1` if `P`'s differ, `q2 = q1` if not);
- four known classes that disagree are a check failure (section 9);
- **at a vertex that is one `LL` record the crossing is known from the record**, since each
  segment crossed the other's thick plane: each polygon's two classes must differ, so one known
  class fixes its own polygon's other class, and "both the same" for either polygon is a check
  failure. (It says nothing about the other polygon: knowing which of `P`'s pieces is inside `Q`
  does not say which of `Q`'s is inside `P`.) A touch record can be either, so only the first two
  rules apply there.

**Pairing** (section 6) at a crossing: the sector `(in P, in Q) = (a, b)` is bounded by the `P`
piece whose class is `b` and the `Q` piece whose class is `a`, so each output sector names its two
rays; at a bounce, `P`'s two rays pair and `Q`'s two pair: the split pairing of 6.4, so the two
rings touch at the vertex and the cut there is never needed.

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
(the midpoint is far from `P`), but the design wants midpoint tests rare, and without the rule 218
midpoint tests were made in 300 random cases (run 2's `near` generator, V1b). The record at `u`
already says that `P` and `Q` cross there, so each polygon's two classes at `u` must differ, and a
known class of `Q`'s piece before `u` fixes `u`-`w` on its own, whatever is known of `P`'s pieces.
Deduction then runs along each ring through its `LL` vertices from a single tested piece of that
ring, and the 218 tests fell to 1.

*It can accept a contradiction.* At a touch, "`P`'s two classes are the same, and so are `Q`'s"
is a bounce, which a touch can be. At an `LL` record it is impossible, since the segments cross.
Run 2's `rays#81`: `P`'s edge runs within `h` of a `Q` edge for thousands of `h`, and a hole of `P`
crosses that `Q` edge (an `LL` record), splitting it, so the two run-together pieces are not a
common pair. Both are band pieces (they lie within `h` of the other boundary all along), both were
midpoint-tested (under run 2's longest-first order) with arbitrary answers, and the answers
happened to make the `LL` vertex a bounce
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
margin, the long piece is tested first (its midpoint is about `4000h` from `Q` in run 2's
`six#135`) and the band piece is deduced from it; with the `LL` rule as well, both are deduced
from vertex-tested pieces and no midpoint test is made. The principle: a class decided inside the band must never be the source of a
deduction for a piece that has a margin. Length is a proxy for margin; margin is the rule.

Under this order, negating every midpoint test whose arc lies wholly within `2h` of the other
boundary (run 2's standing adversary) changed only slivers: regions whose points lie within
`0.5h` of the nearer boundary, of at most a few thousand `h^2` (a band piece thousands of `h`
long and under `h` wide), in 1500 random cases of the five main generators, with no oracle or
check failure. (Dense shared edges under the adversary are the residue of 7.6 and 9.3.)

### 7.6 Six or more ends: the rotational order, and what is left of it

A vertex with six or more ends (the wedge, a neck, three rings of `Q` within `h` of one point,
`P`'s exterior and hole touching on `Q`'s edge) is resolved by the sector walk of section 4.3, in
the exact rotational order of its rays: the classes alone do not say which of `Q`'s two passes `P`
threads between. Three things keep this safe:

- Each ray is the exact tangent of its segment's plane at the position where the piece ends (its
  original vertex or its record), ordered about the vertex's output position with exact
  orientation signs, never by a difference of two nearby positions. At a vertex of one record, or
  of records at one point, every ray leaves from that point; at a chained identity group the rays
  leave from positions up to the group's extent apart, which is the case the bundle rule below is
  for. In doubles the order agreed with
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
`2h`, merging its ends into one cluster. Run 1 and run 2 measured it (variant V2, and V2b, which
also makes any arc with both ends in one cluster internal; V2 alone failed 5 catalogue cases):

- It fails. `P`'s edge running down a neck of `Q` with the neck's two vertices on it (7.5's
  picture) is two four-end vertices `0.4h` to `1.3h` apart under identity, each resolved without
  geometry; contraction merges them into one six-end cluster whose rays leave from two different
  points, nearly parallel, and the rotational walk round it is inconsistent: 19 of 300 cases, every
  rule and adversary, in exact arithmetic (run 2, section 6). Contraction makes it worse in two
  ways. It *creates* six-end vertices: identity keeps this configuration as two four-end vertices,
  which need no geometry at all (7.4), and contraction joins them into one that needs the
  rotational order. And it *spoils* that order: a cluster's rays leave from different member
  positions, up to the cluster's width apart, so their order round the cluster is not the order at
  any one point and can be wrong even in exact arithmetic. An identity vertex has the same
  weakness when it is a chained group, whose rays also leave from several positions, and when
  several rays leave along one thick plane the order is noise there too; such vertices need 7.6's
  bundle rule. But that took dense coincident boundaries (4 of 299 cases), where contraction's failure
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
This is FHP's one failure (their Fig. 21b to d; their section 6 says the algorithm "may fail" there
and gives no reason; requirements section 1.3 gives our reading: phase 1's one-to-one links cannot
hold a point that is one vertex of `Q` and two of `P`).

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
`(1, 1)` sectors, the lower parts of the lobes: two triangles touching at `X`, the correct result
for FHP's Fig. 21b (the figure draws a result only for 21a). `P \ Q` is the two `(1, 0)` sectors, the upper parts.

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
oracle passed both. The ring through `X` twice passes there inside two different arcs (`A`-`E` and
`C`-`F`), or, at the exact tie, at two different vertices `E` and `F`, so section 6.4's split (which looks for a repeated vertex) does not see it: it is the input's
twist carried through (section 8), not something the overlay made.

**Ties in general.** Records with equal keys along one segment sort in no defined order. Under
identity vertices a tie between records of one vertex is not a tie at all: the records are one
stop on the ring (7.2). A tie between
different vertices, as here, gives two orders of a zero-length piece whose class is deduced, and
either order is a valid output. Run 1 and run 2 ran every tie order, and the output's point set
never depended on it.

### 7.9 Clean-up, last

After linking: consecutive repeated positions are removed and counted; a ring with fewer than three
distinct positions is dropped and counted; nothing else is merged or dropped. The dropped rings are
the collapsed regions of 7.2, zero area by construction. Whether to also merge regions all of whose
points lie within about `h` of both boundaries (slivers of the true answer, which contraction used
to remove by accident) is a separate, distance-based rule on the output, for the design (Phase 4b).

## 8. A twist away from `Q`

`GeometryIntersect` intersects `P` with `Q`, never with itself, so a point `X` where `P` crosses
itself is not a record, and not a vertex of the overlay, even when it lies on `Q` (section 7.8). It lies
inside one piece (if both crossing edges are in the same piece) or inside two. Each of those
pieces is still on one side of `Q`, and its test, at an original vertex, is unaffected. Its class
is also right: point-in-polygon is parity from `R`, so a twisted lobe of `P` is inside `P`
whichever way it is wound, as the interior rule says (design doc section 3.1).

Example: `P` is the bow-tie of section 7.8, with `Q`'s top edge lower down, so that `X` is above it
and the edge cuts both lobes:

```
  A               C
  | \           / |
  |   \       /   |
  |       X       |
  |   /       \   |
  Y_L-F-------E-Y_R    Q's top edge (Q below)
  | /           \ |
  B               D
```

The records are the four crossings of `Q`'s edge: by `B`-`A` at `Y_L`, by `C`-`B` at `F`, by `A`-`D`
at `E` and by `D`-`C` at `Y_R`. `X` is not among them. `P ∩ Q` is the two small triangles
`Y_L, F, B` and `E, Y_R, D`; the part of `Q`'s edge from `F` to `E` is outside `P`, in the gap
between the lobes below `X`. `P \ Q` is the two lobes above the edge, and they come out as one
ring, `Y_L, A, E, Y_R, C, F`: `P`'s piece from `Y_L` up to `A` and down the diagonal to `E`, `Q`'s
edge from `E` to `Y_R`, `P`'s piece up to `C` and down the other diagonal to `F`, and `Q`'s edge
back to `Y_L`. Its arcs `A`-`E` and `C`-`F` cross at `X`: the output keeps the twist, exactly as the
input had it.

**Why the output is still right.** Counted mod 2, the boundary of `P ∩ Q` is the part of `P`'s
boundary inside `Q` plus the part of `Q`'s boundary inside `P` (with the common chains the sector
rule assigns), whether or not any ring is simple; those are exactly the kept pieces. The same holds
for `P \ Q` and the other operations.

Which side of those rings is the output is a separate question, and on the sphere it has two
answers: the same rings bound a region and also its complement, the rest of the sphere. (In the
plane there is no choice, because the unbounded side is always outside.) The interior rule chooses
ring by ring, reading each ring's interior as its smaller side, and combines the rings by parity.
So the output is read as intended when every output ring encloses less than a hemisphere, which
holds when `P`'s exterior ring does, since every ring of `P ∩ Q` or `P \ Q` lies inside it
(requirements section 1.6). The condition is on each ring, not on the region's area: a small
region whose exterior ring encloses more than a hemisphere, with a large hole, would be read
wrongly.

**What it means for the output.** The twist is not an intersection vertex, so the split of
section 6.4 (open question 11) does not see it: the output crosses itself where the input did.
Partitioning does not make the input worse, but it does not make it simple either; a simple output
needs the self-overlay of requirements section 3.7 (filed in the inbox as a possible PR after B).

**A twist near `Q`.** If `X` is close to `Q`'s edge, `Q`'s edge is crossed twice close together,
and the piece of `Q` between the two crossings is short. However short, the two crossings stay two
`LL` vertices, each an ordinary crossing, and the short piece is deduced from either of them
(sections 7.4 and 7.8); on `Q` exactly, it has zero length and the same is true.

FHP handle a twist away from `Q` too (their Fig. 21a); what they cannot represent is a twist on `Q`.

## 9. Checks

### 9.1 What is automatic and what is checked

Two properties hold by construction, so they are assertions on the overlay's own code, not checks
on the data:

- At every vertex each polygon has an even number of ends (section 6.2), so the walk round the
  vertex, flipping memberships, comes back to where it started.
- Every kept piece has two ends and each is paired, so every walk along the kept pieces closes.

What the data can get wrong is the classes, and two checks test them:

1. **At a vertex, the memberships fixed by different ends must agree.** Each non-common end fixes
   the other polygon's membership in the two sectors beside it, and the walk needs only one sector
   to start from. At an ordinary crossing, with ends `p1`, `p2`, `q1` and `q2`, `p1` and `p2` each
   fix `Q`'s membership and `q1` and `q2` each fix `P`'s, so two of the four are redundant: `p1`
   and `p2` must have different classes (`P` crosses `Q` there), and so must `q1` and `q2`. At a
   bounce they must have the same class. (The rotational order, `p p q q`, would show it too, but
   at four ends it is not used: section 7.4.) At a vertex that is one `LL` record a bounce is
   itself a failure, since the record says the segments cross (7.4).
   At a chain end (`p`, `q`, `c`) nothing is redundant: `p` and `q` fix exactly what is needed.
2. **A common chain must get the same class from both its ends.** This is where a chain's
   redundancy is.

A third check is worth keeping in debug builds: **test every original vertex of a piece, not just
one, and require them to agree.** Phase 4a run 1 dropped one of the wedge's two records on purpose:
`Q`'s ring then came out as one piece with one class, consistent at every vertex and wrong; the
checks saw nothing, and this diagnostic flagged the piece in three of the four runs examined. It costs a test per vertex, so it is not for release builds.

### 9.2 What a failure means

Each test is exact for its own point, so a disagreement means the classes do not describe one
consistent arrangement. The causes:

- a missed or spurious intersection (R9, R11): a piece that really crosses the other polygon has no
  single class;
- a single-arc piece whose midpoint test was decided inside the band, and propagated (section
  7.5 orders the tests to prevent it);
- a wrong rotational order at a vertex with six or more ends, where several rays leave along one
  thick plane (section 7.6, the bundle rule still to be designed).

The failure names the vertex and the pieces, so it is local and can be debugged. FHP's equivalent
error, one wrong label, inverts the rest of the ring and shows nowhere.

### 9.3 Repair (open question 5)

Proposed:

- **Trust the tests made at original vertices** (margin at least the thickness, section 3). When a
  single-arc piece's midpoint test disagrees with them, give it the class its end vertex deduces
  from them.
- **If trusted tests disagree,** an intersection was missed or invented. Assert in debug builds. In
  release builds flip the class of the least reliable piece involved (a single arc before a longer
  piece, a shorter piece before a longer one), recheck the vertex at its other end, and carry on.
  Never fail the call: in the cascade every plate must get an answer.
- **Count the repairs.** The Phase 5 hunt requires zero on its generated data, outside the cases
  built to provoke them.

What Phase 4a measured: under the chosen configuration (section 7) no repair was needed in any
normal run. Repair is a safety net, not a mechanism: where the vertex rule itself was wrong (the
rotational order at four ends, contraction's clusters, run 1 and run 2), single flips never made
the output right, and under the midpoint adversary in dense shared edges the repair ran into its
round limit. A check failure that repair cannot clear should be reported in debug builds and left
as the best consistent answer in release builds.

## 10. Rings with no intersection

A ring of either polygon with no record is one closed piece (section 2), classified by one of its
vertices against the other polygon. No vertex of such a ring is within the thickness of the other
polygon (or it would be a record), so the test has the full margin. It is then kept or not like any other
piece. With every ring of both polygons taking part, this fixes three of the four bugs in PLAN.md's
Status, which all come from today's code intersecting exterior rings only and deciding a polygon
whose exterior ring meets nothing by one vertex.

**`Q` wholly inside `P`** (the first bug, where today all of `P` goes outside):

```
  +-----------------+
  |  P              |
  |     +-----+     |
  |     |  Q  |     |
  |     +-----+     |
  |                 |
  +-----------------+
```

`P`'s ring is outside `Q`, and `Q`'s ring is inside `P`. So `P ∩ Q` keeps `Q`'s ring: it is `Q`. And
`P \ Q` keeps both rings: it is `P` with `Q` as a hole.

**Holes.** Every ring of both polygons takes part, so the same rule covers the other two bugs: a
partitioning polygon that crosses only a hole of `P` (`P`'s exterior ring has no record, is one
piece, and is classified against `Q`), and an input crossing only a hole of `Q` (`Q`'s exterior ring
likewise). A hole of `Q` lying inside `P` with no contact is a piece inside `P`: it is kept in
`P ∩ Q`, as a hole of the intersection, and in `P \ Q`, where it bounds the part of `P` inside
`Q`'s hole.

## 11. Grouping the rings into polygons

After the linking and the split of section 6.4, each output is a set of rings that do not cross
each other (twists apart, section 8) and may touch at vertices. They become one or more polygons,
each an exterior ring with its holes.

**Why the grouping must be right.** Under the parity rule the point set of all the rings together
does not depend on how they are grouped. But the output is several polygons, and a user reads their
union. A hole given to the wrong exterior would lie outside that exterior, so under parity it would
become an island of that polygon, and the union would be wrong. (Requirements section 3.3 now says
the same; its first version said the grouping was only for the user, which is true of a single
polygon holding every ring, not of several.)

**Exteriors and holes need no test.** The linking knows which side of each ring the output is on,
from the sector memberships. A ring with the output on its smaller side is an exterior; one with
the output on its larger side is a hole (the hole itself is the smaller side, lying inside an
exterior).

**Which exterior a hole belongs to** is the smallest exterior that contains it: test one vertex of
the hole, not shared with that exterior, against it, threshold off, with bounding circles to skip
most candidates. An island inside a hole is an exterior of its own and makes its own polygon.

**Open question 7 (the style).** Proposed: OGC simple features. Each polygon is an exterior with
holes; no ring touches itself (section 6.4); a hole may touch its exterior at a point. The
exceptions are inherited from the input: a twist of `P` stays a twist (section 8), and a twist on
`Q` comes out as a ring through the twist point twice at two different vertices (7.8), which the
split does not see.

**Open question 6 (orientation).** Since the output side of every ring is known, the rings can be
oriented consistently at no cost, with the output on the left: exteriors anticlockwise seen from
outside the sphere, holes clockwise. GeoJSON asks for that and Shapefile for the reverse; a fixed
convention serves either. The alternative, following `P`'s direction where the ring runs along
`P`, gives no consistent orientation, because GPlates never normalises its input. (The question's
first wording, keeping each piece's original direction, is not possible: a ring is walked one way,
and some of its pieces go against their original direction whatever is chosen.) The interior rule
does not care either way. Proposed: output on the left.

## 12. Polylines

When `P` is a polyline, steps 1 to 3 are the same: split at every record, and classify each piece
by its test (section 3). There are no sides, so a piece lying along `Q`'s boundary cannot use the
sector rule; it gets the class of open question 2 (proposed: inside, matching the inclusive
threshold, so that in the cascade a polyline along a boundary shared by two plates goes to the
first of them).

There is no linking. Consecutive pieces of one class are joined back into one polyline, so that a
touch (a bounce) does not break a polyline in two. An end point of the polyline lying on `Q` has
just one end of `P`, which needs no pairing. There is also less to check: the pieces of `Q` bound
nothing, so they are not classified, and no end is redundant. A single-arc piece can still be
deduced across a crossing (`LL` record: the polyline's two pieces there differ, section 7.4), but
not across a touch, where only a test can say. A wrong midpoint test of a single-arc piece gives
one short piece the wrong class, with no check to catch it. (Phase 4a ran polygons only; polylines
are for the Phase 5 hunt.)

Points and multi-points are unchanged: one point-in-polygon test each.

## 13. Provenance

Every output vertex records where it came from:

- an original vertex of `P` or of `Q`: its ring and index;
- an intersection vertex: its segment and position along the segment on *both* polygons, which
  `GeometryIntersect` already gives (R1);
- an identity vertex holding several records (section 7.2): the provenance of every record in
  it.

This is what `PartitionFeatureUtils` needs for scalar coverages, replacing its epsilon map of
original points and its nearest-segment search (PLAN.md, *Context*). A value at an original vertex
of `P` is copied, and one at an intersection vertex is interpolated along `P`'s segment. Along a
cut, a piece of `Q` inside `P`, the coverage has no value at all; what to give those points is a
Phase 4 decision for the developer (interpolating along the cut between its two ends is one option).

## 14. Union and the other Boolean operations

Write an operation as `f(in P, in Q)`: intersection is *and*, union is *or*, `P \ Q` is *in `P` and
not in `Q`*. A piece is on the output's boundary exactly when `f` differs between its two sides, so
each operation keeps:

| Piece | `P ∩ Q` | `P ∪ Q` | `P \ Q` | `Q \ P` | symmetric difference |
| --- | --- | --- | --- | --- | --- |
| of `P`, inside `Q` | kept | | | kept | kept |
| of `P`, outside `Q` | | kept | kept | | kept |
| of `Q`, inside `P` | kept | | kept | | kept |
| of `Q`, outside `P` | | kept | | kept | kept |
| common, interiors on the same side | kept | kept | | | |
| common, interiors on opposite sides | | | kept | kept | |

For example, a piece of `P` inside `Q` has `(in P, in Q)` equal to `(1, 1)` on one side and `(0, 1)`
on the other; `f` differs for *and* (1 and 0) but not for *or* (1 and 1). Partitioning is the
intersection and `P \ Q`. In the two-squares example the union keeps `p2` and `q2`. The linking,
the split and the grouping are the same for every operation: they use only which sectors are in the
output.

**The hemisphere limit.** `P ∩ Q`, `P \ Q` and `Q \ P` lie inside an input, so their rings enclose
less than a hemisphere whenever the inputs' rings do (section 8). A union, or a symmetric
difference, can exceed one, and a ring cannot mean more than a hemisphere (design doc section 3.1;
the fix, an explicit side, is deferred in its section 7). The grouping detects it: the output then
has a ring with the output on its larger side and no exterior containing it. The operation fails
with an error rather than return the complement.

## 15. Not walked through

The requirements' section 3.3 ends with the case for and against the overlay, and sections 3.4 to
3.6 compare it with the alternatives. They are the reasons for a choice the developer has made
(2026-10-25), so they are not repeated here.
