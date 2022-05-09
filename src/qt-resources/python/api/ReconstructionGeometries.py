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


def get_overriding_and_subducting_plates(shared_sub_segment, return_subduction_polarity=False):
    """get_overriding_and_subducting_plates([return_subduction_polarity=False])
    Returns the overriding and subducting plates at this subduction zone.
    
    :param return_subduction_polarity: whether to also return the subduction polarity
    :type return_subduction_polarity: bool
    :returns: a 2-tuple containing the overriding and subducting resolved boundaries/networks, or
              a 3-tuple that also contains the subduction polarity (eg, 'Left', 'Right', 'Unknown') if
              *return_subduction_polarity* is ``True``, or ``None`` if the subduction polarity is not properly set or
              there are not exactly 2 topologies sharing this sub-segment
    :rtype: 2-tuple of :class:`ReconstructionGeometry`, or 3-tuple appending a str, or ``None``
    
    .. note:: Returns ``None`` if either the subduction polarity is not properly set or
       there are not exactly 2 topologies sharing the sub-segment.
    
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
                overriding_and_subducting_plates = shared_sub_segment.get_overriding_and_subducting_plates(True)
                if overriding_and_subducting_plates:
                    overriding_plate, subducting_plate, subduction_polarity = overriding_and_subducting_plates
                    overriding_plate_id = overriding_plate.get_feature().get_reconstruction_plate_id()
                    subducting_plate_id = subducting_plate.get_feature().get_reconstruction_plate_id()
    
    .. versionadded:: 0.23
    """
    
    # Get the subduction polarity of the subduction zone line.
    subduction_polarity = shared_sub_segment.get_feature().get_enumeration(PropertyName.gpml_subduction_polarity)
    if (not subduction_polarity) or (subduction_polarity == 'Unknown'):
        return None

    # There should be two sharing topologies - one is the overriding plate and the other the subducting plate.
    sharing_resolved_topologies = shared_sub_segment.get_sharing_resolved_topologies()
    if len(sharing_resolved_topologies) != 2:
        return None

    overriding_plate = None
    subducting_plate = None
    
    geometry_reversal_flags = shared_sub_segment.get_sharing_resolved_topology_geometry_reversal_flags()
    for index in range(2):

        sharing_resolved_topology = sharing_resolved_topologies[index]
        geometry_reversal_flag = geometry_reversal_flags[index]

        if sharing_resolved_topology.get_resolved_boundary().get_orientation() == PolygonOnSphere.Orientation.clockwise:
            # The current topology sharing the subducting line has clockwise orientation (when viewed from above the Earth).
            # If the overriding plate is to the 'left' of the subducting line (when following its vertices in order) and
            # the subducting line is reversed when contributing to the topology then that topology is the overriding plate.
            # A similar test applies to the 'right' but with the subducting line not reversed in the topology.
            if ((subduction_polarity == 'Left' and geometry_reversal_flag) or
                (subduction_polarity == 'Right' and not geometry_reversal_flag)):
                overriding_plate = sharing_resolved_topology
            else:
                subducting_plate = sharing_resolved_topology
        else:
            # The current topology sharing the subducting line has counter-clockwise orientation (when viewed from above the Earth).
            # If the overriding plate is to the 'left' of the subducting line (when following its vertices in order) and
            # the subducting line is not reversed when contributing to the topology then that topology is the overriding plate.
            # A similar test applies to the 'right' but with the subducting line reversed in the topology.
            if ((subduction_polarity == 'Left' and not geometry_reversal_flag) or
                (subduction_polarity == 'Right' and geometry_reversal_flag)):
                overriding_plate = sharing_resolved_topology
            else:
                subducting_plate = sharing_resolved_topology
    
    if overriding_plate is None:
        return None
    
    if subducting_plate is None:
        return None
    
    if return_subduction_polarity:
        return overriding_plate, subducting_plate, subduction_polarity
    else:
        return overriding_plate, subducting_plate

# Add the module function as a class method.
ResolvedTopologicalSharedSubSegment.get_overriding_and_subducting_plates = get_overriding_and_subducting_plates
# Delete the module reference to the function - we only keep the class method.
del get_overriding_and_subducting_plates


