.. _pygplates_query_network_triangulation_and_rigid_blocks:

Query the triangulation and rigid blocks of deforming networks
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example looks inside resolved deforming networks: at the triangulation that deforms each
network, and at the rigid blocks inside it.

A deforming network, resolved at a geological time, has a boundary polygon, any number of interior
rigid blocks, and a triangulation. The vertices of the triangulation are the points of the boundary,
of the rigid blocks and of any other interior geometries, and each moves with the feature it comes from.

The network deforms as its vertices move together or apart, and the strain rate of each triangle
is calculated from the velocities of its three vertices.

The first example lists the rigid blocks, vertices and triangles of each network. The second finds
the triangle that contains a point, and compares the strain rate of that triangle with the strain
rate at the point.

.. contents::
   :local:
   :depth: 2

Data files
++++++++++

Both scripts read the same files:

``topologies.gpml``
    Deforming network features, and the features that their boundaries and interiors are built from.
    Topological plate polygon features can be in the file too. There must be networks at 10Ma,
    since the second script chooses its points from them. Leave out inactive networks, such as
    those in ``Zahirovic_etal_2022_Deforming_Networks_Inactive.gpml`` in the GPlates sample data,
    so that the points are chosen from networks that are deforming.

``rotations.rot``
    A rotation file. It must contain rotations for the plate IDs of the features that the deforming
    networks are built from, since those are what move the vertices of each triangulation.

Files of these kinds are in the GPlates `sample data <https://www.gplates.org/download/>`_.

List the rigid blocks, vertices and triangles of each network
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Sample code
"""""""""""

.. sample-code:: pygplates_query_network_triangulation_and_rigid_blocks.py

Details
"""""""

A :class:`topological model<pygplates.TopologicalModel>` is created from the topological features and the rotation file.

.. sample-code:: pygplates_query_network_triangulation_and_rigid_blocks.py
   :fragment: create-topological-model

The topological features are resolved at 10Ma using :meth:`pygplates.TopologicalModel.topological_snapshot`.

.. sample-code:: pygplates_query_network_triangulation_and_rigid_blocks.py
   :fragment: topological-snapshot

:meth:`pygplates.TopologicalSnapshot.get_resolved_topologies` returns the resolved topologies of the
types it is asked for.

With ``pygplates.ResolveTopologyType.network`` those are the deforming networks only, each a
:class:`pygplates.ResolvedTopologicalNetwork`.

.. sample-code:: pygplates_query_network_triangulation_and_rigid_blocks.py
   :fragment: resolved-networks

The plate ID and name of each network are read from its feature
(:meth:`pygplates.ResolvedTopologicalNetwork.get_feature`).

.. sample-code:: pygplates_query_network_triangulation_and_rigid_blocks.py
   :fragment: network-feature

:meth:`pygplates.ResolvedTopologicalNetwork.get_rigid_blocks` returns the interior rigid blocks of the
network, as a list of :class:`pygplates.ReconstructedFeatureGeometry`. The list is empty if the
network has none.

A rigid block is a polygon feature inside the network that does not deform. It moves as one piece,
with the reconstruction plate ID of its feature.

Its :meth:`reconstructed geometry<pygplates.ReconstructedFeatureGeometry.get_reconstructed_geometry>`
is its outline at 10Ma, which is a :class:`pygplates.PolygonOnSphere`.

:meth:`pygplates.PolygonOnSphere.get_area` returns an area on a sphere of unit radius, so it is
multiplied by the square of the Earth's radius to get square kilometres.

.. sample-code:: pygplates_query_network_triangulation_and_rigid_blocks.py
   :fragment: rigid-blocks

:meth:`pygplates.ResolvedTopologicalNetwork.get_network_triangulation` returns the triangulation of
the network, a :class:`pygplates.NetworkTriangulation`.

Its :meth:`vertices<pygplates.NetworkTriangulation.get_vertices>` and
:meth:`triangles<pygplates.NetworkTriangulation.get_triangles>` are each a read-only sequence: it
has a length, and it can be iterated over and indexed.

.. sample-code:: pygplates_query_network_triangulation_and_rigid_blocks.py
   :fragment: network-triangulation

Each vertex is a :class:`pygplates.NetworkTriangulation.Vertex`. Its
:attr:`position<pygplates.NetworkTriangulation.Vertex.position>` is a
:class:`pygplates.PointOnSphere`. Its
:meth:`velocity<pygplates.NetworkTriangulation.Vertex.get_velocity>` is a
:class:`pygplates.Vector3D`, in kilometres per million years unless the *velocity_units* argument
says otherwise. It is the velocity of the feature that the vertex comes from. (A vertex where two
sections of the boundary meet comes from two features, and has the average of their velocities.)

A vertex can be used as a key in a dictionary. Here a dictionary maps each vertex to its index in
the sequence of vertices, which the triangles need below.

.. sample-code:: pygplates_query_network_triangulation_and_rigid_blocks.py
   :fragment: vertices

Each triangle is a :class:`pygplates.NetworkTriangulation.Triangle`. Two more lists are filled from
the triangles.

.. sample-code:: pygplates_query_network_triangulation_and_rigid_blocks.py
   :fragment: iterate-triangles

Not every triangle is part of the deforming region of the network.

The triangulation covers the convex hull of its vertices, so where the boundary of the network is
concave there are triangles outside the boundary. There are also triangles inside each rigid block.

:attr:`pygplates.NetworkTriangulation.Triangle.is_in_deforming_region` is ``False`` for those, and
their strain rate is zero. They are skipped.

.. sample-code:: pygplates_query_network_triangulation_and_rigid_blocks.py
   :fragment: skip-non-deforming-triangles

:meth:`pygplates.NetworkTriangulation.Triangle.get_vertex` returns one of the three vertices of a
triangle, given an index of 0, 1 or 2. The dictionary turns each vertex into its index.

The :attr:`strain rate<pygplates.NetworkTriangulation.Triangle.strain_rate>` of a triangle is a
:class:`pygplates.StrainRate`. It is the same everywhere in the triangle.

.. sample-code:: pygplates_query_network_triangulation_and_rigid_blocks.py
   :fragment: triangle-vertices-and-strain-rate

The script now has the triangulation as three lists: the positions of the vertices, the three
vertex indices of each triangle, and a value (the strain rate) for each triangle.

That is the form in which plotting libraries usually take a triangular mesh. Here the lists are
only printed.

.. sample-code:: pygplates_query_network_triangulation_and_rigid_blocks.py
   :fragment: print-triangulation

.. note:: A vertex also has a :attr:`strain rate<pygplates.NetworkTriangulation.Vertex.strain_rate>`,
   which is an average of the strain rates of the deforming triangles around it, and it can list the
   :meth:`vertices<pygplates.NetworkTriangulation.Vertex.get_incident_vertices>` and
   :meth:`triangles<pygplates.NetworkTriangulation.Vertex.get_incident_triangles>` that it is joined to.

Find the triangle that contains a point
+++++++++++++++++++++++++++++++++++++++

This example finds where points are in the networks at 10Ma. For a point in the deforming region
of a network it finds the triangle that contains the point, and for a point in a rigid block it
finds the block. It chooses four points in the deforming regions, and one in a rigid block if
there is one, to run with.

Sample code
"""""""""""

