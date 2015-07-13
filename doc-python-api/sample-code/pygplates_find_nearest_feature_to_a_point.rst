.. _pygplates_find_nearest_feature_to_a_point:

Find nearest reconstructed feature to a point
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example iterates over a collection of reconstructed features and finds the feature that is nearest to a point.

Sample code
"""""""""""

::

    import pygplates
    

    # Load one or more rotation files into a rotation model.
    rotation_model = pygplates.RotationModel('rotations.rot')
    
    # Load some features.
    features = pygplates.FeatureCollection('features.gpml')
    
    # Reconstruct features to 10Ma.
    reconstruction_time = 10
    
    # All features have their distance calculated relative to this point.
    point_latitude = 0
    point_longitude = 0
    point = pygplates.PointOnSphere(point_latitude, point_longitude)
    
    # Reconstruct the features to 10Ma.
    reconstructed_features = []
    pygplates.reconstruct(features, rotation_model, reconstructed_features, reconstruction_time, group_with_feature=True)
    
    # The minimum distance to all features and the nearest feature.
    min_distance_to_all_features = None
    nearest_feature = None
    
    # Iterate over all reconstructed features.
    for feature, feature_reconstructed_geometries in reconstructed_features:
        
        # Iterate over all reconstructed geometries of the current feature.
        for feature_reconstructed_geometry in feature_reconstructed_geometries:
            
            # Get the minimum distance from point to the current reconstructed geometry.
            min_distance_to_feature = pygplates.GeometryOnSphere.distance(
                    point,
                    feature_reconstructed_geometry.get_reconstructed_geometry(),
                    min_distance_to_all_features)
            
            # If the current geometry is nearer than all previous geometries then
            # its associated feature is the nearest feature so far.
            if min_distance_to_feature is not None:
                min_distance_to_all_features = min_distance_to_feature
                nearest_feature = feature
    
    if nearest_feature is not None:
        print "The nearest feature, to point %s, has feature ID %s and a minimum distance of %fkms" % (
                point.to_lat_lon(),
                nearest_feature.get_feature_id(),
                min_distance_to_all_features * pygplates.Earth.mean_radius_in_kms)

Details
"""""""

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.
::

    rotation_model = pygplates.RotationModel('rotations.rot')

The reconstructable features are loaded into a :class:`pygplates.FeatureCollection`.
::

    features = pygplates.FeatureCollection('features.gpml')

The features are reconstructed to their 10Ma positions.
::

    reconstruction_time = 10

| The test point has zero latitude and zero longitude.
| All features are tested to see which one is closest to this :class:`point<pygplates.PointOnSphere>`.

::

    point_latitude = 0
    point_longitude = 0
    point = pygplates.PointOnSphere(point_latitude, point_longitude)

| All features are reconstructed to 10Ma using :func:`pygplates.reconstruct`.
| We specify a ``list`` for *reconstructed_features* instead of a filename.
| We also set the output parameter *group_with_feature* to ``True`` (it defaults to ``False``)
  so that our :class:`reconstructed feature geometries<pygplates.ReconstructedFeatureGeometry>`
  are grouped with their :class:`feature<pygplates.Feature>`. This isn't strictly necessary
  in this particular example though.

::

    reconstructed_features = []
    pygplates.reconstruct(features, rotation_model, reconstructed_features, reconstruction_time, group_with_feature=True)

| Initially we don't have a minimum distance of the point to all features.
| This value is also used as the threshold to the :meth:`distance<pygplates.GeometryOnSphere.distance>`
  function and initially this will be ``None`` which means no threshold.

::

    min_distance_to_all_features = None
    nearest_feature = None

Each item in the *reconstructed_features* list is a tuple containing a feature and its associated
reconstructed geometries.
::

    for feature, feature_reconstructed_geometries in reconstructed_features:

A feature can have more than one geometry and hence will have more than one *reconstructed* geometry.
::

    for feature_reconstructed_geometry in feature_reconstructed_geometries:

| Calculate the minimum distance from the point to a reconstructed feature geometry using :meth:`pygplates.GeometryOnSphere.distance`.
| *min_distance_to_all_features* is specified as the distance threshold since we're only interested
  in geometries that are nearer than the closest geometry encountered so far.

::

    min_distance_to_feature = pygplates.GeometryOnSphere.distance(
            point,
            feature_reconstructed_geometry.get_reconstructed_geometry(),
            min_distance_to_all_features)

| If ``None`` was returned then the distance was greater than *min_distance_to_all_features*.
| So a valid returned value means the current geometry is the nearest geometry encountered so far.
| In this case we record the nearest feature and the new minimum distance.

::

    if min_distance_to_feature is not None:
        min_distance_to_all_features = min_distance_to_feature
        nearest_feature = feature

Once we've tested all features (if any were in the file) we print out the nearest feature and its
(minimum) distance to the point.
::

    if nearest_feature is not None:
        print "The nearest feature, to point %s, has feature ID %s and a minimum distance of %fkms" % (
                point.to_lat_lon(),
                nearest_feature.get_feature_id(),
                min_distance_to_all_features * pygplates.Earth.mean_radius_in_kms)
