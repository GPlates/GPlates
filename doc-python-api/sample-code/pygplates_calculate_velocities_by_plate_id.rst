.. _pygplates_calculate_velocities_by_plate_id:

Calculate velocities by plate ID
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example calculates velocities at all points in geometries of a collection of features
using the plate IDs of those features and a rotation model.

.. contents::
   :local:
   :depth: 2

Sample code
"""""""""""

::

    import pygplates
    

    # Load one or more rotation files into a rotation model.
    rotation_model = pygplates.RotationModel('rotations.rot')
    
    # Load the features that contain the geometries we will calculate velocities at.
    # Calling them 'domain' features since using them as input to velocities (but can be any type of feature).
    domain_features = pygplates.FeatureCollection('features.gpml')
    
    # Calculate velocities at 10Ma using a delta time interval of 1Ma.
    reconstruction_time = 10
    delta_time = 1
    
    # All reconstructed geometry points and associated (magnitude, azimuth, inclination) velocities.
    all_reconstructed_points = []
    all_velocities = []
    
    # Iterate over all geometries in all domain features and calculate velocities at each of their points.
    for domain_feature in domain_features:
        
        # We need the feature's plate ID to get the equivalent stage rotation of that tectonic plate.
        domain_plate_id = domain_feature.get_reconstruction_plate_id()
        
        # Get the rotation of plate 'domain_plate_id' from present day (0Ma) to 'reconstruction_time'.
        equivalent_total_rotation = rotation_model.get_rotation(reconstruction_time, domain_plate_id)
        
        # Get the rotation of plate 'domain_plate_id' from 'reconstruction_time + delta_time' to 'reconstruction_time'.
        equivalent_stage_rotation = rotation_model.get_rotation(
                reconstruction_time, domain_plate_id, reconstruction_time + delta_time)
        
        # A feature usually has a single geometry but it could have more - iterate over them all.
        for geometry in domain_feature.get_geometries():
        
            # Reconstruct the geometry to 'reconstruction_time'.
            reconstructed_geometry = equivalent_total_rotation * geometry
            reconstructed_points = reconstructed_geometry.get_points()

            # Calculate velocities at the reconstructed geometry points.
            # This is from 'reconstruction_time + delta_time' to 'reconstruction_time' on plate 'domain_plate_id'.
            velocity_vectors = pygplates.calculate_velocities(reconstructed_points, equivalent_stage_rotation, delta_time)

            # Convert global 3D velocity vectors to local (magnitude, azimuth, inclination) tuples (one tuple per point).
            velocities = pygplates.LocalCartesian.convert_from_geocentric_to_magnitude_azimuth_inclination(
                    reconstructed_points, velocity_vectors)

            # Append results for the current geometry to the final results.
            all_reconstructed_points.extend(reconstructed_points)
            all_velocities.extend(velocities)

Details
"""""""

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.
::

    rotation_model = pygplates.RotationModel('rotations.rot')

The features to calculate velocities at are loaded into a :class:`pygplates.FeatureCollection`.
They can be any :class:`type<pygplates.FeatureType>` of feature as long as they have a
:meth:`reconstruction plate ID<pygplates.Feature.get_reconstruction_plate_id>`
(and of course some :meth:`geometry<pygplates.Feature.get_geometries>`).
::

    domain_features = pygplates.FeatureCollection('features.gpml')

| The velocities are calculated at geometries reconstructed to their 10Ma positions.
| And the velocities are calculated over a 1My interval from 11Ma to 10Ma.

::

    reconstruction_time = 10
    delta_time = 1

:class:`pygplates.RotationModel` enables to calculate both the rotation from present day to 10Ma
of a particular tectonic plate relative to the anchor plate (defaults to zero):
::

    equivalent_total_rotation = rotation_model.get_rotation(reconstruction_time, domain_plate_id)

...and the *stage* rotation from 11Ma to 10Ma:
::

    equivalent_stage_rotation = rotation_model.get_rotation(
            reconstruction_time, domain_plate_id, reconstruction_time + delta_time)

| A :class:`pygplates.Feature` usually contains a single geometry property but sometimes it contains more.
| This is why we use :meth:`pygplates.Feature.get_geometries` instead of :meth:`pygplates.Feature.get_geometry`.
| Actually ``domain_feature.get_geometries()`` is just a convenient alternative to
  ``domain_feature.get_geometry(property_return=PropertyReturn.all)``.

::

    for geometry in domain_feature.get_geometries():

The :class:`geometries<pygplates.GeometryOnSphere>` extracted from :class:`features<pygplates.Feature>`
are in present day coordinates and need to be reconstructed to their 10Ma positions.
::

    reconstructed_geometry = equivalent_total_rotation * geometry

| The (reconstructed) geometry could be a :class:`pygplates.PointOnSphere`, :class:`pygplates.MultiPointOnSphere`,
  :class:`pygplates.PolylineOnSphere` or :class:`pygplates.PolygonOnSphere`.
| We convert it into a list of :class:`pygplates.PointOnSphere` to calculate velocities at using
  :meth:`pygplates.GeometryOnSphere.get_points`.

::

    reconstructed_points = reconstructed_geometry.get_points()

| The velocities are :func:`calculated<pygplates.calculate_velocities>` at the reconstructed geometry positions (10Ma) using the stage rotation.
| This returns a list of :class:`pygplates.Vector3D` (one global cartesian velocity vector per geometry point).

::

    velocity_vectors = pygplates.calculate_velocities(reconstructed_points, equivalent_stage_rotation, delta_time)

| If the velocities need to be in local (magnitude, azimuth, inclination) coordinates then the global
  cartesian vectors can be converted using :meth:`pygplates.LocalCartesian.convert_from_geocentric_to_magnitude_azimuth_inclination`.
| Note that each point in ``reconstructed_points`` determines a separate local coordinate system.
  For example, the velocity *azimuth* is relative to North as viewed from a particular point position.
  
::

    velocities = pygplates.LocalCartesian.convert_from_geocentric_to_magnitude_azimuth_inclination(
            reconstructed_points, velocity_vectors)

| Finally we add the reconstructed points and velocities to two large lists for *all* features.

::

    all_reconstructed_points.extend(reconstructed_points)
    all_velocities.extend(velocities)
