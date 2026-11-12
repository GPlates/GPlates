# Phase 4b, step 2: the consumers of partitioning

Written 2026-10-31 (Opus), from the code at `e7d702919`, as input to the written design (step 3).
Doc section 5.1 (`spherical-geometry.md`) already describes `PolygonPartitioner`,
`PolylineIntersections`, the cascade and the no-intersection shortcut, so this file does not repeat
them. It covers who calls partitioning, what each caller assumes about the output, the scalar
coverage paths, and what the design must provide. It ends with the questions for the developer's
checkpoint. Statements marked *measured* were run against the pyGPlates build in this worktree; the
rest is read from the code, with the line cited.

## 1. Who calls what

| caller | entry point | what it uses | after PR B |
| --- | --- | --- | --- |
| `GeometryCookieCutter::partition_geometries` (`GeometryCookieCutter.cc:341-443`) | `PolygonPartitioner::partition_geometry` | inside and outside pieces; the cascade feeds the outside pieces to the next polygon | receives polygons; pieces of pieces (section 3) |
| `PartitionFeatureUtils` (Assign Plate IDs: partition, and both "most overlapping" methods) | `GeometryCookieCutter::partition_geometry` (`PartitionFeatureUtils.cc:366`) | pieces per plate, scalar coverage ranges per piece | provenance replaces the epsilon map and the segment search (section 4) |
| pyGPlates `PlatePartitioner.partition_geometry` (`PyPlatePartitioner.cc:252-340`) | `GeometryCookieCutter::partition_geometry`/`partition_geometries` | pieces per plate, as Python lists | polygons; docstring warning goes |
| pyGPlates `PlatePartitioner.partition_features`, `partition_into_plates` (`PlatePartitioning.py:86-409`) | `partition_geometry` (Python) | pieces per plate become feature geometries | polygons; coverage handling (section 4.3) |
| pyGPlates `PolygonOnSphere.partition` (`PyGeometriesOnSphere.cc:3641-3719`, docstring `:4446-4518`) | `PolygonPartitioner::partition_geometry` | result enum, inside and outside lists | polygons; docstring warning goes |
| `GLReconstructedStaticPolygonMeshes.cc:917-978` | `PolygonPartitioner::partition_polygon`, no output lists | the result enum only, per quad-tree node | needs a cheap result-only path (section 5) |
| `GenerateVelocityDomainCitcoms.cc:272-277` | `PolylineIntersections::partition` on two two-point polylines | the first intersection point | one call to `GeometryIntersect` (or `GreatCircleArc::intersect`); then `PolylineIntersections` has no caller left |
| `VgpPartitionFeatureTask.cc:91`, `PlateVelocityUtils.cc:307`, `PyReconstructSnapshot.cc:291` | `GeometryCookieCutter::partition_point` | the first containing polygon | unchanged |

`GeometryCrossing` was deleted by PR A (F4). Nothing else includes `PolylineIntersections.h` or
`PolygonPartitioner.h` (`grep` over `src/`).

## 2. The result contract

`PolygonPartitioner::Result` is `GEOMETRY_INSIDE`, `GEOMETRY_OUTSIDE` or `GEOMETRY_INTERSECTING`,
exposed in pyGPlates as `PolygonOnSphere.PartitionResult` (`PyGeometriesOnSphere.cc:4701-4716`,
picklable). Today:

- a polyline or polygon with any record at all is `INTERSECTING` (`PolygonPartitioner.cc:451-459`,
  `:494-502`), even if it only touches the partitioner from one side, and its pieces are output;
- a polygon with no record is output whole, as the original object (`PolygonPartitioner.cc:154-167`);
- a point on the outline is inside (`partition_point` uses the threshold-on test); a multi-point is
  `INTERSECTING` only if it has points on both sides (`:577-581`).

The pyGPlates test asserts today's polylines for an intersecting polygon
(`test_geometries_on_sphere.py:1848-1876`, "will be fixed in future"), and the whole-polygon
identity for the non-intersecting cases (`inside_geometries == [polygon]`). The plate partitioner
tests (`test_app_logic/test.py:1649-1770`) partition polylines only.

**Proposed** (question 4): when every piece of a polyline or polygon has one class, return that
class and the original geometry, not pieces. A touch from one side then changes nothing, the
cascade does not re-split a geometry at a boundary it only touches, and the output keeps the input's
vertices and provenance trivially.

## 3. The cascade

`GeometryCookieCutter::partition_geometries` partitions the input by each polygon in turn and
passes the outside pieces on (`GeometryCookieCutter.cc:378-433`). With polygon output, plate N+1
partitions pieces cut by plate N, whose edges lie along the boundary N shares with N+1: the
common-chain case the overlay was designed for (walk-through section 5; run 2 section 8 ran 157
cascade steps clean).

