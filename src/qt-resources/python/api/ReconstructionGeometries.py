# Copyright (C) 2019 The University of Sydney, Australia
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


def get_overriding_and_subducting_plates(
        shared_sub_segment,
        return_subduction_polarity=False,
        enforce_single_plates=True):
    """get_overriding_and_subducting_plates([return_subduction_polarity=False], [enforce_single_plates=True])
    Returns the overriding and subducting plates at this subduction zone.
    
    :param return_subduction_polarity: whether to also return the subduction polarity (defaults to ``False``)
    :type return_subduction_polarity: bool
    :param enforce_single_plates: whether to require a *single* overriding plate/network and a *single* subducting plate/network (defaults to ``True``)
    :type enforce_single_plates: bool
    :returns: a 2-tuple containing the overriding and subducting resolved
              :class:`boundaries <ResolvedTopologicalBoundary>`/:class:`networks <ResolvedTopologicalNetwork>`,
              or a 3-tuple that also contains the subduction polarity ('Left' or 'Right') if
              *return_subduction_polarity* is ``True``, or ``None`` if the subduction polarity is not 'Left' or 'Right'
              (or doesn't exist), or ``None`` if *enforce_single_plates* is ``True`` and there is not exactly one overriding plate or
              one overriding network or one overriding plate and network attached to this sub-segment, or ``None`` if
              *enforce_single_plates* is ``True`` and there is not exactly one subducting plate or one subducting network or one
              subducting plate and network attached to this sub-segment
    :rtype: tuple[ReconstructionGeometry | None, ReconstructionGeometry | None], or \
        tuple[ReconstructionGeometry | None, ReconstructionGeometry | None, str], or None
    
    .. note:: If there is an overriding plate and an overriding network attached to this sub-segment then the
       overriding network is returned (since networks overlay plates). The same applies to *subducting* plates and networks.
    
    .. note:: If *enforce_single_plates* is ``False``, then ``None`` could be returned for the overriding plate/network (if none are found), and
       if more than one overriding plate (or more than one overriding network) is found then it is arbitrary which overriding plate (or network)
       is returned. The same applies to *subducting* plates and networks. This also means it's possible that ``None`` could be returned for
       *both* the overriding and subducting plates.
    
    .. note:: This method does not require the feature type (of this sub-segment) to be ``pygplates.FeatureType.gpml_subduction_zone``.
       It only requires a ``pygplates.PropertyName.gpml_subduction_polarity`` feature property.
    
    To find the overriding and subducting plate IDs of all subduction zone lines:
    ::
    
        # Resolve our topological plate polygons (and deforming networks) to the current 'time'.
        # We generate both the resolved topology boundaries and the boundary sections between them.
        resolved_topologies = []
        shared_boundary_sections = []
        pygplates.resolve_topologies(topology_features, rotation_model, resolved_topologies, time, shared_boundary_sections)
        
        # Iterate over the shared boundary sections of all resolved topologies.
        for shared_boundary_section in shared_boundary_sections:
        
            # Skip sections that are not subduction zones.
            if shared_boundary_section.get_feature().get_feature_type() != pygplates.FeatureType.gpml_subduction_zone:
                continue
            
            # Iterate over the shared sub-segments of the current subducting line.
            # These are the parts of the subducting line that actually contribute to topological boundaries.
            for shared_sub_segment in shared_boundary_section.get_shared_sub_segments():
            
                # Get the overriding and subducting resolved plates/networks on either side of the current shared sub-segment.
                overriding_and_subducting_plates = shared_sub_segment.get_overriding_and_subducting_plates(
                        return_subduction_polarity=True)
                if overriding_and_subducting_plates:
                    overriding_plate, subducting_plate, subduction_polarity = overriding_and_subducting_plates
                    overriding_plate_id = overriding_plate.get_feature().get_reconstruction_plate_id()
                    subducting_plate_id = subducting_plate.get_feature().get_reconstruction_plate_id()
    
    .. seealso:: :meth:`get_overriding_plate` and :meth:`get_subducting_plate`
    
    .. versionadded:: 0.23

    .. versionchanged:: 0.50

       * Allow one overriding plate and one overriding network (latter is then returned).
       * Allow one subducting plate and one subducting network (latter is then returned).
       * Added *enforce_single_plates* argument.
    """
    # Get the subduction polarity of the subduction zone line.
    subduction_polarity = shared_sub_segment.get_feature().get_enumeration(PropertyName.gpml_subduction_polarity)
    if not subduction_polarity:
        return
    
    # Ensure the subduction polarity is known (and set properly).
    if subduction_polarity == 'Left':
        overriding_plate_is_on_left = True
    elif subduction_polarity == 'Right':
        overriding_plate_is_on_left = False
    else:
        return

    # Can have a resolved topological network that overlays a resolved topological boundary.
    overriding_boundary = None
    overriding_network = None
    subducting_boundary = None
    subducting_network = None
    
    # Iterate over the resolved topologies sharing the subduction sub-segment.
    # We are looking for exactly one overriding plate, or one overriding network, or one overriding plate and network.
    # And also looking for exactly one subducting plate, or one subducting network, or one subducting plate and network.
    sharing_resolved_topologies = shared_sub_segment.get_sharing_resolved_topologies()
    resolved_topology_on_left_flags = shared_sub_segment.get_sharing_resolved_topology_on_left_flags()
    for index in range(len(sharing_resolved_topologies)):

        sharing_resolved_topology = sharing_resolved_topologies[index]
        resolved_topology_is_on_left = resolved_topology_on_left_flags[index]

        # If the current topology is on the same side of the subduction polarity then it's the overriding plate
        # (otherwise it's the subducting plate).
        if ((resolved_topology_is_on_left and overriding_plate_is_on_left) or
            (not resolved_topology_is_on_left and not overriding_plate_is_on_left)):
            if isinstance(sharing_resolved_topology, ResolvedTopologicalBoundary):
                if enforce_single_plates and overriding_boundary:
                    # Return None if previously found a ResolvedTopologicalBoundary (since it's ambiguous).
                    return
                overriding_boundary = sharing_resolved_topology
            else:  # ResolvedTopologicalNetwork...
                if enforce_single_plates and overriding_network:
                    # Return None if previously found a ResolvedTopologicalNetwork (since it's ambiguous).
                    return
                overriding_network = sharing_resolved_topology
        else:
            if isinstance(sharing_resolved_topology, ResolvedTopologicalBoundary):
                if enforce_single_plates and subducting_boundary:
                    # Return None if previously found a ResolvedTopologicalBoundary (since it's ambiguous).
                    return
                subducting_boundary = sharing_resolved_topology
            else:  # ResolvedTopologicalNetwork...
                if enforce_single_plates and subducting_network:
                    # Return None if previously found a ResolvedTopologicalNetwork (since it's ambiguous).
                    return
                subducting_network = sharing_resolved_topology

    # Resolved topological networks will get higher preference than resolved topological boundaries
    # (since former can overlay the latter).
    #
    if overriding_network:
        overriding_plate = overriding_network
    else:
        overriding_plate = overriding_boundary  # can be None
    #
    if subducting_network:
        subducting_plate = subducting_network
    else:
        subducting_plate = subducting_boundary  # can be None
    
    if enforce_single_plates:
        # If unable to find overriding plate (boundary/network) AND subducting plate (boundary/network) then return None.
        if not (overriding_plate and subducting_plate):
            return
    
    if return_subduction_polarity:
        return overriding_plate, subducting_plate, subduction_polarity
    else:
        return overriding_plate, subducting_plate

