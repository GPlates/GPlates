.. _pygplates_find_nearest_feature_to_a_point:

Find nearest reconstructed feature to a point
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example reconstructs a collection of features, and finds the reconstructed feature that is
nearest to a point.

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

.. sample-code:: pygplates_find_nearest_feature_to_a_point.py

Details
"""""""

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.

.. sample-code:: pygplates_find_nearest_feature_to_a_point.py
   :fragment: load-rotations

The reconstructable features are loaded into a :class:`pygplates.FeatureCollection`.

.. sample-code:: pygplates_find_nearest_feature_to_a_point.py
   :fragment: load-features

The features will be reconstructed to their 10Ma positions.

.. sample-code:: pygplates_find_nearest_feature_to_a_point.py
   :fragment: reconstruction-time

The test :class:`point<pygplates.PointOnSphere>` is at zero latitude and zero longitude. The
script finds the reconstructed feature that is closest to it.

.. sample-code:: pygplates_find_nearest_feature_to_a_point.py
   :fragment: point

``polygons_are_solid`` says how the distance to a polygon is measured. With ``False``, as here, a
polygon is only its outline: the distance to it is the distance to its outline, even where the
point is inside the polygon. With ``True`` a polygon is filled, so a point inside it is at a
distance of zero from it. The distance to any other type of geometry is the same either way.

.. sample-code:: pygplates_find_nearest_feature_to_a_point.py
   :fragment: polygons-are-solid

The features are reconstructed to 10Ma by creating a :class:`pygplates.ReconstructSnapshot`. A
feature that does not exist at 10Ma, according to its valid time period, is not reconstructed.

:meth:`pygplates.ReconstructSnapshot.get_reconstructed_features` returns the
:class:`reconstructed feature geometries<pygplates.ReconstructedFeatureGeometry>` grouped by
their :class:`feature<pygplates.Feature>`. This example does not strictly need the grouping:
:meth:`pygplates.ReconstructSnapshot.get_reconstructed_geometries` would also do.

.. sample-code:: pygplates_find_nearest_feature_to_a_point.py
   :fragment: reconstruct

``min_distance_to_all_features`` is the distance from the point to the nearest geometry found so
far. It starts as ``None``, since nothing has been found yet.

It is also passed to the :meth:`distance()<pygplates.GeometryOnSphere.distance>` method below as
the distance threshold, where ``None`` means there is no threshold.

.. sample-code:: pygplates_find_nearest_feature_to_a_point.py
   :fragment: initial-minimum-distance

Each item in the ``reconstructed_features`` list is a tuple of a feature and a list of its
reconstructed geometries.

.. sample-code:: pygplates_find_nearest_feature_to_a_point.py
   :fragment: iterate-features

A feature can have more than one geometry, and so more than one *reconstructed* geometry.

.. sample-code:: pygplates_find_nearest_feature_to_a_point.py
   :fragment: iterate-geometries

The :meth:`distance()<pygplates.GeometryOnSphere.distance>` method of the point returns the
minimum distance, in radians, from the point to a reconstructed geometry. ``polygons_are_solid``
is passed as its *other_geometry_is_solid* argument.

``min_distance_to_all_features`` is the distance threshold. With a threshold, the method returns
``None`` for a geometry that is further away than the threshold. Only a geometry that is nearer
than the nearest one found so far is of interest.

.. sample-code:: pygplates_find_nearest_feature_to_a_point.py
   :fragment: distance

If ``None`` was returned, the current geometry is further away than
``min_distance_to_all_features``, and is ignored.

Otherwise the current geometry is the nearest one found so far. Its feature becomes the nearest
feature, and its distance becomes the new minimum distance.

.. sample-code:: pygplates_find_nearest_feature_to_a_point.py
   :fragment: nearest-so-far

Once all the features have been tested, the script prints the nearest feature and its distance
from the point, converted to kilometres. Nothing is printed if no feature was reconstructed.

.. sample-code:: pygplates_find_nearest_feature_to_a_point.py
   :fragment: print-nearest

Advanced
""""""""

To also find the nearest position on the nearest feature, set *return_closest_positions* to
``True`` when calculating the distance to a geometry:
::

    nearest_point_on_nearest_feature = None
    
    ...
    
    min_distance_to_feature_and_closest_positions = point.distance(
            feature_reconstructed_geometry.get_reconstructed_geometry(),
            min_distance_to_all_features,
            other_geometry_is_solid=polygons_are_solid,
            return_closest_positions=True)

When *return_closest_positions* is ``True``, the :meth:`distance()<pygplates.GeometryOnSphere.distance>`
method returns a 3-tuple: the distance, the closest point on the geometry it is called on, and the
closest point on the other geometry. It still returns ``None`` if the distance is greater than
the threshold. The closest point on a polygon is always on its outline, even when the polygon is
filled (``polygons_are_solid = True``) and the point is inside it, at a distance of zero.

Otherwise the tuple can be unpacked into its three elements.

::

    if min_distance_to_feature_and_closest_positions is not None:
        min_distance_to_all_features, _, nearest_point_on_nearest_feature = min_distance_to_feature_and_closest_positions
        nearest_feature = feature

Here ``_`` is just a placeholder for the second element of the tuple, which is ignored. It is the
closest point on the test point, which is the test point itself. Only the closest point on the
geometry of the feature is of interest.

See also
""""""""

- Reference: :class:`pygplates.ReconstructSnapshot`, :meth:`pygplates.ReconstructSnapshot.get_reconstructed_features`,
  :meth:`pygplates.GeometryOnSphere.distance`, :class:`pygplates.PointOnSphere`
- Sample code: :ref:`pygplates_find_features_overlapping_a_polygon`,
  :ref:`pygplates_find_overriding_plate_of_closest_subducting_line`
