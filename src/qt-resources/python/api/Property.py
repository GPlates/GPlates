def get_value(property, time=0, property_value_type=None):
    """get_value([time=0[, property_value_type=None]]) -> PropertyValue or None
    Extracts the value, of our possibly time-dependent property, at the reconstruction *time*.
    
    :param time: the time to extract value (defaults to present day)
    :type time: float
    :param property_value_type: the property value *type* to match, if specified (defaults to ``None``)
    :type property_value_type: a class inheriting :class:`PropertyValue`
    :rtype: :class:`PropertyValue` or None
    
    If this property has a time-dependent property value (:class:`GpmlConstantValue`,
    :class:`GpmlIrregularSampling` or :class:`GpmlPiecewiseAggregation`) then a nested property
    value is extracted at the reconstruction *time* and returned. Otherwise our property value
    instance is simply returned as is (since it's not a time-dependent property value).
    See :meth:`PropertyValue.get_value` for more details.
    
    Returns ``None`` if *property_value_type* is specified but does not match the type of the
    extracted property value.
    
    Returns ``None`` if *property_value_type* is one of the time-dependent types
    (:class:`GpmlConstantValue`, :class:`GpmlIrregularSampling` or :class:`GpmlPiecewiseAggregation`).
    
    Note that this method never returns a time-dependent property value (:class:`GpmlConstantValue`,
    :class:`GpmlIrregularSampling` or :class:`GpmlPiecewiseAggregation`).
    You can use :meth:`get_time_dependent_container` for that.
    """
    
    # Get the property value using a private method.
    property_value = property._get_value();
    
    # Look up the value at the specified time.
    return property_value.get_value(time, property_value_type)

# Add the module function as a class method.
Property.get_value = get_value
# Delete the module reference to the function - we only keep the class method.
del get_value


def get_time_dependent_container(property, time_dependent_property_value_type):
    """get_time_dependent_container(time_dependent_property_value_type) -> PropertyValue or None
    Returns the time-dependent property value container.
    
    :param time_dependent_property_value_type: the time-dependent property value container *type* to match
    :type time_dependent_property_value_type: :class:`GpmlConstantValue`, :class:`GpmlIrregularSampling` or :class:`GpmlPiecewiseAggregation`
    :rtype: :class:`PropertyValue` or None
    
    This method is useful if you want to access the time-dependent property value container directly.
    An example is inspecting or modifying the time samples in a :class:`GpmlIrregularSampling`.
    Otherwise :meth:`get_value` is generally more useful since it extracts a value from the container.
    
    Note that this method always returns a time-dependent property value (:class:`GpmlConstantValue`,
    :class:`GpmlIrregularSampling` or :class:`GpmlPiecewiseAggregation`), or ``None`` if the property
    value is not actually time-dependent. Alternatively you can use :meth:`get_value` for
    extracting a contained property value at a reconstruction time.
    
    Returns ``None`` if *time_dependent_property_value_type* does not match the type of the property value.
    
    Returns ``None`` if *time_dependent_property_value_type* is not one of the time-dependent types
    (:class:`GpmlConstantValue`, :class:`GpmlIrregularSampling` or :class:`GpmlPiecewiseAggregation`).
    """
    
    # Get the property value using a private method.
    property_value = property._get_value();
    
    # Return the property value if it matches the requested property value type (which must
    # also be one of the time-dependent types).
    if isinstance(property_value, time_dependent_property_value_type):
        if (isinstance(property_value, GpmlConstantValue) or
            isinstance(property_value, GpmlIrregularSampling) or
            isinstance(property_value, GpmlPiecewiseAggregation)):
            return property_value


# Add the module function as a class method.
Property.get_time_dependent_container = get_time_dependent_container
# Delete the module reference to the function - we only keep the class method.
del get_time_dependent_container
