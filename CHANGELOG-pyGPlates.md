pyGPlates Changelog
===================

Changes in each pyGPlates release, newest first. Changes to GPlates (the desktop application) are listed in `CHANGELOG-GPlates.md`.

pyGPlates 1.1 (unreleased)
==========================

Changes since 1.0.0:

* Supports Python 3.9 to 3.14 (Python 3.8 is no longer supported).
* Type stub included in the package (PEP 561), so editors and type checkers get completion, signature help and type checking for the whole API.
* The API reference documents every enumeration (eg, `PartitionMethod`) and exception (eg, `InvalidLatLonError`) on its own page, and parameter types and raised exceptions now link to those pages.
* Pickling is much faster (eg, a `RotationModel` pickles about 7 times faster and unpickles about 5 times faster).
  * Pickles written by pyGPlates 1.0 can still be loaded, but pickles written by 1.1 cannot be loaded by 1.0.
  * Fixed pickling an object that had extra attributes added to it from Python (previously raised "Incomplete pickle support").
* Added `TopologicalSnapshot.reconstruct_points()` to incrementally reconstruct points (lying within the snapshot's resolved plates and networks) to another time.
* `GpmlTopologicalSection.create()` accepts a feature whose geometry is a polygon (returning a line section), as the topology building tools in GPlates do.
* `GpmlTopologicalSection.create()` and `create_network_interior()` take a `property_return` argument: with `PropertyReturn.first` they reference a feature that has more than one geometry with the same property name, instead of returning `None`.
* Added an `anchor_plate_id` argument to `PlatePartitioner` and `partition_into_plates()` (it defaults to the rotation model's default anchor plate, which was previously always used):
  * Created from plate features, it reconstructs or resolves them with that anchor plate (as does `partition_into_plates()`).
  * Created from plates already reconstructed or resolved, it should be the anchor plate they used (eg, `TopologicalSnapshot(..., anchor_plate_id=701)`), so that `partition_features()` reverse reconstructs the partitioned features with it.
* Fixed `Feature.get_geometry()` and `Feature.get_topological_geometry()` with `PropertyReturn.first` when nothing matches: they now always return `None` as documented (previously undefined behaviour, which could crash on some platforms).
* Fixed `GpmlTopologicalSection.create()` and `create_network_interior()`, without a geometry property name, for a feature whose type has no default geometry property name (not in the information model): they now return `None` (previously undefined behaviour).
* Documentation sample code:
  * Every sample is now a Python 3 script that the test suite runs, so it can be copied and run as shown (many samples previously used Python 2 `print` statements).
  * Samples use `ReconstructSnapshot` and `PlatePartitioner` instead of `reconstruct()` and `partition_into_plates()` (except where a one-off partition is clearer).
  * Each sample page lists the data files it needs and where to get them, and links to related Primer sections, Reference entries and samples.
* Installation:
  * The pip wheels and the conda package no longer depend on Qt GUI or OpenGL libraries.
    * So pyGPlates imports on a minimal Linux system (no `libGL` or `glib` packages needed) and the packages are smaller.
  * Linux wheels require glibc 2.28 or later (manylinux_2_28), instead of glibc 2.17 (manylinux2014).
  * Wheels on all platforms bundle the same PROJ (9.8.1) and GDAL (3.12.4) versions.
  * Requires NumPy 1.19 or later, and `pip` now enforces it: installing pyGPlates upgrades an older NumPy (previously the oldest NumPy that worked depended on the Python version, and an older one failed `import pygplates`).
* Documentation: corrected the supported Python versions and platforms in *Getting started*, fixed the introductory examples (which raised `NameError`), and documented the Python console in GPlates.
* Documentation: the *Primer* is split into one page per topic (rotations, topologies and deformation). Links to sections of the old single-page Primer still work, redirecting to the section on its new page.
* Removed the undocumented classes `Colour`, `Palette`, `PaletteKey`, `OldFeature` and `OldFeatureCollection` (they were only meaningful inside GPlates).
* Removed `namedtuple`, `partial`, `math`, `itertools`, `numpy`, `iteritems`, `itervalues`, `listitems` and `listvalues` from the `pygplates` module. They were internal imports and Python 2 helpers, exposed as `pygplates.<name>` by accident; import them from the standard library (or `numpy`) instead.
* Bug fixes:
  * Fixed `GeometryOnSphere.distance()` returning inconsistent closest positions and segment indices when two polylines/polygons intersect more than once.
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
  * `insert()` on the list-like classes (eg, `GpmlIrregularSampling`, `GpmlTimeSampleList`) clamps an out-of-range index to the start or end, like `list.insert()` (previously raised `IndexError`).
  * Fixed `GpmlIrregularSampling.get_value()` returning `None` at the time of the only enabled time sample, and (for an `XsDouble`) where two time samples have the same time. Likewise `get_time_samples_bounding_time()` now returns the only enabled time sample (as both samples) at its time. Times are compared with a small tolerance (eg, a time sample at 0.3 is found at 0.1 + 0.2).
  * Fixed `InformationModelError` giving the wrong one of two messages: adding a property the information model recognises, but not for the feature type, said the property name was not recognised (and adding an unknown property name said it was not valid for the feature type).
  * Fixed the pip wheels loading the GDAL plugins named by a `GDAL_DRIVER_PATH` environment variable (eg, set by an active conda environment with GDAL, or by OSGeo4W or QGIS). Those plugins are built for another GDAL, and reading a file could crash (eg, a Shapefile after `import pygmt`).

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
