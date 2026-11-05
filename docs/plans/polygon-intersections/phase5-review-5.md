# Phase 5 review 5: step 11, the partition layer

Reviewed 2026-11-05 (Fable), the diff `e28d73f67..cb1844796` (code commit `e2d847bfe`, plan commit
`cb1844796`): `src/maths/PolygonPartitioner.{h,cc}` rewritten on `src/maths/PolygonOverlay.{h,cc}`
(`classify(stop_when_result_known)`, `compose_subject_location`'s `original_subject` and
`cascade_clips`, the `DIFFERENCE` undef), `src/app-logic/GeometryCookieCutter.{h,cc}`, the three
list readers, `src/unit-test/PolygonPartitionerTest.cc` and the two pyGPlates tests, against
`phase4b-design.md` sections 4.1, 5.3, 7.1, 7.2, 8.1, 8.2, 8.4, 8.5, 9.2, 10.2 and 14, and PLAN.md's
step 11 and 2026-11-05 entries. Line numbers are at `cb1844796`. Evidence is marked *run* (a build of
`gplates-unit-test` in the Release tree with temporary probe tests and environment-free mutations in
`PolygonPartitioner.cc` and `GeometryCookieCutter.cc`, all since removed by `git checkout` and a
touch; both suites rebuilt and green afterwards, 175 of 175 and 7 of 7; `git status` shows only this
file) or *reading*.

Baseline: 66 `PolygonPartitionerTest` and `PolygonOverlay*` tests pass. The four uncommitted sets
report `0 differ`, and a probe run inside the `File` test over all 8490 cases found, through the
partitioner itself: the result-only path and the full path agree on every polygon case except four
(section 1.1); every whole result is the input object; every piece's table is the length of its
`GeometryUtils::get_geometry_points` and every location resolves within `3h` (subject vertices
exactly), over 25303 first-step polygon pieces (2437 of them with holes, 12081 hole points), 13689
second-step pieces cut again by the clip through `partition_geometry(PartitionedGeometry,
..., CascadeProvenance)` with the original and the clips given, and 5340 polyline runs.

No silent wrong answer was found. The findings are one semantic point the design does not know
about (a second caller of the result-only path), stale pyGPlates docstrings, then test gaps: nine
further mutations, every one of which survives the committed suite.

## 1. Findings, most severe first

### 1.1 pyGPlates `PolygonOnSphere.partition(geometry)` with no lists is a second caller of the result-only path (reading, with the run of 1.1's cases)

Design 9.2 and 10.2 give the result-only path one caller, `GLReconstructedStaticPolygonMeshes`
(8.5), which is why it may answer `GEOMETRY_INTERSECTING` for an inconsistent overlay instead of
throwing (`PolygonPartitioner.cc:619-627`) and for a region the cut collapses to nothing
(`:606-628`, no extraction). But `PolygonOnSphere.partition` (`PyGeometriesOnSphere.cc:3687-3695`)
calls `partition_geometry(geometry, inside, outside)` with whatever lists the user gave, and with
none it reaches `visit_polygon_on_sphere`'s no-list branch (`:306-311`), which is the result-only
path. So, for a user:

- `polygon.partition(p)` returns `intersecting` where `polygon.partition(p, inside, outside)` raises
  (an inconsistent overlay, 9.2's "never a silent wrong answer"; the same polygon, a different
  answer by whether lists were passed);
- `polygon.partition(p)` returns `intersecting` where the list form returns `outside` with `p`
  whole. *Run*: 4 of the 4192 large-set cases (`shared#206 seed 1`, `shared#247 seed 1`, each with
  its `jitter1`): the full path's intersection has only collapsed rings (3 and 1 dropped), so it
  answers `GEOMETRY_OUTSIDE`; none in the chain, holes or polyline sets. The pyGPlates test
  `test_geometries_on_sphere.py:1849` itself uses the no-list form.

