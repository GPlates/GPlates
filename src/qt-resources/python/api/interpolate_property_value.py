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
    
    # Currently on GpmlFiniteRotation and XsDouble can be interpolated...
    
    def visit_gpml_finite_rotation(self, gpml_finite_rotation1):
        # No need to test 'time1==time2' since 'interpolate_finite_rotations()' does that for us.
        
        # Second property value should also be the same type.
        self.interpolated_property_value = GpmlFiniteRotation(
            interpolate_finite_rotations(
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


def interpolate_total_reconstruction_pole(total_reconstruction_pole, time):
    """interpolate_total_reconstruction_pole(total_reconstruction_pole, time) -> FiniteRotation or None
    Interpolates the *total reconstruction pole* property value at the specified time.
    
    :param total_reconstruction_pole: the *total reconstruction pole* time-dependent property value
    :type total_reconstruction_pole: :class:`GpmlIrregularSampling`
    :param time: the time at which to interpolate
    :type time: float or :class:`GeoTimeInstant`
    :rtype: :class:`FiniteRotation` or None
    :return: the interpolated rotation or None
    :raises: InterpolationError if *time* is :meth:`distant past<GeoTimeInstant.is_distant_past>` or \
    :meth:`distant future<GeoTimeInstant.is_distant_future>`
    
    Returns ``None`` if *time* is not spanned by any time samples in the *total reconstruction pole*, or
    if all time samples are disabled.
    
    See :func:`interpolate_total_reconstruction_sequence` and :func:`interpolate_finite_rotations` for a similar functions.
    """
    
    # Get the time samples bounding the time (and filter out the disabled time samples).
    adjacent_time_samples = total_reconstruction_pole.get_time_samples_bounding_time(time)
    
    # Return early if all time samples are disabled or time is outside time samples range.
    if not adjacent_time_samples:
        return
    
    # The first time sample is further in the past (less recent).
    # Although it doesn't really matter for our interpolation.
    begin_time_sample, end_time_sample = adjacent_time_samples
    
    return interpolate_finite_rotations(
            begin_time_sample.get_value().get_finite_rotation(),
            end_time_sample.get_value().get_finite_rotation(),
            begin_time_sample.get_time(),
            end_time_sample.get_time(),
            time)


def interpolate_total_reconstruction_sequence(total_reconstruction_sequence_feature, time):
    """interpolate_total_reconstruction_sequence(total_reconstruction_sequence_feature, time) -> (int, int, FiniteRotation) or None
    Interpolates the *time-dependent* total reconstruction pole in a *total reconstruction sequence* feature at the specified time.
    
    Features of type *total reconstruction sequence* are usually read from a GPML rotation file or a PLATES4 rotation ('.rot') file.
    
    :param total_reconstruction_sequence_feature: the rotation feature with :class:`feature type<FeatureType>` 'gpml:TotalReconstructionSequence'
    :type total_reconstruction_sequence_feature: :class:`Feature`
    :param time: the time at which to interpolate
    :type time: float or :class:`GeoTimeInstant`
    :rtype: tuple(int, int, :class:`FiniteRotation`) or None
    :return: A tuple containing (fixed plate id, moving plate id, interpolated rotation) or None
    :raises: InterpolationError if *time* is :meth:`distant past<GeoTimeInstant.is_distant_past>` or \
    :meth:`distant future<GeoTimeInstant.is_distant_future>`
    
    Returns ``None`` if the feature does not contain a 'gpml:fixedReferenceFrame' plate id,
    a 'gpml:movingReferenceFrame' plate id and a 'gpml:totalReconstructionPole' :class:`GpmlIrregularSampling` with
    time samples containing :class:`GpmlFiniteRotation` instances (or *time* is not spanned by any time samples, or
    all time samples are disabled).
    
    A feature with :class:`feature type<FeatureType>` 'gpml:TotalReconstructionSequence' should have these properties
    if it conforms to the GPlates Geological Information Model (GPGIM).
    
    See :func:`interpolate_total_reconstruction_pole` and :func:`interpolate_finite_rotations` for a similar functions.
    """

    total_reconstruction_pole = get_total_reconstruction_pole(total_reconstruction_sequence_feature)
    if total_reconstruction_pole is None:
        return
    
    (fixed_plate_id, moving_plate_id, total_reconstruction_pole_rotation) = total_reconstruction_pole
    
    # Interpolate the 'GpmlIrregularSampling' of 'GpmlFiniteRotations'.
    interpolated_rotation = interpolate_total_reconstruction_pole(total_reconstruction_pole_rotation, time)
    
    if interpolated_rotation is None:
        return
    
    return (fixed_plate_id, moving_plate_id, interpolated_rotation)
