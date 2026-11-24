.. _pygplates_partition_features_into_plates:

Partition features into plates
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example partitions features, and a single geometry, into plates at a past geological time using
a :class:`pygplates.PlatePartitioner`.

A plate partitioner holds a set of plates at one time: static polygons reconstructed to that time,
or topological plates (and deforming networks) resolved at that time. It cuts whatever it is given
along their boundaries.

The first example partitions features whose geometries were digitised at a past time. It assigns
each piece the plate ID of its plate, so that the features can be reconstructed to any other time.

The second example partitions a single polygon into resolved topological plates, to find how much
of it each plate covers.

Both partition polygons, which a plate partitioner cuts into polygons.

.. seealso:: :ref:`pygplates_import_geometries_and_assign_plate_ids`, which partitions present day
   points and polylines once, using the function :func:`pygplates.partition_into_plates`.

.. contents::
   :local:
   :depth: 2

.. _pygplates_partition_features_at_a_past_time:

Partition features at a past time
+++++++++++++++++++++++++++++++++

This example:

- creates a plate partitioner from static polygons reconstructed to 50Ma, with Africa as the anchor plate,
- splits features whose geometries are positions at 50Ma (relative to Africa) into the plates,
  assigning plate IDs to the pieces,
- prints the polygons and scalar values of the pieces, and saves them, and
- assigns plate IDs a second way, without splitting the features.

