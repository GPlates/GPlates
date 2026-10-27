.. _pygplates_primer_topologies:

Topologies
----------

This section covers topologies in pyGPlates.

.. contents::
   :local:
   :depth: 4

.. _pygplates_primer_topological_model:

Topological model
^^^^^^^^^^^^^^^^^

A topological model is represented by a :class:`pygplates.TopologicalModel`.
It can be created from topological features (in files, :class:`features <pygplates.Feature>` or
:class:`feature collections <pygplates.FeatureCollection>`) and a rotation model (created from rotation files,
:class:`features <pygplates.Feature>` or :class:`feature collections <pygplates.FeatureCollection>`):
::

   rotation_model = pygplates.RotationModel('rotations.rot')
   topological_model = pygplates.TopologicalModel('topologies.gpml', rotation_model)

.. note:: Alternatively you can just pass the rotation filenames (or features) directly into the topological model:
   ::

      topological_model = pygplates.TopologicalModel('topologies.gpml', 'rotations.rot')

You can also control *how* topologies are resolved using :class:`pygplates.ResolveTopologyParameters` -
currently these parameters only affect deforming networks.
This is done by specifying the ``default_resolve_topology_parameters`` - see
:ref:`pygplates_primer_strain_rate_clamping`, :ref:`pygplates_primer_strain_rate_smoothing` and
:ref:`pygplates_primer_exponential_rift_stretching_profile` for more details.

.. note:: You can also override the default parameters for each topological entry (eg, each file, feature collection or feature)
   by specifying a 2-tuple (topological entry, :class:`pygplates.ResolveTopologyParameters`) instead of just the topological entry.

A topological model can:

* Create a :ref:`pygplates_primer_topological_snapshot` at a reconstruction time.
* :ref:`pygplates_primer_topologically_reconstruct_geometries` over a time range.

.. _pygplates_primer_topological_snapshot:

Topological snapshot
""""""""""""""""""""

A topological snapshot is represented by a :class:`pygplates.TopologicalSnapshot`.
It can be created by resolving a :class:`pygplates.TopologicalModel` to a specific reconstruction time.
For example, to create a topological snapshot for each reconstruction time from 0 to 1000Ma in 1Myr increments:
::

   for reconstruction_time in range(1000):
      topological_snapshot = topological_model.topological_snapshot(reconstruction_time)

Alternatively, a topological snapshot can be created directly from topological features and a rotation model
(similar to :ref:`how a topological model is created <pygplates_primer_topological_model>`) and a reconstruction time:
::

   for reconstruction_time in range(1000):
      topological_snapshot = pygplates.TopologicalSnapshot('topologies.gpml', rotation_model, reconstruction_time)

.. note:: It is more efficient to generate snapshots from a :class:`pygplates.TopologicalModel` (rather than explicity using
   ``pygplates.TopologicalSnapshot(...)``). This is because a topological model only needs to read/parse the input topological and
   rotation features once, rather than at each time step. And also, the topological snapshots are cached internally within the
   topological model, so requesting the same snapshot again (at the same reconstruction time) will not require the topologies to
   be resolved again (at that reconstruction time).

A topological snapshot can:

* Generate :ref:`pygplates_primer_plate_boundary_statistics` (like convergence/divergence) along plate boundaries.

.. _pygplates_primer_plate_boundary_statistics:

Plate boundary statistics
'''''''''''''''''''''''''

.. note:: The following sample codes use plate boundary statistics:

   * :ref:`pygplates_find_divergence_at_subduction_zones_and_convergence_at_ridges`
   * :ref:`pygplates_sample_intra-plate_strain_rates_at_subduction_zones`

Statistics at uniformly spaced points along plate boundaries can be generated from a topological snapshot using
:meth:`pygplates.TopologicalSnapshot.calculate_plate_boundary_statistics`.
For example, to generate statistics at points spaced 1 degree apart (along all plate boundaries):
::

   uniform_point_spacing_radians = math.radians(1)
   plate_boundary_stats = topological_snapshot.calculate_plate_boundary_statistics(uniform_point_spacing_radians)

You can also restrict which plate boundaries to generate points along.
For example, to generate points only along subduction zones and mid-ocean ridges:
::

   plate_boundary_stats = topological_snapshot.calculate_plate_boundary_statistics(
         uniform_point_spacing_radians,
         boundary_section_filter = [pygplates.FeatureType.gpml_subduction_zone,
                                    pygplates.FeatureType.gpml_mid_ocean_ridge])

...or even define your own criteria as a filter function accepting a single argument of type :class:`pygplates.ResolvedTopologicalSection`
and returning ``True`` if uniform points should be generated along that boundary section. For example, the equivalent of the above
example (generating points only along subduction zones and mid-ocean ridges) would be:
::

   def boundary_section_filter_function(resolved_topological_section):
      feature_type = resolved_topological_section.get_feature().get_feature_type()
      return (feature_type == pygplates.FeatureType.gpml_subduction_zone or
              feature_type == pygplates.FeatureType.gpml_mid_ocean_ridge)
   
   plate_boundary_stats = topological_snapshot.calculate_plate_boundary_statistics(
         uniform_point_spacing_radians,
         boundary_section_filter = boundary_section_filter_function)

.. note:: You can also group uniform points with the :class:`shared sub-segment <pygplates.ResolvedTopologicalSharedSubSegment>`
   they came from by setting :meth:`return_shared_sub_segment_dict <pygplates.TopologicalSnapshot.calculate_plate_boundary_statistics>` to ``True``.

Each point gets its own statistic represented by a :class:`pygplates.PlateBoundaryStatistic`.
For example, to query the uniformly spaced point locations and their convergence velocity magnitudes and obliquities:
::

   for stat in plate_boundary_stats:
      boundary_point = stat.boundary_point
      if convergence_velocity is not None:  # make sure the left and right plates exist
         convergence_velocity_magnitude = stat.convergence_velocity_magnitude
         convergence_velocity_obliquity = stat.convergence_velocity_obliquity

There are many other :class:`statistics <pygplates.PlateBoundaryStatistic>` such as plate *boundary* velocity, plate boundary *normal* direction,
left and right plate velocities, left and right plate identifiers (ie, which plate, or deforming network, is left and right of the point)
and distance to the ends of the boundary section (containing the point).

