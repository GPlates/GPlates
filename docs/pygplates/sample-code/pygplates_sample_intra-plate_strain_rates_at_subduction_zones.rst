.. _pygplates_sample_intra-plate_strain_rates_at_subduction_zones:

Sample strain rates inside the overriding plate at subduction zones
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example calculates strain rates just inside the overriding plate along subduction zones over time by :

- sampling along plate boundaries that are subduction zones,
- determining which side the overriding plate is on,
- rotating each sample point 50 kms inside the overriding plate (along plate boundary normal direction),
- sampling the strain rate of the overriding plate at the rotated point location.

.. contents::
   :local:
   :depth: 2

Data files
""""""""""

``topologies.gpml``
    Topological plate polygon and deforming network features. Subduction zones are recognised by the
    feature type (``gpml_subduction_zone``) of the boundary section features, and each needs a
    subduction polarity of ``'Left'`` or ``'Right'`` (others are skipped). Strain rates are only
    non-zero where the overriding plate is a deforming network.

``rotations.rot``
    A rotation file. It must contain rotations for the plate IDs of the topological features.

Files of these kinds are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_sample_intra-plate_strain_rates_at_subduction_zones.py

Details
"""""""

| First create a :class:`topological model<pygplates.TopologicalModel>` from topological features and rotation files.
| The topological features can be plate polygons and/or deforming networks.
| More than one file containing topological features can be specified here, however we're only specifying one file.
| Also note that more than one rotation file (or even a single :class:`pygplates.RotationModel`) can be specified here,
  however we're only specifying a single rotation file.

.. sample-code:: pygplates_sample_intra-plate_strain_rates_at_subduction_zones.py
   :fragment: create-topological-model

.. note:: We create our :class:`pygplates.TopologicalModel` **outside** the time loop since that does not require ``time``.

| Get a snapshot of our resolved topologies.
| Here the topological features are resolved to the current ``time``
  using :func:`pygplates.TopologicalModel.topological_snapshot`.

.. sample-code:: pygplates_sample_intra-plate_strain_rates_at_subduction_zones.py
   :fragment: topological-snapshot

| Next we :meth:`calculate boundary statistics <pygplates.TopologicalSnapshot.calculate_plate_boundary_statistics>`
  along subduction zones (using the ``boundary_section_filter`` argument).
  Hence only boundaries of feature type ``pygplates.FeatureType.gpml_subduction_zone`` are looked at.
| We also use the ``return_shared_sub_segment_dict`` argument to return a dictionary that maps
  each :class:`shared sub-segment <pygplates.ResolvedTopologicalSharedSubSegment>` to a list of
  boundary sample points associated with it.
| Also the sample points are spaced 0.5 degrees apart along the subduction zones.

.. sample-code:: pygplates_sample_intra-plate_strain_rates_at_subduction_zones.py
   :fragment: plate-boundary-statistics

|  Then we iterate over the shared sub-segments. Each one has a list of its associated boundary sample points.

.. sample-code:: pygplates_sample_intra-plate_strain_rates_at_subduction_zones.py
   :fragment: iterate-shared-sub-segments

| Next we determine the subduction polarity of the current shared sub-segment (of a subducting line).
| This enables us to determine which side of the subducting line the overriding plate is on.
  If it's on the left then if you follow the order of vertices in the subducting line then it means
  the overriding plate is to the left. Similarly for the right side.

.. sample-code:: pygplates_sample_intra-plate_strain_rates_at_subduction_zones.py
   :fragment: subduction-polarity

| Then we iterate over the :class:`pygplates.PlateBoundaryStatistic`'s of the sampled points along the
  current shared sub-segment (of a subducting line).

.. sample-code:: pygplates_sample_intra-plate_strain_rates_at_subduction_zones.py
   :fragment: iterate-statistics

| For each sampled point we obtain its :class:`topology location <pygplates.TopologyPointLocation>` for the overriding plate.
  For example, if the overriding plate is on the left side then we query the resolved topology on its left side (similarly if on the right side).

.. sample-code:: pygplates_sample_intra-plate_strain_rates_at_subduction_zones.py
   :fragment: overriding-plate

| Since strain rates are non-zero only for *deforming* networks, we use a zero strain rate
  if the overriding plate is *not* deforming (ie, if it's not a resolved *network*).

::

    overriding_resolved_network = overriding_plate.located_in_resolved_network()
    if overriding_resolved_network:
        ...
    else:
        overriding_strain_rate = pygplates.StrainRate.zero

| If the overriding plate is *deforming* then we rotate the sampled point *off* the plate boundary (subduction zone)
  into the interior of the deforming network of the overriding plate by 50 kms.

| In order to rotate the sampled point along the boundary normal direction we need a rotation *pole* that is orthogonal to the boundary point and normal.

.. sample-code:: pygplates_sample_intra-plate_strain_rates_at_subduction_zones.py
   :fragment: off-boundary-rotation-pole

| Since the boundary normal points to the *left* side, if the overriding plate is on the right side then we need to reverse the rotation direction.

.. sample-code:: pygplates_sample_intra-plate_strain_rates_at_subduction_zones.py
   :fragment: reverse-rotation-pole

| Next we determine the finite rotation that will move the sampled point *off* the boundary by 50 kms.

.. sample-code:: pygplates_sample_intra-plate_strain_rates_at_subduction_zones.py
   :fragment: off-boundary-rotation

| Then we rotate the sampled boundary point *into* the overriding plate.

.. sample-code:: pygplates_sample_intra-plate_strain_rates_at_subduction_zones.py
   :fragment: off-boundary-point

| Finally we :meth:`sample the strain rate <pygplates.ResolvedTopologicalNetwork.get_point_strain_rate>`
  in the overriding deforming network at the off-boundary point.
| The off-boundary point might no longer be *inside* the overriding *deforming* network.
  This is possible if the sampled boundary point is very close to the edge of the deforming network.
  In this case we just set the strain rate to zero.

.. sample-code:: pygplates_sample_intra-plate_strain_rates_at_subduction_zones.py
   :fragment: sample-strain-rate

| Now that we've sampled all the strain rates, we create a :class:`feature <pygplates.Feature>` containing the boundary points and their strain rates.
| Note that each feature only exists at the current ``time``. This makes it easier when visualising over a range of times in GPlates.
| We also :meth:`set the geometry <pygplates.Feature.set_geometry>` as a coverage geometry (ie, a multipoint and scalar values).
  This causes the dilatation rate scalar values to show up in GPlates as a separate layer.

.. sample-code:: pygplates_sample_intra-plate_strain_rates_at_subduction_zones.py
   :fragment: create-strain-rate-feature

| Finally we write to a file that contains all points (and their dilatation strain rates) at all times along subduction zones.

.. sample-code:: pygplates_sample_intra-plate_strain_rates_at_subduction_zones.py
   :fragment: write-output

| We can now load this file into GPlates (along with the topological model used to generate it) and
  visualise the intra-plate strain rates along subduction zones.

See also
""""""""

- Primer: :ref:`pygplates_primer_plate_boundary_statistics`, :ref:`pygplates_primer_topological_network`,
  :ref:`pygplates_primer_strain_rates_in_triangulation`
- Reference: :meth:`pygplates.TopologicalSnapshot.calculate_plate_boundary_statistics`, :class:`pygplates.PlateBoundaryStatistic`,
  :class:`pygplates.TopologyPointLocation`, :meth:`pygplates.ResolvedTopologicalNetwork.get_point_strain_rate`,
  :class:`pygplates.StrainRate`
- Sample code: :ref:`pygplates_find_divergence_at_subduction_zones_and_convergence_at_ridges`,
  :ref:`pygplates_find_overriding_plate_of_closest_subducting_line`, :ref:`pygplates_reconstruct_strain_and_strain_rate`
