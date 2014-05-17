def get_value(property_value, time=0):
    """get_value([time=0]) -> PropertyValue or None
    Extracts the value, of this possibly time-dependent property value, at the reconstruction *time*.
    
    :param time: the time to extract value (defaults to present day)
    :type time: float
    :rtype: :class:`PropertyValue` or None
    
    If this property value is a time-dependent property (:class:`GpmlConstantValue`,
    :class:`GpmlIrregularSampling` or :class:`GpmlPiecewiseAggregation`) then a nested property
    value is extracted at the reconstruction *time* and returned. Otherwise this property value
    instance is simply returned as is (since it's not a time-dependent property value).

    Returns ``None`` if this property value is a time-dependent property (:class:`GpmlConstantValue`,
    :class:`GpmlIrregularSampling` or :class:`GpmlPiecewiseAggregation`) and *time* is outside its
    time range (of time samples or time windows).
    
    Note that if this property value is a :class:`GpmlIrregularSampling` instance then the extracted
    property value is interpolated (at reconstruction *time*) if property value can be interpolated
    (currently only :class:`GpmlFiniteRotation` and :class:`XsDouble`), otherwise ``None`` is returned.
    
    The following example demonstrates extracting an interpolated finite rotation from a
    :class:`total reconstruction pole<GpmlIrregularSampling>` at time 20Ma:
    ::
    
      gpml_finite_rotation_20Ma = total_reconstruction_pole.get_value(20)
      if gpml_finite_rotation_20Ma:
        print 'Interpolated finite rotation at 20Ma: %s' % gpml_finite_rotation_20Ma.get_finite_rotation()
    """
    
    # By default we assume the property value is not time-dependent.
    # So just return it as is.
    #
    # Note that the time-dependent property value types GpmlConstantValue, GpmlIrregularSampling and
    # GpmlPiecewiseAggregation will override this behaviour.
    return property_value

# Add the module function as a class method.
PropertyValue.get_value = get_value
# Delete the module reference to the function - we only keep the class method.
del get_value
