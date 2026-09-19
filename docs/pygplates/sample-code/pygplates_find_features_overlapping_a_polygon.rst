.. _pygplates_find_features_overlapping_a_polygon:

Find reconstructed features overlapping a polygon
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example iterates over a collection of reconstructed features and finds those whose geometry overlaps a polygon.

.. contents::
   :local:
   :depth: 2

Sample code
"""""""""""

.. sample-code:: pygplates_find_features_overlapping_a_polygon.py

Details
"""""""

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.

.. sample-code:: pygplates_find_features_overlapping_a_polygon.py
   :fragment: load-rotations

The reconstructable features are loaded into a :class:`pygplates.FeatureCollection`.

.. sample-code:: pygplates_find_features_overlapping_a_polygon.py
   :fragment: load-features

The features are reconstructed to their 10Ma positions.

.. sample-code:: pygplates_find_features_overlapping_a_polygon.py
   :fragment: reconstruction-time

The test polygon will capture all features whose reconstructed geometry(s) overlap it.

.. sample-code:: pygplates_find_features_overlapping_a_polygon.py
   :fragment: polygon

| All features are reconstructed to 10Ma in a :class:`pygplates.ReconstructSnapshot`.
| We ask the snapshot for its :meth:`reconstructed features<pygplates.ReconstructSnapshot.get_reconstructed_features>`
  (rather than its reconstructed geometries) so that our
  :class:`reconstructed feature geometries<pygplates.ReconstructedFeatureGeometry>`
  are grouped with their :class:`feature<pygplates.Feature>`.

.. sample-code:: pygplates_find_features_overlapping_a_polygon.py
   :fragment: reconstruct

Each item in the *reconstructed_features* list is a tuple containing a feature and its associated
reconstructed geometries.

.. sample-code:: pygplates_find_features_overlapping_a_polygon.py
   :fragment: iterate-features

A feature can have more than one geometry and hence will have more than one *reconstructed* geometry.

.. sample-code:: pygplates_find_features_overlapping_a_polygon.py
   :fragment: iterate-geometries

| Calculate the minimum distance between the polygon and a reconstructed feature geometry using :meth:`pygplates.GeometryOnSphere.distance`.
| *geometry1_is_solid* is set to True in case the reconstructed geometry lies entirely inside
  the polygon in which case it will return a distance of zero.
| If we did not specify this it would have returned the distance to the polygon's boundary outline
  which could be non-zero if the reconstructed geometry did not intersect the outline.
| And *geometry2_is_solid* is set to True in case the polygon lies entirely inside the reconstructed
  geometry (if it's a polygon also). This also constitutes an overlap.

.. sample-code:: pygplates_find_features_overlapping_a_polygon.py
   :fragment: distance

| A minimum distance of zero means the current reconstructed geometry either intersects the polygon's
  boundary or is inside it.
| Or, conversely, the polygon could be inside the reconstructed geometry (if it's a polygon) which also constitutes an overlap.

.. sample-code:: pygplates_find_features_overlapping_a_polygon.py
   :fragment: overlap-test

| Finally we write the overlapping features to a file.
| We could then load them into `GPlates <http://www.gplates.org>`_, reconstruct to 10Ma and check the results.

.. sample-code:: pygplates_find_features_overlapping_a_polygon.py
   :fragment: write-output
