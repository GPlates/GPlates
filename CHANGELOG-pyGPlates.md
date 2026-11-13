pyGPlates Changelog
===================

Changes in each pyGPlates release, newest first. Changes to GPlates (the desktop application) are listed in `CHANGELOG-GPlates.md`.

pyGPlates 1.1 (unreleased)
==========================

Changes since 1.0.0:

* Supports Python 3.9 to 3.15 (Python 3.8 is no longer supported).
* Type stub included in the package (PEP 561), so editors and type checkers get completion, signature help and type checking for the whole API.
* The API reference documents every enumeration (eg, `PartitionMethod`) and exception (eg, `InvalidLatLonError`) on its own page, and parameter types and raised exceptions now link to those pages.
* Pickling is much faster (eg, a `RotationModel` pickles about 7 times faster and unpickles about 5 times faster).
  * Pickles written by pyGPlates 1.0 can still be loaded, but pickles written by 1.1 cannot be loaded by 1.0.
  * Fixed pickling an object that had extra attributes added to it from Python (previously raised "Incomplete pickle support").
* Added `TopologicalSnapshot.reconstruct_points()` to incrementally reconstruct points (lying within the snapshot's resolved plates and networks) to another time.
* Added `ResolvedTopologicalNetwork.get_geometry_sub_segments()` (the same as `get_boundary_sub_segments()`), so that all three resolved topology types now have it, as resolved lines and boundaries already did.
* `GeometryOnSphere.distance()` can be called on any geometry (eg, `point.distance(polygon)`), as well as on the class as before (`GeometryOnSphere.distance(point, polygon)`).
  * Its arguments `geometry2`, `geometry1_is_solid` and `geometry2_is_solid` are renamed `other_geometry`, `geometry_is_solid` and `other_geometry_is_solid`. The old names (including `geometry1`) still work, but not mixed with the new ones in the same call.
  * Calling it on a geometry with two other geometries (eg, `geometry.distance(point, polygon)`, which ignored `geometry`) no longer works.
* `Vector3D.angle_between()`, `Vector3D.dot()` and `Vector3D.cross()` can likewise be called on a vector (eg, `vec1.dot(vec2)`), as well as on the class as before (`Vector3D.dot(vec1, vec2)`).
  * Their argument `vector2` is renamed `other_vector`. The old names (`vector1` and `vector2`) still work, but not mixed with the new one in the same call.
  * Calling them on a vector with two other vectors (eg, `vec.dot(vec1, vec2)`, which ignored `vec`) no longer works.
* Deprecated converting at a single origin with the `LocalCartesian.convert_...()` functions (eg, `convert_from_geocentric_to_north_east_down(local_origin, vector)`). They still work, but use the matching method of a `LocalCartesian` at that origin instead (eg, `LocalCartesian(local_origin).from_geocentric_to_north_east_down(vector)`). The `convert_...()` functions that convert a sequence of vectors, each at its own origin, are unchanged.
* Type stub: arguments that take an (x,y,z) vector or a (magnitude, azimuth, inclination) tuple (eg, in `Vector3D.dot()` and `LocalCartesian`) now accept a tuple or list of floats. The stub expected a sequence of tuples, so type checkers rejected correct calls.
* Type stub: some forms that still work, for compatibility, aren't in the stub, so type checkers report them as errors. These are the old argument names of `GeometryOnSphere.distance()` and `Vector3D.angle_between()`/`dot()`/`cross()`, converting at a single origin with `LocalCartesian.convert_...()`, and calling `Vector3D.angle_between()`/`dot()`/`cross()` on the class with an (x,y,z) tuple as the first argument.
* `GpmlTopologicalSection.create()` accepts a feature whose geometry is a polygon (returning a line section), as the topology building tools in GPlates do.
* `GpmlTopologicalSection.create()` and `create_network_interior()` take a `property_return` argument: with `PropertyReturn.first` they reference a feature that has more than one geometry with the same property name, instead of returning `None`.
* Added an `anchor_plate_id` argument to `PlatePartitioner` and `partition_into_plates()` (it defaults to the rotation model's default anchor plate, which was previously always used):
  * Created from plate features, it reconstructs or resolves them with that anchor plate (as does `partition_into_plates()`).
  * Created from plates already reconstructed or resolved, it should be the anchor plate they used (eg, `TopologicalSnapshot(..., anchor_plate_id=701)`), so that `partition_features()` reverse reconstructs the partitioned features with it.
* Fixed `Feature.get_geometry()` and `Feature.get_topological_geometry()` with `PropertyReturn.first` when nothing matches: they now always return `None` as documented (previously undefined behaviour, which could crash on some platforms).
* Fixed `GpmlTopologicalSection.create()` and `create_network_interior()`, without a geometry property name, for a feature whose type has no default geometry property name (not in the information model): they now return `None` (previously undefined behaviour).
* A scalar coverage on a polygon with interior rings has a scalar value for every point of every ring, in the order of `PolygonOnSphere.get_points()`. `Feature.set_geometry()` already required that, but `Feature.get_geometry()` (with `CoverageReturn.geometry_and_scalars`) then didn't find the coverage, since it expected values for the exterior ring only.
  * Files from earlier versions with values for the exterior ring only can still be read: `Feature.get_geometry()` returns that coverage's geometry as a polygon of the exterior ring alone.
* Fixed `Feature.set_geometry()` with a coverage that has the wrong number of scalar values, a property name that doesn't support coverages, or two coverages with the same number of points: it raises without changing the feature (previously the new geometry was set first, leaving it with the old scalar values).
* `PolygonOnSphere.partition()`, `PlatePartitioner.partition_geometry()`, `PlatePartitioner.partition_features()` and `partition_into_plates()`:
  * A polygon that intersects a partitioning polygon or plate is now partitioned into polygons (with interior rings where needed), not polylines.
  * The interior rings of both polygons take part (previously only exterior rings were intersected). For example, a partitioning polygon wholly inside a polygon is now its own piece inside, leaving a hole in the piece outside (previously the whole polygon was outside).
  * A geometry that only touches a partitioning polygon, or runs along its outline, is no longer split there: it is returned whole, inside or outside (a polyline along the outline is inside).
  * Where a polygon's rings cross each other or themselves (eg, an interior ring crossing the exterior ring, or a bow-tie), in either polygon, the pieces follow `PolygonOnSphere.is_point_in_polygon()` (which reads the rings by parity): rings of a piece that cross each other are kept in one polygon, so the pieces inside and outside don't overlap (but, rarely, within a few hundred metres of where the rings meet). An interior ring of such a piece can lie partly or wholly outside its exterior ring; the piece is not dropped as a sliver, and its area (`PolygonOnSphere.get_area()`, and so for `PartitionMethod.most_overlapping_plate`) is its exterior ring's less its interior rings' (for a bow-tie, the two lobes' areas cancel).
  * Raise the new `PartitionError` if a cut is inconsistent (rare: the only case found is two polygons that each have a hole crossing their exterior ring, with outlines running within about 9 metres of each other along many closely spaced vertices).
* `PlatePartitioner.partition_features()` and `partition_into_plates()` (with `PartitionMethod.split_into_plates`) carry each geometry's scalar coverage to its pieces, as GPlates' Assign Plate IDs does (previously the coverage was dropped): values at the original's points are kept, values where a piece's outline crosses the original's are interpolated along its edge, and values along a cut are interpolated between the cut's ends. Each piece with a coverage is returned in a feature of its own, so that no two coverages share a property name in one feature. A coverage from an older file, with values for a polygon's exterior ring only, keeps that form.
* `PlatePartitioner` and `partition_into_plates()` drop the slivers a cut can leave (pieces of a polygon narrower than about 90 metres, where its outline was digitised along a plate boundary), rather than assigning them the neighbouring plate. The new `sliver_width_threshold_radians` argument sets the width, and zero keeps every piece. `PolygonOnSphere.partition()` takes the same argument, defaulting to about 9 metres (the distance within which two outlines count as touching).
* `PlatePartitioner.partition_features()` and `partition_into_plates()` (with `PartitionMethod.split_into_plates`) partition time-dependent geometry as GPlates' Assign Plate IDs does. A geometry in a single time window that contains the reconstruction time is partitioned like a constant geometry, and its pieces are set as constant geometry. Any other time-dependent geometry is kept unchanged on every feature returned (previously its window at present day was partitioned and its other windows were lost), and with `PartitionMethod.most_overlapping_plate` it no longer decides the plate (previously its window at present day did, whatever the reconstruction time).
* `PartitionMethod.most_overlapping_plate` compares the area of polygons inside each plate (previously the length of their outlines, which the cuts inflate), then the length of polylines, then the number of points.
* Documentation sample code:
  * Every sample is now a Python 3 script that the test suite runs, so it can be copied and run as shown (many samples previously used Python 2 `print` statements).
  * Samples use `ReconstructSnapshot` and `PlatePartitioner` instead of `reconstruct()` and `partition_into_plates()` (except where a one-off partition is clearer).
  * Each sample page lists the data files it needs and where to get them, and links to related Primer sections, Reference entries and samples.
* Resolving topologies, and partitioning a polyline with `PolygonOnSphere.partition()` or `PlatePartitioner`, now treat two geometries less than about 9 metres apart as touching (previously about 6 metres), the same distance within which `PolygonOnSphere.is_point_in_polygon()` counts a point as on the outline. A few plate boundary junctions therefore move onto a nearby vertex of a boundary section, typically by less than 10 metres.
* Installation:
  * The pip wheels and the conda package no longer depend on Qt GUI or OpenGL libraries.
    * So pyGPlates imports on a minimal Linux system (no `libGL` or `glib` packages needed) and the packages are smaller.
  * Linux wheels require glibc 2.28 or later (manylinux_2_28), instead of glibc 2.17 (manylinux2014).
  * Wheels on all platforms bundle the same PROJ (9.8.1) and GDAL (3.12.4) versions.
  * Requires NumPy 1.19 or later, and `pip` now enforces it: installing pyGPlates upgrades an older NumPy (previously the oldest NumPy that worked depended on the Python version, and an older one failed `import pygplates`).
* Documentation: corrected the supported Python versions and platforms in *Getting started*, fixed the introductory examples (which raised `NameError`), and documented the Python console in GPlates.
* Documentation: methods a class inherits (eg, `distance()` listed on the `PointOnSphere` page) link to their documentation in the base class (previously they were listed without a link).
* Documentation: the *Primer* is split into one page per topic (rotations, topologies and deformation). Links to sections of the old single-page Primer still work, redirecting to the section on its new page.
* Removed the undocumented classes `Colour`, `Palette`, `PaletteKey`, `OldFeature` and `OldFeatureCollection` (they were only meaningful inside GPlates).
* Removed `namedtuple`, `partial`, `math`, `itertools`, `numpy`, `iteritems`, `itervalues`, `listitems` and `listvalues` from the `pygplates` module. They were internal imports and Python 2 helpers, exposed as `pygplates.<name>` by accident; import them from the standard library (or `numpy`) instead.
* A flowline now spreads from its geometry import time, like a `HalfStageRotationVersion3` mid-ocean ridge (see `Feature.create_flowline()`). So changing its times doesn't shift it (at any reconstruction time it stays where it was, only gaining or losing points), the pieces of a flowline with the same seed point and geometry import time join up (as for motion paths), and rotations younger than its geometry import time don't change it at that time.
  * This is exact unless a plate's rotation changes pole between two of the flowline's times, or outside them. There the flowline depends on which times it has (slightly between them, more so far outside them), since each step of its spreading uses only the rotations at its two ends.
  * Reverse reconstructing a flowline's seed point (eg, `Feature.create_flowline(..., reverse_reconstruct=...)`, `Feature.set_geometry(..., reverse_reconstruct=...)` or `reverse_reconstruct()`) now sets its geometry import time, as it does for other features. Otherwise `Feature.create_flowline()` sets it to the youngest time.
  * A flowline without a geometry import time spreads from its youngest time, as before, so existing flowlines stay where they were at every reconstruction time. `Feature.set_times()` first gives such a flowline a geometry import time at its old youngest time.
  * Shapefile, GeoJSON, GeoPackage and OGR-GMT files don't store a flowline's times, but do store its geometry import time (`IMPORT_AGE`). Flowlines written to them now store the time they spread from (their youngest time if they have no geometry import time), so once given times again they don't shift. Earlier versions wrote no value for a flowline without one, which OGR-GMT files read back as 0: such a flowline (or one later written to GPML) now spreads from present day once it has times again, so unless its youngest time is 0 it is shifted, at every reconstruction time, from where earlier versions put it. Removing its geometry import time (`feature.remove(pygplates.PropertyName.gpml_geometry_import_time)`) puts it back there.
  * GPML files saved by pyGPlates 1.1 record information model version 1.6.0340. pyGPlates 1.0 reads them, but ignores a flowline's geometry import time.
* A polygon's interior is now the smaller side of each of its rings (the rings combining as before: a point inside an odd number of them is inside), so a polygon that wraps round the globe (eg, a C-shaped landmass made by merging continents) is no longer inside out. Previously the side away from the antipode of `PolygonOnSphere.get_boundary_centroid()` was inside, which for such a ring could be the larger side. This applies to `PolygonOnSphere.is_point_in_polygon()` and what uses it (eg, `partition()`, `PlatePartitioner`, `TopologicalSnapshot.get_point_locations()` and `GeometryOnSphere.distance()` with a solid polygon), and to `get_area()`, `get_signed_area()`, `get_orientation()` and `get_interior_centroid()`. Polygons that were already right are unchanged.
  * A ring that encloses more than half the sphere now means the region outside it.
  * A ring that halves the sphere (eg, four points on the equator) has its interior to its left.
  * Fixed `GeometryOnSphere.distance()` with a solid polygon and a threshold returning `None` for a point inside a polygon that wraps round the globe.
* `PolygonOnSphere.get_interior_centroid()` (and `get_centroid()`) is now exact for a polygon wider than about 0.01 radians. It was off by up to about 0.3 degrees for a polygon one radian across.
* Bug fixes:
  * Fixed `PlatePartitioner.partition_features()` and `partition_into_plates()` (with `PartitionMethod.split_into_plates`) dropping a feature with no geometry: it is now returned unpartitioned, as with `PartitionMethod.most_overlapping_plate`.
  * Fixed `GeometryOnSphere.distance()` returning inconsistent closest positions and segment indices when two polylines/polygons intersect more than once.
  * Fixed `GeometryOnSphere.distance()` for geometries exactly PI apart (eg, antipodal points): a threshold of PI returned `None`, and with `return_closest_indices=True` a multi-point, polyline or polygon returned an undefined index.
  * Fixed `GeometryOnSphere.distance()` returning `None` for intersecting polylines/polygons when the threshold was zero (or very nearly zero).
  * Fixed `GeometryOnSphere.distance()` sometimes returning `None` when the threshold equals the distance (eg, a distance it returned earlier, passed back as the threshold, or a threshold of zero for geometries it reports as zero distance apart). A NaN threshold now raises `ValueError` (it was ignored).
  * Fixed `GeometryOnSphere.distance()` returning the closest position of one candidate and the closest index of another, equally close, candidate (eg, two points of a multi-point equally close to a polyline).
  * Fixed `GeometryOnSphere.distance()` returning, for polylines/polygons that cross at a very shallow angle, a closest position that could lie slightly off one of them (up to about a metre for long segments).
  * Fixed `GeometryOnSphere.distance()` ignoring the last point of a polyline whose last segment is very short (under about 6 metres), so the distance could be up to that much too large.
  * Point-in-polygon tests: `PolygonOnSphere.is_point_in_polygon()`, and what uses it (eg, `PolygonOnSphere.partition()`, `PlatePartitioner` and `TopologicalSnapshot.get_point_locations()`):
    * A point within about 9 metres of a polygon's outline, including at a vertex, is now always inside, as intended. Some were outside, so a point on a boundary shared by two plates could be in neither.
    * Fixed a point beyond the end of one of a polygon's edges, but near the line of that edge, counting as inside up to about 640 metres outside the polygon.
    * Fixed a point near the antipode of a tiny, thin or self-intersecting polygon counting as inside, but only for the first few points tested against that polygon (also in `GeometryOnSphere.distance()` with a solid polygon). The answer no longer depends on how many points a polygon has been tested with.
  * Fixed `PolygonOnSphere.get_orientation()` for some concave polygons: it could be wrong depending on which vertex came first (unless `get_signed_area()` had been called first). This also affected which side of a shared plate boundary each plate is on (`ResolvedTopologicalSharedSubSegment.get_sharing_resolved_topology_on_left_flags()` and `TopologicalSnapshot.calculate_plate_boundary_statistics()`), which side of a polygon `PolygonOnSphere.partition()` put the pieces of a polyline crossing it, and which polygons `ReconstructSnapshot.export_reconstructed_geometries(force_polygon_orientation=...)` and `TopologicalSnapshot.export_resolved_topologies(force_boundary_orientation=...)` reversed.
  * Fixed a rare "function 'sqrt' invoked with invalid argument" error (from a rounding error when calculating angular extents).
  * A malformed `gml:pos` or `gml:coordinates` in a GPML file is now dropped as a read error instead of silently loading as a point at (0, 0).
  * Fixed crustal thinning factors in GPML files written before GPlates 1.6.338 not being upgraded when loaded.
  * Fixed saving rotation files in GROT format under paths containing non-ASCII characters on Windows.
  * GROT (`.grot`) rotation files:
    * Fixed disabled poles in a written file being read back as enabled poles of plate 999.
    * Fixed reading a file never finishing when a multi-line (`"""`) attribute is not closed (as written for disabled poles with multi-line metadata).
    * Fixed a `"""` attribute that opens and closes on one line swallowing the line after it (often a pole).
    * Fixed multi-line pole metadata gaining blank lines each time it is written.
    * Fixed a pole line that is ignored when reading (eg, `999 0.0 0.0 0.0 0.0 999`) adding the pole before it a second time.
  * Reading a file in a format that can only be written (`.xy`) now raises `FileFormatNotSupportedError`, instead of returning an empty feature collection. Likewise writing a format that can only be read (`.vgp`, `.gsml`), which wrote nothing.
  * Fixed `reverse_reconstruct()` emptying a `.xy` file given as a filename (it read the file as empty, then wrote that back). It now raises `FileFormatNotSupportedError` for a file it can't read, or can't write back (`.vgp`, `.gsml`).
  * Writing a Shapefile, GeoJSON, GeoPackage or OGR GMT file that would contain no geometries (eg, only topological features) now raises `GPlatesError`, instead of writing no file (and deleting any existing file of that name).
  * Writing points and multi-points together to OGR formats (also when exporting reconstructed geometries):
    * Fixed GeoJSON and OGR GMT raising "Error creating OGR layer" (and leaving some files written).
    * Fixed the multi-points of a GeoPackage file being lost when read back.
    * Multi-points written with other geometry types now always go in their own `<name>_multi_point` file (previously `<name>_point` when there were no points).
    * An export of points and multi-points goes in a `<name>` folder, like any other export of several geometry types (previously a Shapefile export put `<name>.shp` and `<name>_multi_point.shp` side by side).
  * Fixed rewriting an OGR GMT file of several geometry types deleting files of the same names (eg, `<name>_point.gmt`) in the current working directory.
  * Fixed the macOS pip wheels crashing (segmentation fault) whenever pyGPlates logged a warning or debug message, eg when writing a `.grot` file, or reading a rotation file with an invalid pole.
  * Velocities and net rotation:
    * `NetRotationModel()` can be created without `velocity_delta_time` and `velocity_delta_time_type`, which default to 1 Myr and `VelocityDeltaTimeType.t_plus_delta_t_to_t` as documented (previously raised `ArgumentError`).
    * An empty `point_distribution` sequence now raises `ValueError` (previously `PreconditionViolationError`).
    * Fixed the net rotation of a rigid plate being far too large at the oldest rotation of its plate ID (eg, about 100 times too large for a plate whose rotations end at 100 Ma). The velocity time interval now moves where a plate has no rotation at one end of it, as it already did for the velocities of plates: so at present day, `t_to_t_minus_delta_t` and `t_plus_minus_half_delta_t` also give the net rotation of rigid plates (previously none, or half).
    * Fixed the velocities of mid-ocean ridges, flowlines and other geometries reconstructed by half-stage rotation in the same way: they were zero at present day with `t_to_t_minus_delta_t`, half as large with `t_plus_minus_half_delta_t`, and far too large at the oldest rotation of either plate.
    * Every function and method that calculates velocities now raises `ValueError` if `velocity_delta_time` (or the `time_interval_in_my` of `calculate_velocities()`) is zero or negative. Only some did; the others returned zero velocities or meaningless ones (`calculate_velocities()` returned infinite velocities for zero).
    * Strain rates in deforming networks use a year of 365.25 days, like the strain accumulated from them (previously 365 days). So strain rates are about 0.07% smaller, and accumulated strains (eg, from `ReconstructedGeometryTimeSpan.get_strains()`) are no longer about 0.07% too large.
    * Fixed velocities in the deforming region of a network pointing into or out of the globe: they are now tangential to it. The radial part is mostly well under 1% of the velocity (in the Müller et al. 2019 model), but is larger where network vertices are far apart. This also corrects the velocity magnitudes and obliquities of plate boundary statistics on the network side of a boundary, and can change which points `TopologicalModel` deactivates as they cross between a network and a rigid plate.
  * Flowlines whose times don't start at present day, or whose left and right plates don't coincide at their first time (eg, present-day rotations that aren't zero):
    * Fixed the right flowline (`ReconstructedFlowline.get_right_flowline()`) not starting at the seed point.
    * Fixed `reconstruct()` returning no flowline at times younger than the first time. Each half of the flowline now starts at that time, and the reconstructed seed point is where the spreading centre is.
    * Fixed `ReconstructedFlowline.get_reconstructed_seed_point_velocity()` not matching how the reconstructed seed point moves, which it now does at all times (previously it could differ by a few percent between widely spaced times).
    * Fixed reverse reconstructing a flowline's seed point at a time older than its oldest time jumping to a different position from just younger than that time. A flowline now keeps spreading at times older than its oldest time (and younger than its youngest), which needs rotations of both plates there: set its valid time if they aren't spreading then.
  * Fixed exported motion paths (eg, `.xy`, Shapefile, GeoJSON) missing the feature's name, which exported flowlines include.
  * `Feature.create_motion_path()` no longer sets a reconstruction method, which a motion path doesn't use (it is reconstructed by its plate IDs).
  * `insert()` on the list-like classes (eg, `GpmlIrregularSampling`, `GpmlTimeSampleList`) clamps an out-of-range index to the start or end, like `list.insert()` (previously raised `IndexError`).
  * Fixed `GpmlIrregularSampling.get_value()` returning `None` at the time of the only enabled time sample, and (for an `XsDouble`) where two time samples have the same time. Likewise `get_time_samples_bounding_time()` now returns the only enabled time sample (as both samples) at its time. Times are compared with a small tolerance (eg, a time sample at 0.3 is found at 0.1 + 0.2).
  * Fixed `InformationModelError` giving the wrong one of two messages: adding a property the information model recognises, but not for the feature type, said the property name was not recognised (and adding an unknown property name said it was not valid for the feature type).
  * `synchronise_crossovers()` writes the rotation files it was given only if it synchronised a crossover (previously it always rewrote them, changing their layout even when no rotation changed).
  * `TopologicalSnapshot.get_resolved_topological_sections()`, unless sorted with `same_order_as_topological_features=True`, returned the sections in an order that could change from one run of a script to the next. They are now in the order they are first found along the boundaries of the resolved topologies.
  * `TopologicalSnapshot.calculate_plate_boundary_statistics()` returns its statistics in the order of the topological features, and then along each boundary section (previously the order could change from one run of a script to the next).
  * `NetRotationSnapshot.get_net_rotation()`, without a resolved topology, returns its `dict` in the order of the resolved boundaries and then networks (previously the order could change from one run of a script to the next).
  * Fixed the pip wheels loading the GDAL plugins named by a `GDAL_DRIVER_PATH` environment variable (eg, set by an active conda environment with GDAL, or by OSGeo4W or QGIS). Those plugins are built for another GDAL, and reading a file could crash (eg, a Shapefile after `import pygmt`).
  * Functions that take an iterable (eg, `PolylineOnSphere(points)`) no longer replace an exception raised while iterating it (eg, a `ValueError` raised by a generator, or `KeyboardInterrupt`) with their own `TypeError`: it now propagates unchanged. Likewise, an exception raised while iterating a sequence (eg, the features given to `FeatureCollection()`) now propagates, instead of being left pending (which surfaced as `SystemError`, possibly in unrelated later code).
  * `FeaturesFunctionArgument.contains_features()` returns `False` for an iterator, including one nested in a sequence (previously raised `TypeError`).
  * A predicate passed to `Feature.get()`, `Feature.get_value()`, `Feature.get_geometry()` (and the other query methods of `Feature` and `FeatureCollection`), `Feature.remove()`, `FeatureCollection.remove()`, or as the `boundary_section_filter` of `TopologicalSnapshot.calculate_plate_boundary_statistics()`, can return any value, which is tested for truth as by `if` (previously only a `bool` or `int` worked: a `str`, `list` or `numpy.bool_` raised `TypeError`).
  * `FeatureCollection.get()`, and `Feature.get()`, `get_value()`, `get_geometry()`, `get_geometries()` and `get_topological_geometry()`, raise `TypeError` for a query of the wrong type (eg, the string `'gml:name'` instead of a `PropertyName`), as `remove()` already did. Previously they raised "'str' object is not callable", or returned `None` (or `[]`) when there was nothing to query.
  * `FeatureCollection.remove()` and `Feature.remove()`:
    * Fixed a list naming the same feature (or property) instance more than once, not next to each other (eg, `[f1, f2, f1]`), raising `ValueError` after removing the others.
    * When `ValueError` is raised (an instance not found), nothing is removed (previously some of the others could already have been removed).
    * A feature added to a collection more than once (`FeatureCollection.add()` doesn't check for duplicates) has every copy removed (previously only one copy, so it was still in the collection, and was written to files).
  * `Feature.add()` and `Feature.set()` with several property values (or `Feature.add()` with several property name/value pairs) leave the feature unchanged when one of them raises `InformationModelError` (eg, a value of the wrong type, or two values of a property that can occur only once). Previously the values before it had already been added or set.
  * `Feature.get_string()`, `get_double()`, `get_integer()` and `get_boolean()` with `PropertyReturn.all` return the *default* when nothing matches, as documented (previously an empty list).
  * Fixed `GpmlKeyValueDictionary()` and `GmlDataBlock()` with a (key, value) pair that is not a tuple or list (eg, an iterator) raising a misleading error (eg, `KeyError: 0`).
  * `PickleBytes()` with something other than `bytes` raises `TypeError` (previously `SystemError`).
  * `Feature.set_times()` raises `ValueError` for a repeated time (eg, `[0, 10, 10, 20]`), as it already did for times out of order.
  * A motion path whose times include a repeated time (eg, from a file) skips it, as flowlines do, rather than repeating a point of the motion path.
  * Fixed iterating over a temporary `GpmlKeyValueDictionary` or `GmlDataBlock` (eg, `[key for key in GpmlKeyValueDictionary({...})]`) reading freed memory (which could crash, raise `RuntimeError`, or return another object's keys).
  * `iter()` on an iterator over a list-like class (eg, `GpmlIrregularSampling`), `GpmlKeyValueDictionary`, `GmlDataBlock` or the edges of a `ReconstructionTree` returns that iterator, as for Python's own iterators (previously a copy, so eg `zip(*[iter(sequence)] * 2)` paired each item with itself).
  * Fixed `reverse()` and `sort()` on an empty list-like object (eg, `GpmlIrregularSampling`, or the list returned by `GpmlIrregularSampling.get_time_samples()`) raising `AssertionFailureError`.
  * The list-like classes (eg, `GpmlIrregularSampling`, `GpmlArray`) behave like a Python list in more cases:
    * `index(x, i, j)` interprets *i* and *j* like slice indices, and raises only `ValueError` (eg, `index(x, 2)` searched the whole list after an index of 0 was treated as the end, an index out of range raised `IndexError`, and so did an empty list).
    * `remove(x)` on an empty list raises `ValueError` (previously `IndexError`).
    * Assigning an empty sequence to an empty extended slice (eg, `a[2:2:2] = []`) does nothing (previously raised `ValueError`), and a slice step of zero raises `ValueError` (previously `IndexError`).
  * `sort()` on the list-like classes (eg, `GpmlIrregularSampling`) now sorts exactly like `list.sort()`: it is stable with `reverse=True` (equal keys keep their order), keys of mixed types (eg, `int` and `float`, or NumPy numbers) compare as in Python (previously raised `TypeError`), and a key function that raises leaves the list unchanged.
  * Fixed `GpmlTimeSample.set_enabled(is_enabled=...)` raising `ArgumentError`: the documented keyword `is_enabled` now works (the keyword that worked, `set_enabled`, no longer does).
  * `GpmlIrregularSampling.set_value(..., is_enabled=None)` gives an enabled time sample, the default, as documented (previously disabled, so a rotation sample set that way was ignored).
  * `GmlDataBlock.set()` can change the number of scalar values of a data block's only scalar type (previously raised `ValueError`), and a replaced scalar type keeps its place in the data block (previously it moved to the end).
  * `set_value()` on `GpmlTimeSample`, `GpmlTimeWindow` and `GpmlConstantValue` raises `ValueError` for a property value of a different type from the one it replaces. Previously it was accepted but the old value type was kept, which could make a file written with it read back wrongly (eg, a whole total reconstruction pole read back uninterpreted, or an `XsInteger` read back as a `GpmlPlateId`).
  * `GpmlPiecewiseAggregation.get_time_window_containing_time()` compares a `float` time within a small epsilon, as it already did for a `GeoTimeInstant` time (previously a `float` time at a window's begin or end time, but for rounding, could miss the window).
  * Documentation: `GpmlIrregularSampling.get_value()` raises `InterpolationError` when a time sample bounding *time* has a distant past or distant future time; a *time* that is itself distant past or future returns `None`.
  * `GpmlIrregularSampling()` sorts its time samples by time, from most recent to least recent (keeping time samples with equal times in the order given), as do reading a GPML file and reading a `.grot` rotation file. Everything using a sampling (eg, `get_value()` and `RotationModel`) relies on that order, and a sampling out of order silently gave `None` or the wrong value.
    * A sampling built least recent first (which `get_value()` and `get_time_samples_bounding_time()` accepted) now has its most recent time sample first.
    * Changing the time samples directly (eg, a time sample's time) must keep them in that order, as `GpmlIrregularSampling` now documents.
  * `PolylineOnSphere.rotation_interpolate()` raises `ValueError` for an interpolate interval that is zero, negative or NaN, or so small that the number of interpolated polylines doesn't fit a 32-bit integer (previously a small negative interval tried to allocate billions of polylines, and zero or NaN silently returned just the two polylines), and for an interpolate ratio outside [0, 1] or NaN (previously `PreconditionViolationError`).
  * `to_tessellated()` on `PolylineOnSphere`, `PolygonOnSphere` and `GreatCircleArc` raises `ValueError` for a NaN threshold (previously accepted), and for a threshold less than about 1.5e-9 radians, where the number of points no longer fits a 32-bit integer (previously undefined behaviour). `DateLineWrapper.wrap()` treats a NaN `tessellate_degrees` as no tessellation, like zero or a negative value, and raises a tiny positive one to the minimum at which the number of points fits a 32-bit integer (previously undefined behaviour).
  * Fixed `FiniteRotation.get_rotation_distance()` returning a negative distance for a rotation with a negative angle (including some returned by `get_inverse()`, `interpolate()` and `*`).
  * Fixed `FiniteRotation.are_equal()` with a threshold comparing pole longitudes without wrapping, so poles either side of the dateline (eg, longitudes 180 and -180) were unequal.
  * Documented argument names now work as keyword arguments (previously raised `ArgumentError`): `GreatCircleArc.get_arc_point()` and `get_arc_direction()`, `FiniteRotation.create_great_circle_point_rotation()`, `create_small_circle_point_rotation()` and `create_segment_rotation()`, the `visit_...()` methods of `PropertyValueVisitor`, and the `create_gpml()`, `create_gml()`, `create_xsi()` and `create_from_qualified_string()` methods of `PropertyName`, `FeatureType`, `EnumerationType`, `ScalarType` and `StructuralType`.
  * `PropertyValueVisitor.visit_gpml_finite_rotation()` names its argument `gpml_finite_rotation` (previously documented as `ppml_finite_rotation`).
  * Fixed `PolylineOnSphere.join()` with `PolylineConversion.raise_if_non_polyline` returning `[]` for an iterator or generator of geometries (it iterated them twice). It is documented as taking any iterable, and the type stub says `Iterable`.
  * Fixed writing a `.grot` rotation file copying a sequence's poles into the next sequence when the two have the same moving plate (`MPRS:pid`), eg either side of a crossover. The copied poles grew with every save. This affected `FeatureCollection.write()` and `synchronise_crossovers()` with a `.grot` file.
  * Fixed a feature with the reconstruction method `ByPlateId` (eg, a motion path created by pyGPlates 1.0, or a feature given it explicitly) losing it when written to a Shapefile, GeoJSON or GeoPackage file and read back (the reader expected `ByPlateID`, which it still accepts).
  * Fixed `synchronise_crossovers()` writing an identity rotation as the synchronised crossover pole (shifting the rest of that sequence to match, and reporting it as synchronised) when there is no plate circuit from the moving plate to the synchronised crossover's fixed plate at the crossover time (eg, when the preserved crossover sequence has only one enabled pole). That crossover is now left unchanged, reported as `CrossoverResult.error`, and `synchronise_crossovers()` returns `False`.
  * A NaN time (eg, `GeoTimeInstant(float('nan'))`, or NaN as a reconstruction time) raises `ValueError` (previously `AbortError`, with a call-stack trace, wherever a time is taken).
  * Fixed `reconstruct()` raising `ArgumentError` for its documented keyword argument `reconstructed_geometries`. The keyword that worked, `reconstructed_feature_geometries`, still works (but not both together).
  * A cache size of zero (`reconstruction_tree_cache_size` of `RotationModel`, `reconstruct_snapshot_cache_size` of `ReconstructModel`, `topological_snapshot_cache_size` of `TopologicalModel`) raises `ValueError` (previously `PreconditionViolationError`, with no description). Fixed a `ReconstructModel` or `TopologicalModel` cache size of 2**32-1 raising `PreconditionViolationError`.
  * `ResolveTopologyParameters()` raises `ValueError` for parameters out of range: a `max_clamped_strain_rate` that is zero, negative or NaN, a `rift_exponential_stretching_constant` outside (0, 100] (the range GPlates allows), a `rift_strain_rate_resolution` that is not positive and finite, or a `rift_edge_length_threshold_degrees` outside [0.001, 180]. So does `Feature.create_topological_network_feature()` for its `rift_parameters`. Previously a zero threshold and resolution crashed the Python process (stack overflow) when querying a rifting network, a negative `max_clamped_strain_rate` flipped the sign of strain rates (and gave NaN where there was no strain), and a zero stretching constant silently gave wrong rift velocities and strain rates.
  * A topological network whose rift parameters, read from a file, are out of range uses the default for each (with a warning) instead, and a rift edge length threshold is at least 0.001 degrees, which bounds how finely rift edges are subdivided.
  * Fixed `resolve_topologies()` ignoring its `default_resolve_topology_parameters` argument (it always used the default parameters, eg, no strain rate clamping).
  * `resolve_topologies()` raises `ValueError` for a `resolve_topological_section_types` other than `ResolveTopologyType.boundary` and/or `ResolveTopologyType.network`, as documented (previously it gave no sections, and wrote no file, without an error). When not specified, it defaults to `resolve_topology_types` without any `ResolveTopologyType.line`.
  * A `ResolvedTopologicalNetwork` keeps the features of its rigid blocks and other interior geometries alive, as it does its boundary sections' features. Previously, once the features used to resolve it were gone (eg, after `TopologicalSnapshot(filenames, ...).get_resolved_topologies()`), a rigid block's `get_feature()` returned `None`, and `get_resolved_geometry_point_features(True)` raised `PreconditionViolationError`.
  * `TopologicalModel.reconstruct_geometry()` raises `ValueError` for a NaN `time_increment`, and for more than 100,000 time slots from `oldest_time` to `youngest_time`. Previously a tiny increment could overflow the number of time slots (undefined behaviour, using gigabytes of memory in a second), and a small one took a very long time (the run time grows with the square of the number of time slots).
  * Fixed `TopologicalModel.reconstruct_geometry()` deactivating points differently depending on earlier calls: a `ReconstructedGeometryTimeSpan.DefaultDeactivatePoints` (including the default `deactivate_points` argument, one instance shared by every call) reused stage rotations from an earlier call with a different time increment or direction.
  * `TopologicalSnapshot.calculate_plate_boundary_statistics()` raises `ValueError` for a negative (or NaN) `first_uniform_point_spacing_radians` (previously the first point of each boundary section was extrapolated off the boundary, giving a negative boundary length), and for a NaN `uniform_point_spacing_radians`.
  * Documentation: `TopologicalModel.reconstruct_geometry()` says that an `initial_time` between time slots is snapped to the nearest one (and what that means with and without `reconstruction_plate_id`), and that a polyline or polygon is accepted as a sequence of points. `ReconstructedGeometryTimeSpan.get_time_span()` gives the right number of time slots (one more than it said), and `DeactivatePoints.deactivate()` says its times are always `float`. The type stub follows.
  * Fixed a `FeatureType` (or other qualified name) read from a GPML file that binds the GPML namespace to another prefix (eg, `xmlns:gp=`) hashing differently from the equal canonical name, so `set` and `dict` lookups missed it.
  * `create_gpml()`, `create_gml()`, `create_xsi()` (of `PropertyName`, `FeatureType`, `EnumerationType`, `ScalarType` and `StructuralType`) raise `ValueError` for an empty name, and `create_from_qualified_string()` returns `None` for one (eg, `''` or `'gpml:'`).
  * A `point_distribution` of zero or less (of `NetRotationModel`, `NetRotationSnapshot`) raises `ValueError`, as documented (previously a negative one raised `OverflowError`).
  * `NetRotation.create_sample_from_finite_rotation()`, `convert_finite_rotation_to_rotation_rate_vector()` and `convert_rotation_rate_vector_to_finite_rotation()` raise `ValueError` for a `time_interval` that is zero, negative, infinite or NaN. Previously zero, infinity or NaN gave a sample with an area but no rotation (which halved a summed net rotation), a negative interval reversed the rotation, and converting with zero raised `PreconditionViolationError`.
  * `Strain()` raises `ValueError` for a deformation gradient whose determinant (the ratio of deformed to original area) is not positive. The docs of `Strain.accumulate()`, `get_dilatation()` and `get_principal_strain()` say what happens when accumulating with too large a strain rate and time increment gives such a strain (previously the principal strains were `(0, 0, 0)` and the dilatation disagreed with its documented formula, without explanation).
  * Writing a GPML file no longer prints "Popping namespace stack." (a leftover debug message) for each element that declares a namespace.
  * Fixed `PlatePartitioner.partition_features()` and `partition_into_plates()` copying a property named in `properties_to_copy` as its value at present day: the whole property is now copied, so a time-dependent one keeps its other times. Previously one with no value at present day raised `AttributeError`.
  * `PlatePartitioner.partition_features()` and `partition_into_plates()` raise `TypeError` for a `properties_to_copy` that is an iterator (eg, a generator), which copied the properties to the first partitioned feature only.

pyGPlates 1.0.0
===============

Changes since 0.36:

* Can now be installed using conda (`conda install -c conda-forge pygplates`) or pip (`pip install pygplates`).
  * The pre-compiled binary packages (a zip file or Debian package, added to `PYTHONPATH` manually) are no longer available.
* Pickle support, so pyGPlates objects can be used in multi-processing workflows.
  * Most classes can be pickled (including feature collections, features and feature properties), but classes containing reconstructed geometries cannot.
  * Each class documents whether it can be pickled.
* Filenames can be `os.PathLike` (such as `pathlib.Path`) in addition to strings.
* Added `ReconstructModel` and `ReconstructSnapshot` classes.
  * The regular-reconstruction counterparts of `TopologicalModel` and `TopologicalSnapshot`, and better than using the `reconstruct()` function.
* Added net rotation:
  * Create a `NetRotationModel` from a `TopologicalModel` (optionally with your own point distribution; defaults to a uniform lat-lon grid).
  * Use it to generate a `NetRotationSnapshot` at a reconstruction time, which calculates a `NetRotation` (total over all topologies, or of a specific topology).
  * Or manually accumulate your own net rotation from points and their finite rotations.
* Can generate statistics along plate boundaries (at uniformly spaced points):
  * `TopologicalSnapshot.calculate_plate_boundary_statistics()` returns a `PlateBoundaryStatistic` at each point, containing:
    * boundary normal/length/velocity,
    * convergence velocity/obliquity/etc,
    * left/right plate velocity/etc (including strain rate), and
    * distances to the start/end of the plate boundary section (eg, trench).
* Improved velocities and deformation:
  * Added `Strain` and `StrainRate` classes (dilatation, principal strain, dilatation rate, total strain rate, etc), and documented the underlying deformation theory.
  * Can query the deforming triangulation (and rigid interior blocks) of a resolved topological network.
  * Added `Feature.create_topological_network_feature()` (enables rift network parameters to be specified).
  * Can query a `TopologicalSnapshot` at static points to find intersected plates/networks, velocities and strain rates.
  * Can query a `ReconstructSnapshot` at static points to find intersected static polygons and velocities.
  * Topologically reconstructed points can directly provide crustal thickness, stretching factor, thinning factor and tectonic subsidence, and can calculate velocities, strain rate and strain.
  * Can incrementally reconstruct a point using a single deforming network (or rigid plate) over one time step.
    * Same functionality as `TopologicalModel.reconstruct_geometry()` but at a finer granularity, giving more control over the reconstruction.
  * All reconstructed and resolved geometries can calculate velocities at their vertices:
    * `ReconstructedFeatureGeometry`, `ReconstructedMotionPath` and `ReconstructedFlowline` (eg, `ReconstructedFeatureGeometry.get_reconstructed_geometry_point_velocities()`).
    * `ResolvedTopologicalLine`, `ResolvedTopologicalBoundary` and `ResolvedTopologicalNetwork` (eg, `ResolvedTopologicalLine.get_resolved_geometry_point_velocities()`).
    * Their sub-segments `ResolvedTopologicalSharedSubSegment` and `ResolvedTopologicalSubSegment`.
  * Fixed querying the overriding/subducting plates/networks at shared boundary segments.
* Added a Primer chapter to the documentation (currently covering topologies and deformation; a work in progress).

pyGPlates 0.36
==============

Changes since 0.28:

* Versioning scheme changed: this release is version 0.36 (instead of revision 36).
* Separate binary packages for macOS on Intel (x86_64) and on M1 (arm64).
* Added `TopologicalModel`:
  * Creates a topological snapshot at a geological time, which can be queried for resolved topological plates and deforming networks, and their shared boundaries (easier than the `resolve_topologies()` function).
  * Reconstructs and deforms points over a time period, giving a history of reconstructed positions, crustal stretching and tectonic subsidence (equivalent to "Reconstruct using topologies" in GPlates).
    * Uses the same algorithm as GPlates for deactivating points (eg, subduction of oceanic points), or your own.
    * Strain rate clamping to avoid excessive crustal stretching (similar to the clamping in GPlates' resolved topological network layers).
* File I/O:
  * GeoJSON and GeoPackage supported when reading and writing feature collections.
  * GeoJSON supported when exporting reconstructed and resolved geometries.
* New ways to create a `FiniteRotation` between two points, or between two lines.
* Interior holes supported in polygons (including dateline-wrapped polygons).
* All geometry types have `get_centroid()` (no need to first test whether the geometry is a point, multi-point, polyline or polygon).
* All NumPy integer and float scalar types accepted as arguments (eg, a function accepting a float also accepts a `numpy.float64`).
* Bug fixes.

pyGPlates 0.28
==============

Changes since 0.18:

* Supports Python 3 (in addition to Python 2.7):
  * Pre-built libraries for Windows and macOS for Python 2.7, 3.5, 3.6, 3.7 and 3.8.
  * Ubuntu packages for 16.04 LTS, 18.04 LTS, 19.10 and 20.04 LTS (Python 2 and 3, 32-bit and 64-bit, except 20.04 which is Python 3 64-bit only).
* macOS libraries are signed and notarized by Apple (no more security prompts, including on macOS Catalina).
* Topological features (dynamic lines, polygons and deforming networks) can now be created in pyGPlates (not only in GPlates).
* Supports deforming trenches when used with the subduction convergence script.
* Much faster querying of reconstructions.
* Bug fixes.

pyGPlates 0.18
==============

Changes since 0.12:

* Compatibility release for data saved by GPlates 2.0 and 2.1 (especially topologies): fixes incorrect reconstruction of half-stage rotation features (such as mid-ocean ridges) created by GPlates 2.0 or later. No major new features.
* The binaries require Python 2.7, but the source code now also works with Python 3.

pyGPlates 0.12
==============

* First beta release of pyGPlates (Python 2.7, on Windows, Linux and macOS): load and save feature data (GPML, Shapefile, etc), create/modify/query features and the plate rotation hierarchy, partition into plates, reconstruct geometries/flowlines/motion paths, resolve topological plates and query their boundary sections, calculate velocities, and distance and geometry queries.
