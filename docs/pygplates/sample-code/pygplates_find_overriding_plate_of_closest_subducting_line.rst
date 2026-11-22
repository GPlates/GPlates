.. _pygplates_find_overriding_plate_of_closest_subducting_line:

Find overriding plate of closest subducting line
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example finds the overriding plate of the subducting line that is nearest to each of a set
of features, over a series of geological times. At each time it:

- reconstructs the regular features,
- resolves the topological plate boundaries,
- finds the subducting line (in the topological plate boundaries) that is nearest to each regular feature, and
- determines the overriding plate of that subducting line.

.. contents::
   :local:
   :depth: 2

Data files
""""""""""

``rotations.rot``
    A rotation file. It must contain rotations for the plate IDs of the features in ``features.gpml``
    and ``topologies.gpml``.

``features.gpml``
    Regular (non-topological) features, each with a reconstruction plate ID and one or more geometries.
    The output below was captured with spreading ridges.

``topologies.gpml``
    Topological plate polygon and/or deforming network features, and the boundary section features
    that they reference. Subduction zones are recognised by the feature type
    (:attr:`pygplates.FeatureType.gpml_subduction_zone`) of the boundary section features, and each
    needs a subduction polarity, since that is what determines its overriding plate.

Files of these kinds are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_find_overriding_plate_of_closest_subducting_line.py

Details
"""""""

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.

.. sample-code:: pygplates_find_overriding_plate_of_closest_subducting_line.py
   :fragment: load-rotations

A :class:`reconstruct model<pygplates.ReconstructModel>` is created from the regular
(non-topological) features and the rotation model. These are the features whose nearest subducting
lines are to be found.

.. sample-code:: pygplates_find_overriding_plate_of_closest_subducting_line.py
   :fragment: reconstruct-model

A :class:`topological model<pygplates.TopologicalModel>` is created from the topological features
and the rotation model.

.. sample-code:: pygplates_find_overriding_plate_of_closest_subducting_line.py
   :fragment: topological-model

:meth:`pygplates.ReconstructModel.reconstruct_snapshot` reconstructs the regular features to the
current ``time``, and returns the result as a :class:`pygplates.ReconstructSnapshot`.

:meth:`pygplates.ReconstructSnapshot.get_reconstructed_features` then returns the
:class:`reconstructed feature geometries<pygplates.ReconstructedFeatureGeometry>` grouped by their
:class:`feature<pygplates.Feature>`.

.. sample-code:: pygplates_find_overriding_plate_of_closest_subducting_line.py
   :fragment: reconstruct-snapshot

Each item in the ``reconstructed_features`` list is a tuple of a feature and a list of its
reconstructed geometries. A feature can have more than one geometry, and so more than one
*reconstructed* geometry.

::

    for feature, feature_reconstructed_geometries in reconstructed_features:
        ...
        for feature_reconstructed_geometry in feature_reconstructed_geometries:

:meth:`pygplates.TopologicalModel.topological_snapshot` resolves the topological features at the
current ``time``, and returns the result as a :class:`pygplates.TopologicalSnapshot`.

:meth:`pygplates.TopologicalSnapshot.get_resolved_topological_sections` then returns the boundary
sections of the resolved topologies in the snapshot. By default these are the sections on the
boundaries of both the resolved plate polygons (:class:`pygplates.ResolvedTopologicalBoundary`) and
the resolved deforming networks (:class:`pygplates.ResolvedTopologicalNetwork`).

.. sample-code:: pygplates_find_overriding_plate_of_closest_subducting_line.py
   :fragment: topological-snapshot

These :class:`boundary sections<pygplates.ResolvedTopologicalSection>` are used because each of
their sub-segments has a list of the topologies that share it. That list is searched to find the
overriding plate of a subducting line.

A boundary section whose feature is not a subduction zone is skipped, since only subducting lines
are of interest.

Not all of the geometry of a boundary section feature is on the boundary of a topology. For
example, the ends of a section that extend past its neighbouring sections are clipped off.
:meth:`pygplates.ResolvedTopologicalSection.get_shared_sub_segments` returns the parts that are on
a boundary.

::

    for shared_boundary_section in shared_boundary_sections:
        if shared_boundary_section.get_feature().get_feature_type() != pygplates.FeatureType.gpml_subduction_zone:
            continue
        for shared_sub_segment in shared_boundary_section.get_shared_sub_segments():
            ...

For each regular feature, the script finds the minimum distance from the feature to all the
subducting lines. To begin with there is no minimum distance, and no nearest subducting line
(shared sub-segment).

.. sample-code:: pygplates_find_overriding_plate_of_closest_subducting_line.py
   :fragment: initial-minimum-distance

The :meth:`distance()<pygplates.GeometryOnSphere.distance>` method of the reconstructed geometry
returns the minimum distance from the reconstructed regular feature to a subducting line.

``min_distance_to_all_subducting_lines`` is the distance threshold. With a threshold, the method
returns ``None`` for a subducting line that is further away than the threshold. Only a subducting
line that is nearer than the nearest one found so far is of interest.

.. sample-code:: pygplates_find_overriding_plate_of_closest_subducting_line.py
   :fragment: distance

If ``None`` was returned, the current subducting line is further away than
``min_distance_to_all_subducting_lines``, and is ignored.

Otherwise the current subducting line is the nearest one found so far. It becomes the nearest
subducting line (shared sub-segment), and its distance becomes the new minimum distance.

.. sample-code:: pygplates_find_overriding_plate_of_closest_subducting_line.py
   :fragment: nearest-so-far

Once the nearest subducting line is found,
:meth:`pygplates.ResolvedTopologicalSharedSubSegment.get_overriding_plate` returns its overriding
plate.

The method uses the subduction polarity of the subducting line to decide whether the overriding
plate is on its left or its right side. It then searches the resolved topologies that share the
subducting line, to find the single plate (or deforming network) on that side.

It returns ``None`` if the subducting line has no subduction polarity of ``'Left'`` or
``'Right'``, or if it cannot find a single plate (or network) on the overriding side.

.. sample-code:: pygplates_find_overriding_plate_of_closest_subducting_line.py
   :fragment: overriding-plate

Finally the script prints the plate ID of the overriding plate, and the distance from the current
feature to its nearest subducting line.

.. sample-code:: pygplates_find_overriding_plate_of_closest_subducting_line.py
   :fragment: print-result

Output
""""""

With spreading ridges as the regular features, the output is like the following:

::

    Time 0.000000
      Feature: IS  GRN_EUR, RI Fram Strait
        overriding plate ID: 701
        distance to subducting line: 3025.617930Kms
      Feature: IS  GRN_EUR, RI GRN Sea
        overriding plate ID: 701
        distance to subducting line: 2909.012775Kms
      Feature: ISO CANADA BAS XR
        overriding plate ID: 101
        distance to subducting line: 1158.983648Kms
      Feature: IS  NAM_EUR, Arctic
        overriding plate ID: 701
        distance to subducting line: 3316.334722Kms
      Feature: Ridge axis (reykanesh?)
        overriding plate ID: 301
        distance to subducting line: 2543.799959Kms
      Feature: Ridge axis-Aegir
        overriding plate ID: 301
        distance to subducting line: 2121.303051Kms
      Feature: Reykjanes/NATL RIDGE AXIS
        overriding plate ID: 301
        distance to subducting line: 2892.821343Kms
      Feature: Reykjanes/NATL RIDGE AXIS
        overriding plate ID: 301
        distance to subducting line: 2576.504659Kms
      Feature: Reykjanes/NATL RIDGE AXIS
        overriding plate ID: 301
        distance to subducting line: 2740.868166Kms
      Feature: Mid-Atlantic Ridge, Klitgord and Schouten 86
        overriding plate ID: 301
        distance to subducting line: 3083.752943Kms
      Feature: Mid-Atlantic Ridge, RDM 6/93 from sat gravity and epicenters
        overriding plate ID: 201
        distance to subducting line: 2705.900894Kms
      Feature: Mid-Atlantic Ridge, Klitgord and Schouten 86
        overriding plate ID: 201
        distance to subducting line: 2383.736448Kms
      Feature: Mid-Atlantic Ridge, Purdy (1990)
        overriding plate ID: 201
        distance to subducting line: 1830.700938Kms
    
    ...

See also
""""""""

- Primer: :ref:`pygplates_primer_topological_snapshot`
- Reference: :class:`pygplates.ReconstructModel`, :class:`pygplates.TopologicalModel`,
  :meth:`pygplates.ResolvedTopologicalSharedSubSegment.get_overriding_plate`,
  :meth:`pygplates.GeometryOnSphere.distance`
- Sample code: :ref:`pygplates_find_nearest_feature_to_a_point`,
  :ref:`pygplates_sample_intra-plate_strain_rates_at_subduction_zones`
