# Phase 5 review 3: steps 7 and 8 of the overlay core

Reviewed 2026-11-05 (Fable), the diff `9cbc8e595..2f00346a8`: `src/maths/PolygonOverlay.{h,cc}`
(`is_kept`, `extract_rings`, `extract`, `is_ring_inside_ring`),
`src/unit-test/PolygonOverlayTest.cc`, `src/unit-test/PolygonOverlayOracle.h`,
`src/unit-test/PolygonOverlayPrototypeTest.cc` and the four re-exported snapshots, against
`phase4b-design.md` sections 3.10, 3.11, 4.2 to 4.4, 5.1, 5.2 and 9 (as amended at `27182759e`),
and the prototype's `kept`, `link`, `split_repeats` and `clean_rings_tagged` under
`CONFIGS['r4t']`. Line numbers are at `2f00346a8` (the source files are unchanged at HEAD).
Evidence is marked *run* (a build of `gplates-unit-test` in the Release tree with temporary probe
tests and source mutations, all since removed; `git status` shows only this file) or *reading*.

Baseline: 44 `PolygonOverlay*` tests pass and `File` is skipped; the two uncommitted sets report
`4192 prototype cases, 0 differ (0 in output only; 67 ring sets not compared, touching at two
vertices; 2642 with crossing input rings not checked as polygons)` and `2099 ..., 0 differ (...;
4 ...; 1883 ...)`.

The translation of the linking, split, clean-up, grouping and orientation is faithful where the
prototype has a counterpart, and the replacement of design 4.3's per-ring output side by the
parity-depth grouping and the one-point hemisphere check is sound (section 3). The findings are two
correctness defects on valid inputs that no committed case reaches, one of them upstream of these
steps but visible only now that there is an output oracle; then design statements the code no
longer matches, test gaps, and smaller matters.

## 1. Findings, most severe first

### 1.1 A vertex whose rays are all common cannot be paired: identical self-touching inputs throw

`PolygonOverlay.cc:3056-3073`, the sector-walk branch of `extract_rings`. At a vertex where every
ray is common, every constraint of `solve_vertex` is an xor (a common ray fixes `in subject != in
clip` only), so the starts `(a, b)` and `(!a, !b)` both solve it and `solution.in_subject` and
`in_clip` are unknown (`:1715-1733`). The branch then counts the vertex `unresolved` and a pairing
failure, and `extract_rings` throws `InconsistentOverlayException` ("could not be paired into
rings") although every piece is classified and `classify()` passed every check. The prototype's
`link` does the same (`overlay_proto.py:1721-1724`), so the comparison cannot see it; design 3.10's
table ("otherwise ... `solve_vertex` without the free rule ... output membership per sector is
`f(mP, mQ)`") does not foresee the unknown absolute memberships, and 9.2's "Linking's `unresolved`
and `pairing_failure` cannot occur once `classify()` has passed its checks" is false.

*Run*, three inputs, each as both subject and clip (identical polygons): a square with a triangular
hole whose apex touches the exterior's bottom edge (OGC-valid); a square with two holes touching at
a corner; a figure-eight pinched at a vertex. In each, the touch vertex has four common rays (the
classes all `same`, by the sector walk, `check_failures` 0), and `extract_rings(INTERSECTION)` and
`extract_rings(UNION)` throw with `unresolved_vertices` 2 and 18 pairing failures (the two
differences and the symmetric difference keep no end there and are right). A square against itself,
with no self-touch, is right under all five operations. Four or more kept common rays need both
inputs to pass one vertex twice along the same two boundaries: identical polygons with a hole
touching the exterior, or two holes touching, which partitioning a model by itself produces.

Fix: when the sector walk's memberships are unknown only in the absolute (every ray common, the
solution consistent, every piece classified), pair the kept rays consecutively in the order used,
either phase. The split of 3.11 then makes the rings right: a ring walked through such a vertex
twice is cut there into the arcs that return to it, which is the one non-crossing decomposition
whatever the pairing was (walk-through 6.4's argument), and a ring passing once pairs as the walk
found it. Alternatively use ring continuity on the subject's ends (3.10's one-input rule): the two
boundaries coincide, so the subject's ring structure is the output's. Either way, add the three
inputs as unit tests (`check_oracle` on each), and correct 9.2.

### 1.2 A shared edge cut on one side only is not a common pair; its band classes corrupt the chains

Upstream of this diff (`find_common_pairs`, `PolygonOverlay.cc:901-962`, step 2) but found by step
7's oracle, and the step 8 policy that lets the wrong output through is in this diff.
`find_common_pairs` pairs a subject piece and a clip piece that are single arcs between the same two
vertices, on the argument that "an original vertex of either inside the stretch would itself be a
record". That is true of a vertex of the *coincident* ring: a record (vertex, other's segment) cuts
both. It is false when the vertex belongs to another ring of the same input: `GeometryIntersect`
never intersects an input with itself, so a hole's apex touching its own exterior's edge cuts the
other input's coincident segment (a record names it) and the hole, but not the exterior's segment.

*Run*: `P` a square with a triangular hole whose apex touches the bottom edge; `Q` the same square
without the hole. Three common pairs (the other sides); `P`'s bottom edge is one piece corner to
corner, `Q`'s two pieces corner to apex to corner, no pair and nothing counted (`common_ambiguous`
needs both sides under one key). All three are band pieces on the other's outline, classified by
midpoint test with the threshold off (arbitrary): `P`'s `in`, `Q`'s both `in`. At the bottom
corners that combination (`P` inside `Q` and `Q` inside `P`, both interiors on the same side) is
consistent only with the three common chains `opposite`, which the sector walk duly deduces; the
checks pass (`check_failures` 0, no repair). The intersection comes out as a zero-area triangle
(corner, apex, corner) plus the hole; the oracle fails 110 of 357 samples under every operation but
the symmetric difference; the union throws "exceeds a hemisphere", which it does not. For
partitioning the point is the intersection and the difference: `hemisphere_check_failures` is
incremented (`:3391-3393`) and **the wrong output is returned**, in every build. Design 9.2's "never
a silent wrong answer" does not hold for it.

