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

.. sample-code:: pygplates_calculate_net_rotation.py


Details
"""""""

| First create a :class:`topological model<pygplates.TopologicalModel>` from topological features and rotation files.
| The topological features can be plate polygons and/or deforming networks.
| More than one file containing topological features can be specified here, however we're only specifying one file.
| Also note that more than one rotation file (or even a single :class:`pygplates.RotationModel`) can be specified here,
  however we're only specifying a single rotation file.

.. sample-code:: pygplates_calculate_net_rotation.py
   :fragment: create-topological-model

Next we create a :class:`net rotation model<pygplates.NetRotationModel>` from the topological model. We also specify the
time interval used to calculate net rotation (in this case we have [time + 1.0, time]). We also could have specified
the distribution of points at which net rotation is calculated (see the *point_distribution* argument of
:meth:`pygplates.NetRotationModel.__init__`) but we leave it as the default which is the same as the GPlates net rotation export
(180 x 360 uniformly spaced latitude-longitude points).

.. sample-code:: pygplates_calculate_net_rotation.py
   :fragment: create-net-rotation-model

Next we iterate over a sequence of times and calculate a :class:`net rotation snapshot<pygplates.NetRotationSnapshot>` at each time
using the net rotation model.

::

    for time in range(0, max_time+1, time_increment):
        ...
        net_rotation_snapshot = net_rotation_model.net_rotation_snapshot(time)

Then we calculate the :meth:`total net rotation<pygplates.NetRotationSnapshot.get_total_net_rotation>` over the entire globe at the current time.
This returns a :class:`pygplates.NetRotation` object that we can use to extract the net rotation as either a
:meth:`finite rotation<pygplates.NetRotation.get_finite_rotation>` or a :meth:`rotation rate vector<pygplates.NetRotation.get_rotation_rate_vector>`.

.. sample-code:: pygplates_calculate_net_rotation.py
   :fragment: total-net-rotation

We arbitrarily choose to extract the net rotation as a rotation rate vector (alternatively we could have extracted it as a finite rotation).
The rotation rate vector is a :class:`3D vector<pygplates.Vector3D>` with a magnitude equal to the rotation rate in radians per million years
and a vector direction representing the rotation pole. Here we only extract the rotation rate.

.. sample-code:: pygplates_calculate_net_rotation.py
   :fragment: total-rotation-rate

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

.. sample-code:: pygplates_calculate_net_rotation.py
   :fragment: rotation-rate

For each resolved topology that contributes net rotation we also query its :meth:`area<pygplates.NetRotation.get_area>` (in steradians or square radians) covered
by point samples (used to calculate net rotation). And we convert from steradians to square kilometres.

.. sample-code:: pygplates_calculate_net_rotation.py
   :fragment: sampled-area

Note that the accuracy of this area depends on how many point samples were used to calculate net rotation. If you need an accurate area then
it’s better to explicitly calculate the :meth:`polygon area<pygplates.PolygonOnSphere.get_area>` of the resolved topology.
