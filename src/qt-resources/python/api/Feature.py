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


# Private variable property names of common feature properties.
# We create them once to avoid constantly re-creating them each time a class method is called.
_gml_description_property_name = PropertyName.create_gml('description')
_gml_name_property_name = PropertyName.create_gml('name')
_gml_valid_time_property_name = PropertyName.create_gml('validTime')
_gpml_left_plate_property_name = PropertyName.create_gpml('leftPlate')
_gpml_right_plate_property_name = PropertyName.create_gpml('rightPlate')
_gpml_reconstruction_method_property_name = PropertyName.create_gpml('reconstructionMethod')
_gpml_reconstruction_method_enumeration_type = EnumerationType.create_gpml('ReconstructionMethodEnumeration')
_gpml_reconstruction_plate_id_property_name = PropertyName.create_gpml('reconstructionPlateId')
_gpml_conjugate_plate_id_property_name = PropertyName.create_gpml('conjugatePlateId')
_gpml_relative_plate_property_name = PropertyName.create_gpml('relativePlate')
_gpml_times_property_name = PropertyName.create_gpml('times')
_gpml_shapefile_attributes_property_name = PropertyName.create_gpml('shapefileAttributes')
_gpml_fixed_reference_frame_property_name = PropertyName.create_gpml('fixedReferenceFrame')
_gpml_moving_reference_frame_property_name = PropertyName.create_gpml('movingReferenceFrame')
_gpml_total_reconstruction_pole_property_name = PropertyName.create_gpml('totalReconstructionPole')


def get_description(feature, default=''):
    """get_description([default='']) -> string
    Return the description of this feature.
    
    :param default: the default description (defaults to an empty string)
    :type default: string or None
    :returns: the description (if exactly one 'gml:description' property found), otherwise *default* is returned
    :rtype: string, or type(*default*)
    
    This is a convenience method that wraps :meth:`get_value` for the common property 'gml:description'.
    
    Return the description as a string (defaults to an empty string if not exactly one found):
    ::
    
      description = feature.get_description()
    
    Set *default* to ``None`` to test that there is exactly one 'gml:description' property:
    ::
    
      description = feature.get_description(None)
      # Compare with None since an empty string evaluates to False.
      if description is not None:
        ...
    
    Test that there is exactly one 'gml:description' property and that it is not the empty string:
    ::
    
      description = feature.get_description()
      if description:
        ...
    """
    
    gml_description = feature.get_value(_gml_description_property_name)
    if not gml_description:
        return default
    
    try:
        return gml_description.get_string()
    except AttributeError:
        # The property value type did not match the property name.
        # This indicates the data does not conform to the GPlates Geological Information Model (GPGIM).
        return default

# Add the module function as a class method.
Feature.get_description = get_description
# Delete the module reference to the function - we only keep the class method.
del get_description


def set_description(feature, description, verify_information_model=VerifyInformationModel.yes):
    """set_description(description, [verify_information_model=VerifyInformationModel.yes]) -> Property
    Sets the description of this feature.
    
    :param description: the description
    :type description: string
    :param verify_information_model: whether to check the information model before setting (default) or not
    :type verify_information_model: *VerifyInformationModel.yes* or *VerifyInformationModel.no*
    :returns: the property containing the description
    :rtype: :class:`Property`
    :raises: InformationModelError if *verify_information_model* is *VerifyInformationModel.yes* and the feature :class:`type<FeatureType>` \
    does not support the 'gml:description' property.
    
    This is a convenience method that wraps :meth:`set` for the common property 'gml:description'.
    
    Set the description to a string:
    ::
    
      feature.set_description('description')
    """
    
    return feature.set(_gml_description_property_name, XsString(description), verify_information_model)

# Add the module function as a class method.
Feature.set_description = set_description
# Delete the module reference to the function - we only keep the class method.
del set_description


def get_name(feature, default='', property_return=PropertyReturn.exactly_one):
    """get_name([default=''], [property_return=PropertyReturn.exactly_one]) -> string or list
    Return the name (or names) of this feature.
    
    :param default: the default name (defaults to an empty string), or default names
    :type default: string or list or None
    :param property_return: whether to return exactly one name, the first name or all names
    :type property_return: *PropertyReturn.exactly_one*, *PropertyReturn.first* or *PropertyReturn.all*
    :rtype: string, or list of strings, or type(*default*)
    
    This is a convenience method that wraps :meth:`get_value` for the common property 'gml:name'.
    
    There can be more than one name for a feature but typically there will be only one.
    
    The following table maps *property_return* values to return values:
    
    ======================================= ==============
    PropertyReturn Value                     Description
    ======================================= ==============
    exactly_one                             Returns the name ``str`` if exactly one 'gml:name' property is found, \
    otherwise *default* is returned.
    first                                   Returns the name ``str`` of the first 'gml:name' property - \
    however note that a feature is an *unordered* collection of properties. Returns *default* \
    if there are no 'gml:name' properties.
    all                                     Returns a ``list`` of names (``str``) of 'gml:name' properties. \
    Returns *default* if there are no 'gml:name' properties. Note that any 'gml:name' property with an \
    empty name string *will* be added to the list.
    ======================================= ==============
    
    Return the name as a string (defaults to an empty string if not exactly one found):
    ::
    
      name = feature.get_name()
    
    Test that there is exactly one 'gml:name' property:
    ::
    
      name = feature.get_name(None)
      # Compare with None since an empty string evaluates to False.
      if name is not None:
        ...
    
    Test that there is exactly one 'gml:name' property and that it is not the empty string:
    ::
    
      name = feature.get_name()
      if name:
        ...
    
    Return the list of names as strings (defaults to an empty list if no names are found):
    ::
    
      names = feature.get_name([], pygplates.PropertyReturn.all)
    
    Test if there are any 'gml:name' properties:
    ::
    
      names = feature.get_names(None, pygplates.PropertyReturn.all)
      if names:
        ...
    
    Test if there are any 'gml:name' properties with a non-empty string:
    ::
    
      names = feature.get_names(None, pygplates.PropertyReturn.all)
      if names and any(names):
        ...
    """
    
    gml_name = feature.get_value(_gml_name_property_name, 0, property_return)
    if gml_name:
        try:
            if property_return == PropertyReturn.all:
                # 'gml_name' is a list of property values.
                return [name.get_string() for name in gml_name]
            else:
                # 'gml_name' is a single property value.
                return gml_name.get_string()
        except AttributeError:
            # The property value type did not match the property name.
            # This indicates the data does not conform to the GPlates Geological Information Model (GPGIM).
            pass
    
    # Return default.
    return default

