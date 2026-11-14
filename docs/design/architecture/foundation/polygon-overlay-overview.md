# The polygon overlay: an overview

How GPlates and pyGPlates cut one polygon by another, explained for a first reading. This page
summarises [section 5 of `spherical-geometry.md`](spherical-geometry.md#5-partitioning), which is
the reference: complete, exact, and mostly about what happens when several intersections fall at or
near one point. Here is only the generic case, two polygons that cross cleanly or share whole
edges, with a worked example for each idea. Where this page and the reference differ, the reference
and the code (`src/maths/PolygonOverlay.{h,cc}`) govern.

`P` is the geometry being cut (the *subject*), `Q` the polygon cutting it (the *clip*). Both may
have holes. Every edge is a great circle arc; the pictures are flat because the examples are small.

## 1. Five steps

The overlay answers every question of the form "which part of `P` and `Q` satisfies ...": the part
of `P` inside `Q` and the part outside it (partitioning), their union, either difference, the
symmetric difference. It does so in five steps:

1. **Intersect.** Find every point where a ring of `P` meets a ring of `Q`
   (`GeometryIntersect`, [section 4.2](spherical-geometry.md#42-geometryintersect)): crossings, a
   vertex of one on an edge of the other, and coincident vertices. Each is a *record*.
2. **Cut into pieces.** Cut every ring of both polygons at its records. A *piece* is the stretch
   of one ring between two consecutive cuts.
3. **Classify.** Label each piece of `P` as inside or outside `Q`, and each piece of `Q` as inside
   or outside `P`.
4. **Keep.** For the operation wanted, keep the pieces that lie on the boundary of the answer.
5. **Link and group.** Join the kept pieces end to end into rings, and sort the rings into polygons
   with holes.

In the code, constructing a `PolygonOverlay::Overlay` does steps 1 and 2, `classify()` does step 3,
and `extract(operation)` does steps 4 and 5. Steps 1 to 3 are done once for a pair of polygons;
every operation is then steps 4 and 5 on the same labels, so partitioning's inside and outside
always agree with each other.

The classical alternative (Greiner-Hormann clipping) labels the *intersections* as entries and
exits and traces the output by alternating them, so one wrong label inverts the rest of a ring,
silently. Here the labels are on the pieces: each is decided on its own, a wrong one affects one
piece, and the labels that meet at a vertex check each other
([section 5.7](spherical-geometry.md#57-rejected-alternatives) has the comparison).

## 2. Pieces

`P` is the lower-left square and `Q` the upper-right one. They cross at `a` and `b`:

```
            +-----------+ Q
            |           |
  P +-------a---+       |
    |       |   |       |
    |       +---b-------+
    |           |
    +-----------+
```

There are two records, both crossings, and so two *vertices* of the overlay, `a` and `b`. Each ring
is cut at both:

| Piece | Runs | |
| --- | --- | --- |
| `p1` | from `a` round `P`'s top-right corner to `b` | inside `Q` |
| `p2` | from `b` the long way round `P` back to `a` | outside `Q` |
| `q1` | from `a` round `Q`'s bottom-left corner to `b` | inside `P` |
| `q2` | from `b` the long way round `Q` back to `a` | outside `P` |

A piece has two *ends*, one at each of its vertices; four ends meet at `a` and four at `b`. A ring
that meets the other polygon nowhere is not cut at all: it is one *closed piece*, the whole ring.

The vertices are the only places where the two boundaries meet, so a piece never crosses the other
polygon's boundary part-way along. That is what makes step 3 possible: a piece is on one side of
the other polygon, all of it.

## 3. Classifying a piece

**Test one of its own vertices.** If a piece passes through an original vertex of its polygon (a
corner that is not itself a record), that corner is tested with point-in-polygon against the other
polygon, and the answer is the class of the whole piece. All four pieces above are classified this
way: `p1` by `P`'s top-right corner, `q1` by `Q`'s bottom-left corner, and so on. Such a corner is
never close to the other polygon's outline (if it were within the tolerance it would be a record),
so the test is never a close call. The test is point-in-polygon's exact one, without its usual
allowance that a point within 9 metres of the outline counts as inside
([the point-in-polygon overview](point-in-polygon-overview.md), section 4): the pieces have to
divide the sphere exactly.

**Otherwise deduce it from its neighbours.** A piece with no corner of its own is a single arc
between two records, and has no safe point to test. Here an edge of `Q` is crossed by two edges of
`P`, at `u` and `w`:

```
            |           |
   Q -------u-----------w------- Q
            |           |
            P           P
```

`Q`'s piece from `u` to `w` has no corner. But at a crossing each polygon passes from one side of
the other to the other side, so each polygon's two pieces there have *different* classes. Once the
piece of `Q` to the left of `u` is known, the piece from `u` to `w` is its opposite, and the piece
beyond `w` the opposite again. A vertex where the polygons only touch gives a weaker rule (`P`
crosses there exactly when `Q` does, so three known classes give the fourth). `classify()` applies
these rules at every vertex until nothing more follows. A piece still unknown after that is tested
at its midpoint, the one farthest from the other outline first.

**Then check.** The same rules, read the other way, are the overlay's check on itself: at every
vertex the classes that meet must describe one possible arrangement (at a crossing, for instance,
both of `P`'s pieces inside `Q` is impossible). A vertex that fails is repaired, by flipping the
class least to be trusted there, if that makes it consistent; otherwise the overlay raises an error
rather than return an answer it knows to be inconsistent
([section 5.6](spherical-geometry.md#56-failure)).

## 4. Common pieces and the sector rule

Plates share boundaries, so the commonest case in practice is two polygons running along the same
edge. Here `Q` sits to the right of `P` and shares part of `P`'s right edge, from `u` to `v`:

```
        +-------+
        |   Q   |
  +-----u       |
  |  P  |c      |
  |     v-------+
  +-----+
```

`u` is a corner of `P` lying on `Q`'s left edge, and `v` a corner of `Q` lying on `P`'s right edge.
Between them each polygon has one piece, and the two lie on top of each other. They are paired as
one *common piece*, `c`. No point of `c` can be tested against the other polygon, since every point
of it is *on* the other polygon. What needs deciding is something else: whether the two interiors
are on the **same side** of `c` or on **opposite sides**.

The answer comes from the end of `c`, without any geometry. Three things leave `u`: the common
piece `c`, `P`'s own piece `p` (its top edge, going left) and `Q`'s own piece `q` (its left edge,
going up). They divide the neighbourhood of `u` into three sectors, and each sector is inside or
outside each polygon. Write `a` for "`q` is inside `P`" and `b` for "`p` is inside `Q`", both
ordinary classes from section 3.

- The sector between `p` and `q` borders both, so there (in `P`, in `Q`) is `(a, b)`.
- Crossing `p` changes only "in `P`": the sector between `p` and `c` is `(not a, b)`.
- Crossing `q` changes only "in `Q`": the sector between `q` and `c` is `(a, not b)`.

Those last two are the two sides of `c`. If `a` and `b` are equal they read `(1, 0)` and `(0, 1)`,
or the reverse: one polygon on each side. If `a` and `b` differ they read `(1, 1)` and `(0, 0)`:
both polygons on one side. So:

> **The interiors are on the same side of a common piece exactly when `a` and `b` differ.**

In the picture `p` is outside `Q` and `q` is outside `P`, so `a = b = 0`: opposite sides, as
neighbouring plates are. Now put `Q` inside `P`, against its right edge:

```
  +-----------+
  |     P     |
  |     +-----u
  |     |  Q  |c
  |     +-----v
  |           |
  +-----------+
```

At `u`, `p` is `P`'s right edge going up, outside `Q` (`b = 0`), and `q` is `Q`'s top edge going
left, inside `P` (`a = 1`). They differ: same side. Both interiors are to the left of `c`.

Three things about this rule matter. It uses only the classes of `p` and `q`, which are tested well
away from the shared edge. It does not depend on which way round either ring is drawn (the code it
replaced decided such a piece by which way round the partitioning polygon ran). And either end of
a shared stretch decides it, so the other end is a free check.

## 5. The keep table

Every Boolean operation is a rule `f(in P, in Q)` for whether a point belongs to the answer:
*and* for the intersection, *or* for the union, *`P` and not `Q`* for the difference. **A piece is
on the boundary of the answer exactly when `f` differs between its two sides.** Take a piece of `P`
that is inside `Q`: on one side of it a point is in both polygons, on the other only in `Q`. For
the intersection `f` gives 1 and 0, so the piece is kept; for the union it gives 1 and 1, so it is
not. Working through every kind of piece gives the whole table:

| Piece | intersection | union | difference `P - Q` | difference `Q - P` | symmetric difference |
| --- | --- | --- | --- | --- | --- |
| of `P`, inside `Q` | kept | | | kept | kept |
| of `P`, outside `Q` | | kept | kept | | kept |
| of `Q`, inside `P` | kept | | kept | | kept |
| of `Q`, outside `P` | | kept | | kept | kept |
| common, interiors on the same side | kept | kept | | | |
| common, interiors on opposite sides | | | kept | kept | |

Read against the examples:

- **The two squares.** The intersection keeps `p1` and `q1` (the small rectangle), the difference
  `P - Q` keeps `p2` and `q1` (an L shape of six points), the union keeps `p2` and `q2` (the
  outline of both, eight points).
- **The neighbouring plates.** `p` and `q` are both "outside", and `c` is "opposite sides". The
  intersection keeps nothing, so `P` is wholly outside `Q`: a polygon that only runs along another
  is not cut by it. The union keeps `p` and `q` and drops `c`: one polygon, merged across the shared
  edge. The difference `P - Q` keeps `p` and `c`, which is `P` itself.
- **`Q` inside `P`, against its edge.** `p` is outside, `q` inside, `c` "same side". The
  intersection keeps `q` and `c` (that is `Q`), the difference keeps `p` and `q` (`P` with a notch
  cut out of its edge), the union keeps `p` and `c` (that is `P`).

Partitioning is the intersection and the difference `P - Q` together. A piece of `Q` inside `P` is
kept by both: it is the cut, and each side of the cut gets a copy.

The symmetric difference is the one operation not built straight from its column. Its rings are
those of the two differences, joined wherever they share a common piece (which is in both columns
and not its own): for the neighbouring plates, `P` and `Q` joined along `c` into one outline
([section 5.2](spherical-geometry.md#52-the-overlay), *The symmetric difference*, says why its own
column was not safe to walk).

## 6. Pairing, the walk and the split

The kept pieces have to be joined into rings. At each vertex the kept ends are *paired*, and a ring
is a walk that arrives at a vertex along one end of a pair and leaves along the other.

Around any vertex the answer's region and its complement alternate, and a kept end lies between
each of them, so the kept ends at a vertex come in even numbers. Nearly always there are two, and
they pair with each other. At `a` in the two squares, the intersection keeps one end of `p1` and
one of `q1`; the walk goes along `p1` to `b`, where it finds `q1`, and along `q1` back to `a`. No
directions are needed: the rings of `P` and `Q` may be drawn either way round, and the walk follows
the pieces whichever way they run.

Four kept ends meet where the answer touches itself at a point. `Q` is a triangle whose apex `v`
lies on `P`'s top edge, and whose sides cross `P`'s bottom edge at `c` and `d`:

```
  +-------v-------+
  |      / \      |
  |  A  /   \  B  |
  |    /  Q  \    |
  +---c-------d---+   P
     /         \
    +-----------+
```

Take the difference `P - Q`, the regions `A` and `B`. It keeps `P`'s two pieces outside `Q` (from
`v` round the left of `P` to `c`, and from `d` round the right to `v`) and `Q`'s two pieces inside
`P` (its sides, `v` to `c` and `d` to `v`). At `c` and at `d` two kept ends meet and pair. At `v`
all four meet. `P` does not cross `Q` there (its pieces on either side of `v` are both outside
`Q`), so `P`'s two ends are paired with each other, and `Q`'s two with each other. The walk is
then:

```
  v -> round the left of P -> c -> up Q's side -> v -> down Q's other side -> d
    -> round the right of P -> back to v
```

One ring, through `v` twice. **The split** cuts any such ring at the repeated vertex: `v, X, v, Y`
becomes the two rings `v, X` and `v, Y`, here the outline of `A` and the outline of `B`. The cut
never makes two rings that cross: of the three ways to pair four ends, two do not cross, the walk
used one and the cut produces the other. The result follows the OGC simple features convention
that most formats share: a ring may not touch itself, though rings may touch each other at a
point.

The other operations on the same picture: the intersection keeps `P`'s piece from `c` to `d` and
`Q`'s two sides, the triangle `v, c, d`; the union keeps `P`'s two outer pieces and `Q`'s base, one
ring of nine points that passes `v` once.

## 7. Grouping the rings into polygons

The rings of one answer do not cross each other; they nest. A ring's *depth* is the number of other
rings it lies inside, and the depth's parity gives its role:

```
  +-------------------------------+
  |  ring 1          depth 0      |
  |   +-----------------------+   |
  |   |  ring 2      depth 1  |   |
  |   |   +---------------+   |   |
  |   |   |  ring 3       |   |   |
  |   |   |      depth 2  |   |   |
  |   |   +---------------+   |   |
  |   +-----------------------+   |
  +-------------------------------+
```

An even depth starts a polygon: ring 1 is an exterior. An odd depth is a hole in the ring just
outside it: ring 2 is a hole of ring 1's polygon. Ring 3, at depth 2, is an island inside the hole,
and is the exterior of a second polygon. (The union of a polygon with a hole and a small polygon
inside that hole comes out exactly like this.) Whether one ring lies inside another is asked with
one point-in-polygon test, of a point of the first ring that is clear of the second.

Each exterior is then turned to run anticlockwise seen from outside the globe, and each hole
clockwise, so that the answer is always on a ring's left, whatever directions the inputs had.

One last check guards the whole answer. On a sphere a ring has two sides, and GPlates reads a ring
as enclosing its *smaller* side (the point-in-polygon overview, section 1). A union can be larger
than a hemisphere, and its outline would then be read as the complement. So one point, well clear
of both inputs, is tested: the rings must say it is in the answer exactly when `f` says so. If they
do not, the overlay raises an error instead of returning the complement.

## 8. How partitioning and pyGPlates use it

Three layers sit on the overlay ([section 5.1](spherical-geometry.md#51-the-layers)):

- **`PolygonPartitioner`** cuts one geometry by one polygon. For a polygon it builds one overlay,
  classifies, and extracts the intersection (the inside pieces) and the difference (the outside
  pieces). If either would be empty the geometry is not cut: it is returned whole, as itself, with
  the answer "inside" or "outside". So a polygon that only touches the partitioning polygon, or
  runs along its edge, or equals it, is never split. Before any of this, two polygons whose
  bounding circles are apart (and whose interiors lie within those circles, as almost every
  polygon's does) are answered "outside" without an overlay, which is how most calls end.
- **`GeometryCookieCutter`** holds one partitioner per plate and cuts a geometry by them in turn:
  what lies inside the first plate is that plate's, what lies outside goes on to the second, and so
  on. The pieces handed on have edges along the boundary the two plates share, so the common pieces
  of section 4 are this cascade's everyday case. GPlates' Assign Plate IDs and pyGPlates'
  `PlatePartitioner`, `partition_features` and `partition_into_plates` all go through it.
- **pyGPlates' `PolygonOnSphere`** has `partition()` (one partitioner, for any geometry) and the
  four set operations. `p.intersection(q)` and `p.difference(q)` are exactly what `q.partition(p)`
  puts inside and outside. `p.union(q)` and `p.symmetric_difference(q)` call the overlay directly.
  All four return a list of polygons, empty when the answer has no area.

A polyline is cut by the same machinery as far as step 3: its pieces are classified, and
consecutive pieces of one class are joined back into polylines. There is nothing to link. A piece
of a polyline that runs along the polygon's outline counts as inside. Points and multi-points do
not use the overlay at all: each point is one point-in-polygon test.

When the overlay cannot make its classes consistent it throws `InconsistentOverlayException`, which
pyGPlates raises as `PartitionError`; Assign Plate IDs leaves that feature as it was and reports it
at the end. The same error is the answer to a union or symmetric difference too large for its
outline to be read correctly (section 7).

## 9. What this page leaves out

Everything above assumes that the records are at distinct points, well apart. Real plate
boundaries are digitised along each other with vertices metres apart, and most of the overlay, and
most of the reference, is about that. In the order the reference takes them, all in
[section 5.2](spherical-geometry.md#52-the-overlay) unless another is named:

- **The thickness band.** `GeometryIntersect` treats anything within `h`, about 9 metres, of an
  edge as on it, so that boundaries meant to coincide do. A piece that lies within the band along
  the other outline has no meaningful side; the rules are arranged so that such a piece's class
  never decides the class of a piece outside the band
  ([section 5.3](spherical-geometry.md#53-tolerances-in-the-overlay)).
- **Vertices by record identity.** Several records can sit at one point (a vertex in the corner of
  the other polygon is reported on both of the corner's edges). Records are made one vertex when
  they name the same original vertex, never by measuring a distance, so the overlay adds no
  tolerance of its own (*Records and vertices*). One case identity misses: where a polygon touches
  *itself* (a hole's tip on its own exterior) at a point inside an edge of the other, the records
  there name different vertices. They are at one position, and are made one vertex for that
  (*Vertices at one point*).
- **The loop and spike rules** remove a piece that starts and ends at one vertex, or runs to a
  vertex and straight back, which bound nothing.
- **Self-touch cuts** add the cut that is missing where a polygon touches *itself* on a shared
  edge, which the intersection of `P` with `Q` never reports. The other polygon is cut there, so
  its pieces show where: a chain of them from one end of the uncut piece to the other, or one each
  way along it from a vertex of the touching polygon (*a passage*).
- **The rotational order, the one-passage rule and bundles.** Where a polygon passes twice through
  one vertex, the order of the pieces round the vertex matters, and near a shared edge that order
  can be noise. Where one polygon passes once its classes need no order (*the one-passage rule*),
  though the linking still needs one: it is built from the classes, the way round nearer the
  measured order; where both pass twice, the pieces whose direction is uncertain are tried in every
  order their uncertainty allows (*the bundle rule*).
- **Chains with no end.** Two rings that coincide all the way round (a polygon partitioned by
  itself) have common pieces and nothing else, so section 4's rule has no end to start from; one
  arc is classified by counting crossings from each polygon's outside point (*Classification*,
  step 4).
- **Repair.** A vertex that fails its check is repaired by flipping the least reliable class
  there; what repair cannot fix is the error of section 3 (*Checks and repair*).
- **The cut at a chain** finds a ring that crosses itself between the two ends of a common piece
  the operation does not keep, which no vertex can see, and cuts it there (*A ring twisted at a
  chain*).
- **Spike walls.** The symmetric difference joins the two differences across each common piece
  between them, except one along which a ring doubles straight back, which separates nothing (*The
  symmetric difference*).
- **Crossing rings.** If an input's own rings cross each other (a self-intersecting polygon), the
  output rings can cross too; such rings are put into one polygon, which reads correctly by
  point-in-polygon's parity rule (*Grouping*).
- **The sliver filter** optionally drops output rings a few metres wide, where an outline was
  digitised along a plate boundary. Its measure of a ring's width is its own, because the usual
  area calculation loses the short edges a cut makes
  ([section 5.5](spherical-geometry.md#55-the-sliver-filter)).
- **Where each output point came from** (its vertex or edge of the original), which lets a scalar
  coverage's values follow its geometry through a cut
  ([section 5.4](spherical-geometry.md#54-provenance-and-scalar-coverages)).

How all of this is tested, against an oracle of sampled points and against the exact-arithmetic
prototype the algorithm was derived with, is in
[section 6](spherical-geometry.md#6-testing); what is known not to work is in
[section 7](spherical-geometry.md#7-deferred-work).
