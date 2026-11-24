.. _pygplates_find_overlap_union_and_difference_of_polygons:

Find the overlap, union and difference of polygons
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example combines pairs of polygons using the four Boolean operations of
:class:`pygplates.PolygonOnSphere`, which were added in pyGPlates 1.1:

- :meth:`intersection()<pygplates.PolygonOnSphere.intersection>`: the region inside both polygons (their overlap),
- :meth:`union()<pygplates.PolygonOnSphere.union>`: the region inside either polygon,
- :meth:`difference()<pygplates.PolygonOnSphere.difference>`: the region inside the first polygon but outside the second, and
- :meth:`symmetric_difference()<pygplates.PolygonOnSphere.symmetric_difference>`: the region inside exactly one of them.

Each returns a list of polygons, since the result can be in several pieces, or empty. The polygons
in a list do not overlap each other, and they have interior rings where the region has holes.

The first example finds how much of a region of interest each reconstructed static polygon covers,
and how much of the region none of them covers.

The second example compares two positions of the same polygon using all four operations.

.. contents::
   :local:
   :depth: 2

Data files
++++++++++

Both scripts read the same files:

``rotations.rot``
    A rotation file. It must contain rotations for the plate IDs of the static polygons.

``static_polygons.gpml``
    Polygon features, each with a reconstruction plate ID. Static polygons are used here, but any
    polygon features will do, such as continental polygons.

Files of these kinds are in the GPlates `sample data <https://www.gplates.org/download/>`_.

.. _pygplates_find_overlap_of_reconstructed_polygons_with_a_region:

Find the overlap of reconstructed polygons with a region
++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Sample code
"""""""""""

.. sample-code:: pygplates_find_overlap_union_and_difference_of_polygons.py

Details
"""""""

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.

.. sample-code:: pygplates_find_overlap_union_and_difference_of_polygons.py
   :fragment: load-rotations

The static polygons are reconstructed to 50Ma by creating a :class:`pygplates.ReconstructSnapshot`.

.. sample-code:: pygplates_find_overlap_union_and_difference_of_polygons.py
   :fragment: reconstruct-snapshot

The region of interest is a :class:`pygplates.PolygonOnSphere` created from four points. It is not
reconstructed: it stays where it is while the static polygons move across it.

.. sample-code:: pygplates_find_overlap_union_and_difference_of_polygons.py
   :fragment: region

The part of the region that no reconstructed polygon covers is kept as a list of polygons. It starts
as the whole region, and each reconstructed polygon is removed from it in the loop below.

.. sample-code:: pygplates_find_overlap_union_and_difference_of_polygons.py
   :fragment: uncovered-region

:meth:`pygplates.ReconstructSnapshot.get_reconstructed_geometries` returns a
:class:`pygplates.ReconstructedFeatureGeometry` for each geometry of each feature that exists at 50Ma.

The Boolean operations are methods of polygons, so a reconstructed geometry of another type is skipped.

.. sample-code:: pygplates_find_overlap_union_and_difference_of_polygons.py
   :fragment: iterate-reconstructed-polygons

:meth:`pygplates.PolygonOnSphere.intersection` returns the polygons that cover the region inside
both the reconstructed polygon and the region of interest.

The list is empty if the two are apart, and also if they only touch (along an edge, or at points).

If the reconstructed polygon is entirely inside the region of interest, the list contains the
reconstructed polygon itself.

.. sample-code:: pygplates_find_overlap_union_and_difference_of_polygons.py
   :fragment: intersection

The area of the overlap is the sum of the areas of the polygons in the list, since they do not
overlap each other. :meth:`pygplates.PolygonOnSphere.get_area` returns an area on a sphere of unit
radius, which is multiplied by the square of the Earth's radius to convert it to square kilometres.

The interior rings of both polygons take part in the operation. So where the region of interest
covers a hole in a reconstructed polygon, the polygon returned has that hole as an interior ring,
and its area does not include it.

.. sample-code:: pygplates_find_overlap_union_and_difference_of_polygons.py
   :fragment: print-intersection

:meth:`pygplates.PolygonOnSphere.difference` returns the polygons that cover the region inside one
polygon but outside another.

Here the reconstructed polygon is removed from each polygon of what is left of the region of
interest, and the pieces that are returned become what is left.

If the two polygons are apart (or only touch) the list contains the first polygon itself, and if
the first is entirely inside the second the list is empty.

Where the second polygon is entirely inside the first, the polygon returned has a hole where
the second is.

.. sample-code:: pygplates_find_overlap_union_and_difference_of_polygons.py
   :fragment: difference

What is left after every reconstructed polygon has been removed is the part of the region of
interest that no polygon covers: the gaps between the reconstructed static polygons.

The areas that the plates cover can add up to more than the rest of the region, because
reconstructed static polygons also move over each other, and the area where two overlap is
counted for both.

.. sample-code:: pygplates_find_overlap_union_and_difference_of_polygons.py
   :fragment: print-difference

.. _pygplates_compare_two_positions_of_a_polygon:

Compare two positions of a polygon
++++++++++++++++++++++++++++++++++

This example compares where each static polygon was at 50Ma with where it is at present day. Two
polygons to compare could equally be the positions that two rotation models give the same feature,
or a reconstructed outline and one digitised from a map.

Sample code
"""""""""""

