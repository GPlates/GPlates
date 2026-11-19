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

::

    import pygplates


    # Load one or more rotation files into a rotation model.
    rotation_model = pygplates.RotationModel('rotations.rot')

    # Create a reconstruct model from some reconstructable features and the rotation model.
    reconstruct_model = pygplates.ReconstructModel('features.gpml', rotation_model)

    # Reconstruct features to this geological time.
    reconstruction_time = 50
    
    # The filename of the exported reconstructed geometries.
    # It's a shapefile called 'reconstructed_50Ma.shp'.
    export_filename = 'reconstructed_{0}Ma.shp'.format(reconstruction_time)

    # Reconstruct the features to the reconstruction time and export them to a shapefile.
    reconstruct_snapshot = reconstruct_model.reconstruct_snapshot(reconstruction_time)
    reconstruct_snapshot.export_reconstructed_geometries(export_filename)

Details
"""""""

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.
::

    rotation_model = pygplates.RotationModel('rotations.rot')

Create a :class:`reconstruct model <pygplates.ReconstructModel>` from the reconstructable features and the rotation model.
::

    reconstruct_model = pygplates.ReconstructModel('features.gpml', rotation_model)

The features will be reconstructed to their 50Ma positions.
::

    reconstruction_time = 50

| All features are :meth:`reconstructed <pygplates.ReconstructModel.reconstruct_snapshot>` to 50Ma.
| We then :meth:`export the reconstructed geometries <pygplates.ReconstructSnapshot.export_reconstructed_geometries>` to a file.

::

    reconstruct_snapshot = reconstruct_model.reconstruct_snapshot(reconstruction_time)
    reconstruct_snapshot.export_reconstructed_geometries(export_filename)

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

::

    import pygplates


    # A function to return the centroid of the geometry (point/multipoint/polyline/polygon).
    def get_geometry_centroid(geometry):
        
        try:
            # See if geometry is a polygon, polyline or multipoint.
            return geometry.get_centroid()
        except AttributeError:
            # Geometry must be a point - it is already its own centroid.
            return geometry


    # Load one or more rotation files into a rotation model.
    rotation_model = pygplates.RotationModel('rotations.rot')

    # Create a reconstruct model from some reconstructable features and the rotation model.
    reconstruct_model = pygplates.ReconstructModel('features.gpml', rotation_model)

    # Reconstruct features to this geological time.
    reconstruction_time = 50

    # Reconstruct the features to the reconstruction time.
    reconstruct_snapshot = reconstruct_model.reconstruct_snapshot(reconstruction_time)
    reconstructed_feature_geometries = reconstruct_snapshot.get_reconstructed_geometries()

    # Iterate over all reconstructed feature geometries.
    for reconstructed_feature_geometry in reconstructed_feature_geometries:
        
        # Calculate distance between:
        #  - the centroid of the present-day geometry, and
        #  - the centroid of the reconstructed geometry.
        distance_reconstructed = pygplates.GeometryOnSphere.distance(
            get_geometry_centroid(reconstructed_feature_geometry.get_present_day_geometry()),
            get_geometry_centroid(reconstructed_feature_geometry.get_reconstructed_geometry()))
        
        # Convert distance from radians to Kms.
        distance_reconstructed_in_kms = distance_reconstructed * pygplates.Earth.mean_radius_in_kms

        # Print the associated feature name and plate ID. And print the distance reconstructed.
        print 'Feature: %s' % reconstructed_feature_geometry.get_feature().get_name()
        print '  plate ID: %d' % reconstructed_feature_geometry.get_feature().get_reconstruction_plate_id()
        print '  distance reconstructed: %f kms' % distance_reconstructed_in_kms

Details
"""""""

| We define a function to return the centroid of a geometry.
| If the geometry is a :class:`pygplates.MultiPointOnSphere`, :class:`pygplates.PolylineOnSphere` or :class:`pygplates.PolygonOnSphere`
  then we can call ``get_centroid()`` on it (since those geometry types all have that method).
  However, if it's a :class:`pygplates.PointOnSphere` then it does not have that method, in which case we just return
  the point since it's already its own centroid.

::

    def get_geometry_centroid(geometry):
        try:
            return geometry.get_centroid()
        except AttributeError:
            return geometry

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.
::

    rotation_model = pygplates.RotationModel('rotations.rot')

Create a :class:`reconstruct model <pygplates.ReconstructModel>` from the reconstructable features and the rotation model.
::

    reconstruct_model = pygplates.ReconstructModel('features.gpml', rotation_model)

The features will be reconstructed to their 50Ma positions.
::

    reconstruction_time = 50

| All features are :meth:`reconstructed <pygplates.ReconstructModel.reconstruct_snapshot>` to 50Ma.
| We then :meth:`query the reconstructed geometries <pygplates.ReconstructSnapshot.get_reconstructed_geometries>`.

::

    reconstruct_snapshot = reconstruct_model.reconstruct_snapshot(reconstruction_time)
    reconstructed_feature_geometries = reconstruct_snapshot.get_reconstructed_geometries()

| We use our ``get_geometry_centroid()`` function to find the centroid of the
  :meth:`present day<pygplates.ReconstructedFeatureGeometry.get_present_day_geometry>` and
  :meth:`reconstructed<pygplates.ReconstructedFeatureGeometry.get_reconstructed_geometry>` geometries.
| We use the :meth:`pygplates.GeometryOnSphere.distance` function to calculate the shortest
  distance between the two centroids and convert it to kilometres using :class:`pygplates.Earth`.

::

    distance_reconstructed = pygplates.GeometryOnSphere.distance(
        get_geometry_centroid(reconstructed_feature_geometry.get_present_day_geometry()),
        get_geometry_centroid(reconstructed_feature_geometry.get_reconstructed_geometry()))
    distance_reconstructed_in_kms = distance_reconstructed * pygplates.Earth.mean_radius_in_kms

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
