.. _pygplates_calculate_velocities_of_motion_path_and_flowline_seed_points:

Calculate velocities of motion path and flowline seed points
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example calculates the velocity of the seed point of each motion path and flowline as it is
reconstructed through geological time.

The seed point of a motion path is a point on a plate, and its motion path is the track of that
point. The velocity of the reconstructed seed point is how fast, and in which direction, the plate
is moving there at the reconstruction time.

The seed point of a flowline is a point on a mid-ocean ridge, and its velocity is that of the
spreading ridge there.

Both velocities are relative to the anchor plate, as other velocities are.

.. contents::
   :local:
   :depth: 2

Data files
""""""""""

``rotations.rot``
    A rotation file. It must contain rotations for the plate IDs of the motion paths and flowlines.

``motion_path_features.gpml``
    Motion path features, each with seed points, a reconstruction plate ID, a relative plate ID
    and a list of times.

``flowline_features.gpml``
    Flowline features, each with seed points, left and right plate IDs and a list of times.

Rotation files and flowlines are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Motion path and flowline features can be created in GPlates, or by
:ref:`pygplates_create_common_feature_types`.

Sample code
"""""""""""

.. sample-code:: pygplates_calculate_velocities_of_motion_path_and_flowline_seed_points.py

Details
"""""""

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.

.. sample-code:: pygplates_calculate_velocities_of_motion_path_and_flowline_seed_points.py
   :fragment: load-rotations

A :class:`reconstruct model<pygplates.ReconstructModel>` is created from both files of features and
the rotation model.

.. sample-code:: pygplates_calculate_velocities_of_motion_path_and_flowline_seed_points.py
   :fragment: reconstruct-model

The velocities at each ``time`` are calculated over a 1My interval, from ``time + delta_time`` to ``time``.

.. sample-code:: pygplates_calculate_velocities_of_motion_path_and_flowline_seed_points.py
   :fragment: delta-time

The features are reconstructed to the current ``time`` using :meth:`pygplates.ReconstructModel.reconstruct_snapshot`.

.. sample-code:: pygplates_calculate_velocities_of_motion_path_and_flowline_seed_points.py
   :fragment: reconstruct-snapshot

By default :meth:`pygplates.ReconstructSnapshot.get_reconstructed_geometries` returns neither
motion paths nor flowlines, so both are requested with the *reconstruct_types* argument
(see :class:`pygplates.ReconstructType`).

It returns a :class:`pygplates.ReconstructedMotionPath` or a :class:`pygplates.ReconstructedFlowline`
for each seed point of each feature. A feature that does not exist at the current ``time``
(according to its valid time) has none. Nor does a motion path or a flowline at, or before, the
oldest of its times, where there is no path or flowline yet.

.. sample-code:: pygplates_calculate_velocities_of_motion_path_and_flowline_seed_points.py
   :fragment: reconstructed-motion-paths-and-flowlines

``get_reconstructed_seed_point()`` returns the position of the seed point at the current ``time``
(see :meth:`pygplates.ReconstructedMotionPath.get_reconstructed_seed_point` and
:meth:`pygplates.ReconstructedFlowline.get_reconstructed_seed_point`).

For a motion path this is the last point of the path, which is the youngest. For a flowline it is
the point on the spreading ridge that its left and right halves spread from.

.. sample-code:: pygplates_calculate_velocities_of_motion_path_and_flowline_seed_points.py
   :fragment: seed-point

``get_reconstructed_seed_point_velocity()`` returns the velocity of the reconstructed seed point
as a :class:`pygplates.Vector3D` (a global cartesian velocity vector)
(see :meth:`pygplates.ReconstructedMotionPath.get_reconstructed_seed_point_velocity` and
:meth:`pygplates.ReconstructedFlowline.get_reconstructed_seed_point_velocity`).

It is in kilometres per million years by default. Here *velocity_units* asks for centimetres per year.

The seed point of a motion path moves with the reconstruction plate ID of its feature, so this is
the velocity of that plate at the seed point. It is relative to the anchor plate, not to the
relative plate ID of the feature, which only affects the shape of the motion path.

The seed point of a flowline moves by half-stage rotation between the left and right plate IDs of
its feature, so its velocity is about midway between the velocities of those two plates there.

.. sample-code:: pygplates_calculate_velocities_of_motion_path_and_flowline_seed_points.py
   :fragment: seed-point-velocity

The vector is converted to a local (magnitude, azimuth, inclination) tuple at the seed point.

A :class:`pygplates.LocalCartesian` is a local coordinate system at one point, here the seed point,
and :meth:`pygplates.LocalCartesian.from_geocentric_to_magnitude_azimuth_inclination` converts a
vector into it.

The magnitude is the speed. The azimuth is the direction of motion, as an angle in radians
clockwise from North (as viewed from the seed point).

.. sample-code:: pygplates_calculate_velocities_of_motion_path_and_flowline_seed_points.py
   :fragment: convert-velocity

Finally the position, speed and direction of each seed point are printed.

.. sample-code:: pygplates_calculate_velocities_of_motion_path_and_flowline_seed_points.py
   :fragment: print-velocity

See also
""""""""

- Reference: :meth:`pygplates.ReconstructedMotionPath.get_reconstructed_seed_point_velocity`,
  :meth:`pygplates.ReconstructedFlowline.get_reconstructed_seed_point_velocity`,
  :class:`pygplates.ReconstructModel`, :class:`pygplates.ReconstructSnapshot`,
  :class:`pygplates.ReconstructType`, :class:`pygplates.LocalCartesian`
- Sample code: :ref:`pygplates_reconstruct_motion_path_features`, :ref:`pygplates_reconstruct_flowline_features`,
  :ref:`pygplates_calculate_velocities_by_plate_id`, :ref:`pygplates_calculate_velocities_of_plate_boundaries`
