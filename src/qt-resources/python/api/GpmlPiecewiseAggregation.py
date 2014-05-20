def get_value(gpml_piecewise_aggregation, time=0):
    """get_value([time=0]) -> PropertyValue or None
    Extracts the value at the reconstruction *time*.
    
    :param time: the time to extract value (defaults to present day)
    :type time: float or :class:`GeoTimeInstant`
    :rtype: :class:`PropertyValue` or None
    
    Returns ``None`` if *time* is outside the time ranges of all :meth:`time windows<get_time_windows>`.
    
    This method overrides :meth:`PropertyValue.get_value`.
    """
    
    time_window = gpml_piecewise_aggregation.get_time_window_containing_time(time)
    if time_window:
        # Recurse to extract contained property value.
        # Also note that it could be another time-dependent sequence.
        return time_window.get_value().get_value(time)

# Add the module function as a class method.
GpmlPiecewiseAggregation.get_value = get_value
# Delete the module reference to the function - we only keep the class method.
del get_value


def get_time_window_containing_time(gpml_piecewise_aggregation, time):
    """get_time_window_containing_time(time) -> GpmlTimeWindow or None
    Return the :class:`time window<GpmlTimeWindow>` that contains *time*.
    
    :param time: the time
    :type time: float or :class:`GeoTimeInstant`
    :rtype: :class:`GpmlTimeWindow` or None

    Returns ``None`` if *time* is outside the time ranges of all time windows.
    """
    
    # Find the time window containing the time.
    for window in gpml_piecewise_aggregation.get_time_windows():
        # Time period includes begin time but not end time.
        if time <= window.get_begin_time() and time >= window.get_end_time():
            return window

# Add the module function as a class method.
GpmlPiecewiseAggregation.get_time_window_containing_time = get_time_window_containing_time
# Delete the module reference to the function - we only keep the class method.
del get_time_window_containing_time
