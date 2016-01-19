# Copyright (C) 2016 The University of Sydney, Australia
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


# This function is private in this 'pygplates' module (function name prefixed with a single underscore).
def _plate_partitioning_set_geometries(feature_without_geometry, geometries_grouped_by_property_name):
    features = [feature_without_geometry.clone()]
    
    for geometry_property_name, geometries_with_property_name in geometries_grouped_by_property_name.iteritems():
            
        # If we fail to set geometry(s) it could be that there are multiple geometries and
        # that multiple geometries are not allowed for the property name (according to information model).
        # If so then try again by cloning one feature per extra geometry.
        try:
            feature = features[-1] # most recent feature
            feature.set_geometry(geometries_with_property_name, geometry_property_name)
        except InformationModelError:
            feature.set_geometry(geometries_with_property_name[0], geometry_property_name)
            for geometry_index in range(1, len(geometries_with_property_name)):
                feature = feature_without_geometry.clone()
                feature.set_geometry(geometries_with_property_name[geometry_index], geometry_property_name)
                features.append(feature)
    
    # Return sequence of features.
    return features


# This function is private in this 'pygplates' module (function name prefixed with a single underscore).
def _plate_partitioning_copy_partition_properties(partitioning_feature, feature, properties_to_copy):
    for property_to_copy in properties_to_copy:
        # If a property cannot be set on the feature (eg, because not supported by feature type)
        # then don't copy that property.
        try:
            if property_to_copy == PartitionProperty.reconstruction_plate_id:
                # Defaults to zero if partitioning feature has no plate ID.
                feature.set_reconstruction_plate_id(partitioning_feature.get_reconstruction_plate_id())
            elif property_to_copy == PartitionProperty.valid_time_period:
                # Defaults to all time if partitioning feature has no valid time property.
                valid_time_begin, valid_time_end = partitioning_feature.get_valid_time()
                feature.set_valid_time(valid_time_begin, valid_time_end)
            elif property_to_copy == PartitionProperty.valid_time_begin:
                # Defaults to distant past if partitioning feature has no valid time property.
                valid_time_begin = partitioning_feature.get_valid_time()[0]
                valid_time_end = feature.get_valid_time()[1]
                # If partitioning feature's begin time is later than feature's end time then set to feature's end time.
                if valid_time_begin < valid_time_end:
                    valid_time_begin = valid_time_end
                feature.set_valid_time(valid_time_begin, valid_time_end)
            elif property_to_copy == PartitionProperty.valid_time_end:
                # Defaults to distant future if partitioning feature has no valid time property.
                valid_time_end = partitioning_feature.get_valid_time()[1]
                valid_time_begin = feature.get_valid_time()[0]
                # If partitioning feature's end time is earlier than feature's begin time then set to feature's begin time.
                if valid_time_end > valid_time_begin:
                    valid_time_end = valid_time_begin
                feature.set_valid_time(valid_time_begin, valid_time_end)
            else: # 'property_to_copy' is a PropertyName...
                properties = partitioning_feature.get(property_to_copy, PropertyReturn.all)
                if properties:
                    feature.set(property_to_copy, (property.get_value().clone() for property in properties))
        except InformationModelError:
            pass


