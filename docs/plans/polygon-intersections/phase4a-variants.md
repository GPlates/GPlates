# Phase 4a: the prototype's variants, explained

Written 2026-10-29 from the discussion of run 1's results (`phase4a-run1.md`), which are the
evidence; this file is the explanation. Sections 1 to 9 describe the variants as run 1 left them;
sections 10 and 11 give the configuration chosen after run 2 (`phase4a-run2.md`) and what run 2
corrected. When a variant is chosen (after run 2), the chosen one goes
into `overlay-walkthrough.md` section 7 (to be rewritten as "One point, several records") and later
the design doc, and the others into the design doc's rejected alternatives, with the reasons below.
Statements marked *measured* come from the prototype (`plans/polygon-intersection-tools/` in the
planning repository); the rest is reasoning.

## 1. What a record names

Every record from `GeometryIntersect` names zero, one or two original vertices:

| record | meaning | original vertices named | its position |
| --- | --- | --- | --- |
| LL | two segment interiors cross | none | computed (cross product of the planes) |
| SL / LS | a vertex of one polygon lies on a segment of the other | one | that vertex, exactly |
| SS | a vertex of each polygon coincide | two | geometry 1's vertex |

A touch record's position is the touching vertex itself, not its projection onto the other
segment. The projection gives only the record's sort key along that segment (fix F2), and so its
place in that ring's order.

## 2. Overlay vertices: the variants

An overlay vertex is a set of records. The variants differ in which records share one.

- **V0**: every record is its own overlay vertex. The wedge's two touches at `P`'s vertex `x` are
  two overlay vertices at the same position, with a zero-length piece of `P` between them.
- **V1**, identity: records that name a common original vertex are one overlay vertex,
  transitively (union-find). An SS record joins a `P` vertex to a `Q` vertex, so a group chains
  across both polygons, alternating `P`, `Q`, `P`, and each link spans at most `h`. LL records name
  no vertex and are always their own overlay vertex. No distance is measured by the overlay, but a
  group can join original vertices that are distinct: SS(`x`, `y1`) and SS(`x`, `y2`) make
  `{y1, x, y2}` one vertex although `y1` and `y2` are distinct vertices of `Q` `0.8h` apart. That
  is a merge by distance in effect, decided by `GeometryIntersect`'s coincidence test, not by the
  overlay.
- **V1b**: V1 plus the loop rule (section 3).
- **V2**: V1, then contraction. Overlay vertices joined by pieces with no original vertex and
  shorter than `2h` become one cluster, which keeps its members as a list and moves no position
  until output. The joining pieces are internal: not classified, not rays, not output. The
  cluster's rays are the pieces leaving it, each from its own member's position. This is the only
  family that merges by distance in the overlay itself.
- **V2b**: V2 plus: any single arc with both ends in one cluster is internal, whatever its length
  (section 4).

Each overlay vertex is output at one position: a `P` original vertex if it holds one (so that
`P`'s vertices stay exact, which scalar coverages want), else a `Q` one, else a record position.

## 3. The loop rule (V1b)

V1's failure (*measured*: every V1 failure in the catalogue): `Q` has vertices `y1` and `y2`
`0.8h` apart, both within `h` of `P`'s vertex `x`. Union-find makes `{y1, x, y2}` one vertex, so
`Q`'s edge `y1`-`y2` starts and ends at it: a loop. It has no original vertex inside (its ends are
records), so it has no test point; its rays run almost parallel to `Q`'s through rays, so it
cannot be deduced; its midpoint lies on `P`. The sector walk at the vertex is inconsistent, and
`P ∩ Q` came out empty.

**The rule:** a single arc (no original vertex) that starts and ends at the same overlay vertex is
internal. `Q` then passes through the vertex once, arriving at `y1` and leaving at `y2`.

**It needs no threshold**, because the test is whether the arc's two ends belong to one overlay
vertex: identity, not length. The arc's length is bounded anyway, by `GeometryIntersect`'s
tolerance rather than by a value the overlay chooses: at most `2h` when both its end records name
the same original vertex (both within `h` of it); more in a chained group (`x ≡ y1 ≡ x' ≡ y2`), by
up to `2h` a link, which run 2 measures. The arc is a there-and-back within the group's extent, so
it bounds nothing in the output.

A loop that holds an original vertex is not covered: it has a test point. The acute wedge's tip
piece `x, v, x` is classified at `v` (*measured*: reliably) and kept; section 5 says what happens
to its ring.

