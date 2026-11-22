.. _pygplates_sample_intra-plate_strain_rates_at_subduction_zones:

Sample strain rates inside the overriding plate at subduction zones
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example calculates strain rates just inside the overriding plate along subduction zones, over
a series of geological times. At each time it:

- generates sample points along the plate boundaries that are subduction zones,
- determines which side of each subduction zone the overriding plate is on,
- moves each sample point 50 kms into the overriding plate (along the normal to the plate boundary), and
- samples the strain rate of the overriding plate at the moved point.

.. contents::
   :local:
   :depth: 2

Data files
""""""""""

``topologies.gpml``
    Topological plate polygon and deforming network features, and the boundary section features that
    they reference. Subduction zones are recognised by the feature type
    (:attr:`pygplates.FeatureType.gpml_subduction_zone`) of the boundary section features, and each
    needs a subduction polarity of ``'Left'`` or ``'Right'`` (others are skipped). Strain rates are
    only non-zero where the overriding plate is a deforming network.

``rotations.rot``
    A rotation file. It must contain rotations for the plate IDs of the topological features.

Files of these kinds are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_sample_intra-plate_strain_rates_at_subduction_zones.py

Details
"""""""

A :class:`topological model<pygplates.TopologicalModel>` is created from the topological features
and the rotation file. The topological features can be plate polygons, deforming networks or both.

Here there is one file of topological features and one rotation file. More than one of each can be
given, and a :class:`pygplates.RotationModel` can be given in place of the rotation files.

.. sample-code:: pygplates_sample_intra-plate_strain_rates_at_subduction_zones.py
   :fragment: create-topological-model

.. note:: The :class:`pygplates.TopologicalModel` is created **outside** the time loop, since it does not depend on ``time``.

:meth:`pygplates.TopologicalModel.topological_snapshot` resolves the topological features at the
current ``time``, and returns the result as a :class:`pygplates.TopologicalSnapshot`.

.. sample-code:: pygplates_sample_intra-plate_strain_rates_at_subduction_zones.py
   :fragment: topological-snapshot

:meth:`pygplates.TopologicalSnapshot.calculate_plate_boundary_statistics` generates sample points
along the plate boundaries, and returns a :class:`pygplates.PlateBoundaryStatistic` for each point.
Here the points are spaced 0.5 degrees apart.

With *boundary_section_filter* set to :attr:`pygplates.FeatureType.gpml_subduction_zone`, there are
only points along the boundary sections that are subduction zones.

With *return_shared_sub_segment_dict* set to ``True``, the statistics are returned in a dictionary.
It maps each :class:`shared sub-segment<pygplates.ResolvedTopologicalSharedSubSegment>` to the list
of statistics of the sample points along it.

.. sample-code:: pygplates_sample_intra-plate_strain_rates_at_subduction_zones.py
   :fragment: plate-boundary-statistics

The script iterates over the shared sub-segments. Each has the list of statistics of its sample points.

.. sample-code:: pygplates_sample_intra-plate_strain_rates_at_subduction_zones.py
   :fragment: iterate-shared-sub-segments

The subduction polarity of the feature of the shared sub-segment (a subducting line) says which
side of the subducting line the overriding plate is on. ``'Left'`` means the overriding plate is on
the left when following the vertices of the subducting line in order, and ``'Right'`` means it is on
the right.

A subducting line with no subduction polarity, or a polarity of ``'Unknown'``, is skipped.

.. sample-code:: pygplates_sample_intra-plate_strain_rates_at_subduction_zones.py
   :fragment: subduction-polarity

The script then iterates over the :class:`statistics<pygplates.PlateBoundaryStatistic>` of the
sample points along the current shared sub-segment.

.. sample-code:: pygplates_sample_intra-plate_strain_rates_at_subduction_zones.py
   :fragment: iterate-statistics

Each statistic has the :class:`topology location<pygplates.TopologyPointLocation>` of its sample
point on each side of the boundary: the plate (or network) on the left, and the one on the right.
The overriding plate is the one on the side given by the subduction polarity.

.. sample-code:: pygplates_sample_intra-plate_strain_rates_at_subduction_zones.py
   :fragment: overriding-plate

Strain rates are only non-zero in *deforming* networks. So if the overriding plate is not a
resolved *network* at the sample point, the strain rate there is
:attr:`zero<pygplates.StrainRate.zero>`.

::

    overriding_resolved_network = overriding_plate.located_in_resolved_network()
    if overriding_resolved_network:
        ...
    else:
        overriding_strain_rate = pygplates.StrainRate.zero

If the overriding plate is a deforming network, the sample point is moved 50 kms *off* the plate
boundary (the subduction zone), into the interior of the network.

The point is moved along the normal to the boundary by rotating it. This needs a rotation *pole*
that is orthogonal to both the boundary point and the boundary normal, which is their cross
product.

.. sample-code:: pygplates_sample_intra-plate_strain_rates_at_subduction_zones.py
   :fragment: off-boundary-rotation-pole

The boundary normal points to the *left* side of the boundary. So if the overriding plate is on the
right side, the pole is reversed, which reverses the direction of rotation.

.. sample-code:: pygplates_sample_intra-plate_strain_rates_at_subduction_zones.py
   :fragment: reverse-rotation-pole

The finite rotation that moves a point 50 kms has that pole, and an angle (in radians) of 50 kms
divided by the Earth's radius.

.. sample-code:: pygplates_sample_intra-plate_strain_rates_at_subduction_zones.py
   :fragment: off-boundary-rotation

The rotation is applied to the sample point, to move it *into* the overriding plate.

.. sample-code:: pygplates_sample_intra-plate_strain_rates_at_subduction_zones.py
   :fragment: off-boundary-point

:meth:`pygplates.ResolvedTopologicalNetwork.get_point_strain_rate` then samples the strain rate
of the overriding deforming network at the off-boundary point.

The off-boundary point might no longer be *inside* the network. This can happen where the network
is narrow, or where the sample point is close to another edge of the network. The method then
returns ``None``, and the script uses a strain rate of zero.

.. sample-code:: pygplates_sample_intra-plate_strain_rates_at_subduction_zones.py
   :fragment: sample-strain-rate

Once the strain rates have been sampled at the current ``time``, a
:class:`feature<pygplates.Feature>` is created to hold the boundary points and the
:meth:`dilatation rates<pygplates.StrainRate.get_dilatation_rate>` of their strain rates.

The feature is given a valid time that extends half a million years either side of the current
``time``. So when the features of all the times are loaded into GPlates, each one is only visible
at its own time.

Its :meth:`geometry is set<pygplates.Feature.set_geometry>` as a coverage: a multipoint, and a
scalar value at each of its points. GPlates displays the scalar values, here the dilatation rates,
in a separate layer.

.. sample-code:: pygplates_sample_intra-plate_strain_rates_at_subduction_zones.py
   :fragment: create-strain-rate-feature

Finally the features are written to a file. It has the points along subduction zones, and their
dilatation rates, at all times.

.. sample-code:: pygplates_sample_intra-plate_strain_rates_at_subduction_zones.py
   :fragment: write-output

The file can now be loaded into GPlates, along with the topological model it was generated from,
to see the strain rates inside the overriding plates along subduction zones.

See also
""""""""

- Primer: :ref:`pygplates_primer_plate_boundary_statistics`, :ref:`pygplates_primer_topological_network`,
  :ref:`pygplates_primer_strain_rates_in_triangulation`
- Reference: :meth:`pygplates.TopologicalSnapshot.calculate_plate_boundary_statistics`, :class:`pygplates.PlateBoundaryStatistic`,
  :class:`pygplates.TopologyPointLocation`, :meth:`pygplates.ResolvedTopologicalNetwork.get_point_strain_rate`,
  :class:`pygplates.StrainRate`
- Sample code: :ref:`pygplates_find_divergence_at_subduction_zones_and_convergence_at_ridges`,
  :ref:`pygplates_find_overriding_plate_of_closest_subducting_line`, :ref:`pygplates_reconstruct_strain_and_strain_rate`,
  :ref:`pygplates_find_distance_and_strain_rate_along_subduction_zones`
