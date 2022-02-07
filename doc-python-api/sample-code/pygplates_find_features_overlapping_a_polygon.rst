.. _pygplates_find_features_overlapping_a_polygon:

Find reconstructed features overlapping a polygon
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example iterates over a collection of reconstructed features and finds those whose geometry overlaps a polygon.

.. contents::
   :local:
   :depth: 2

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
    
    # All features have their geometries tested for overlap with this polygon.
    # The polygon is created using a sequence of (latitude, longitude) points.
    polygon = pygplates.PolygonOnSphere([(0,0), (90,0), (0,90)])
    
    # Reconstruct the features to 10Ma.
    reconstructed_features = []
    pygplates.reconstruct(features, rotation_model, reconstructed_features, reconstruction_time, group_with_feature=True)
    
    # The list of features that overlap the polygon when reconstructed to 10Ma.
    overlapping_features = []
    
    # Iterate over all reconstructed features.
    for feature, feature_reconstructed_geometries in reconstructed_features:
        
        # Iterate over all reconstructed geometries of the current feature.
        for feature_reconstructed_geometry in feature_reconstructed_geometries:
            
            # Get the minimum distance from polygon to the current reconstructed geometry.
            # We treat the polygon as solid so anything inside it has a distance of zero.
            # We also treat the reconstructed geometry as solid (in case it's also a polygon).
            min_distance_to_feature = pygplates.GeometryOnSphere.distance(
                    polygon,
                    feature_reconstructed_geometry.get_reconstructed_geometry(),
                    geometry1_is_solid=True,
                    geometry2_is_solid=True)
            
            # A minimum distance of zero means the current reconstructed geometry either
            # intersects the polygon's boundary or is inside it (or both).
            if min_distance_to_feature == 0:
                overlapping_features.append(feature)
                
                # We've finished with the current feature (don't want to add it more than once).
                break
    
    # If there are any overlapping features then write them to a file.
    if overlapping_features:

        # Put the overlapping features in a feature collection so we can write them to a file.
        overlapping_feature_collection = pygplates.FeatureCollection(overlapping_features)

        # Create a filename (for overlapping features) with the reconstruction time in it.
        overlapping_features_filename = 'features_overlapping_at_{0}Ma.gpml'.format(reconstruction_time)

        # Write the overlapping features to a new file.
        overlapping_feature_collection.write(overlapping_features_filename)

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

The test polygon will capture all features whose reconstructed geometry(s) overlap it.
::

    polygon = pygplates.PolygonOnSphere([(0,0), (90,0), (0,90)])

| All features are reconstructed to 10Ma using :func:`pygplates.reconstruct`.
| We specify a ``list`` for *reconstructed_features* instead of a filename.
| We also set the output parameter *group_with_feature* to ``True`` (it defaults to ``False``)
  so that our :class:`reconstructed feature geometries<pygplates.ReconstructedFeatureGeometry>`
  are grouped with their :class:`feature<pygplates.Feature>`.

::

    reconstructed_features = []
    pygplates.reconstruct(features, rotation_model, reconstructed_features, reconstruction_time, group_with_feature=True)

Each item in the *reconstructed_features* list is a tuple containing a feature and its associated
reconstructed geometries.
::

    for feature, feature_reconstructed_geometries in reconstructed_features:

A feature can have more than one geometry and hence will have more than one *reconstructed* geometry.
::

    for feature_reconstructed_geometry in feature_reconstructed_geometries:

| Calculate the minimum distance between the polygon and a reconstructed feature geometry using :meth:`pygplates.GeometryOnSphere.distance`.
| *geometry1_is_solid* is set to True in case the reconstructed geometry lies entirely inside
  the polygon in which case it will return a distance of zero.
| If we did not specify this it would have returned the distance to the polygon's boundary outline
  which could be non-zero if the reconstructed geometry did not intersect the outline.
| And *geometry2_is_solid* is set to True in case the polygon lies entirely inside the reconstructed
  geometry (if it's a polygon also). This also constitutes an overlap.

::

    min_distance_to_feature = pygplates.GeometryOnSphere.distance(
            polygon,
            feature_reconstructed_geometry.get_reconstructed_geometry(),
            geometry1_is_solid=True,
            geometry2_is_solid=True)

| A minimum distance of zero means the current reconstructed geometry either intersects the polygon's
  boundary or is inside it.
| Or, conversely, the polygon could be inside the reconstructed geometry (if it's a polygon) which also constitutes an overlap.

::

    if min_distance_to_feature == 0:
        overlapping_features.append(feature)
        break

| Finally we write the overlapping features to a file.
| We could then load them into `GPlates <http://www.gplates.org>`_, reconstruct to 10Ma and check the results.

::

    overlapping_feature_collection = pygplates.FeatureCollection(overlapping_features)
    overlapping_features_filename = 'features_overlapping_at_{0}Ma.gpml'.format(reconstruction_time)
    overlapping_feature_collection.write(overlapping_features_filename)