# Add the module function as a class method.
Feature.get_name = get_name
# Delete the module reference to the function - we only keep the class method.
del get_name


def set_name(feature, name, verify_information_model=VerifyInformationModel.yes):
    """set_name(name, [verify_information_model=VerifyInformationModel.yes]) -> Property or list
    Set the name (or names) of this feature.
    
    :param name: the name or names
    :type name: string, or sequence of string
    :param verify_information_model: whether to check the information model before setting (default) or not
    :type verify_information_model: *VerifyInformationModel.yes* or *VerifyInformationModel.no*
    :returns: the property containing the name, or properties containing the names
    :rtype: :class:`Property`, or list of :class:`Property`
    :raises: InformationModelError if *verify_information_model* is *VerifyInformationModel.yes* and the feature :class:`type<FeatureType>` \
    does not support the 'gml:name' property.
    
    This is a convenience method that wraps :meth:`set` for the common property 'gml:name'.
    
    There can be more than one name for a feature but typically there will be only one.
    
    Set the name to a string:
    ::
    
      feature.set_name('name')
    
    Set the names to strings:
    ::
    
      feature.set_name(['name1', 'name2'])
    """
    
    # If 'name' is a sequence.
    if hasattr(name, '__iter__'):
        return feature.set(_gml_name_property_name, [XsString(n) for n in name], verify_information_model)
    
    return feature.set(_gml_name_property_name, XsString(name), verify_information_model)

# Add the module function as a class method.
Feature.set_name = set_name
# Delete the module reference to the function - we only keep the class method.
del set_name


def get_valid_time(feature, default=(float('inf'), float('-inf'))):
    """get_valid_time([default=(float('inf'), float('-inf'))]) -> begin_time, end_time
    Returns the valid time range of this feature.
    
    :param default: the default time range (defaults to all time)
    :type default: tuple (float,float) or None
    :returns: begin and end times (if exactly one 'gml:validTime' property found), otherwise *default* is returned
    :rtype: tuple (float,float), or type(*default*)
    
    This is a convenience method that wraps :meth:`get_value` for the common property 'gml:validTime'.
    
    Return the valid time range as a tuple of begin and end times (defaults to all time if not exactly one found):
    ::
    
      begin_time, end_time = feature.get_valid_time()
      
      # Valid time for an empty feature defaults to all time.
      empty_feature = pygplates.Feature()
      begin_time, end_time = empty_feature.get_valid_time()
      assert(begin_time == pygplates.GeoTimeInstant.create_distant_past())
      assert(end_time == pygplates.GeoTimeInstant.create_distant_future())
    
    Set *default* to ``None`` to test that there is exactly one 'gml:validTime' property:
    ::
    
      valid_time = feature.get_valid_time(None)
      if valid_time:
        begin_time, end_time = valid_time
    """
    
    gml_valid_time = feature.get_value(_gml_valid_time_property_name)
    if not gml_valid_time:
        return default
    
    try:
        return gml_valid_time.get_begin_time(), gml_valid_time.get_end_time()
    except AttributeError:
        # The property value type did not match the property name.
        # This indicates the data does not conform to the GPlates Geological Information Model (GPGIM).
        return default

# Add the module function as a class method.
Feature.get_valid_time = get_valid_time
# Delete the module reference to the function - we only keep the class method.
del get_valid_time


def set_valid_time(feature, begin_time, end_time, verify_information_model=VerifyInformationModel.yes):
    """set_valid_time(begin_time, end_time, [verify_information_model=VerifyInformationModel.yes]) -> Property
    Sets the valid time range of this feature.
    
    :param begin_time: the begin time (time of appearance)
    :type begin_time: float or :class:`GeoTimeInstant`
    :param end_time: the end time (time of disappearance)
    :type end_time: float or :class:`GeoTimeInstant`
    :param verify_information_model: whether to check the information model before setting (default) or not
    :type verify_information_model: *VerifyInformationModel.yes* or *VerifyInformationModel.no*
    :returns: the property containing the valid time range
    :rtype: :class:`Property`
    :raises: GmlTimePeriodBeginTimeLaterThanEndTimeError if begin time is later than end time
    :raises: InformationModelError if *verify_information_model* is *VerifyInformationModel.yes* and the feature :class:`type<FeatureType>` \
    does not support the 'gml:validTime' property.
    
    This is a convenience method that wraps :meth:`set` for the common property 'gml:validTime'.
    
    Set the valid time range to include all geological time up until present day:
    ::
    
      feature.set_valid_time(pygplates.GeoTimeInstant.create_distant_past(), 0)
    """
    
    return feature.set(_gml_valid_time_property_name, GmlTimePeriod(begin_time, end_time), verify_information_model)

# Add the module function as a class method.
Feature.set_valid_time = set_valid_time
# Delete the module reference to the function - we only keep the class method.
del set_valid_time