def plate_partitioner_partition_features(
        plate_partitioner,
        features,
        properties_to_copy=(PartitionProperty.reconstruction_plate_id,),
        partition_method=PartitionMethod.split_into_plates,
        partition_return=PartitionReturn.combined_partitioned_and_unpartitioned):
    """partition_features(features, \
        [properties_to_copy=[PartitionProperty.reconstruction_plate_id]], \
        [partition_method=PartitionMethod.split_into_plates], \
        [partition_return=PartitionReturn.combined_partitioned_and_unpartitioned])
    Partitions features into partitioning plates.
    
    :param features: the features to partition
    :type features: :class:`FeatureCollection`, or string, or :class:`Feature`, \
        or sequence of :class:`Feature`, or sequence of any combination of those four types
    
    :param properties_to_copy: the properties to copy from partitioning plate features to the partitioned features \
        (defaults to just the reconstruction plate ID)
    :type properties_to_copy: a sequence of any combination of :class:`PropertyName` and \
        the *PartitionProperty* enumeration values (see table below), or None
    
    :param partition_method: how the features are to be partitioned by the partitioning plates (defaults to *PartitionMethod.split_into_plates*)
    :type partition_method: a *PartitionMethod* enumeration value (see table below), or None
    
    :param partition_return: how to return the partitioned and unpartitioned features and whether to include the partitioning plates \
        (defaults to *PartitionReturn.combined_partitioned_and_unpartitioned*)
    :type partition_return: a *PartitionReturn* enumeration value (see table below), or None
    
    :returns: the partitioned and unpartitioned features \
        (**note:** new features are always returned, never the originals passed in via *features*)
    :rtype: depends on *partition_return* (see table below)
    
    .. note:: New features are always returned. The original features (passed into the *features* argument) are never modified or returned.
    
    To partition features at present day and write them to a new file:
    ::

        features = plate_partitioner.partition_features('features_to_partition.gpml')
        
        feature_collection = pygplates.FeatureCollection(features)
        feature_collection.write('partitioned_and_unpartitioned_features.gpml')
    
    *partition_method* specifies how the features are to be partitioned by the partitioning plates.
    
    *partition_method* supports the following enumeration values:
    
    +----------------------------------------------------+-------------------------------------------------------------------------------------+
    | Value                                              | Description                                                                         |
    +====================================================+=====================================================================================+
    | *PartitionMethod.split_into_plates*                | Split each feature into partitioning plates and into unpartitioned parts that       |
    |                                                    | are outside all partitioning plates (if plates don't have global coverage).         |
    |                                                    |                                                                                     |
    |                                                    | For example, if a feature overlaps two plates then it will get cloned twice.        |
    |                                                    | Each clone will have its geometry set to the part of the original feature geometry  |
    |                                                    | contained within the respective partitioning plate. Any part (or parts) of the      |
    |                                                    | original feature geometry outside all the plates will result in a third cloned      |
    |                                                    | feature containing the unpartitioned geometry(s).                                   |
    |                                                    |                                                                                     |
    |                                                    | The two partitioned cloned features will have properties copied from the            |
    |                                                    | respective partitioned plate feature (as determined by *properties_to_copy*).       |
    |                                                    | The unpartitioned cloned feature will not have any properties copied to it.         |
    +----------------------------------------------------+-------------------------------------------------------------------------------------+
    | *PartitionMethod.most_overlapping_plate*           | Don't split each feature into partitioning plates, instead use the partitioning     | 
    |                                                    | plate that most overlaps the feature's geometry.                                    |
    |                                                    |                                                                                     |
    |                                                    | For example, if a feature overlaps two plates then it will still only get cloned    |
    |                                                    | once (and its geometry unmodified). Only the most overlapping partitioning plate    |
    |                                                    | (if any) is selected. The overlap is measured based on the length of the polyline   |
    |                                                    | or polygon geometry contained within each partitioning plate (or number of points   |
    |                                                    | if geometry is a multipoint or point).                                              |
    |                                                    |                                                                                     |
    |                                                    | The cloned feature will have properties copied from the most overlapping            |
    |                                                    | partitioned plate feature (as determined by *properties_to_copy*) if it overlaps    |
    |                                                    | any, otherwise it will not have any properties copied to it.                        |
    |                                                    |                                                                                     |
    |                                                    | Note that if a feature contains multiple geometries then they are treated as one    |
    |                                                    | composite geometry in the overlap calculation.                                      |
    +----------------------------------------------------+-------------------------------------------------------------------------------------+
    
    To partition features using the most overlapping partitioning plate:
    ::

        features = plate_partitioner.partition_features(
                'features_to_partition.gpml',
                partition_method = pygplates.PartitionMethod.most_overlapping_plate)
    
    *properties_to_copy* specifies the properties to copy from the partitioning features to the features that are being partitioned.
    
    *properties_to_copy* supports a sequence of any of the following arguments:
    
    +----------------------------------------------------+----------------------------------------------------------------------------------+
    | Type                                               | Description                                                                      |
    +====================================================+==================================================================================+
    | :class:`PropertyName`                              | Any property name. If the partitioning feature has one or more properties        |
    |                                                    | with this name then they will be copied/cloned to the feature being partitioned  |
    |                                                    | provided its :class:`feature type<FeatureType>` supports the property name.      |
    +----------------------------------------------------+----------------------------------------------------------------------------------+
    | *PartitionProperty.reconstruction_plate_id*        | The reconstruction plate ID. This is an alternative to specifying the property   |
    |                                                    | name ``PropertyName.gpml_reconstruction_plate_id``.                              |
    +----------------------------------------------------+----------------------------------------------------------------------------------+
    | *PartitionProperty.valid_time_period*              | The valid time period. This is an alternative to specifying the property name    |
    |                                                    | ``PropertyName.gml_valid_time``.                                                 |
    +----------------------------------------------------+----------------------------------------------------------------------------------+
    | *PartitionProperty.valid_time_begin*               | Only the *begin* time of the valid time period of the partitioning feature is    |
    |                                                    | copied (the *end* time remains unchanged). If the *begin* time is later than     |
    |                                                    | (has a smaller value than) the *end* time then it is set to the *end* time.      |
    |                                                    |                                                                                  |
    |                                                    | Note that there is no equivalent way to specify this using a *PropertyName*.     |
    +----------------------------------------------------+----------------------------------------------------------------------------------+
    | *PartitionProperty.valid_time_end*                 | Only the *end* time of the valid time period of the partitioning feature is      |
    |                                                    | copied (the *begin* time remains unchanged). If the *end* time is earlier than   |
    |                                                    | (has a larger value) the *begin* time then it is set to the *begin* time.        |
    |                                                    |                                                                                  |
    |                                                    | Note that there is no equivalent way to specify this using a *PropertyName*.     |
    +----------------------------------------------------+----------------------------------------------------------------------------------+
    
    To copy reconstruction plate ID, valid time period and name from the partitioning features to their associated partitioned features:
    ::

        features = plate_partitioner.partition_features(
                'features_to_partition.gpml',
                properties_to_copy = [
                    pygplates.PartitionProperty.reconstruction_plate_id,
                    pygplates.PartitionProperty.valid_time_period,
                    pygplates.PropertyName.gml_name])
    
    *partition_return* specifies how the features are to be partitioned by the partitioning plates. This applies regardless of the value of *partition_method*.
    
    *partition_return* supports the following enumeration values:
    
    +----------------------------------------------------------+----------------------------------------------------------+------------------------------------------------------------------------------------+
    | Value                                                    | Return Type                                              | Description                                                                        |
    +==========================================================+==========================================================+====================================================================================+
    | *PartitionReturn.combined_partitioned_and_unpartitioned* | ``list`` of :class:`Feature`                             | Return a single combined ``list`` of partitioned and unpartitioned features.       |
    +----------------------------------------------------------+----------------------------------------------------------+------------------------------------------------------------------------------------+
    | *PartitionReturn.separate_partitioned_and_unpartitioned* | 2-tuple (                                                | Return a 2-tuple whose first element is a ``list`` of partitioned  features and    |
    |                                                          | ``list`` of partitioned :class:`features<Feature>`,      | whose second element is a ``list`` of unpartitioned  features.                     |
    |                                                          | ``list`` of unpartitioned :class:`features<Feature>`)    |                                                                                    |
    +----------------------------------------------------------+----------------------------------------------------------+------------------------------------------------------------------------------------+
    | *PartitionReturn.partitioned_groups_and_unpartitioned*   | 2-tuple (                                                | Return a 2-tuple whose first element is a ``list`` of partitioned groups and       |
    |                                                          | ``list`` of 2-tuple (                                    | whose second element is a ``list`` of unpartitioned features.                      |
    |                                                          | :class:`partitioning plate<ReconstructionGeometry>`,     |                                                                                    |
    |                                                          | ``list`` of partitioned :class:`features<Feature>`),     | Each partitioned group associates a partitioning plate with its partitioned        |
    |                                                          | ``list`` of unpartitioned :class:`features<Feature>`)    | features and consists of a 2-tuple whose first element is the partitioning plate   |
    |                                                          |                                                          | and whose second element is a ``list`` of features partitioned by that plate.      |
    +----------------------------------------------------------+----------------------------------------------------------+------------------------------------------------------------------------------------+
    
    To write only the partitioned features to a new file excluding any unpartitioned features
    (features that did not intersect any partitioning plates):
    ::

        partitioned_features, unpartitioned_features = plate_partitioner.partition_features(
                'features_to_partition.gpml',
                partition_return = pygplates.PartitionReturn.separate_partitioned_and_unpartitioned)
        
        pygplates.FeatureCollection(partitioned_features).write('partitioned_features.gpml')
    """
    
    # Turn function argument into something more convenient for extracting features.
    features = FeaturesFunctionArgument(features)
    
    # Group partitioned features by partitioning plate (if requested).
    if partition_return == PartitionReturn.partitioned_groups_and_unpartitioned:
        partitioned_feature_groups = {}
    
    # Regular lists of partitioned and unpartitioned features.
    partitioned_features = []
    unpartitioned_features = []
    
    for feature in features.get_features():
        # Special case handling for VGP features.
        if feature.get_feature_type() == FeatureType.gpml_virtual_geomagnetic_pole:
            
            
            continue
        
        if partition_method == PartitionMethod.split_into_plates:
            
            # Group partitioned inside geometries by partitioning plate and then by geometry property name.
            partitioned_geometries = { }
            # Group partitioned outside geometries by geometry property name.
            unpartitioned_geometries = { }
            
            feature_without_geometry = feature.clone()
            for property in feature_without_geometry:
                property_value = property.get_value()
                if not property_value:
                    continue
                
                geometry = property_value.get_geometry()
                if not geometry:
                    continue
                
                # Remove geometry property.
                feature_without_geometry.remove(property)
                
                # Partition the current geometry into inside/outside geometries.
                partitioned_inside_geometries = []
                partitioned_outside_geometries = []
                plate_partitioner.partition_geometry(geometry, partitioned_inside_geometries, partitioned_outside_geometries)
                
                geometry_property_name = property.get_name()
                
                if partitioned_inside_geometries:
                    for partitioning_plate, inside_geometries in partitioned_inside_geometries:
                        # Group by partition.
                        geometries_inside_partition = partitioned_geometries.setdefault(partitioning_plate, {})
                        # Group again by property name - there could be multiple geometry properties (per feature) with the same name.
                        geometries_in_partition_with_property_name = geometries_inside_partition.setdefault(geometry_property_name, [])
                        geometries_in_partition_with_property_name.extend(inside_geometries)
                
                if partitioned_outside_geometries:
                    # Group by property name - there could be multiple geometry properties (per feature) with the same name.
                    unpartitioned_geometries_with_property_name = unpartitioned_geometries.setdefault(geometry_property_name, [])
                    unpartitioned_geometries_with_property_name.extend(partitioned_outside_geometries)
            
            for partitioning_plate, geometries_inside_partition in partitioned_geometries.iteritems():
                features_inside_partition = _plate_partitioning_set_geometries(feature_without_geometry, geometries_inside_partition)
                
                # Copy the requested properties over from the partitioning feature.
                for feature in features_inside_partition:
                    _plate_partitioning_copy_partition_properties(partitioning_plate.get_feature(), feature, properties_to_copy)
                
                partitioned_features.extend(features_inside_partition)
                
                # Also group partitioned features by partitioning plate (if requested).
                if partition_return == PartitionReturn.partitioned_groups_and_unpartitioned:
                    partitioned_feature_groups.setdefault(partitioning_plate, []).extend(features_inside_partition)
            
            if unpartitioned_geometries:
                features_outside_partition = _plate_partitioning_set_geometries(feature_without_geometry, unpartitioned_geometries)
                unpartitioned_features.extend(features_outside_partition)
            
        else: # partition_method == PartitionMethod.most_overlapping_plate
            
            # Partition all geometries of the current feature so that we can do partition overlap testing.
            partitioned_inside_geometries = []
            plate_partitioner.partition_geometry(feature.get_all_geometries(), partitioned_inside_geometries)
            
            if partitioned_inside_geometries:
                # Determine which partitioning plate (if feature is partitioned into any) overlaps the current feature the most.
                max_geometry_size_measure = (0.0, 0)
                most_overlapping_partitioning_plate = None
                for partitioning_plate, inside_geometries in partitioned_inside_geometries:
                    # Accumulate the size of the geometries inside the current partitioning plate.
                    geometries_arc_length = 0.0
                    geometries_num_points = 0
                    for geometry in inside_geometries:
                        # If geometry is a polyline or polygon then accumulate its arc length.
                        # Otherwise it's a point or multipoint so accumulate number of points.
                        try:
                            geometries_arc_length += geometry.get_arc_length()
                        except AttributeError:
                            geometries_num_points += len(geometry.get_points())
                    
                    # When comparing measures we favour arc length (for polyline/polygon),
                    # followed by number points (for point/multipoint).
                    geometry_size_measure = (geometries_arc_length, geometries_num_points)
                    if geometry_size_measure > max_geometry_size_measure:
                        max_geometry_size_measure = geometry_size_measure
                        most_overlapping_partitioning_plate = partitioning_plate
                
                # Clone the feature so we don't modify the original (we're returning copies).
                partitioned_feature = feature.clone()
                
                # Copy the requested properties over from the partitioning feature.
                _plate_partitioning_copy_partition_properties(
                        most_overlapping_partitioning_plate.get_feature(), partitioned_feature, properties_to_copy)
                
                partitioned_features.append(partitioned_feature)
                
                # Also group partitioned features by partitioning plate (if requested).
                if partition_return == PartitionReturn.partitioned_groups_and_unpartitioned:
                    partitioned_feature_groups.setdefault(most_overlapping_partitioning_plate, []).append(partitioned_feature)
                
            else:
                # Feature does not overlap any partitions so it is unpartitioned.
                unpartitioned_feature = feature.clone()
                unpartitioned_features.append(unpartitioned_feature)
    
    # Reverse reconstruct all partitioned features (using their new plate IDs) if their geometries are not at present day.
    reconstruction_time = plate_partitioner._get_reconstruction_time()
    if reconstruction_time != GeoTimeInstant(0):
        rotation_model = plate_partitioner._get_rotation_model()
        reverse_reconstruct(partitioned_features, rotation_model, reconstruction_time)
    
    # Return partitioned and unpartitioned features in the format requested by the caller.
    if partition_return == PartitionReturn.combined_partitioned_and_unpartitioned:
        return partitioned_features + unpartitioned_features
    elif partition_return == PartitionReturn.separate_partitioned_and_unpartitioned:
        return (partitioned_features, unpartitioned_features)
    else: # partition_return == PartitionReturn.partitioned_groups_and_unpartitioned
        return (partitioned_feature_groups.items(), unpartitioned_features)