**Provenance must compose through the cascade.** `PartitionFeatureUtils` maps each output point to
the *original* domain (`PartitionFeatureUtils.cc:454-576`). A piece cut by plate N+1 from a piece
cut by plate N has points whose provenance, relative to its immediate input, is "on segment `k` of
piece `p`". The design must give provenance relative to the original geometry, either by feeding the
partitioner an input that carries per-vertex provenance and composing, or by partitioning by all
plates in one overlay (not proposed: it is a different algorithm, and the inbox's spatial-index
note is where many-polygon operations belong).

**Networks' rigid blocks** (`GeometryCookieCutter.cc:535-568`): each rigid block is added as a
partitioning polygon, then the network boundary with the blocks as interior rings. Today the
interior rings are invisible to the cut (doc 5.1), so the order matters. Once interior rings are
cut properly, the network no longer claims a block's area, and the order of a block and its
network stops mattering (doc 5.1 notes that sorting can move a block after its network).

## 4. Scalar coverages

### 4.1 What GPlates does today

`PartitionFeatureUtils::partition_feature` (`PartitionFeatureUtils.cc:810-840`) gets each geometry
property's coverage (`ScalarCoverageFeatureProperties::get_coverages`), partitions the domain, and
builds a range per piece (`partition_range`, `:429-590`): a piece's point found in an epsilon
`std::map` of the original points takes that point's values; any other point is located on the
nearest original segment by `minimum_distance` (within 0.5 degrees, then unlimited) and interpolated
along it. Doc 5.1 describes this.

**A pre-existing out-of-range read** (reading the code; not run): for a polygon domain,
`partition_range` interpolates between `domain_points[closest_segment_index]` and
`domain_points[closest_segment_index + 1]` (`:522-523`, and `range_scalars[... + 1]` at `:539`). A
polygon's last segment, from its last vertex back to its first, has index `N - 1`, so an
intersection on it reads index `N`, one past the end of both vectors. Today it can be reached by any
polygon whose closing segment crosses a plate boundary. Also, for a polygon with holes,
`minimum_distance`'s segment index can refer to an interior ring (`GeometryDistance.h:96`), which
would index past the exterior points; that cannot happen today, since `PolylineIntersections`
leaves interior rings out, but it would with interior rings cut. PR B replaces this code with
provenance, which removes both.

### 4.2 The exterior-only assumption

A coverage's domain is the exterior ring's points: `ScalarCoverageFeatureProperties.cc:222-223` and
`:274` match a range to a domain by `get_num_geometry_exterior_points`. The other sites that rely on
it:

- `ReconstructedScalarCoverage.cc:72` (reconstructed points, "to match `get_coverages`");
- `TopologyReconstruct.cc:2780-2798` (`create_import_sample`, exterior ring only, tessellated or
  not), which deforming coverages use;
- `RenderedColoured{Polygon,Polyline,MultiPoint}OnSphere.h:61-62` (asserts one colour per exterior
  point), drawn by `GlobeRenderedGeometryLayerPainter` and `MapRenderedGeometryLayerPainter`;
- `PartitionFeatureUtils.cc:213`, `:237`, `:438`;
- `ReconstructScalarCoverageLayerProxy.cc` and `GpmlFormatReconstructedScalarCoverageExport.cc` go
  through `get_coverages` and the reconstructed points.

**pyGPlates disagrees with it** (*measured*, `cov_rings.py` in the session scratchpad):
`Feature.set_geometry((polygon, scalars))` checks the number of scalars against *all* the points,
holes included (`PyFeature.cc:1071-1072`, `:1245-1246`, `get_num_geometry_points`). On a square
with a square hole (4 + 4 points), 4 scalars are refused ("must match number of points in
geometry"), and 8 are accepted, but `get_geometry(coverage_return=geometry_and_scalars)` then returns
`None`, because reading goes through `get_coverages`, which expects 4. So pyGPlates cannot create a
readable coverage on a polygon with holes, and a GPlates coverage on one (exterior values only) is
one pyGPlates' `set_geometry` would refuse. (`TopologicalModel.reconstruct_geometry` takes only
points and multi-points, `PyTopologicalModel.cc:146-174`, so it is not involved.)

### 4.3 pyGPlates' `partition_features` drops coverages

`plate_partitioner_partition_features` (`PlatePartitioning.py:285-338`) partitions each geometry
property and sets the pieces with `feature.set_geometry(pieces, name)`. Setting a plain geometry
removes the coverage range associated with the domain's property name (`PyFeature.cc:1038-1049`),
so a partitioned feature loses its scalar coverage; only GPlates' Assign Plate IDs carries coverages
over. (Reading the code; not run.) Whether pyGPlates should carry them is question 2.

### 4.4 What a piece's points are, and which have values

Under an exterior-only coverage, the points of an output polygon's rings fall into these kinds:

| point | on the input's | value |
| --- | --- | --- |
| an original exterior vertex | exterior | its own |
| an intersection on an exterior segment | exterior | interpolated along the segment |
| an original hole vertex, or an intersection on a hole segment | hole | **none** (holes are outside the domain) |
| a partitioner vertex inside the input, or a point of a cut | interior | **none** |

