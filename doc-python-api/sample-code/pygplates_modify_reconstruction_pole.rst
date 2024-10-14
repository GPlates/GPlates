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

.. seealso:: :ref:`pygplates_create_total_reconstruction_sequence_feature`

.. seealso:: :ref:`pygplates_query_total_reconstruction_sequence_feature`

.. contents::
   :local:
   :depth: 2

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

    # Reconstruct our point feature to obtain the reconstructed point location.
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
            
            # The rotation adjustment needs to be applied to the rotation feature (total reconstruction pole).
            # Since this is a rotation relative to the fixed plate of the rotation feature, and not the anchored plate,
            # we need to transform the adjustment appropriately before applying it.
            fixed_plate_frame = rotation_model_before_adjustment.get_rotation(reconstruction_time, fixed_plate_id)
            fixed_plate_frame_rotation_adjustment = fixed_plate_frame.get_inverse() * rotation_adjustment * fixed_plate_frame
            adjusted_rotation = fixed_plate_frame_rotation_adjustment * rotation
            
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

The filenames of one or more rotation files. We'll be writing modifications back out to these files.
::

    rotation_filenames = ['rotations.rot']

| The rotation adjustment will get applied at 60Ma.
| We wrap the reconstruction time in a :class:`pygplates.GeoTimeInstant` purely because its comparison
  operators (==, !=, <, <=, >, >=) handle numerical tolerance in floating-point comparisons. This is
  a good idea in general when comparing floating-point numbers even though in our case the sample code
  would probably still work if we directly compared floating-point numbers (without a comparison threshold) -
  in other words if we wrote this as ``reconstruction_time = 60`` instead.

::

    reconstruction_time = pygplates.GeoTimeInstant(60)

| The desired reconstructed position is the location we want the present day point position to
  reconstruct to at 60Ma.
| We specify point locations by passing a latitude and longitude to :class:`pygplates.PointOnSphere`.

::

    present_day_latitude = -20
    present_day_longitude = 135
    present_day_position = pygplates.PointOnSphere(
        present_day_latitude, present_day_longitude)

    desired_reconstructed_latitude = -45
    desired_reconstructed_longitude = 130
    desired_reconstructed_position = pygplates.PointOnSphere(
        desired_reconstructed_latitude, desired_reconstructed_longitude)

| Before we can reconstruct the point location we need to create a :class:`pygplates.Feature`.
| This contains the information (plate ID and present day position) needed to reconstruct the point to the reconstruction time.

::

    point_feature = pygplates.Feature()
    point_feature.set_reconstruction_plate_id(reconstruction_plate_id)
    point_feature.set_geometry(present_day_position)

| We use the utility class :class:`pygplates.FeaturesFunctionArgument` to load our rotation file(s).
| This makes it a little easier for us to write changes to the rotation features back out to the same files.
| Alternatively we could have loaded each rotation file into its own :class:`pygplates.FeatureCollection` and then
  later :meth:`saved<pygplates.FeatureCollection.write>` them back to their rotation file(s).

::

    rotation_features = pygplates.FeaturesFunctionArgument(rotation_filenames)

| We use the unmodified rotation features to generate a :class:`rotation model<pygplates.RotationModel>`.
| We'll use this model to reconstruct the point and to help us make an adjustment to the total reconstruction pole.

::

    rotation_model_before_adjustment = pygplates.RotationModel(rotation_features.get_features())

