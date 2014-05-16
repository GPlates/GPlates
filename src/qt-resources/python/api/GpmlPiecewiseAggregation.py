def get_value(gpml_piecewise_aggregation, time=0, property_value_type=None):
    """get_value([time=0[, property_value_type=None]]) -> PropertyValue or None
    Extracts the value at the reconstruction *time*.
    
    :param time: the time to extract value (defaults to present day)
    :type time: float
    :param property_value_type: the property value *type* to match, if specified (defaults to ``None``)
    :type property_value_type: a class inheriting :class:`PropertyValue`
    :rtype: :class:`PropertyValue` or None
    
    Returns ``None`` if *time* is outside the time ranges of all :meth:`time windows<get_time_windows>`.
    
    Returns ``None`` if *property_value_type* is specified but does not match the type of the
    extracted property value.
    
    Returns ``None`` if *property_value_type* is one of the time-dependent types
    (:class:`GpmlConstantValue`, :class:`GpmlIrregularSampling` or :class:`GpmlPiecewiseAggregation`).
    
    This method overrides :meth:`PropertyValue.get_value`.
    """
    
    time_window = get_time_window_containing_time(gpml_piecewise_aggregation, time)
    if time_window:
        # Recurse to extract contained property value.
        # Also note that it could be another time-dependent sequence.
        return time_window.get_value().get_value(time, property_value_type)

# Add the module function as a class method.
GpmlPiecewiseAggregation.get_value = get_value
# Delete the module reference to the function - we only keep the class method.
del get_value
