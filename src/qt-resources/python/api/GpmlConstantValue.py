def get_value(gpml_constant_value, time=0, property_value_type=None):
    """get_value([time=0[, property_value_type=None]]) -> PropertyValue or None
    Extracts the constant value contained within.
    
    :param time: the time to extract value (defaults to present day)
    :type time: float
    :param property_value_type: the property value *type* to match, if specified (defaults to ``None``)
    :type property_value_type: a class inheriting :class:`PropertyValue`
    :rtype: :class:`PropertyValue` or None
    
    Since the contained property value is constant, the *time* parameter is ignored.
    
    Returns ``None`` if *property_value_type* is specified but does not match the type of the
    extracted property value.
    
    Returns ``None`` if *property_value_type* is one of the time-dependent types
    (:class:`GpmlConstantValue`, :class:`GpmlIrregularSampling` or :class:`GpmlPiecewiseAggregation`).
    
    This method overrides :meth:`PropertyValue.get_value`.
    """
    
    # Get the nested property value using a private method.
    nested_property_value = gpml_constant_value._get_value();
    
    # Recurse to extract contained property value.
    # Also note that it could be another time-dependent sequence (not that it should happen).
    return nested_property_value.get_value(time, property_value_type)

# Add the module function as a class method.
GpmlConstantValue.get_value = get_value
# Delete the module reference to the function - we only keep the class method.
del get_value