def get_left_plate(feature, default=0):
    """get_left_plate([default=0]) -> int
    Returns the left plate ID of this feature.
    
    :param default: the default left plate id (defaults zero)
    :type default: int or None
    :returns: the left plate id (if exactly one 'gpml:leftPlate' property found), otherwise *default* is returned
    :rtype: int, or type(*default*)
    
    This is a convenience method that wraps :meth:`get_value` for the common property 'gpml:leftPlate'.
    
    Return the left plate ID as an integer (defaults to zero if not exactly one found):
    ::
    
      left_plate = feature.get_left_plate()
    
    Set *default* to ``None`` to test that there is exactly one 'gpml:leftPlate' property:
    ::
    
      left_plate = feature.get_left_plate(None)
      # Compare with None since a plate id of zero evaluates to False.
      if left_plate is not None:
        ...
    """
    
    gpml_left_plate = feature.get_value(_gpml_left_plate_property_name)
    if not gpml_left_plate:
        return default
    
    try:
        return gpml_left_plate.get_plate_id()
    except AttributeError:
        # The property value type did not match the property name.
        # This indicates the data does not conform to the GPlates Geological Information Model (GPGIM).
        return default

# Add the module function as a class method.
Feature.get_left_plate = get_left_plate
# Delete the module reference to the function - we only keep the class method.
del get_left_plate


def set_left_plate(feature, left_plate, verify_information_model=VerifyInformationModel.yes):
    """set_left_plate(left_plate, [verify_information_model=VerifyInformationModel.yes]) -> Property
    Sets the left plate ID of this feature.
    
    :param left_plate: the left plate id
    :type left_plate: int
    :param verify_information_model: whether to check the information model before setting (default) or not
    :type verify_information_model: *VerifyInformationModel.yes* or *VerifyInformationModel.no*
    :returns: the property containing the left plate id
    :rtype: :class:`Property`
    :raises: InformationModelError if *verify_information_model* is *VerifyInformationModel.yes* and the feature :class:`type<FeatureType>` \
    does not support the 'gpml:leftPlate' property.
    
    This is a convenience method that wraps :meth:`set` for the common property 'gpml:leftPlate'.
    
    Set the left plate ID to an integer:
    ::
    
      feature.set_left_plate(201)
    """
    
    return feature.set(_gpml_left_plate_property_name, GpmlPlateId(left_plate), verify_information_model)

# Add the module function as a class method.
Feature.set_left_plate = set_left_plate
# Delete the module reference to the function - we only keep the class method.
del set_left_plate


def get_right_plate(feature, default=0):
    """get_right_plate([default=0]) -> int
    Returns the right plate ID of this feature.
    
    :param default: the default right plate id (defaults zero)
    :type default: int or None
    :returns: the right plate id (if exactly one 'gpml:rightPlate' property found), otherwise *default* is returned
    :rtype: int, or type(*default*)
    
    This is a convenience method that wraps :meth:`get_value` for the common property 'gpml:rightPlate'.
    
    Return the right plate ID as an integer (defaults to zero if not exactly one found):
    ::
    
      right_plate = feature.get_right_plate()
    
    Set *default* to ``None`` to test that there is exactly one 'gpml:rightPlate' property:
    ::
    
      right_plate = feature.get_right_plate(None)
      # Compare with None since a plate id of zero evaluates to False.
      if right_plate is not None:
        ...
    """
    
    gpml_right_plate = feature.get_value(_gpml_right_plate_property_name)
    if not gpml_right_plate:
        return default
    
    try:
        return gpml_right_plate.get_plate_id()
    except AttributeError:
        # The property value type did not match the property name.
        # This indicates the data does not conform to the GPlates Geological Information Model (GPGIM).
        return default

# Add the module function as a class method.
Feature.get_right_plate = get_right_plate
# Delete the module reference to the function - we only keep the class method.
del get_right_plate


def set_right_plate(feature, right_plate, verify_information_model=VerifyInformationModel.yes):
    """set_right_plate(right_plate, [verify_information_model=VerifyInformationModel.yes]) -> Property
    Sets the right plate ID of this feature.
    
    :param right_plate: the right plate id
    :type right_plate: int
    :param verify_information_model: whether to check the information model before setting (default) or not
    :type verify_information_model: *VerifyInformationModel.yes* or *VerifyInformationModel.no*
    :returns: the property containing the right plate id
    :rtype: :class:`Property`
    :raises: InformationModelError if *verify_information_model* is *VerifyInformationModel.yes* and the feature :class:`type<FeatureType>` \
    does not support the 'gpml:rightPlate' property.
    
    This is a convenience method that wraps :meth:`set` for the common property 'gpml:rightPlate'.
    
    Set the right plate ID to an integer:
    ::
    
      feature.set_right_plate(701)
    """
    
    return feature.set(_gpml_right_plate_property_name, GpmlPlateId(right_plate), verify_information_model)

# Add the module function as a class method.
Feature.set_right_plate = set_right_plate
# Delete the module reference to the function - we only keep the class method.
del set_right_plate


