# Private variable property names of common feature properties.
# We create them once to avoid constantly re-creating them each time a class method is called.
_gml_description_property_name = PropertyName.create_gml('description')
_gml_name_property_name = PropertyName.create_gml('name')
_gml_valid_time_property_name = PropertyName.create_gml('validTime')
_gpml_left_plate_property_name = PropertyName.create_gpml('leftPlate')
_gpml_right_plate_property_name = PropertyName.create_gpml('rightPlate')
_gpml_reconstruction_plate_id_property_name = PropertyName.create_gpml('reconstructionPlateId')
_gpml_conjugate_plate_id_property_name = PropertyName.create_gpml('conjugatePlateId')
_gpml_fixed_reference_frame_property_name = PropertyName.create_gpml('fixedReferenceFrame')
_gpml_moving_reference_frame_property_name = PropertyName.create_gpml('movingReferenceFrame')
_gpml_total_reconstruction_pole_property_name = PropertyName.create_gpml('totalReconstructionPole')


def get_description(feature, default=''):
    """get_description([default='']) -> str
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


def set_description(feature, description):
    """set_description(description) -> Property
    Sets the description of this feature.
    
    :param description: the description
    :type description: string
    :returns: the property containing the description
    :rtype: :class:`Property`
    
    This is a convenience method that wraps :meth:`set` for the common property 'gml:description'.
    
    Set the description to a string:
    ::
    
      feature.set_description('description')
    """
    
    return feature.set(_gml_description_property_name, XsString(description))

# Add the module function as a class method.
Feature.set_description = set_description
# Delete the module reference to the function - we only keep the class method.
del set_description


def get_name(feature, default='', property_return=PropertyReturn.exactly_one):
    """get_name([default=''], [property_return=PropertyReturn.exactly_one]) -> str or list
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
      if name:
        ...
    
    Test that there is exactly one 'gml:name' property and that it is not the empty string:
    ::
    
      name = feature.get_name()
      if name:
        ...
    
    Return the list of names as strings (defaults to an empty list if no names are found):
    ::
    
      names = feature.get_name([], PropertyReturn.all)
    
    Test if there are any 'gml:name' properties:
    ::
    
      names = feature.get_names(None, PropertyReturn.all)
      if names:
        ...
    
    Test if there are any 'gml:name' properties with a non-empty string:
    ::
    
      names = feature.get_names(None, PropertyReturn.all)
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