Data files
""""""""""

``rotations.rot``
    A rotation file. It must contain rotations for the plate IDs of the static polygons.

``static_polygons.gpml``
    Static polygon features, each with a reconstruction plate ID. They are reconstructed to 50Ma to
    become the partitioning plates.

``features_at_50Ma.gpml``
    The features to partition. Their geometries are not at present day positions: they are where the
    features were at 50Ma, relative to Africa (plate 701). Data digitised over a reconstruction, such
    as a palaeogeographic map, is like this. The features need no plate IDs: each piece inside a
    plate is given the plate ID of that plate.

Rotation files and static polygons are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_partition_features_into_plates.py

Details
"""""""

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.

.. sample-code:: pygplates_partition_features_into_plates.py
   :fragment: load-rotations

The features to partition are at their 50Ma positions, in a reference frame that holds Africa (plate 701) fixed.

The plates must be at the same time and in the same reference frame, or the features will not be
where the plates are.

.. sample-code:: pygplates_partition_features_into_plates.py
   :fragment: reconstruction-time-and-anchor-plate

A :class:`pygplates.PlatePartitioner` is created from the static polygons. It reconstructs them to
*reconstruction_time*, relative to *anchor_plate_id*, and these reconstructed polygons are its
partitioning plates.

Without *reconstruction_time* the plates are at present day. Without *anchor_plate_id* the anchor
plate is the default anchor plate of the rotation model, which is zero unless the rotation model
was created with another.

The plate partitioner can now partition any number of features and geometries. To partition at
another time, create another plate partitioner.

.. sample-code:: pygplates_partition_features_into_plates.py
   :fragment: create-plate-partitioner

:meth:`pygplates.PlatePartitioner.partition_features` splits each feature into the plates that it overlaps.
This is the default *partition_method*, :class:`PartitionMethod.split_into_plates<pygplates.PartitionMethod>`.

A feature that overlaps two plates becomes two features, each a copy of the original with the part
of its geometry that is inside one plate. Each is assigned the reconstruction plate ID of its plate
(other properties of the plate, such as its valid time, can be copied too: see the *properties_to_copy* argument).

Each of these partitioned features is then reverse reconstructed, with its new plate ID, from 50Ma
to present day, using the rotation model and the anchor plate of the plate partitioner. Its
geometry is now a present day position, like that of any other feature.

Any part of a feature that is outside all the plates becomes an unpartitioned feature. It is not
assigned a plate ID (it keeps one that the original feature had) and it is not reverse
reconstructed, so its geometry is still a position at 50Ma.

With *partition_return* set to :class:`PartitionReturn.separate_partitioned_and_unpartitioned<pygplates.PartitionReturn>`
the partitioned and unpartitioned features are returned in two lists. By default they are returned in one.

The features that were passed in are not changed. New features are always returned.

.. sample-code:: pygplates_partition_features_into_plates.py
   :fragment: partition-features

Each partitioned feature has the name of the feature it was cut from, and the plate ID of its plate.

.. sample-code:: pygplates_partition_features_into_plates.py
   :fragment: iterate-partitioned-features

A polygon that crosses a plate boundary is cut into polygons (before pyGPlates 1.1 it was cut into polylines).

The polygons have interior rings where they need them. For example, where the original polygon
covers a small plate and the plate around it, the piece on the plate around it has a hole where
the small plate is.

The area of a polygon is :meth:`pygplates.PolygonOnSphere.get_area`, which takes its interior rings
into account, and :meth:`pygplates.PolygonOnSphere.get_number_of_interior_rings` counts them.

.. sample-code:: pygplates_partition_features_into_plates.py
   :fragment: partitioned-polygons

A geometry can have scalar values, one at each of its points (a *coverage*). When such a geometry
is split, each piece keeps the values at the original's points, and gets interpolated values at
the points that the cut adds.

:meth:`pygplates.Feature.get_geometries` returns each geometry with its scalar values when
*coverage_return* is :class:`CoverageReturn.geometry_and_scalars<pygplates.CoverageReturn>`. The
values are in a ``dict`` mapping each :class:`pygplates.ScalarType` to a list with one value per point.

Each piece that has scalar values is returned in a feature of its own.

.. sample-code:: pygplates_partition_features_into_plates.py
   :fragment: partitioned-coverages

The reconstructed static polygons do not cover the globe. They have moved apart since present day,
leaving gaps.

An unpartitioned feature has the parts of one original feature that are in the gaps.

.. sample-code:: pygplates_partition_features_into_plates.py
   :fragment: unpartitioned-features

The partitioned features are saved to a file. With their plate IDs and present day geometries they
can be reconstructed to any time, like the features in :ref:`pygplates_reconstruct_regular_features`.

.. sample-code:: pygplates_partition_features_into_plates.py
   :fragment: write-partitioned-features

A cut can leave slivers: where the outline of a polygon was digitised along a plate boundary it
strays a few metres over the boundary, and each excursion is a thin piece on the neighbouring plate.

A plate partitioner drops the pieces of a polygon that are narrower than its *sliver_width_threshold_radians*
argument, which is about 90 metres by default. So a polygon digitised along a plate boundary
stays whole, on one plate.

With a threshold of zero every piece is kept, and the pieces cover the original polygon exactly.

.. sample-code:: pygplates_partition_features_into_plates.py
   :fragment: keep-slivers

With *partition_method* set to :class:`PartitionMethod.most_overlapping_plate<pygplates.PartitionMethod>`
the features are not split. Each feature is assigned the plate ID of the one plate that most overlaps it,
and its whole geometry is reverse reconstructed with that plate ID.

The plate that most overlaps a polygon is the one containing the largest area of it (before
pyGPlates 1.1 this was measured by the length of the polygon's outline). For a polyline it is the
plate containing the greatest length, and for points the greatest number.

A feature that overlaps no plate is returned as it was.

.. sample-code:: pygplates_partition_features_into_plates.py
   :fragment: most-overlapping-plate

.. _pygplates_partition_a_polygon_into_resolved_plates:

Partition a polygon into resolved plates
++++++++++++++++++++++++++++++++++++++++

This example:

- creates a plate partitioner from the topological plates (and deforming networks) resolved at 10Ma,
- partitions a polygon into them, and
- prints the area of the polygon that each plate covers.

Data files
""""""""""

``rotations.rot``
    A rotation file. It must contain rotations for the plate IDs of the features that the
    topologies are built from.

``topologies.gpml``
    Topological plate polygon features, with or without deforming network features. They are
    resolved at 10Ma to become the partitioning plates.