# Add the module function as a class method.
ResolvedTopologicalSharedSubSegment.get_overriding_and_subducting_plates = get_overriding_and_subducting_plates
# Delete the module reference to the function - we only keep the class method.
del get_overriding_and_subducting_plates


def get_overriding_plate(
        shared_sub_segment,
        return_subduction_polarity=False,
        enforce_single_plate=True):
    """get_overriding_plate([return_subduction_polarity=False], [enforce_single_plate=True])
    Returns the overriding plate (or network) at this subduction zone.
    
    :param return_subduction_polarity: whether to also return the subduction polarity
    :type return_subduction_polarity: bool
    :param enforce_single_plate: whether to require a *single* overriding plate/network (defaults to ``True``)
    :type enforce_single_plate: bool
    :returns: overriding resolved :class:`boundary <ResolvedTopologicalBoundary>`/:class:`network <ResolvedTopologicalNetwork>`,
              or a 2-tuple that also contains the subduction polarity ('Left' or 'Right') if *return_subduction_polarity*
              is ``True``, or ``None`` if the subduction polarity is not 'Left' or 'Right' (or doesn't exist), or ``None`` if
              *enforce_single_plate* is ``True`` and there is not exactly one overriding plate or one overriding network or
              one overriding plate and network attached to this sub-segment
    :rtype: ReconstructionGeometry, or tuple[ReconstructionGeometry | None, str], or None

    .. note:: If there is an overriding plate and an overriding network attached to this sub-segment then the
       overriding network is returned (since networks overlay plates).
    
    .. note:: If *enforce_single_plate* is ``False``, then ``None`` could be returned for the overriding plate/network (if none are found), and
       if more than one overriding plate (or more than one overriding network) is found then it is arbitrary which overriding plate (or network)
       is returned. This means if *return_subduction_polarity* is ``True`` then it's possible a 2-tuple is returned with ``None`` for the
       overriding plate and a valid value for the subduction polarity.

    .. note:: The number of *subducting* plates and networks is not considered (only overriding plates/networks are considered). In other words,
       if *enforce_single_plate* is ``True``, it is *not* required to have exactly one subducting plate or one subducting network or one
       subducting plate and network attached to this sub-segment.
    
    .. note:: This method does not require the feature type (of this sub-segment) to be ``pygplates.FeatureType.gpml_subduction_zone``.
       It only requires a ``pygplates.PropertyName.gpml_subduction_polarity`` feature property.
    
    To find the plate ID of each overriding plate attached to each subduction zone line sub-segment:
    ::
    
        # Resolve our topological plate polygons (and deforming networks) to the current 'time'.
        # We generate both the resolved topology boundaries and the boundary sections between them.
        resolved_topologies = []
        shared_boundary_sections = []
        pygplates.resolve_topologies(topology_features, rotation_model, resolved_topologies, time, shared_boundary_sections)
        
        # Iterate over the shared boundary sections of all resolved topologies.
        for shared_boundary_section in shared_boundary_sections:
        
            # Skip sections that are not subduction zones.
            if shared_boundary_section.get_feature().get_feature_type() != pygplates.FeatureType.gpml_subduction_zone:
                continue
            
            # Iterate over the shared sub-segments of the current subducting line.
            # These are the parts of the subducting line that actually contribute to topological boundaries.
            for shared_sub_segment in shared_boundary_section.get_shared_sub_segments():
            
                # Get the overriding resolved plate/network of the current shared sub-segment.
                overriding_plate_and_polarity = shared_sub_segment.get_overriding_plate(True)
                if overriding_plate_and_polarity:
                    overriding_plate, subduction_polarity = overriding_plate_and_polarity
                    overriding_plate_id = overriding_plate.get_feature().get_reconstruction_plate_id()
    
    .. seealso:: :meth:`get_subducting_plate` and :meth:`get_overriding_and_subducting_plates`
    
    .. versionadded:: 0.50
    """
    
    # Get the subduction polarity of the subducting line.
    subduction_polarity = shared_sub_segment.get_feature().get_enumeration(PropertyName.gpml_subduction_polarity)
    if not subduction_polarity:
        return
    
    # Ensure the subduction polarity is known (and set properly).
    if subduction_polarity == 'Left':
        overriding_plate_is_on_left = True
    elif subduction_polarity == 'Right':
        overriding_plate_is_on_left = False
    else:
        return

    # Can have a resolved topological network that overlays a resolved topological boundary.
    overriding_boundary = None
    overriding_network = None
    
    # Iterate over the resolved topologies sharing the subduction sub-segment.
    # We are looking for exactly one overriding plate, or one overriding network, or one overriding plate and network.
    #
    # Note: There can be zero, one or more subducting plates/networks but that does not affect us (since only looking for overriding plate).
    sharing_resolved_topologies = shared_sub_segment.get_sharing_resolved_topologies()
    resolved_topology_on_left_flags = shared_sub_segment.get_sharing_resolved_topology_on_left_flags()
    for index in range(len(sharing_resolved_topologies)):

        sharing_resolved_topology = sharing_resolved_topologies[index]
        resolved_topology_is_on_left = resolved_topology_on_left_flags[index]

        # If the current topology is on the same side of the subduction polarity then it's the overriding plate.
        if ((resolved_topology_is_on_left and overriding_plate_is_on_left) or
            (not resolved_topology_is_on_left and not overriding_plate_is_on_left)):

            if isinstance(sharing_resolved_topology, ResolvedTopologicalBoundary):
                if enforce_single_plate and overriding_boundary:
                    # Return None if previously found a ResolvedTopologicalBoundary (since it's ambiguous).
                    return
                overriding_boundary = sharing_resolved_topology
            else:  # ResolvedTopologicalNetwork...
                if enforce_single_plate and overriding_network:
                    # Return None if previously found a ResolvedTopologicalNetwork (since it's ambiguous).
                    return
                overriding_network = sharing_resolved_topology

    # Resolved topological networks will get higher preference than resolved topological boundaries
    # (since former can overlay the latter).
    if overriding_network:
        overriding_plate = overriding_network
    else:
        overriding_plate = overriding_boundary  # can be None
    
    if enforce_single_plate:
        # If unable to find overriding plate (boundary/network) then return None.
        if not overriding_plate:
            return
    
    if return_subduction_polarity:
        return overriding_plate, subduction_polarity
    else:
        return overriding_plate

