.. _pygplates_calculate_net_rotation:

Calculate net rotation
^^^^^^^^^^^^^^^^^^^^^^

This example calculates the total net rotation of a topological model at each geological time in
a time period. At each time it also calculates the individual net rotation of each topological
plate polygon (and deforming network).

.. contents::
   :local:
   :depth: 2

Data files
""""""""""

``topologies.gpml``
    Topological plate polygon and/or deforming network features. Each plate polygon should have a
    reconstruction plate ID: one without is treated as plate zero, and still contributes.

``rotations.rot``
    A rotation file. It must contain rotations for the plate IDs of the topological features.

Files of these kinds are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_calculate_net_rotation.py


Details
"""""""

A :class:`topological model<pygplates.TopologicalModel>` is created from the topological features
and the rotation file. The topological features can be plate polygons, deforming networks or both.

Here there is one file of topological features and one rotation file. More than one of each can be
given, and a :class:`pygplates.RotationModel` can be given in place of the rotation files.

.. sample-code:: pygplates_calculate_net_rotation.py
   :fragment: create-topological-model

A :class:`net rotation model<pygplates.NetRotationModel>` is created from the topological model.
The other two arguments give the time interval over which net rotation is calculated: here from
``time + 1.0`` to ``time``.

Net rotation is calculated at a distribution of points over the globe. The *point_distribution*
argument of :meth:`pygplates.NetRotationModel.__init__` can change it. It is left as the default
here, which is the same as the GPlates net rotation export: 180 x 360 uniformly spaced
latitude-longitude points.

.. sample-code:: pygplates_calculate_net_rotation.py
   :fragment: create-net-rotation-model

The script then iterates over a sequence of times. At each time it gets a
:class:`net rotation snapshot<pygplates.NetRotationSnapshot>` from the net rotation model.

::

    for time in range(0, max_time+1, time_increment):
        ...
        net_rotation_snapshot = net_rotation_model.net_rotation_snapshot(time)

:meth:`pygplates.NetRotationSnapshot.get_total_net_rotation` returns the *total* net rotation, over
the entire globe, at the current time.

It is a :class:`pygplates.NetRotation`. This can give the net rotation either as a
:meth:`finite rotation<pygplates.NetRotation.get_finite_rotation>` or as a
:meth:`rotation rate vector<pygplates.NetRotation.get_rotation_rate_vector>`.

.. sample-code:: pygplates_calculate_net_rotation.py
   :fragment: total-net-rotation

Here the script gets the total net rotation as a rotation rate vector (the choice is arbitrary).
This is a :class:`3D vector<pygplates.Vector3D>` whose direction is the rotation pole, and whose
magnitude is the rotation rate in radians per million years. The script uses only the rotation rate.

.. sample-code:: pygplates_calculate_net_rotation.py
   :fragment: total-rotation-rate

Next is the *individual* net rotation of each resolved topology (topological plate polygon or
deforming network) at the current time.

:meth:`pygplates.NetRotationSnapshot.get_topological_snapshot` returns the topological snapshot
that the net rotation snapshot was calculated from, and
:meth:`pygplates.TopologicalSnapshot.get_resolved_topologies` returns its resolved topological
boundaries and networks. For each of these,
:meth:`pygplates.NetRotationSnapshot.get_net_rotation` returns its net rotation.

Not every resolved topology contributes to net rotation. One that contains none of the sample
points does not (it might be so thin that it falls between the points). Its net rotation is then
``None``.

::

    for resolved_topology in net_rotation_snapshot.get_topological_snapshot().get_resolved_topologies():
        net_rotation = net_rotation_snapshot.get_net_rotation(resolved_topology)
        if net_rotation:
            ...

For each resolved topology that does contribute, the script gets its net rotation as a
:class:`finite rotation<pygplates.FiniteRotation>`. This is only to contrast with the total net
rotation above. Either could have been a finite rotation or a rotation rate vector.

The finite rotation is the net rotation over one million years, so its angle is the rotation rate.
Again the script uses only the rotation rate, and ignores the rotation pole.

.. sample-code:: pygplates_calculate_net_rotation.py
   :fragment: rotation-rate

The script also gets the :meth:`area<pygplates.NetRotation.get_area>` of each of those resolved
topologies. This is the area covered by the sample points that are inside the topology, in
steradians (square radians). Multiplying it by the square of the Earth's radius converts it to
square kilometres.

.. sample-code:: pygplates_calculate_net_rotation.py
   :fragment: sampled-area

The accuracy of this area depends on how many sample points were used to calculate net rotation.
If an accurate area is needed, it is better to calculate the
:meth:`polygon area<pygplates.PolygonOnSphere.get_area>` of the resolved topology.

See also
""""""""

- Primer: :ref:`pygplates_primer_topological_model`
- Reference: :class:`pygplates.NetRotationModel`, :class:`pygplates.NetRotationSnapshot`, :class:`pygplates.NetRotation`,
  :class:`pygplates.TopologicalModel`
- Sample code: :ref:`pygplates_calculate_net_rotation_from_point_samples`,
  :ref:`pygplates_find_average_area_and_subducting_boundary_proportion_of_topologies`,
  :ref:`pygplates_calculate_velocities_in_dynamic_plates`