.. _pygplates_primer_topologically_reconstruct_geometries:

Topologically reconstruct geometries
""""""""""""""""""""""""""""""""""""

.. note:: The following sample codes use topological reconstruction:

   * :ref:`pygplates_reconstruct_strain_and_strain_rate`
   * :ref:`pygplates_reconstruct_crustal_thickness_and_tectonic_subsidence`

Usually features are reconstructed using :class:`pygplates.ReconstructModel`, which relies only on the properties of the features
(such as their reconstruction plate IDs) to reconstruct them to past geological times.

An alternative approach is to use topologies (topological closed plate polygons and networks) to reconstruct an initial geometry.
In this case it is the topological plates and networks that determine how the geometry moves over time.
In other words, the geometry rigidly rotates when it is in rigid plates and deforms when it is in deforming networks.

.. contents::
   :local:
   :depth: 2

.. _pygplates_primer_what_is_topological_reconstruction:

What is topological reconstruction?
'''''''''''''''''''''''''''''''''''

Topological reconstruction is an incremental process whereby each point in an initial geometry is reconstructed over a time period by dividing
that time period into a series of smaller time intervals. Within each time interval, the geometry at the start of the interval is reconstructed
to the end of the interval using the resolved topologies (at the start of the interval). This incremental reconstruction is performed iteratively
over the full time period to obtain a history of reconstruction snapshots (of the geometry), with each snapshot occupying a time slot.

For each new time interval, the resolved topologies can change, such as plates splitting/merging and deforming networks appearing/disappearing.
So at the start of each time interval, each point of the geometry is tested to see which topological plate or network it lies within
(with higher priority given to networks since they typically overlay the rigid plates). Then each point is reconstructed over the time interval
using the topological plate (see :meth:`pygplates.ResolvedTopologicalBoundary.reconstruct_point`) or topological network
(see :meth:`pygplates.ResolvedTopologicalNetwork.reconstruct_point`) that the point lies within.

.. _pygplates_primer_what_is_topological_reconstruction_reconstruction_plate_id:

Reconstruction plate ID
***********************

Since topological reconstruction is peformed using topologies, no feature properties are needed (in contrast with non-topological reconstruction
using :class:`pygplates.ReconstructModel`). However, if a geometry point does not intersect any resolved topologies during a time interval then an
optional reconstruction plate ID is used to *rigidly* reconstruct it over that time interval. And if a reconstruction plate ID was not provided then
the point does not move over that time interval.

.. note:: Some geometry points can fail to intersect topologies if the topologies are regional (instead of global), or if there are cracks
   between adjacent topologies (due to the way they were built).

.. _pygplates_primer_what_is_topological_reconstruction_deactivating_points:

Deactivating points
*******************

The history of reconstruction snapshots covers a time range from an oldest time to a youngest time. And the initial geometry is provided at an
initial time (that can be inside or outside that time range). Hence an initial geometry can be topologically reconstructed forward in time, or
backward in time, or both, depending on where the initial time is in relation to the oldest and youngest times.

Initially all geometry points are active at the *initial time*, but can get progressively deactivated as they are topologically reconstructed
*away* from the initial time. When a point is deactivated it becomes inactive and is no longer topologically reconstructed for subsequent
time slots (*further* from the initial time). When reconstructed *forward* in time, points on oceanic crust get deactivated as they are subducted.
And when reconstructed *backward* in time, they get deactivated as they reach their time of appearance (at a mid-ocean ridge).

.. _pygplates_primer_using_topological_reconstruction:

Using topological reconstruction
''''''''''''''''''''''''''''''''

Topological reconstruction requires a :class:`pygplates.TopologicalModel` and a geometry. Currently the geometry can only be one or more points.
Then :meth:`pygplates.TopologicalModel.reconstruct_geometry` can be used to generate a reconstructed history of snapshots of the geometry points,
and associated quantities (like velocity), that are stored in the returned :class:`pygplates.ReconstructedGeometryTimeSpan`.
For example, to topologically reconstruct points from their initial positions at 100 Ma to present day, in increments of 1 Myr:
::

   # Convert from latitudes and longitudes to a list of pygplates.PointOnSphere.
   lats = [...]  # point latitudes
   lons = [...]  # point longitudes
   points = [pygplates.PointOnSphere(lat, lon) for lat, lon in zip(lats, lons)]

   reconstructed_geometry_time_span = topological_model.reconstruct_geometry(
         points,
         initial_time=100)

The returned :class:`pygplates.ReconstructedGeometryTimeSpan` contains 101 reconstructed snapshots of the initial geometry in its history.
You can use it to query the reconstructed geometry at any reconstruction time. For example, to query the reconstructed points at 50 Ma:
::

   reconstructed_points = reconstructed_geometry_time_span.get_geometry_points(50)

   if reconstructed_points:
      # Convert from a list of pygplates.PointOnSphere to a list of (latitude, longitude) tuples.
      reconstructed_lat_lons = [point.to_lat_lon() for point in reconstructed_points]

.. seealso:: :ref:`pygplates_primer_reconstructed_geometry_time_span` for more details on querying reconstruction snapshots.

.. _pygplates_primer_using_topological_reconstruction_time_spans:

Time spans
**********

The time span of snapshots is determined by the oldest and youngest times.

In the above example, points were reconstructed *forward* in time (from 100 Ma to present day).
So the oldest time was 100 Ma and the youngest was 0 Ma.

You can also reconstruct *backward* in time. For example, to reconstruct from present day to 100 Ma, in increments of 1 Myr:
::

   reconstructed_geometry_time_span = topological_model.reconstruct_geometry(
         points,
         initial_time=0,
         oldest_time=100)

...where we needed to explicitly specify ``oldest_time`` because otherwise it defaults to ``initial_time`` (which in this example is present day).

.. note:: Even though the reconstruction is *backward* in time, the oldest and youngest times are still 100 Ma and 0 Ma, respectively.

In the above cases, the youngest time defaults to present day. However you can explicitly set it using the ``youngest_time`` argument.
For example, if you only want a history of snapshots from 100 Ma to 50 Ma (instead of 100 Ma to present day):
::

   reconstructed_geometry_time_span = topological_model.reconstruct_geometry(
         points,
         initial_time=0,
         oldest_time=100,
         youngest_time=50)

It's also possible to reconstruct both *forward* and *backward* in time. This happens when the initial time is *between* the oldest and youngest times.
For example, if the initial points are at 50 Ma but you want a time range of snapshots from 100 Ma to present day:
::

   reconstructed_geometry_time_span = topological_model.reconstruct_geometry(
         points,
         initial_time=50,
         oldest_time=100)

In this case, the initial points are reconstructed both *forward* in time from 50 Ma to present day **and** *backward* in time from 50 Ma to 100 Ma
(in order to generate all snapshots from 100 Ma to present day).

You can also choose the time interval between reconstruction snapshots using the ``time_increment`` argument (which defaults to 1 Myr).
The time increment, along with the oldest and youngest times, determine the time slots.

.. note:: ``oldest_time - youngest_time`` must be an integer multiple of ``time_increment``.

If you choose a large time increment then the snapshots will be spaced farther apart and the resulting reconstruction accuracy will suffer.
Another source of inaccuracy is due to the initial time of the initial geometry being internally snapped to the nearest time slot.
For these reasons the time increment defaults to 1 Myr (which is typically the smallest time resolution used in topological models).

.. _pygplates_primer_using_topological_reconstruction_reconstruction_plate_id:

Reconstruction plate ID
***********************

In the above cases, the geometry is already in the correct position at the initial time. In other words, the geometry is a snapshot at the initial time.
For example, it could be uniform points spread across the entire globe at the initial time (and we want to see where they end up at other times).
So we did **not** specify the ``reconstruction_plate_id`` argument.

However if the geometry is a *present day* geometry localised to a specific plate (and the initial time is in the past) then specifying a
reconstruction plate ID will reconstruct it to the initial time (to become the snapshot at the initial time, before topologically reconstruction into
the other snapshots proceeds). For example, if the geometry represents its present day position on plate 701 (and we're reconstructing *forward* in time
from 100 Ma to present day) then:
::

   reconstructed_geometry_time_span = topological_model.reconstruct_geometry(
         points_at_present_day,
         initial_time=100,
         reconstruction_plate_id=701)

...will first rigidly reconstruct ``points_at_present_day`` from present day to 100 Ma using plate ``701``, and then topologically reconstruct them
from 100 Ma to present day (generating snapshots at 1 Myr intervals).

.. note:: ``reconstruction_plate_id`` also has other purposes. For example, when
   :ref:`generating the history of snapshots <pygplates_primer_what_is_topological_reconstruction_reconstruction_plate_id>` and when
   :ref:`querying geometry points <pygplates_primer_reconstructed_geometry_time_span_geometry_points>`.

.. _pygplates_primer_using_topological_reconstruction_deactivating_points:

Deactivating points
*******************

By default, geometry points can get progressively deactivated when they are topologically reconstructed away from the initial time.
This is useful for points on *oceanic* crust because that crust can get subducted, and it is typically younger than continental crust.
Therefore, oceanic points will get deactivated as they are subducted going *forward* in time and deactivated as they reach their time of appearance
(at mid-ocean ridges) going *backward* in time. This is the default behaviour and works for both oceanic and continental crust.

To disable this ability you can explicitly set the ``deactivate_points`` argument to ``None``. Then the points will always remain active.
This can be used (but is not necessary) when the points are all within the interior of *continents* (where crust exists at present day and has existed
for a long time). For example, to reconstruct *continental* points forward in time from 100 Ma to present day (without attempting to deactivate any):
::

   reconstructed_geometry_time_span = topological_model.reconstruct_geometry(
         points,
         initial_time=100,
         deactivate_points=None)

You can also change *how* points are deactivated, by either:

* changing the parameters of the *default* deactivation algorithm, or
* implementing your own deactivation algorithm.

The *default* deactivation algorithm is implemented in :class:`pygplates.ReconstructedGeometryTimeSpan.DefaultDeactivatePoints`.

To use the *default* parameters of the *default* deactivation algorithm you don't need to specify the ``deactivate_points`` argument.
For example, you can just call:
::

   topological_model.reconstruct_geometry(
         points,
         initial_time=100)

...since that is equivalent to calling:
::

   topological_model.reconstruct_geometry(
         points,
         initial_time=100,
         deactivate_points=pygplates.ReconstructedGeometryTimeSpan.DefaultDeactivatePoints())

However, you can change the parameters of the *default* deactivation algorithm.
For example, when reconstructing *oceanic* points (forward in time from 100 Ma to present day):
::

   reconstructed_geometry_time_span = topological_model.reconstruct_geometry(
         points,
         initial_time=100,
         deactivate_points=pygplates.ReconstructedGeometryTimeSpan.DefaultDeactivatePoints(
               # Choose our own parameters that are different than the defaults...
               threshold_velocity_delta = 0.9, # cms/yr
               threshold_distance_to_boundary = 15, # kms/myr
               deactivate_points_that_fall_outside_a_network = True))

You can also implement your own deactivation algorithm by implementing your own class that inherits from
:class:`pygplates.ReconstructedGeometryTimeSpan.DeactivatePoints` and overrides its
:meth:`deactivate method <pygplates.ReconstructedGeometryTimeSpan.DeactivatePoints.deactivate>`.

.. note:: This is what the *default* deactivation algorithm does.
   In other words, the :class:`DefaultDeactivatePoints <pygplates.ReconstructedGeometryTimeSpan.DefaultDeactivatePoints>` class
   inherits from the :class:`DeactivatePoints <pygplates.ReconstructedGeometryTimeSpan.DeactivatePoints>` class.

.. seealso:: :class:`pygplates.ReconstructedGeometryTimeSpan.DeactivatePoints` for more details.

.. _pygplates_primer_reconstructed_geometry_time_span:

Reconstructed geometry time span
''''''''''''''''''''''''''''''''

A :class:`pygplates.ReconstructedGeometryTimeSpan` contains a history of reconstruction snapshots generated by :meth:`pygplates.TopologicalModel.reconstruct_geometry`.

Each snapshot stores the following quantities:

* :ref:`pygplates_primer_reconstructed_geometry_time_span_geometry_points`
* :ref:`pygplates_primer_reconstructed_geometry_time_span_velocities`
* :ref:`pygplates_primer_reconstructed_geometry_time_span_topology_locations`
* :ref:`pygplates_primer_reconstructed_geometry_time_span_strain_rates`
* :ref:`pygplates_primer_reconstructed_geometry_time_span_strains`
* :ref:`pygplates_primer_reconstructed_geometry_time_span_scalar_values`

  * :ref:`pygplates_primer_reconstructed_geometry_time_span_crustal_thickness_factors`
  * :ref:`pygplates_primer_reconstructed_geometry_time_span_tectonic_subsidence`

The history of snapshots is stored in time slots defined by :meth:`pygplates.ReconstructedGeometryTimeSpan.get_time_span` whose time range is
determined by the *oldest_time* and *youngest_time* arguments of :meth:`pygplates.TopologicalModel.reconstruct_geometry`.
For example, to iterate over the *stored* history of :ref:`reconstructed geometry points <pygplates_primer_reconstructed_geometry_time_span_geometry_points>`
(from oldest time to youngest time):
::

   oldest_time, youngest_time, time_increment, num_time_slots = reconstructed_geometry_time_span.get_time_span()
   for time_slot in range(num_time_slots):
      reconstruction_time = oldest_time - time_slot * time_increment
      reconstructed_points = reconstructed_geometry_time_span.get_geometry_points(reconstruction_time)

However, you can query the snapshots at any *arbitrary* reconstruction time, it does not have to match a time slot.
And it can be outside the :meth:`time range <pygplates.ReconstructedGeometryTimeSpan.get_time_span>` of snapshots
(although typically you would generate a time range that contains all desired reconstruction times).
For times not matching a time slot, the behaviour is defined separately for each snapshot quantity.

For example, to iterate over the reconstructed geometry points at 1Myr intervals (from oldest time to youngest time)
*regardless* of the time slot intervals (which could be larger than 1Myr):
::

   oldest_time, youngest_time, _, _ = reconstructed_geometry_time_span.get_time_span()
   reconstruction_time = oldest_time
   while reconstruction_time >= youngest_time:
      reconstructed_points = reconstructed_geometry_time_span.get_geometry_points(reconstruction_time)
      reconstruction_time -= 1.0

When querying the various quantities in a snapshot (such as points or their velocities), each query has a ``return_inactive_points`` argument.
It defaults to ``False`` so that only quantities associated with *active* points are returned. However, if you set it to ``True`` then
quantities associated with both *active* and *inactive* points are returned. Each inactive point will have a value of ``None``
(since quantities cannot be calculated at inactive points). This can be useful when you need to keep track of points and their quantities
over time, since you can use point indices (an integer index into an array of points) which is not possible otherwise. For example, to find the maximum
:ref:`velocity <pygplates_primer_reconstructed_geometry_time_span_velocities>` of each point (in a geometry) over the time range of the snapshots:
::

   import numpy as np

   # The number of initial points in the geometry (initially all points are active).
   num_initial_points = len(initial_points)

   # A NumPy array of zeros (one for each point).
   # This will later get updated with the max velocity of each point (in the same order as the points).
   max_point_velocities = np.zeros(num_initial_points)

   # Topologically reconstruct the initial points from 100 Ma to present day (at 1 Myr intervals).
   reconstructed_geometry_time_span = topological_model.reconstruct_geometry(initial_points, initial_time=100)

   # Iterate over the time slots (100, 99, ..., 1, 0 Ma).
   oldest_time, youngest_time, time_increment, num_time_slots = reconstructed_geometry_time_span.get_time_span()
   for time_slot in range(num_time_slots):
      reconstruction_time = oldest_time - time_slot * time_increment

      # Get the velocity at each point (for each inactive point it will be 'None').
      reconstructed_velocities = reconstructed_geometry_time_span.get_velocities(
            reconstruction_time,
            return_inactive_points=True)
      
      # If all points are inactive (in the current time slot) then 'reconstructed_velocities' itself will be 'None'.
      #
      # Note: If it is 'None' then you could potentially finish here (because once all points are deactivated
      #       they can't be reactivated). However this depends on the order you visit the time slots. It's only
      #       possible if you start at the initial time (slot) which in our case happens to be the oldest time
      #       (since we're reconstructing *forward* in time from 100 Ma to present day).
      if reconstructed_velocities:
         # Iterate over all the points (some might be inactive).
         for point_index in range(num_initial_points):
            velocity = reconstructed_velocities[point_index]
            # If the current point is active (in the current time slot) then it will have a velocity.
            if velocity is not None:
               # See if velocity is the maximum for the current point over all time slots visited so far.
               velocity_magnitude = velocity.get_magnitude()
               if velocity_magnitude > max_point_velocities[point_index]:
                  max_point_velocities[point_index] = velocity_magnitude
   
   # Print out the maximum velocity of each geometry point.
   for point_index in range(num_initial_points):
      lat, lon = initial_points[point_index].to_lat_lon()
      velocity_magnitude = max_point_velocities[point_index]
      print('Max velocity of point initially at lon/lat ({}, {}) is {} km/myr'.format(lon, lat, velocity_magnitude))

...where we've associated a (maximum) velocity with each initial geometry point (such that the maximum velocity, and initial position,
of any geometry point can be found using its ``point_index``).

.. _pygplates_primer_reconstructed_geometry_time_span_geometry_points:

Geometry points
***************

The reconstructed geometry points at a reconstruction time can be queried using :meth:`pygplates.ReconstructedGeometryTimeSpan.get_geometry_points`:
::

   reconstructed_points = reconstructed_geometry_time_span.get_geometry_points(reconstruction_time)

   # If none of the points are active at 'reconstruction_time' then this will be 'None'.
   if reconstructed_points:
      ...

If the requested reconstruction time matches a time slot in the :meth:`time span <pygplates.ReconstructedGeometryTimeSpan.get_time_span>` then
the geometry points of the snapshot in that time slot are returned.

If the requested reconstruction time is *within* the :meth:`time range <pygplates.ReconstructedGeometryTimeSpan.get_time_span>` of the snapshots,
but does not match a time slot, then the geometry points in the time slot (of the two time slots nearest the reconstruction time) that is closest
to the initial time (specified in :meth:`pygplates.TopologicalModel.reconstruct_geometry`) will be incrementally reconstructed (away from the initial time)
to the requested reconstruction time using :meth:`pygplates.ResolvedTopologicalBoundary.reconstruct_point` or :meth:`pygplates.ResolvedTopologicalNetwork.reconstruct_point`
(depending on which plate/network in the time slot each active point lies within). And those reconstructed points will be returned.

.. note:: The returned geometry points will have the same active status as the time slot they're incrementally reconstructed *from*.
   In other words, if a point is active in the source time slot then it'll also be active in the returned geometry points.

If the requested reconstruction time is *outside* the :meth:`time range <pygplates.ReconstructedGeometryTimeSpan.get_time_span>` of the snapshots then the
reconstruction plate ID specified in :meth:`pygplates.TopologicalModel.reconstruct_geometry` will be used to *rigidly* reconstruct the geometry points from
the oldest time slot (if the requested reconstruction time is older), or from the youngest time slot (if the requested reconstruction time is younger),
to the requested reconstruction time. And those reconstructed points will be returned.

.. note:: The active status of the returned points will be the same as those in the oldest or youngest time slot. Which means there can still be active geometry points
   when the reconstruction time is *outside* the :meth:`time range <pygplates.ReconstructedGeometryTimeSpan.get_time_span>` of the snapshots.

.. note:: If no reconstruction plate ID was specified then there will be no rigid rotation, and so the geometry points from the oldest or youngest time slot will
   effectively be returned. However typically you would generate a time range that contains all desired reconstruction times (so this situation would not typically occur).

.. note:: The reconstruction plate ID can also be used when the requested reconstruction time is *inside* the time range and some geometry points are
   *outside* all resolved topologies (and hence cannot be reconstructed by the topologies). This can happen if the topologies are regional (instead of global)
   or if there are cracks between adjacent topologies (due to the way they were built).

In all cases, if *none* of the geometry points are active at the reconstruction time then ``None`` will be returned.

.. _pygplates_primer_reconstructed_geometry_time_span_topology_locations:

Topology locations
******************

A :class:`topology location <pygplates.TopologyPointLocation>` identifies the resolved topology boundary or network that contains a reconstructed geometry point,
or identifies no resolved topologies if the point does not intersect any.

The topology location of each reconstructed geometry point at a reconstruction time can be queried using :meth:`pygplates.ReconstructedGeometryTimeSpan.get_topology_point_locations`:
::

   reconstructed_topology_locations = reconstructed_geometry_time_span.get_topology_point_locations(reconstruction_time)

   # If none of the points are active at 'reconstruction_time' then this will be 'None'.
   if reconstructed_topology_locations:
      ...

A topology location is returned for each geometry point that is *active at the requested reconstruction time*
(see :ref:`pygplates_primer_reconstructed_geometry_time_span_geometry_points`). If *none* of the points are active then ``None`` will be returned. 

If the requested reconstruction time is *within* the :meth:`time range <pygplates.ReconstructedGeometryTimeSpan.get_time_span>` of the snapshots
then the returned topology locations are those of the geometry points in the time slot (of the two time slots nearest the reconstruction time)
that is closest to the initial time (specified in :meth:`pygplates.TopologicalModel.reconstruct_geometry`).

.. note:: The topology locations are at the time of the time slot (rather than the reconstruction time) because topologies are only resolved at the time slots.

If the requested reconstruction time is *outside* the :meth:`time range <pygplates.ReconstructedGeometryTimeSpan.get_time_span>` of the snapshots then the returned
topology locations identify no resolved topologies. In other words, :meth:`pygplates.TopologyPointLocation.not_located_in_resolved_topology` will return ``True``
for each geometry point. This is because topologies are *not* resolved outside the time range.

.. seealso:: :meth:`pygplates.ResolvedTopologicalBoundary.get_point_location` and :meth:`pygplates.ResolvedTopologicalNetwork.get_point_location`

.. _pygplates_primer_reconstructed_geometry_time_span_velocities:

Velocities
**********

The velocities of reconstructed geometry points at a reconstruction time can be queried using :meth:`pygplates.ReconstructedGeometryTimeSpan.get_velocities`:
::

   reconstructed_velocities = reconstructed_geometry_time_span.get_velocities(reconstruction_time)

   # If none of the points are active at 'reconstruction_time' then this will be 'None'.
   if reconstructed_velocities:
      ...

A velocity is calculated for each geometry point that is *active at the requested reconstruction time*
(see :ref:`pygplates_primer_reconstructed_geometry_time_span_geometry_points`). If *none* of the points are active then ``None`` will be returned. 

If the requested reconstruction time is *within* the :meth:`time range <pygplates.ReconstructedGeometryTimeSpan.get_time_span>` of the snapshots
then the returned velocities are calculated at the geometry points in the time slot (of the two time slots nearest the reconstruction time)
that is closest to the initial time (specified in :meth:`pygplates.TopologicalModel.reconstruct_geometry`).

.. note:: The velocities are calculated at the time of the time slot (rather than the reconstruction time), and at the positions of the active geometry points
   in the time slot (rather than the geometry points at the reconstruction time - see :ref:`pygplates_primer_reconstructed_geometry_time_span_geometry_points`).
   So, this is more like a nearest neighbour interpolation (rather than a linear interpolation) of the two nearest time slots.
   This is done since velocities are calculated using topologies, which are only resolved at the time slots, and the active status of velocities
   needs to be synchronised with the geometry points.

The velocities are determined by the topologies (rigid plates and deforming networks) resolved at the time of the time slot using
:meth:`pygplates.ResolvedTopologicalBoundary.get_point_velocity` or :meth:`pygplates.ResolvedTopologicalNetwork.get_point_velocity`
(depending on which plate/network in the time slot each active point lies within).

If the requested reconstruction time is *outside* the :meth:`time range <pygplates.ReconstructedGeometryTimeSpan.get_time_span>` of the snapshots then the
returned velocities are determined by the reconstruction plate ID specified in :meth:`pygplates.TopologicalModel.reconstruct_geometry`, and they're calculated
at the positions of the active geometry points at the reconstruction time (see :ref:`pygplates_primer_reconstructed_geometry_time_span_geometry_points`).

.. note:: If no reconstruction plate ID was specified then the velocities will be zero.

.. note:: The reconstruction plate ID can also be used when the requested reconstruction time is *inside* the time range and some geometry points are
   *outside* all resolved topologies (and hence their velocities cannot be determined by the topologies). This can happen if the topologies are regional
   (instead of global) or if there are cracks between adjacent topologies (due to the way they were built).

.. _pygplates_primer_reconstructed_geometry_time_span_strain_rates:

Strain rates
************

The :class:`strain rates <pygplates.StrainRate>` of reconstructed geometry points at a reconstruction time can be queried using
:meth:`pygplates.ReconstructedGeometryTimeSpan.get_strain_rates`:
::

   reconstructed_strain_rates = reconstructed_geometry_time_span.get_strain_rates(reconstruction_time)

   # If none of the points are active at 'reconstruction_time' then this will be 'None'.
   if reconstructed_strain_rates:
      ...

A strain rate is returned for each geometry point that is *active at the requested reconstruction time*
(see :ref:`pygplates_primer_reconstructed_geometry_time_span_geometry_points`). If *none* of the points are active then ``None`` will be returned. 

If the requested reconstruction time is *within* the :meth:`time range <pygplates.ReconstructedGeometryTimeSpan.get_time_span>` of the snapshots
then the returned strain rates are interpolated between the two time slots nearest the reconstruction time.

.. note:: The time slot *further* from the initial time (specified in :meth:`pygplates.TopologicalModel.reconstruct_geometry`) might have deactivated
   some points from the time slot *closer* to the initial time (see :ref:`pygplates_primer_what_is_topological_reconstruction_deactivating_points`).
   For these points the strain rate is not interpolated (instead it's obtained from the time slot *closer* to the initial time).

The strain rates in time slots are determined by the topologies (rigid plates and deforming networks) resolved at the time of the time slot using
:meth:`pygplates.ResolvedTopologicalBoundary.get_point_strain_rate` or :meth:`pygplates.ResolvedTopologicalNetwork.get_point_strain_rate`
(depending on which plate/network in the time slot each active point lies within). And the strain rate will be zero for each geometry point (in a time slot)
that is *not* within a deforming network.

If the requested reconstruction time is *outside* the :meth:`time range <pygplates.ReconstructedGeometryTimeSpan.get_time_span>` of the snapshots then the
returned strain rates will be zero (no deformation).

.. _pygplates_primer_reconstructed_geometry_time_span_strains:

Strains
*******

The :class:`strains <pygplates.Strain>` of reconstructed geometry points at a reconstruction time can be queried using
:meth:`pygplates.ReconstructedGeometryTimeSpan.get_strains`:
::

   reconstructed_strains = reconstructed_geometry_time_span.get_strains(reconstruction_time)

   # If none of the points are active at 'reconstruction_time' then this will be 'None'.
   if reconstructed_strains:
      ...

A strain is returned for each geometry point that is *active at the requested reconstruction time*
(see :ref:`pygplates_primer_reconstructed_geometry_time_span_geometry_points`). If *none* of the points are active then ``None`` will be returned. 

If the requested reconstruction time is *within* the :meth:`time range <pygplates.ReconstructedGeometryTimeSpan.get_time_span>` of the snapshots
then the returned strains are interpolated between the two time slots nearest the reconstruction time.

.. note:: The time slot *further* from the initial time (specified in :meth:`pygplates.TopologicalModel.reconstruct_geometry`) might have deactivated
   some points from the time slot *closer* to the initial time (see :ref:`pygplates_primer_what_is_topological_reconstruction_deactivating_points`).
   For these points the strain is not interpolated (instead it's obtained from the time slot *closer* to the initial time).

The strain of each geometry point is generated (in the time slots) by accumulating its strain rates *forward* in time, over the
:meth:`time range <pygplates.ReconstructedGeometryTimeSpan.get_time_span>` of the snapshots, using :meth:`pygplates.Strain.accumulate`.
The initial strain of each active geometry point in the oldest time slot will be the *identity* strain (since deformation has not yet occurred).
And the accumulated strain of a geometry point will only change (going forward in time) if the point undergoes deformation
(ie, is in a deforming network in one or more time slots).

If the requested reconstruction time is *outside* the :meth:`time range <pygplates.ReconstructedGeometryTimeSpan.get_time_span>` of the snapshots then the
returned strains will be identity strains (no deformation) if the requested reconstruction time is older than the oldest time slot, and will be the
accumulated strains of the youngest time slot if the requested reconstruction time is younger than the youngest time slot (since strains do not accumulate
outside the time range of the snapshots because strain rates are zero there).

.. _pygplates_primer_reconstructed_geometry_time_span_scalar_values:

Scalar values
*************

Each geometry point can have one or more scalar values.
And each scalar value (per point) is associated with a :class:`scalar type <pygplates.ScalarType>`.

Each scalar *type* belongs to one of two categories:

*  *Built-in scalar types*: whose scalar values *change* over time due to deformation

   These are:

   * ``pygplates.ScalarType.gpml_crustal_thickness`` - see :ref:`pygplates_primer_reconstructed_geometry_time_span_crustal_thickness_factors`
   * ``pygplates.ScalarType.gpml_crustal_stretching_factor`` - see :ref:`pygplates_primer_reconstructed_geometry_time_span_crustal_thickness_factors`
   * ``pygplates.ScalarType.gpml_crustal_thinning_factor`` - see :ref:`pygplates_primer_reconstructed_geometry_time_span_crustal_thickness_factors`
   * ``pygplates.ScalarType.gpml_tectonic_subsidence`` - see :ref:`pygplates_primer_reconstructed_geometry_time_span_tectonic_subsidence`

   Scalar values for these scalar types are always available.
   
   And their initial scalar values have default values (at the initial time).
   Hence the *initial_scalars* argument of :meth:`pygplates.TopologicalModel.reconstruct_geometry` does not need to be specified.

*  *User-defined scalar types*: whose scalar values are *constant* over time

   .. note:: Even though these scalar values are constant (over time) they still get deactivated when their associated
      geometry points get deactivated.

   These can be any :class:`pygplates.ScalarType` that you define.
   They are simply a way to associate your own data with the reconstructed geometry points.

   Scalar values for these scalar types are *only* available if you define them
   (using the *initial_scalars* argument of :meth:`pygplates.TopologicalModel.reconstruct_geometry`).
   For example:
   ::

      # Define your own scalar types.
      my_scalar_type_0 = pygplates.ScalarType.create_gpml('MyScalarType_0')
      my_scalar_type_1 = pygplates.ScalarType.create_gpml('MyScalarType_1')

      # Define associated scalar values (one per geometry point per scalar type).
      my_scalar_type_0_values = [...]
      my_scalar_type_1_values = [...]

      reconstructed_geometry_time_span = topological_model.reconstruct_geometry(
            initial_points,
            initial_time = 100,
            # dict with two entries (each entry mapping a scalar type to its scalar values)...
            initial_scalars = { my_scalar_type_0 : my_scalar_type_0_values, my_scalar_type_1 : my_scalar_type_1_values })

   .. note:: The *built-in scalar types* are still available when *user-defined scalar types* are defined.

The scalar values of each reconstructed geometry point at a reconstruction time can be queried using
:meth:`pygplates.ReconstructedGeometryTimeSpan.get_scalar_values`. By default this will return a ``dict`` mapping *all* scalar types
(built-in and any user-defined) to their scalar values. For example:
::

   # Get all active scalar values (associated with all built-in and user-defined scalar types).
   active_scalar_values = reconstructed_geometry_time_span.get_scalar_values(reconstruction_time)

   # If none of the points are active at 'reconstruction_time' then this will be 'None'.
   if active_scalar_values:
      
      # Extract the scalar values associated with the built-in scalar types.
      crustal_thicknesses_in_kms = active_scalar_values[pygplates.ScalarType.gpml_crustal_thickness]
      crustal_stretching_factors = active_scalar_values[pygplates.ScalarType.gpml_crustal_stretching_factor]
      crustal_thinning_factors = active_scalar_values[pygplates.ScalarType.gpml_crustal_thinning_factor]
      tectonic_subsidences = active_scalar_values[pygplates.ScalarType.gpml_tectonic_subsidence]

      # Extract the scalar values associated with the user-defined scalar types.
      my_active_scalar_values_0 = active_scalar_values[my_scalar_type_0]
      my_active_scalar_values_1 = active_scalar_values[my_scalar_type_1]

.. note:: Alternatively, you can specify a scalar type directly to :meth:`pygplates.ReconstructedGeometryTimeSpan.get_scalar_values`.
   For example:
   ::
   
      my_active_scalar_values_0 = reconstructed_geometry_time_span.get_scalar_values(
            reconstruction_time,
            my_scalar_type_0)

A scalar value (per scalar type) is returned for each geometry point that is *active at the requested reconstruction time*
(see :ref:`pygplates_primer_reconstructed_geometry_time_span_geometry_points`). If *none* of the points are active then ``None`` will be returned. 

If the requested reconstruction time is *within* the :meth:`time range <pygplates.ReconstructedGeometryTimeSpan.get_time_span>` of the snapshots
then the returned scalar values are those of the geometry points in the time slot (of the two time slots nearest the reconstruction time)
that is closest to the initial time (specified in :meth:`pygplates.TopologicalModel.reconstruct_geometry`).

.. note:: This is at the time of the time slot (rather than the reconstruction time), so this is more like a nearest neighbour interpolation
   (rather than a linear interpolation) of the two nearest time slots. This only matters for built-in scalar types since user-defined
   scalar types are constant over time.

If the requested reconstruction time is *outside* the :meth:`time range <pygplates.ReconstructedGeometryTimeSpan.get_time_span>` of the snapshots then the
returned scalar values will be from the oldest time slot (if the requested reconstruction time is older) or from the youngest time slot
(if the requested reconstruction time is younger).

.. _pygplates_primer_reconstructed_geometry_time_span_crustal_thickness_factors:

Crustal thickness factors
*************************

The crustal thickness factor :math:`F(t) = \frac{T(t)}{T(t_{initial})}` measures the ratio of the crustal thickness
at a reconstruction time :math:`T(t)` to the initial crustal thickness at the initial time :math:`T(t_{initial})`.
It is only calculated internally, and always has a value of ``1.0`` at the initial time (:math:`F(t_{initial}) = 1.0`).

Publicly, there are three built-in :ref:`scalar values <pygplates_primer_reconstructed_geometry_time_span_scalar_values>`
that depend on the internal crustal thickness factor :math:`F(t)`:

*  *Crustal thickness* (in kms): :math:`T(t)`

   The crustal thickness is calculated as:

   :math:`T(t) = F(t) \, T(t_{initial})`

   By default, the initial crustal thickness :math:`T(t_{initial})` is ``40`` kms.
   But you can specify a different value for each initial geometry point:
   ::

      # Specify one initial crustal thickness (in kms) per initial point.
      initial_crustal_thicknesses_in_kms = [...]

      reconstructed_geometry_time_span = topological_model.reconstruct_geometry(
            initial_points,
            initial_time = 100,
            # dict with a single entry that maps the crustal thickness scalar type to initial values...
            initial_scalars = { pygplates.ScalarType.gpml_crustal_thickness : initial_crustal_thicknesses_in_kms })
   
   The crustal thicknesses can be queried at any reconstruction time using
   :meth:`pygplates.ReconstructedGeometryTimeSpan.get_crustal_thicknesses`:
   ::

      reconstructed_crustal_thicknesses_in_kms = reconstructed_geometry_time_span.get_crustal_thicknesses(
            reconstruction_time)

      # If none of the points are active at 'reconstruction_time' then this will be 'None'.
      if reconstructed_crustal_thicknesses_in_kms:
         ...

   .. note:: This is the equivalent of:
      ::
      
         reconstructed_crustal_thicknesses_in_kms = reconstructed_geometry_time_span.get_scalar_values(
               reconstruction_time,
               pygplates.ScalarType.gpml_crustal_thickness)
         ...

*  *Crustal stretching factor*: :math:`\beta(t) = \frac{T(t_{initial})}{T(t)}`

   By default, the initial crustal stretching factor :math:`\beta(t_{initial})` is ``1``.
   And so the crustal stretching factor is calculated as:

   :math:`\beta(t) = \frac{1}{F(t)}`

   But you can specify a different :math:`\beta(t_{initial})` value for *each* initial geometry point:
   ::

      # Specify one initial crustal stretching factor per initial point.
      initial_crustal_stretching_factors = [...]

      reconstructed_geometry_time_span = topological_model.reconstruct_geometry(
            initial_points,
            initial_time = 100,
            # dict with a single entry that maps the crustal stretching factor scalar type to initial values...
            initial_scalars = { pygplates.ScalarType.gpml_crustal_stretching_factor : initial_crustal_stretching_factors })

   ...where each crustal stretching factor is then calculated as:

   :math:`\beta(t) = \frac{\beta(t_{initial})}{F(t)}`
   
   The crustal stretching factors can be queried at any reconstruction time using
   :meth:`pygplates.ReconstructedGeometryTimeSpan.get_crustal_stretching_factors`:
   ::

      reconstructed_crustal_stretching_factors = reconstructed_geometry_time_span.get_crustal_stretching_factors(
            reconstruction_time)

      # If none of the points are active at 'reconstruction_time' then this will be 'None'.
      if reconstructed_crustal_stretching_factors:
         ...

   .. note:: This is the equivalent of:
      ::
      
         reconstructed_crustal_stretching_factors = reconstructed_geometry_time_span.get_scalar_values(
               reconstruction_time,
               pygplates.ScalarType.gpml_crustal_stretching_factor)
         ...

*  *Crustal thinning factor*: :math:`\gamma(t) = 1 - \frac{T(t)}{T(t_{initial})}`

   By default, the initial crustal thinning factor :math:`\gamma(t_{initial})` is ``0``.
   And so the crustal thinning factor is calculated as:

   :math:`\gamma(t) = 1 - F(t)`

   But you can specify a different :math:`\gamma(t_{initial})` value for *each* initial geometry point:
   ::

      # Specify one initial crustal thinning factor per initial point.
      initial_crustal_thinning_factors = [...]

      reconstructed_geometry_time_span = topological_model.reconstruct_geometry(
            initial_points,
            initial_time = 100,
            # dict with a single entry that maps the crustal thinning factor scalar type to initial values...
            initial_scalars = { pygplates.ScalarType.gpml_crustal_thinning_factor : initial_crustal_thinning_factors })

   ...where each crustal thinning factor is then calculated as:

   :math:`\gamma(t) = 1 - (1 - \gamma(t_{initial})) \, F(t)`
   
   The crustal thinning factors can be queried at any reconstruction time using
   :meth:`pygplates.ReconstructedGeometryTimeSpan.get_crustal_thinning_factors`:
   ::

      reconstructed_crustal_thinning_factors = reconstructed_geometry_time_span.get_crustal_thinning_factors(
            reconstruction_time)

      # If none of the points are active at 'reconstruction_time' then this will be 'None'.
      if reconstructed_crustal_thinning_factors:
         ...

   .. note:: This is the equivalent of:
      ::
      
         reconstructed_crustal_thinning_factors = reconstructed_geometry_time_span.get_scalar_values(
               reconstruction_time,
               pygplates.ScalarType.gpml_crustal_thinning_factor)
         ...

.. seealso:: :ref:`pygplates_primer_reconstructed_geometry_time_span_scalar_values`, for more details on how scalar values are queried when the
   requested reconstruction time is *inside* or *outside* the :meth:`time range <pygplates.ReconstructedGeometryTimeSpan.get_time_span>` of the snapshots.

.. _pygplates_primer_reconstructed_geometry_time_span_tectonic_subsidence:

Tectonic subsidence
*******************

Tectonic subsidence is one of the built-in :ref:`scalar values <pygplates_primer_reconstructed_geometry_time_span_scalar_values>` that evolves over time due to deformation.
By default, the initial tectonic subsidence (at the initial time) is zero for each geometry point.
However you can optionally specify your own initial tectonic subsidence values (using the *initial_scalars* argument of :meth:`pygplates.TopologicalModel.reconstruct_geometry`):
::

   # Specify one initial tectonic subsidence value (in kms) per initial point.
   initial_tectonic_subsidences_in_kms = [...]

   reconstructed_geometry_time_span = topological_model.reconstruct_geometry(
         initial_points,
         initial_time = 100,
         # dict with a single entry that maps the tectonic subsidence scalar type to initial values...
         initial_scalars = { pygplates.ScalarType.gpml_tectonic_subsidence : initial_tectonic_subsidences_in_kms })

.. note:: The default (zero tectonic subsidence at the initial time) does not require the ``initial_scalars`` argument to be specified.

The tectonic subsidence of each reconstructed geometry point at any reconstruction time can then be queried using
:meth:`pygplates.ReconstructedGeometryTimeSpan.get_tectonic_subsidences`:
::

   reconstructed_tectonic_subsidences_in_kms = reconstructed_geometry_time_span.get_tectonic_subsidences(
         reconstruction_time)

   # If none of the points are active at 'reconstruction_time' then this will be 'None'.
   if reconstructed_tectonic_subsidences_in_kms:
      ...

.. note:: This is the equivalent of:
   ::
   
      reconstructed_tectonic_subsidences_in_kms = reconstructed_geometry_time_span.get_scalar_values(
            reconstruction_time,
            pygplates.ScalarType.gpml_tectonic_subsidence)
      ...

.. seealso:: :ref:`pygplates_primer_reconstructed_geometry_time_span_scalar_values`, for more details on how scalar values are queried when the
   requested reconstruction time is *inside* or *outside* the :meth:`time range <pygplates.ReconstructedGeometryTimeSpan.get_time_span>` of the snapshots.
