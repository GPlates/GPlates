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
* Installation:
  * The pip wheels and the conda package no longer depend on Qt GUI or OpenGL libraries.
    * So pyGPlates imports on a minimal Linux system (no `libGL` or `glib` packages needed) and the packages are smaller.
  * Linux wheels require glibc 2.28 or later (manylinux_2_28), instead of glibc 2.17 (manylinux2014).
  * Wheels on all platforms bundle the same PROJ (9.8.1) and GDAL (3.12.4) versions.
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