.. sample-code:: pygplates_find_overlap_union_and_difference_of_polygons_two_positions.py

Details
"""""""

The static polygons are reconstructed to 50Ma, as in the first example.

.. sample-code:: pygplates_find_overlap_union_and_difference_of_polygons_two_positions.py
   :fragment: reconstruct-snapshot

Each of the four operations returns a list of polygons that do not overlap each other, so one
function finds the area of any of the results.

.. sample-code:: pygplates_find_overlap_union_and_difference_of_polygons_two_positions.py
   :fragment: area-function

A :class:`pygplates.ReconstructedFeatureGeometry` has both of the polygons to compare:
:meth:`pygplates.ReconstructedFeatureGeometry.get_present_day_geometry` and
:meth:`pygplates.ReconstructedFeatureGeometry.get_reconstructed_geometry`.

.. sample-code:: pygplates_find_overlap_union_and_difference_of_polygons_two_positions.py
   :fragment: two-polygons

:meth:`pygplates.PolygonOnSphere.intersection` returns the region that the polygon covers at both times.

.. sample-code:: pygplates_find_overlap_union_and_difference_of_polygons_two_positions.py
   :fragment: intersection

:meth:`pygplates.PolygonOnSphere.difference` returns the region that the polygon covered at 50Ma but
does not cover now.

Unlike the other three operations, the order of the two polygons matters:
``present_day_polygon.difference(reconstructed_polygon)`` is the region that it covers now but
did not cover at 50Ma.

.. sample-code:: pygplates_find_overlap_union_and_difference_of_polygons_two_positions.py
   :fragment: difference

:meth:`pygplates.PolygonOnSphere.symmetric_difference` returns the region that the polygon covers
at one of the times but not the other. This is both of the differences above.

Its area measures how much two outlines of the same thing disagree: it is zero if they are identical.

.. sample-code:: pygplates_find_overlap_union_and_difference_of_polygons_two_positions.py
   :fragment: symmetric-difference

:meth:`pygplates.PolygonOnSphere.union` returns the region that the polygon covers at either time.

Two polygons that overlap, or share an edge, are merged into one polygon. Two that are apart, or
touch only at points, are returned as they are, so the list then has two polygons.

.. sample-code:: pygplates_find_overlap_union_and_difference_of_polygons_two_positions.py
   :fragment: union

The four areas are related: the area inside either polygon is the area inside both plus the area
inside one only.

.. sample-code:: pygplates_find_overlap_union_and_difference_of_polygons_two_positions.py
   :fragment: print-areas

A polygon returned by any of the operations has an interior ring wherever the region it covers has
a hole. For example, the union of two positions of a polygon that has a hole in it has a hole where
the two positions of that hole overlap.

.. sample-code:: pygplates_find_overlap_union_and_difference_of_polygons_two_positions.py
   :fragment: print-interior-rings

.. note:: Each ring of a polygon bounds the smaller of its two sides, so no ring can enclose more
   than a hemisphere. :meth:`pygplates.PolygonOnSphere.union` and
   :meth:`pygplates.PolygonOnSphere.symmetric_difference` raise :class:`pygplates.PartitionError` if
   an outer ring of their result would (two very large polygons that overlap, say).

.. note:: Pieces of a result that are narrower than about 9 metres are dropped. The
   *sliver_width_threshold_radians* argument of each operation changes that width, and zero keeps
   every piece.

See also
++++++++

- Reference: :meth:`pygplates.PolygonOnSphere.intersection`, :meth:`pygplates.PolygonOnSphere.union`,
  :meth:`pygplates.PolygonOnSphere.difference`, :meth:`pygplates.PolygonOnSphere.symmetric_difference`,
  :meth:`pygplates.PolygonOnSphere.partition`, :meth:`pygplates.PolygonOnSphere.get_area`,
  :class:`pygplates.ReconstructSnapshot`, :class:`pygplates.ReconstructedFeatureGeometry`
- Sample code: :ref:`pygplates_partition_features_into_plates`,
  :ref:`pygplates_find_features_overlapping_a_polygon`, :ref:`pygplates_reconstruct_regular_features`
