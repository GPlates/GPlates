def reconstructed_feature_geometry_get_present_day_geometry(rfg):
    """get_present_day_geometry() -> GeometryOnSphere
    Returns the present day geometry.
    
    :rtype: :class:`GeometryOnSphere`
    """
    
    property = rfg.get_property()
    if property:
        property_value = property.get_value()
        if property_value:
            return get_geometry_from_property_value(property_value)

# Add the module function as a class method.
ReconstructedFeatureGeometry.get_present_day_geometry = reconstructed_feature_geometry_get_present_day_geometry
# Delete the module reference to the function - we only keep the class method.
del reconstructed_feature_geometry_get_present_day_geometry
