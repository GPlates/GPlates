.. _pygplates_find_features_overlapping_a_polygon:

Find reconstructed features overlapping a polygon
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example reconstructs a collection of features, and finds those whose reconstructed geometry
overlaps a polygon.

.. contents::
   :local:
   :depth: 2

Data files
""""""""""

``rotations.rot``
    A rotation file. It must contain rotations for the plate IDs of the features in ``features.gpml``.

``features.gpml``
    Reconstructable features, each with a reconstruction plate ID and one or more geometries.

Files of these kinds are in the GPlates `sample data <https://www.gplates.org/download/>`_.

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

The features will be reconstructed to their 10Ma positions.

.. sample-code:: pygplates_find_features_overlapping_a_polygon.py
   :fragment: reconstruction-time

The test :class:`polygon<pygplates.PolygonOnSphere>` is created from a sequence of (latitude,
longitude) points. The script finds every feature that has a reconstructed geometry overlapping it.

.. sample-code:: pygplates_find_features_overlapping_a_polygon.py
   :fragment: polygon

The features are reconstructed to 10Ma by creating a :class:`pygplates.ReconstructSnapshot`. A
feature that does not exist at 10Ma, according to its valid time period, is not reconstructed.

:meth:`pygplates.ReconstructSnapshot.get_reconstructed_features` returns the
:class:`reconstructed feature geometries<pygplates.ReconstructedFeatureGeometry>` grouped by
their :class:`feature<pygplates.Feature>`. It is used, rather than
:meth:`pygplates.ReconstructSnapshot.get_reconstructed_geometries`, so that a feature is tested
as a whole and added to the results only once.

.. sample-code:: pygplates_find_features_overlapping_a_polygon.py
   :fragment: reconstruct

Each item in the ``reconstructed_features`` list is a tuple of a feature and a list of its
reconstructed geometries.

.. sample-code:: pygplates_find_features_overlapping_a_polygon.py
   :fragment: iterate-features

A feature can have more than one geometry, and so more than one *reconstructed* geometry.

.. sample-code:: pygplates_find_features_overlapping_a_polygon.py
   :fragment: iterate-geometries

The :meth:`distance()<pygplates.GeometryOnSphere.distance>` method of the polygon returns the
minimum distance between the polygon and a reconstructed geometry.

*geometry_is_solid* is set to ``True`` so that the distance is zero when the reconstructed
geometry is entirely inside the polygon. Without it the distance would be to the outline of the
polygon, which is not zero when the reconstructed geometry does not touch the outline.

*other_geometry_is_solid* is set to ``True`` for the opposite case, where the polygon is entirely
inside the reconstructed geometry (if that is a polygon too). This also counts as an overlap.

.. sample-code:: pygplates_find_features_overlapping_a_polygon.py
   :fragment: distance

A minimum distance of zero means the two overlap: the reconstructed geometry intersects the
outline of the polygon, or is inside the polygon, or (if it is a polygon) has the polygon inside it.

The feature is then added to the list of overlapping features. The ``break`` skips the rest of
its reconstructed geometries, so that the feature is not added more than once.

.. sample-code:: pygplates_find_features_overlapping_a_polygon.py
   :fragment: overlap-test

Finally, if any features overlap the polygon, they are written to a file. The file can be loaded
into `GPlates <http://www.gplates.org>`_ and reconstructed to 10Ma, to check the results.

.. sample-code:: pygplates_find_features_overlapping_a_polygon.py
   :fragment: write-output

See also
""""""""

- Reference: :class:`pygplates.ReconstructSnapshot`, :meth:`pygplates.ReconstructSnapshot.get_reconstructed_features`,
  :meth:`pygplates.GeometryOnSphere.distance`, :class:`pygplates.PolygonOnSphere`
- Sample code: :ref:`pygplates_find_nearest_feature_to_a_point`, :ref:`pygplates_reconstruct_regular_features`
