import math
import pygplates


# [fragment: create-topological-model]
# Create a topological model from our topological features (can be plate polygons and/or deforming networks)
# and rotation file(s).
topological_model = pygplates.TopologicalModel('topologies.gpml', 'rotations.rot')
# [end: create-topological-model]

# [fragment: create-net-rotation-model]
# Create a net rotation model from the topological model.
# Net rotations will be calculated with a velocity delta from 'time + 1.0' to 'time'.
net_rotation_model = pygplates.NetRotationModel(topological_model, 1.0, pygplates.VelocityDeltaTimeType.t_plus_delta_t_to_t)
# [end: create-net-rotation-model]

# We'll query net rotation from 410Ma to present day in 1 Myr intervals.
max_time = 410
time_increment = 1

# Iterate over the time snapshots of net rotation.
for time in range(0, max_time+1, time_increment):

    # Print the current time.
    print(f'Time {time}:')

    # Net rotation snapshot at the current 'time'.
    net_rotation_snapshot = net_rotation_model.net_rotation_snapshot(time)

    # [fragment: total-net-rotation]
    # The total net rotation over the entire globe (over all topologies) at the current 'time'.
    total_net_rotation = net_rotation_snapshot.get_total_net_rotation()
    # [end: total-net-rotation]

    # [fragment: total-rotation-rate]
    # The magnitude of the rotation rate vector is the rotation rate in radians per million years.
    total_rotation_rate_in_radians_per_myr = total_net_rotation.get_rotation_rate_vector().get_magnitude()
    # [end: total-rotation-rate]

    # Print the total net rotation.
    print(f'  Total net rotation: {math.degrees(total_rotation_rate_in_radians_per_myr)} degrees/myr')

    # Iterate over each resolved topology at the current time.
    for resolved_topology in net_rotation_snapshot.get_topological_snapshot().get_resolved_topologies():

        # Net rotation for the current resolved topology.
        net_rotation = net_rotation_snapshot.get_net_rotation(resolved_topology)

        # Not all resolved topologies in our topological snapshot will necessarily contribute net rotation.
        if net_rotation:

            # [fragment: rotation-rate]
            # An alternative way to extract the rotation rate (in degrees per million years) just to demonstrate extracting
            # from a finite rotation instead of a rotation rate vector (like we did above for the total net rotation).
            _, _, rotation_rate_in_degrees_per_myr = net_rotation.get_finite_rotation().get_lat_lon_euler_pole_and_angle_degrees()
            # [end: rotation-rate]

            # [fragment: sampled-area]
            # The area of current topology covered by the net rotation point samples (used to calculate net rotation).
            sampled_area_in_square_kms = net_rotation.get_area() * pygplates.Earth.mean_radius_in_kms**2
            # [end: sampled-area]

            # Print the current topology's net rotation and sampled area.
            print(f'  Topology "{resolved_topology.get_feature().get_name()}" has net rotation '
                  f'{rotation_rate_in_degrees_per_myr} degrees/myr sampled over an area of '
                  f'{sampled_area_in_square_kms} square kms')