def get_reconstruction_method(feature, default='ByPlateId'):
    """get_reconstruction_method([default='ByPlateId']) -> string
    Returns the reconstruction method of this feature.
    
    :param default: the default reconstruction method (defaults to 'ByPlateId')
    :type default: string or None
    :returns: the reconstruction method ('ByPlateId', 'HalfStageRotation' or 'HalfStageRotationVersion2') \
    if exactly one 'gpml:reconstructionMethod' property found containing an :class:`enumeration type<EnumerationType>` \
    of 'gpml:ReconstructionMethodEnumeration', otherwise *default* is returned
    :rtype: string, or type(*default*)
    
    This is a convenience method that wraps :meth:`get_value` for the common property 'gpml:reconstructionMethod'.
    
    Return the reconstruction method as a string (defaults to 'ByPlateId' if not exactly one found):
    ::
    
      reconstruction_method = feature.get_reconstruction_method()
    
    Set *default* to ``None`` to test that there is exactly one 'gpml:reconstructionMethod' property:
    ::
    
      reconstruction_method = feature.get_reconstruction_method(None)
      if reconstruction_method:
        ...
    """
    
    gpml_reconstruction_method = feature.get_value(_gpml_reconstruction_method_property_name)
    if gpml_reconstruction_method:
        try:
            # We're expecting a certain enumeration type - if we don't get it then we return 'default'.
            if gpml_reconstruction_method.get_type() == _gpml_reconstruction_method_enumeration_type:
                return gpml_reconstruction_method.get_content()
        except AttributeError:
            # The property value type did not match the property name.
            # This indicates the data does not conform to the GPlates Geological Information Model (GPGIM).
            pass
    
    # Return default.
    return default

# Add the module function as a class method.
Feature.get_reconstruction_method = get_reconstruction_method
# Delete the module reference to the function - we only keep the class method.
del get_reconstruction_method


def set_reconstruction_method(feature, reconstruction_method, verify_information_model=VerifyInformationModel.yes):
    """set_reconstruction_method(reconstruction_method, [verify_information_model=VerifyInformationModel.yes]) -> Property
    Sets the reconstruction method of this feature.
    
    :param reconstruction_method: the reconstruction method ('ByPlateId', 'HalfStageRotation' or 'HalfStageRotationVersion2')
    :type reconstruction_method: string
    :param verify_information_model: whether to check the information model before setting (default) or not
    :type verify_information_model: *VerifyInformationModel.yes* or *VerifyInformationModel.no*
    :returns: the property containing the reconstruction method
    :rtype: :class:`Property`
    :raises: InformationModelError if *verify_information_model* is *VerifyInformationModel.yes* and the feature :class:`type<FeatureType>` \
    does not support the 'gpml:reconstructionMethod' property, or *reconstruction_method* is not a recognised reconstruction method.
    
    This is a convenience method that wraps :meth:`set` for the common property 'gpml:reconstructionMethod'.
    
    Set the reconstruction method such that reconstructions of the feature will be done using half-stage rotations:
    ::
    
      feature.set_reconstruction_method('HalfStageRotationVersion2')
    """
    
    return feature.set(
            _gpml_reconstruction_method_property_name,
            Enumeration(_gpml_reconstruction_method_enumeration_type, reconstruction_method, verify_information_model),
            verify_information_model)

# Add the module function as a class method.
Feature.set_reconstruction_method = set_reconstruction_method
# Delete the module reference to the function - we only keep the class method.
del set_reconstruction_method


def get_reconstruction_plate_id(feature, default=0):
    """get_reconstruction_plate_id([default=0]) -> int
    Returns the reconstruction plate ID of this feature.
    
    :param default: the default reconstruction plate id (defaults zero)
    :type default: int or None
    :returns: the reconstruction plate id (if exactly one 'gpml:reconstructionPlateId' property found), otherwise *default* is returned
    :rtype: int, or type(*default*)
    
    This is a convenience method that wraps :meth:`get_value` for the common property 'gpml:reconstructionPlateId'.
    
    Return the reconstruction plate ID as an integer (defaults to zero if not exactly one found):
    ::
    
      reconstruction_plate_id = feature.get_reconstruction_plate_id()
    
    Set *default* to ``None`` to test that there is exactly one 'gpml:reconstructionPlateId' property:
    ::
    
      reconstruction_plate_id = feature.get_reconstruction_plate_id(None)
      # Compare with None since a plate id of zero evaluates to False.
      if reconstruction_plate_id is not None:
        ...
    """
    
    gpml_reconstruction_plate_id = feature.get_value(_gpml_reconstruction_plate_id_property_name)
    if not gpml_reconstruction_plate_id:
        return default
    
    try:
        return gpml_reconstruction_plate_id.get_plate_id()
    except AttributeError:
        # The property value type did not match the property name.
        # This indicates the data does not conform to the GPlates Geological Information Model (GPGIM).
        return default

# Add the module function as a class method.
Feature.get_reconstruction_plate_id = get_reconstruction_plate_id
# Delete the module reference to the function - we only keep the class method.
del get_reconstruction_plate_id


def set_reconstruction_plate_id(feature, reconstruction_plate_id, verify_information_model=VerifyInformationModel.yes):
    """set_reconstruction_plate_id(reconstruction_plate_id, [verify_information_model=VerifyInformationModel.yes]) -> Property
    Sets the reconstruction plate ID of this feature.
    
    :param reconstruction_plate_id: the reconstruction plate id
    :type reconstruction_plate_id: int
    :param verify_information_model: whether to check the information model before setting (default) or not
    :type verify_information_model: *VerifyInformationModel.yes* or *VerifyInformationModel.no*
    :returns: the property containing the reconstruction plate id
    :rtype: :class:`Property`
    :raises: InformationModelError if *verify_information_model* is *VerifyInformationModel.yes* and the feature :class:`type<FeatureType>` \
    does not support the 'gpml:reconstructionPlateId' property.
    
    This is a convenience method that wraps :meth:`set` for the common property 'gpml:reconstructionPlateId'.
    
    Set the reconstruction plate ID to an integer:
    ::
    
      feature.set_reconstruction_plate_id(701)
    """
    
    return feature.set(_gpml_reconstruction_plate_id_property_name, GpmlPlateId(reconstruction_plate_id), verify_information_model)