| To find the *actual* reconstructed point location at 60Ma we :func:`reconstruct<pygplates.reconstruct>` our point feature.
| Since our point feature is valid for all time (by default if we don't :meth:`set its valid time<pygplates.Feature.set_valid_time>`)
  we should get one :class:`pygplates.ReconstructedFeatureGeometry` from which we obtain the
  :meth:`reconstructed point position<pygplates.ReconstructedFeatureGeometry.get_reconstructed_geometry>`.

::

    reconstructed_feature_geometries = []
    pygplates.reconstruct(point_feature, rotation_model_before_adjustment, reconstructed_feature_geometries, reconstruction_time)
    reconstructed_position = reconstructed_feature_geometries[0].get_reconstructed_geometry()

| If the *actual reconstructed position* differs from the *desired reconstructed position* then we need to adjust
  the appropriate rotation feature(s) so that they match.
| The rotation adjustment is the rotation from ``reconstructed_position`` to ``desired_reconstructed_position``.
  The rotation is created using the :meth:`constructor<pygplates.FiniteRotation.__init__>` of :class:`pygplates.FiniteRotation`.

::

    if reconstructed_position != desired_reconstructed_position:
        rotation_adjustment = pygplates.FiniteRotation(reconstructed_position, desired_reconstructed_position)

| Next we iterate over all the rotation features to find those whose moving plate ID matches the plate ID
  of our point feature. This is because we only want to our rotation adjustment to affect the plate on
  which our point lies (and all :ref:`child plates<pygplates_primer_plate_reconstruction_hierarchy>`
  at the reconstruction time).
| We obtain the moving/fixed plate IDs and the time-varying total reconstruction poles from the rotation feature
  using :meth:`pygplates.Feature.get_total_reconstruction_pole`.

::

    for rotation_feature in rotation_features.get_features():
        total_reconstruction_pole = rotation_feature.get_total_reconstruction_pole()
        if not total_reconstruction_pole:
            continue
        fixed_plate_id, moving_plate_id, rotation_sequence = total_reconstruction_pole
        if moving_plate_id != reconstruction_plate_id:
            continue

| A rotation sequence is a :class:`time sequence<pygplates.GpmlIrregularSampling>` of total rotations of
  a moving plate relative to a fixed plate.
| Not all rotation samples in the sequence are necessarily enabled. So we ignore the disabled samples by
  calling :meth:`pygplates.GpmlIrregularSampling.get_enabled_time_samples`.
| We use the enabled rotation samples to determine if the time range of the rotation sequence includes the reconstruction time.
| Note that since ``reconstruction_time`` is a :class:`pygplates.GeoTimeInstant`, comparisons with it
  will handle numerical tolerance (as mentioned above). This ensures that the test will pass if the
  reconstruction time coincides with the time of the first or last rotation sample.

::

    enabled_rotation_samples = rotation_sequence.get_enabled_time_samples()
    if not enabled_rotation_samples:
        continue
    if not (enabled_rotation_samples[0].get_time() <= reconstruction_time and
            enabled_rotation_samples[-1].get_time() >= reconstruction_time):
        continue

| If one of the enabled rotation samples matches the reconstruction time then
  get its description so we don't clobber it when we write the adjusted rotation.
| Each rotation sample usually has a comment/description in the rotation file and this
  enables us to retain them when writing back out to the rotation file.

::

    rotation_description = None
    for rotation_sample in enabled_rotation_samples:
        if rotation_sample.get_time() == reconstruction_time:
            rotation_description = rotation_sample.get_description()
            break

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

The reconstruction of the present day point position is given by the equation for the :ref:`pygplates_primer_equivalent_total_rotation`
which shows the equivalent total rotation of  moving plate :math:`P_{M}` (relative to anchored plate :math:`P_{A}`) at time :math:`t` (relative to present day) is:

.. math::

   \text{reconstructed_position} = R(0 \rightarrow t,P_{A} \rightarrow P_{M}) \times \text{present_day_position}

Using the approach in :ref:`pygplates_primer_composing_finite_rotations` we write the *desired reconstructed position*
in terms of the *actual reconstructed position*:

.. math::

   \text{desired_reconstructed_position} &= R(\text{reconstructed_position} \rightarrow \text{desired_reconstructed_position}) \times \text{reconstructed_position} \\
                         &= R(\text{reconstructed_position} \rightarrow \text{desired_reconstructed_position}) \times R(0 \rightarrow t,P_{A} \rightarrow P_{M}) \times \text{present_day_position}

...where the rotation adjustment :math:`R(\text{reconstructed_position} \rightarrow \text{desired_reconstructed_position})` represents the
:class:`rotation<pygplates.FiniteRotation>` from :math:`\text{reconstructed_position}` to :math:`\text{desired_reconstructed_position}` which (in pyGPlates) is
``pygplates.FiniteRotation(reconstructed_position, desired_reconstructed_position)``.

The composed rotation from *present day position* to *desired reconstructed position* represents the adjusted *equivalent* rotation:

.. math::

   \text{desired_reconstructed_position} &= R(0 \rightarrow t,P_{A} \rightarrow P_{M})_{adjusted} \times \text{present_day_position} \\
   R(0 \rightarrow t,P_{A} \rightarrow P_{M})_{adjusted} &= R(\text{reconstructed_position} \rightarrow \text{desired_reconstructed_position}) \times R(0 \rightarrow t,P_{A} \rightarrow P_{M})

| However we want to adjust a total rotation pole in a rotation feature. But a rotation feature represents a *relative* rotation between a moving and fixed plate pair.
| So we need to rewrite the adjusted *equivalent* rotation (which is relative to the anchored plate) as an adjusted *relative* rotation (relative to the fixed plate
  :math:`P_{F}` of the rotation feature/pole) using the result :math:`R(P_{A} \rightarrow P_{M}) = R(P_{A} \rightarrow P_{F}) \times R(P_{F} \rightarrow P_{M})`
  from :ref:`pygplates_primer_plate_circuit_paths`:

.. math::

   R(0 \rightarrow t,P_{A} \rightarrow P_{M})_{adjusted} &= R(\text{reconstructed_position} \rightarrow \text{desired_reconstructed_position}) \times R(0 \rightarrow t,P_{A} \rightarrow P_{M}) \\
   R(0 \rightarrow t,P_{A} \rightarrow P_{F}) \times R(0 \rightarrow t,P_{F} \rightarrow P_{M})_{adjusted} &= R(\text{reconstructed_position} \rightarrow \text{desired_reconstructed_position}) \times R(0 \rightarrow t,P_{A} \rightarrow P_{F}) \times R(0 \rightarrow t,P_{F} \rightarrow P_{M})

Pre-multiplying both sides by :math:`R(0 \rightarrow t,P_{A} \rightarrow P_{F})^{-1}` gives:

.. math::

   R(0 \rightarrow t,P_{F} \rightarrow P_{M})_{adjusted} &= R(0 \rightarrow t,P_{A} \rightarrow P_{F})^{-1} \times R(\text{reconstructed_position} \rightarrow \text{desired_reconstructed_position}) \times R(0 \rightarrow t,P_{A} \rightarrow P_{F}) \times R(0 \rightarrow t,P_{F} \rightarrow P_{M})

...which represents the *adjusted* relative rotation :math:`R(0 \rightarrow t,P_{F} \rightarrow P_{M})_{adjusted}`
in terms of the *original* relative rotation :math:`R(0 \rightarrow t,P_{F} \rightarrow P_{M})`.

This is written in pyGPlates as:
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

| Our rotation adjustment may require crossovers to be re-synchronised. This can happen when
  a child plate (a plate that moves relative to the plate we made the adjustment on) crosses over
  from another plate (or to another plate) at the reconstruction time of the rotation adjustment (60Ma).
  The two crossover rotations will no longer match resulting in a jump in the reconstruction.
| So we call :func:`pygplates.synchronise_crossovers` to synchronise all crossover rotations.
| How each encountered crossover is synchronised needs to be specified. For example, do we synchronise
  the younger or older rotation sequence (younger/older relative to the crossover time) ?  Here we
  use the function ``pygplates.CrossoverTypeFunction.type_from_xo_tags_in_comment_default_xo_ys`` to
  determine this for us. It will use ``@xo_`` tags in the rotation file (pole comments/descriptions)
  to determine this and default to the ``@xo_ys`` tag if not present for a particular crossover.
  See :func:`pygplates.synchronise_crossovers` for more details.
| Note that this modifies the rotation features in-place.

::

    if not pygplates.synchronise_crossovers(
            rotation_features.get_features(),
            crossover_threshold_degrees = 0.01,
            crossover_type_function = pygplates.CrossoverTypeFunction.type_from_xo_tags_in_comment_default_xo_ys):
        print >> sys.stderr, 'Unable to synchronise all crossovers.'


| Now we reconstruct the point feature again, but this time using the modified rotation features.
| This time the reconstructed point location should match the desired reconstructed point location.

::

    rotation_model_after_adjustment = pygplates.RotationModel(rotation_features.get_features())
    reconstructed_feature_geometries = []
    pygplates.reconstruct(point_feature, rotation_model_after_adjustment, reconstructed_feature_geometries, reconstruction_time)
    reconstructed_position = reconstructed_feature_geometries[0].get_reconstructed_geometry()
    
    print 'Reconstructed lat/lon position after adjustment (%f, %f)' % reconstructed_position.to_lat_lon()

| The last step is to write the (modified) rotation features back to the files they came from.
| This is made a little easier for us by using the ability of :class:`pygplates.FeaturesFunctionArgument`
  to list those feature collections that came from files as well as their associated filenames.

::

    rotation_files = rotation_features.get_files()
    if rotation_files:
        for feature_collection, filename in rotation_files:
            feature_collection.write(filename)

Output
""""""

::

    Reconstructed lat/lon position before adjustment (-45.962028, 131.398490)
    Desired reconstructed lat/lon position (-45.000000, 130.000000)
    Reconstructed lat/lon position after adjustment (-45.000000, 130.000000)
