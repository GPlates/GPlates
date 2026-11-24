.. _pygplates_calculate_net_rotation_from_point_samples:

Calculate net rotation from point samples
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example calculates net rotation by adding up the contributions of sample points, where
:ref:`pygplates_calculate_net_rotation` leaves that to a :class:`pygplates.NetRotationModel`.

A :class:`pygplates.NetRotationModel` needs a topological model, and decides for itself how each
sample point moves. Adding the samples yourself is for everything else: a plate model without
topologies, motions that come from somewhere other than a plate model, or a net rotation over
some of the points only.

A :class:`pygplates.NetRotation` is an accumulator. A new one is zero, each sample point
contributes one that is created from the point, the area it stands for and its motion, and they are added together.

The first example calculates the net rotation of static polygons, giving each sample point the
rotation of its plate. The second calculates net rotation from a velocity at each sample point.

.. contents::
   :local:
   :depth: 2

.. _pygplates_calculate_net_rotation_of_static_polygons:

Calculate net rotation of static polygons
+++++++++++++++++++++++++++++++++++++++++

This example calculates the net rotation of the plates of a model that has static polygons but no
topologies, and the net rotation of each plate, at each time over a time period.

Data files
""""""""""

``rotations.rot``
    A rotation file. It must contain rotations for the plate IDs of the static polygons.

``static_polygons.gpml``
    Static polygon features, each with a reconstruction plate ID.

Files of these kinds are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_calculate_net_rotation_from_point_samples.py

Details
"""""""

The rotations are loaded into a :class:`pygplates.RotationModel`, which is kept because the
rotation of each plate is needed below.

A :class:`reconstruct model<pygplates.ReconstructModel>` is created from the static polygons and
the rotation model.

.. sample-code:: pygplates_calculate_net_rotation_from_point_samples.py
   :fragment: create-reconstruct-model

Net rotation is an integral over the surface of the globe, which is approximated by a sum over
sample points. Each sample point has a *sample area*: the area of the globe that it stands for,
in steradians (square radians).

Here the points are a uniform latitude-longitude grid, which is what
:class:`pygplates.NetRotationModel` uses by default (at a finer spacing). On such a grid the sample
area shrinks with the cosine of the latitude, because the points are closer together near the poles.

Any distribution of points will do, as long as each has its sample area. For points spread evenly
over the sphere that is the same for every point: the area of the sphere divided by the number of points.

.. sample-code:: pygplates_calculate_net_rotation_from_point_samples.py
   :fragment: sample-points

The motion of each plate is taken over a 1My interval, from ``time + delta_time`` to ``time``.

.. sample-code:: pygplates_calculate_net_rotation_from_point_samples.py
   :fragment: delta-time

At each time the static polygons are reconstructed, and :meth:`pygplates.ReconstructSnapshot.get_point_locations`
finds the :class:`reconstructed static polygon<pygplates.ReconstructedFeatureGeometry>` that
contains each sample point (see :ref:`pygplates_calculate_velocities_in_static_polygons`).

.. sample-code:: pygplates_calculate_net_rotation_from_point_samples.py
   :fragment: point-locations

``pygplates.NetRotation()`` creates a zero net rotation.

One accumulates the net rotation of all the plates. The others, one for each plate, are kept in a
dictionary whose keys are plate IDs.

.. sample-code:: pygplates_calculate_net_rotation_from_point_samples.py
   :fragment: zero-net-rotations

A sample point that is outside all the reconstructed static polygons is skipped. It contributes
neither a rotation nor its sample area.

So the total is the net rotation of the area that the polygons cover, not of the whole globe
(see the note below).

.. sample-code:: pygplates_calculate_net_rotation_from_point_samples.py
   :fragment: iterate-samples

The motion of a sample point is the motion of its plate: the
:meth:`stage rotation<pygplates.RotationModel.get_rotation>` of the polygon's reconstruction plate ID
from ``time + delta_time`` to ``time``.

Many sample points are on the same plate, so the stage rotation of each plate is looked up once
and kept in a dictionary.

.. sample-code:: pygplates_calculate_net_rotation_from_point_samples.py
   :fragment: stage-rotation

:meth:`pygplates.NetRotation.create_sample_from_finite_rotation` creates the contribution of one
sample point from the point, its sample area, and the finite rotation that moves it.

The last argument is the time interval of that finite rotation (which defaults to 1My), so that
the rotation can be converted to a rate.

.. sample-code:: pygplates_calculate_net_rotation_from_point_samples.py
   :fragment: create-sample

Net rotations are added with ``+=`` (or with ``+``, which returns a new net rotation).

The same sample is added to the net rotation of all the plates and to the net rotation of its own plate.

.. sample-code:: pygplates_calculate_net_rotation_from_point_samples.py
   :fragment: accumulate-samples

A net rotation can be extracted as a :meth:`finite rotation<pygplates.NetRotation.get_finite_rotation>`
over one million years, or as a :meth:`rotation rate vector<pygplates.NetRotation.get_rotation_rate_vector>`.

Here the pole and the angle of the finite rotation are printed. The angle is the rotation rate in
degrees per million years.

.. sample-code:: pygplates_calculate_net_rotation_from_point_samples.py
   :fragment: print-total-net-rotation

The net rotation of each plate is printed as the magnitude of its rotation rate vector, which is
in radians per million years.

:meth:`pygplates.NetRotation.get_area` is the sum of the sample areas that were added, so here it
is the area of the plate as its sample points measure it.

.. sample-code:: pygplates_calculate_net_rotation_from_point_samples.py
   :fragment: print-plate-net-rotations

.. note:: A net rotation is an average over the area of its own samples. The net rotation of a plate
   is averaged over the area of that plate, and the total here over the area that the reconstructed
   polygons cover, which shrinks as gaps open between them at past times. To average over the whole
   globe instead, also add a sample for each point that is outside all the polygons, with a rotation
   of :meth:`pygplates.FiniteRotation.create_identity_rotation`. It adds its sample area but no
   rotation, and so reduces the net rotation in proportion.

.. _pygplates_calculate_net_rotation_from_velocities:

Calculate net rotation from velocities
++++++++++++++++++++++++++++++++++++++

This example calculates net rotation from a velocity at each sample point. Here the velocities come
from resolved topologies, but they could come from anywhere, such as a file of velocities.

Data files
""""""""""

