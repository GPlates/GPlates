import pygplates


# [fragment: create-topological-model]
# Create a topological model from our topological features (plate polygons and deforming networks) and rotation file(s).
topological_model = pygplates.TopologicalModel('topologies.gpml', 'rotations.rot')
# [end: create-topological-model]

# [fragment: topological-snapshot]
# Get a snapshot of our resolved topologies at 10Ma.
time = 10
topological_snapshot = topological_model.topological_snapshot(time)
# [end: topological-snapshot]

# [fragment: resolved-networks]
# Get the resolved deforming networks (and not the resolved rigid plates).
resolved_networks = topological_snapshot.get_resolved_topologies(pygplates.ResolveTopologyType.network)
# [end: resolved-networks]

for resolved_network in resolved_networks:

    # [fragment: network-feature]
    network_feature = resolved_network.get_feature()
    print(f'Network {network_feature.get_reconstruction_plate_id()} ({network_feature.get_name()}) at {time}Ma:')
    # [end: network-feature]

    # [fragment: rigid-blocks]
    # The interior rigid blocks of the network. This is an empty list if the network has none.
    for rigid_block in resolved_network.get_rigid_blocks():
        rigid_block_feature = rigid_block.get_feature()
        # The outline of the rigid block at 10Ma, which is a polygon.
        rigid_block_polygon = rigid_block.get_reconstructed_geometry()
        rigid_block_area_in_sq_kms = rigid_block_polygon.get_area() * pygplates.Earth.mean_radius_in_kms ** 2
        print(f'  Rigid block "{rigid_block_feature.get_name()}" moves with plate '
              f'{rigid_block_feature.get_reconstruction_plate_id()} and has area {rigid_block_area_in_sq_kms:.0f} km^2')
    # [end: rigid-blocks]

    # [fragment: network-triangulation]
    # The triangulation of the network, and its vertices and triangles.
    network_triangulation = resolved_network.get_network_triangulation()
    vertices = network_triangulation.get_vertices()
    triangles = network_triangulation.get_triangles()
    # [end: network-triangulation]

    # [fragment: vertices]
    # The position and speed of each vertex.
    # Each vertex is also mapped to its index, so that a triangle can refer to its vertices by index (below).
    vertex_indices = {}
    vertex_lat_lons = []
    vertex_speeds = []
    for vertex_index, vertex in enumerate(vertices):
        vertex_indices[vertex] = vertex_index
        vertex_lat_lons.append(vertex.position.to_lat_lon())
        # The velocity of the vertex is a vector (in cms/yr here), and the speed is its magnitude.
        vertex_velocity = vertex.get_velocity(velocity_units=pygplates.VelocityUnits.cms_per_yr)
        vertex_speeds.append(vertex_velocity.get_magnitude())
    # [end: vertices]

    # [fragment: iterate-triangles]
    # The vertices (as indices into the vertex lists) and the strain rate of each triangle in the deforming region.
    triangle_vertex_indices = []
    triangle_strain_rates = []
    for triangle in triangles:
        # [end: iterate-triangles]

        # [fragment: skip-non-deforming-triangles]
        # Skip triangles that are outside the network's boundary, or inside one of its rigid blocks.
        if not triangle.is_in_deforming_region:
            continue
        # [end: skip-non-deforming-triangles]

        # [fragment: triangle-vertices-and-strain-rate]
        triangle_vertex_indices.append(
                tuple(vertex_indices[triangle.get_vertex(index)] for index in range(3)))
        # The strain rate is constant across the triangle.
        triangle_strain_rates.append(triangle.strain_rate)
        # [end: triangle-vertices-and-strain-rate]

    # [fragment: print-triangulation]
    print(f'  {len(vertices)} vertices:')
    for vertex_index, (latitude, longitude) in enumerate(vertex_lat_lons):
        print(f'    Vertex {vertex_index} at ({latitude:.2f}, {longitude:.2f}) has speed {vertex_speeds[vertex_index]:.2f} cms/yr')

    print(f'  {len(triangles)} triangles, of which {len(triangle_vertex_indices)} are in the deforming region:')
    for vertex_index_triple, triangle_strain_rate in zip(triangle_vertex_indices, triangle_strain_rates):
        print(f'    Triangle with vertices {vertex_index_triple} has dilatation rate '
              f'{triangle_strain_rate.get_dilatation_rate():.3e} 1/s and total strain rate '
              f'{triangle_strain_rate.get_total_strain_rate():.3e} 1/s')
    # [end: print-triangulation]