.. sample-code:: pygplates_query_network_triangulation_and_rigid_blocks_locate_points.py

Details
"""""""

The topological model and the snapshot at 10Ma are the same as in the first example.

The points are chosen from the networks, rather than written into the script, so that the example
works with any topological features that have deforming networks at 10Ma. They are chosen from a
coarse grid of points, 10 degrees apart, from latitude 60 to -60. It stops at 60 degrees because,
in the GPlates sample data, the grid points nearer the poles happen to fall where the networks
barely deform at 10Ma, so they would make poor examples. The grid is scanned from north to south,
and the first points found are used.

The points are given as (latitude, longitude) tuples. They are positions at 10Ma, since they are
located in the topologies resolved at that time.

.. sample-code:: pygplates_query_network_triangulation_and_rigid_blocks_locate_points.py
   :fragment: load-inputs

:meth:`pygplates.TopologicalSnapshot.get_point_locations` returns the
:class:`location<pygplates.TopologyPointLocation>` of each point in the resolved topologies, in the
order of the points.

.. sample-code:: pygplates_query_network_triangulation_and_rigid_blocks_locate_points.py
   :fragment: point-locations

The grid points in the deforming region of a network, which
:meth:`pygplates.TopologyPointLocation.located_in_resolved_network_deforming_region` finds (see
below), are kept with their locations, and the first four of them are taken. If there are none,
the script stops with an error message that says so.

The first grid point in a rigid block of a network, which
:meth:`pygplates.TopologyPointLocation.located_in_resolved_network_rigid_block` finds (see below),
is taken too, if there is one.

.. sample-code:: pygplates_query_network_triangulation_and_rigid_blocks_locate_points.py
   :fragment: deforming-points

