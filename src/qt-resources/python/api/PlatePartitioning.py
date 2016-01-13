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
                    feature.set(property_to_copy, [property.get_value().clone() for property in properties])
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
    
    :param properties_to_copy: the properties to copy from partitioning plate features to the partitioned features
    :type properties_to_copy: a sequence of any of :class:`PropertyName` or *PartitionProperty.reconstruction_plate_id* or \
    *PartitionProperty.valid_time_period* or *PartitionProperty.valid_time_begin* or *PartitionProperty.valid_time_end*
    
    :param partition_method: whether to split features into partitioning plates, or don't split but \
        choose a single most-overlapping partitioning plate (eg, to copy properties from)
    :type partition_method: *PartitionMethod.split_into_plates* or *PartitionMethod.most_overlapping_plate*
    
    :param partition_return: whether to return a single combined list of partitioned and \
        unpartitioned features, or two separate lists, or a list of partitioned groups and a list of unpartitioned features \
        (where each partitioned group associates a partitioning plate with its partitioned features)
    :type partition_return: *PartitionReturn.combined_partitioned_and_unpartitioned*, \
        *PartitionReturn.separate_partitioned_and_unpartitioned* or \
        *PartitionReturn.partitioned_groups_and_unpartitioned*
    
    :rtype: list of partitioned and unpartitioned :class:`features<Feature>`, \
        or 2-tuple (list of partitioned :class:`features<Feature>`, list of unpartitioned :class:`features<Feature>`), \
        or 2-tuple (list of 2-tuple (:class:`partitioning plate<ReconstructionGeometry>`, list of partitioned :class:`features<Feature>`), \
            list of unpartitioned :class:`features<Feature>`) - depending on *partition_return*
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
    
    :param properties_to_copy: the properties to copy from partitioning plate features to the partitioned features
    :type properties_to_copy: a sequence of any of :class:`PropertyName` or *PartitionProperty.reconstruction_plate_id* or \
    *PartitionProperty.valid_time_period* or *PartitionProperty.valid_time_begin* or *PartitionProperty.valid_time_end*
    
    :param reconstruction_time: the specific geological time to reconstruct/resolve the \
        *partitioning_features* to
    :type reconstruction_time: float or :class:`GeoTimeInstant`
    
    :param partition_method: whether to split features into partitioning plates, or don't split but \
        choose a single most-overlapping partitioning plate (eg, to copy properties from)
    :type partition_method: *PartitionMethod.split_into_plates* or *PartitionMethod.most_overlapping_plate*
    
    :param partition_return: whether to return a single combined list of partitioned and \
        unpartitioned features, or two separate lists, or a list of partitioned groups and a list of unpartitioned features \
        (where each partitioned group associates a partitioning plate with its partitioned features)
    :type partition_return: *PartitionReturn.combined_partitioned_and_unpartitioned*, \
        *PartitionReturn.separate_partitioned_and_unpartitioned* or \
        *PartitionReturn.partitioned_groups_and_unpartitioned*
    
    :param sort_partitioning_plates: optional sort order of partitioning plates \
        (defaults to *SortPartitioningPlates.by_partition_type_then_plate_id*)
    :type sort_partitioning_plates: one of the values in the *SortPartitioningPlates* table below, or None
    
    :rtype: list of partitioned and unpartitioned :class:`features<Feature>`, \
        or 2-tuple (list of partitioned :class:`features<Feature>`, list of unpartitioned :class:`features<Feature>`), \
        or 2-tuple (list of 2-tuple (:class:`partitioning plate<ReconstructionGeometry>`, list of partitioned :class:`features<Feature>`), \
            list of unpartitioned :class:`features<Feature>`) - depending on *partition_return*
    
    This is a convenience function that essentially does the following:
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
    
    To partition features at present day and write only the partitioned features to a new file:
    ::

        partitioned_features, unpartitioned_features = pygplates.partition_into_plates(
                'static_polygons.gpml',
                'rotations.rot',
                'features_to_partition.gpml',
                partition_return = PartitionReturn.separate_partitioned_and_unpartitioned)
        
        pygplates.FeatureCollection(partitioned_features).write('partitioned_features.gpml')
    
    .. seealso:: :meth:`PlatePartitioner.partition_features`
    """
    
    plate_partitioner = PlatePartitioner(partitioning_features, rotation_model, reconstruction_time, sort_partitioning_plates)
    
    return plate_partitioner.partition_features(features_to_partition, properties_to_copy, partition_method, partition_return)