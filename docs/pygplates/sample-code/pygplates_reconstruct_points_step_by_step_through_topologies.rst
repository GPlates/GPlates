.. _pygplates_reconstruct_points_step_by_step_through_topologies:

Reconstruct points step by step through topologies
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example reconstructs points through topological plate polygons and deforming networks one time
step at a time, from a past geological time to present day.

At each time the topologies are resolved, and :meth:`pygplates.TopologicalSnapshot.reconstruct_points`
(added in pyGPlates 1.1) moves each point to where it is one time step later. A point in a rigid
plate is rotated with the plate, and a point in a deforming network is deformed with the network.

The script holds the points between one step and the next, so it can do what it likes with them there.

The first example reconstructs all the points together at each step. The second reconstructs each
point using the plate or network that contains it, and reports when a point passes from one plate
or network to another.

.. contents::
   :local:
   :depth: 2

Step by step, or over a time span
+++++++++++++++++++++++++++++++++

:meth:`pygplates.TopologicalModel.reconstruct_geometry` reconstructs points through the same
topologies in the same way, but does all the time steps in one call. It is what
:ref:`pygplates_reconstruct_strain_and_strain_rate` and
:ref:`pygplates_reconstruct_crustal_thickness_and_tectonic_subsidence` use.

- Use :meth:`pygplates.TopologicalModel.reconstruct_geometry` when all the points are known at the
  start and their history is wanted. What it returns gives the position of every point at every time
  step, and its velocity, strain rate, accumulated strain and scalar values (such as crustal
  thickness and tectonic subsidence). It also deactivates points, such as those on oceanic crust that
  are subducted.
- Reconstruct step by step, as on this page, when the script has to act between the steps. For example,
  to add points along the way (such as new crust at a mid-ocean ridge), to look at which plate each
  point is in before moving it, or to keep only the current positions of the points rather than
  their history.

Reconstructing step by step never deactivates a point, and accumulates nothing: a step gives
positions and no more. (The strain rate at each position can be found separately, as in
:ref:`pygplates_find_plates_and_strain_rates_at_static_points`.)

Data files
++++++++++

Both scripts read the same files:

``topologies.gpml``
    Topological plate polygon and deforming network features. A point moves only while it is inside
    one of them.

``rotations.rot``
    A rotation file. It must contain rotations for the plate IDs of the topological features, and
    of the features that the boundaries of the deforming networks are built from.

Files of these kinds are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Reconstruct points one time step at a time
++++++++++++++++++++++++++++++++++++++++++

Sample code
"""""""""""

.. sample-code:: pygplates_reconstruct_points_step_by_step_through_topologies.py

Details
"""""""

A :class:`topological model<pygplates.TopologicalModel>` is created from the topological features and the rotation file.

The topological features are not resolved here. That happens at each time, in the loop below.

.. sample-code:: pygplates_reconstruct_points_step_by_step_through_topologies.py
   :fragment: create-topological-model

The points are given as (latitude, longitude) tuples. Each is where a point is at the initial time,
not at present day.

Any of the forms of point that :meth:`pygplates.TopologicalSnapshot.reconstruct_points` accepts would
do, such as a list of :class:`pygplates.PointOnSphere` or a :class:`pygplates.MultiPointOnSphere`.

.. sample-code:: pygplates_reconstruct_points_step_by_step_through_topologies.py
   :fragment: initial-points

The points are reconstructed from 30Ma to present day in steps of 1My.

Each step moves the points using the plates and networks as they are at the start of the step.
Plate boundaries and plate motions change over time, so a small time step, such as 1My, is best.

.. sample-code:: pygplates_reconstruct_points_step_by_step_through_topologies.py
   :fragment: time-steps

``points`` holds the positions of the points at the current time. It is replaced at every time step.

.. sample-code:: pygplates_reconstruct_points_step_by_step_through_topologies.py
   :fragment: current-points

Each pass of the loop is one time step, from ``time`` to ``next_time``. The last one ends at present
day, and is shorter than the others if ``time_increment`` does not divide ``initial_time``.

.. sample-code:: pygplates_reconstruct_points_step_by_step_through_topologies.py
   :fragment: time-loop

The topological features are resolved to the current ``time`` using :meth:`pygplates.TopologicalModel.topological_snapshot`.

.. sample-code:: pygplates_reconstruct_points_step_by_step_through_topologies.py
   :fragment: topological-snapshot

:meth:`pygplates.TopologicalSnapshot.reconstruct_points` reconstructs the points from the time of
the snapshot to the time it is given. It returns a list with a :class:`pygplates.PointOnSphere`
for each point, in the order of the points.

Here ``next_time`` is younger than the time of the snapshot, so the points move forward in time. A time
that is older moves them backward in time.

Where a deforming network overlays a rigid plate, a point inside both is reconstructed by the network.

A point in the deforming region of a network is moved by interpolating the motions of the
vertices of the network's triangulation around it. Natural neighbour interpolation is used unless
the *use_natural_neighbour_interpolation* argument is ``False``, and then barycentric interpolation.

A point that is outside all the resolved plates and networks cannot be reconstructed. By default
its entry in the list is ``None``. With *return_input_if_not_intersect* set to ``True``, as here,
the point is returned where it is, so every entry is a point and the list can be reconstructed
again at the next time step. (A list with ``None`` in it cannot: the ``None`` entries have to be
removed first.)

.. sample-code:: pygplates_reconstruct_points_step_by_step_through_topologies.py
   :fragment: reconstruct-points

Finally the positions of the points at the end of the time step are printed.

.. sample-code:: pygplates_reconstruct_points_step_by_step_through_topologies.py
   :fragment: print-points

.. note:: These are the positions that :meth:`pygplates.TopologicalModel.reconstruct_geometry` gives
   at the same times, for the same initial points and time step, when its *deactivate_points*
   argument is ``None``. By default it deactivates points, which this script does not: a point on a
   subducting plate is carried to the trench and across it, and from then on it moves with the
   overriding plate (or with a deforming network there).

Reconstruct each point using the plate or network that contains it
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

This example reconstructs the same points to the same positions, but one point at a time. At each
time step it finds the plate or network that contains each point, and reconstructs the point using
that. It prints a line whenever a point is in a different plate or network than it was a time step
before.

Sample code
"""""""""""

