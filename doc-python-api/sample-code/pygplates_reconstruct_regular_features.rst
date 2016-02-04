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

    # Load some features.
    features = pygplates.FeatureCollection('features.gpml')

    # Reconstruct features to this geological time.
    reconstruction_time = 50
    
    # The filename of the exported reconstructed geometries.
    # It's a shapefile called 'reconstructed_50Ma.shp'.
    export_filename = 'reconstructed_{0}Ma.shp'.format(reconstruction_time)

    # Reconstruct the features to the reconstruction time and export them to a shapefile
    pygplates.reconstruct(features, rotation_model, export_filename, reconstruction_time)

Details
"""""""

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.
::

    rotation_model = pygplates.RotationModel('rotations.rot')

The reconstructable features are loaded into a :class:`pygplates.FeatureCollection`.
::

    features = pygplates.FeatureCollection('features.gpml')

The features will be reconstructed to their 50Ma positions.
::

    reconstruction_time = 50

| All features are reconstructed to 50Ma using :func:`pygplates.reconstruct`.
| We specify a filename to export the reconstructed geometries to.

::

    pygplates.reconstruct(features, rotation_model, export_filename, reconstruction_time)

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
        
        # See if geometry is a polygon.
        try:
            return geometry.get_interior_centroid()
        except AttributeError:
            # Not a polygon so keeping going.
            pass
        
        # See if geometry is a polyline or multipoint.
        try:
            return geometry.get_centroid()
        except AttributeError:
            # Not a polyline or multipoint so keeping going.
            pass
        
        # Geometry must be a point - it is already its own centroid.
        return geometry


    # Load one or more rotation files into a rotation model.
    rotation_model = pygplates.RotationModel('rotations.rot')

    # Load some features.
    features = pygplates.FeatureCollection('features.gpml')

    # Reconstruct features to this geological time.
    reconstruction_time = 50

    # Reconstruct the features to the reconstruction time.
    reconstructed_feature_geometries = []
    pygplates.reconstruct(features, rotation_model, reconstructed_feature_geometries, reconstruction_time)

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
| We don't necessarily know whether the geometry is a :class:`pygplates.PointOnSphere`,
  :class:`pygplates.MultiPointOnSphere`, :class:`pygplates.PolylineOnSphere` or :class:`pygplates.PolygonOnSphere`.
| Each geometry type requires a different method for obtaining its centroid.
  We use the standard Python approach of attempting to use a method and if it fails try something else.
| So first we see if it's a polygon and call :meth:`pygplates.PolygonOnSphere.get_interior_centroid`.
  Then we see if it's a polyline or multipoint - both of which have a ``get_centroid()`` method
  (:meth:`pygplates.PolylineOnSphere.get_centroid` and :meth:`pygplates.MultiPointOnSphere.get_centroid`).
  If they all fail then it must be a point geometry so we just return that as the centroid.

::

    def get_geometry_centroid(geometry):
        try:
            return geometry.get_interior_centroid()
        except AttributeError:
            pass
        try:
            return geometry.get_centroid()
        except AttributeError:
            pass
        return geometry

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.
::

    rotation_model = pygplates.RotationModel('rotations.rot')

The reconstructable features are loaded into a :class:`pygplates.FeatureCollection`.
::

    features = pygplates.FeatureCollection('features.gpml')

The features will be reconstructed to their 50Ma positions.
::

    reconstruction_time = 50

| All features are reconstructed to 50Ma using :func:`pygplates.reconstruct`.
| We specify a ``list`` for *reconstructed_feature_geometries* instead of a filename so that we
  can queries the reconstructed geometries easily.

::

    reconstructed_feature_geometries = []
    pygplates.reconstruct(features, rotation_model, reconstructed_feature_geometries, reconstruction_time)

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

.. seealso:: :ref:`pygplates_find_nearest_feature_to_a_point` for an example using the *group_with_feature*
   argument in :func:`pygplates.reconstruct`