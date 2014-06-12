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
