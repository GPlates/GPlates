def get_value(gpml_irregular_sampling, time=0, property_value_type=None):
    """get_value([time=0[, property_value_type=None]]) -> PropertyValue or None
    Extracts the value at the reconstruction *time*.
    
    :param time: the time to extract value (defaults to present day)
    :type time: float
    :param property_value_type: the property value *type* to match, if specified (defaults to ``None``)
    :type property_value_type: a class inheriting :class:`PropertyValue`
    :rtype: :class:`PropertyValue` or None

    Returns ``None`` if *time* is outside the time range of the :meth:`time samples<get_time_samples>`.
    
    Note that the extracted property value is interpolated (at reconstruction *time*) if property
    value can be interpolated (currently only :class:`GpmlFiniteRotation` and :class:`XsDouble`),
    otherwise ``None`` is returned.
    
    Returns ``None`` if *property_value_type* is specified but does not match the type of the
    extracted property value.
    
    Returns ``None`` if *property_value_type* is one of the time-dependent types
    (:class:`GpmlConstantValue`, :class:`GpmlIrregularSampling` or :class:`GpmlPiecewiseAggregation`).
    
    This method overrides :meth:`PropertyValue.get_value`.
    """
    
    # Get the time samples bounding the time (and filter out the disabled time samples).
    adjacent_time_samples = get_time_samples_bounding_time(gpml_irregular_sampling, time)
    if adjacent_time_samples:
        begin_time_sample, end_time_sample = adjacent_time_samples
        
        # If caller requested a specific property value type then check that first.
        if property_value_type and not isinstance(begin_time_sample.get_value(), property_value_type):
            return None
        
        # Use private module function defined in another module code section (file).
        return _interpolate_property_value(
                begin_time_sample.get_value(),
                end_time_sample.get_value(),
                begin_time_sample.get_time().get_value(),
                end_time_sample.get_time().get_value(),
                time)

# Add the module function as a class method.
GpmlIrregularSampling.get_value = get_value
# Delete the module reference to the function - we only keep the class method.
del get_value