def set_name(feature, name):
    """set_name(name) -> Property or list
    Set the name (or names) of this feature.
    
    :param name: the name or names
    :type name: string, or sequence of string
    :returns: the property containing the name, or properties containing the names
    :rtype: :class:`Property`, or list of :class:`Property`
    
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
        return feature.set(_gml_name_property_name, [XsString(n) for n in name])
    
    return feature.set(_gml_name_property_name, XsString(name))

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


def set_valid_time(feature, begin_time, end_time):
    """set_valid_time(begin_time, end_time) -> Property
    Sets the valid time range of this feature.
    
    :param begin_time: the begin time (time of appearance)
    :type begin_time: float or :class:`GeoTimeInstant`
    :param end_time: the end time (time of disappearance)
    :type end_time: float or :class:`GeoTimeInstant`
    :returns: the property containing the valid time range
    :rtype: :class:`Property`
    
    This is a convenience method that wraps :meth:`set` for the common property 'gml:validTime'.
    
    Set the valid time range to include all geological time up until present day:
    ::
    
      feature.set_valid_time(pygplates.GeoTimeInstant.create_distant_past(), 0)
    """
    
    return feature.set(_gml_valid_time_property_name, GmlTimePeriod(begin_time, end_time))

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
      if left_plate:
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


def set_left_plate(feature, left_plate):
    """set_left_plate(left_plate) -> Property
    Sets the left plate ID of this feature.
    
    :param left_plate: the left plate id
    :type left_plate: int
    :returns: the property containing the left plate id
    :rtype: :class:`Property`
    
    This is a convenience method that wraps :meth:`set` for the common property 'gpml:leftPlate'.
    
    Set the left plate ID to an integer:
    ::
    
      feature.set_left_plate(201)
    """
    
    return feature.set(_gpml_left_plate_property_name, GpmlPlateId(left_plate))

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
      if right_plate:
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


def set_right_plate(feature, right_plate):
    """set_right_plate(right_plate) -> Property
    Sets the right plate ID of this feature.
    
    :param right_plate: the right plate id
    :type right_plate: int
    :returns: the property containing the right plate id
    :rtype: :class:`Property`
    
    This is a convenience method that wraps :meth:`set` for the common property 'gpml:rightPlate'.
    
    Set the right plate ID to an integer:
    ::
    
      feature.set_right_plate(701)
    """
    
    return feature.set(_gpml_right_plate_property_name, GpmlPlateId(right_plate))

# Add the module function as a class method.
Feature.set_right_plate = set_right_plate
# Delete the module reference to the function - we only keep the class method.
del set_right_plate


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
      if reconstruction_plate_id:
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


def set_reconstruction_plate_id(feature, reconstruction_plate_id):
    """set_reconstruction_plate_id(reconstruction_plate_id) -> Property
    Sets the reconstruction plate ID of this feature.
    
    :param reconstruction_plate_id: the reconstruction plate id
    :type reconstruction_plate_id: int
    :returns: the property containing the reconstruction plate id
    :rtype: :class:`Property`
    
    This is a convenience method that wraps :meth:`set` for the common property 'gpml:reconstructionPlateId'.
    
    Set the reconstruction plate ID to an integer:
    ::
    
      feature.set_reconstruction_plate_id(701)
    """
    
    return feature.set(_gpml_reconstruction_plate_id_property_name, GpmlPlateId(reconstruction_plate_id))

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
    :rtype: int, or list of ints, or type(*default*)
    
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
      if conjugate_plate_id:
        ...
    
    Test that there is exactly one 'gpml:conjugatePlateId' property and that it is not zero:
    ::
    
      conjugate_plate_id = feature.get_conjugate_plate_id()
      if conjugate_plate_id:
        ...
    
    Return the list of conjugate plate IDs as integers (defaults to an empty list if no conjugate plate IDs are found):
    ::
    
      conjugate_plate_ids = feature.get_conjugate_plate_id([], PropertyReturn.all)
    
    Test if there are any 'gpml:conjugatePlateId' properties:
    ::
    
      conjugate_plate_ids = feature.get_conjugate_plate_id(None, PropertyReturn.all)
      if conjugate_plate_ids:
        ...
    
    Test if there are any 'gpml:conjugatePlateId' properties with a non-zero plate ID:
    ::
    
      conjugate_plate_ids = feature.get_conjugate_plate_id(None, PropertyReturn.all)
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


def set_conjugate_plate_id(feature, conjugate_plate_id):
    """set_conjugate_plate_id(conjugate_plate_id) -> Property or list
    Set the conjugate plate ID (or IDs) of this feature.
    
    :param conjugate_plate_id: the conjugate plate ID or plate IDs
    :type conjugate_plate_id: int, or sequence of int
    :returns: the property containing the conjugate plate ID, or properties containing the conjugate plate IDs
    :rtype: :class:`Property`, or list of :class:`Property`
    
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
        return feature.set(_gpml_conjugate_plate_id_property_name, [GpmlPlateId(id) for id in conjugate_plate_id])
    
    return feature.set(_gpml_conjugate_plate_id_property_name, GpmlPlateId(conjugate_plate_id))

# Add the module function as a class method.
Feature.set_conjugate_plate_id = set_conjugate_plate_id
# Delete the module reference to the function - we only keep the class method.
del set_conjugate_plate_id


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
      interpolated_finite_rotation = total_reconstruction_pole.get_value(reconstruction_time).get_finite_rotation()
    
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
    :type total_reconstruction_pole: :class:`GpmlIrregularSampling` of :class:`rotations<FiniteRotation>`
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
    
      feature.set_total_reconstruction_pole(701, 201, total_reconstruction_pole)
    """
    
    return (
        feature.set(_gpml_fixed_reference_frame_property_name, GpmlPlateId(fixed_plate_id), verify_information_model),
        feature.set(_gpml_moving_reference_frame_property_name, GpmlPlateId(moving_plate_id), verify_information_model),
        feature.set(_gpml_total_reconstruction_pole_property_name, total_reconstruction_pole, verify_information_model))

# Add the module function as a class method.
Feature.set_total_reconstruction_pole = set_total_reconstruction_pole
# Delete the module reference to the function - we only keep the class method.
del set_total_reconstruction_pole
