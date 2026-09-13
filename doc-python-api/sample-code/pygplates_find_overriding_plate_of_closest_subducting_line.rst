.. _pygplates_find_overriding_plate_of_closest_subducting_line:

Find overriding plate of closest subducting line
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example finds the overriding plate of the nearest subducting line over time by:

- reconstructing regular features,
- resolving topological plate boundaries,
- finding the nearest subducting line (in the topological plate boundaries) to each regular feature,
- determining the overriding plate of that subducting line.

.. contents::
   :local:
   :depth: 2

Sample code
"""""""""""

.. sample-code:: pygplates_find_overriding_plate_of_closest_subducting_line.py

Details
"""""""

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.

.. sample-code:: pygplates_find_overriding_plate_of_closest_subducting_line.py
   :fragment: load-rotations

Create a :class:`reconstruct model <pygplates.ReconstructModel>` from the regular (non-topological) features and the rotation model.
These are the regular features that we want to see which subducting lines (in the topologies) are closest to.

.. sample-code:: pygplates_find_overriding_plate_of_closest_subducting_line.py
   :fragment: reconstruct-model

Create a :class:`topological model<pygplates.TopologicalModel>` from topological features and the rotation model.

.. sample-code:: pygplates_find_overriding_plate_of_closest_subducting_line.py
   :fragment: topological-model

| All regular features are reconstructed to the current ``time`` using :meth:`pygplates.ReconstructModel.reconstruct_snapshot`
  that returns a :class:`pygplates.ReconstructSnapshot`.
| We then call :meth:`pygplates.ReconstructSnapshot.get_reconstructed_features` so that our
  :class:`reconstructed feature geometries<pygplates.ReconstructedFeatureGeometry>` are grouped with their :class:`feature<pygplates.Feature>`.

.. sample-code:: pygplates_find_overriding_plate_of_closest_subducting_line.py
   :fragment: reconstruct-snapshot

| Each item in the *reconstructed_features* list is a tuple containing a feature and its associated
  reconstructed geometries.
| A feature can have more than one geometry and hence will have more than one *reconstructed* geometry.

::

    for feature, feature_reconstructed_geometries in reconstructed_features:
        ...
        for feature_reconstructed_geometry in feature_reconstructed_geometries:

| Get a snapshot of our resolved topologies at the current ``time`` using :func:`pygplates.TopologicalModel.topological_snapshot`.
| And from the snapshot extract the boundary sections between our resolved topological plate polygons (and deforming networks).
  By default both :class:`pygplates.ResolvedTopologicalBoundary` (used for dynamic plate polygons) and
  :class:`pygplates.ResolvedTopologicalNetwork` (used for deforming regions) are listed in the boundary sections.

.. sample-code:: pygplates_find_overriding_plate_of_closest_subducting_line.py
   :fragment: topological-snapshot

| These :class:`boundary sections<pygplates.ResolvedTopologicalSection>` are actually what
  we're interested in because their sub-segments have a list of topologies on them.
| And it's that list of topologies that we'll be searching to find the overriding plate of a subducting line.

We ignore features that are not subduction zones because we're only interested in finding the
nearest subducting lines.

| Not all parts of a topological section feature's geometry contribute to the boundaries of topologies.
| Little bits at the ends get clipped off.
| The parts that do contribute can be found using :meth:`pygplates.ResolvedTopologicalSection.get_shared_sub_segments`.

::

    for shared_boundary_section in shared_boundary_sections:
        if shared_boundary_section.get_feature().get_feature_type() != pygplates.FeatureType.gpml_subduction_zone:
            continue
        for shared_sub_segment in shared_boundary_section.get_shared_sub_segments():
            ...

| For each regular feature we want to find the minimum distance to all subducting lines.
| Initially we don't have a minimum distance or the nearest subducting line (shared sub-segment).

.. sample-code:: pygplates_find_overriding_plate_of_closest_subducting_line.py
   :fragment: initial-minimum-distance

| Calculate the minimum distance from the reconstructed regular feature to the subducting line using
  :meth:`pygplates.GeometryOnSphere.distance`.
| *min_distance_to_subducting_line* is specified as the distance threshold since we're only interested
  in subducting lines that are nearer than the closest one encountered so far.

.. sample-code:: pygplates_find_overriding_plate_of_closest_subducting_line.py
   :fragment: distance

| If ``None`` was returned then the distance was greater than *min_distance_to_subducting_line*.
| So a valid returned value means the current subducting line is the nearest one encountered so far.
| In this case we record the nearest subducting line (shared sub-segment) and the new minimum distance.

.. sample-code:: pygplates_find_overriding_plate_of_closest_subducting_line.py
   :fragment: nearest-so-far

| Now that we have found the nearest subducting line we can find its overriding plate using
  :meth:`pygplates.ResolvedTopologicalSharedSubSegment.get_overriding_plate`.
| This uses the subduction polarity of the subducting line to determine whether the overriding
  plate is on its left or right side, and then it searches the resolved topologies attached to
  the subducting line to find the single plate (or deforming network) on the overriding side.

.. sample-code:: pygplates_find_overriding_plate_of_closest_subducting_line.py
   :fragment: overriding-plate

When we've found the overriding plate of the nearest subduction zone to the current feature we print out
the overriding plate ID and the distance to nearest subducting line.

.. sample-code:: pygplates_find_overriding_plate_of_closest_subducting_line.py
   :fragment: print-result

Output
""""""

When spreading ridges are used as the regular input features then we get output like the following:

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