.. sample-code:: pygplates_reconstruct_points_step_by_step_through_topologies_by_topology.py

Details
"""""""

The topological model, points and time steps are the same as in the first example.

.. sample-code:: pygplates_reconstruct_points_step_by_step_through_topologies_by_topology.py
   :fragment: load-inputs

The plate or network that contains each point is kept from one time step to the next, so that a
change can be noticed.

.. sample-code:: pygplates_reconstruct_points_step_by_step_through_topologies_by_topology.py
   :fragment: previous-plate-ids

At each time, :meth:`pygplates.TopologicalSnapshot.get_point_locations` returns the
:class:`location<pygplates.TopologyPointLocation>` of each point in the resolved topologies, in the
order of the points.

.. sample-code:: pygplates_reconstruct_points_step_by_step_through_topologies_by_topology.py
   :fragment: point-locations

Each point is visited with its location.

.. sample-code:: pygplates_reconstruct_points_step_by_step_through_topologies_by_topology.py
   :fragment: iterate-points

:meth:`pygplates.TopologyPointLocation.located_in_resolved_boundary` returns the
:class:`resolved topological boundary<pygplates.ResolvedTopologicalBoundary>` (the rigid plate) that
contains the point, or ``None``.

:meth:`pygplates.TopologyPointLocation.located_in_resolved_network` returns the
:class:`resolved topological network<pygplates.ResolvedTopologicalNetwork>` that contains the point,
or ``None``.

A point is located in one or the other, not both: where a network overlays a plate, the point is
located in the network. A point located in neither is outside all the plates and networks.

.. sample-code:: pygplates_reconstruct_points_step_by_step_through_topologies_by_topology.py
   :fragment: containing-topology

Both classes have a ``reconstruct_point()`` method that reconstructs one point from the time the
topology was resolved at to another time
(see :meth:`pygplates.ResolvedTopologicalBoundary.reconstruct_point` and
:meth:`pygplates.ResolvedTopologicalNetwork.reconstruct_point`).

A plate rotates the point by the motion of its plate ID. A network deforms a point that is in its
deforming region, and rotates a point that is in one of its interior rigid blocks with that block.

The plate ID of the plate (or of the network) is read from its feature. It is not what moves a
point in a network: that comes from the features the network is built from.

The plate or network is labelled by its plate ID and by whether it is a plate or a network, such as
``plate 901`` or ``network 608``. A deforming network can have the plate ID of the plate it
overlays, so the plate ID alone would not tell the two apart.

A point outside all the plates and networks is left where it is, as it was in the first example,
and labelled ``no plate or network``.

.. sample-code:: pygplates_reconstruct_points_step_by_step_through_topologies_by_topology.py
   :fragment: reconstruct-point

A point whose plate or network differs from the one a time step before has passed from one to
another, or into or out of the plates altogether.

The labels are compared, rather than the resolved plates, because each snapshot has resolved
topologies of its own: the plate that contains a point at one time is never the same object as the
plate that contains it at another.

Nor are the feature IDs of the plates compared. A model can define a plate by a succession of
features, each for its own span of time (the GPlates sample data does), and a point would seem to
change plates whenever its plate changed from one feature to the next. The labels do not tell apart
two networks that have the same plate ID, though.

This is where a script can apply a rule of its own. For example, it could stop following a point
that passes from a subducting plate to the plate overriding it.

.. sample-code:: pygplates_reconstruct_points_step_by_step_through_topologies_by_topology.py
   :fragment: plate-change

The reconstructed points, and the labels of the plates and networks they were in, are the starting
points of the next time step.

.. sample-code:: pygplates_reconstruct_points_step_by_step_through_topologies_by_topology.py
   :fragment: next-time-step

After the last time step the points are at their present day positions.

.. sample-code:: pygplates_reconstruct_points_step_by_step_through_topologies_by_topology.py
   :fragment: print-points

.. note:: :meth:`pygplates.TopologicalSnapshot.reconstruct_points` gives the same positions in one
   call, and is noticeably faster, since getting the plate or network out of each point's location takes
   time. Reconstruct each point by itself, as here, only when the plate or network of each point is needed.

See also
++++++++

- Primer: :ref:`pygplates_primer_what_is_topological_reconstruction`,
  :ref:`pygplates_primer_using_topological_reconstruction`, :ref:`pygplates_primer_topological_snapshot`
- Reference: :meth:`pygplates.TopologicalSnapshot.reconstruct_points`,
  :meth:`pygplates.ResolvedTopologicalBoundary.reconstruct_point`,
  :meth:`pygplates.ResolvedTopologicalNetwork.reconstruct_point`,
  :meth:`pygplates.TopologicalSnapshot.get_point_locations`, :class:`pygplates.TopologyPointLocation`,
  :meth:`pygplates.TopologicalModel.reconstruct_geometry`
- Sample code: :ref:`pygplates_reconstruct_strain_and_strain_rate`,
  :ref:`pygplates_reconstruct_crustal_thickness_and_tectonic_subsidence`,
  :ref:`pygplates_find_plates_and_strain_rates_at_static_points`