`PlatePartitioner.partition_geometry` is not affected: the cookie cutter always gives each
partitioner lists (`GeometryCookieCutter.cc:458-472`), so the cascade never takes the result-only
path even when the caller wants only the boolean. Not wrong, but a decision for step 13 (the
`PartitionError` design): either `visit_polygon_on_sphere` takes the full path and lets the exception
propagate (the result-only path is then reached only through `partition_polygon` without lists,
which is 8.5's call; `partition_geometry` with no lists for a polygon costs the extraction it does
not need, which pyGPlates can afford), or the docstring says that the no-list form never raises and
can say `intersecting` where the list form says whole. Amend 9.2, 10.2 and 8.4 either way (section
4).

### 1.2 The pyGPlates docstrings still describe the old behaviour (reading)

`PyGeometriesOnSphere.cc:4488-4494` ("Support for partitioning a polygon geometry is partial ...
partitioned **polylines** (not polygons)") and `PyPlatePartitioner.cc:708-709` (the same warning by
reference). The built module now returns polygons and the tests assert it; the docstrings say the
opposite. Design 8.4 lists their removal and a `.. versionchanged:: 1.1`, and the step-11 "Done"
note covers the tests only; neither PLAN.md step 12 nor 13 names the docstrings and the stub.
Assign them (step 13 with `PartitionError`, presumably) so that they are not lost. The stub
(`:rtype:`) is unchanged by this, so `pygplates-stub-test` cannot catch it.

### 1.3 Minor observations (reading unless marked)

- `Options::clip_speed` (`PolygonOverlay.h:684-685`, "the partitioner's choice") is never set by the
  partitioner: `get_overlay_options` (`PolygonPartitioner.cc:399-415`), `partition_polyline` (`:573`)
  and `partition_polygon` (`:621`, `:635`) use the default. `d_partition_point_speed_and_memory`
  reaches `partition_point` only, as its header comment says (`PolygonPartitioner.h:70-71`).
  Performance only; either plumb it or fix the overlay's comment.
- `partition_polyline` (`:587-595`) and `visit_polyline_on_sphere` (`:370-385`) create a
  `PolylineOnSphere` from each run with no guard on its length. `extract_polylines`
  (`PolygonOverlay.cc:4055-4075`) drops exact repeats, so a run whose stops all share one stored
  position would be one point and `PolylineOnSphere::create` would throw
  `InvalidPointsForPolylineConstructionError`. Not seen in 7239 run pieces (*run*, the committed and
  uncommitted polyline sets); I am not sure it can happen (two stops with bitwise-equal positions).
  A one-line guard, or an assertion, would settle it.
- The test helper `partition` (`PolygonPartitionerTest.cc:152-178`) says it checks "that the
  geometry-level entry gives the same pieces"; it compares the list sizes and the result.
- `#undef DIFFERENCE` (`PolygonOverlay.h:45-51`): right for every present translation unit (the
  tests include gtest, which includes `<windows.h>`, first). A unit that includes a Windows header
  *after* this one and then names `Overlay::DIFFERENCE` would still break; the comment says as much.
- Style: hard tabs, Allman braces and broken signatures throughout. About 30 added lines exceed 100
  columns (most in `PolygonPartitioner.cc` and the test: the `boost::optional<... &>` parameters,
  the `?:` provenance expressions at `:282`, `:383`, `EXPECT_EQ` lines at
  `PolygonPartitionerTest.cc:191`, `:213`, `:241`, `:267`, `:275`); the header's long signature lines
  match the old file. Comments were checked against the code; all say what it does (1.2 and the
  helper above excepted).

## 2. Test gaps

*Run*: nine mutations, one at a time, each rebuilt and run against the committed
`PolygonPartitionerTest` (8 tests) and against the probe tests described below. The session's eight
were not repeated.

