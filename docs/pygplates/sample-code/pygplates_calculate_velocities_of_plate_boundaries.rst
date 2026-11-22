.. _pygplates_calculate_velocities_of_plate_boundaries:

Calculate velocities of plate boundaries
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example calculates the velocities of the points along the boundaries of resolved topological
plates (and deforming networks): how fast the boundaries themselves are moving.

A plate boundary does not move with the plate inside it. The boundary of a topological plate is
assembled from sections (trenches, ridges and so on), each a feature that moves in its own way, and
each point of the resolved boundary moves as the section it comes from does. A trench might move
with the plate that overrides it, and a mid-ocean ridge spreads midway between the two plates
beside it.

So the velocity of a boundary point need not be the velocity that either of the plates beside it
has there. (For those see :ref:`pygplates_calculate_velocities_in_dynamic_plates`, and for how
fast two plates converge across a boundary see
:ref:`pygplates_find_divergence_at_subduction_zones_and_convergence_at_ridges`.)

This example finds the velocity of each boundary point, and the boundary section each belongs to,
and prints the mean speed of each section of each plate's boundary.

.. contents::
   :local:
   :depth: 2

Data files
""""""""""

``rotations.rot``
    A rotation file. It must contain rotations for the plate IDs of the boundary sections that the
    topologies are built from, since those are what move the boundary points.

``topologies.gpml``
    Topological plate polygon features, with or without deforming network features, and the
    boundary section features that they reference.

Files of these kinds are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_calculate_velocities_of_plate_boundaries.py

Details
"""""""

A :class:`topological model<pygplates.TopologicalModel>` is created from the topological features and the rotation file.

.. sample-code:: pygplates_calculate_velocities_of_plate_boundaries.py
   :fragment: create-topological-model

The velocities are calculated at boundaries resolved at 10Ma. And the velocities are calculated over a 1My interval from 11Ma to 10Ma.

.. sample-code:: pygplates_calculate_velocities_of_plate_boundaries.py
   :fragment: reconstruction-time-and-delta-time

The topological features are resolved at 10Ma using :meth:`pygplates.TopologicalModel.topological_snapshot`.

.. sample-code:: pygplates_calculate_velocities_of_plate_boundaries.py
   :fragment: topological-snapshot

:meth:`pygplates.TopologicalSnapshot.get_resolved_topologies` returns a
:class:`pygplates.ResolvedTopologicalBoundary` for each topological plate and a
:class:`pygplates.ResolvedTopologicalNetwork` for each deforming network. The three methods used
below are methods of both.

.. sample-code:: pygplates_calculate_velocities_of_plate_boundaries.py
   :fragment: iterate-resolved-topologies

``get_resolved_geometry_points()`` returns the points of the resolved boundary, as a list of
:class:`pygplates.PointOnSphere`
(see :meth:`pygplates.ResolvedTopologicalBoundary.get_resolved_geometry_points`).

They are in order around the boundary, one section after another. Where two sections cross or
touch, the point where they meet appears twice: once as the last point of one section and once
as the first point of the next.

.. sample-code:: pygplates_calculate_velocities_of_plate_boundaries.py
   :fragment: boundary-points

``get_resolved_geometry_point_velocities()`` returns the velocities of those points, in the same order
(see :meth:`pygplates.ResolvedTopologicalBoundary.get_resolved_geometry_point_velocities`).

This is a list of :class:`pygplates.Vector3D` (one global cartesian velocity vector per point),
in kilometres per million years unless the *velocity_units* argument says otherwise.

By default each velocity is calculated from ``reconstruction_time + velocity_delta_time`` to ``reconstruction_time``.

.. sample-code:: pygplates_calculate_velocities_of_plate_boundaries.py
   :fragment: boundary-point-velocities

``get_resolved_geometry_point_features()`` returns the feature that each point comes from, again in the same order
(see :meth:`pygplates.ResolvedTopologicalBoundary.get_resolved_geometry_point_features`).

That feature is the boundary section the point belongs to, and it is what determines the velocity
of the point. A point of a section that has a reconstruction plate ID moves with that plate. A
point of a mid-ocean ridge that is reconstructed by half-stage rotation moves midway between its
left and right plates.

.. sample-code:: pygplates_calculate_velocities_of_plate_boundaries.py
   :fragment: boundary-point-features

The vectors are converted to local (magnitude, azimuth, inclination) tuples, one per point, using
:meth:`pygplates.LocalCartesian.convert_from_geocentric_to_magnitude_azimuth_inclination`.

Each point determines its own local coordinate system. For example, the velocity *azimuth* is
relative to North as viewed from that point.

.. sample-code:: pygplates_calculate_velocities_of_plate_boundaries.py
   :fragment: convert-velocities

The speeds of the points are grouped by boundary section.

The sections are told apart by :meth:`feature ID<pygplates.Feature.get_feature_id>`, which is the key of
both dictionaries: one keeps the feature of each section, and the other a list of the speeds of its points.

.. sample-code:: pygplates_calculate_velocities_of_plate_boundaries.py
   :fragment: group-by-section

Finally the sections of the plate's boundary are printed, each with its
:meth:`feature type<pygplates.Feature.get_feature_type>` (such as ``gpml:SubductionZone``), its name,
its number of points and their mean speed.

The plate ID and name of the plate itself come from its feature
(:meth:`pygplates.ResolvedTopologicalBoundary.get_feature`, or
:meth:`pygplates.ResolvedTopologicalNetwork.get_feature` for a network).

.. sample-code:: pygplates_calculate_velocities_of_plate_boundaries.py
   :fragment: print-sections

.. note:: A boundary that two plates share is visited twice here, once for each plate. To visit each
   boundary once, iterate over :meth:`pygplates.TopologicalSnapshot.get_resolved_topological_sections`
   and the :meth:`shared sub-segments<pygplates.ResolvedTopologicalSection.get_shared_sub_segments>`
   of each (as :ref:`pygplates_find_total_ridge_and_subduction_zone_lengths` does). A
   :class:`pygplates.ResolvedTopologicalSharedSubSegment` has the same three methods, as do a
   :class:`pygplates.ResolvedTopologicalSubSegment` and a :class:`pygplates.ResolvedTopologicalLine`.

.. note:: These are velocities at the points that the boundary happens to have, which can be far
   apart along a straight section. :meth:`pygplates.TopologicalSnapshot.calculate_plate_boundary_statistics`
   calculates the velocity of the boundary at points uniformly spaced along it
   (:attr:`pygplates.PlateBoundaryStatistic.boundary_velocity`), with the velocities of the plates on either side.

See also
""""""""

- Primer: :ref:`pygplates_primer_topological_model`, :ref:`pygplates_primer_topological_snapshot`
- Reference: :meth:`pygplates.ResolvedTopologicalBoundary.get_resolved_geometry_point_velocities`,
  :meth:`pygplates.ResolvedTopologicalBoundary.get_resolved_geometry_point_features`,
  :class:`pygplates.ResolvedTopologicalNetwork`, :class:`pygplates.ResolvedTopologicalSharedSubSegment`,
  :class:`pygplates.TopologicalSnapshot`, :class:`pygplates.LocalCartesian`
- Sample code: :ref:`pygplates_calculate_velocities_in_dynamic_plates`,
  :ref:`pygplates_calculate_velocities_by_plate_id`,
  :ref:`pygplates_find_divergence_at_subduction_zones_and_convergence_at_ridges`,
  :ref:`pygplates_calculate_velocities_of_motion_path_and_flowline_seed_points`