*Measured*: V1b/free is clean on the whole catalogue (0 oracle, 0 check, 0 repairs).

## 4. Clusters, and why V2b is needed (V2's failure)

The case (`d4 len=+2.5h phi=45 hole-between`): `Q`'s top edge is horizontal. `P`'s short segment
`u`-`w`, `2.5h` long at 45 degrees, crosses it with both ends within `h` of it, one each side, so
`GeometryIntersect` reports two touches (at `u` and at `w`) and no crossing. A hole of `P` has its
apex `a` exactly on `Q`'s edge between the two foots, a third touch. Along `Q` the three records are
consecutive: `w`'s foot, `a` `1.08h` further, `u`'s foot `1.45h` beyond.

- **V1b** keeps `u`, `a` and `w` as three vertices. `P`'s segment cannot pair with `Q`'s path as a
  common piece, because `a` lies between them along `Q`, so it is an ordinary single arc, classified
  at its midpoint, and output at its real positions: the region between it and `Q`'s path `w`, `a`,
  `u` (two triangles about `1.25h` by `0.9h`) is a real sliver with an arbitrary class.
- **V2's joining**: the two short `Q` pieces (`1.08h`, `1.45h`, under `2h`, no original vertex)
  join `w`, `a` and `u` into one cluster. `P`'s segment, at `2.5h`, is not short enough to be
  internal, but both its ends are now members of that one cluster: to V2 it is a `2.5h` loop from
  the cluster back to itself, a piece with two rays lying along `P`'s through rays. It is deduced
  the wrong class, the walk round the cluster's eight rays breaks, and `P ∩ Q` came out empty
  (*measured*, 516 of 900 oracle points wrong).
- **V2b** marks that arc internal as well. Both ends are in one cluster, the one the short `Q`
  pieces made, not two clusters; the arc is longer than `2h` but is a minor arc between two members,
  so it lies within the cluster's extent (`2.5h` here). Nothing is cut out of a ring afterwards; the
  arc is simply never classified, never a ray and never output. The cluster has six rays left and
  resolves (*measured*: clean).

**Why the region collapses to zero area under V2b**: at output the cluster is one vertex at one
position (its first `P` original vertex, `u` or `w`), and every piece leaving it starts there. The
two triangles are not represented at all. That is contraction's cost: the arcs that ended at `w`
now end at `u`, so the output moves by up to the cluster's width.

## 5. Zero area in the output: collapsed by the records, or by contraction

**The wedge tip.** The two touches of the acute wedge both sit at exactly `P`'s vertex `x` (section
1), so they are one overlay vertex in V1 and V1b, at one position. `Q`'s tip piece leaves `x` along
one edge, passes `Q`'s vertex `v` and returns along the other, so its output ring is the positions
`x`, `v`, `x`: two real vertices, and the arcs `x`-`v` and `v`-`x` are the same arc. The ring
really has zero area, and the clean-up's drop of it removes nothing from the output's point set.
What it stands for, the true tip triangle (thinner than `2h`, possibly long at a sharp corner), was
lost earlier, when `GeometryIntersect` collapsed `P`'s two crossings of the corner onto `x`.

**A midpoint-tested piece** runs between two *different* overlay vertices (otherwise the loop rule
makes it internal), each output at its own real position, so the sliver it bounds has real,
non-zero area in V0 and V1b. Only contraction (V2b) collapses it.

| | the wedge tip `x, v, x` | a midpoint-tested piece (V0, V1b) |
| --- | --- | --- |
| its class | reliable (tested at `v`) | undecided: no test point, not deducible |
| what loses the region | `GeometryIntersect` collapsing two crossings onto `x` | the choice of class |
| area in the output | zero; dropping the ring changes nothing | a few `h^2`; the two classes give different point sets |
| against the true geometry | the true tip is lost (under `2h` wide) | either class is within tolerance; neither is more right |

The positions collapse only where records share an overlay vertex: by identity in V1 and V1b, and
also by distance in V2 and V2b.

## 6. Midpoint tests: what is left of the midpoint idea

A piece with an original vertex is always tested there (walk-through section 3; the longest-arc
midpoint was rejected). A piece with no original vertex is deduced from its end vertices first, and
tested at its midpoint only when deduction fails. *Measured* fallback counts, every tie order: V0
84 in 42 cases, V1b 13 in 13 (all `d4 ... hole-between`), V2b 0.

