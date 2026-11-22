.. _pygplates_reconstruct_regular_features:

Reconstruct regular features
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example shows two scenarios that reconstruct *regular* features to a past geological time.

Regular features are all features except
`topological <http://www.gplates.org/docs/gpgim/#gpml:TopologicalFeature>`_ features,
`motion path <http://www.gplates.org/docs/gpgim/#gpml:MotionPath>`_ features and
`flowline <http://www.gplates.org/docs/gpgim/#gpml:Flowline>`_ features.

.. seealso:: :ref:`pygplates_reconstruct_motion_path_features` and :ref:`pygplates_reconstruct_flowline_features`

.. contents::
   :local:
   :depth: 2

Data files
++++++++++

Both scripts read the same files:

``rotations.rot``
    A rotation file. It must contain rotations for the plate IDs of the features in ``features.gpml``.

``features.gpml``
    Regular features, each with a reconstruction plate ID and one or more geometries. The output
    below was captured with coastlines.

Files of these kinds are in the GPlates `sample data <https://www.gplates.org/download/>`_.


.. _pygplates_export_reconstructed_features_to_a_file:

Exported reconstructed features to a file
+++++++++++++++++++++++++++++++++++++++++

This example reconstructs regular features and exports the results to a Shapefile.

Sample code
"""""""""""

.. sample-code:: pygplates_reconstruct_regular_features_export.py

Details
"""""""

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.

.. sample-code:: pygplates_reconstruct_regular_features_export.py
   :fragment: load-rotations

A :class:`reconstruct model <pygplates.ReconstructModel>` is created from the reconstructable
features and the rotation model.

.. sample-code:: pygplates_reconstruct_regular_features_export.py
   :fragment: reconstruct-model

The features will be reconstructed to their 50Ma positions.

.. sample-code:: pygplates_reconstruct_regular_features_export.py
   :fragment: reconstruction-time

:meth:`pygplates.ReconstructModel.reconstruct_snapshot` reconstructs the features to 50Ma, and
returns the result as a :class:`pygplates.ReconstructSnapshot`. A feature that does not exist at
50Ma, according to its valid time period, is not reconstructed.

:meth:`pygplates.ReconstructSnapshot.export_reconstructed_geometries` then exports the
reconstructed geometries to a file. The filename extension selects the file format, here ``.shp``
for a Shapefile.

.. sample-code:: pygplates_reconstruct_regular_features_export.py
   :fragment: reconstruct-and-export

Output
""""""

There should now be a file called ``reconstructed_50Ma.shp`` containing the feature geometries
reconstructed to their 50Ma positions.

A Shapefile holds only one type of geometry. If the reconstructed geometries are of more than one
type (points, multipoints, polylines or polygons), a directory called ``reconstructed_50Ma`` is
created instead. It contains a Shapefile for each type, such as ``reconstructed_50Ma_polyline.shp``
and ``reconstructed_50Ma_polygon.shp``.


.. _pygplates_calculate_distance_a_feature_is_reconstructed:

Calculate distance a feature is reconstructed
+++++++++++++++++++++++++++++++++++++++++++++

This example calculates the distance between the present day location of a feature geometry and
its reconstructed location. Both locations are taken at the centroid of the geometry.

Sample code
"""""""""""

.. sample-code:: pygplates_reconstruct_regular_features_distance.py

Details
"""""""

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.

.. sample-code:: pygplates_reconstruct_regular_features_distance.py
   :fragment: load-rotations

A :class:`reconstruct model <pygplates.ReconstructModel>` is created from the reconstructable
features and the rotation model.

.. sample-code:: pygplates_reconstruct_regular_features_distance.py
   :fragment: reconstruct-model

The features will be reconstructed to their 50Ma positions.

.. sample-code:: pygplates_reconstruct_regular_features_distance.py
   :fragment: reconstruction-time

:meth:`pygplates.ReconstructModel.reconstruct_snapshot` reconstructs the features to 50Ma.

:meth:`pygplates.ReconstructSnapshot.get_reconstructed_geometries` then returns the reconstructed
geometries. There is a :class:`pygplates.ReconstructedFeatureGeometry` for each geometry of each
feature that exists at 50Ma.

.. sample-code:: pygplates_reconstruct_regular_features_distance.py
   :fragment: reconstruct

A reconstructed feature geometry has both the
:meth:`present day<pygplates.ReconstructedFeatureGeometry.get_present_day_geometry>` geometry and
the :meth:`reconstructed<pygplates.ReconstructedFeatureGeometry.get_reconstructed_geometry>`
geometry. The centroid of each is found with ``get_centroid()``, which every geometry type has (a
:class:`point<pygplates.PointOnSphere>` is its own centroid).

The :meth:`distance()<pygplates.GeometryOnSphere.distance>` method of one centroid returns the
shortest distance to the other centroid, in radians. It is converted to kilometres by multiplying
it by :attr:`pygplates.Earth.mean_radius_in_kms`.

.. sample-code:: pygplates_reconstruct_regular_features_distance.py
   :fragment: distance-reconstructed

Output
""""""

For each reconstructed geometry the script prints the name and plate ID of its feature, and the
distance it was reconstructed.

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

See also
++++++++

- Reference: :class:`pygplates.ReconstructModel`, :meth:`pygplates.ReconstructModel.reconstruct_snapshot`,
  :meth:`pygplates.ReconstructSnapshot.export_reconstructed_geometries`,
  :meth:`pygplates.ReconstructSnapshot.get_reconstructed_geometries`, :class:`pygplates.ReconstructedFeatureGeometry`,
  :meth:`pygplates.GeometryOnSphere.distance`
- Sample code: :ref:`pygplates_reconstruct_motion_path_features`, :ref:`pygplates_reconstruct_flowline_features`,
  :ref:`pygplates_find_nearest_feature_to_a_point`, :ref:`pygplates_calculate_velocities_by_plate_id`
