# The overlay, explained

Written 2026-10-25 and 26, while walking the developer through `phase2-requirements.md`
section 3.3 one paragraph at a time. Section 3.3 is the summary, good for coming back to once the
algorithm is understood but hard to follow on a first reading. This file explains it from the
start, with examples, and records what the walk-through corrected. When partitioning is
implemented, this file becomes the basis of the design doc's section 5.3 onwards (Phase 6),
rewritten against the code, with section 3.3 as the overview. Until then it describes a design,
not code, so it stays in the plan (AGENTS.md, *Planning documents*).

Notation: `P` is the geometry being cut (the subject), `Q` the polygon cutting it (the clip), `t`
the thickness of `GeometryIntersect` (about `1e-6` radians), and `s` the point-on-outline threshold
(about `1.414e-6` radians). FHP is Foster, Hormann and Popa 2019 (requirements section 1.3).

## 1. Overview

The overlay computes `P ∩ Q` and `P \ Q` (partitioning's two outputs), or any other Boolean
operation, in five steps:

1. Intersect all the rings of both polygons with `GeometryIntersect`.
2. Split every ring of both polygons at every intersection, into *pieces*.
3. Classify each piece as inside or outside the other polygon, with one point-in-polygon test.
   Pieces where `P` and `Q` run together (*common pieces*) cannot be tested; the *sector rule*
   classifies them instead.
4. Keep the pieces that bound the output.
5. Link the kept pieces into rings, and group the rings into polygons with holes.

Each piece is classified on its own, so a wrong decision affects one piece, and the linking checks
every vertex, so a wrong decision is found where it was made. FHP labels each intersection vertex
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
no intersection record is farther than `t` from `Q`'s boundary, or `GeometryIntersect` would have
reported it as a touch. So the test has a margin of at least `t`, about `10^10` times
point-in-polygon's rounding.

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
at `x = 0`, `B` at `(b, -δ)` and `A` at `(a, h)`, `a < 0 < b`. `B` is a touch, so `|δ| <= t`; with
`δ > 0` it lies inside `Q`. The edge passes `C` at height `y_C > t`. When `A` is just left of `C`
(`a` near 0), the midpoint is at height `(h - δ)/2`, about `(y_C - δ)/2`. With `B` on the outside
(`δ <= 0`) that is more than `t/2`: safe. With `B` inside `Q` it is positive but can be arbitrarily
small, when `y_C` is just above `t` and `δ` just below it: then the edge crosses `Q`'s top edge
near its middle with no record (the touch at `B` stands for the contact), and the midpoint test is
decided by rounding. The vertex `A` is more than `t` from `Q` whatever happens, so it is the point
to test.

Had the midpoint been tested, the result would not have been silent: a wrong class for this piece
contradicts the tests of `Q`'s pieces at `B` (the general vertex, section 4.3), and the checks
find it.

**A piece with no original vertex** is a single arc between two intersection vertices, and there
the midpoint is the only point, without a guaranteed margin. If the arc's two ends lie on the same
edge of `Q`, with no vertex of either between, it is a common piece, and section 4 applies. If
not, it is classified by its midpoint, and the checks compare that class with the other pieces at
its two end vertices. Whether such pieces should be deduced from their end vertices first, and
tested only when that fails, is for Phase 4. Also for Phase 3 to confirm: that a vertex in no
record really is farther than `t` from the other geometry everywhere, corners included (the
audit's D3 is over-reporting in a corner's wedge, which does no harm here).

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

**The general vertex.** Sort the ends by angle round `v` (the tangent of each piece's first arc).
Start from a sector that a non-common piece fixes, and go round, flipping `P` at a `P` end, `Q` at
a `Q` end, and both at a common ray. Every other non-common piece fixes a membership too, and they
must agree: a disagreement is a detected error.

**Against FHP.** FHP decides a chain (delayed crossing or bouncing) from side tests at its ends:
is `P-` left or right of `Q`'s chain, and `P+` at the other end. Those are signed areas of points
within the tolerance of the chain, the fragile decisions in this data. The sector rule uses only
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
   and separately with `Q`'s, exactly as `is_point_in_polygon` does, but leaving out the arc that
   contains `X` (its `P` copy and its `Q` copy). The arc from `R` to `X` meets that arc's great
   circle only at `X`, so the points just before `X` lie on `R`'s side of it, and the two parities
   are the memberships `(in P, in Q)` on that side; the other side has both flipped. Nothing is
   offset, so no thin face can defeat it, and the crossing tests are point-in-polygon's own exact
   sign tests (audited in #106). The developer's worry, the arc from `R` arriving parallel to the
   chain at `X`, is why `X` is chosen on the arc farthest from `R`'s great circles: it fails only if
   `R` lies on the great circle of every arc of the ring, which no ring does. Cost: one test per
   polygon at the slow speed, `O(N)`, only for anchorless rings. It needs a point-in-polygon entry
   that leaves out given edges.
4. **An offset arc, intersected with both polygons**, testing the midpoint between the chain and
   the first intersection along it (also the developer's). It works, but `GeometryIntersect` cannot
   see a boundary within `t` of the arc's start, which is on the chain, so it needs care; option 3
   does the same with exact tests that already exist.

**Recommended: option 3.** Open question 3 is now only about this fallback: the sector rule is the
primary rule (developer, 2026-10-26).

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
chain's two, but three rays.) Contracting a short piece merges two vertices and adds their ends,
so the count stays even. A polyline's end point can have one end, but polylines are not linked.

The kept ends are even too. Going round a vertex, output membership flips at every kept end and
nowhere else, and it must come back to where it started.

### 6.3 Pairing

At each vertex the kept ends are paired, and a ring is a walk that arrives along one end of a pair
and leaves along the other.

- **Two kept ends (the usual case):** one pairing, no geometry. The kept pieces form cycles, and
  walking them closes the rings. No direction is needed either: GPlates never normalises ring
  orientation, so a `Q` piece may run either way relative to the `P` pieces it joins, and the walk
  just follows the pieces.
- **Four or more:** the ends are sorted by angle round the vertex, so the sectors alternate between
  output and not, and the two ends bounding each output sector are paired. The rings then touch at
  the vertex but never cross.

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

## 7. Contracting short pieces

### 7.1 The problem

Splitting at every record (section 2) assumes the records are at distinct points. When two records
are at one point, or within the tolerance of each other, the piece between them has zero or tiny
length, and two things fail. It has no test point: no original vertex, and its midpoint lies on the
other polygon. And it has no direction: the rotational order at its ends (section 4.3) needs the
tangent of its first arc, and a zero-length arc has none.

Where it happens:

- **A twist of `P` lying on `Q`'s boundary.** `P`'s ring is `A, D, C, B`: the edges `A`-`D` and
  `C`-`B` cross at `X`, so `P` is a bow-tie, and `X` lies on `Q`'s top edge, with `Q` below:

  ```
    A               C
    | \           / |
    |   \       /   |
    Y_L---- X ----Y_R      Q's top edge (Q below)
    |   /       \   |
    | /           \ |
    B               D
  ```

  `Q`'s edge is crossed twice at `X`, once by `A`-`D` and once by `C`-`B`, so there are two
  crossing records at one point. Along `Q` they have equal keys and sort in no defined order, with a
  piece of `Q` of zero length between them. This is FHP's one failure (their Fig. 21).
- **Ties along a segment.** Records at the same distance along one segment sort in no defined order
  (`phase3-audit.md`, R5, caveat 2).
- **Two touches up to `2t` apart** (audit D4). A segment of `P` shorter than `2t` that crosses an
  edge of `Q` has both its vertices within `t` of the edge, so it gives two touch records and no
  crossing. Such a pair is usually consecutive along both polygons with no original vertex between,
  so it forms a common piece and the sector rule decides it (section 4); but not always.

### 7.2 Contraction

A piece with no original vertex, shorter than the contraction tolerance, is removed, and its two end
vertices become one. Several short pieces in a row become one vertex too (each merge joins two
clusters). The merged vertex's ends are the sum of its parts' ends, so each polygon still has an
even number there (section 6.2).

At the twist, the merged `X` has six ends: `P`'s four, towards `A`, `B`, `C` and `D`, and `Q`'s two,
along its edge towards `Y_L` and `Y_R`. Going anticlockwise round `X` from `Q`'s edge on the right
(as drawn), with each sector's (in `P`, in `Q`):

| Sector | Where | (in `P`, in `Q`) |
| --- | --- | --- |
| `Y_R` to `C` | upper part of the right lobe | (1, 0) |
| `C` to `A` | above `X`, between the lobes | (0, 0) |
| `A` to `Y_L` | upper part of the left lobe | (1, 0) |
| `Y_L` to `B` | lower part of the left lobe | (1, 1) |
| `B` to `D` | below `X`, between the lobes | (0, 1) |
| `D` to `Y_R` | lower part of the right lobe | (1, 1) |

Each ray flips one membership (`P`'s rays flip `P`, `Q`'s flip `Q`), as it must. For `P ∩ Q` the
output sectors are the two `(1, 1)` ones, so the kept ends are `Y_L`, `B`, `D` and `Y_R`, and pairing
across the output sectors gives `{Y_L, B}` and `{D, Y_R}`: two triangles, `X, Y_L, B` and
`X, Y_R, D`, touching at `X`. That is the result FHP's Fig. 21 expects: `P`'s lobes, which crossed
at `X` in the input, touch there in the output. `P \ Q` is the two upper triangles, likewise.

Without contraction there are two vertices at `X`, with the zero-length piece of `Q` between them. Its
class is a coin toss, and at each of the two vertices one ray has no direction, so the sectors are
undefined: the walk round the vertex fails, or the pairing lets the lobes cross.

### 7.3 The choices

- **The tolerance is at least `2t`** (`2s` if the thickness is raised to `s`, open question 1),
  because of the touches up to `2t` apart. Records with equal keys on one segment are within `2t` of
  each other, so contracting by distance also settles ties.
- **Only pieces with no original vertex.** A piece with one has a reliable test (section 3), and
  contracting it would drop an original vertex of the input.
- **The merged vertex's position**, proposed: an original vertex of `P` if one was merged, so that
  `P`'s vertices stay exact (scalar coverages want this); else a crossing, whose position is
  accurate to about `2e-10` radians (audit D5); else any. The arcs ending there move by up to the
  tolerance. Its provenance is that of every merged record: it lies on both polygons.
- **Fixed at the source, not by contraction:** the sort order of touch records (audit F2, for D1),
  and a vertex in a corner's wedge being reported once per edge (audit F3, for D3). The latter's
  piece of `Q` runs through the corner vertex, so it has an original vertex, and at a sharp corner it
  is longer than any fixed tolerance.
- **The cost:** a chain of short pieces contracts to one vertex that can be wider than the
  tolerance, and the point set then moves by up to that width. The Phase 5 hunt should measure how
  wide such clusters get in the cascade. Without contraction the checks would still catch a short
  piece's wrong class, but only as an inconsistency to repair.

## 8. Still to walk through

Section 3.3's remaining paragraphs: a twist away from `Q`, the checks,
rings with no intersection and the grouping by containment, polylines, provenance, and the other
Boolean operations; then the open questions of the requirements' section 4.
