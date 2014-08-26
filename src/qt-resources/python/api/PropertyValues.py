# Copyright (C) 2014 The University of Sydney, Australia
# 
# This file is part of GPlates.
# 
# GPlates is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License, version 2, as published by
# the Free Software Foundation.
# 
# GPlates is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
# 
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.


# Private helper class (has '_' prefix) for interpolation.
class _InterpolateVisitor(PropertyValueVisitor):
    def __init__(self, property_value1, property_value2, time1, time2, target_time):
        super(_InterpolateVisitor, self).__init__()
        self.property_value1 = property_value1
        self.property_value2 = property_value2
        # Make sure time values are 'GeoTimeInstant' and not 'float'...
        self.time1 = GeoTimeInstant(time1)
        self.time2 = GeoTimeInstant(time2)
        self.target_time = GeoTimeInstant(target_time)
        self.interpolated_property_value = None
        # Times must not be distant past/future (since then cannot interpolate).
        if not self.time1.is_real() or not self.time2.is_real() or not self.target_time.is_real():
            raise InterpolationError(
                "Interpolation time values cannot be distant-past (float('inf')) or distant-future (float('-inf')).")
    
    def interpolate(self):
        self.interpolated_property_value = None
        # Visit the first property value.
        self.property_value1.accept_visitor(self)
        return self.interpolated_property_value
    
    # Currently only GpmlFiniteRotation and XsDouble can be interpolated...
    
    def visit_gpml_finite_rotation(self, gpml_finite_rotation1):
        # No need to test 'time1==time2' since 'FiniteRotation.interpolate()' does that for us.
        
        # Second property value should also be the same type.
        self.interpolated_property_value = GpmlFiniteRotation(
            FiniteRotation.interpolate(
                gpml_finite_rotation1.get_finite_rotation(), self.property_value2.get_finite_rotation(),
                self.time1, self.time2, self.target_time))
    
    def visit_xs_double(self, xs_double1):
        # Use the epsilon comparison implicitly provided by GeoTimeInstant.
        if self.time2 == self.time1:
            return xs_double1
        
        interpolation = (self.target_time.get_value() - self.time1.get_value()) / (
                self.time2.get_value() - self.time1.get_value())
        
        # Second property value should also be the same type.
        self.interpolated_property_value = XsDouble(
            (1 - interpolation) * xs_double1.get_double() +
                interpolation * self.property_value2.get_double())


# This function is private in this 'pygplates' module (function name prefixed with a single underscore).
# It's used by other sections of the 'pygplates' module, but not meant for public use.
# It's only in this code section because it does interpolation.
def _interpolate_property_value(property_value1, property_value2, time1, time2, target_time):
    
    visitor = _InterpolateVisitor(property_value1, property_value2, time1, time2, target_time)
    return visitor.interpolate()