The same configuration with the touching ring in the other input (`P` and `Q` sharing an edge, `Q`'s
hole apex on it, and the mirror) happens to classify consistently with the truth and is right apart
from a zero-area three-point ring in the intersection (the sliver between the two unpaired
coincident pieces; step 14's filter would drop it). With the apex `0.5h` off the edge, the same. So
the trigger is a self-touch of one input on a boundary it shares with the other: rare, OGC-valid,
and silent.

Two fixes, independent. (a) In `find_common_pairs`, when one side's piece between vertices `u` and
`w` has no partner, look for a chain of the other side's pieces from `u` to `w` through vertices
that lie within the thickness of the piece's arc, and pair the piece with the chain (or cut the
piece at those vertices: the stop list allows a stop inside a segment); counted. (b) In `extract`,
treat a hemisphere-check failure of the intersection or a difference as what it is, the rings not
describing `f`: throw when `Options::throw_if_inconsistent` is set, count otherwise, as every other
failure does (`:3379-3393`); and the union's message should not name a cause the check cannot
distinguish ("the output read by parity is not the operation's at the reference point: the output
exceeds a hemisphere, or the classes are wrong"). The partition layer (step 11) must not rely on the
counter alone.

### 1.3 Design statements the code no longer matches

- **9.2**: "Linking's `unresolved` and `pairing_failure` cannot occur once `classify()` has passed
  its checks; they are assertions." Both occur (1.1), and the code throws or counts them
  (`:3294-3303`), which the plan records as a deviation. Rewrite as: a pairing failure throws
  `InconsistentOverlayException` (counted when not throwing), and the all-common vertex is either
  fixed (1.1) or listed as the one consistent input that reaches it.
- **9.1**: "the kept rays at a sector-walked vertex equal the keep-rule's (`keep_mismatch`) ...
  These are `GPlatesGlobal::Assert` in debug builds". `keep_mismatches` is counted only
  (`:3095-3098`); no assertion in any build. Say so, or assert it.
- **4.3 as built**: "An output exceeding a hemisphere is read as its complement, which flips every
  point (its outer ring's smaller side is the complement, and every other ring is then at depth
  0), so one point detects it." The conclusion is right but the parenthesis is not the reason: with
  a correct edge set the parity over the rings' smaller sides is one of exactly two regions, the
  output or its complement, everywhere (each ring's smaller side differs from its true side by the
  whole sphere, so an odd number of such rings complements the reading and an even number leaves
  it), and that is why one point decides and why the check cannot misfire on a correct output.
  What the check detects is "an odd number of rings whose true interior is their larger side",
  which is "exceeds a hemisphere" only for a correct output; for a wrong one (1.2) it is a class
  error. State both.
- **4.2**: "`O(R_out^2)` bounding-circle checks and at most that many point tests" survives from
  the rejected pre-check that the paragraph above it says "was not needed"; the cost is
  `O(R_out^2)` containment tests, each a minimum-distance scan of the ring's vertices and a
  point-in-polygon test.
- **11.3**: the second oracle item ("the inside and outside outputs are disjoint and their areas
  sum to `P`'s") is not implemented; `PolygonOverlayOracle.h` has the point oracle (rings and
  polygons) and the orientation count only. Either add it (with `get_area()` on the output
  polygons, filter off) or mark it for step 11.
- **3.10**, the walk: "Each output point records the stop it came from" is step 10; `Entry` keeps
  the vertex index only. Fine as a forward reference; say so.

### 1.4 Minor observations (reading unless marked)

- The hemisphere check throws for `UNION` and `SYMMETRIC_DIFFERENCE` whatever
  `Options::throw_if_inconsistent` says (`:3381-3390`, documented in the header). The hunts run
  with the option off to count rather than stop; a union over a hemisphere in a hunt then aborts
  the case instead of being counted. Consider honouring the option (count
  `hemisphere_check_failures`) and leaving "never return the complement" to the callers that set
  it, or keep it and say why in the header (the comparison already catches the exception,
  `PolygonOverlayPrototypeTest.cc:789-797`).
- `is_ring_inside_ring` (`:192-239`): the fallback's "farthest" vertex or midpoint may be within
  `1e-10` rad of the other ring and is then tested with the threshold off; exact, except within
  the length of a zero-length edge of the other ring (the plan's point-in-polygon note). A triangle
  of three touch vertices whose edge midpoints all lie within that of a zero-length edge is
  unlikely; counted nowhere if wrong (it would show as `grouping_failures` or a polygon oracle
  failure).
- The walk follows the subject piece of a common pair (`ray_end`, `:2902-2906`), so the output's
  positions along a shared boundary are the subject's; the clip's may differ by up to `h`. Design
  3.10 does not say which; for partitioning the subject's is the right choice. Record it (and it
  matters for provenance, 7.1: such a point has both locations).
- `extract_rings` on a piece end with no vertex (a polyline's dangling end, step 9) would index
  `used[no_ray.first]` (`:3172-3173`): `arrive` must be checked before step 9 adds polylines.
- `std::vector<int> first_seen(d_vertices.size(), -1)` per walked ring is `O(V)` per ring; fine at
  these sizes.
- Code style: nothing found in the diff (hard tabs, Allman braces, no line over 100 columns,
  pointers and references bound to the name). `PolygonOverlayOracle.h` is `inline` functions in a
  namespace, as the file's purpose allows.

## 2. Test gaps

*Run*, mutations of `extract_rings` and `extract` (one at a time, the suite and the two uncommitted
sets; "differ" counts cases):

| Mutation | Suite | large | chains |
| --- | --- | --- | --- |
| M1 the free rule's crossing pairing replaced by the bounce one (`:2979`) | 8 tests | 884 | 161 |
| M2 ring continuity replaced by consecutive kept rays (`:3025-3048`) | passes | 0 | 0 |
| M3 the sector walk's other phase (`:3105`) | `Hunt` 1 case | 4 | 0 |
| M4 the bounce pairing replaced by `P1-Q1`, `P2-Q2` (`:3003-3004`) | passes | 6 | 0 |
| M5 the split disabled (`:3239`) | 3 comparisons, no unit test | 1159 | 451 |
| M6 containment by the first vertex, margin ignored (`:218`) | 9 tests | 713 | 119 |
| M7 the reference point's margin dropped (`:3356-3357`) | 3 tests | 121 | 229 |
| M8 every ring's orientation reversed (`:3452`) | 12 tests | 1550 | 216 |

(M1 fails `LinkTwoSquares`, the five `Group*`, `Orientation*` and `Union*` tests, `Catalogue` in 69
cases and `Hunt` in 45; M5 fails `Catalogue` 57, `Hunt` 74 and `Bundle` 12; M7 fails
`LinkCatalogueFamilies`, `Catalogue` 17 and `Hunt` 13.) So the committed suite alone would miss M3
and M4 but for one hunt case each, and the uncommitted sets catch them by a handful; M2 is caught
by nothing; and M5 passes every unit test. The reasons, and what to add:

1. **A wrong pairing at one vertex is undone by the split** (the plan's own observation), so a
   pairing rule is tested only where it is wrong *systematically* (M1, at every crossing) or where
   two wrong vertices conspire. M3 and M4 differ from the right rule only at vertices that are rare
   (M3: six or more ends with both inputs passing more than once, in four `six` cases; M4: a bounce
   with four kept ends, in six `near` cases), and the ring comparison skips exactly the cases where
   pairing matters most (rings touching at two vertices, or a ring through a vertex twice: 67 + 4 +
   8 + 7 ring sets). Commit `six#27` and `#207`, `near#48` and `#277` (seed 1) to the hunt
   snapshot, and add two unit tests that pin the pairing by ring sizes: a bounce with four ends
   kept (the union of two squares whose corner touches the other's edge from inside: two rings, not
   one), and a both-pass-twice vertex (two pinched eights whose lobes alternate about the vertex,
   under the union: four rings of three; *run*, right today).
2. **The split is reached by one unit test and pinned by none.** *Run*: `rings_split` is 4 and
   `collapsed_rings_dropped` 6 in `LinkCatalogueFamilies`' wedge (the tip rings `x, v, x`), 0 in
   every other unit test; `LinkPinch` (`PolygonOverlayTest.cc:1472-1482`) says "one walk passes
   that vertex twice and is cut there", but the apex is a four-end bounce, whose split pairing
   (3.10) never needs the cut. Assert the two counts in the wedge, and the ring sizes of its
   intersection and difference.
3. **Ring continuity is verified by nothing.** M2 pairs the kept ends of a one-input vertex in
   index order; that equals continuity whenever the two passages of the input do not cross each
   other (its rays are then adjacent in the measured order), so it differs only for self-crossing
   inputs, which the polygon checks skip, and the prototype's rings there pass a vertex twice,
   which the ring comparison skips. There is no valid input on which the rule's choice is
   observable after the split, so this is a property of the rule, not a test to write; say so at
   `:3017-3022`.
4. **No test has a vertex whose rays are all common with four or more kept** (1.1), and none runs
   `extract` on identical inputs at all: `ClassifyAnchorless` (`PolygonOverlayTest.cc:1201-1260`)
   stops at the classes. Add `check_oracle` on the identical square and on 1.1's three inputs.
5. **No test has a shared edge cut on one side only** (1.2). Add it; until the fix, it pins the
   counted failure.
6. **Orientation is checked against the function that produced it.** `count_misoriented_rings`
   (`PolygonOverlayOracle.h:280-300`) calls `PolygonOrientation::calculate_polygon_orientation`, as
   `oriented` does (`:3449-3451`), so a wrong convention in that function (or a case where it
   fails, such as a ring whose gnomonic projection fails and whose fan sum is zero) passes. One
   independent check suffices: for a sample point known to be inside an exterior ring and far from
   it, the ring's winding about the point (the sum of the signed angles of its edges) is `+2pi`;
   or compare against `SphericalArea::calculate_polygon_exterior_ring_signed_area` where the ring
   is not tiny. `OrientationIndependentOfInputs` checks only that reversed inputs give the same
   direction.
7. **The polygon checks skip 63 % of the large set and 90 % of the chain set** (crossing inputs).
   That is by design, but the grouping is then exercised by about 1550 + 216 generated cases,
   nearly all of them one exterior with no hole. The committed hand tests cover holes two and
   three deep; add a generated set with holes (the `tinyhole` generator without the vertex moves,
   or one placing a small clip hole inside the subject's hole) so that the parent choice is
   covered at scale.
8. **`hemisphere_check_skipped` is asserted zero only**; no input reaches it (a ring through the
   six axis points does not: *run*). Fine; note it.
9. **The prototype ring comparison** (`PolygonOverlayPrototypeTest.cc:429-520`) matches rings by
   `same_ring` in either direction, so it cannot see orientation, by design; and it compares the
   intersection, difference and union only. The reverse difference and the symmetric difference
   are covered by the oracle and `LinkTwoSquares` (and M1 shows the symmetric difference's pairing
   is caught through the grouping checks). Fine.
10. The oracle's area-sum item of design 11.3 is not implemented (1.3). Cheap to add on the
    polygon outputs of the intersection and the difference.

## 3. Checked and found correct

Each item was traced against the prototype function the design names, by reading, and the suite,
the two uncommitted sets and the probe inputs of section 1 were run with per-operation output.

- **The keep-rule** (`is_kept`, `:2847-2872` against `kept`): the four tests of 3.10, a common
  piece's class read from `common_class`, `none` while unclassified; `is_in_output` is 5.1's
  table. Checked by hand for every operation at a crossing (two of four ends kept, always one of
  each input, so the single pairing is right) and at a bounce (zero, two of one input, or four).
- **The free rule's pairing** (`:2973-3007` against `link`'s `_is_four_end` branch): the sector
  `(mP, mQ)` with `f` true is bounded by the subject ray of class `mQ` and the clip ray of class
  `mP`, which are the two rays adjacent to that sector (its neighbours are the clip ray inside `P`
  iff `mP` and the subject ray inside `Q` iff `mQ`), so the symmetric difference's two pairs are
  adjacent-ray pairs and never cross; a bounce's pairing by input is non-crossing since neither
  input's rays separate the other's. M1 confirms the suite sees the crossing case.
- **Ring continuity** (`:3017-3054` against `_ring_neighbour`): the next non-internal piece of the
  ring beyond the end, cyclic, skipping internal pieces (a spike's two pieces both start and end
  at the vertex, so the skip stays there); the neighbour's end at this vertex is required and its
  piece kept, else a failure; symmetric, so each pair is set twice to the same value. The
  condition matches the prototype's `len(set(flips)) == 1` (a common ray has both flips).
- **The sector walk's pairing** (`:3056-3113` against the rest of `link`): `solve_vertex` without
  the free rule returns the order used (the one-passage rule's built order, the bundle's chosen
  order, or the measured one) and per-position memberships of the sector after that position; a
  ray at position `j` is kept iff the output differs between sectors `j - 1` and `j`; the kept set
  is compared with the keep-rule's (sorted, as the prototype's set comparison); the sequence is
  rotated so that its first ray is followed by an output sector and consecutive pairs bound output
  sectors. The vertex is unresolved if `consistent` is false or any membership is unknown (1.1 for
  when that happens with every piece classified).
- **The walk** (`:3115-3194`): rays in vertex and ray order (deterministic, where the prototype's
  `next(iter(unused))` is not), the vertex's output position first, the piece's interior stops in
  the direction walked (reversed when leaving from the piece's end; a cyclic stop range for a
  closed ring, `:3154-3162`), the arriving ray marked used, its partner next, closed on return to
  the start. The guard of `2 * pieces` bounds a walk that cannot close; every partnered ray is in
  exactly one cycle of the partner-and-piece permutation, so a consistent pairing always closes.
  Kept closed pieces of closed rings are rings of their stops (`:3197-3211`; a polyline's
  whole-piece is excluded, for step 9).
- **The split** (`:3218-3250` against `split_repeats`): the first vertex seen twice, by identity;
  `v, A` and `v, B` as `[first, second)` and `[second, end) + [0, first)`; the rest ring examined
  first (both push onto `walked`, popped from the back), until no ring repeats; the results
  reversed into walk order. Terminates: each cut shortens.
- **Clean-up** (`:3257-3292` against `clean_rings_tagged`): consecutive repeats and a last equal to
  the first, exact (`is_same_position`, coordinate for coordinate, which
  `PointOnSphere::operator==` is not); fewer than three distinct positions dropped; counted. The
  prototype's `tiny_area` count has no counterpart (4.5's filter is step 14).
- **The comparison's ring matching** (`compare_rings`): prototype names resolved through the
  record map to the overlay's vertex positions and ring vertices, near repeats within `1e-9` rad
  removed on both sides, cyclic match in either direction, each overlay ring used once, counts
  compared; the ambiguity skip is exactly a repeated name in one ring or two names shared by two
  rings. Nothing is compared for a case with a band or common difference, as intended.
- **The hemisphere check** (`:3327-3398`): sound as a one-point test, for the reason in 1.3 (the
  parity over smaller sides is the output or its complement everywhere, given a correct edge set),
  so it neither misses nor misfires on a correct output, touching rings included (touching changes
  no edge). The reference point is at least `10h` from both outlines, and every output edge lies
  within about `h` of an input edge (its ends are stops on the outline and a record's position is
  within `h` of its segment), so every point-in-polygon test it makes is clear of the band; the
  candidates are deterministic and the first clear one is taken. *Run*, inputs whose rings' smaller
  sides are the far side of the sphere (a belt between 60 and 80 degrees from a point, as an
  exterior ring 100 degrees from its antipode and a hole 120 degrees from it, which
  `PointInPolygon` reads as the belt, area 0.163 of the sphere): with a small clip inside the belt,
  a clip crossing the inner ring, and a 70-degree cap about the point, every operation passes the
  polygon oracle, no ring is misoriented, nothing throws and nothing is counted; the belt's rings
  group as exterior and hole by their smaller sides, as `PointInPolygon` would read them. Two caps
  100 and 95 degrees in radius about antipodal points, whose union is all but a thin belt, give two
  polygons and no throw: "exceeds a hemisphere" is per ring, as it should be. The union over a
  hemisphere and the shared-edge symmetric difference throw as the test says. Crossing input
  rings: `PointInPolygon` reads a ring by its winding number against the rounded mean, so a
  twist's lobes are inside whichever way they wind, and the output's self-crossing rings are read
  the same way; the check does not misfire on the catalogue's twists (0 hemisphere failures in all
  774 committed cases).