So each point is in a network, and a point in a network is either in one of its rigid blocks or in
its deforming region.

For a point in a rigid block,
:meth:`pygplates.TopologyPointLocation.located_in_resolved_network_rigid_block` returns a 2-tuple:
the resolved network, and the rigid block (one of those that
:meth:`pygplates.ResolvedTopologicalNetwork.get_rigid_blocks` returns). For any other point it
returns ``None``.

.. sample-code:: pygplates_query_network_triangulation_and_rigid_blocks_locate_points.py
   :fragment: rigid-block

For a point in the deforming region of a network,
:meth:`pygplates.TopologyPointLocation.located_in_resolved_network_deforming_region` returns the
resolved network.

With *return_network_triangle* set to ``True`` it returns a 2-tuple instead: the resolved network,
and the :class:`triangle<pygplates.NetworkTriangulation.Triangle>` of its triangulation that
contains the point.

Either way it returns ``None`` for a point that is not in the deforming region of a network. The
script does not test for that: a point here that is not in a rigid block is in a deforming region,
since that is how the points were chosen.

.. sample-code:: pygplates_query_network_triangulation_and_rigid_blocks_locate_points.py
   :fragment: network-triangle

The positions of the three vertices of the triangle are converted to (latitude, longitude) tuples.

.. sample-code:: pygplates_query_network_triangulation_and_rigid_blocks_locate_points.py
   :fragment: triangle-vertices

The strain rate of the triangle is compared with the strain rate at the point, which
:meth:`pygplates.ResolvedTopologicalNetwork.get_point_strain_rate` returns.

The two usually differ. By default the strain rate at a point is smoothed: it is interpolated from
the strain rates of the vertices near the point, each of which is an average over the triangles
around that vertex. So it varies from place to place within a triangle.

Without smoothing, the strain rate at a point is the strain rate of the triangle that contains it
(see :ref:`pygplates_clamp_and_smooth_strain_rates_and_set_rift_parameters`).

.. sample-code:: pygplates_query_network_triangulation_and_rigid_blocks_locate_points.py
   :fragment: strain-rates

:meth:`pygplates.NetworkTriangulation.Triangle.get_adjacent_triangle` returns the triangle on the
other side of one edge: the edge opposite the vertex at the given index.

It returns ``None`` if there is no triangle there, which happens at the outer edge of the triangulation.

The neighbours that are not in the deforming region are left out.

.. sample-code:: pygplates_query_network_triangulation_and_rigid_blocks_locate_points.py
   :fragment: adjacent-triangles

Finally the vertices of the triangle are printed, with the three kinds of strain rate.

.. sample-code:: pygplates_query_network_triangulation_and_rigid_blocks_locate_points.py
   :fragment: print-triangle

.. note:: The triangle that contains a point is found in a flat map projection of the network, where
   the edges of the triangles are not quite where they are on the globe. So a point near an edge can
   be given the triangle on the other side of that edge. For the same reason, a point in the
   deforming region is occasionally given a triangle whose
   :attr:`pygplates.NetworkTriangulation.Triangle.is_in_deforming_region` is ``False``. The strain
   rate of such a triangle is zero, while the smoothed strain rate at the point need not be. See
   :meth:`pygplates.TopologyPointLocation.located_in_resolved_network_deforming_region`.

See also
++++++++

- Primer: :ref:`pygplates_primer_topological_network`, :ref:`pygplates_primer_rigid_blocks`,
  :ref:`pygplates_primer_network_triangulation`, :ref:`pygplates_primer_strain_rates_in_triangulation`
- Reference: :meth:`pygplates.ResolvedTopologicalNetwork.get_network_triangulation`,
  :meth:`pygplates.ResolvedTopologicalNetwork.get_rigid_blocks`, :class:`pygplates.NetworkTriangulation`,
  :class:`pygplates.NetworkTriangulation.Triangle`, :class:`pygplates.NetworkTriangulation.Vertex`,
  :meth:`pygplates.TopologyPointLocation.located_in_resolved_network_deforming_region`,
  :meth:`pygplates.TopologyPointLocation.located_in_resolved_network_rigid_block`,
  :meth:`pygplates.ResolvedTopologicalNetwork.get_point_strain_rate`, :class:`pygplates.StrainRate`
- Sample code: :ref:`pygplates_find_plates_and_strain_rates_at_static_points`,
  :ref:`pygplates_clamp_and_smooth_strain_rates_and_set_rift_parameters`,
  :ref:`pygplates_reconstruct_strain_and_strain_rate`, :ref:`pygplates_create_topological_features`
