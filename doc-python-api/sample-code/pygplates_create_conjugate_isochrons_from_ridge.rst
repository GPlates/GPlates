.. _pygplates_create_conjugate_isochrons_from_ridge:

Create conjugate isochrons from a ridge
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example creates a conjugate pair of isochrons from a mid-ocean ridge at each specified geological time in a series.

.. contents::
   :local:
   :depth: 2

Sample code
"""""""""""

::

    import pygplates


    # Load one or more rotation files into a rotation model.
    rotation_model = pygplates.RotationModel('rotations.rot')

    # Load the mid-ocean ridge features.
    ridge_features = pygplates.FeatureCollection('ridges.gpml')

    # The times at which to create isochrons.
    isochron_creation_times = [40, 30, 20, 10, 0]
    
    # We'll store the created isochrons here - later we'll write it to a file.
    isochron_feature_collection = pygplates.FeatureCollection()

    # Iterate over the ridge features.
    for ridge_feature in ridge_features:
        
        # Get the ridge left and right plate ids, and time of appearance.
        left_plate_id = ridge_feature.get_left_plate()
        right_plate_id = ridge_feature.get_right_plate()
        time_of_appearance, time_of_disappearance = ridge_feature.get_valid_time()
        
        # Iterate over our list of creation times for the left/right isochrons.
        for isochron_creation_time in isochron_creation_times:
            
            # If creation time is later than ridge birth time then we can create an isochron.
            if isochron_creation_time < time_of_appearance:
                
                # Reconstruct the mid-ocean ridge to isochron creation time.
                # The ridge geometry will be in the same position as the left/right isochrons at that time.
                reconstructed_ridges = []
                pygplates.reconstruct(ridge_feature, rotation_model, reconstructed_ridges, isochron_creation_time)
                
                # Get the isochron geometry from the ridge reconstruction.
                # This is the geometry at 'isochron_creation_time' (not present day).
                isochron_geometry_at_creation_time = [reconstructed_ridge.get_reconstructed_geometry()
                        for reconstructed_ridge in reconstructed_ridges]
                
                # Create the left and right isochrons.
                # Since they are conjugates they have swapped left and right plate IDs.
                # And reverse reconstruct the mid-ocean ridge geometries to present day.
                left_isochron_feature = pygplates.Feature.create_reconstructable_feature(
                        pygplates.FeatureType.gpml_isochron,
                        isochron_geometry_at_creation_time,
                        name = ridge_feature.get_name(None),
                        description = ridge_feature.get_description(None),
                        valid_time = (isochron_creation_time, 0),
                        reconstruction_plate_id = left_plate_id,
                        conjugate_plate_id = right_plate_id,
                        reverse_reconstruct = (rotation_model, isochron_creation_time))
                right_isochron_feature = pygplates.Feature.create_reconstructable_feature(
                        pygplates.FeatureType.gpml_isochron,
                        isochron_geometry_at_creation_time,
                        name = ridge_feature.get_name(None),
                        description = ridge_feature.get_description(None),
                        valid_time = (isochron_creation_time, 0),
                        reconstruction_plate_id = right_plate_id,
                        conjugate_plate_id = left_plate_id,
                        reverse_reconstruct = (rotation_model, isochron_creation_time))
                
                # Add isochrons to feature collection.
                isochron_feature_collection.add(left_isochron_feature)
                isochron_feature_collection.add(right_isochron_feature)
    
    # Write the isochrons to a new file.
    isochron_feature_collection.write('isochrons.gpml')


Details
"""""""

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.
::

    rotation_model = pygplates.RotationModel('rotations.rot')

The ridge features are loaded into a :class:`pygplates.FeatureCollection`.
::

    ridge_features = pygplates.FeatureCollection('ridges.gpml')

The plate IDs and time period are obtained using :meth:`pygplates.Feature.get_left_plate`,
:meth:`pygplates.Feature.get_right_plate` and :meth:`pygplates.Feature.get_valid_time`.
::

    left_plate_id = ridge_feature.get_left_plate()
    right_plate_id = ridge_feature.get_right_plate()
    time_of_appearance, time_of_disappearance = ridge_feature.get_valid_time()

Smaller time values are closer to present day (younger).
::

    if isochron_creation_time < time_of_appearance:

The ridges are reconstructed to their locations at time 'isochron_creation_time' using
:meth:`pygplates.reconstruct`.
::

    reconstructed_ridges = []
    pygplates.reconstruct(ridge_feature, rotation_model, reconstructed_ridges, isochron_creation_time)

A Python list comprehension is used to build a list of :class:`pygplates.GeometryOnSphere` from a
list of :class:`pygplates.ReconstructedFeatureGeometry`.
::

    isochron_geometry_at_creation_time = [reconstructed_ridge.get_reconstructed_geometry()
            for reconstructed_ridge in reconstructed_ridges]

`Isochron <http://www.gplates.org/docs/gpgim/#gpml:Isochron>`_ features are created using
:meth:`pygplates.Feature.create_reconstructable_feature`.
::

    left_isochron_feature = pygplates.Feature.create_reconstructable_feature(
            pygplates.FeatureType.gpml_isochron,
            isochron_geometry_at_creation_time,
            name = ridge_feature.get_name(None),
            description = ridge_feature.get_description(None),
            valid_time = (isochron_creation_time, 0),
            reconstruction_plate_id = left_plate_id,
            conjugate_plate_id = right_plate_id,
            reverse_reconstruct = (rotation_model, isochron_creation_time))

The ``reverse_reconstruct`` parameter is needed because all :class:`features<pygplates.Feature>`
must store their geometry in present day coordinates which means *reverse* reconstructing from
``isochron_creation_time`` to present day using the rotation model.

.. note:: The use of ``None`` in, for example, ``ridge_feature.get_name(None)`` results in a
   :meth:`name<pygplates.Feature.get_name>` property only getting created if the ridge feature has a name.

And finally the isochrons are saved to a new file using :meth:`pygplates.FeatureCollection.write`.
::

    isochron_feature_collection.write('isochrons.gpml')


Advanced
""""""""

If we want to be a bit more robust then we can check that our ridge features are actually ridges and
we can make sure they contain left/right plate IDs and a time of appearance/disappearance:
::

    ...
    
    # Iterate over the ridge features.
    for ridge_feature in ridge_features:
    
        # Ignore anything that's not a mid-ocean ridge.
        if ridge_feature.get_feature_type() != pygplates.FeatureType.gpml_mid_ocean_ridge:
            continue
        
        # Get the ridge left and right plate ids, and time of appearance.
        # We don't need to specify 'None', but if we do then it allows us to test if the ridge feature
        # is missing plate IDs or begin/end time period.
        left_plate_id = ridge_feature.get_left_plate(None)
        right_plate_id = ridge_feature.get_right_plate(None)
        valid_time = ridge_feature.get_valid_time(None)
        
        # Ignore mid-ocean ridges that don't have a left and right plate id and time of appearance.
        if (left_plate_id is None or
            right_plate_id is None or
            valid_time is None):
            continue
        
        # Extract time of appearance/disappearance from the tuple.
        time_of_appearance, time_of_disappearance = valid_time
        
        ...

By specifying ``None`` in:
::

    left_plate_id = ridge_feature.get_left_plate(None)
    right_plate_id = ridge_feature.get_right_plate(None)
    valid_time = ridge_feature.get_valid_time(None)

| ...we will get ``None`` returned to us if the feature property (eg, left plate ID) is missing
  in the ridge feature.
| If we didn't specify ``None`` then a default value would be returned if a property
  was missing. For ``get_left_plate()`` and ``get_right_plate()`` this is plate ID 0 and for
  ``get_valid_time()`` this is a time period from *distant past* to *distant future*.
