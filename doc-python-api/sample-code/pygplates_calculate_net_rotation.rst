.. _pygplates_calculate_net_rotation:

Calculate net rotation
^^^^^^^^^^^^^^^^^^^^^^

This example calculates the total net rotation of a topological model at each geological time over a time period.
It also calculates the individual net rotation of each topological plate polygon (and deforming network) at each time.

.. contents::
   :local:
   :depth: 2

Sample code
"""""""""""

::

    import math
    import pygplates


    # Create a topological model from our topological features (can be plate polygons and/or deforming networks)
    # and rotation file(s).
    topological_model = pygplates.TopologicalModel('topologies.gpml', 'rotations.rot')

    # Create a net rotation model from the topological model.
    # Net rotations will be calculated with a velocity delta from 'time + 1.0' to 'time'.
    net_rotation_model = pygplates.NetRotationModel(topological_model, 1.0, pygplates.VelocityDeltaTimeType.t_plus_delta_t_to_t)

    # We'll query net rotation from 410Ma to present day in 1 Myr intervals.
    max_time = 410
    time_increment = 1

    # Iterate over the time snapshots of net rotation.
    for time in range(0, max_time+1, time_increment):

        # Print the current time.
        print('Time {}:'.format(time))
        
        # Net rotation snapshot at the current 'time'.
        net_rotation_snapshot = net_rotation_model.net_rotation_snapshot(time)

        # The total net rotation over the entire globe (over all topologies) at the current 'time'.
        total_net_rotation = net_rotation_snapshot.get_total_net_rotation()

        # The magnitude of the rotation rate vector is the rotation rate in radians per million years.
        total_rotation_rate_in_radians_per_myr = total_net_rotation.get_rotation_rate_vector().get_magnitude()

        # Print the total net rotation.
        print('  Total net rotation: {} degrees/myr'.format(math.degrees(total_rotation_rate_in_radians_per_myr)))
        
        # Iterate over each resolved topology at the current time.
        for resolved_topology in net_rotation_snapshot.get_topological_snapshot().get_resolved_topologies():

            # Net rotation for the current resolved topology.
            net_rotation = net_rotation_snapshot.get_net_rotation(resolved_topology)

            # Not all resolved topologies in our topological snapshot will necessarily contribute net rotation.
            if net_rotation:

                # An alternative way to extract the rotation rate (in degrees per million years) just to demonstrate extracting
                # from a finite rotation instead of a rotation rate vector (like we did above for the total net rotation).
                _, _, rotation_rate_in_degrees_per_myr = net_rotation.get_finite_rotation().get_lat_lon_euler_pole_and_angle_degrees()

                # The area of current topology covered by the net rotation point samples (used to calculate net rotation).
                sampled_area_in_square_kms = net_rotation.get_area() * pygplates.Earth.mean_radius_in_kms**2

                # Print the current topology's net rotation and sampled area.
                print('  Topology "{}" has net rotation {} degrees/myr sampled over an area of {} square kms'.format(
                      resolved_topology.get_feature().get_name(),
                      rotation_rate_in_degrees_per_myr,
                      sampled_area_in_square_kms))


Details
"""""""

| First create a :class:`topological model<pygplates.TopologicalModel>` from topological features and rotation files.
| The topological features can be plate polygons and/or deforming networks.
| More than one file containing topological features can be specified here, however we're only specifying one file.
| Also note that more than one rotation file (or even a single :class:`pygplates.RotationModel`) can be specified here,
  however we're only specifying a single rotation file.

::

    topological_model = pygplates.TopologicalModel('topologies.gpml', 'rotations.rot')

Next we create a :class:`net rotation model<pygplates.NetRotationModel>` from the topological model. We also specify the
time interval used to calculate net rotation (in this case we have [time + 1.0, time]). We also could have specified
the distribution of points at which net rotation is calculated (see the *point_distribution* argument of
:meth:`pygplates.NetRotationModel.__init__`) but we leave it as the default which is the same as the GPlates net rotation export
(180 x 360 uniformly spaced latitude-longitude points).

::

    net_rotation_model = pygplates.NetRotationModel(topological_model, 1.0, pygplates.VelocityDeltaTimeType.t_plus_delta_t_to_t)

Next we iterate over a sequence of times and calculate a :class:`net rotation snapshot<pygplates.NetRotationSnapshot>` at each time
using the net rotation model.

::

    for time in range(0, max_time+1, time_increment):
        ...
        net_rotation_snapshot = net_rotation_model.net_rotation_snapshot(time)

Then we calculate the :meth:`total net rotation<pygplates.NetRotationSnapshot.get_total_net_rotation>` over the entire globe at the current time.
This returns a :class:`pygplates.NetRotation` object that we can use to extract the net rotation as either a
:meth:`finite rotation<pygplates.NetRotation.get_finite_rotation>` or a :meth:`rotation rate vector<pygplates.NetRotation.get_rotation_rate_vector>`.

::

    total_net_rotation = net_rotation_snapshot.get_total_net_rotation()

We arbitrarily choose to extract the net rotation as a rotation rate vector (alternatively we could have extracted it as a finite rotation).
The rotation rate vector is a :class:`3D vector<pygplates.Vector3D>` with a magnitude equal to the rotation rate in radians per million years
and a vector direction representing the rotation pole. Here we only extract the rotation rate.

::

    total_rotation_rate_in_radians_per_myr = total_net_rotation.get_rotation_rate_vector().get_magnitude()

Now that we have the *total* net rotation, we next calculate the *individual* net rotation of each resolved topology
(topological plate polygon and deforming network) at the current time. From the net rotation snapshot we obtain its associated
:meth:`topological snapshot<pygplates.NetRotationSnapshot.get_topological_snapshot>` and iterate over its
:meth:`resolved topological boundaries and networks<pygplates.TopologicalSnapshot.get_resolved_topologies>`.
For each resolved topology we retrieve its :meth:`individual net rotation<pygplates.NetRotationSnapshot.get_net_rotation>` from the net rotation snapshot.
However not all resolved topologies will necessarily contribute to net rotation. This can happen if a resolved topology did not intersect
any of the sample points used to calculate net rotation (eg, because the resolved topology was too thin and fell between the points).
It can also happen to a resolved plate boundary when it does not have a plate ID.

::

    for resolved_topology in net_rotation_snapshot.get_topological_snapshot().get_resolved_topologies():
        net_rotation = net_rotation_snapshot.get_net_rotation(resolved_topology)
        if net_rotation:
            ...

For each resolved topology that contributes net rotation we arbitrarily choose to extract its net rotation as a :class:`finite rotation<pygplates.FiniteRotation>`
(just to contrast with the total net rotation above that was extracted as a rotation rate vector, which we could have done here as well).
And again, we're ignoring the rotation pole and only extracting the rotation rate (which is the angle of the finite rotation representing the net rotation over a million years).

::

    _, _, rotation_rate_in_degrees_per_myr = net_rotation.get_finite_rotation().get_lat_lon_euler_pole_and_angle_degrees()

For each resolved topology that contributes net rotation we also query its :meth:`area<pygplates.NetRotation.get_area>` (in steradians or square radians) covered
by point samples (used to calculate net rotation). And we convert from steradians to square kilometres.

::

    sampled_area_in_square_kms = net_rotation.get_area() * pygplates.Earth.mean_radius_in_kms**2

Note that the accuracy of this area depends on how many point samples were used to calculate net rotation. If you need an accurate area then
it’s better to explicitly calculate the :meth:`polygon area<pygplates.PolygonOnSphere.get_area>` of the resolved topology.