Files of these kinds are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_partition_features_into_plates_geometry.py

Details
"""""""

A :class:`topological model<pygplates.TopologicalModel>` is created from the topological features and the rotation file.

.. sample-code:: pygplates_partition_features_into_plates_geometry.py
   :fragment: create-topological-model

The topological features are resolved at 10Ma using :meth:`pygplates.TopologicalModel.topological_snapshot`.

.. sample-code:: pygplates_partition_features_into_plates_geometry.py
   :fragment: topological-snapshot

A :class:`pygplates.PlatePartitioner` can also be created from plates that have already been
reconstructed or resolved. Here they are the resolved topological plates and networks of the
snapshot, from :meth:`pygplates.TopologicalSnapshot.get_resolved_topologies`.

The plate partitioner also needs the rotation model and the anchor plate that the plates were
resolved with. :meth:`pygplates.TopologicalSnapshot.get_rotation_model` provides both, since the
default anchor plate of the rotation model it returns is the anchor plate of the snapshot.

Where a deforming network overlays a plate, the network is searched first, so what is inside both
is partitioned into the network (see :class:`pygplates.SortPartitioningPlates`).

.. sample-code:: pygplates_partition_features_into_plates_geometry.py
   :fragment: create-plate-partitioner

The polygon to partition is created from four points.

A geometry given to a plate partitioner is not reconstructed. It is taken to be at the time of
the partitioning plates (10Ma here) and in the reference frame of their anchor plate.

.. sample-code:: pygplates_partition_features_into_plates_geometry.py
   :fragment: region

:meth:`pygplates.PlatePartitioner.partition_geometry` partitions the polygon, appending to the two lists it is given.

To the first it appends a 2-tuple for each plate that contains part of the polygon: the plate,
and a list of the pieces inside that plate.

To the second it appends the pieces that are outside all the plates.

As with features, a polygon is partitioned into polygons.

.. sample-code:: pygplates_partition_features_into_plates_geometry.py
   :fragment: partition-geometry

Each plate is a :class:`pygplates.ResolvedTopologicalBoundary` or a :class:`pygplates.ResolvedTopologicalNetwork`.

A rigid block inside a deforming network is a plate of its own here: a
:class:`pygplates.ReconstructedFeatureGeometry`, whose feature is the block's. The network then
contains only what is in its deforming region, so a piece of the polygon that surrounds a rigid
block has an interior ring where the block is.

All three have a ``get_feature()`` method, which provides the plate ID and name.

The area that a plate covers is the sum of the areas of its pieces.

.. sample-code:: pygplates_partition_features_into_plates_geometry.py
   :fragment: pieces-inside-plates

The pieces outside all the plates are where the polygon extends beyond the topological model.

.. sample-code:: pygplates_partition_features_into_plates_geometry.py
   :fragment: pieces-outside-plates

.. note:: To partition with a single polygon, :meth:`pygplates.PolygonOnSphere.partition` does the
   same without a plate partitioner. :ref:`pygplates_find_overlap_union_and_difference_of_polygons`
   shows the polygon operations that return the same pieces.

See also
++++++++

- Reference: :class:`pygplates.PlatePartitioner`, :meth:`pygplates.PlatePartitioner.partition_features`,
  :meth:`pygplates.PlatePartitioner.partition_geometry`, :class:`pygplates.PartitionMethod`,
  :class:`pygplates.PartitionReturn`, :class:`pygplates.PartitionProperty`,
  :class:`pygplates.SortPartitioningPlates`, :func:`pygplates.partition_into_plates`,
  :meth:`pygplates.PolygonOnSphere.partition`
- Sample code: :ref:`pygplates_import_geometries_and_assign_plate_ids`,
  :ref:`pygplates_find_overlap_union_and_difference_of_polygons`,
  :ref:`pygplates_calculate_velocities_in_static_polygons`, :ref:`pygplates_reconstruct_regular_features`