# Add the module function as a class method.
Feature.set_reconstruction_plate_id = set_reconstruction_plate_id
# Delete the module reference to the function - we only keep the class method.
del set_reconstruction_plate_id


def get_conjugate_plate_id(feature, default=0, property_return=PropertyReturn.exactly_one):
    """get_conjugate_plate_id([default=0], [property_return=PropertyReturn.exactly_one]) -> int or list
    Return the conjugate plate ID (or IDs) of this feature.
    
    :param default: the default plate ID (defaults to zero), or default plate IDs
    :type default: int or list or None
    :param property_return: whether to return exactly one ID, the first ID or all IDs
    :type property_return: *PropertyReturn.exactly_one*, *PropertyReturn.first* or *PropertyReturn.all*
    :rtype: int, or list of int, or type(*default*)
    
    This is a convenience method that wraps :meth:`get_value` for the common property 'gpml:conjugatePlateId'.
    
    There can be more than one conjugate plate ID for a feature but typically there will be only one.
    
    The following table maps *property_return* values to return values:
    
    ======================================= ==============
    PropertyReturn Value                     Description
    ======================================= ==============
    exactly_one                             Returns the integer plate ID if exactly one 'gpml:conjugatePlateId' property is found, \
    otherwise *default* is returned.
    first                                   Returns the integer plate ID of the first 'gpml:conjugatePlateId' property - \
    however note that a feature is an *unordered* collection of properties. Returns *default* \
    if there are no 'gpml:conjugatePlateId' properties.
    all                                     Returns a ``list`` of integer plate IDs of 'gpml:conjugatePlateId' properties. \
    Returns *default* if there are no 'gpml:conjugatePlateId' properties. Note that any 'gpml:conjugatePlateId' property with \
    a zero plate ID *will* be added to the list.
    ======================================= ==============
    
    Return the conjugate plate ID as an integer (defaults to zero if not exactly one found):
    ::
    
      conjugate_plate_id = feature.get_conjugate_plate_id()
    
    Test that there is exactly one 'gpml:conjugatePlateId' property:
    ::
    
      conjugate_plate_id = feature.get_conjugate_plate_id(None)
      # Compare with None since a plate id of zero evaluates to False.
      if conjugate_plate_id is not None:
        ...
    
    Test that there is exactly one 'gpml:conjugatePlateId' property and that it is not zero:
    ::
    
      conjugate_plate_id = feature.get_conjugate_plate_id()
      if conjugate_plate_id:
        ...
    
    Return the list of conjugate plate IDs as integers (defaults to an empty list if no conjugate plate IDs are found):
    ::
    
      conjugate_plate_ids = feature.get_conjugate_plate_id([], pygplates.PropertyReturn.all)
    
    Test if there are any 'gpml:conjugatePlateId' properties:
    ::
    
      conjugate_plate_ids = feature.get_conjugate_plate_id(None, pygplates.PropertyReturn.all)
      if conjugate_plate_ids:
        ...
    
    Test if there are any 'gpml:conjugatePlateId' properties with a non-zero plate ID:
    ::
    
      conjugate_plate_ids = feature.get_conjugate_plate_id(None, pygplates.PropertyReturn.all)
      if conjugate_plate_ids and any(conjugate_plate_ids):
        ...
    """
    
    gml_conjugate_plate_id = feature.get_value(_gpml_conjugate_plate_id_property_name, 0, property_return)
    if gml_conjugate_plate_id:
        try:
            if property_return == PropertyReturn.all:
                # 'gml_conjugate_plate_id' is a list of property values.
                return [id.get_plate_id() for id in gml_conjugate_plate_id]
            else:
                # 'gml_conjugate_plate_id' is a single property value.
                return gml_conjugate_plate_id.get_plate_id()
        except AttributeError:
            # The property value type did not match the property name.
            # This indicates the data does not conform to the GPlates Geological Information Model (GPGIM).
            pass
    
    # Return default.
    return default

# Add the module function as a class method.
Feature.get_conjugate_plate_id = get_conjugate_plate_id
# Delete the module reference to the function - we only keep the class method.
del get_conjugate_plate_id


def set_conjugate_plate_id(feature, conjugate_plate_id, verify_information_model=VerifyInformationModel.yes):
    """set_conjugate_plate_id(conjugate_plate_id, [verify_information_model=VerifyInformationModel.yes]) -> Property or list
    Set the conjugate plate ID (or IDs) of this feature.
    
    :param conjugate_plate_id: the conjugate plate ID or plate IDs
    :type conjugate_plate_id: int, or sequence of int
    :param verify_information_model: whether to check the information model before setting (default) or not
    :type verify_information_model: *VerifyInformationModel.yes* or *VerifyInformationModel.no*
    :returns: the property containing the conjugate plate ID, or properties containing the conjugate plate IDs
    :rtype: :class:`Property`, or list of :class:`Property`
    :raises: InformationModelError if *verify_information_model* is *VerifyInformationModel.yes* and the feature :class:`type<FeatureType>` \
    does not support the 'gpml:conjugatePlateId' property.
    
    This is a convenience method that wraps :meth:`set` for the common property 'gpml:conjugatePlateId'.
    
    There can be more than one conjugate plate ID for a feature but typically there will be only one.
    
    Set the conjugate plate ID to an integer:
    ::
    
      feature.set_conjugate_plate_id(201)
    
    Set the conjugate plate IDs to integers:
    ::
    
      feature.set_conjugate_plate_id([903, 904])
    """
    
    # If 'conjugate_plate_id' is a sequence.
    if hasattr(conjugate_plate_id, '__iter__'):
        return feature.set(_gpml_conjugate_plate_id_property_name, [GpmlPlateId(id) for id in conjugate_plate_id], verify_information_model)
    
    return feature.set(_gpml_conjugate_plate_id_property_name, GpmlPlateId(conjugate_plate_id), verify_information_model)

