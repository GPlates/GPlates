def get_enabled_time_samples(gpml_irregular_sampling):
    """get_enabled_time_samples(gpml_irregular_sampling) -> list
    Filter out the disabled :class:`time samples<GpmlTimeSample>` and return a list of enabled time samples.
    
    :param gpml_irregular_sampling: the time-dependent property value containing the time samples
    :type gpml_irregular_sampling: :class:`GpmlIrregularSampling`
    :rtype: list
    :return: the list of enabled :class:`time samples<GpmlTimeSample>` (if any) or None
    
    Returns an empty list if all time samples are disabled.
    
    This function essentially does the following:
    ::
    
      return filter(lambda ts: ts.is_enabled(), gpml_irregular_sampling.get_time_samples())
    """

    # Filter out the disabled time samples.
    return filter(lambda ts: ts.is_enabled(), gpml_irregular_sampling.get_time_samples())


def get_time_samples_bounding_time(gpml_irregular_sampling, time, include_disabled_samples=False):
    """get_time_samples_bounding_time(gpml_irregular_sampling, time[, include_disabled_samples=False]) -> tuple
    Return the two adjacent :class:`time samples<GpmlTimeSample>`, of a :class:`GpmlIrregularSampling`, that surround *time*.
    
    :param gpml_irregular_sampling: the time-dependent property value containing the time samples
    :type gpml_irregular_sampling: :class:`GpmlIrregularSampling`
    :param time: the time
    :type time: float
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
        time_samples = get_enabled_time_samples(gpml_irregular_sampling)
    
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


def get_time_window_containing_time(gpml_piecewise_aggregation, time):
    """get_time_window_containing_time(gpml_piecewise_aggregation, time) -> GpmlTimeWindow or None
    Return the :class:`time window<GpmlTimeWindow>`, of a :class:`GpmlPiecewiseAggregation`, that contains *time*.
    
    :param gpml_piecewise_aggregation: the time-dependent property value containing the time windows
    :type gpml_piecewise_aggregation: :class:`GpmlPiecewiseAggregation`
    :param time: the time
    :type time: float
    :rtype: :class:`GpmlTimeWindow` or None

    Returns ``None`` if *time* is outside the time ranges of all time windows.
    """
    
    # Find the time window containing the time.
    for window in gpml_piecewise_aggregation.get_time_windows():
        # Time period includes begin time but not end time.
        if time <= window.get_begin_time() and time >= window.get_end_time():
            return window


def get_geometry_from_property_value(property_value, geometry_on_sphere_type=GeometryOnSphere):
    """get_geometry_from_property_value(property_value[, geometry_on_sphere_type=GeometryOnSphere]) -> GeometryOnSphere or None
    Extracts the :class:`geometry<GeometryOnSphere>` from *property_value* if it contains a geometry of type *geometry_on_sphere_type*.
    
    :param property_value: the property value to extract the geometry from
    :type property_value: :class:`PropertyValue`
    :param geometry_on_sphere_type: the geometry *type* to extract
    :type geometry_on_sphere_type: either :class:`GeometryOnSphere` or one of the four geometry types inheriting :class:`GeometryOnSphere`
    :rtype: :class:`GeometryOnSphere` or None
    
    This function only searches for a geometry in the following standard geometry property value types:
    
    * :class:`GmlPoint`
    * :class:`GmlMultiPoint`
    * :class:`GmlPolygon`
    * :class:`GmlLineString`
    * :class:`GmlOrientableCurve` (a possible wrapper around :class:`GmlLineString`)
    * :class:`GpmlConstantValue` (a possible wrapper around any of the above)
    
    If the default for *geometry_on_sphere_type* (:class:`GeometryOnSphere`) is used then any of the four geometry types
    inheriting :class:`GeometryOnSphere` will be searched, otherwise only the specified geometry type will be searched.
    
    If *property_value* does not contain a geometry, or the geometry does not match *geometry_on_sphere_type*, then
    None is returned.
    
    Time-dependent geometry properties are *not* yet supported, so the only time-dependent property value wrapper currently
    supported by this function is :class:`GpmlConstantValue`.
    
    For example:
    ::
    
        gml_line_string = pygplates.GpmlConstantValue(pygplates.GmlLineString(pygplates.PolylineOnSphere(...)))
        ...
        polyline = pygplates.get_geometry_from_property_value(gml_line_string, pygplates.PolylineOnSphere)
    """
    
    class GeometryPropertyVisitor(PropertyValueVisitor):
        def __init__(self, geometry_on_sphere_type):
            super(GeometryPropertyVisitor, self).__init__()
            self.geometry_on_sphere_type = geometry_on_sphere_type
            self.geometry_on_sphere = None
        
        # NOTE: We don't yet support time-dependent geometries, but they can still be wrapped in
        # constant-value wrappers. Once time-dependent geometries are supported we should also
        # visit GpmlPiecewiseAggregation and GpmlIrregularSampling (requires geometry interpolation).
        def visit_gpml_constant_value(self, gpml_constant_value):
            gpml_constant_value.get_value().accept_visitor(self)
        
        def visit_gml_orientable_curve(self, gml_orientable_curve):
            # GmlLineString can be wrapped in a GmlOrientableCurve.
            # But currently we don't use the orientation attribute of GmlOrientableCurve (ie, cannot be reversed).
            gml_orientable_curve.get_base_curve().accept_visitor(self)
        
        def visit_gml_point(self, gml_point):
            point_on_sphere = gml_point.get_point()
            if isinstance(point_on_sphere, self.geometry_on_sphere_type):
                self.geometry_on_sphere = point_on_sphere
        
        def visit_gml_multi_point(self, gml_multi_point):
            multi_point_on_sphere = gml_multi_point.get_multi_point()
            if isinstance(multi_point_on_sphere, self.geometry_on_sphere_type):
                self.geometry_on_sphere = multi_point_on_sphere
        
        def visit_gml_line_string(self, gml_line_string):
            polyline_on_sphere = gml_line_string.get_polyline()
            if isinstance(polyline_on_sphere, self.geometry_on_sphere_type):
                self.geometry_on_sphere = polyline_on_sphere
        
        def visit_gml_polygon(self, gml_polygon):
            polygon_on_sphere = gml_polygon.get_polygon()
            if isinstance(polygon_on_sphere, self.geometry_on_sphere_type):
                self.geometry_on_sphere = polygon_on_sphere
    
    # Attempt to extract the geometry from the property value.
    visitor = GeometryPropertyVisitor(geometry_on_sphere_type)
    property_value.accept_visitor(visitor)
    
    # Returns None if not a *geometry* property value.
    return visitor.geometry_on_sphere


def get_feature_properties_by_name(feature, property_name, time=0):
    """get_feature_properties_by_name(feature, property_name[, time=0]) -> list
    Return the :class:`properties<Property>` of *feature* with property name *property_name*
    and also return the property values extracted at the reconstruction *time*.
    
    :param feature: the feature to query the properties of
    :type feature: :class:`Feature`
    :param property_name: the property name to match
    :type property_name: :class:`PropertyName`
    :param time: the time to extract the property value
    :type time: float
    :rtype: list of tuples of (:class:`Property`, :class:`PropertyValue`)
    :return: the list of matching properties and their extracted property value at *time*
    
    This function uses :meth:`Property.get_value` to extract property values at *time*.
    
    This function essentially does the following:
    ::
    
        get_feature_properties_by_name(feature, property_name, time=0):
            properties = []
            for property in feature:
                if property.get_name() == property_name:
                    property_value = property.get_value(time)
                    if property_value:
                        properties.append((property, property_value))
            return properties
    """
    
    properties = []
    
    for property in feature:
        if property.get_name() == property_name:
            property_value = property.get_value(time)
            if property_value:
                properties.append((property, property_value))
    
    return properties


def get_feature_geometry_properties(feature, geometry_on_sphere_type=GeometryOnSphere):
    """get_feature_geometry_properties(feature[, geometry_on_sphere_type=GeometryOnSphere]) -> list
    Return the geometry :class:`properties<Property>` of *feature* with geometry types matching
    *geometry_on_sphere_type*, and also return the geometries extracted.
    
    :param feature: the feature to query the geometry properties of
    :type feature: :class:`Feature`
    :param geometry_on_sphere_type: the geometry *type* to extract
    :type geometry_on_sphere_type: :class:`GeometryOnSphere`
    :rtype: list of tuples of (:class:`Property`, :class:`GeometryOnSphere`)
    :return: the list of matching properties and their extracted geometries
    
    This function uses :func:`get_geometry_from_property_value` to extract geometries from property values.
    
    This function essentially does the following:
    ::
    
        get_feature_geometry_properties(feature, geometry_on_sphere_type=GeometryOnSphere):
            properties = []
            for property in feature:
                geometry_on_sphere = pygplates.get_geometry_from_property_value(property.get_value(), geometry_on_sphere_type)
                if geometry_on_sphere:
                    properties.append((property, geometry_on_sphere))
            return properties
    """
    
    properties = []
    
    for property in feature:
        property_value = property.get_value()
        if property_value:
            geometry_on_sphere = get_geometry_from_property_value(property_value, geometry_on_sphere_type)
            if geometry_on_sphere:
                properties.append((property, geometry_on_sphere))
    
    return properties


def get_feature_geometry_properties_by_name(feature, property_name, geometry_on_sphere_type=GeometryOnSphere):
    """get_feature_geometry_properties_by_name(feature, property_name[, geometry_on_sphere_type=GeometryOnSphere]) -> list
    Return the geometry :class:`properties<Property>` of *feature* with property name *property_name* and with
    geometry types matching *geometry_on_sphere_type*, and also return the geometries extracted.
    
    :param feature: the feature to query the geometry properties of
    :type feature: :class:`Feature`
    :param property_name: the property name to match
    :type property_name: :class:`PropertyName`
    :param geometry_on_sphere_type: the geometry *type* to extract
    :type geometry_on_sphere_type: :class:`GeometryOnSphere`
    :rtype: list of tuples of (:class:`Property`, :class:`GeometryOnSphere`)
    :return: the list of matching properties and their extracted geometries
    
    This function uses :func:`get_geometry_from_property_value` to extract geometries from property values.
    
    This function essentially does the following:
    ::
    
        get_feature_geometry_properties_by_name(feature, property_name, geometry_on_sphere_type=GeometryOnSphere):
            properties = []
            for property in feature:
                if property.get_name() == property_name:
                    geometry_on_sphere = pygplates.get_geometry_from_property_value(property.get_value(), geometry_on_sphere_type)
                    if geometry_on_sphere:
                        properties.append((property, geometry_on_sphere))
            return properties
    """
    
    properties = []
    
    for property in feature:
        if property.get_name() == property_name:
            property_value = property.get_value()
            if property_value:
                geometry_on_sphere = get_geometry_from_property_value(property_value, geometry_on_sphere_type)
                if geometry_on_sphere:
                    properties.append((property, geometry_on_sphere))
    
    return properties


def get_total_reconstruction_pole(total_reconstruction_sequence_feature):
    """get_total_reconstruction_pole(total_reconstruction_sequence_feature) -> (int, int, GpmlIrregularSampling) or None
    Returns the *time-dependent* total reconstruction pole in a *total reconstruction sequence* feature.
    
    Features of type *total reconstruction sequence* are usually read from a GPML rotation file or a PLATES4 rotation ('.rot') file.
    
    :param total_reconstruction_sequence_feature: the rotation feature with :class:`feature type<FeatureType>` 'gpml:TotalReconstructionSequence'
    :type total_reconstruction_sequence_feature: :class:`Feature`
    :rtype: tuple(int, int, :class:`GpmlIrregularSampling`) or None
    :return: A tuple containing (fixed plate id, moving plate id, time sequence of finite rotations) or None
    
    Returns ``None`` if the feature does not contain a 'gpml:fixedReferenceFrame' plate id,
    a 'gpml:movingReferenceFrame' plate id and a 'gpml:totalReconstructionPole' :class:`GpmlIrregularSampling` with
    time samples containing :class:`GpmlFiniteRotation` instances.
    
    A feature with :class:`feature type<FeatureType>` 'gpml:TotalReconstructionSequence' should have these properties
    if it conforms to the GPlates Geological Information Model (GPGIM).
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
                total_reconstruction_pole = property.get_time_dependent_container(GpmlIrregularSampling)
        except AttributeError:
            # The property value type did not match the property name.
            # This indicates the data does not conform to the GPlates Geological Information Model (GPGIM).
            return

    if fixed_plate_id is None or moving_plate_id is None or total_reconstruction_pole is None:
        return
    
    return (fixed_plate_id, moving_plate_id, total_reconstruction_pole)
