def get_value(gpml_constant_value, time=0):
    """get_value([time=0]) -> PropertyValue or None
    Extracts the constant value contained within.
    
    :param time: the time to extract value (defaults to present day)
    :type time: float
    :rtype: :class:`PropertyValue` or None
    
    Since the contained property value is constant, the *time* parameter is ignored.
    
    This method overrides :meth:`PropertyValue.get_value`.
    """
    
    # Get the nested property value using a private method.
    nested_property_value = gpml_constant_value._get_value();
    
    # Recurse to extract contained property value.
    # Also note that it could be another time-dependent sequence (not that it should happen).
    return nested_property_value.get_value(time)

# Add the module function as a class method.
GpmlConstantValue.get_value = get_value
# Delete the module reference to the function - we only keep the class method.
del get_value