``rotations.rot``
    A rotation file. It must contain rotations for the plate IDs of the topological features.

``topologies.gpml``
    Topological plate polygon features, with or without deforming network features.

Files of these kinds are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_calculate_net_rotation_from_point_samples_velocities.py

Details
"""""""

A :class:`topological model<pygplates.TopologicalModel>` is created from the topological features
and the rotation file. The sample points, their sample areas and the time interval are the same as
in the first example.

.. sample-code:: pygplates_calculate_net_rotation_from_point_samples_velocities.py
   :fragment: load-inputs

At each time, :meth:`pygplates.TopologicalSnapshot.get_point_velocities` calculates the velocity
at each sample point from the plate or deforming network that contains it (see
:ref:`pygplates_calculate_velocities_in_dynamic_plates`).

The units and the Earth radius are the defaults. They are spelled out because the conversion
below depends on them.

.. sample-code:: pygplates_calculate_net_rotation_from_point_samples_velocities.py
   :fragment: point-velocities

A sample point that is outside all the plates has no velocity, and is skipped.

.. sample-code:: pygplates_calculate_net_rotation_from_point_samples_velocities.py
   :fragment: iterate-samples

The motion of a sample point is given as a rotation rate vector: its direction is the axis that
the point is rotating about, and its magnitude is the rate in radians per million years.

For a point at position :math:`\boldsymbol r` (a unit vector) with velocity :math:`\boldsymbol v`,
the vector :math:`\boldsymbol r \times \boldsymbol v` is such a rotation rate, once the velocity is
divided by the radius of the Earth to convert kilometres to radians.

It is not the only rotation rate that gives the point that velocity. A rotation about an axis
through the point itself doesn't move the point, so any amount of it can be added without changing
the velocity. Net rotation measures how the lithosphere moves, so it uses only the part of a
sample's rotation rate that moves the point, and it doesn't matter which of these rotation rates is
given. :math:`\boldsymbol r \times \boldsymbol v` is the simplest of them.

.. sample-code:: pygplates_calculate_net_rotation_from_point_samples_velocities.py
   :fragment: rotation-rate

:meth:`pygplates.NetRotation.create_sample_from_rotation_rate` creates the contribution of one
sample point from the point, its sample area and its rotation rate vector. It is added to the net
rotation with ``+=``.

.. sample-code:: pygplates_calculate_net_rotation_from_point_samples_velocities.py
   :fragment: accumulate-samples

The net rotation is printed as the magnitude of its rotation rate vector, converted to degrees per million years.

.. sample-code:: pygplates_calculate_net_rotation_from_point_samples_velocities.py
   :fragment: print-net-rotation

.. note:: With the same sample points and areas (its *point_distribution* argument), a
   :class:`pygplates.NetRotationSnapshot` calculates the same net rotation for points inside rigid
   plates. Inside the deforming region of a network the two differ slightly: the snapshot
   interpolates the *stage rotations* of the corners of the network triangle that contains the point,
   where :meth:`pygplates.TopologicalSnapshot.get_point_velocities` interpolates the *velocities* of
   the vertices near the point, using natural neighbour interpolation.

See also
++++++++

- Primer: :ref:`pygplates_primer_equivalent_stage_rotation`
- Reference: :class:`pygplates.NetRotation`, :meth:`pygplates.NetRotation.create_sample_from_finite_rotation`,
  :meth:`pygplates.NetRotation.create_sample_from_rotation_rate`, :meth:`pygplates.ReconstructSnapshot.get_point_locations`,
  :meth:`pygplates.TopologicalSnapshot.get_point_velocities`, :meth:`pygplates.RotationModel.get_rotation`
- Sample code: :ref:`pygplates_calculate_net_rotation`, :ref:`pygplates_calculate_velocities_in_static_polygons`,
  :ref:`pygplates_calculate_velocities_in_dynamic_plates`