# Add the module function as a class method.
Feature.set_conjugate_plate_id = set_conjugate_plate_id
# Delete the module reference to the function - we only keep the class method.
del set_conjugate_plate_id


def get_relative_plate(feature, default=0):
    """get_relative_plate([default=0]) -> int
    Returns the relative plate ID of this feature.
    
    :param default: the default relative plate id (defaults zero)
    :type default: int or None
    :returns: the relative plate id (if exactly one 'gpml:relativePlate' property found), otherwise *default* is returned
    :rtype: int, or type(*default*)
    
    This is a convenience method that wraps :meth:`get_value` for the common property 'gpml:relativePlate'.
    
    Return the relative plate ID as an integer (defaults to zero if not exactly one found):
    ::
    
      relative_plate_id = feature.get_relative_plate()
    
    Set *default* to ``None`` to test that there is exactly one 'gpml:relativePlate' property:
    ::
    
      relative_plate_id = feature.get_relative_plate(None)
      # Compare with None since a plate id of zero evaluates to False.
      if relative_plate_id is not None:
        ...
    """
    
    gpml_relative_plate = feature.get_value(_gpml_relative_plate_property_name)
    if not gpml_relative_plate:
        return default
    
    try:
        return gpml_relative_plate.get_plate_id()
    except AttributeError:
        # The property value type did not match the property name.
        # This indicates the data does not conform to the GPlates Geological Information Model (GPGIM).
        return default

# Add the module function as a class method.
Feature.get_relative_plate = get_relative_plate
# Delete the module reference to the function - we only keep the class method.
del get_relative_plate


def set_relative_plate(feature, relative_plate, verify_information_model=VerifyInformationModel.yes):
    """set_relative_plate(relative_plate, [verify_information_model=VerifyInformationModel.yes]) -> Property
    Sets the relative plate ID of this feature.
    
    :param relative_plate: the relative plate id
    :type relative_plate: int
    :param verify_information_model: whether to check the information model before setting (default) or not
    :type verify_information_model: *VerifyInformationModel.yes* or *VerifyInformationModel.no*
    :returns: the property containing the relative plate id
    :rtype: :class:`Property`
    :raises: InformationModelError if *verify_information_model* is *VerifyInformationModel.yes* and the feature :class:`type<FeatureType>` \
    does not support the 'gpml:relativePlate' property.
    
    This is a convenience method that wraps :meth:`set` for the common property 'gpml:relativePlate'.
    
    Set the relative plate ID to an integer:
    ::
    
      feature.set_relative_plate(701)
    """
    
    return feature.set(_gpml_relative_plate_property_name, GpmlPlateId(relative_plate), verify_information_model)

# Add the module function as a class method.
Feature.set_relative_plate = set_relative_plate
# Delete the module reference to the function - we only keep the class method.
del set_relative_plate


def get_times(feature):
    """get_times() -> list or None
    Returns the list of times of this flowline or motion path feature.
    
    :returns: the list of times (if exactly one 'gpml:times' property found), otherwise None is returned
    :rtype: list of float, or None
    
    This is a convenience method that wraps :meth:`get_value` for the common property 'gpml:times' used in flowlines and motion paths.
    
    The list is from most recent (closest to present day) to least recent (furthest in the geological past).
    
    Return the list of times (returns ``None`` if not exactly one 'gpml:times' property found):
    ::
    
      times = feature.get_times()
      if times:
        for time in times:
          ...
    
    Note that the 'gpml:times' property actually contains a :class:`list<GpmlArray>` of :class:`time periods<GmlTimePeriod>`
    (not time instants). So this method converts the time periods to a list of time instants (by assuming the time periods
    do not overlap each other and do not have gaps between them).
    """
    
    gpml_times = feature.get_value(_gpml_times_property_name)
    if gpml_times:
        try:
            times = []
            
            # Iterate over the GpmlArray property value.
            for gml_time_period in gpml_times:
                # Add end time of each time period.
                times.append(gml_time_period.get_end_time())
            
            # Finish by adding begin time of last time period.
            times.append(gpml_times[-1].get_begin_time())
            
            return times
        except AttributeError:
            # The property value type did not match the property name.
            # This indicates the data does not conform to the GPlates Geological Information Model (GPGIM).
            pass

# Add the module function as a class method.
Feature.get_times = get_times
# Delete the module reference to the function - we only keep the class method.
del get_times


def set_times(feature, times, verify_information_model=VerifyInformationModel.yes):
    """set_times(times, [verify_information_model=VerifyInformationModel.yes]) -> Property
    Sets the list of times of this flowline or motion path feature.
    
    :param times: the list of times (if exactly one 'gpml:times' property found), otherwise None is returned
    :type times: sequence (eg, ``list`` or ``tuple``) of float or :class:`GeoTimeInstant`
    :param verify_information_model: whether to check the information model before setting (default) or not
    :type verify_information_model: *VerifyInformationModel.yes* or *VerifyInformationModel.no*
    :returns: the property containing the list of times
    :rtype: :class:`Property`
    :raises: InformationModelError if *verify_information_model* is *VerifyInformationModel.yes* and the feature :class:`type<FeatureType>` \
    does not support the 'gpml:times' property.
    :raises: ValueError if the time values in *times* are not in monotonically increasing order, or there are fewer than two time values.
    
    This is a convenience method that wraps :meth:`set` for the common property 'gpml:times' used in flowlines and motion paths.
    
    The list of times must progressively be from most recent (closest to present day) to least recent (furthest in the geological past)
    otherwise *ValueError* will be raised.
    
    Set the list of times:
    ::
    
      feature.set_times([0, 10, 20, 30, 40])
    
    Note that the 'gpml:times' property actually contains a :class:`list<GpmlArray>` of :class:`time periods<GmlTimePeriod>`
    (not time instants). So this method converts the time instants (in *times*) to adjoining time periods when creating the property.
    """
    
    if len(times) < 2:
        raise ValueError('Time sequence must contain at least two time values')
    
    gml_time_periods = []
    
    # Iterate over the times.
    prev_time = times[0]
    for time_index in range(1, len(times)):
        time = times[time_index]
        if time < prev_time:
            raise ValueError('Time sequence is not in monotonically increasing order')
        
        # Add the time period from current time to previous time.
        gml_time_periods.append(GmlTimePeriod(time, prev_time))
        
        prev_time = time
    
    return feature.set(_gpml_times_property_name, GpmlArray(gml_time_periods), verify_information_model)

