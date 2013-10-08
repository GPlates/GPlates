# This function is private in this 'pygplates' module (function name prefixed with a single underscore).
# It's used by other sections of the 'pygplates' module, but not meant for public use.
# It's only in this code section because it does interpolation.
def _interpolate_property_value(property_value1, property_value2, time1, time2, target_time):
    class InterpolateVisitor(PropertyValueVisitor):
        def __init__(self, property_value1, property_value2, time1, time2, target_time):
            super(InterpolateVisitor, self).__init__()
            self.property_value1 = property_value1
            self.property_value2 = property_value2
            self.time1 = time1
            self.time2 = time2
            self.target_time = target_time
            self.interpolated_property_value = None
        
        def interpolate(self):
            self.interpolated_property_value = None
            # Visit the first property value.
            self.property_value1.accept_visitor(self)
            return self.interpolated_property_value
        
        # Currently on GpmlFiniteRotation and XsDouble can be interpolated...
        
        def visit_gpml_finite_rotation(self, gpml_finite_rotation1):
            # Second property value should also be the same type.
            self.interpolated_property_value = interpolate_finite_rotations(
                gpml_finite_rotation1.get_finite_rotation(), self.property_value2.get_finite_rotation(),
                self.time1, self.time2, self.target_time)
        
        def visit_xs_double(self, xs_double1):
            # Second property value should also be the same type.
            self.interpolated_property_value = interpolate_finite_rotations(
                xs_double1.get_double(), self.property_value2.get_double(),
                self.time1, self.time2, self.target_time)
    
    visitor = InterpolateVisitor(property_value1, property_value2, time1, time2, target_time)
    return visitor.interpolate()


def interpolate_total_reconstruction_pole(total_reconstruction_pole, time):
    """interpolate_total_reconstruction_pole(total_reconstruction_pole, time) -> FiniteRotation or None
    Interpolates the *total reconstruction pole* property value at the specified time.
    
    :param total_reconstruction_pole: the *total reconstruction pole* time-dependent property value
    :type total_reconstruction_pole: :class:`GpmlIrregularSampling`
    :param time: the time at which to interpolate
    :type time: float
    :rtype: :class:`FiniteRotation` or None
    :return: the interpolated rotation or None
    
    Returns ``None`` if *time* is not spanned by any time samples in the *total reconstruction pole*, or
    if all time samples are disabled.
    
    See :func:`interpolate_total_reconstruction_sequence` and :func:`interpolate_finite_rotations` for a similar functions.
    """
    
    # Get the time samples bounding the time (and filter out the disabled time samples).
    adjacent_time_samples = get_time_samples_bounding_time(total_reconstruction_pole, time)
    
    # Return early if all time samples are disabled or time is outside time samples range.
    if not adjacent_time_samples:
        return
    
    # The first time sample is further in the past (less recent).
    # Although it doesn't really matter for our interpolation.
    begin_time_sample, end_time_sample = adjacent_time_samples
    
    return interpolate_finite_rotations(
            begin_time_sample.get_value().get_finite_rotation(),
            end_time_sample.get_value().get_finite_rotation(),
            begin_time_sample.get_time().get_value(),
            end_time_sample.get_time().get_value(),
            time)


def interpolate_total_reconstruction_sequence(total_reconstruction_sequence_feature, time):
    """interpolate_total_reconstruction_sequence(total_reconstruction_sequence_feature, time) -> (int, int, FiniteRotation) or None
    Interpolates the *time-dependent* total reconstruction pole in a *total reconstruction sequence* feature at the specified time.
    
    Features of type *total reconstruction sequence* are usually read from a GPML rotation file or a PLATES4 rotation ('.rot') file.
    
    :param total_reconstruction_sequence_feature: the rotation feature with :class:`feature type<FeatureType>` 'gpml:TotalReconstructionSequence'
    :type total_reconstruction_sequence_feature: :class:`Feature`
    :param time: the time at which to interpolate
    :type time: float
    :rtype: tuple(int, int, :class:`FiniteRotation`) or None
    :return: A tuple containing (fixed plate id, moving plate id, interpolated rotation) or None
    
    Returns ``None`` if the feature does not contain a 'gpml:fixedReferenceFrame' plate id,
    a 'gpml:movingReferenceFrame' plate id and a 'gpml:totalReconstructionPole' :class:`GpmlIrregularSampling` with
    time samples containing :class:`GpmlFiniteRotation` instances (or *time* is not spanned by any time samples, or
    all time samples are disabled).
    
    A feature with :class:`feature type<FeatureType>` 'gpml:TotalReconstructionSequence' should have these properties
    if it conforms to the GPlates Geological Information Model (GPGIM).
    
    See :func:`interpolate_total_reconstruction_pole` and :func:`interpolate_finite_rotations` for a similar functions.
    """

    fixed_plate_id = None
    moving_plate_id = None
    total_reconstruction_pole = None
    
    fixed_reference_frame_property_name = PropertyName.create_gpml('fixedReferenceFrame')
    moving_reference_frame_property_name = PropertyName.create_gpml('movingReferenceFrame')
    total_reconstruction_pole_property_name = PropertyName.create_gpml('totalReconstructionPole')
    
    # Find the fixed/moving plate ids and the time sample sequence of total reconstruction poles.
    for property in total_reconstruction_sequence_feature:
        try:
            if property.get_name() == fixed_reference_frame_property_name:
                fixed_plate_id = property.get_value().get_plate_id()
            elif property.get_name() == moving_reference_frame_property_name:
                moving_plate_id = property.get_value().get_plate_id()
            elif property.get_name() == total_reconstruction_pole_property_name:
                total_reconstruction_pole = property.get_value()
        except AttributeError:
            # The property value type did not match the property name.
            # This indicates the data does not conform to the GPlates Geological Information Model (GPGIM).
            return

    if fixed_plate_id is None or moving_plate_id is None or total_reconstruction_pole is None:
        return
    
    # Interpolate the 'GpmlIrregularSampling' of 'GpmlFiniteRotations'.
    interpolated_rotation = interpolate_total_reconstruction_pole(total_reconstruction_pole, time)
    
    if interpolated_rotation is None:
        return
    
    return (fixed_plate_id, moving_plate_id, interpolated_rotation)
