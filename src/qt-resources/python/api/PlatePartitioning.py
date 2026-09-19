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
    
    for geometry_property_name, geometries_with_property_name in geometries_grouped_by_property_name.items():
            
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
    # The caller specified either a sequence of properties to copy or
    # a single callable that copies the properties.
    if hasattr(properties_to_copy, '__iter__'):
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
    
    else: # ...'properties_to_copy' is a callable...
        properties_to_copy(partitioning_feature, feature)


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
    :type features: FeatureCollection, or str, or os.PathLike, or Feature, \
        or sequence of Feature, or sequence of any combination of those four types
    
    :param properties_to_copy: the properties to copy from partitioning plate features to the partitioned features \
        (defaults to just the reconstruction plate ID)
    :type properties_to_copy: sequence of any combination of PropertyName and PartitionProperty enumeration values
    
    :param partition_method: how the features are to be partitioned by the partitioning plates (defaults to *PartitionMethod.split_into_plates*)
    :type partition_method: PartitionMethod
    
    :param partition_return: how to return the partitioned and unpartitioned features and whether to include the partitioning plates \
        (defaults to *PartitionReturn.combined_partitioned_and_unpartitioned*)
    :type partition_return: PartitionReturn
    
    :returns: the partitioned and unpartitioned features, in the format specified by *partition_return* \
        (see :class:`PartitionReturn`) \
        (**note:** new features are always returned, never the originals passed in via *features*)
    :rtype: list[Feature], or tuple[list[Feature], list[Feature]], or \
        tuple[list[tuple[ReconstructionGeometry, list[Feature]]], list[Feature]]
    
    The features in *features* are tested for overlap/intersection with the partitioning plates using the partition method
    specified by *partition_method*. Properties are copied from the partitioning plate features to the
    features partitioned by them as specified by *properties_to_copy* (by default this is only the reconstruction plate ID).
    
    .. note:: New features are always returned. The original features (passed into the *features* argument) are never modified or returned.
    
    So while the partitioning polygons are reconstructed/resolved to the reconstruction time before testing for overlap/intersection,
    the geometries in the features to be partitioned (*features*) are not since they effectively represent a snapshot of the features at the reconstruction time.
    In other words the features to be partitioned effectively contain geometry at the reconstruction time (rather than present day) and hence they are *not* reconstructed
    to the reconstruction time before testing for overlap/intersection with the partitioning plates (even if they already happen to have a reconstruction plate ID property).
    
    To partition features at present day (and assign reconstruction plate IDs) and write them to a new file:
    ::

        plate_partitioner = pygplates.PlatePartitioner('static_polygons.gpml', 'rotations.rot')
        features = plate_partitioner.partition_features('features_to_partition.gpml')
        
        feature_collection = pygplates.FeatureCollection(features)
        feature_collection.write('partitioned_and_unpartitioned_features.gpml')
    
    *partition_method* specifies how the features are to be partitioned by the partitioning plates.
    
    See :class:`PartitionMethod` for a description of each value.
    
    .. note:: VirtualGeomagneticPole features (of :class:`type<FeatureType>` ``FeatureType.gpml_virtual_geomagnetic_pole``) ignore *partition_method*
       since these features are always partitioned using the average sample site position (``PropertyName.gpml_average_sample_site_position``).
       The pole position (``PropertyName.gpml_pole_position``) is not used during the partitioning.
    
    To assign reconstruction plate IDs to features using the most overlapping partitioning plate:
    ::

        plate_partitioner = pygplates.PlatePartitioner('static_polygons.gpml', 'rotations.rot')
        features = plate_partitioner.partition_features(
                'features_to_partition.gpml',
                partition_method = pygplates.PartitionMethod.most_overlapping_plate)
    
    *properties_to_copy* specifies the properties to copy from the partitioning features to the features that are being partitioned.
    
    *properties_to_copy* is a sequence whose entries are any of the :class:`PartitionProperty` values, or a :class:`PropertyName` (any property of the partitioning feature with that name is copied to the partitioned feature, provided the partitioned feature's :class:`feature type<FeatureType>` supports it).
    
    .. note:: If a property cannot copied into a feature (eg, because the property is not supported the feature's type) then that copy is silently ignored.
    
    To copy/assign reconstruction plate ID, valid time period and name from the partitioning features to their associated partitioned features:
    ::

        plate_partitioner = pygplates.PlatePartitioner('static_polygons.gpml', 'rotations.rot')
        features = plate_partitioner.partition_features(
                'features_to_partition.gpml',
                properties_to_copy = [
                    pygplates.PartitionProperty.reconstruction_plate_id,
                    pygplates.PartitionProperty.valid_time_period,
                    pygplates.PropertyName.gml_name])
    
    *properties_to_copy* can also be a single callable (function):
    
    +----------------------------------------------------+----------------------------------------------------------------------------------+
    | Type                                               | Description                                                                      |
    +====================================================+==================================================================================+
    | Arbitrary callable (function)                      | A callable accepting the following arguments:                                    |
    |                                                    |                                                                                  |
    |                                                    | - the partitioning :class:`feature<Feature>`                                     |
    |                                                    | - the :class:`feature<Feature>` being partitioned                                |
    |                                                    |                                                                                  |
    |                                                    | This can be used to write your own implementation for copying properties.        |
    +----------------------------------------------------+----------------------------------------------------------------------------------+
    
    An alternative way to copy/assign reconstruction plate ID, valid time period and name from the partitioning features to their associated partitioned features:
    ::
    
        def properties_to_copy_func(partitioning_feature, feature):
            # If a property cannot be set on the feature (eg, because not supported by feature type)
            # then don't copy that property (ie, do nothing if pygplates.InformationModelError is raised).
            try:
                feature.set_reconstruction_plate_id(partitioning_feature.get_reconstruction_plate_id())
            except pygplates.InformationModelError:
                pass
                
            begin, end = partitioning_feature.get_valid_time()
            try:
                feature.set_valid_time(begin, end)
            except pygplates.InformationModelError:
                pass
                
            try:
                feature.set_name(partitioning_feature.get_name())
            except pygplates.InformationModelError:
                pass
                
            except pygplates.InformationModelError:
                pass
        
        
        plate_partitioner = pygplates.PlatePartitioner('static_polygons.gpml', 'rotations.rot')
        features = plate_partitioner.partition_features(
                'features_to_partition.gpml',
                properties_to_copy = properties_to_copy_func)
    
    *partition_return* specifies how the features are to be partitioned by the partitioning plates. This applies regardless of the value of *partition_method*.
    
    See :class:`PartitionReturn` for the return value that each value selects.
    
    To reset the reconstruction plate ID (to zero) for all unpartitioned features (features that did not intersect any partitioning plates):
    ::

        plate_partitioner = pygplates.PlatePartitioner('static_polygons.gpml', 'rotations.rot')
        partitioned_features, unpartitioned_features = plate_partitioner.partition_features(
                'features_to_partition.gpml',
                partition_return = pygplates.PartitionReturn.separate_partitioned_and_unpartitioned)
        
        for unpartitioned_feature in unpartitioned_features:
            unpartitioned_feature.set_reconstruction_plate_id(0)
    
    ...this is useful when the features to be partitioned already have reconstruction plate IDs but
    they are deemed to be incorrect. By resetting them to zero we ensure the unpartitioned features remain stationary
    and do not reconstruct incorrectly over geological time. Any partitioned features will get a new plate ID.

    .. versionchanged:: 0.44
       Filenames can be `os.PathLike <https://docs.python.org/3/library/os.html#os.PathLike>`_ \
    (such as `pathlib.Path <https://docs.python.org/3/library/pathlib.html>`_) in addition to strings.
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
            # Partition the average sample site position.
            average_sample_site_position = feature.get_geometry(PropertyName.gpml_average_sample_site_position)
            if average_sample_site_position:
                partitioning_plate = plate_partitioner.partition_point(average_sample_site_position)
                if partitioning_plate:
                    # Clone the feature so we don't modify the original (we're returning copies).
                    partitioned_feature = feature.clone()
                    
                    # Copy the requested properties over from the partitioning feature.
                    _plate_partitioning_copy_partition_properties(
                            partitioning_plate.get_feature(), partitioned_feature, properties_to_copy)
                    
                    partitioned_features.append(partitioned_feature)
                    
                    # Also group partitioned features by partitioning plate (if requested).
                    if partition_return == PartitionReturn.partitioned_groups_and_unpartitioned:
                        partitioned_feature_groups.setdefault(partitioning_plate, []).append(partitioned_feature)
                    
                    continue
            
            # Feature does not overlap any partitions (or is missing an average sample site position)
            # so it is unpartitioned.
            unpartitioned_feature = feature.clone()
            unpartitioned_features.append(unpartitioned_feature)
            
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
            
            for partitioning_plate, geometries_inside_partition in partitioned_geometries.items():
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
                #
                # Note: It's possible for an inside polyline/polygon to have zero arc length (if all its points are coincident).
                #       So we need to start with a 'max_geometry_size_measure' that will pass 'geometry_size_measure > max_geometry_size_measure'
                #       when 'geometry_size_measure' is '(0.0, 0)' otherwise 'most_overlapping_partitioning_plate' will remain None.
                #       So we use '(0.0, -1)'.
                max_geometry_size_measure = (0.0, -1)
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
    
    # Reverse reconstruct all partitioned features (using their new plate IDs).
    # Note: Reverse reconstruct even when reconstruction time is zero
    #       (since can have non-zero finite rotation at present day, not advisable though).
    reconstruction_time = plate_partitioner._get_reconstruction_time()
    rotation_model = plate_partitioner._get_rotation_model()
    reverse_reconstruct(partitioned_features, rotation_model, reconstruction_time)
    
    # Return partitioned and unpartitioned features in the format requested by the caller.
    if partition_return == PartitionReturn.combined_partitioned_and_unpartitioned:
        return partitioned_features + unpartitioned_features
    elif partition_return == PartitionReturn.separate_partitioned_and_unpartitioned:
        return (partitioned_features, unpartitioned_features)
    else: # partition_return == PartitionReturn.partitioned_groups_and_unpartitioned
        return (list(partitioned_feature_groups.items()), unpartitioned_features)


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
    :type partitioning_features: FeatureCollection, or str, or os.PathLike, or Feature, \
        or sequence of Feature, or sequence of any combination of those four types
    
    :param rotation_model: A rotation model. Or a rotation feature collection, or a rotation filename, \
        or a rotation feature, or a sequence of rotation features, or a sequence of any combination of those four types.
    :type rotation_model: RotationModel. Or FeatureCollection, or str, or os.PathLike, \
        or Feature, or sequence of Feature, or sequence of any combination of those four types
    
    :param features_to_partition: the features to be partitioned
    :type features_to_partition: FeatureCollection, or str, or os.PathLike, or Feature, \
        or sequence of Feature, or sequence of any combination of those four types
    
    :param properties_to_copy: the properties to copy from partitioning plate features to the partitioned features \
        (defaults to just the reconstruction plate ID)
    :type properties_to_copy: sequence of any combination of PropertyName and PartitionProperty enumeration values
    
    :param reconstruction_time: the specific geological time to reconstruct/resolve the \
        *partitioning_features* to (defaults to zero)
    :type reconstruction_time: float or GeoTimeInstant
    
    :param partition_method: how the features are to be partitioned by the partitioning plates (defaults to *PartitionMethod.split_into_plates*)
    :type partition_method: PartitionMethod
    
    :param partition_return: how to return the partitioned and unpartitioned features and whether to include the partitioning plates \
        (defaults to *PartitionReturn.combined_partitioned_and_unpartitioned*)
    :type partition_return: PartitionReturn
    
    :param sort_partitioning_plates: optional sort order of partitioning plates \
        (defaults to *SortPartitioningPlates.by_partition_type_then_plate_id*)
    :type sort_partitioning_plates: SortPartitioningPlates, or None
    
    :returns: the partitioned and unpartitioned features, in the format specified by *partition_return* \
        (see :class:`PartitionReturn`) \
        (**note:** new features are always returned, never the originals passed in via *features_to_partition*)
    :rtype: list[Feature], or tuple[list[Feature], list[Feature]], or \
        tuple[list[tuple[ReconstructionGeometry, list[Feature]]], list[Feature]]
    
    The features in *features_to_partition* are tested for overlap/intersection with the partitioning plates using the partition method
    specified by *partition_method*. Properties are copied from the partitioning plate features to the
    features partitioned by them as specified by *properties_to_copy* (by default this is only the reconstruction plate ID).
    
    .. note:: New features are always returned. The original features (passed into the *features_to_partition* argument) are never modified or returned.
    
    The partitioning plates are generated internally by :func:`reconstructing any regular geological features<reconstruct>`
    and :func:`resolving any topological features<resolve_topologies>` in *partitioning_features* using the rotation model and
    optional reconstruction time.
    
    .. note:: Only those reconstructed/resolved geometries that contain a *polygon* boundary are actually used for partitioning.
       For :func:`resolved topologies<resolve_topologies>` this includes :class:`ResolvedTopologicalBoundary` and
       :class:`ResolvedTopologicalNetwork`. For :func:`reconstructed geometries<reconstruct>`, a :class:`ReconstructedFeatureGeometry`
       is only included if its reconstructed geometry is a :class:`PolygonOnSphere`.
    
    So while the partitioning polygons are reconstructed/resolved to the reconstruction time before testing for overlap/intersection,
    the geometries in the features to be partitioned (*features_to_partition*) are not since they effectively represent a snapshot of the features at the reconstruction time.
    In other words the features to be partitioned effectively contain geometry at the reconstruction time (rather than present day) and hence they are *not* reconstructed
    to the reconstruction time before testing for overlap/intersection with the partitioning plates (even if they already happen to have a reconstruction plate ID property).
    
    To partition features at present day (and assign reconstruction plate IDs) and write them to a new file:
    ::

        features = pygplates.partition_into_plates(
                'static_polygons.gpml',
                'rotations.rot',
                'features_to_partition.gpml')
        
        feature_collection = pygplates.FeatureCollection(features)
        feature_collection.write('partitioned_and_unpartitioned_features.gpml')
    
    *partition_method* specifies how the features are to be partitioned by the partitioning plates.
    
    See :class:`PartitionMethod` for a description of each value.
    
    .. note:: VirtualGeomagneticPole features (of :class:`type<FeatureType>` ``FeatureType.gpml_virtual_geomagnetic_pole``) ignore *partition_method*
       since these features are always partitioned using the average sample site position (``PropertyName.gpml_average_sample_site_position``).
       The pole position (``PropertyName.gpml_pole_position``) is not used during the partitioning.
    
    To assign reconstruction plate IDs to features using the most overlapping partitioning plate:
    ::

        features = pygplates.partition_into_plates(
                'static_polygons.gpml',
                'rotations.rot',
                'features_to_partition.gpml',
                partition_method = pygplates.PartitionMethod.most_overlapping_plate)
    
    *properties_to_copy* specifies the properties to copy from the partitioning features to the features that are being partitioned.
    
    *properties_to_copy* is a sequence whose entries are any of the :class:`PartitionProperty` values, or a :class:`PropertyName` (any property of the partitioning feature with that name is copied to the partitioned feature, provided the partitioned feature's :class:`feature type<FeatureType>` supports it).
    
    .. note:: If a property cannot copied into a feature (eg, because the property is not supported the feature's type) then that copy is silently ignored.
    
    To copy/assign reconstruction plate ID, valid time period and name from the partitioning features to their associated partitioned features:
    ::

        features = pygplates.partition_into_plates(
                'static_polygons.gpml',
                'rotations.rot',
                'features_to_partition.gpml',
                properties_to_copy = [
                    pygplates.PartitionProperty.reconstruction_plate_id,
                    pygplates.PartitionProperty.valid_time_period,
                    pygplates.PropertyName.gml_name])
    
    *properties_to_copy* can also be a single callable (function):
    
    +----------------------------------------------------+----------------------------------------------------------------------------------+
    | Type                                               | Description                                                                      |
    +====================================================+==================================================================================+
    | Arbitrary callable (function)                      | A callable accepting the following arguments:                                    |
    |                                                    |                                                                                  |
    |                                                    | - the partitioning :class:`feature<Feature>`                                     |
    |                                                    | - the :class:`feature<Feature>` being partitioned                                |
    |                                                    |                                                                                  |
    |                                                    | This can be used to write your own implementation for copying properties.        |
    +----------------------------------------------------+----------------------------------------------------------------------------------+
    
    An alternative way to copy/assign reconstruction plate ID, valid time period and name from the partitioning features to their associated partitioned features:
    ::
    
        def properties_to_copy_func(partitioning_feature, feature):
            # If a property cannot be set on the feature (eg, because not supported by feature type)
            # then don't copy that property (ie, do nothing if pygplates.InformationModelError is raised).
            try:
                feature.set_reconstruction_plate_id(partitioning_feature.get_reconstruction_plate_id())
            except pygplates.InformationModelError:
                pass
                
            begin, end = partitioning_feature.get_valid_time()
            try:
                feature.set_valid_time(begin, end)
            except pygplates.InformationModelError:
                pass
                
            try:
                feature.set_name(partitioning_feature.get_name())
            except pygplates.InformationModelError:
                pass
                
            except pygplates.InformationModelError:
                pass
        
        
        features = pygplates.partition_into_plates(
                'static_polygons.gpml',
                'rotations.rot',
                'features_to_partition.gpml',
                properties_to_copy = properties_to_copy_func)
    
    
    *partition_return* specifies how the features are to be partitioned by the partitioning plates. This applies regardless of the value of *partition_method*.
    
    See :class:`PartitionReturn` for the return value that each value selects.
    
    To reset the reconstruction plate ID (to zero) for all unpartitioned features (features that did not intersect any partitioning plates):
    ::

        partitioned_features, unpartitioned_features = pygplates.partition_into_plates(
                'static_polygons.gpml',
                'rotations.rot',
                'features_to_partition.gpml',
                partition_return = pygplates.PartitionReturn.separate_partitioned_and_unpartitioned)
        
        for unpartitioned_feature in unpartitioned_features:
            unpartitioned_feature.set_reconstruction_plate_id(0)
    
    ...this is useful when the features to be partitioned already have reconstruction plate IDs but
    they are deemed to be incorrect. By resetting them to zero we ensure the unpartitioned features remain stationary
    and do not reconstruct incorrectly over geological time. Any partitioned features will get a new plate ID.
    
    *sort_partitioning_plates* determines the order in which the partitioning plates are searched.
    See :class:`SortPartitioningPlates` for the sorting criteria of each value (and why the order matters when plates overlap).
    
    .. note:: If you don't want to sort the partitioning plates (for example, if you have already sorted them)
      then you'll need to explicitly specify ``None`` for the *sort_partitioning_plates* parameter
      (eg, ``pygplates.partition_into_plates(..., sort_partitioning_plates=None)``).
      
      This is because not specifying anything defaults to *SortPartitioningPlates.by_partition_type_then_plate_id*
      (since this always gives deterministic partitioning results).
    
    
    When partitioning many *points* into topological plates and networks, sorting by plate *area* is faster
    (and still deterministic since resolved topologies do not tend to overlap):
    ::
    
        features = pygplates.partition_into_plates(...,
            sort_partitioning_plates=pygplates.SortPartitioningPlates.by_partition_type_then_plate_area)
    
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

    .. versionchanged:: 0.44
       Filenames can be `os.PathLike <https://docs.python.org/3/library/os.html#os.PathLike>`_ \
    (such as `pathlib.Path <https://docs.python.org/3/library/pathlib.html>`_) in addition to strings.
    """
    
    plate_partitioner = PlatePartitioner(partitioning_features, rotation_model, reconstruction_time, sort_partitioning_plates)
    
    return plate_partitioner.partition_features(features_to_partition, properties_to_copy, partition_method, partition_return)
