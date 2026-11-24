.. _pygplates_create_conjugate_isochrons_from_ridge:

Create conjugate isochrons from a ridge
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example creates a conjugate pair of isochrons from each mid-ocean ridge, at each of a
series of geological times.

.. contents::
   :local:
   :depth: 2

Data files
""""""""""

``rotations.rot``
    A rotation file. It must contain rotations for the left and right plates of the ridges, since
    each ridge is reconstructed to each isochron creation time and the isochrons are reverse
    reconstructed from there to present day.

``ridges.gpml``
    Mid-ocean ridge features. Each must have left and right plate IDs, a half-stage reconstruction
    method (so that the ridge moves midway between its plates), a valid time period (the ridge's times
    of appearance and disappearance) and geometry. A name and description are copied to the isochrons
    if present.

Files of these kinds are in the GPlates `sample data <https://www.gplates.org/download/>`_.

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

:meth:`pygplates.Feature.get_left_plate`, :meth:`pygplates.Feature.get_right_plate` and
:meth:`pygplates.Feature.get_valid_time` return the left plate ID, the right plate ID and the
valid time period of each ridge. The valid time period is a tuple of the time the ridge appears
and the time it disappears.

.. sample-code:: pygplates_create_conjugate_isochrons_from_ridge.py
   :fragment: plate-ids-and-valid-time

Isochrons are created at a creation time only if the ridge exists then: after it appears, and
not after it disappears. Smaller time values are closer to present day (younger).

.. sample-code:: pygplates_create_conjugate_isochrons_from_ridge.py
   :fragment: creation-time-test

The ridge is reconstructed to ``isochron_creation_time`` by creating a
:class:`pygplates.ReconstructSnapshot`. At that time the ridge is where the two isochrons are, since
that is when their crust forms at the ridge.

.. sample-code:: pygplates_create_conjugate_isochrons_from_ridge.py
   :fragment: reconstruct-ridge

A Python list comprehension builds a list of :class:`geometries<pygplates.GeometryOnSphere>` from
the list of :class:`pygplates.ReconstructedFeatureGeometry`. These are the geometries of the
isochrons at ``isochron_creation_time``, not at present day.

.. sample-code:: pygplates_create_conjugate_isochrons_from_ridge.py
   :fragment: isochron-geometry

:meth:`pygplates.Feature.create_reconstructable_feature` creates each
`isochron <http://www.gplates.org/docs/gpgim/#gpml:Isochron>`_ feature. The left isochron moves
with the left plate of the ridge, and has the right plate as its conjugate. The right isochron,
created the same way, has the two plate IDs swapped.

.. sample-code:: pygplates_create_conjugate_isochrons_from_ridge.py
   :fragment: create-left-isochron

The *reverse_reconstruct* argument is needed because a :class:`feature<pygplates.Feature>` stores
its geometry at its present day position. So the geometry is *reverse* reconstructed, from
``isochron_creation_time`` to present day, using the rotation model and the reconstruction plate
ID of the isochron.

.. note:: Passing ``None`` to, for example, ``ridge_feature.get_name(None)`` means that ``None`` is
   returned if the ridge feature has no name. The isochron is then created without a
   :meth:`name<pygplates.Feature.get_name>` property.

Finally the isochrons are saved to a new file with :meth:`pygplates.FeatureCollection.write`.

.. sample-code:: pygplates_create_conjugate_isochrons_from_ridge.py
   :fragment: write-isochrons


Advanced
""""""""

A more robust script would check that each ridge feature really is a mid-ocean ridge, and that it
has left and right plate IDs and a valid time period:
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

Passing ``None`` in:
::

    left_plate_id = ridge_feature.get_left_plate(None)
    right_plate_id = ridge_feature.get_right_plate(None)
    valid_time = ridge_feature.get_valid_time(None)

...means that ``None`` is returned if the ridge feature does not have that property (the left
plate ID, for example).

Without the ``None``, a default value is returned for a missing property. For ``get_left_plate()``
and ``get_right_plate()`` the default is plate ID 0. For ``get_valid_time()`` it is a time period
from the *distant past* to the *distant future*.

See also
""""""""

- Reference: :meth:`pygplates.Feature.create_reconstructable_feature`, :class:`pygplates.ReconstructSnapshot`,
  :meth:`pygplates.Feature.get_left_plate`, :meth:`pygplates.Feature.get_right_plate`,
  :meth:`pygplates.Feature.get_valid_time`, :meth:`pygplates.FeatureCollection.write`
- Sample code: :ref:`pygplates_create_isochron_feature`, :ref:`pygplates_query_mid_ocean_ridge_feature`,
  :ref:`pygplates_split_isochron_into_ridges_and_transforms`