# Add the module function as a class method.
ResolvedTopologicalSharedSubSegment.get_overriding_plate = get_overriding_plate
# Delete the module reference to the function - we only keep the class method.
del get_overriding_plate


def get_subducting_plate(
        shared_sub_segment,
        return_subduction_polarity=False,
        enforce_single_plate=True):
    """get_subducting_plate([return_subduction_polarity=False], [enforce_single_plate=True])
    Returns the subducting plate (or network) at this subduction zone.
    
    :param return_subduction_polarity: whether to also return the subduction polarity
    :type return_subduction_polarity: bool
    :param enforce_single_plate: whether to require a *single* subducting plate/network (defaults to ``True``)
    :type enforce_single_plate: bool
    :returns: subducting resolved :class:`boundary <ResolvedTopologicalBoundary>`/:class:`network <ResolvedTopologicalNetwork>`,
              or a 2-tuple that also contains the subduction polarity ('Left' or 'Right') if *return_subduction_polarity*
              is ``True``, or ``None`` if the subduction polarity is not 'Left' or 'Right' (or doesn't exist), or ``None`` if
              *enforce_single_plate* is ``True`` and there is not exactly one subducting plate or one subducting network or
              one subducting plate and network attached to this sub-segment
    :rtype: ReconstructionGeometry, or tuple[ReconstructionGeometry | None, str], or None
    
    .. note:: If there is a subducting plate and a subducting network attached to this sub-segment then the
       subducting network is returned (since networks overlay plates).
    
    .. note:: If *enforce_single_plate* is ``False``, then ``None`` could be returned for the subducting plate/network (if none are found), and
       if more than one subducting plate (or more than one subducting network) is found then it is arbitrary which subducting plate (or network)
       is returned. This means if *return_subduction_polarity* is ``True`` then it's possible a 2-tuple is returned with ``None`` for the
       subducting plate and a valid value for the subduction polarity.

    .. note:: The number of *overriding* plates and networks is not considered (only subducting plates/networks are considered). In other words,
       if *enforce_single_plate* is ``True``, it is *not* required to have exactly one overriding plate or one overriding network or one
       overriding plate and network attached to this sub-segment.
    
    .. note:: This method does not require the feature type (of this sub-segment) to be ``pygplates.FeatureType.gpml_subduction_zone``.
       It only requires a ``pygplates.PropertyName.gpml_subduction_polarity`` feature property.
    
    To find the plate ID of each subducting plate attached to each subduction zone line sub-segment:
    ::
    
        # Resolve our topological plate polygons (and deforming networks) to the current 'time'.
        # We generate both the resolved topology boundaries and the boundary sections between them.
        resolved_topologies = []
        shared_boundary_sections = []
        pygplates.resolve_topologies(topology_features, rotation_model, resolved_topologies, time, shared_boundary_sections)
        
        # Iterate over the shared boundary sections of all resolved topologies.
        for shared_boundary_section in shared_boundary_sections:
        
            # Skip sections that are not subduction zones.
            if shared_boundary_section.get_feature().get_feature_type() != pygplates.FeatureType.gpml_subduction_zone:
                continue
            
            # Iterate over the shared sub-segments of the current subducting line.
            # These are the parts of the subducting line that actually contribute to topological boundaries.
            for shared_sub_segment in shared_boundary_section.get_shared_sub_segments():
            
                # Get the subducting resolved plate/network of the current shared sub-segment.
                subducting_plate_and_polarity = shared_sub_segment.get_subducting_plate(True)
                if subducting_plate_and_polarity:
                    subducting_plate, subduction_polarity = subducting_plate_and_polarity
                    subducting_plate_id = subducting_plate.get_feature().get_reconstruction_plate_id()
    
    .. seealso:: :meth:`get_overriding_plate` and :meth:`get_overriding_and_subducting_plates`
    
    .. versionadded:: 0.30

    .. versionchanged:: 0.50

       * Allow one subducting plate and one subducting network (latter is then returned).
       * Added *enforce_single_plate* argument.
    """
    
    # Get the subduction polarity of the subducting line.
    subduction_polarity = shared_sub_segment.get_feature().get_enumeration(PropertyName.gpml_subduction_polarity)
    if not subduction_polarity:
        return
    
    # Ensure the subduction polarity is known (and set properly).
    if subduction_polarity == 'Left':
        overriding_plate_is_on_left = True
    elif subduction_polarity == 'Right':
        overriding_plate_is_on_left = False
    else:
        return

    # Can have a resolved topological network that overlays a resolved topological boundary.
    subducting_boundary = None
    subducting_network = None
    
    # Iterate over the resolved topologies sharing the subduction sub-segment.
    # We are looking for exactly one subducting plate, or one subducting network, or one subducting plate and network.
    #
    # Note: There can be zero, one or more overriding plates/networks but that does not affect us (since only looking for subducting plate).
    sharing_resolved_topologies = shared_sub_segment.get_sharing_resolved_topologies()
    resolved_topology_on_left_flags = shared_sub_segment.get_sharing_resolved_topology_on_left_flags()
    for index in range(len(sharing_resolved_topologies)):

        sharing_resolved_topology = sharing_resolved_topologies[index]
        resolved_topology_is_on_left = resolved_topology_on_left_flags[index]

        # If the current topology is on the opposite side of the subduction polarity (overriding plate) then it's the subducting plate.
        if ((resolved_topology_is_on_left and not overriding_plate_is_on_left) or
            (not resolved_topology_is_on_left and overriding_plate_is_on_left)):

            if isinstance(sharing_resolved_topology, ResolvedTopologicalBoundary):
                if enforce_single_plate and subducting_boundary:
                    # Return None if previously found a ResolvedTopologicalBoundary (since it's ambiguous).
                    return
                subducting_boundary = sharing_resolved_topology
            else:  # ResolvedTopologicalNetwork...
                if enforce_single_plate and subducting_network:
                    # Return None if previously found a ResolvedTopologicalNetwork (since it's ambiguous).
                    return
                subducting_network = sharing_resolved_topology

    # Resolved topological networks will get higher preference than resolved topological boundaries
    # (since former can overlay the latter).
    if subducting_network:
        subducting_plate = subducting_network
    else:
        subducting_plate = subducting_boundary  # can be None
    
    if enforce_single_plate:
        # If unable to find subducting plate (boundary/network) then return None.
        if not subducting_plate:
            return
    
    if return_subduction_polarity:
        return subducting_plate, subduction_polarity
    else:
        return subducting_plate

# Add the module function as a class method.
ResolvedTopologicalSharedSubSegment.get_subducting_plate = get_subducting_plate
# Delete the module reference to the function - we only keep the class method.
del get_subducting_plate