| Mutation | Committed suite | Probe |
| --- | --- | --- |
| M1 `GeometryCookieCutter::partition`: `partitioning_polygons` reversed before use (`:398-403`) | passes | passes (not observable: only `provenance_clip_mismatches` can change, 7.2 as built) |
| M3 `input_point_provenance` ignores the input table (`:422-425`): a multi-point's second cut loses its composition | passes | caught |
| M4 `visit_polyline_on_sphere`: a run's table dropped (`:381-383`) | passes | caught |
| M7 `overlay_polygon`: intersection empty answers `GEOMETRY_INSIDE` (`:116-120`) | passes | passes |
| M9 `is_outside_by_bounds`: the subject's `is_outside_bounding_small_circle_outside_polygon` dropped (`:81-82`) | passes | passes |
| M10 `IdentityProvenance`: interior ring `i` named ring `i` instead of `i + 1` (`:490`) | passes | caught |
| M22 `get_polygon_provenance`: interiors' tables appended in reverse order (`:179-185`) | passes | passes the hand probe (one hole per piece); caught by the `holes` set probe (60 of 885 pieces fail to resolve) |
| M27 `GeometryCookieCutter::partition`, no partitioning polygons: the identity table dropped (`:378-383`) | passes | caught |
| M29 `visit_multi_point_on_sphere`: a whole multi-point put in the other list (`:265-269`) | passes | passes |

So all nine survive the committed suite; the plan's "seven of eight mutations fail a committed test"
is true of its eight and says less about the suite's reach than it sounds. Inputs and tests to add:

1. **Other geometry types through the cookie cutter with provenance** (M3, M4, M10, M27). No test
   sends a multi-point, a polyline or a point through `GeometryCookieCutter::partition_geometry` with
   `want_provenance`, and none asks a cutter with no partitioning polygons. The probe that caught
   these: the cascade test's three plates; a five-point multi-point with points in A, B, far, outside
   and A again (expected tables `{0, 4}`, `{3}`, `{1}`, `{2}`, as indices into the input); a
   four-vertex polyline from outside across A and B to outside (four pieces, every point resolving
   with a subject location); a point (table of one); and an empty cutter given a polygon with a hole
   (identity table of eight, ring `i / 4`, index `i % 4`).
2. **A piece keeping two or more holes, with provenance** (M22). `CookieCutterCascade`'s pieces have
   none ("the hole is cut open") and the provenance tests use boxes. A subject with two holes on one
   side of the cut, through `partition_geometry(PartitionedGeometry(P), ..., CascadeProvenance())`,
   with every hole vertex named by its ring and index and `count_provenance_failures` zero.
3. **A whole multi-point and a whole point** (M29): no C++ test returns a multi-point whole; the
   pyGPlates tests partition a split one only (`test_geometries_on_sphere.py:1812-1826`). Assert the
   object identity, as `TouchesComeBackWhole` does for polygons.
4. **A collapsed region** (M7, and the emptiness rule of `:115-125` as a whole): no test input has
   one, so `GEOMETRY_INSIDE`/`OUTSIDE` from an empty extraction is untested in both directions, and
   so is the result-only path's documented difference. `shared#206 seed 1` and `shared#247 seed 1`
   (large set) are such inputs: commit one as a hand case, or derive one (two boxes sharing an edge,
   the subject overlapping the clip by less than `h`).
5. **A polygon whose interior reaches past its outline's circle** (M9), as subject and as
   partitioner, with the other polygon inside the far cap. The interior rule's `wrap_around_band`
   (`spherical-geometry.md:1410`) is such a polygon. A thin belt round 300 degrees of the equator is
   *not* one: *run*, both orientations have the belt as interior and pass
   `is_outside_bounding_small_circle_outside_polygon`, so that shortcut case is right by the
   function's contract, not by any test.
6. **The catch on the result-only path** (`:624-627`): no test input is inconsistent, so removing it
   survives (certain by reading: every committed polygon passes the list form, which would throw).
   The two known bundle-vertex failures (`shared#1434 seed 12`, `#2587 seed 13`, uncommitted) would
   pin it: `partition_polygon(P)` is `GEOMETRY_INTERSECTING` and the list form throws.
7. `partition_polygon` with one list only (`:650-663`) is untested, and the helper `partition`
   compares the geometry entry's pieces by count only (1.3).

## 3. Checked and found correct