# Add the module function as a class method.
PlatePartitioner.partition_features = plate_partitioner_partition_features
# Delete the module reference to the function - we only keep the class method.
del plate_partitioner_partition_features


def partition_into_plates(
        partitioning_features,
        rotation_model,
        features_to_partition,
        properties_to_copy=(PartitionProperty.reconstruction_plate_id,),
        reconstruction_time=0,
        partition_method=PartitionMethod.split_into_plates,
        partition_return=PartitionReturn.combined_partitioned_and_unpartitioned,
        sort_partitioning_plates=SortPartitioningPlates.by_partition_type_then_plate_id):
    """partition_into_plates(partitioning_features, rotation_model, features_to_partition, \
        [properties_to_copy=[PartitionProperty.reconstruction_plate_id]], \
        [reconstruction_time=0], \
        [partition_method=PartitionMethod.split_into_plates], \
        [partition_return=PartitionReturn.combined_partitioned_and_unpartitioned], \
        [sort_partitioning_plates=SortPartitioningPlates.by_partition_type_then_plate_id])
    Partition features into plates.
    
    :param partitioning_features: the partitioning features
    :type partitioning_features: :class:`FeatureCollection`, or string, or :class:`Feature`, \
        or sequence of :class:`Feature`, or sequence of any combination of those four types
    
    :param rotation_model: A rotation model or a rotation feature collection or a rotation \
        filename or a sequence of rotation feature collections and/or rotation filenames
    :type rotation_model: :class:`RotationModel` or :class:`FeatureCollection` or string \
        or sequence of :class:`FeatureCollection` instances and/or strings
    
    :param features_to_partition: the features to be partitioned
    :type features_to_partition: :class:`FeatureCollection`, or string, or :class:`Feature`, \
        or sequence of :class:`Feature`, or sequence of any combination of those four types
    
    :param properties_to_copy: the properties to copy from partitioning plate features to the partitioned features \
        (defaults to just the reconstruction plate ID)
    :type properties_to_copy: a sequence of any combination of :class:`PropertyName` and \
        the *PartitionProperty* enumeration values (see table below), or None
    
    :param reconstruction_time: the specific geological time to reconstruct/resolve the \
        *partitioning_features* to (defaults to zero)
    :type reconstruction_time: float or :class:`GeoTimeInstant`, or None
    
    :param partition_method: how the features are to be partitioned by the partitioning plates (defaults to *PartitionMethod.split_into_plates*)
    :type partition_method: a *PartitionMethod* enumeration value (see table below), or None
    
    :param partition_return: how to return the partitioned and unpartitioned features and whether to include the partitioning plates \
        (defaults to *PartitionReturn.combined_partitioned_and_unpartitioned*)
    :type partition_return: a *PartitionReturn* enumeration value (see table below), or None
    
    :param sort_partitioning_plates: optional sort order of partitioning plates \
        (defaults to *SortPartitioningPlates.by_partition_type_then_plate_id*)
    :type sort_partitioning_plates: a *SortPartitioningPlates* enumeration value (see table below), or None
    
    :returns: the partitioned and unpartitioned features \
        (**note:** new features are always returned, never the originals passed in via *features_to_partition*)
    :rtype: depends on *partition_return* (see table below)
    
    .. note:: New features are always returned. The original features (passed into the *features_to_partition* argument) are never modified or returned.
    
    To partition features at present day and write them to a new file:
    ::

        features = pygplates.partition_into_plates(
                'static_polygons.gpml',
                'rotations.rot',
                'features_to_partition.gpml')
        
        feature_collection = pygplates.FeatureCollection(features)
        feature_collection.write('partitioned_and_unpartitioned_features.gpml')
    
    *partition_method* specifies how the features are to be partitioned by the partitioning plates.
    
    *partition_method* supports the following enumeration values:
    
    +----------------------------------------------------+-------------------------------------------------------------------------------------+
    | Value                                              | Description                                                                         |
    +====================================================+=====================================================================================+
    | *PartitionMethod.split_into_plates*                | Split each feature into partitioning plates and into unpartitioned parts that       |
    |                                                    | are outside all partitioning plates (if plates don't have global coverage).         |
    |                                                    |                                                                                     |
    |                                                    | For example, if a feature overlaps two plates then it will get cloned twice.        |
    |                                                    | Each clone will have its geometry set to the part of the original feature geometry  |
    |                                                    | contained within the respective partitioning plate. Any part (or parts) of the      |
    |                                                    | original feature geometry outside all the plates will result in a third cloned      |
    |                                                    | feature containing the unpartitioned geometry(s).                                   |
    |                                                    |                                                                                     |
    |                                                    | The two partitioned cloned features will have properties copied from the            |
    |                                                    | respective partitioned plate feature (as determined by *properties_to_copy*).       |
    |                                                    | The unpartitioned cloned feature will not have any properties copied to it.         |
    +----------------------------------------------------+-------------------------------------------------------------------------------------+
    | *PartitionMethod.most_overlapping_plate*           | Don't split each feature into partitioning plates, instead use the partitioning     | 
    |                                                    | plate that most overlaps the feature's geometry.                                    |
    |                                                    |                                                                                     |
    |                                                    | For example, if a feature overlaps two plates then it will still only get cloned    |
    |                                                    | once (and its geometry unmodified). Only the most overlapping partitioning plate    |
    |                                                    | (if any) is selected. The overlap is measured based on the length of the polyline   |
    |                                                    | or polygon geometry contained within each partitioning plate (or number of points   |
    |                                                    | if geometry is a multipoint or point).                                              |
    |                                                    |                                                                                     |
    |                                                    | The cloned feature will have properties copied from the most overlapping            |
    |                                                    | partitioned plate feature (as determined by *properties_to_copy*) if it overlaps    |
    |                                                    | any, otherwise it will not have any properties copied to it.                        |
    |                                                    |                                                                                     |
    |                                                    | Note that if a feature contains multiple geometries then they are treated as one    |
    |                                                    | composite geometry in the overlap calculation.                                      |
    +----------------------------------------------------+-------------------------------------------------------------------------------------+
    
    To partition features using the most overlapping partitioning plate:
    ::

        features = pygplates.partition_into_plates(
                'static_polygons.gpml',
                'rotations.rot',
                'features_to_partition.gpml',
                partition_method = pygplates.PartitionMethod.most_overlapping_plate)
    
    *properties_to_copy* specifies the properties to copy from the partitioning features to the features that are being partitioned.
    
    *properties_to_copy* supports a sequence of any of the following arguments:
    
    +----------------------------------------------------+----------------------------------------------------------------------------------+
    | Type                                               | Description                                                                      |
    +====================================================+==================================================================================+
    | :class:`PropertyName`                              | Any property name. If the partitioning feature has one or more properties        |
    |                                                    | with this name then they will be copied/cloned to the feature being partitioned  |
    |                                                    | provided its :class:`feature type<FeatureType>` supports the property name.      |
    +----------------------------------------------------+----------------------------------------------------------------------------------+
    | *PartitionProperty.reconstruction_plate_id*        | The reconstruction plate ID. This is an alternative to specifying the property   |
    |                                                    | name ``PropertyName.gpml_reconstruction_plate_id``.                              |
    +----------------------------------------------------+----------------------------------------------------------------------------------+
    | *PartitionProperty.valid_time_period*              | The valid time period. This is an alternative to specifying the property name    |
    |                                                    | ``PropertyName.gml_valid_time``.                                                 |
    +----------------------------------------------------+----------------------------------------------------------------------------------+
    | *PartitionProperty.valid_time_begin*               | Only the *begin* time of the valid time period of the partitioning feature is    |
    |                                                    | copied (the *end* time remains unchanged). If the *begin* time is later than     |
    |                                                    | (has a smaller value than) the *end* time then it is set to the *end* time.      |
    |                                                    |                                                                                  |
    |                                                    | Note that there is no equivalent way to specify this using a *PropertyName*.     |
    +----------------------------------------------------+----------------------------------------------------------------------------------+
    | *PartitionProperty.valid_time_end*                 | Only the *end* time of the valid time period of the partitioning feature is      |
    |                                                    | copied (the *begin* time remains unchanged). If the *end* time is earlier than   |
    |                                                    | (has a larger value) the *begin* time then it is set to the *begin* time.        |
    |                                                    |                                                                                  |
    |                                                    | Note that there is no equivalent way to specify this using a *PropertyName*.     |
    +----------------------------------------------------+----------------------------------------------------------------------------------+
    
    To copy reconstruction plate ID, valid time period and name from the partitioning features to their associated partitioned features:
    ::

        features = pygplates.partition_into_plates(
                'static_polygons.gpml',
                'rotations.rot',
                'features_to_partition.gpml',
                properties_to_copy = [
                    pygplates.PartitionProperty.reconstruction_plate_id,
                    pygplates.PartitionProperty.valid_time_period,
                    pygplates.PropertyName.gml_name])
    
    *partition_return* specifies how the features are to be partitioned by the partitioning plates. This applies regardless of the value of *partition_method*.
    
    *partition_return* supports the following enumeration values:
    
    +----------------------------------------------------------+----------------------------------------------------------+------------------------------------------------------------------------------------+
    | Value                                                    | Return Type                                              | Description                                                                        |
    +==========================================================+==========================================================+====================================================================================+
    | *PartitionReturn.combined_partitioned_and_unpartitioned* | ``list`` of :class:`Feature`                             | Return a single combined ``list`` of partitioned and unpartitioned features.       |
    +----------------------------------------------------------+----------------------------------------------------------+------------------------------------------------------------------------------------+
    | *PartitionReturn.separate_partitioned_and_unpartitioned* | 2-tuple (                                                | Return a 2-tuple whose first element is a ``list`` of partitioned  features and    |
    |                                                          | ``list`` of partitioned :class:`features<Feature>`,      | whose second element is a ``list`` of unpartitioned  features.                     |
    |                                                          | ``list`` of unpartitioned :class:`features<Feature>`)    |                                                                                    |
    +----------------------------------------------------------+----------------------------------------------------------+------------------------------------------------------------------------------------+
    | *PartitionReturn.partitioned_groups_and_unpartitioned*   | 2-tuple (                                                | Return a 2-tuple whose first element is a ``list`` of partitioned groups and       |
    |                                                          | ``list`` of 2-tuple (                                    | whose second element is a ``list`` of unpartitioned features.                      |
    |                                                          | :class:`partitioning plate<ReconstructionGeometry>`,     |                                                                                    |
    |                                                          | ``list`` of partitioned :class:`features<Feature>`),     | Each partitioned group associates a partitioning plate with its partitioned        |
    |                                                          | ``list`` of unpartitioned :class:`features<Feature>`)    | features and consists of a 2-tuple whose first element is the partitioning plate   |
    |                                                          |                                                          | and whose second element is a ``list`` of features partitioned by that plate.      |
    +----------------------------------------------------------+----------------------------------------------------------+------------------------------------------------------------------------------------+
    
    To write only the partitioned features to a new file excluding any unpartitioned features
    (features that did not intersect any partitioning plates):
    ::

        partitioned_features, unpartitioned_features = pygplates.partition_into_plates(
                'static_polygons.gpml',
                'rotations.rot',
                'features_to_partition.gpml',
                partition_return = pygplates.PartitionReturn.separate_partitioned_and_unpartitioned)
        
        pygplates.FeatureCollection(partitioned_features).write('partitioned_features.gpml')
    
    The partitioning plates are generated internally by :func:`reconstructing the regular geological features<reconstruct>`
    and :func:`resolving the topological features<resolve_topologies>` in *partitioning_features* using the rotation model and
    optional reconstruction time.
    
    *sort_partitioning_plates* determines the sorting criteria used to order the partitioning plates:
    
    +----------------------------------------------------------+--------------------------------------------------------------------------------------+
    |  Value                                                   | Description                                                                          |
    +==========================================================+======================================================================================+
    | SortPartitioningPlates.by_partition_type                 | Group in order of resolved topological networks then resolved topological boundaries |
    |                                                          | then reconstructed static polygons, but with no sorting within each group            |
    |                                                          | (ordering within each group is unchanged).                                           |
    +----------------------------------------------------------+--------------------------------------------------------------------------------------+
    | SortPartitioningPlates.by_partition_type_then_plate_id   | Same as *by_partition_type*, but also sort by plate ID (from highest to lowest)      |
    |                                                          | within each partition type group.                                                    |
    +----------------------------------------------------------+--------------------------------------------------------------------------------------+
    | SortPartitioningPlates.by_partition_type_then_plate_area | Same as *by_partition_type*, but also sort by plate area (from highest to lowest)    |
    |                                                          | within each partition type group.                                                    |
    +----------------------------------------------------------+--------------------------------------------------------------------------------------+
    | SortPartitioningPlates.by_plate_id                       | Sort by plate ID (from highest to lowest), but no grouping by partition type.        |
    +----------------------------------------------------------+--------------------------------------------------------------------------------------+
    | SortPartitioningPlates.by_plate_area                     | Sort by plate area (from highest to lowest), but no grouping by partition type.      |
    +----------------------------------------------------------+--------------------------------------------------------------------------------------+
    
    .. note:: If you don't want to sort the partitioning plates (for example, if you have already sorted them)
      then you'll need to explicitly specify ``None`` for the *sort_partitioning_plates* parameter
      (eg, ``pygplates.partition_into_plates(..., sort_partitioning_plates=None)``).
      
      This is because not specifying anything defaults to *SortPartitioningPlates.by_partition_type_then_plate_id*
      (since this always gives deterministic partitioning results).
    
    If the partitioning plates overlap each other then their final ordering determines the partitioning results.
    Resolved topologies do not tend to overlap, but reconstructed static polygons do overlap
    (for non-zero reconstruction times) and hence the sorting order becomes relevant.
    
    Partitioning of points is more efficient if you sort by plate *area* because an arbitrary
    point is likely to be found sooner when testing against larger partitioning polygons first
    (and hence more remaining partitioning polygons can be skipped). Since resolved topologies don't tend
    to overlap you don't need to sort them by plate *ID* to get deterministic partitioning results.
    So we are free to sort by plate *area* (well, plate area is also deterministic but not as deterministic
    as sorting by plate *ID* since modifications to the plate geometries change their areas but not their plate IDs).
    Note that we also group by partition type in case the topological networks happen
    to overlay the topological plate boundaries (usually this isn't the case though):
    ::
    
        features = pygplates.partition_into_plates(...,
            sort_partitioning_plates=pygplates.SortPartitioningPlates.by_partition_type_then_plate_area)
    
    .. note:: Only those reconstructed/resolved geometries that contain a *polygon* boundary are actually used for partitioning.
       For :func:`resolved topologies<resolve_topologies>` this includes :class:`ResolvedTopologicalBoundary` and
       :class:`ResolvedTopologicalNetwork`. For :func:`reconstructed geometries<reconstruct>`, a :class:`ReconstructedFeatureGeometry`
       is only included if its reconstructed geometry is a :class:`PolygonOnSphere`.
    
    .. seealso:: :meth:`PlatePartitioner.partition_features`
    
       :func:`partition_into_plates` is a convenience function that essentially uses
       :meth:`PlatePartitioner.partition_features` in the following way:
       ::
    
        def partition_into_plates(
                partitioning_features,
                rotation_model,
                features_to_partition,
                properties_to_copy = [PartitionProperty.reconstruction_plate_id],
                reconstruction_time = 0,
                partition_method = PartitionMethod.split_into_plates,
                partition_return = PartitionReturn.combined_partitioned_and_unpartitioned,
                sort_partitioning_plates = pygplates.SortPartitioningPlates.by_partition_type_then_plate_id):
            
            plate_partitioner = pygplates.PlatePartitioner(partitioning_features, rotation_model, reconstruction_time, sort_partitioning_plates)
            
            return plate_partitioner.partition_features(features_to_partition, properties_to_copy, partition_method, partition_return)
    """
    
    plate_partitioner = PlatePartitioner(partitioning_features, rotation_model, reconstruction_time, sort_partitioning_plates)
    
    return plate_partitioner.partition_features(features_to_partition, properties_to_copy, partition_method, partition_return)