**The inversion experiment** (*measured*): the whole catalogue rerun with every midpoint answer
reversed, as if rounding had decided each the other way, with deduction rerun after each: V0 and
V1b still 0 oracle, 0 check, 0 repairs. A piece reaches a midpoint test only when the classes
already known at both its end vertices allow either answer, and then its class decides only a
sliver thinner than the oracle's margin (`10h`). So the test does no useful work: after deduction
the C++ could take a fixed class and drop it, which also removes a decision that rounding makes.
Shown on the catalogue only; run 2 keeps the inversion on as an adversary and measures the sliver
(its area, and how far its points get from the boundaries) rather than relying on the margin.

## 7. Slivers in the output

Every variant but V2b can output slivers, of two kinds:

1. **Arbitrary-class slivers**, as in section 4: a region thinner than about `2h` whose class was
   left to a midpoint test. V0 had them in 42 cases, V1b in 13, V2b in none.
2. **Slivers of the true answer**: with a twist `3h` from `Q`'s edge, the two small triangles of
   `P ∩ Q` really exist. V0 and V1b output them; V2b outputs them when their vertices are more than
   `2h` apart and collapses them otherwise.

Partitioning feeds Assign Plate IDs, so a sliver can become a tiny extra polygon assigned to a
plate, which users see. **A design question for Phase 4b**: whichever variant wins, should the
output drop or merge any region all of whose points lie within about `h` of both inputs'
boundaries? That rule would be distance-based, on output regions rather than on records or pieces.
Where any merge goes is open (developer, 2026-10-29): the concern was only that removing
information early might cause problems nobody foresaw, so an earlier merge (such as V2b's) is fine
if the prototype's testing shows it works better.

## 8. Geometry at a vertex

The no-geometry rule at a vertex each polygon passes once (four ends: `P` crosses iff its two
classes differ, which must equal whether `Q`'s differ; a missing class follows; pairing by sector
memberships at a crossing, same-polygon at a bounce under Q11's split) is what every clean variant
has in common (*measured*: wherever it disagreed with the rotational order, the oracle agreed with
it). It removes, at four ends, one of the two reasons walk-through section 7.1 gave for
contracting: a piece with no direction.

At six or more ends the rotational order is still used, to deduce a class and to pair kept ends;
the classes alone do not fix the cyclic arrangement there (which of `Q`'s two passes `P` threads
between is geometric). The narrow neck (a `P` vertex between two `Q` edges under `2h` apart) is one
identity vertex with six ends and two exactly parallel rays, one from each side (*measured*: passed
in exact arithmetic; in doubles the order is a coin toss). Contraction does not help: the neck is
one vertex, not a cluster of short pieces, so V2b orders the same rays. If run 2 shows such orders
are a real risk, the fix is an extension of the no-geometry rule to six or more ends, whichever
variant wins. (V3 as first briefed, choosing the classes of a cluster's internal pieces by solving
the vertex parity constraints, was narrower; it was not needed in run 1.)

## 9. Where the variants stand after run 1

| variant | merges by distance | midpoint fallbacks | output depends on tie order | arbitrary slivers |
| --- | --- | --- | --- | --- |
| V0 / free | no | 84 | 90 cases, ring structure only | 42 cases |
| V1b / free | only as the records name it (section 2) | 13 | 5 cases, ring structure only | 13 cases |
| V2b / free | yes, at `2h` | 0 | never | none |

All three are clean on the catalogue; V1 and V2 fail (sections 3, 4), and the rotational order fails
somewhere under every variant but V2b. Run 2 tries to break the three with random clusters and
perturbations, keeps the midpoint inversion on, measures sliver areas, chained-group extents and
cluster widths, and probes six-end vertices; then the simplest survivor is chosen.

## 10. After run 2: the chosen configuration

Chosen by the developer, 2026-10-30, from `phase4a-run2.md` (the evidence) and
`phase4a-section7-draft.md` (the walk-through's account). Phase 4b extends it; this section says
what it is now and why each part is there.

Run 2's random hunt broke all three of run 1's candidates. V0 and V1b broke on the order in which
undetermined pieces were tested, which two new rules fix; V2b broke on its clusters, which nothing
fixed. The configuration:

1. **Identity vertices with the loop rule (V1b)**, as in sections 2 and 3.
2. **The spike rule**: two consecutive single arcs of one ring, out from a vertex to another and
   straight back (`A, B, A`), are internal. It is the loop rule's two-arc case, needs no threshold
   for the same reason, and is the commonest way a ring revisits an identity vertex along densely
   coincident boundaries (run 2: it fixed two of the four residual failures).
3. **The `LL` rule (`freex`)**: at a four-end vertex that is one `LL` record, the record says the
   two polygons cross, so each polygon's two classes must differ, one known class fixes the other
   three, and a bounce there is a check failure. Without it, a single arc between crossings could
   not be deduced until three neighbours were known, and went to a midpoint test (229 of 300 random
   cases needed one; 4 with the rule). It also let two arbitrary band pieces make an `LL` vertex
   look like a bounce, consistent and wrong (138 of 900 oracle points in `rays#81`). A touch record
   gets no such rule, because a touch can be a crossing or a bounce. The rule reads the record kind,
   which `GeometryIntersect` decided, so the overlay still makes no geometric decision.
4. **Midpoint tests in order of margin**: when deduction stalls, the undetermined single arc whose
   midpoint is farthest from the other polygon's boundary is tested first, then deduction runs
   again. Run 1 tested in piece order. In a neck, a `1.2h` band piece came first, its arbitrary
   class was deduced onwards through a four-end vertex, and a piece `10^4 h` long came out wrong (9
   of 300 `six` cases; V0 likewise, 33 of 300, through its zero-length pieces). The principle: a
   class decided inside the band must never be the source of a deduction for a piece with a
   margin. With it, what is left to midpoint tests are band pieces, whose class is arbitrary by
   nature.
5. **Option 3 for anchorless chains (Q3)**: a whole-ring common chain is classified by counting
   crossings from `R`, leaving out the chain's own arc, along a path that stays on `R`'s side of
   that arc's great circle (run 2 found that a path through an unrestricted waypoint arrived from
   the wrong side in about half the runs). Option 1, an offset test point, fails every thin face.

**What it was tested on** (all measured, run 2): 0 oracle, check or repair failures in 1498 random
cases of five generators, 60 chain cases, 60 shared-edge cases, the 463-case catalogue, 157 cascade
steps, 20 anchorless cases and 600 cuts by an open arc. The same under ulp jitter of the inputs,
under the rotational order computed in doubles, and under the fair midpoint adversary (section 11).

**What is still open, for Phase 4b:**

- **The bundle rule.** V1b fails 4 of 299 dense jittered shared edges (2 with the spike rule) and
  one six-end case from another seed. Each failure is at an identity vertex with six or more ends
  where several rays leave along one thick plane, so their order by angle is noise. The missing
  rule treats such rays as one bundle and solves the classes of the pieces between them from the
  vertex's parity constraints, instead of reading them off an order. The mechanism is V2b's
  failure seen from the other side: V2b meets it at every cluster, V1b only when boundaries
  coincide densely (vertices 4 to 11 m apart along each other).
- **Slivers of the true answer** (section 7): up to about `8.8e3 h^2` in run 2, output by every
  variant. Whether to merge or drop them is the distance-based output rule of section 7.
- **Chained identity groups have no fixed bound**: `28.8h` for 65 records on an edge digitised
  every `0.45h` along another. The output moves by the group's extent, set by the data;
  contraction would not shorten it.

## 11. What run 2 corrected in run 1's conclusions

- **The midpoint inversion of run 1 was unfair.** It negated every midpoint test, including
  reliable ones on long arcs that the free rule had failed to deduce (item 3 above). Negating a
  reliable test is simply a wrong class, and fails the oracle. The fair adversary negates a test
  only when the whole arc lies within `2h` of the other boundary; then it changes only slivers
  under `0.5h` wide (section 6's conclusion, that the test does no useful work, holds only for
  those band pieces).
- **V0 is not clean** (33 of 300 `six` cases under run 1's rules): its zero-length pieces can only
  ever be deduced, and testing one at its midpoint is testing a point on both boundaries.
- **V2b is not clean** (19 of 300 `six`, 3 of 60 `chain`, 18 of 299 `shared`, in exact arithmetic,
  under every rule and test order). Contraction joins two four-end vertices, which need no geometry,
  into one six-end cluster, which does, and whose rays leave from different member positions, so
  their order is not the order at any one point. It also moved the output by up to `20h`, and lost
  anchors for common chains. An earlier merge was acceptable if it tested better (section 7); it
  tested worse.
- **Float rounding never mattered.** The rotational order in doubles agreed with the exact order at
  every vertex where it was used, down to rays `1.8e-15` rad apart; two parallel rays can only swap
  with each other, which changes nothing. The neck's coin toss of section 8 is harmless.
- **Cascades did not grow clusters or groups** from step to step (40 cascades of three to five
  plates).
