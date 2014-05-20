def get_value(gpml_irregular_sampling, time=0):
    """get_value([time=0]) -> PropertyValue or None
    Extracts the value at the reconstruction *time*.
    
    :param time: the time to extract value (defaults to present day)
    :type time: float or :class:`GeoTimeInstant`
    :rtype: :class:`PropertyValue` or None

    Returns ``None`` if *time* is outside the time range of the :meth:`time samples<get_time_samples>`.
    
    Note that the extracted property value is interpolated (at reconstruction *time*) if property
    value can be interpolated (currently only :class:`GpmlFiniteRotation` and :class:`XsDouble`),
    otherwise ``None`` is returned.
    
    This method overrides :meth:`PropertyValue.get_value`.
    """
    
    # Get the time samples bounding the time (and filter out the disabled time samples).
    adjacent_time_samples = gpml_irregular_sampling.get_time_samples_bounding_time(time)
    if adjacent_time_samples:
        begin_time_sample, end_time_sample = adjacent_time_samples
        
        # Use private module function defined in another module code section (file).
        return _interpolate_property_value(
                begin_time_sample.get_value(),
                end_time_sample.get_value(),
                begin_time_sample.get_time(),
                end_time_sample.get_time(),
                time)

# Add the module function as a class method.
GpmlIrregularSampling.get_value = get_value
# Delete the module reference to the function - we only keep the class method.
del get_value


def get_enabled_time_samples(gpml_irregular_sampling):
    """get_enabled_time_samples() -> list
    Filter out the disabled :class:`time samples<GpmlTimeSample>` and return a list of enabled time samples.
    
    :rtype: list
    :return: the list of enabled :class:`time samples<GpmlTimeSample>` (if any) or None
    
    Returns an empty list if all time samples are disabled.
    
    This method essentially does the following:
    ::
    
      return filter(lambda ts: ts.is_enabled(), get_time_samples())
    """

    # Filter out the disabled time samples.
    return filter(lambda ts: ts.is_enabled(), gpml_irregular_sampling.get_time_samples())

# Add the module function as a class method.
GpmlIrregularSampling.get_enabled_time_samples = get_enabled_time_samples
# Delete the module reference to the function - we only keep the class method.
del get_enabled_time_samples


def get_time_samples_bounding_time(gpml_irregular_sampling, time, include_disabled_samples=False):
    """get_time_samples_bounding_time(time[, include_disabled_samples=False]) -> tuple
    Return the two adjacent :class:`time samples<GpmlTimeSample>` that surround *time*.
    
    :param time: the time
    :type time: float or :class:`GeoTimeInstant`
    :param include_disabled_samples: if True then disabled time samples are included in the search
    :type include_disabled_samples: bool
    :rtype: the tuple (:class:`GpmlTimeSample`, :class:`GpmlTimeSample`), or None
    :return: the two time samples surrounding *time*, or None

    Returns ``None`` if *time* is outside the range of times (later than the most recent time sample
    or earlier than the least recent time sample).
    
    *Note:* The returned time samples are ordered forward in time (the first sample is further in the past than the second sample).
    This is opposite the typical ordering of time samples in a :class:`GpmlIrregularSampling` (which are progressively further
    back in time) but is similar to :class:`GpmlTimeWindow` (where its *begin* time is further in the past than its *end* time).
    """
    
    if include_disabled_samples:
        time_samples = gpml_irregular_sampling.get_time_samples()
    else:
        # Filter out the disabled time samples.
        time_samples = gpml_irregular_sampling.get_enabled_time_samples()
    
    if not time_samples:
        return
    
    # GpmlIrregularSampling should already be sorted most recent to least recent
    # (ie, backwards in time from present to past times).
    # But create a reversed list if need be.
    if time_samples[0].get_time() > time_samples[-1].get_time():
        time_samples = list(reversed(time_samples))
    
    # If the requested time is later than the first (most-recent) time sample then
    # it is outside the time range of the time sample sequence.
    if time < time_samples[0].get_time():
        return
    
    # Find adjacent time samples that span the requested time.
    for i in range(1, len(time_samples)):
        # If the requested time is later than (more recent) or equal to the sample's time.
        if time <= time_samples[i].get_time():
            # Return times ordered least recent to most recent.
            return (time_samples[i], time_samples[i-1])

# Add the module function as a class method.
GpmlIrregularSampling.get_time_samples_bounding_time = get_time_samples_bounding_time
# Delete the module reference to the function - we only keep the class method.
del get_time_samples_bounding_time