- **Point order and the tables** (brief item 1): `create_polygon` (`:155-169`) passes the exterior
  then `interiors` in order; `PolygonOnSphere::create` (`PolygonOnSphere.h:2271-2311`,
  `generate_rings_and_swap` `:2330-2360`, `generate_ring` `:2364`) keeps every point and every ring
  in order (no distinctness check, only antipodal segments rejected);
  `GeometryUtils::get_geometry_points` is the polygon's
  `vertex_begin()` order, exterior then interiors (`GeometryUtils.cc:101-129`);
  `get_polygon_provenance` (`:173-187`) is `exterior.provenances` then each interior's. *Run* at
  scale as in the baseline: every table the right length, every vertex exact. The overlay's
  `extract` reverses provenances with points on orientation (`PolygonOverlay.cc:3967-3968`), and
  clean-up pops both together (`:3761-3782`).
- **The bounding-circle shortcut** (`is_outside_by_bounds`, `:68-83`; brief item 2). Disjoint
  circles mean disjoint outlines; a filled polygon can still contain the other only if its interior
  reaches past its own outline's circle, which `is_outside_bounding_small_circle_outside_polygon`
  (`PolygonOnSphere.cc:846-868`: the outside point outside the circle, else the antipode of the
  centre tested by point-in-polygon) excludes for both polygons, and for the partitioner alone with
  a polyline subject (`:140`), whose "interior" is its outline. The polygon's circle bounds every
  ring (`SmallCircleBounds.h:558-566` adds the interiors). `intersect` (`SmallCircleBounds.h:288-296`)
  compares the centres' angle with the sum of the extents, `is_precisely_less_than`, no
  epsilon: two outlines within `h` of each other but with disjoint
  circles get `OUTSIDE` here where the overlay would make a touch record and, by 5.3, also answer
  `OUTSIDE` (or, on the result-only path, `INTERSECTING` for a collapsed sliver: the shortcut is the
  less conservative of the two, by at most `h`). *Run*: `ResultOnlyPath`'s far box, and the belt
  probe above.
- **The result-only path** (brief item 3). `classify_pieces(true)` (`PolygonOverlay.cc:2246-2271`)
  stops only at `INTERSECTING`, after the vertex tests, when a subject piece is inside and another
  outside or a clip piece is inside; those tests are at interior original vertices, at least `h`
  from the other outline, so they are exact. Otherwise the code is the full path's up to
  `get_result_from_classes` (`:2510-2561`), which both paths share, and the full path can only
  *demote* `INTERSECTING` to whole after extraction (`PolygonPartitioner.cc:115-125`). So no input
  gets `INSIDE` or `OUTSIDE` here where the full path gives pieces on both sides; *run*, 0 of 6591
  polygon cases, and 0 polyline disagreements (a polyline has no early stop and `classify`'s polyline
  branch, `:1981-1996`, throws nothing: the throws are at `:2498`, `:3806`, `:3896`, all polygon
  classification or extraction). A stopped overlay cannot extract (`:3300-3304`). The catch
  (`PolygonPartitioner.cc:619-627`) covers the constructor and `classify`.
  `GLReconstructedStaticPolygonMeshes.cc:962-985`: unchanged; its own containment test after
  `OUTSIDE` is now unreachable for the case it was written for (a node surrounding the mesh polygon
  is `INTERSECTING` by the clip-inside stop) and harmless.
- **The cascade's provenance** (brief item 4). `clip_index` is the iterator's offset into
  `d_partitioning_geometries` (`GeometryCookieCutter.cc:436`), which the constructors sort before
  any call (`sort_plates_in_partitioning_group`, `:705-733`, from `:559`, `:582`, `:644`, `:678`
  only); `partitioning_polygons` (`:395-400`) is built in the same order, so a table's clip index
  and `cascade_clips[index]` name the same plate. A whole piece keeps its table (`add_whole`,
  `:444-451`: the input's, else the identity of the current geometry, which is the original exactly
  when there is no table). The no-polygons return gives the identity (`:373-385`). The multi-point
  path composes by index (`:418-429`, `:253`), and a multi-point is one ring. `want_provenance` and
  the tables agree on every path: every `push_back` in `GeometryPartitioner` is gated on
  `d_cascade_provenance` (`:281-283`, `:339-341`, `:381-383`, `:444-450`), and the overlay fills
  tables whenever `options.want_provenance` (`:405`). *Run*: `CookieCutterCascade` both ways and
  the probes of section 2.
