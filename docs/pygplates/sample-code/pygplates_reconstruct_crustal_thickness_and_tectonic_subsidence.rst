.. _pygplates_reconstruct_crustal_thickness_and_tectonic_subsidence:

Reconstruct crustal thickness and tectonic subsidence
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example reconstructs (and deforms) points through topological plate polygons (and deforming
networks), from a past geological time to present day. At each time along the way it finds:

- the reconstructed positions of the initial points,
- their crustal thicknesses,
- their crustal stretching factors, and
- their tectonic subsidences.

.. contents::
   :local:
   :depth: 2

Data files
""""""""""

``topologies.gpml``
    Topological plate polygon and/or deforming network features. The crustal thickness, crustal
    stretching factor and tectonic subsidence of a point only change while it is inside the deforming
    region of a network.

``rotations.rot``
    A rotation file. It must contain rotations for the plate IDs of the topological features.

Files of these kinds are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_reconstruct_crustal_thickness_and_tectonic_subsidence.py

Details
"""""""

A :class:`topological model<pygplates.TopologicalModel>` is created from the topological features
and the rotation file. The topological features can be plate polygons, deforming networks or both.

Here there is one file of topological features and one rotation file. More than one of each can be
given, and a :class:`pygplates.RotationModel` can be given in place of the rotation files.

.. sample-code:: pygplates_reconstruct_crustal_thickness_and_tectonic_subsidence.py
   :fragment: create-topological-model

:meth:`pygplates.TopologicalModel.reconstruct_geometry` reconstructs (and deforms) the
``initial_points`` from their positions at ``initial_time`` to present day, in 1 Myr intervals.

It returns a :class:`reconstructed geometry time span<pygplates.ReconstructedGeometryTimeSpan>`.
This holds the history of the reconstructed point positions and of the scalar values at the points,
such as crustal thickness and tectonic subsidence, which change while a point is passing through a
deforming network.

The *initial_scalars* argument is not given. So the initial crustal thicknesses, crustal stretching
factors and tectonic subsidences have their default values, of 40 kms, 1.0 and 0.0. The last two
mean that there is no stretching, and that the subsidence is at sea level, at the initial time.

The *deactivate_points* argument is not given either. So points are deactivated in the default
way, which is the way that GPlates does it (with ``Reconstruct using topologies`` in a green
layer). This means an initial point on oceanic crust can be subducted and disappear (be
deactivated).

.. sample-code:: pygplates_reconstruct_crustal_thickness_and_tectonic_subsidence.py
   :fragment: reconstruct-geometry

:meth:`pygplates.ReconstructedGeometryTimeSpan.get_geometry_points` returns the reconstructed
positions at the current ``time``. Some of the initial points may have been deactivated by then,
so ``reconstructed_points`` can have fewer points than ``initial_points``.

.. sample-code:: pygplates_reconstruct_crustal_thickness_and_tectonic_subsidence.py
   :fragment: reconstructed-points

If *all* the points have been deactivated by the current ``time`` then
:meth:`pygplates.ReconstructedGeometryTimeSpan.get_geometry_points` returns ``None`` rather than an empty list,
and so do the other queries below. So the script tests for that first.

.. sample-code:: pygplates_reconstruct_crustal_thickness_and_tectonic_subsidence.py
   :fragment: no-active-points

:meth:`pygplates.ReconstructedGeometryTimeSpan.get_topology_point_locations` returns the resolved
topology that each reconstructed position is in at the current ``time``. It returns one
:class:`location<pygplates.TopologyPointLocation>` for each point in ``reconstructed_points``, in
the same order. The script gets the locations but does not print them.

.. sample-code:: pygplates_reconstruct_crustal_thickness_and_tectonic_subsidence.py
   :fragment: topology-point-locations

The crustal thickness (in kms), the crustal stretching factor and the tectonic subsidence (in kms)
of each point in ``reconstructed_points`` are returned by
:meth:`pygplates.ReconstructedGeometryTimeSpan.get_crustal_thicknesses`,
:meth:`pygplates.ReconstructedGeometryTimeSpan.get_crustal_stretching_factors` and
:meth:`pygplates.ReconstructedGeometryTimeSpan.get_tectonic_subsidences`.

Each returns a list with one value for each point in ``reconstructed_points``, in the same order.

The crustal stretching factor is the initial crustal thickness divided by the crustal thickness at the current ``time``.
So crust that has been stretched to half its initial thickness has a crustal stretching factor of 2.

These three scalar types are builtin, so they are always available (even when no initial scalar values are provided).
Each method is the equivalent of calling :meth:`pygplates.ReconstructedGeometryTimeSpan.get_scalar_values`
with the corresponding :class:`scalar type<pygplates.ScalarType>`.

.. sample-code:: pygplates_reconstruct_crustal_thickness_and_tectonic_subsidence.py
   :fragment: crustal-thickness-and-subsidence

Finally the script prints the position, crustal thickness, crustal stretching factor and tectonic subsidence of each point.

.. sample-code:: pygplates_reconstruct_crustal_thickness_and_tectonic_subsidence.py
   :fragment: print-values

See also
""""""""

- Primer: :ref:`pygplates_primer_using_topological_reconstruction`, :ref:`pygplates_primer_reconstructed_geometry_time_span_scalar_values`,
  :ref:`pygplates_primer_reconstructed_geometry_time_span_crustal_thickness_factors`,
  :ref:`pygplates_primer_reconstructed_geometry_time_span_tectonic_subsidence`
- Reference: :meth:`pygplates.TopologicalModel.reconstruct_geometry`, :class:`pygplates.ReconstructedGeometryTimeSpan`,
  :meth:`pygplates.ReconstructedGeometryTimeSpan.get_crustal_thicknesses`,
  :meth:`pygplates.ReconstructedGeometryTimeSpan.get_crustal_stretching_factors`,
  :meth:`pygplates.ReconstructedGeometryTimeSpan.get_tectonic_subsidences`,
  :meth:`pygplates.ReconstructedGeometryTimeSpan.get_scalar_values`, :class:`pygplates.ScalarType`
- Sample code: :ref:`pygplates_reconstruct_strain_and_strain_rate`, :ref:`pygplates_reconstruct_points_step_by_step_through_topologies`