def property_value_get_value(property_value, time=0):
    """get_value([time=0]) -> PropertyValue or None
    Extracts the value, of this possibly time-dependent property value, at the reconstruction *time*.
    
    :param time: the time to extract value (defaults to present day)
    :type time: float or :class:`GeoTimeInstant`
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
PropertyValue.get_value = property_value_get_value
# Delete the module reference to the function - we only keep the class method.
del property_value_get_value


def gpml_constant_value_get_value(gpml_constant_value, time=0):
    """get_value([time=0]) -> PropertyValue or None
    Extracts the constant value contained within.
    
    :param time: the time to extract value (defaults to present day)
    :type time: float or :class:`GeoTimeInstant`
    :rtype: :class:`PropertyValue` or None
    
    Since the contained property value is constant, the *time* parameter is ignored.
    
    This method overrides :meth:`PropertyValue.get_value`.
    """
    
    # Get the nested property value using a private method.
    nested_property_value = gpml_constant_value._get_value()
    
    # Recurse to extract contained property value.
    # Also note that it could be another time-dependent sequence (not that it should happen).
    return nested_property_value.get_value(time)

# Add the module function as a class method.
GpmlConstantValue.get_value = gpml_constant_value_get_value
# Delete the module reference to the function - we only keep the class method.
del gpml_constant_value_get_value


def gpml_piecewise_aggregation_get_value(gpml_piecewise_aggregation, time=0):
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
GpmlPiecewiseAggregation.get_value = gpml_piecewise_aggregation_get_value
# Delete the module reference to the function - we only keep the class method.
del gpml_piecewise_aggregation_get_value


def gpml_piecewise_aggregation_set_value(gpml_piecewise_aggregation, property_value, begin_time, end_time):
    """set_value(property_value, begin_time, end_time) -> GpmlTimeWindow
    Sets the value in the specified time window.
    
    :param property_value: the property value to set
    :type property_value: :class:`PropertyValue`
    :param begin_time: the begin time of the time window for the new property value
    :type begin_time: float or :class:`GeoTimeInstant`
    :param end_time: the end time of the time window for the new property value
    :type end_time: float or :class:`GeoTimeInstant`
    :returns: the time window that is inserted into the time window sequence
    :rtype: :class:`GpmlTimeWindow`
    :raises: GmlTimePeriodBeginTimeLaterThanEndTimeError if begin time is later than end time

    Any existing :meth:`time windows<get_time_windows>` that overlap the new time window
    (*begin_time*, *end_time*) are clipped, removed or split (depending on how they overlap) such
    that they no longer overlap with the new time window. The new time window is then inserted into
    the existing sequence such that the :meth:`time windows<get_time_windows>` remained ordered by time.
    
    This method assumes the precondition that the :meth:`time windows<get_time_windows>` are
    ordered by time from most recent to least recent.
    
    Note that *begin_time* can be :meth:`distant past<GeoTimeInstant.create_distant_past>` and/or
    *end_time* can be :meth:`distant future<GeoTimeInstant.create_distant_future>`
    
    Set the property value between :meth:`distant past<GeoTimeInstant.create_distant_past>` and 10Ma:
    ::
    
      piecewise_aggregation.set_value(property_value, pygplates.GeoTimeInstant.create_distant_past(), 10)
    
    Alternatively you can change the value in an existing time window (if you just want to change
    the value within that time window and not affect other time windows):
    ::
    
      time_window = piecewise_aggregation.get_time_window_containing_time(time)
      if time_window:
          time_window.set_value(property_value)
    """
    
    # Use GeoTimeInstant to compare (within epsilon) floating point time values.
    begin_time = GeoTimeInstant(begin_time)
    end_time = GeoTimeInstant(end_time)
    
    if begin_time < end_time:
        raise GmlTimePeriodBeginTimeLaterThanEndTimeError("Begin time cannot be later than end time.")
    
    time_windows = gpml_piecewise_aggregation.get_time_windows()
    len_time_windows = len(time_windows)
    
    # Loop over existing time windows and adjust their extents if they overlap with new time window.
    time_window_index = 0
    while time_window_index < len_time_windows:
        time_window = time_windows[time_window_index]
        time_window_begin_time = time_window.get_begin_time()
        time_window_end_time = time_window.get_end_time()
        
        # If current time window overlaps new time window...
        if begin_time > time_window_end_time and end_time < time_window_begin_time:
            if begin_time >= time_window_begin_time:
                if end_time > time_window_end_time:
                    time_window.set_begin_time(end_time)
                else:
                    # Current time window is completely overlapped by new time window so delete current window.
                    del time_windows[time_window_index]
                    time_window_index = time_window_index - 1
                    len_time_windows = len_time_windows - 1
            else:
                if end_time <= time_window_end_time:
                    time_window.set_end_time(begin_time)
                else:
                    # Current time window completely overlaps the new time window so split current window into two.
                    time_window.set_end_time(begin_time)
                    # The second split window needs to be created - clone the property value of the first split window.
                    adjusted_time_window = GpmlTimeWindow(time_window.get_value().clone(), end_time, time_window_end_time)
                    # Insert just before the current time window (since inserted window is more recent).
                    time_windows.insert(time_window_index, adjusted_time_window)
                    time_window_index = time_window_index + 1
                    len_time_windows = len_time_windows + 1
        
        time_window_index = time_window_index + 1
    
    # Insert the new time window into the existing windows in the correct time order.
    for time_window_index, time_window in enumerate(time_windows):
        if begin_time < time_window.get_begin_time():
            new_time_window = GpmlTimeWindow(property_value, begin_time, end_time)
            time_windows.insert(time_window_index, new_time_window)
            return new_time_window
    
    # The begin time of the new window is further in the past than any existing time windows (so just append to end of sequence).
    new_time_window = GpmlTimeWindow(property_value, begin_time, end_time)
    time_windows.append(new_time_window)
    
    return new_time_window

# Add the module function as a class method.
GpmlPiecewiseAggregation.set_value = gpml_piecewise_aggregation_set_value
# Delete the module reference to the function - we only keep the class method.
del gpml_piecewise_aggregation_set_value


def gpml_piecewise_aggregation_get_time_window_containing_time(gpml_piecewise_aggregation, time):
    """get_time_window_containing_time(time) -> GpmlTimeWindow or None
    Return the :class:`time window<GpmlTimeWindow>` that contains *time*.
    
    :param time: the time
    :type time: float or :class:`GeoTimeInstant`
    :rtype: :class:`GpmlTimeWindow` or None

    Returns ``None`` if *time* is outside the time ranges of all time windows.
    """
    
    # Find the time window containing the time.
    for window in gpml_piecewise_aggregation.get_time_windows():
        # Time period includes begin time and end time.
        if time <= window.get_begin_time() and time >= window.get_end_time():
            return window

# Add the module function as a class method.
GpmlPiecewiseAggregation.get_time_window_containing_time = gpml_piecewise_aggregation_get_time_window_containing_time
# Delete the module reference to the function - we only keep the class method.
del gpml_piecewise_aggregation_get_time_window_containing_time


def gpml_irregular_sampling_get_value(gpml_irregular_sampling, time=0):
    """get_value([time=0]) -> PropertyValue or None
    Extracts the value at the reconstruction *time*.
    
    :param time: the time to extract value (defaults to present day)
    :type time: float or :class:`GeoTimeInstant`
    :rtype: :class:`PropertyValue` or None
    :raises: InterpolationError if *time* is :meth:`distant past<GeoTimeInstant.is_distant_past>` or \
    :meth:`distant future<GeoTimeInstant.is_distant_future>`

    Returns ``None`` if *time* is outside the time range of the :meth:`time samples<get_time_samples>`.
    
    Note that the extracted property value is interpolated (at reconstruction *time*) if property
    value can be interpolated (currently only :class:`GpmlFiniteRotation` and :class:`XsDouble`),
    otherwise ``None`` is returned. The function :meth:`FiniteRotation.interpolate` is used
    internally when the extracted property value type is :class:`GpmlFiniteRotation`.
    
    **NOTE:** modifying the returned :class:`property value<PropertyValue>` does **not** modify the sequence
    of :meth:`time samples<get_time_samples>` (because the returned :class:`property value<PropertyValue>`
    is a new interpolated time sample that does not exist in the :meth:`sequence<get_time_samples>`).
    Instead :meth:`set_value` can be used to modify the sequence for a specific time value.
    
    This method overrides :meth:`PropertyValue.get_value`.
    """
    
    # Get the time samples bounding the time (and filter out the disabled time samples).
    adjacent_time_samples = gpml_irregular_sampling.get_time_samples_bounding_time(time)
    
    # Return early if all time samples are disabled or time is outside time samples range.
    if not adjacent_time_samples:
        return
    
    # The first time sample is further in the past (less recent).
    # Although it doesn't really matter for our interpolation.
    begin_time_sample, end_time_sample = adjacent_time_samples
    
    # Use private module function defined in another module code section (file).
    return _interpolate_property_value(
            begin_time_sample.get_value(),
            end_time_sample.get_value(),
            begin_time_sample.get_time(),
            end_time_sample.get_time(),
            time)

# Add the module function as a class method.
GpmlIrregularSampling.get_value = gpml_irregular_sampling_get_value
# Delete the module reference to the function - we only keep the class method.
del gpml_irregular_sampling_get_value


def gpml_irregular_sampling_set_value(gpml_irregular_sampling, property_value, time, description=None, is_enabled=True):
    """set_value(property_value, time, [description], [is_enabled=True]) -> GpmlTimeSample
    Sets the value at the reconstruction *time*.
    
    :param property_value: the property value to set
    :type property_value: :class:`PropertyValue`
    :param time: the time to set the value
    :type time: float or :class:`GeoTimeInstant`
    :param description: description of the time sample
    :type description: string or None
    :param is_enabled: whether time sample is enabled
    :type is_enabled: bool or None
    :returns: the time sample that is modified, or inserted into the time sequence
    :rtype: :class:`GpmlTimeSample`
    :raises: ValueError if *time* is :meth:`distant past<GeoTimeInstant.is_distant_past>` or \
    :meth:`distant future<GeoTimeInstant.is_distant_future>`

    If an existing :meth:`time sample<get_time_samples>` matches *time* then it will be modified,
    otherwise a new :class:`time sample<GpmlTimeSample>` will be inserted into the sequence
    such that the :meth:`time samples<get_time_samples>` remain ordered by time.
    
    This method assumes the precondition that the :meth:`time samples<get_time_samples>` are
    ordered by time from most recent to least recent.
    
    Note that *time* can be outside the time range of existing :meth:`time samples<get_time_samples>`.
    """
    
    # Use GeoTimeInstant to compare (within epsilon) floating point time values.
    time = GeoTimeInstant(time)
    
    # Time must not be distant past/future.
    if not time.is_real():
        raise ValueError("Time value cannot be distant-past (float('inf')) or distant-future (float('-inf')).")
    
    time_samples = gpml_irregular_sampling.get_time_samples()
    
    # Insert a new time sample or modify existing time sample
    # (based on whether time matches an existing time sample).
    # This assumes time samples are ordered by time (most recent to least recent).
    for time_sample_index, time_sample in enumerate(time_samples):
        time_sample_time = time_sample.get_time()
        if time < time_sample_time:
            # Insert new time sample.
            new_time_sample = GpmlTimeSample(property_value, time, description, is_enabled)
            time_samples.insert(time_sample_index, new_time_sample)
            return new_time_sample
        elif time == time_sample_time:
            # Times match so modify existing time sample.
            time_sample.set_value(property_value)
            time_sample.set_time(time)
            time_sample.set_description(description)
            time_sample.set_enabled(is_enabled)
            return time_sample
    
    # Time is further in the past than any existing time samples (so just append to end of sequence).
    new_time_sample = GpmlTimeSample(property_value, time, description, is_enabled)
    time_samples.append(new_time_sample)
    
    return new_time_sample

# Add the module function as a class method.
GpmlIrregularSampling.set_value = gpml_irregular_sampling_set_value
# Delete the module reference to the function - we only keep the class method.
del gpml_irregular_sampling_set_value


def gpml_irregular_sampling_get_enabled_time_samples(gpml_irregular_sampling):
    """get_enabled_time_samples() -> list
    Filter out the disabled :class:`time samples<GpmlTimeSample>` and return a list of enabled time samples.
    
    :rtype: list
    :return: the list of enabled :class:`time samples<GpmlTimeSample>` (if any)
    
    Returns an empty list if all time samples are disabled.
    
    This method essentially does the following:
    ::
    
      return filter(lambda ts: ts.is_enabled(), get_time_samples())
    """

    # Filter out the disabled time samples.
    return filter(lambda ts: ts.is_enabled(), gpml_irregular_sampling.get_time_samples())

# Add the module function as a class method.
GpmlIrregularSampling.get_enabled_time_samples = gpml_irregular_sampling_get_enabled_time_samples
# Delete the module reference to the function - we only keep the class method.
del gpml_irregular_sampling_get_enabled_time_samples


def gpml_irregular_sampling_get_time_samples_bounding_time(gpml_irregular_sampling, time, include_disabled_samples=False):
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
GpmlIrregularSampling.get_time_samples_bounding_time = gpml_irregular_sampling_get_time_samples_bounding_time
# Delete the module reference to the function - we only keep the class method.
del gpml_irregular_sampling_get_time_samples_bounding_time