.. _pygplates_find_divergence_at_subduction_zones_and_convergence_at_ridges:

Find divergence at subduction zones and convergence at ridges
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example finds points on plate boundaries, over a series of geological times:

- where the plates *converge* along boundary sections labelled as *mid-ocean ridges*, and
- where the plates *diverge* along boundary sections labelled as *subduction zones*.

This is useful for locating the parts of mid-ocean ridges that are **not** diverging as expected,
and the parts of subduction zones that are **not** converging as expected.

.. contents::
   :local:
   :depth: 2

Data files
""""""""""

``topologies.gpml``
    Topological plate polygon and/or deforming network features, and the boundary section features
    that they reference. Mid-ocean ridges and subduction zones are recognised by the feature type
    (:attr:`pygplates.FeatureType.gpml_mid_ocean_ridge` or
    :attr:`pygplates.FeatureType.gpml_subduction_zone`) of the boundary section features.

``rotations.rot``
    A rotation file. It must contain rotations for the plate IDs of the topological features.

Files of these kinds are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_find_divergence_at_subduction_zones_and_convergence_at_ridges.py

Details
"""""""

A :class:`topological model<pygplates.TopologicalModel>` is created from the topological features
and the rotation file. The topological features can be plate polygons, deforming networks or both.

Here there is one file of topological features and one rotation file. More than one of each can be
given, and a :class:`pygplates.RotationModel` can be given in place of the rotation files.

.. sample-code:: pygplates_find_divergence_at_subduction_zones_and_convergence_at_ridges.py
   :fragment: create-topological-model

.. note:: The :class:`pygplates.TopologicalModel` is created **outside** the time loop, since it does not depend on ``time``.

:meth:`pygplates.TopologicalModel.topological_snapshot` resolves the topological features at the
current ``time``, and returns the result as a :class:`pygplates.TopologicalSnapshot`.

.. sample-code:: pygplates_find_divergence_at_subduction_zones_and_convergence_at_ridges.py
   :fragment: topological-snapshot

Next a function is defined that finds the converging, or the diverging, points along one type of
plate boundary. It is a function so that very similar code is not written twice: once for
converging points and once for diverging points.

The function has three arguments:

- the feature type of the boundary sections to look at (mid-ocean ridges or subduction zones, for example),
- a velocity threshold, which the plates must converge or diverge faster than, and
- whether to find converging points (``True``) or diverging points (``False``).

.. sample-code:: pygplates_find_divergence_at_subduction_zones_and_convergence_at_ridges.py
   :fragment: define-function

The function first calls :meth:`pygplates.TopologicalSnapshot.calculate_plate_boundary_statistics`.
It generates points along the plate boundaries, and returns a
:class:`pygplates.PlateBoundaryStatistic` for each point.

Here the points are spaced 1 degree apart, and velocities are in centimetres per year. With
*boundary_section_filter* set to a :class:`feature type<pygplates.FeatureType>`, there are only
points along the boundary sections of that feature type.

.. sample-code:: pygplates_find_divergence_at_subduction_zones_and_convergence_at_ridges.py
   :fragment: plate-boundary-statistics

The function then iterates over the statistics, to collect the locations and convergence
velocities of the points it is looking for.

.. sample-code:: pygplates_find_divergence_at_subduction_zones_and_convergence_at_ridges.py
   :fragment: iterate-statistics

At some points there might be no plate (or network) on one side of the boundary. This happens
where the topological model does not cover the whole globe, or where there is an unintended gap
between plates. The convergence velocity is then ``float('nan')``, and the point is skipped.

.. sample-code:: pygplates_find_divergence_at_subduction_zones_and_convergence_at_ridges.py
   :fragment: skip-nan

``stat.convergence_velocity_signed_magnitude`` is a *signed* magnitude. It is positive where the
plates are converging and negative where they are diverging.

So a converging point is one where it is greater than the threshold. And a diverging point is
one where it is less than minus the threshold (the plates are diverging faster than the
threshold). The location and convergence velocity of each such point are added to two lists.

.. sample-code:: pygplates_find_divergence_at_subduction_zones_and_convergence_at_ridges.py
   :fragment: test-threshold

If any points were found, a :class:`feature<pygplates.Feature>` is created to hold them and their
convergence velocities.

The feature is given a valid time that extends half a million years either side of the current
``time``. So when the features of all the times are loaded into GPlates, each one is only visible
at its own time.

Its :meth:`geometry is set<pygplates.Feature.set_geometry>` as a coverage: a multipoint, and a
scalar value at each of its points. GPlates displays the scalar values, here the convergence
velocities, in a separate layer.

.. sample-code:: pygplates_find_divergence_at_subduction_zones_and_convergence_at_ridges.py
   :fragment: create-points-feature

That is the end of the function ``calculate_converging_or_diverging_points``. It is called twice at
each time: once to find the converging points along mid-ocean ridges, and once to find the
diverging points along subduction zones.

.. sample-code:: pygplates_find_divergence_at_subduction_zones_and_convergence_at_ridges.py
   :fragment: call-function

Finally the features are written to two files. The first file has the points along mid-ocean
ridges that are converging, at all times. The second file has the points along subduction zones
that are diverging, at all times.

.. sample-code:: pygplates_find_divergence_at_subduction_zones_and_convergence_at_ridges.py
   :fragment: write-output

The two files can now be loaded into GPlates, along with the topological model they were generated
from. This shows which parts of mid-ocean ridges are unexpectedly converging, and which parts of
subduction zones are unexpectedly diverging.

See also
""""""""

- Primer: :ref:`pygplates_primer_plate_boundary_statistics`
- Reference: :meth:`pygplates.TopologicalSnapshot.calculate_plate_boundary_statistics`, :class:`pygplates.PlateBoundaryStatistic`,
  :meth:`pygplates.Feature.set_geometry`
- Sample code: :ref:`pygplates_sample_intra-plate_strain_rates_at_subduction_zones`,
  :ref:`pygplates_find_total_ridge_and_subduction_zone_lengths`, :ref:`pygplates_find_distance_and_strain_rate_along_subduction_zones`
