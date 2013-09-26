def interpolate_total_reconstruction_pole(total_reconstruction_pole, time):
    """interpolate_total_reconstruction_pole(total_reconstruction_pole, time) -> FiniteRotation or None
    Interpolates the *total reconstruction pole* property value at the specified time.
    
    :param total_reconstruction_pole: the *total reconstruction pole* time-dependent property value
    :type total_reconstruction_pole: :class:`GpmlIrregularSampling`
    :param time: the time at which to interpolate
    :type time: float
    :rtype: :class:`FiniteRotation` or None
    :return: the interpolated rotation or None
    
    Returns ``None`` if the *total_reconstruction_pole* property value is not a :class:`GpmlIrregularSampling` with
    time samples containing :class:`GpmlFiniteRotation` instances (or *time* is not spanned by any time samples).
    
    See :func:`interpolate_total_reconstruction_sequence` and :func:`interpolate_finite_rotations` for a similar functions.
    """
    
    # Convert 'float' to 'GeoTimeInstant' (for time comparisons).
    time = GeoTimeInstant(time)
    
    try:
        # Filter out the disabled time samples.
        time_samples = filter(lambda ts: not ts.is_disabled(), total_reconstruction_pole.get_time_samples())
    except AttributeError:
        # The property value type is unexpected.
        return
    
    # If the requested time is later than the first (most-recent) time sample then
    # it is outside the time range of the time sample sequence.
    if time > time_samples[0].get_time():
        return
    
    # If time matches first time sample then no interpolation is required.
    if time == time_samples[0].get_time():
        return time_samples[0].get_value()
    
    # Find adjacent time samples that span the requested time.
    for i in range(1, len(time_samples)):
        # If the requested time is later than (more recent) or equal to the sample's time.
        if time >= time_samples[i].get_time():
            # Note that "time < time_samples[i-1].get_time()" rather than '<='
            # which means "time_samples[i-1].get_value() != time_samples[i].get_value()"
            # which means interpolate will not throw an exception for equal times.
            interpolated_rotation = interpolate_finite_rotations(
                    time_samples[i-1].get_value().get_finite_rotation(),
                    time_samples[i].get_value().get_finite_rotation(),
                    time_samples[i-1].get_time().get_value(),
                    time_samples[i].get_time().get_value(),
                    time.get_value())
            return interpolated_rotation


def interpolate_total_reconstruction_sequence(total_reconstruction_sequence_feature, time):
    """interpolate_total_reconstruction_sequence(total_reconstruction_sequence_feature, time) -> (int, int, FiniteRotation) or None
    Interpolates the total reconstruction poles in a *total reconstruction sequence* feature at the specified time.
    
    Features of type *total reconstruction sequence* are usually read from a GPML rotation file or a PLATES4 rotation ('.rot') file.
    
    :param total_reconstruction_sequence_feature: the rotation feature with :class:`feature type<FeatureType>` 'gpml:TotalReconstructionSequence'
    :type total_reconstruction_sequence_feature: :class:`Feature`
    :param time: the time at which to interpolate
    :type time: float
    :rtype: tuple(int, int, :class:`FiniteRotation`) or None
    :return: A tuple containing (fixed plate id, moving plate id, interpolated rotation) or None
    
    Returns ``None`` if the feature does not contain a 'gpml:fixedReferenceFrame' plate id,
    a 'gpml:movingReferenceFrame' plate id and a 'gpml:totalReconstructionPole' :class:`GpmlIrregularSampling` with
    time samples containing :class:`GpmlFiniteRotation` instances (or *time* is not spanned by any time samples).
    
    A feature with :class:`feature type<FeatureType>` 'gpml:TotalReconstructionSequence' should have these properties
    if it conforms to the GPlates Geological Information Model (GPGIM).
    
    See :func:`interpolate_total_reconstruction_pole` and :func:`interpolate_finite_rotations` for a similar functions.
    """

    fixed_plate_id = None
    moving_plate_id = None
    total_reconstruction_pole = None
    
    fixed_reference_frame_property_name = PropertyName.create_gpml('fixedReferenceFrame')
    moving_reference_frame_property_name = PropertyName.create_gpml('movingReferenceFrame')
    total_reconstruction_pole_property_name = PropertyName.create_gpml('totalReconstructionPole')
    
    # Find the fixed/moving plate ids and the time sample sequence of total reconstruction poles.
    for property in total_reconstruction_sequence_feature:
        try:
            if property.get_name() == fixed_reference_frame_property_name:
                fixed_plate_id = property.get_value().get_plate_id()
            elif property.get_name() == moving_reference_frame_property_name:
                moving_plate_id = property.get_value().get_plate_id()
            elif property.get_name() == total_reconstruction_pole_property_name:
                total_reconstruction_pole = property.get_value()
        except AttributeError:
            # The property value type did not match the property name.
            # This indicates the data does not conform to the GPlates Geological Information Model (GPGIM).
            return

    if not (fixed_plate_id and moving_plate_id and total_reconstruction_pole):
        return
    
    # Interpolate the 'GpmlIrregularSampling' of 'GpmlFiniteRotations'.
    interpolated_rotation = interpolate_total_reconstruction_pole(total_reconstruction_pole, time)
    
    if not interpolated_rotation:
        return
    
    return (fixed_plate_id, moving_plate_id, interpolated_rotation)
