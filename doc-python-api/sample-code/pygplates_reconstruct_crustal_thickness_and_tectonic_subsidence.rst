.. _pygplates_reconstruct_crustal_thickness_and_tectonic_subsidence:

Reconstruct crustal thickness and tectonic subsidence
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example reconstructs (and deforms) points through topological plate polygons (and deforming networks) to find
the following quantities over a time period spanning a past geological time to present day:

- the reconstructed positions of the initial points,
- their crustal stretching factors, and
- their tectonic subsidences.

.. contents::
   :local:
   :depth: 2

Sample code
"""""""""""

::

    import pygplates


    # Create a topological model from our topological features (can be plate polygons and/or deforming networks)
    # and rotation file(s).
    topological_model = pygplates.TopologicalModel('topologies.gpml', 'rotations.rot')

    # Our reconstruction will span from 250Ma to present day in 1 Myr intervals.
    initial_time = 250
    time_increment = 1

    # Reconstruct the initial points through the topological model from initial time to present day.
    reconstructed_time_span = topological_model.reconstruct_geometry(
            initial_points,
            initial_time,
            time_increment=time_increment)

    # Get the history of reconstructed positions, crustal stretching and tectonic subsidence of the initial points from initial time to present day.
    for time in range(initial_time, -1, -time_increment):  # initial_time, initial_time - time_increment, ..., 0
        # Reconstructed positions at the current 'time'.
        reconstructed_points = reconstructed_time_span.get_geometry_points(time)

        # In which resolved topologies are the reconstructed positions located at the current 'time'.
        reconstructed_topology_point_locations = reconstructed_time_span.get_topology_point_locations(time)

        # The crustal stretching factor and tectonic subsidence (in kms) for each point at the current 'time'.
        reconstructed_scalar_values = reconstructed_time_span.get_scalar_values(time)
        reconstructed_crustal_stretching_factors = reconstructed_scalar_values[pygplates.ScalarType.gpml_crustal_stretching_factor]
        reconstructed_tectonic_subsidences = reconstructed_scalar_values[pygplates.ScalarType.gpml_tectonic_subsidence]


Details
"""""""

| First create a :class:`topological model<pygplates.TopologicalModel>` from topological features and rotation files.
| The topological features can be plate polygons and/or deforming networks.
| More than one file containing topological features can be specified here, however we're only specifying one file.
| Also note that more than one rotation file (or even a single :class:`pygplates.RotationModel`) can be specified here,
  however we're only specifying a single rotation file.

::

    topological_model = pygplates.TopologicalModel('topologies.gpml', 'rotations.rot')

Next we get our topological model to reconstruct (and deform) our ``initial_points`` from their position at ``initial_time``
to present day in 1 Myr intervals using :meth:`pygplates.TopologicalModel.reconstruct_geometry`. This returns a
:class:`reconstructed geometry time span<pygplates.ReconstructedGeometryTimeSpan>` containing a history of the reconstructed
point positions and their associated scalar values, such as crustal stretching factor and tectonic subsidence, that change over time
when passing through deforming networks. Since we did not specify the *initial_scalars* parameter, the initial crustal stretching factors
and initial tectonic subsidences will have default values (of 1.0 and 0.0 respectively) indicating no initial stretching and sea-level subsidence.
And since we did not specify the *deactivate_points* parameter, the default deactivation is used which matches GPlates
(when using 'reconstruct by topologies' in a green visual layer). This means any initial points on oceanic crust could get subducted and
disappear (get deactivated).
::

    reconstructed_time_span = topological_model.reconstruct_geometry(
            initial_points,
            initial_time,
            time_increment=time_increment)

Get the reconstructed positions at the current ``time``. Note that some of the initial points can be deactivated, in which case the number
of points in ``reconstructed_points`` could be less than ``initial_points``.
::

    reconstructed_points = reconstructed_time_span.get_geometry_points(time)

Query in which resolved topologies the reconstructed positions are located at the current ``time``.
Note that the number of values in ``reconstructed_topology_point_locations`` matches the number of points in ``reconstructed_points``.
::

    reconstructed_topology_point_locations = reconstructed_time_span.get_topology_point_locations(time)

Extract the reconstructed/deformed crustal stretching factor and tectonic subsidence (in kms) for each point in ``reconstructed_points``.
Note that :meth:`pygplates.ReconstructedGeometryTimeSpan.get_scalar_values` returns a Python dictionary mapping scalar types to their scalar values
(unless a specific scalar type is specified). From that dictionary we then extract the crustal stretching factor and tectonic subsidence values.
Also note that these builtin scalar types are always available (even when no initial scalar values are provided).
::

    reconstructed_scalar_values = reconstructed_time_span.get_scalar_values(time)
    reconstructed_crustal_stretching_factors = reconstructed_scalar_values[pygplates.ScalarType.gpml_crustal_stretching_factor]
    reconstructed_tectonic_subsidences = reconstructed_scalar_values[pygplates.ScalarType.gpml_tectonic_subsidence]
