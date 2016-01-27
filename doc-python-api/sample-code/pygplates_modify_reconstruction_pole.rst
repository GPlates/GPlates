.. _pygplates_modify_reconstruction_pole:

Modify a reconstruction pole
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example:

- reads a rotation file,
- modifies a total reconstruction pole such that a reconstructed point coincides with its desired location
  at a particular reconstruction time, and
- writes the modifications back to the same rotation file.

The functionality in this example is similar to the ``Modify Reconstruction Pole`` tool
in `GPlates <http://www.gplates.org>`_.

Sample code
"""""""""""

::

    import pygplates
    import sys


    # One or more rotation filenames.
    rotation_filenames = ['rotations.rot']

    # The reconstruction time (Ma) at which the rotation adjustment needs to be applied.
    reconstruction_time = pygplates.GeoTimeInstant(60)

    # The plate ID of the plate whose rotation we are adjusting.
    reconstruction_plate_id = 801

    # The present day point position specified using latitude and longitude (in degrees).
    present_day_latitude = -20
    present_day_longitude = 135
    present_day_position = pygplates.PointOnSphere(
        present_day_latitude, present_day_longitude)

    # The desired reconstructed point position specified using latitude and longitude (in degrees).
    desired_reconstructed_latitude = -45
    desired_reconstructed_longitude = 130
    desired_reconstructed_position = pygplates.PointOnSphere(
        desired_reconstructed_latitude, desired_reconstructed_longitude)

    # Create a point feature so we can reconstruct it using its plate ID.
    point_feature = pygplates.Feature()
    point_feature.set_reconstruction_plate_id(reconstruction_plate_id)
    point_feature.set_geometry(present_day_position)

    # Load the rotation features from rotation files.
    # Using a 'pygplates.FeaturesFunctionArgument' to make it easier to write changes back out to the files.
    rotation_features = pygplates.FeaturesFunctionArgument(rotation_filenames)

    # A rotation model using the rotation features before they are modified.
    rotation_model_before_adjustment = pygplates.RotationModel(rotation_features.get_features())

    reconstructed_feature_geometries = []
    pygplates.reconstruct(point_feature, rotation_model_before_adjustment, reconstructed_feature_geometries, reconstruction_time)
    reconstructed_position = reconstructed_feature_geometries[0].get_reconstructed_geometry()

    # Print the actual and desired reconstructed point positions to show they are different.
    print 'Reconstructed lat/lon position before adjustment (%f, %f)' % reconstructed_position.to_lat_lon()
    print 'Desired reconstructed lat/lon position (%f, %f)' % desired_reconstructed_position.to_lat_lon()

    # If the actual and desired reconstructed positions are different then adjust rotation feature(s) to make them the same.
    if reconstructed_position != desired_reconstructed_position:
        
        # The rotation that moves the actual reconstructed position to the desired position.
        rotation_adjustment = pygplates.FiniteRotation(reconstructed_position, desired_reconstructed_position)
        
        # Modify the rotation feature (or features) corresponding to the reconstruction plate ID.
        for rotation_feature in rotation_features.get_features():
            
            # Get the rotation feature information.
            total_reconstruction_pole = rotation_feature.get_total_reconstruction_pole()
            if not total_reconstruction_pole:
                # Not a rotation feature.
                continue
            
            fixed_plate_id, moving_plate_id, rotation_sequence = total_reconstruction_pole
            # We're only interested in rotation features whose moving plate ID matches are reconstruction plate ID.
            if moving_plate_id != reconstruction_plate_id:
                continue
            
            # Get the enabled rotation samples - ignore the disabled samples.
            enabled_rotation_samples = rotation_sequence.get_enabled_time_samples()
            if not enabled_rotation_samples:
                # All time samples are disabled.
                continue
            
            # Match sure the time span of the rotation feature's enabled samples spans the reconstruction time.
            if not (enabled_rotation_samples[0].get_time() <= reconstruction_time and
                    enabled_rotation_samples[-1].get_time() >= reconstruction_time):
                continue
            
            # Get the finite rotation at the reconstruction time.
            # If the reconstruction time is between rotation samples then it will get interpolated.
            rotation_property_value = rotation_sequence.get_value(reconstruction_time)
            if not rotation_property_value:
                continue
            
            rotation = rotation_property_value.get_finite_rotation()
            
            # Get the finite rotation of the fixed plate at the reconstruction time
            # (relative to the anchored plate and present day).
            fixed_plate_frame = rotation_model_before_adjustment.get_rotation(reconstruction_time, fixed_plate_id)
            
            adjusted_rotation = fixed_plate_frame.get_inverse() * rotation_adjustment * fixed_plate_frame * rotation
            
            # If one of the enabled rotation samples matches the reconstruction time then
            # get its description so we don't clobber it when we write the adjusted rotation.
            rotation_description = None
            for rotation_sample in enabled_rotation_samples:
                if rotation_sample.get_time() == reconstruction_time:
                    rotation_description = rotation_sample.get_description()
                    break
            
            # Set the adjusted rotation back into the rotation sequence.
            rotation_sequence.set_value(
                pygplates.GpmlFiniteRotation(adjusted_rotation),
                reconstruction_time,
                rotation_description)
        
        # Our rotation adjustment may require crossovers to be re-synchronised.
        if not pygplates.synchronise_crossovers(
                rotation_features.get_features(),
                crossover_threshold_degrees = 0.01,
                # Default to 'pygplates.CrossoverType.synch_old_crossover_and_stages' when/if crossover tags
                # are missing in the rotation file...
                crossover_type_function = pygplates.CrossoverTypeFunction.type_from_xo_tags_in_comment_default_xo_ys):
            print >> sys.stderr, 'Unable to synchronise all crossovers.'
        
        # Get a new rotation model that uses the adjusted rotation features.
        rotation_model_after_adjustment = pygplates.RotationModel(rotation_features.get_features())
        reconstructed_feature_geometries = []
        pygplates.reconstruct(point_feature, rotation_model_after_adjustment, reconstructed_feature_geometries, reconstruction_time)
        reconstructed_position = reconstructed_feature_geometries[0].get_reconstructed_geometry()
        
        # Print the adjusted reconstructed point position - should now be same as desired position.
        print 'Reconstructed lat/lon position after adjustment (%f, %f)' % reconstructed_position.to_lat_lon()
        
        # Write the (modified) rotation feature collections back to the files they came from.
        rotation_files = rotation_features.get_files()
        if rotation_files:
            for feature_collection, filename in rotation_files:
                feature_collection.write(filename)


Details
"""""""

| We obtain the original rotation (at the reconstruction time) from the rotation feature using :meth:`pygplates.GpmlIrregularSampling.get_value`.
| This will :meth:`interpolate<pygplates.FiniteRotation.interpolate>` between the two nearest rotation time samples in the rotation sequence
  if the reconstruction time does not coincide with a rotation sample.

::

    rotation_property_value = rotation_sequence.get_value(reconstruction_time)
    if not rotation_property_value:
        continue
    rotation = rotation_property_value.get_finite_rotation()

Now that we have the original rotation from the rotation feature we need to calculate a rotation adjustment such that the new rotation
will result in the *present day position* reconstructing to the *desired reconstructed position*.

The reconstruction of the present day point position is given by the equation for the :ref:`pygplates_foundations_equivalent_total_rotation`
which shows the equivalent total rotation of  moving plate :math:`P_{M}` (relative to anchored plate :math:`P_{A}`) at time :math:`t` (relative to present day) is:

.. math::

   \text{reconstructed_position} = R(0 \rightarrow t,P_{A} \rightarrow P_{M}) \times \text{present_day_position}

Using the approach in :ref:`pygplates_foundations_composing_finite_rotations` we write the *desired reconstructed position*
in terms of the *actual reconstructed position*:

.. math::

   \text{desired_reconstructed_position} &= R(\text{reconstructed_position} \rightarrow \text{desired_reconstructed_position}) \times \text{reconstructed_position} \\
                         &= R(\text{reconstructed_position} \rightarrow \text{desired_reconstructed_position}) \times R(0 \rightarrow t,P_{A} \rightarrow P_{M}) \times \text{present_day_position}

...where the rotation adjustment :math:`R(\text{reconstructed_position} \rightarrow \text{desired_reconstructed_position})` represents the
:class:`rotation<pygplates.FiniteRotation>` from :math:`\text{reconstructed_position}` to :math:`\text{desired_reconstructed_position}` which (in *pygplates*) is
``pygplates.FiniteRotation(reconstructed_position, desired_reconstructed_position)``.

The composed rotation from *present day position* to *desired reconstructed position* represents the adjusted *equivalent* rotation:

.. math::

   \text{desired_reconstructed_position} &= R(0 \rightarrow t,P_{A} \rightarrow P_{M})_{adjusted} \times \text{present_day_position} \\
   R(0 \rightarrow t,P_{A} \rightarrow P_{M})_{adjusted} &= R(\text{reconstructed_position} \rightarrow \text{desired_reconstructed_position}) \times R(0 \rightarrow t,P_{A} \rightarrow P_{M})

| However we want to adjust a total rotation pole in a rotation feature. But a rotation feature represents a *relative* rotation between a moving and fixed plate pair.
| So we need to rewrite the adjusted *equivalent* rotation (which is relative to the anchored plate) as an adjusted *relative* rotation (relative to the fixed plate
  :math:`P_{F}` of the rotation feature/pole) using the result :math:`R(P_{A} \rightarrow P_{M}) = R(P_{A} \rightarrow P_{F}) \times R(P_{F} \rightarrow P_{M})`
  from :ref:`pygplates_foundations_plate_circuit_paths`:

.. math::

   R(0 \rightarrow t,P_{A} \rightarrow P_{M})_{adjusted} &= R(\text{reconstructed_position} \rightarrow \text{desired_reconstructed_position}) \times R(0 \rightarrow t,P_{A} \rightarrow P_{M}) \\
   R(0 \rightarrow t,P_{A} \rightarrow P_{F}) \times R(0 \rightarrow t,P_{F} \rightarrow P_{M})_{adjusted} &= R(\text{reconstructed_position} \rightarrow \text{desired_reconstructed_position}) \times R(0 \rightarrow t,P_{A} \rightarrow P_{F}) \times R(0 \rightarrow t,P_{F} \rightarrow P_{M})

Pre-multiplying both sides by :math:`R(0 \rightarrow t,P_{A} \rightarrow P_{F})^{-1}` gives:

.. math::

   R(0 \rightarrow t,P_{F} \rightarrow P_{M})_{adjusted} &= R(0 \rightarrow t,P_{A} \rightarrow P_{F})^{-1} \times R(\text{reconstructed_position} \rightarrow \text{desired_reconstructed_position}) \times R(0 \rightarrow t,P_{A} \rightarrow P_{F}) \times R(0 \rightarrow t,P_{F} \rightarrow P_{M})

...which represents the *adjusted* relative rotation :math:`R(0 \rightarrow t,P_{F} \rightarrow P_{M})_{adjusted}`
in terms of the *original* relative rotation :math:`R(0 \rightarrow t,P_{F} \rightarrow P_{M})`.

This is written in pygplates as:
::

  fixed_plate_frame = rotation_model_before_adjustment.get_rotation(reconstruction_time, fixed_plate_id)
  adjusted_rotation = fixed_plate_frame.get_inverse() * rotation_adjustment * fixed_plate_frame * rotation

...where ``fixed_plate_frame`` represents :math:`R(0 \rightarrow t,P_{A} \rightarrow P_{F})`.

| Now that we have calculated the adjusted relative rotation we need to set it back in the rotation feature.
| The process of getting the original rotation, adjusting it and setting the adjusted rotation is essentially the following:

::

    rotation = rotation_sequence.get_value(reconstruction_time).get_finite_rotation()
    
    adjusted_rotation = fixed_plate_frame.get_inverse() * rotation_adjustment * fixed_plate_frame * rotation
    
    rotation_sequence.set_value(
        pygplates.GpmlFiniteRotation(adjusted_rotation),
        reconstruction_time,
        rotation_description)


And finally the output should look something like:
::

    Reconstructed lat/lon position before adjustment (-45.962028, 131.398490)
    Desired reconstructed lat/lon position (-45.000000, 130.000000)
    Reconstructed lat/lon position after adjustment (-45.000000, 130.000000)