def get_subducting_plate(shared_sub_segment, return_subduction_polarity=False):
    """get_subducting_plate([return_subduction_polarity=False])
    Returns the subducting plate at this subduction zone.
    
    :param return_subduction_polarity: whether to also return the subduction polarity
    :type return_subduction_polarity: bool
    :returns: subducting resolved boundary/network, or
              a 2-tuple that also contains the subduction polarity (eg, 'Left', 'Right', 'Unknown') if
              *return_subduction_polarity* is ``True``, or ``None`` if the subduction polarity is not properly set or
              there is not exactly one subducting plate attached to this sub-segment
    :rtype: :class:`ReconstructionGeometry`, or 2-tuple appending a str, or ``None``
    
    .. note:: Returns ``None`` if either the subduction polarity is not properly set or
       there is not exactly one subducting plate attached to this sub-segment.
    
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
    
    .. versionadded:: 0.30
    """
    
    # Get the subduction polarity of the subducting line.
    subduction_polarity = shared_sub_segment.get_feature().get_enumeration(PropertyName.gpml_subduction_polarity)
    if (not subduction_polarity) or (subduction_polarity == 'Unknown'):
        return

    subducting_plate = None
    
    # Iterate over the resolved topologies sharing the subduction sub-segment.
    # We are looking for exactly one subducting plate.
    #
    # There can be zero, one or more overriding plates but that does not affect us (since only looking for subducting plate).
    # This actually makes things more robust because it's possible the topologies were built in such a way that a subduction line
    # is inadvertently duplicated such that the subducting plate uses one of the subduction lines as its boundary and the overriding plate
    # uses the other. In this case the subducting line attached to the subducting plate will not also be attached to the overriding plate
    # and hence there will be zero overriding plates here.
    # Another example is having two overriding plates (or at least there will be two topologies on the overriding side of the subduction line).
    # So all these overriding cases do not affect us, which means we are more likely to find the subducting plate than if we were asked to find
    # both the subducting and overriding plates. However we will still have a problem if not exactly one subducting plate is found.
    sharing_resolved_topologies = shared_sub_segment.get_sharing_resolved_topologies()
    geometry_reversal_flags = shared_sub_segment.get_sharing_resolved_topology_geometry_reversal_flags()
    for index in range(len(sharing_resolved_topologies)):

        sharing_resolved_topology = sharing_resolved_topologies[index]
        geometry_reversal_flag = geometry_reversal_flags[index]

        if sharing_resolved_topology.get_resolved_boundary().get_orientation() == PolygonOnSphere.Orientation.clockwise:
            # The current topology sharing the subducting line has clockwise orientation (when viewed from above the Earth).
            # If the overriding plate (subduction polarity) is to the 'left' of the subducting line (when following its vertices in order)
            # and the subducting line is not reversed when contributing to the topology then that topology is the subducting plate.
            # A similar test applies to the 'right' but with the subducting line reversed in the topology.
            if ((subduction_polarity == 'Left' and not geometry_reversal_flag) or
                (subduction_polarity == 'Right' and geometry_reversal_flag)):
                # If we've already previously found the subducting plate then it's ambiguous, so return None.
                if subducting_plate is not None:
                    return
                subducting_plate = sharing_resolved_topology
        else:
            # The current topology sharing the subducting line has counter-clockwise orientation (when viewed from above the Earth).
            # If the overriding plate (subduction polarity) is to the 'left' of the subducting line (when following its vertices in order)
            # and the subducting line is reversed when contributing to the topology then that topology is the subducting plate.
            # A similar test applies to the 'right' but with the subducting line not reversed in the topology.
            if ((subduction_polarity == 'Left' and geometry_reversal_flag) or
                (subduction_polarity == 'Right' and not geometry_reversal_flag)):
                # If we've already previously found the subducting plate then it's ambiguous, so return None.
                if subducting_plate is not None:
                    return
                subducting_plate = sharing_resolved_topology
    
    # Unable to find subducting plate, so return None.
    if subducting_plate is None:
        return
    
    if return_subduction_polarity:
        return subducting_plate, subduction_polarity
    else:
        return subducting_plate

# Add the module function as a class method.
ResolvedTopologicalSharedSubSegment.get_subducting_plate = get_subducting_plate
# Delete the module reference to the function - we only keep the class method.
del get_subducting_plate
