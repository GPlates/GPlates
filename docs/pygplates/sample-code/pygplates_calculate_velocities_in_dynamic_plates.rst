.. _pygplates_calculate_velocities_in_dynamic_plates:

Calculate velocities in dynamic plates
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example shows three similar ways to calculate velocities of topological plates at static point locations.

.. contents::
   :local:
   :depth: 2

Calculate velocities by assigning plate IDs of dynamic plates to static points
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Sample code
"""""""""""

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_assign_plate_ids.py

Calculate velocities by grouping static points into dynamic plates
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

| This example is similar to the above example except it groups velocities by partitioning plates.
| It is also slightly faster than the above example, but only by one or two percent.

Sample code
"""""""""""

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_group_points.py

Calculate velocities by individually partitioning each static point into dynamic plates
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

| This example is **ten times slower** than the above two examples.
| However it has the advantage of keeping the output velocities (and domain positions) in the same
  order as the input domain points (ie, the order of points in each domain multipoint).

Sample code
"""""""""""

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_partition_points.py