# Add the module function as a class method.
Feature.set_times = set_times
# Delete the module reference to the function - we only keep the class method.
del set_times


def get_shapefile_attribute(feature, key, default_value=None):
    """get_shapefile_attribute(key, [default_value]) -> int or float or string or None
    Returns the value of a shapefile attribute associated with a key.
    
    :param key: the key of the shapefile attribute
    :type key: string
    :param default_value: the default value to return if *key* does not exist (if not specified then it defaults to None)
    :type default_value: int or float or string or None
    :returns: the value of the shapefile attribute associated with *key*, otherwise *default_value* if *key* does not exist
    :rtype: integer or float or string or type(*default_value*) or None
    
    Shapefile attributes are stored in a :class:`GpmlKeyValueDictionary` property named 'gpml:shapefileAttributes' and
    contain attributes imported from a Shapefile.
    
    This is a convenience method that wraps :meth:`get_value` for the common property 'gpml:shapefileAttributes' and
    accesses the attribute value associated with *key* within that property using :meth:`GpmlKeyValueDictionary.get`.
    
    Note that *default_value* is returned if either a 'gpml:shapefileAttributes' property does not exist in this feature, or
    one does exist but does not contain a shapefile attribute associated with *key*.
    
    To test if a key is present and retrieve its value:
    ::
    
      value = feature.get_shapefile_attribute('key')
      # Compare with None since an integer (or float) value of zero, or an empty string, evaluates to False.
      if value is not None:
      ...
    
    Return the integer value of the attribute associated with 'key' (default to zero if not present):
    ::
    
      integer_value = feature.get_shapefile_attribute('key', 0))
    """
    
    gpml_shapefile_attributes = feature.get_value(_gpml_shapefile_attributes_property_name)
    if not gpml_shapefile_attributes:
        return default_value
    
    try:
        return gpml_shapefile_attributes.get(key, default_value)
    except AttributeError:
        # The property value type did not match the property name.
        # This indicates the data does not conform to the GPlates Geological Information Model (GPGIM).
        return default_value

# Add the module function as a class method.
Feature.get_shapefile_attribute = get_shapefile_attribute
# Delete the module reference to the function - we only keep the class method.
del get_shapefile_attribute


def set_shapefile_attribute(feature, key, value, verify_information_model=VerifyInformationModel.yes):
    """set_shapefile_attribute(key, value, [verify_information_model=VerifyInformationModel.yes]) -> Property
    Sets the value of a shapefile attribute associated with a key.
    
    :param key: the key of the shapefile attribute
    :type key: string
    :param value: the value of the shapefile attribute
    :type value: integer, float or string
    :param verify_information_model: whether to check the information model before setting (default) or not
    :type verify_information_model: *VerifyInformationModel.yes* or *VerifyInformationModel.no*
    :returns: the property containing all the shapefile attributes
    :rtype: :class:`Property` containing a :class:`GpmlKeyValueDictionary` property value
    :raises: InformationModelError if *verify_information_model* is *VerifyInformationModel.yes* and the feature :class:`type<FeatureType>` \
    does not support the 'gpml:shapefileAttributes' property (although all feature :class:`types<FeatureType>` do support it).
    :raises: InformationModelError if a 'gpml:shapefileAttributes' property name is found in this feature but the property value is not a \
    :class:`GpmlKeyValueDictionary` (this should not normally happen).
    
    Shapefile attributes are stored in a :class:`GpmlKeyValueDictionary` property named 'gpml:shapefileAttributes' and
    contain attributes imported from a Shapefile.
    
    This is a convenience method that wraps :meth:`set` for the common property 'gpml:shapefileAttributes' and
    sets the attribute value associated with *key* within that property using :meth:`GpmlKeyValueDictionary.set`.
    
    If a 'gpml:shapefileAttributes' property does not exist in this feature then one is first created and added to this feature
    before setting the shapefile attribute in it.
    
    Set an integer attribute value associated with 'key':
    ::
    
      feature.set_shapefile_attribute('key', 100)
    """
    
    # Get the existing shapefile attributes dictionary (if exists), otherwise create a new dictionary.
    gpml_shapefile_attributes = feature.get_value(_gpml_shapefile_attributes_property_name)
    if not gpml_shapefile_attributes:
        # Add an empty key/value dictionary property to the feature.
        gpml_shapefile_attributes = feature.set(
                _gpml_shapefile_attributes_property_name, GpmlKeyValueDictionary(), verify_information_model).get_value()
    
    try:
        gpml_shapefile_attributes.set(key, value)
    except AttributeError:
        # The property value type did not match the property name.
        # This indicates the data does not conform to the GPlates Geological Information Model (GPGIM).
        raise InformationModelError(
                "Expected a GpmlKeyValueDictionary property value for property name '%s'" %
                    _gpml_shapefile_attributes_property_name.to_qualified_string())

