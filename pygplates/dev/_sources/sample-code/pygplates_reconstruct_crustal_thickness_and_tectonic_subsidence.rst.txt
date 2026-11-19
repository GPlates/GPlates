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

Data files
""""""""""

``topologies.gpml``
    Topological plate polygon and/or deforming network features. The crustal stretching factor and
    tectonic subsidence of a point only change while it is inside a deforming network.

``rotations.rot``
    A rotation file. It must contain rotations for the plate IDs of the topological features.

Files of these kinds are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_reconstruct_crustal_thickness_and_tectonic_subsidence.py

Details
"""""""

| First create a :class:`topological model<pygplates.TopologicalModel>` from topological features and rotation files.
| The topological features can be plate polygons and/or deforming networks.
| More than one file containing topological features can be specified here, however we're only specifying one file.
| Also note that more than one rotation file (or even a single :class:`pygplates.RotationModel`) can be specified here,
  however we're only specifying a single rotation file.

.. sample-code:: pygplates_reconstruct_crustal_thickness_and_tectonic_subsidence.py
   :fragment: create-topological-model

Next we get our topological model to reconstruct (and deform) our ``initial_points`` from their position at ``initial_time``
to present day in 1 Myr intervals using :meth:`pygplates.TopologicalModel.reconstruct_geometry`. This returns a
:class:`reconstructed geometry time span<pygplates.ReconstructedGeometryTimeSpan>` containing a history of the reconstructed
point positions and their associated scalar values, such as crustal stretching factor and tectonic subsidence, that change over time
when passing through deforming networks. Since we did not specify the *initial_scalars* parameter, the initial crustal stretching factors
and initial tectonic subsidences will have default values (of 1.0 and 0.0 respectively) indicating no initial stretching and sea-level subsidence.
And since we did not specify the *deactivate_points* parameter, the default deactivation is used which matches GPlates
(when using 'reconstruct by topologies' in a green visual layer). This means any initial points on oceanic crust could get subducted and
disappear (get deactivated).

.. sample-code:: pygplates_reconstruct_crustal_thickness_and_tectonic_subsidence.py
   :fragment: reconstruct-geometry

Get the reconstructed positions at the current ``time``. Note that some of the initial points can be deactivated, in which case the number
of points in ``reconstructed_points`` could be less than ``initial_points``.

.. sample-code:: pygplates_reconstruct_crustal_thickness_and_tectonic_subsidence.py
   :fragment: reconstructed-points

Query in which resolved topologies the reconstructed positions are located at the current ``time``.
Note that the number of values in ``reconstructed_topology_point_locations`` matches the number of points in ``reconstructed_points``.

.. sample-code:: pygplates_reconstruct_crustal_thickness_and_tectonic_subsidence.py
   :fragment: topology-point-locations

Extract the reconstructed/deformed crustal stretching factor and tectonic subsidence (in kms) for each point in ``reconstructed_points``.
Note that :meth:`pygplates.ReconstructedGeometryTimeSpan.get_scalar_values` returns a Python dictionary mapping scalar types to their scalar values
(unless a specific scalar type is specified). From that dictionary we then extract the crustal stretching factor and tectonic subsidence values.
Also note that these builtin scalar types are always available (even when no initial scalar values are provided).

.. sample-code:: pygplates_reconstruct_crustal_thickness_and_tectonic_subsidence.py
   :fragment: scalar-values

See also
""""""""

- Primer: :ref:`pygplates_primer_using_topological_reconstruction`, :ref:`pygplates_primer_reconstructed_geometry_time_span_scalar_values`,
  :ref:`pygplates_primer_reconstructed_geometry_time_span_crustal_thickness_factors`,
  :ref:`pygplates_primer_reconstructed_geometry_time_span_tectonic_subsidence`
- Reference: :meth:`pygplates.TopologicalModel.reconstruct_geometry`, :class:`pygplates.ReconstructedGeometryTimeSpan`,
  :meth:`pygplates.ReconstructedGeometryTimeSpan.get_scalar_values`, :class:`pygplates.ScalarType`
- Sample code: :ref:`pygplates_reconstruct_strain_and_strain_rate`
