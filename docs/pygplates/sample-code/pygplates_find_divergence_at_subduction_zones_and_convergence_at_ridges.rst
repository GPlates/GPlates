.. _pygplates_find_divergence_at_subduction_zones_and_convergence_at_ridges:

Find divergence at subduction zones and convergence at ridges
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example finds points on plate boundaries:

- where there's *convergence* along boundary sections labelled as *mid-ocean ridges*, and
- where there's *divergence* along boundary sections labelled as *subduction zones*

...over a series of geological times.

This is useful to locate regions along *mid-ocean ridges* that are **not** diverging as expected.
Or regions along *subduction zones* that are **not** converging as expected.

.. contents::
   :local:
   :depth: 2

Data files
""""""""""

``topologies.gpml``
    Topological plate polygon and/or deforming network features. Mid-ocean ridges and subduction
    zones are recognised by the feature type (``gpml_mid_ocean_ridge`` or ``gpml_subduction_zone``)
    of the boundary section features.

``rotations.rot``
    A rotation file. It must contain rotations for the plate IDs of the topological features.

Files of these kinds are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_find_divergence_at_subduction_zones_and_convergence_at_ridges.py

Details
"""""""

| First create a :class:`topological model<pygplates.TopologicalModel>` from topological features and rotation files.
| The topological features can be plate polygons and/or deforming networks.
| More than one file containing topological features can be specified here, however we're only specifying one file.
| Also note that more than one rotation file (or even a single :class:`pygplates.RotationModel`) can be specified here,
  however we're only specifying a single rotation file.

.. sample-code:: pygplates_find_divergence_at_subduction_zones_and_convergence_at_ridges.py
   :fragment: create-topological-model

.. note:: We create our :class:`pygplates.TopologicalModel` **outside** the time loop since that does not require ``time``.

| Get a snapshot of our resolved topologies.
| Here the topological features are resolved to the current ``time``
  using :func:`pygplates.TopologicalModel.topological_snapshot`.

.. sample-code:: pygplates_find_divergence_at_subduction_zones_and_convergence_at_ridges.py
   :fragment: topological-snapshot

| Next we define a function to calculate plate boundary statistics.
| We use a function so that we don't have to write very similar code twice.
  Once for diverging regions and again for converging regions.
| The function accepts an argument that determines the type of boundary to look at
  (for example, mid-ocean ridges or subduction zones).
  The second function argument is a divergence/convergence velocity threshold.
  And a third function argument is whether to detect converging points (or diverging points).

.. sample-code:: pygplates_find_divergence_at_subduction_zones_and_convergence_at_ridges.py
   :fragment: define-function

| Within this function we first :meth:`calculate boundary statistics <pygplates.TopologicalSnapshot.calculate_plate_boundary_statistics>`
  along the requested boundary type. Only boundaries of the requested :class:`feature type <pygplates.FeatureType>` are looked at.
| Also the sample points are spaced 1 degree apart along the plate boundaries.
  And velocities are calculated in cms/yr.

.. sample-code:: pygplates_find_divergence_at_subduction_zones_and_convergence_at_ridges.py
   :fragment: plate-boundary-statistics

| Next we iterate over the :class:`pygplates.PlateBoundaryStatistic`'s of the sampled points along the plate boundaries to
  collect their point locations and associated convergence velocities.

.. sample-code:: pygplates_find_divergence_at_subduction_zones_and_convergence_at_ridges.py
   :fragment: iterate-statistics

| At some sample locations along the plate boundaries there might not be a plate (or network) on one side of the boundary.
  This can happen when the topological model does not have global coverage or if there are inadvertent cracks/gaps between plates.
  In this case the convergence velocity will be ``float('nan')``, and we ignore these sample locations.

.. sample-code:: pygplates_find_divergence_at_subduction_zones_and_convergence_at_ridges.py
   :fragment: skip-nan

| Next we detect if the convergence velocity is above the threshold (if we're looking for converging locations) or
  below minus the threshold, that is, diverging faster than the threshold (if we're looking for diverging locations).
  If found then these points and associated convergence velocities are added to the output.
| Note that ``stat.convergence_velocity_signed_magnitude`` is a *signed* magnitude, and so it's positive if the plates
  are converging and negative if they’re diverging.

.. sample-code:: pygplates_find_divergence_at_subduction_zones_and_convergence_at_ridges.py
   :fragment: test-threshold

| Then we create a :class:`feature <pygplates.Feature>` containing the output points and their convergence velocities.
| Note that each feature only exists at the current ``time``. This makes it easier when visualising over a range of times in GPlates.
| We also :meth:`set the geometry <pygplates.Feature.set_geometry>` as a coverage geometry (ie, a multipoint and scalar values).
  This causes the convergence velocity scalar values to show up in GPlates as a separate layer.

.. sample-code:: pygplates_find_divergence_at_subduction_zones_and_convergence_at_ridges.py
   :fragment: create-points-feature

| So that ends the definition of our function called ``calculate_converging_or_diverging_points``.
| Next we call that function twice.
  Once to find converging points along mid-ocean ridges.
  And again to find diverging points along subduction zones.

.. sample-code:: pygplates_find_divergence_at_subduction_zones_and_convergence_at_ridges.py
   :fragment: call-function

| Finally we write the output points to two separate files.
  The first file contains all points at all times along mid-ocean ridges that are converging.
  And the second file contains all points at all times along subduction zones that are diverging.

.. sample-code:: pygplates_find_divergence_at_subduction_zones_and_convergence_at_ridges.py
   :fragment: write-output

| We can now load these two files into GPlates (along with the topological model used to generate them) and
  see what parts of mid-ocean ridges are unexpectedly converging and what parts of subduction zones are
  unexpectedly diverging.

See also
""""""""

- Primer: :ref:`pygplates_primer_plate_boundary_statistics`
- Reference: :meth:`pygplates.TopologicalSnapshot.calculate_plate_boundary_statistics`, :class:`pygplates.PlateBoundaryStatistic`,
  :meth:`pygplates.Feature.set_geometry`
- Sample code: :ref:`pygplates_sample_intra-plate_strain_rates_at_subduction_zones`,
  :ref:`pygplates_find_total_ridge_and_subduction_zone_lengths`