# Add the module function as a class method.
Feature.set_shapefile_attribute = set_shapefile_attribute
# Delete the module reference to the function - we only keep the class method.
del set_shapefile_attribute


def get_total_reconstruction_pole(feature):
    """get_total_reconstruction_pole() -> (int, int, GpmlIrregularSampling) or None
    Returns the *time-dependent* total reconstruction pole of this feature.
    
    :rtype: tuple(int, int, :class:`GpmlIrregularSampling`) or None
    :return: A tuple containing (fixed plate id, moving plate id, time sequence of finite rotations) or None
    
    This is a convenience method that wraps :meth:`get_value` for the common properties 'gpml:fixedReferenceFrame',
    'gpml:movingReferenceFrame' and 'gpml:totalReconstructionPole'.
    
    Returns ``None`` if the feature does not contain a 'gpml:fixedReferenceFrame' plate id,
    a 'gpml:movingReferenceFrame' plate id and a 'gpml:totalReconstructionPole' :class:`GpmlIrregularSampling` (with
    time samples containing :class:`GpmlFiniteRotation` instances).
    
    A feature with :class:`type<FeatureType>` 'gpml:TotalReconstructionSequence' should have these properties
    if it conforms to the GPlates Geological Information Model (GPGIM). These feature types are usually read from a
    GPML rotation file or a PLATES4 rotation ('.rot') file.
    
    Calculate the interpolated :class:`finite rotation<FiniteRotation>` that represents the rotation of a
    moving plate relative to a fixed plate from present day to a specific reconstruction time:
    ::
    
      fixed_plate_id, moving_plate_id, total_reconstruction_pole = rotation_feature.get_total_reconstruction_pole()
      interpolated_finite_rotation_property_value = total_reconstruction_pole.get_value(reconstruction_time)
      if interpolated_finite_rotation_property_value:
          interpolated_finite_rotation = interpolated_finite_rotation_property_value.get_finite_rotation()
    
    ...although it is much easier to use :class:`RotationModel`.
    """

    fixed_plate_id_property_value = feature.get_value(_gpml_fixed_reference_frame_property_name)
    if not fixed_plate_id_property_value:
        return
    
    moving_plate_id_property_value = feature.get_value(_gpml_moving_reference_frame_property_name)
    if not moving_plate_id_property_value:
        return
    
    total_reconstruction_pole_property = feature.get(_gpml_total_reconstruction_pole_property_name)
    if not total_reconstruction_pole_property:
        return
    total_reconstruction_pole_property_value = total_reconstruction_pole_property.get_time_dependent_container()
    if not total_reconstruction_pole_property_value:
        return
    
    try:
        return (fixed_plate_id_property_value.get_plate_id(),
                moving_plate_id_property_value.get_plate_id(),
                total_reconstruction_pole_property_value)
    except AttributeError:
        # The property value type did not match the property name.
        # This indicates the data does not conform to the GPlates Geological Information Model (GPGIM).
        return

# Add the module function as a class method.
Feature.get_total_reconstruction_pole = get_total_reconstruction_pole
# Delete the module reference to the function - we only keep the class method.
del get_total_reconstruction_pole


def set_total_reconstruction_pole(feature, fixed_plate_id, moving_plate_id, total_reconstruction_pole, verify_information_model=VerifyInformationModel.yes):
    """set_total_reconstruction_pole(fixed_plate_id, moving_plate_id, total_reconstruction_pole, \
    [verify_information_model=VerifyInformationModel.yes]) -> Property, Property, Property
    Sets the *time-dependent* total reconstruction pole of this feature.
    
    :param fixed_plate_id: the fixed plate id
    :type fixed_plate_id: int
    :param moving_plate_id: the moving plate id
    :type moving_plate_id: int
    :param total_reconstruction_pole: the time-sequence of rotations
    :type total_reconstruction_pole: :class:`GpmlIrregularSampling` of :class:`GpmlFiniteRotation`
    :param verify_information_model: whether to check the information model before setting (default) or not
    :type verify_information_model: *VerifyInformationModel.yes* or *VerifyInformationModel.no*
    :returns: the fixed plate id property, the moving plate id property and the total reconstruction pole property
    :rtype: tuple of three :class:`Property`
    :raises: InformationModelError if *verify_information_model* is *VerifyInformationModel.yes* and the feature :class:`type<FeatureType>` \
    does not support 'gpml:fixedReferenceFrame', 'gpml:movingReferenceFrame' and 'gpml:totalReconstructionPole' properties.
    
    This is a convenience method that wraps :meth:`set` for the common properties 'gpml:fixedReferenceFrame',
    'gpml:movingReferenceFrame' and 'gpml:totalReconstructionPole'.
    
    A feature with :class:`type<FeatureType>` 'gpml:TotalReconstructionSequence' should support these properties
    if it conforms to the GPlates Geological Information Model (GPGIM). These feature types are usually read from a
    GPML rotation file or a PLATES4 rotation ('.rot') file.
    
    Set the total reconstruction pole with two integer plate IDs:
    ::
    
      feature.set_total_reconstruction_pole(550, 801, total_reconstruction_pole)
    """
    
    return (
        feature.set(_gpml_fixed_reference_frame_property_name, GpmlPlateId(fixed_plate_id), verify_information_model),
        feature.set(_gpml_moving_reference_frame_property_name, GpmlPlateId(moving_plate_id), verify_information_model),
        feature.set(_gpml_total_reconstruction_pole_property_name, total_reconstruction_pole, verify_information_model))

# Add the module function as a class method.
Feature.set_total_reconstruction_pole = set_total_reconstruction_pole
# Delete the module reference to the function - we only keep the class method.
del set_total_reconstruction_pole