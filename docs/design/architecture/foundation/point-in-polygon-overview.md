# Point-in-polygon on the sphere: an overview

What it means for a point to be inside a polygon on a globe, and how GPlates and pyGPlates decide
it, explained for a first reading. This page summarises
[section 3 of `spherical-geometry.md`](spherical-geometry.md#3-point-in-polygon), which is the
reference: complete and exact. Where this page and the reference differ, the reference and the code
(`src/maths/PointInPolygon.{h,cc}`, and `PolygonOnSphere::is_point_in_polygon`) govern.

A polygon here is an exterior ring and any number of interior rings, each a closed chain of great
circle arcs.

## 1. What "inside" means

On a plane a closed curve has an inside and an outside, and nobody has to say which is which: the
outside is the side that goes on for ever. On a sphere neither side does. A ring drawn round
Australia is just as much a ring drawn round everything except Australia. So the first decision is
a convention, and GPlates' is this:

> **A ring's interior is its smaller side.** A polygon's interior is the set of points inside an
> **odd number** of its rings.

The direction a ring is drawn in plays no part. Three rings, each tried both ways round, all give
what the eye expects:

| Ring | Its interior |
| --- | --- |
| a circle 30 degrees round the north pole | the cap round the north pole |
| a circle 120 degrees from the north pole (60 from the south) | the cap round the *south* pole: the smaller side |
| the equator | a tie; here alone the direction decides, and the interior is the hemisphere to the ring's left (the northern one, for a ring drawn eastwards) |

The second row is the rule's one limit: **a ring cannot mean more than a hemisphere.** A ring round
a region larger than half the globe means the rest of the globe instead. No tectonic plate of the
models measured comes to half the sphere; saying which side is meant, for one that did, is
[deferred work](spherical-geometry.md#7-deferred-work).

The rings then combine by counting. Exterior and interior rings are treated alike: a point inside
one ring is inside the polygon, inside two it is not (it is in a hole), inside three it is again.

```
            +-----------------------------------+
            |  exterior ring                    |
            |      +---------------------+      |
            |      |  interior ring      |      |
            |      |     +---------+     |      |
            |  A   |  B  |    C    |     |      |      D
            |      |     +---------+     |      |
            |      |      a second       |      |
            |      |      interior ring  |      |
            |      +---------------------+      |
            +-----------------------------------+

     A: in 1 ring, inside      B: in 2, outside (the hole)
     C: in 3, inside           D: in 0, outside
```

Counting has consequences for polygons that are not tidy, and they are deliberate:

- An interior ring that pokes out of the exterior ring makes the part outside the exterior
  *inside* the polygon (it is in one ring), and only the overlap a hole.
- A ring that crosses itself, a bow-tie, has both lobes inside, whichever way each is wound.
- Nothing requires a hole to run the opposite way to its exterior, as Shapefile and GeoJSON do.
  GPlates has never enforced ring directions, and a rule based on them would turn about half of all
  existing polygons inside out.

Counting is also what the globe draws. A filled polygon is painted ring by ring so that a pixel
covered an odd number of times is filled, which is the same parity. The definition was chosen so
that a point is inside a polygon exactly where the polygon is drawn filled
([section 3.1](spherical-geometry.md#31-definition)).

## 2. The outside point

"The smaller side" defines the answer but is no way to compute it. The computation needs one thing:
a single point known to be **outside** the polygon. Call it `R`. Every test, every area, and every
filled polygon on the globe starts from the same `R`, which is why they all agree.

For almost every polygon `R` is free. Take the polygon's boundary centroid `C` (the average
position of its exterior ring, weighted by edge length). If every vertex of every ring is less
than 90 degrees from `C`, the polygon sits in the hemisphere about `C`, and the point on the far
side of the globe, `-C`, is outside it:

```
                    . - ~ ~ ~ - .
                . '               ' .
              /       +-----+         \
             |        |  C  |          |      the hemisphere about C holds the
             |        +-----+          |      whole polygon, so -C, on the far
              \                       /       side of the globe, is outside
                ' .               . '
                    ' - . _ . - '
                          :
                         -C   (behind)
```

That test is one dot product per vertex, and `R = -C` was what the code always assumed. The
assumption fails for a polygon that wraps round the globe: a C-shaped ring, or a long band that
spirals. Its outline's centroid can sit anywhere, and `-C` can be inside it. For such a polygon,
and only such a polygon, `R` is searched for: 34 candidates spread over the sphere (`-C`, `C`, and
the 12 vertices and 20 face centres of an icosahedron) are taken farthest from the outline first,
and the first one that is outside is `R`.

Whether a candidate is outside such a ring is decided by the definition itself, the smaller side,
measured directly: the ring's area is summed as a fan of triangles from the point opposite the
candidate, and the total says which side of the ring the candidate is on, with a wide margin
because the candidate is far from the ring
([section 3.2](spherical-geometry.md#32-choosing-r) has the argument, which also covers rings that
cross themselves). This costs a pass over the polygon per candidate, once, and the result is cached
on the polygon.

## 3. Counting crossings

With `R` in hand, a test point `P` is decided by drawing the great circle arc from `R` to `P` and
counting how many edges of the polygon it crosses, over every ring. Each crossing changes sides, and
the arc starts outside, so:

> **`P` is inside if the arc from `R` to `P` crosses an odd number of edges.**

```
            +-----------------------------------+
            |                                   |
            |      +---------------------+      |
            |      |                     |      |
  R ........x..A...x.........B...........x..C...x.....D
            |      |                     |      |
            |      +---------------------+      |
            +-----------------------------------+

     crossings on the way from R:    A: 1, inside     B: 2, outside
                                     C: 3, inside     D: 4, outside
```

This is the planar "ray casting" test with one difference: on a plane the ray goes to infinity, and
here it goes to `R`. One count over all the rings gives the parity of section 1 at once; the rings
need not be told apart.

**The count is exact.** Whether an edge crosses the arc is decided by which side of the arc's plane
each end of the edge is on, and which side of the edge's plane each end of the arc is on: plain
sign tests, with no tolerance. The delicate case is an arc that passes exactly through a vertex,
where two edges meet:

```
           a                               a     b
            \                               \   /
   R --------v-------- P           R --------v-------- P       (the arc runs through v)
              \
               b

   the outline crosses the arc:        the outline touches the arc and turns back:
   must count once                     must count twice, or not at all
```

Both come out right because each vertex is put on one side of the arc or the other, once, and that
one decision is used for both of its edges. An edge can count only if its two ends are on different
sides. On the left `a` and `b` are on different sides, so exactly one of the two edges at `v`
counts; on the right they are on the same side, so both edges count or neither does. Either way the
parity is right ([section 3.4](spherical-geometry.md#34-strict-parity)).

## 4. The outline counts as inside

Crossing parity is exact, so a point exactly on an edge would be inside or outside by an accident
of rounding. That is not good enough for plates. Two neighbouring plates share a boundary, and a
point on that boundary must not fall into neither. So, after the count:

> **A point within about 9 metres of any edge or vertex is inside**, whatever the count said.

The distance is `1.4e-6` radians, the same number that the distance and intersection code treat as
"touching", so "on the outline" means one thing everywhere. It is checked as a real distance to
each edge, separately from the counting, and only when the count says outside.

The threshold is **inclusive on purpose**. In the plate models measured, about 6% of the vertices
that two neighbouring plates share are not bit-for-bit the same in both, so the plates' edges are
not quite the same edges; with exact rules a point on the boundary could be outside both. With the
threshold every point on a shared boundary is in *both* plates, and code that assigns a point to a
plate takes the first one that claims it.

It can be turned off (`use_point_on_polygon_threshold = false`), and is, wherever a point near a
polygon but outside it has to stay outside. The polygon overlay classifies its pieces that way
([the overlay overview](polygon-overlay-overview.md), section 3), and so does the distance code when
it asks whether a geometry is inside a solid polygon
([section 3.5](spherical-geometry.md#35-the-on-outline-test)).

## 5. Three speeds, one answer

Counting every edge for every point is fine for a few points and wasteful for a million. So the
same test runs at three speeds, and a polygon moves up through them as it is asked more questions.

**Low: no preparation.** Count over every edge. Nothing is built.

**Medium: two circles about `R`.** Measure, once, how near and how far the outline comes to `R`.
That gives two circles centred on `R` with the whole outline between them:

```
                      . - ~ ~ ~ ~ - .
                 . '                   ' .
              .'      . - ~ ~ ~ - .       '.
             /     .'               '.      \
            |     /         R         \      |     nearer to R than any edge:
            |    |      (outside)      |     |       no edge to cross, so outside
            |     \                   /      |
             \     '.               .'      /      farther from R than every edge:
              '.      ' - ~ ~ ~ - '       .'         no edge between here and -R,
                 ' .   outline lies    . '           so the same as -R (worked out once)
                      ' - in here - '
                                                   in between: count the edges
```

A point inside the inner circle has no edge between it and `R`, so it is outside. A point beyond
the outer circle has no edge between it and `-R`, the point opposite `R`, so it has whatever answer
`-R` has. For an ordinary polygon `R` is on the far side of the globe, so "inside the inner circle"
is most of the planet: nearly every point far from the polygon is answered by one comparison.

**High: the lune tree.** Every arc from `R` runs like a line of longitude from a pole. Split the
globe into segments like those of an orange, with `R` and `-R` as the poles:

```
                 \      |      /
                  \     |     /
                   \    |    /            seen from above R: each segment (a lune)
            - - - - - ( R ) - - - - -     runs from R to -R; the arc from R to a
                   /    |    \            point stays inside the point's own segment
                  /     |     \
                 /      |      \
```

The arc from `R` to a point never leaves the point's segment, so only the edges that enter that
segment can be crossed. Each segment keeps a list of its edges, and is split again while the list
is long. A test finds its segment and counts over a handful of edges instead of all of them.

**The policy.** By default a polygon starts at low speed, builds the circles on its eighth test,
and builds the tree once 20 of its tests have had to count edges (or on the thousandth test
regardless). Each structure is paid for only when the tests it would have made cheaper have cost
about as much as building it ([section 3.9](spherical-geometry.md#39-the-adaptive-policy) has the
measurements).

**The speeds must agree**, to the last bit, or a point would change sides as a polygon warmed up.
Two things make that hard, and both concern the 9 metres of section 4: a shortcut that answers
"outside" skips the outline check, so the circles are drawn slightly wide; and an edge can pass
within 9 metres of a segment without entering it, so segments list their near neighbours too
([section 3.7](spherical-geometry.md#37-the-three-speeds)).

## 6. What the rest of the code relies on

The interior is used far beyond the test itself, and every user takes the same `R` from the one
accessor (`PolygonOnSphere::get_outside_point`), so they cannot disagree about a polygon
([section 3.3](spherical-geometry.md#33-consumers-of-the-interior)):

| Who | What it relies on |
| --- | --- |
| Area and signed area (`get_area`, `get_signed_area`) | The area is that of the interior as defined here, so a ring's area is never more than half the sphere. The sign says whether the interior is to the ring's left |
| Orientation (`get_orientation`) | Counter-clockwise means the exterior ring's interior is on its left |
| Filled polygons on the globe, and the other fills | Each ring is drawn as a fan of triangles from `-R`, filled by parity, which is the crossing count of section 3 in another form |
| The interior centroid | A point at the middle of the interior, by area |
| Shortcuts by bounding circle | "Outside the circle round the outline" means "outside the polygon" only if `R` is out there too. For a polygon that wraps round the globe it may not be, and each shortcut asks first |
| The distance code | A point inside a solid polygon is at distance zero from it |
| The dateline wrapper, the raster meshes | They call point-in-polygon, so they follow |
| The polygon overlay | Its pieces are classified by exact tests (section 4's threshold off); a shared ring with nothing else to go on is classified by counting crossings from `R`; and its output rings mean their smaller sides, which is why a union larger than a hemisphere is an error |

For an ordinary polygon (one that fits in the hemisphere about `C`) the test, the area, the
orientation and the fills are exactly as they were before the smaller-side rule existed, bit for
bit: `R` is `-C`, as it always was. The rule changes those answers only for the polygons the old
assumption got wrong. (The interior centroid did change, for polygons more than about a degree
across, to an exact formula.)

## 7. What this page leaves out

- **Rings that cross themselves, and ties.** The precise definition is by winding number, of which
  "the smaller side" is the simple case; a ring within `1e-8` steradians of halving the sphere
  (under half a square kilometre on the Earth) is a tie, broken by its direction
  ([section 3.1](spherical-geometry.md#31-definition)).
- **How a candidate for `R` is tested**, and what happens when no candidate is far from the outline
  ([section 3.2](spherical-geometry.md#32-choosing-r)).
- **Edges nearly parallel to the arc, and edges of no length**, which take a different crossing
  test ([section 3.4](spherical-geometry.md#34-strict-parity)).
- **A test point at `R` or at `-R`**, where the arc from `R` has no direction. It is counted from a
  second reference point, about a quarter of the way round the globe
  ([section 3.8](spherical-geometry.md#38-degenerate-crossing-arcs)).
- **What was tried and rejected**: deciding the outline by loosening the crossing test, exact
  half-open edges, ring direction as the default, one outside point per ring
  ([section 3.6](spherical-geometry.md#36-rejected-alternatives)).
- **Threads.** The cached structures are built without locks, so one polygon must not be tested
  from two threads at once ([section 3.10](spherical-geometry.md#310-thread-safety)).
- **Every number**: the tolerances are one table,
  [section 1](spherical-geometry.md#1-tolerances).
