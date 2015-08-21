.. _pygplates_find_average_area_and_subducting_boundary_proportion_of_topologies:

Find average area and subducting boundary proportion of topologies
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example resolves topological plate polygons (and deforming networks) and determines:

- the average area enclosed by a topology's boundary, and
- the average proportion of a topology's boundary length that is subducting

...over a series of geological times.

Sample code
"""""""""""

::

    import pygplates


    # Load one or more rotation files into a rotation model.
    rotation_model = pygplates.RotationModel('rotations.rot')

    # Load the topological features (can be plate polygons and/or deforming networks).
    topology_features = pygplates.FeatureCollection('topologies.gpml')

    # Our geological times will be from 0Ma to 'num_time_steps' Ma (inclusive) in 1 My intervals.
    num_time_steps = 140

    # 'time' = 0, 1, 2, ... , 140
    for time in range(num_time_steps + 1):
        
        # Resolve our topological plate polygons (and deforming networks) to the current 'time'.
        resolved_topologies = []
        pygplates.resolve_topologies(topology_features, rotation_model, resolved_topologies, time)
        
        # We will accumulate the total area and subduction length proportion for the current 'time'.
        total_area = 0
        total_subduction_length_proportion = 0
        
        # Iterate over the resolved topologies.
        for resolved_topology in resolved_topologies:
            
            # Topological plate polygons and deforming networks have a boundary polygon with an area.
            total_area += resolved_topology.get_resolved_boundary().get_area()
            
            # Iterate over the boundary sub-segments of the current topology and look for subduction zone sub-segments.
            subduction_zone_length = 0
            for boundary_sub_segment in resolved_topology.get_boundary_sub_segments():
                
                # See if the current boundary sub-segment is a subduction zone.
                if boundary_sub_segment.get_resolved_feature().get_feature_type() == pygplates.FeatureType.create_gpml('SubductionZone'):
                    
                    # Each sub-segment has a polyline with a length.
                    subduction_zone_length += boundary_sub_segment.get_resolved_geometry().get_arc_length()
            
            # Calculate the proportion of the current topology's boundary length that is subducting.
            # It is the subduction zone length divided by the boundary polygon length.
            subduction_length_proportion = subduction_zone_length / resolved_topology.get_resolved_boundary().get_arc_length()
            
            # Accumulate the total subduction length proportion.
            total_subduction_length_proportion += subduction_length_proportion
        
        num_topologies = len(resolved_topologies)
        
        # The area is for a unit-length sphere so we must multiple by the Earth's radius squared.
        average_area = total_area / num_topologies
        average_area_in_sq_kms = average_area * pygplates.Earth.mean_radius_in_kms * pygplates.Earth.mean_radius_in_kms
        
        average_subduction_length_proportion = total_subduction_length_proportion / num_topologies
            
        print "At time %dMa, average topology area is %f square kms and average subduction length proportion is %f." % (
                time, average_area_in_sq_kms, average_subduction_length_proportion)


Details
"""""""

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.
::

    rotation_model = pygplates.RotationModel('rotations.rot')

The topological features are loaded into a :class:`pygplates.FeatureCollection`.
::

    topology_features = pygplates.FeatureCollection('topologies.gpml')

| The topological features are resolved to the current ``time`` using :func:`pygplates.resolve_topologies`.
| By default both :class:`pygplates.ResolvedTopologicalBoundary` (used for dynamic plate polygons) and
  :class:`pygplates.ResolvedTopologicalNetwork` (used for deforming regions) and are appended to the
  list ``resolved_topologies``.

::

    resolved_topologies = []
    pygplates.resolve_topologies(topology_features, rotation_model, resolved_topologies, time)

| The boundary polygon of a resolved topology is found by calling
  ``resolved_topology.get_resolved_boundary()`` which is available for both
  :class:`pygplates.ResolvedTopologicalBoundary` and :class:`pygplates.ResolvedTopologicalNetwork`.
| Then the area of the boundary polygon is obtained with :meth:`pygplates.PolygonOnSphere.get_area`.

::

    total_area += resolved_topology.get_resolved_boundary().get_area()

The boundary sub-segments are obtained using 
``resolved_topology.get_boundary_sub_segments()`` which is available for both
:class:`pygplates.ResolvedTopologicalBoundary` and :class:`pygplates.ResolvedTopologicalNetwork`.
::

    for boundary_sub_segment in resolved_topology.get_boundary_sub_segments():

The :meth:`feature type<pygplates.Feature.get_feature_type>` of the boundary sub-segment is checked
to see if it's a subduction zone using :meth:`pygplates.FeatureType.create_gpml`.
::

    if boundary_sub_segment.get_resolved_feature().get_feature_type() == pygplates.FeatureType.create_gpml('SubductionZone'):

The boundary sub-segment :meth:`polyline<pygplates.ResolvedTopologicalSubSegment.get_resolved_geometry>`
length is obtained using :meth:`pygplates.PolylineOnSphere.get_arc_length`.
::

    subduction_zone_length += boundary_sub_segment.get_resolved_geometry().get_arc_length()

The boundary polygon of a resolved topology also has a length (obtained using :meth:`pygplates.PolygonOnSphere.get_arc_length`).
::

    subduction_length_proportion = subduction_zone_length / resolved_topology.get_resolved_boundary().get_arc_length()

The area is for a unit-length sphere so we must multiple by the Earth's radius squared (see :class:`pygplates.Earth`).
::

    average_area_in_sq_kms = average_area * pygplates.Earth.mean_radius_in_kms * pygplates.Earth.mean_radius_in_kms

Finally the results for the current 'time' are printed.
::

    print "At time %dMa, average topology area is %f square kms and average subduction length proportion is %f." % (
            time, average_area_in_sq_kms, average_subduction_length_proportion)

...which should print something like:
::

    At time 0Ma, average topology area is 18891256.145186 square kms and average subduction length proportion is 0.357645.
    At time 1Ma, average topology area is 18891250.521188 square kms and average subduction length proportion is 0.356976.
    At time 2Ma, average topology area is 18891207.389694 square kms and average subduction length proportion is 0.352452.
    At time 3Ma, average topology area is 18891124.141200 square kms and average subduction length proportion is 0.350560.
    At time 4Ma, average topology area is 18891091.403800 square kms and average subduction length proportion is 0.344877.
    At time 5Ma, average topology area is 18890973.871916 square kms and average subduction length proportion is 0.343886.
    At time 6Ma, average topology area is 19618716.483243 square kms and average subduction length proportion is 0.330439.
    At time 7Ma, average topology area is 19618746.282826 square kms and average subduction length proportion is 0.332180.
