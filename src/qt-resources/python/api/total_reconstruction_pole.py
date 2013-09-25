def interpolate_total_reconstruction_poles(total_reconstruction_sequence_feature, time):
    """interpolate_total_reconstruction_poles(total_reconstruction_sequence_feature, time) -> (int, int, FiniteRotation) or None
    Searches a 
    
    :param time: the time at which to interpolate
    :param type: :class:`GeoTimeInstant`
    :rtype: tuple or None
    :return: A tuple containing (fixed plate id, moving plate id, interpolated rotation)
    
    Returns ``None`` if the feature does not contain a :class:`GpmlIrregularSampling` of
    :class:`GpmlFiniteRotation` instances, or 
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
                total_reconstruction_pole = property.get_value().get_time_samples()
        except AttributeError:
            # The property value type did not match the property name.
            # This indicates the data does not conform to the GPlates Geological Information Model (GPGIM).
            return

    if not (fixed_plate_id and moving_plate_id and total_reconstruction_pole):
        return
    
    # Filter out the disabled time samples.
    time_samples = filter(lambda ts: not ts.is_disabled(), total_reconstruction_pole)
    if not time_samples:
        return
    
    # If the requested time is later than the first (most-recent) time sample then
    # it is outside the time range of the time sample sequence.
    if time > time_samples[0].get_time():
        return
    
    # If time matches first time sample then no interpolation is required.
    if time == time_samples[0].get_time():
        return (fixed_plate_id, moving_plate_id, time_samples[0].get_value())
    
    # Find adjacent time samples that span the requested time.
    for i in range(1, len(time_samples)):
        # If the requested time is later than (more recent) or equal to the sample's time.
        if time >= time_samples[i].get_time():
            # Note that "time < time_samples[i-1].get_time()" rather than '<='
            # which means "time_samples[i-1].get_value() != time_samples[i].get_value()"
            # which means interpolate will not throw an exception for equal times.
            interpolated_rotation = interpolate(
                    time_samples[i-1].get_value(),
                    time_samples[i].get_value(),
                    time_samples[i-1].get_time().get_value(),
                    time_samples[i].get_time().get_value(),
                    time)
            return (fixed_plate_id, moving_plate_id, interpolated_rotation)
