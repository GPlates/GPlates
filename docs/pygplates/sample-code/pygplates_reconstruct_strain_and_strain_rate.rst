.. _pygplates_reconstruct_strain_and_strain_rate:

Reconstruct strain and strain rate
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example reconstructs and deforms points through topological plate polygons and deforming
networks, from a past geological time to present day. At each time along the way it finds:

- the reconstructed positions of the initial points,
- their accumulated strains (since the initial time), and
- their instantaneous strain rates and velocities.

This is the equivalent of the *deformation export* in GPlates.

.. contents::
   :local:
   :depth: 2

Data files
""""""""""

``topologies.gpml``
    Topological plate polygon and deforming network features. Strain only accumulates, and the strain
    rate is only non-zero, while a point is inside a deforming network.

``rotations.rot``
    A rotation file. It must contain rotations for the plate IDs of the topological features.

Files of these kinds are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_reconstruct_strain_and_strain_rate.py

Details
"""""""

A :class:`topological model<pygplates.TopologicalModel>` is created from the topological features
(plate polygons and deforming networks) and the rotation file.

Here there is one file of topological features and one rotation file. More than one of each can be
given, and a :class:`pygplates.RotationModel` can be given in place of the rotation files.

.. sample-code:: pygplates_reconstruct_strain_and_strain_rate.py
   :fragment: create-topological-model

:meth:`pygplates.TopologicalModel.reconstruct_geometry` reconstructs (and deforms) the
``initial_points`` from their positions at ``initial_time`` to present day, in 1 Myr intervals.

It returns a :class:`reconstructed geometry time span<pygplates.ReconstructedGeometryTimeSpan>`.
This holds the history of the reconstructed point positions, and of the strains and strain rates
at the points.

The strain and strain rate of a point change while it is passing through a deforming network. A
point in a rigid plate (outside all deforming networks) has a strain rate of zero, so it
accumulates no strain there.

The *deactivate_points* argument is not given. So points are deactivated in the default way,
which is the way that GPlates does it (with ``Reconstruct using topologies`` in a green layer).
This means an initial point on oceanic crust can be subducted and disappear (be deactivated).

.. sample-code:: pygplates_reconstruct_strain_and_strain_rate.py
   :fragment: reconstruct-geometry

:meth:`pygplates.ReconstructedGeometryTimeSpan.get_geometry_points` returns the reconstructed
positions at the current ``time``. Some of the initial points may have been deactivated by then,
so ``reconstructed_points`` can have fewer points than ``initial_points``.

.. sample-code:: pygplates_reconstruct_strain_and_strain_rate.py
   :fragment: reconstructed-points

If *all* the points have been deactivated by the current ``time`` then
:meth:`pygplates.ReconstructedGeometryTimeSpan.get_geometry_points` returns ``None`` rather than an empty list,
and so do the other queries below. So the script tests for that first, and skips the current ``time``.

.. sample-code:: pygplates_reconstruct_strain_and_strain_rate.py
   :fragment: no-active-points

Each of the queries below returns a list with one value for each point in ``reconstructed_points``,
in the same order.

:meth:`pygplates.ReconstructedGeometryTimeSpan.get_topology_point_locations` returns the resolved
topology (rigid plate or deforming network) that each reconstructed position is in at the current
``time``.

.. sample-code:: pygplates_reconstruct_strain_and_strain_rate.py
   :fragment: topology-point-locations

:meth:`pygplates.ReconstructedGeometryTimeSpan.get_velocities` returns the velocity of each point.
Here the velocities are in cms/yr, and are calculated over a 1 Myr interval from ``time+1`` to ``time``.

.. sample-code:: pygplates_reconstruct_strain_and_strain_rate.py
   :fragment: velocities

:meth:`pygplates.ReconstructedGeometryTimeSpan.get_strains` returns the accumulated strain of each
point at the current ``time``, and :meth:`pygplates.ReconstructedGeometryTimeSpan.get_strain_rates`
returns its instantaneous strain rate.

.. sample-code:: pygplates_reconstruct_strain_and_strain_rate.py
   :fragment: strains-and-strain-rates

For each point, the script then gets several quantities from its strain and its strain rate.
:class:`pygplates.Strain` and :class:`pygplates.StrainRate` define these quantities.

The :meth:`dilatation<pygplates.Strain.get_dilatation>` is the change in crustal area at the
point, as a proportion of the initial area. It is positive where the crust has expanded and
negative where it has contracted.

.. sample-code:: pygplates_reconstruct_strain_and_strain_rate.py
   :fragment: dilatation

The :meth:`principal strains<pygplates.Strain.get_principal_strain>` are the maximum and minimum
strains, which are along the major and minor principal axes. Each is the change in length per
unit initial length (since the initial time) in the direction of its axis.

The angle (in radians) is the direction of the *major* principal axis. With
``pygplates.PrincipalAngleType.major_azimuth`` it is measured clockwise from North, in the range
:math:`[0,2\pi]`.

.. sample-code:: pygplates_reconstruct_strain_and_strain_rate.py
   :fragment: principal-strain

The :meth:`dilatation rate<pygplates.StrainRate.get_dilatation_rate>` is the *rate* of change of
crustal area per unit area at the point, in units of 1/second. It is positive where the crust is
expanding and negative where it is contracting.

.. sample-code:: pygplates_reconstruct_strain_and_strain_rate.py
   :fragment: dilatation-rate

The :meth:`total strain rate<pygplates.StrainRate.get_total_strain_rate>` is the magnitude of the
strain rate, in units of 1/second. It includes both the normal (extension and compression) and
the shear components.

.. sample-code:: pygplates_reconstruct_strain_and_strain_rate.py
   :fragment: total-strain-rate

The :meth:`strain rate style<pygplates.StrainRate.get_strain_rate_style>` categorises the type of
deformation. A value of ``-1`` represents contraction (eg, pure reverse faulting), ``0`` represents
pure strike-slip faulting and ``1`` represents extension (eg, pure normal faulting). It is ``NaN``
where the strain rate is zero, as it is for every point on a rigid plate.

.. sample-code:: pygplates_reconstruct_strain_and_strain_rate.py
   :fragment: strain-rate-style

See also
""""""""

- Primer: :ref:`pygplates_primer_using_topological_reconstruction`, :ref:`pygplates_primer_reconstructed_geometry_time_span`,
  :ref:`pygplates_primer_reconstructed_geometry_time_span_strain_rates`,
  :ref:`pygplates_primer_reconstructed_geometry_time_span_strains`, :ref:`pygplates_primer_deformation`
- Reference: :meth:`pygplates.TopologicalModel.reconstruct_geometry`, :class:`pygplates.ReconstructedGeometryTimeSpan`,
  :class:`pygplates.Strain`, :class:`pygplates.StrainRate`
- Sample code: :ref:`pygplates_reconstruct_crustal_thickness_and_tectonic_subsidence`,
  :ref:`pygplates_sample_intra-plate_strain_rates_at_subduction_zones`, :ref:`pygplates_reconstruct_points_step_by_step_through_topologies`
