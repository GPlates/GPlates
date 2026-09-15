.. _pygplates_reconstruct_regular_features:

Reconstruct regular features
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

| This example shows a couple of different scenarios involving the reconstruction of *regular* features to geological times.
| Regular features exclude `topological <http://www.gplates.org/docs/gpgim/#gpml:TopologicalFeature>`_ features,
  `motion path <http://www.gplates.org/docs/gpgim/#gpml:MotionPath>`_ features and
  `flowline <http://www.gplates.org/docs/gpgim/#gpml:Flowline>`_ features.

.. seealso:: :ref:`pygplates_reconstruct_motion_path_features`

.. seealso:: :ref:`pygplates_reconstruct_flowline_features`

.. contents::
   :local:
   :depth: 2


.. _pygplates_export_reconstructed_features_to_a_file:

Exported reconstructed features to a file
+++++++++++++++++++++++++++++++++++++++++

In this example we reconstruct regular features and export the results to a Shapefile.

Sample code
"""""""""""

.. sample-code:: pygplates_reconstruct_regular_features_export.py

Details
"""""""

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.

.. sample-code:: pygplates_reconstruct_regular_features_export.py
   :fragment: load-rotations

Create a :class:`reconstruct model <pygplates.ReconstructModel>` from the reconstructable features and the rotation model.

.. sample-code:: pygplates_reconstruct_regular_features_export.py
   :fragment: reconstruct-model

The features will be reconstructed to their 50Ma positions.

.. sample-code:: pygplates_reconstruct_regular_features_export.py
   :fragment: reconstruction-time

| All features are :meth:`reconstructed <pygplates.ReconstructModel.reconstruct_snapshot>` to 50Ma.
| We then :meth:`export the reconstructed geometries <pygplates.ReconstructSnapshot.export_reconstructed_geometries>` to a file.

.. sample-code:: pygplates_reconstruct_regular_features_export.py
   :fragment: reconstruct-and-export

Output
""""""

We should now have a file called ``reconstructed_50Ma.shp`` containing feature geometries reconstructed
to their 50Ma positions.


.. _pygplates_calculate_distance_a_feature_is_reconstructed:

Calculate distance a feature is reconstructed
+++++++++++++++++++++++++++++++++++++++++++++

In this example we calculate the distance between a feature geometry's present day (centroid) location
and its reconstructed (centroid) location.

Sample code
"""""""""""

.. sample-code:: pygplates_reconstruct_regular_features_distance.py

Details
"""""""

| We define a function to return the centroid of a geometry.
| If the geometry is a :class:`pygplates.MultiPointOnSphere`, :class:`pygplates.PolylineOnSphere` or :class:`pygplates.PolygonOnSphere`
  then we can call ``get_centroid()`` on it (since those geometry types all have that method).
  However, if it's a :class:`pygplates.PointOnSphere` then it does not have that method, in which case we just return
  the point since it's already its own centroid.

.. sample-code:: pygplates_reconstruct_regular_features_distance.py
   :fragment: centroid-function

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.

.. sample-code:: pygplates_reconstruct_regular_features_distance.py
   :fragment: load-rotations

Create a :class:`reconstruct model <pygplates.ReconstructModel>` from the reconstructable features and the rotation model.

.. sample-code:: pygplates_reconstruct_regular_features_distance.py
   :fragment: reconstruct-model

The features will be reconstructed to their 50Ma positions.

.. sample-code:: pygplates_reconstruct_regular_features_distance.py
   :fragment: reconstruction-time

| All features are :meth:`reconstructed <pygplates.ReconstructModel.reconstruct_snapshot>` to 50Ma.
| We then :meth:`query the reconstructed geometries <pygplates.ReconstructSnapshot.get_reconstructed_geometries>`.

.. sample-code:: pygplates_reconstruct_regular_features_distance.py
   :fragment: reconstruct

| We use our ``get_geometry_centroid()`` function to find the centroid of the
  :meth:`present day<pygplates.ReconstructedFeatureGeometry.get_present_day_geometry>` and
  :meth:`reconstructed<pygplates.ReconstructedFeatureGeometry.get_reconstructed_geometry>` geometries.
| We use the :meth:`pygplates.GeometryOnSphere.distance` function to calculate the shortest
  distance between the two centroids and convert it to kilometres using :class:`pygplates.Earth`.

.. sample-code:: pygplates_reconstruct_regular_features_distance.py
   :fragment: distance-reconstructed

Output
""""""

::

    Feature: Pacific
      plate ID: 982
      distance reconstructed: 3815.013838 kms
    Feature: Marie Byrd Land
      plate ID: 804
      distance reconstructed: 514.440695 kms
    Feature: Pacific
      plate ID: 901
      distance reconstructed: 3795.781009 kms
    Feature: Pacific
      plate ID: 901
      distance reconstructed: 3786.206123 kms
    Feature: Pacific
      plate ID: 901
      distance reconstructed: 3786.068477 kms
    Feature: Pacific
      plate ID: 901
      distance reconstructed: 3785.868706 kms
    Feature: Pacific
      plate ID: 901
      distance reconstructed: 3785.465344 kms
    Feature: Pacific
      plate ID: 901
      distance reconstructed: 3788.422368 kms
    Feature: Pacific
      plate ID: 901
      distance reconstructed: 3790.540180 kms
    Feature: Pacific
      plate ID: 901
      distance reconstructed: 3554.951168 kms
    Feature: Pacific
      plate ID: 901
      distance reconstructed: 3553.133934 kms
    Feature: Northwest Africa
      plate ID: 714
      distance reconstructed: 643.521413 kms
    
    ...