- **The composition's new inputs** (`compose_subject_location`, `PolygonOverlay.cc:4312-4330`,
  `:4380-4415`): the wrap-around's last segment from the original polygon's ring sizes
  (`x.ring <= number_of_interior_rings()` guards the index); a polyline or multi-point original gives
  no wrap (the `dynamic_cast` is null), right for both; the clip branch indexes `cascade_clips` by
  `a.clip_index` after checking `a.clip_index == b.clip_index` and the bound. *Run*: 13689
  second-step pieces through the partitioner, 0 failures.
- **Behaviour changes** (brief item 5), all intended by 5.3 or 8.1: pyGPlates `partition` and
  `partition_geometry` return polygons for a cut polygon, a touching or along-the-edge geometry
  whole, and a whole multi-point or point as the input itself (*run* from Python: equality holds;
  Boost.Python makes a new wrapper either way, so `is` was never true); Assign Plate IDs partitions
  polygons as polygons and can now see `InconsistentOverlayException` (step 12); CitcomS no longer
  references `PolylineIntersections` (nothing in `src/` does); the result-only path's catch is the
  only new swallow (1.1). `GeometryCookieCutter`'s other users (`AssignPlateIds`,
  `PlateVelocityUtils`, `PyReconstructSnapshot`, the partition tasks) call `partition_point` only.
  A coverage still does not travel through `partition_into_plates` for a cut polygon (it never
  did: the pieces were polylines); step 12.
- `visit_point_on_sphere` (`:290-299`) and `visit_multi_point_on_sphere` (`:228-286`): a boundary
  point is inside, as before; `any_intersecting` cannot be set (`partition_point` never returns
  `INTERSECTING`), kept for the same reason `partition_multipoint` keeps it.
- `partition_polyline` and `partition_polygon` whole returns push the input object (`:576-585`,
  `:639-648`); the geometry entry's `add_whole` does the same.
- Design 8.4's claim about `test.py:2131`'s polygon: its closing edge from `(30, -90)` to `(0, 0)`
  crosses the `-30` meridian at `16.1` degrees north (by hand), inside that edge's `0` to `30`
  range, so the input does cross itself and the area statement is right.
- `GeometryIntersect.h:358-360`'s comment, the dependency matrix and the CMake lists match the
  deletion; `pygplates-source-closure-test` passes.

## 4. Design statements to amend

- **9.2 and 10.2** ("the one caller of that path", "the conservative answer for the one caller"):
  pyGPlates `PolygonOnSphere.partition` with no lists is a second caller (1.1); say what it gets, or
  change it in step 13.
- **8.4**: the docstring warnings and the `versionchanged` are not done (1.2); name the step. The
  "Done in step 11" note should say "tests only".
- **5.3** ("with an identity provenance table"): as built, only when a table is asked for, and the
  input's own table when it has one (7.2 as built says this; 5.3 does not).
- **4.1 as built** ("each polygon's interior lies within its own circle"): for a polyline subject the
  check is the partitioning polygon's alone (`:140`), which is enough.
- **10.2 as built**, "its no-record case needs no code of its own": true, and worth one more clause:
  with no records every ring is one closed piece with an interior vertex, so all are classified in
  step (a) and the early stop or `get_result_from_classes` answers at once.
- **PLAN.md step 11**, "seven of eight mutations fail a committed test": add that nine further
  mutations survive it (section 2), so the suite pins the polygon path and the cascade's polygon
  pieces, not the other geometry types, holes in a piece's table, or the empty-extraction rule.
- **`Options::clip_speed`'s comment** ("the partitioner's choice"): the partitioner makes none (1.3).