A piece's exterior ring can contain all four kinds: where the partitioner cuts through an input
hole, the hole's boundary becomes part of a piece's exterior. So an exterior-only coverage on an
input with holes cannot give every point of a piece's exterior a value from the input alone, and a
cut gives points with no value even without holes. Question 1 is the policy for the points with no
value; question 2 is whether coverages move to all rings.

## 5. What the design must provide, from the consumers

1. **Polygon pieces with holes**, grouped into polygons (the ring grouping of design bullet 4).
2. **Provenance per output point**, relative to the *original* geometry through the cascade
   (section 3): original vertex (ring, index), on an original segment (ring, segment, ratio), or on
   the partitioner's boundary inside the input (with the partitioner's ring, segment and ratio, so
   that a cut can be interpolated along).
3. **A result-only path.** `GLReconstructedStaticPolygonMeshes` calls `partition_polygon` with no
   output lists once per quad-tree node, and the cookie cutter returns early when no lists are wanted
   (`GeometryCookieCutter.cc:421-428`). Today that path still builds the whole
   `PolylineIntersections` graph. The overlay needs only the records and enough classification to
   tell `INSIDE`, `OUTSIDE` or `INTERSECTING`, and should stop there.
4. **The no-intersection decision by point-in-polygon** with the threshold off, on every ring of
   both polygons (requirements; doc 5.1's three bugs), which also makes
   `GLReconstructedStaticPolygonMeshes`' own containment test (`:966-980`) redundant but harmless.
5. **Polylines** through the same core (run 3 task 2), so `PolylineIntersections` can be deleted
   once `GenerateVelocityDomainCitcoms` calls the primitive directly.
6. **No speed regression that users notice.** Assign Plate IDs and `partition_into_plates` partition
   every feature by up to every plate. Phase 5 should time a real case (a global coastline file by a
   plate model) before and after.

## 6. Questions for the developer's checkpoint

Each has a recommendation. Questions 5 and 6 wait for prototype run 3's results.

1. **Values for coverage points with none** (section 4.4: cut points, and hole points under an
   exterior-only coverage).
   - (a) Interpolate along each cut, by arc length, between the values at the cut's two ends,
     where those ends have values (they lie on the input's outline). **Recommended** for cut points.
   - (b) The value of the nearest point on the input's exterior with a value.
   - (c) Drop the coverage from any piece that has a point with no value.

   Hole points under an exterior-only coverage have no good answer: (a) along the hole between its
   two ends where it meets a cut, (b), or (c). This is the argument for question 2.
2. **Coverage scope.** Make a polygon coverage's domain all of its rings, consistently: GPlates'
   extraction, reconstruction, deformation (`TopologyReconstruct`'s import sample), rendering and
   export, and pyGPlates' reading (which already writes all rings, section 4.2).
   - (a) In a separate PR before PR B, so that PR B partitions all-ring coverages and needs no
     answer for hole points. **Recommended**: it fixes the pyGPlates inconsistency on its own, and
     keeps PR B's review on partitioning. Existing files with exterior-only coverages on polygons
     with holes would need reading as before (a range the length of the exterior is the exterior's).
   - (b) In PR B.
   - (c) After 1.1; PR B gives hole points the answer chosen in question 1.

   Also: should pyGPlates' `partition_features` carry coverages over as Assign Plate IDs does
   (section 4.3)? Recommended yes, in PR B, since provenance makes it the same code; until then it
   is a known gap rather than a regression.
3. **"Most overlapping" for polygons.** Both products choose the plate with the greatest summed arc
   length of pieces (`PartitionFeatureUtils.cc:647-663`, `PlatePartitioning.py:351-372`). Polygon
   pieces add the cut edges to their perimeter, so the measure stops being "how much of the outline
   is in the plate".
   - (a) Area for polygons, arc length for polylines. **Recommended**: it is what "overlaps the most"
     means for a region. A visible change, so changelog entries in both products.
   - (b) The length of the input's outline within each plate, from provenance: today's measure,
     unchanged.
4. **A geometry that only touches** returns the original geometry and `INSIDE` or `OUTSIDE`, not
   pieces and `INTERSECTING` (section 2). Recommended.
5. **A polyline piece along the partitioner's boundary** (requirements question 2): inside, or by
   which side the polygon's interior is on? Run 3 measures what the choice changes.
6. **Slivers** (variants section 7): drop or merge output regions whose points all lie within about
   the thickness of both inputs' boundaries? Run 2 measured slivers of the true answer up to about
   `8.8e3 h^2`; run 3 measures them under the bundle rule.
7. **`DateLineWrapper`.** Run 2 (section 10) showed the core cuts by an open arc with three
   additions, and that no valid case failed. Recommended: the design keeps those three additions in
   its data structures, and the port stays a separate PR after B (in 1.1 if time allows, as decided
   2026-10-23). A polygon round a pole, the dateline's own case, has not been run.
8. **Where it lives.** Recommended: a new `src/maths/PolygonOverlay.{h,cc}` (the core, Boolean
   operations, polylines, provenance), with `PolygonPartitioner` keeping its interface as the
   partition layer on top, and `PolylineIntersections` deleted.
