import pygplates


# [fragment: rift-plates]
# The plate IDs of the two plates that the rift is between (here South America and Africa).
rift_left_plate_id = 201
rift_right_plate_id = 701
# [end: rift-plates]

# [fragment: load-features]
# Load the topological features: the deforming networks of the rift, and the features their boundaries are built from.
topological_features = pygplates.FeatureCollection('topologies.gpml')
# Load the rotations.
rotation_model = pygplates.RotationModel('rotations.rot')
# [end: load-features]

# [fragment: iterate-features]
# Make a second list of the features, in which each deforming network is replaced by a copy that has rift parameters.
rift_topological_features = []
for feature in topological_features:
    # [end: iterate-features]

    # [fragment: other-features]
    # A feature that is not a deforming network goes into the second list as it is.
    if feature.get_feature_type() != pygplates.FeatureType.gpml_topological_network:
        rift_topological_features.append(feature)
        continue
    # [end: other-features]

    # [fragment: create-rift-network]
    # Create a network feature with the same topological geometry, name and valid time as the deforming network,
    # and with the plate IDs of the rift.
    rift_network_feature = pygplates.Feature.create_topological_network_feature(
            feature.get_topological_geometry(),
            name=feature.get_name(),
            valid_time=feature.get_valid_time(),
            rift_parameters=(rift_left_plate_id, rift_right_plate_id))
    rift_network_feature.set_reconstruction_plate_id(feature.get_reconstruction_plate_id())

    rift_topological_features.append(rift_network_feature)
    # [end: create-rift-network]

# [fragment: topological-models]
# Three topological models of the rift, each with a name to print.
topological_models = {

    # The networks as they were loaded, without rift parameters.
    'without rift parameters': pygplates.TopologicalModel(topological_features, rotation_model),

    # The networks that have rift parameters.
    'with rift parameters': pygplates.TopologicalModel(rift_topological_features, rotation_model),

    # The same, but with an exponential stretching constant of 2 (the default is 1).
    'with rift parameters and a stretching constant of 2': pygplates.TopologicalModel(
            rift_topological_features,
            rotation_model,
            default_resolve_topology_parameters=pygplates.ResolveTopologyParameters(
                    rift_exponential_stretching_constant=2.0)),
}
# [end: topological-models]

# [fragment: profile-points]
# Points along a profile across one side of the rift at 122Ma, as (latitude, longitude).
# The profile runs from the un-stretched margin of South America (at longitude 3W) to the rift axis (at longitude 0).
time = 122
profile_points = [(-25, -2.75), (-25, -2.25), (-25, -1.75), (-25, -1.25), (-25, -0.75), (-25, -0.25)]
# [end: profile-points]

for name, topological_model in topological_models.items():

    # Get a snapshot of the topologies resolved at 122Ma.
    topological_snapshot = topological_model.topological_snapshot(time)

    # [fragment: count-triangles]
    # The total number of triangles in the triangulations of the deforming networks.
    num_triangles = 0
    for resolved_network in topological_snapshot.get_resolved_topologies(pygplates.ResolveTopologyType.network):
        num_triangles += len(resolved_network.get_network_triangulation().get_triangles())
    # [end: count-triangles]

    # [fragment: profile-strain-rates]
    # The strain rate at each point of the profile.
    profile_strain_rates = topological_snapshot.get_point_strain_rates(profile_points)
    # [end: profile-strain-rates]

    # [fragment: print-profile]
    print(f'{name}: the networks have {num_triangles} triangles')
    for (latitude, longitude), strain_rate in zip(profile_points, profile_strain_rates):
        # A point that is outside all the resolved plates (and deforming networks) has no strain rate.
        if strain_rate is not None:
            print(f'  dilatation rate at ({latitude}, {longitude}): {strain_rate.get_dilatation_rate():.3e} 1/s')
    # [end: print-profile]
