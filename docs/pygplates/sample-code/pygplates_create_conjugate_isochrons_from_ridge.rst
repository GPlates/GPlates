.. _pygplates_create_conjugate_isochrons_from_ridge:

Create conjugate isochrons from a ridge
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example creates a conjugate pair of isochrons from a mid-ocean ridge at each specified geological time in a series.

.. contents::
   :local:
   :depth: 2

Sample code
"""""""""""

.. sample-code:: pygplates_create_conjugate_isochrons_from_ridge.py

Details
"""""""

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.

.. sample-code:: pygplates_create_conjugate_isochrons_from_ridge.py
   :fragment: load-rotations

The ridge features are loaded into a :class:`pygplates.FeatureCollection`.

.. sample-code:: pygplates_create_conjugate_isochrons_from_ridge.py
   :fragment: load-ridges

The plate IDs and time period are obtained using :meth:`pygplates.Feature.get_left_plate`,
:meth:`pygplates.Feature.get_right_plate` and :meth:`pygplates.Feature.get_valid_time`.

.. sample-code:: pygplates_create_conjugate_isochrons_from_ridge.py
   :fragment: plate-ids-and-valid-time

Smaller time values are closer to present day (younger).

.. sample-code:: pygplates_create_conjugate_isochrons_from_ridge.py
   :fragment: creation-time-test

The ridges are reconstructed to their locations at time 'isochron_creation_time' using
:meth:`pygplates.reconstruct`.

.. sample-code:: pygplates_create_conjugate_isochrons_from_ridge.py
   :fragment: reconstruct-ridge

A Python list comprehension is used to build a list of :class:`pygplates.GeometryOnSphere` from a
list of :class:`pygplates.ReconstructedFeatureGeometry`.

.. sample-code:: pygplates_create_conjugate_isochrons_from_ridge.py
   :fragment: isochron-geometry

`Isochron <http://www.gplates.org/docs/gpgim/#gpml:Isochron>`_ features are created using
:meth:`pygplates.Feature.create_reconstructable_feature`.

.. sample-code:: pygplates_create_conjugate_isochrons_from_ridge.py
   :fragment: create-left-isochron

The ``reverse_reconstruct`` parameter is needed because all :class:`features<pygplates.Feature>`
must store their geometry in present day coordinates which means *reverse* reconstructing from
``isochron_creation_time`` to present day using the rotation model.

.. note:: The use of ``None`` in, for example, ``ridge_feature.get_name(None)`` results in a
   :meth:`name<pygplates.Feature.get_name>` property only getting created if the ridge feature has a name.

And finally the isochrons are saved to a new file using :meth:`pygplates.FeatureCollection.write`.

.. sample-code:: pygplates_create_conjugate_isochrons_from_ridge.py
   :fragment: write-isochrons


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
