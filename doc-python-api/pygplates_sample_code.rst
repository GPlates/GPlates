.. _pygplates_sample_code:

Sample code
===========

This document contains sample code that shows *pygplates* solving common plate tectonic problems.

.. note:: The :ref:`pygplates_getting_started` section has a
   :ref:`Getting Started tutorial<pygplates_getting_started_tutorial>` as well as an
   :ref:`installion guide<pygplates_getting_started_installation>` to help get started.

.. contents::
   :local:
   :depth: 2

Calculate velocities by plate ID
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example calculates velocities at all points in :class:`feature<pygplates.Feature>` geometries
using the plate IDs of those features and a :class:`rotation model<pygplates.RotationModel>`.

::

    import pygplates
    

    # Load one or more rotation files into a rotation model.
    rotation_model = pygplates.RotationModel('rotations.rot')
    
    # Load the features that contain the geometries we will calculate velocities at.
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
        for geometry in domain_feature.get_geometry(property_return = pygplates.PropertyReturn.all):
        
            # Reconstruct the geometry to 'reconstruction_time'.
            reconstructed_geometry = equivalent_total_rotation * geometry
            reconstructed_points = reconstructed_geometry.get_points()
            
            # Calculate velocities at the geometry points.
            # This is from 'reconstruction_time + delta_time' to 'reconstruction_time' on plate 'domain_plate_id'.
            velocity_vectors = pygplates.calculate_velocities(reconstructed_points, equivalent_stage_rotation, delta_time)
            
            # Convert global 3D velocity vectors to local (magnitude, azimuth, inclination) tuples (one tuple per point).
            velocities = pygplates.LocalCartesian.convert_from_geocentric_to_magnitude_azimuth_inclination(
                    reconstructed_points, velocity_vectors)
            
            # Append results for the current geometry to the final results.
            all_reconstructed_points.extend(reconstructed_points)
            all_velocities.extend(velocities)

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
| This is why we need to specify ``pygplates.PropertyReturn.all`` in :meth:`pygplates.Feature.get_geometry`.

::

    for geometry in domain_feature.get_geometry(property_return = pygplates.PropertyReturn.all):

The :class:`geometries<pygplates.GeometryOnSphere>` extracted from :class:`features<pygplates.Feature>`
are in present day coordinates and need to be reconstructed to their 10Ma positions.
::

    reconstructed_geometry = equivalent_total_rotation * geometry

| The (reconstructed) geometry could be a :class:`pygplates.PointOnSphere`, :class:`pygplates.MultiPointOnSphere`,
  :class:`pygplates.PolylineOnSphere` or :class:`pygplates.PolygonOnSphere`.
| We convert it into a list of :class:`points<pygplates.PointOnSphere>` to calculate velocities at using
  :meth:`pygplates.GeometryOnSphere.get_points`.

::

    reconstructed_points = reconstructed_geometry.get_points()

| The velocities are :func:`calculated<pygplates.calculate_velocities>` at the reconstructed geometry positions (10Ma) using the stage rotation.
| This returns a list of :class:`pygplates.Vector3D` (one global cartesian velocity vector per geometry point).

::

    velocity_vectors = pygplates.calculate_velocities(reconstructed_points, equivalent_stage_rotation, delta_time)

| If the velocities need to be in local (magnitude, azimuth, inclination) coordinates then the global
  cartesian vectors can be converted using :meth:`pygplates.LocalCartesian.convert_from_geocentric_to_magnitude_azimuth_inclination`.
| Note that each point in ``reconstructed_points`` determines its own local coordinate system.
  For example, the velocity *azimuth* is relative to North as viewed from a particular point position.
  
::

    velocities = pygplates.LocalCartesian.convert_from_geocentric_to_magnitude_azimuth_inclination(
            reconstructed_points, velocity_vectors)