- **Grouping** (`:3400-3436`, `is_ring_inside_ring` `:192-239`): the first vertex of `r` farther
  than the point-on-polygon threshold from `r'` decides, threshold off; else the farthest of the
  vertices and edge midpoints; none farther than zero means coincident (counted); every pair
  tested both ways; the parent is the container with the most containers (for nested rings the
  containers form a chain, so depths are distinct); an odd-depth ring whose parent is not an
  exterior is kept as an exterior and counted. *Run*: `GroupIslandsInHoles` and the belt inputs
  give the expected depths; M6 shows the margin matters.
- **Orientation** (`:3438-3457`): `PolygonOrientation::calculate_polygon_orientation` on the ring
  alone, `COUNTERCLOCKWISE` meaning the interior (the smaller side, or for a self-crossing ring
  the parity interior) on the left as seen from outside the sphere; reversed when that is not what
  the ring's role needs. The function's sign test is a plain `<` on the projected area (no `real_t`
  tolerance, `PolygonOrientation.cc:206`, `Real.h:129-133`), so a ring the thickness across still
  has a sign; where the projection fails (a point more than 45 degrees from the boundary centroid)
  the fan sum from the apex `PointInPolygon` chose agrees with its interior. The design's reason
  for not using `SphericalArea` directly (zero for a ring of zero-length edges) is right.
- **The polygon oracle** (`count_failures` on polygons): each output polygon read as
  `PolygonOnSphere` reads it (parity over its rings), a sample in two polygons counted as a
  failure, which is what catches a crossing or ungrouped pair (M1's `grouping_failures` and
  `num_containing > 1`). `get_samples` is as 11.3 describes (a global disc plus annuli round every
  vertex, margin `max(10h, 2 x cluster width)`).
- **Diagnostics and failure**: `d_diagnostics` is `mutable` and the linking and grouping counters
  accumulate over every `extract` (documented at the members); a pairing failure throws after the
  rings are built, with the count in the message, unless `throw_if_inconsistent` is off (then the
  affected rings are missing and the rest returned, as the header says).
- **Robustness**: `ray_of_end` is complete for every end a ray carries, so `arrive` is always set
  for a polygon subject; `partner[v]` is sized to the rays; `solution.order[j]` indexes rays;
  `sequence.size()` is even before pairing; `ring_piece_range` is a linear scan per kept end of a
  one-input vertex (fine); the split indexes `first_seen` by vertex; `PolygonOnSphere::create` is
  given at least three distinct points by clean-up. No non-determinism: no unordered container
  reaches the output.
