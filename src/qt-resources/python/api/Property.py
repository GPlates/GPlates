
"""
    Copyright (C) 2014 The University of Sydney, Australia
    
    This file is part of GPlates.
    
    GPlates is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License, version 2, as published by
    the Free Software Foundation.
    
    GPlates is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.
    
    You should have received a copy of the GNU General Public License along
    with this program; if not, write to Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
"""

def get_value(property, time=0):
    """get_value([time=0]) -> PropertyValue or None
    Extracts the value, of our possibly time-dependent property, at the reconstruction *time*.
    
    :param time: the time to extract value (defaults to present day)
    :type time: float or :class:`GeoTimeInstant`
    :rtype: :class:`PropertyValue` or None
    
    If this property has a time-dependent property value (:class:`GpmlConstantValue`,
    :class:`GpmlIrregularSampling` or :class:`GpmlPiecewiseAggregation`) then a nested property
    value is extracted at the reconstruction *time* and returned. Otherwise our property value
    instance is simply returned as is (since it's not a time-dependent property value).
    See :meth:`PropertyValue.get_value` for more details.
    
    Note that this method never returns a time-dependent property value (:class:`GpmlConstantValue`,
    :class:`GpmlIrregularSampling` or :class:`GpmlPiecewiseAggregation`).
    You can use :meth:`get_time_dependent_container` for that.
    """
    
    # Get the property value using a private method.
    property_value = property._get_value();
    
    # Look up the value at the specified time.
    return property_value.get_value(time)

# Add the module function as a class method.
Property.get_value = get_value
# Delete the module reference to the function - we only keep the class method.
del get_value


def get_time_dependent_container(property):
    """get_time_dependent_container() -> PropertyValue or None
    Returns the time-dependent property value container.
    
    :rtype: :class:`PropertyValue` or None
    
    Returns a time-dependent property value (:class:`GpmlConstantValue`, :class:`GpmlIrregularSampling` or
    :class:`GpmlPiecewiseAggregation`), or ``None`` if the property value is not actually time-dependent.
    Alternatively you can use :meth:`get_value` for extracting a contained property value at a reconstruction time.
    
    This method is useful if you want to access the time-dependent property value container directly.
    An example is inspecting or modifying the time samples in a :class:`GpmlIrregularSampling`.
    Otherwise :meth:`get_value` is generally more useful since it extracts a value from the container.
    """
    
    # Get the property value using a private method.
    property_value = property._get_value();
    
    # Return the property value if it is one of the time-dependent types.
    if isinstance(property_value, (GpmlConstantValue, GpmlIrregularSampling, GpmlPiecewiseAggregation)):
        return property_value


# Add the module function as a class method.
Property.get_time_dependent_container = get_time_dependent_container
# Delete the module reference to the function - we only keep the class method.
del get_time_dependent_container
