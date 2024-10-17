.. _pygplates_primer:

Primer
======

This document covers the main areas of pyGPlates functionality, and some plate tectonic foundations.

.. contents::
   :local:
   :depth: 3


.. _pygplates_primer_plate_reconstruction_hierarchy:

Plate reconstruction hierarchy
------------------------------

.. note:: `Next-generation plate-tectonic reconstructions using GPlates <http://www.gplates.org/publications.html>`_
   contains a good introduction to plate reconstruction hierarchies.

A plate-reconstruction hierarchy consists of a tree of total reconstruction poles at an instant in geological time.

Plate motions are described in terms of relative rotations between pairs of plates.
Every plate in the model moves relative to some other plate where, within each
of these plate pairs, one plate is considered the *moving* plate relative to the
other *fixed* plate. That *fixed* plate, in turn, moves relative to another plate
thus forming a tree-like structure known as the *reconstruction tree*.
Each of these *relative* rotations is an *edge* in the tree.

The following diagram shows a subset of the hierarchy of relative rotations used in GPlates:
::

                  000
                   |
                   |  finite rotation (001 rel. 000)
                   |
                  001
                   |
                   |  finite rotation (701 rel. 001)
                   |
                  701(AFR)
                  /|\
                 / | \  finite rotation (802 rel. 701)
                /  |  \
             201  702  802(ANT)
              /   / \    \
             /   /   \    \  finite rotation (801 rel. 802)
            /   /     \    \
         202  704     705  801(AUS)
         / \
        /   \
       /     \
     290     291

...where *000* is the anchored plate (the top of the reconstruction tree).
The edge *802 rel. 701* contains the rotation of *802* (the moving plate in the pair) relative to
*701* (the fixed plate in the pair).

An *equivalent* rotation is the rotation of a plate relative to the *anchored* plate.
So the equivalent rotation of plate *802* is the composition of relative rotations along the
plate circuit *edge* path from anchored plate *000* to plate *802*.

A *relative* rotation is the rotation of one plate relative to *another* plate
(as opposed to the *anchored* plate). Note that, like *equivalent* rotations,
the plate circuit *edge* path can consist of one or more edges.
For example, the rotation of plate *801* relative to plate *291* follows an *edge*
path that goes via plates *202*, *201*, *701* and *802*. However it should be noted
that if the edge between *001* and *701* did not exist then, even though a path
would still exist between *291* and *801*, the *relative* rotation (and *equivalent*
rotations of *291* and *801* for that matter) would be an :meth:`identity rotation
<pygplates.FiniteRotation.represents_identity_rotation>`. This is because the sub-tree
below *001* would not get built into the reconstruction tree and hence all plates
in the sub-tree would be missing. This can happen when the rotation sequence
for a moving/fixed plate pair (eg, *701*/*101*) does not span a large enough time
period. You can work around this situation by setting the anchor plate to the relative plate
(eg, *291* in the above example).

A *total* rotation is a rotation at a time in the past relative to *present day* (0Ma).
In other words *from* present day *to* a past time.

A *stage* rotation is a rotation at a time in the past relative to *another* time
in the past.


.. _pygplates_primer_working_with_finite_rotations:

Working with finite rotations
-----------------------------

A finite rotation represents the motion of a plate (relative to another plate) on the surface of the
globe over a period of geological time.

In pyGPlates, finite rotations are represented by :class:`pygplates.FiniteRotation`.

In the following sections we will first cover some rotation maths and then derive the four
fundamental finite rotation categories:

* :ref:`pygplates_primer_equivalent_total_rotation`
* :ref:`pygplates_primer_relative_total_rotation`
* :ref:`pygplates_primer_equivalent_stage_rotation`
* :ref:`pygplates_primer_relative_stage_rotation`

In pyGPlates, these can be obtained from a :class:`pygplates.RotationModel`.


.. _pygplates_primer_composing_finite_rotations:

Composing finite rotations
^^^^^^^^^^^^^^^^^^^^^^^^^^

In the following examples a composed rotation :math:`R2 \times R1` means the rotation :math:`R1`
is the first rotation to be applied followed by the rotation :math:`R2` such that a geometry is
rotated in the following way:

.. math::

   \text{geometry_final} &= R2 \times(R1 \times \text{geometry_initial}) \\
                         &= R2 \times R1 \times \text{geometry_initial}

...which is the equivalent of...

.. math::

   \text{geometry_intermediate} &= R1 \times \text{geometry_initial} \\
   \text{geometry_final} &= R2 \times \text{geometry_intermediate} \\
                         &= R2 \times (R1 \times \text{geometry_initial}) \\
                         &= R2 \times R1 \times \text{geometry_initial}

.. note:: Rotations are *not* commutative (:math:`R2 \times R1 \neq R1 \times R2`)

The composed rotation :math:`R2 \times R1` can be written in pyGPlates as either:
::

  R2 * R1

...or...
::

  pygplates.FiniteRotation.compose(R2, R1)

For example, the above geometry rotation can be written as either:
::

  geometry_final = R2 * R1 * geometry_initial

...or...
::

  geometry_final = pygplates.FiniteRotation.compose(R2, R1) * geometry_initial


.. _pygplates_primer_plate_circuit_paths:

Plate circuit paths
^^^^^^^^^^^^^^^^^^^

The rotation from present day (0Ma) to the *geological time* :math:`t_{2}` (via time :math:`t_{1}`) is given by:

.. math::

   R(0 \rightarrow t_{2}) = R(t_{1} \rightarrow t_{2}) \times R(0 \rightarrow t_{1})

...or by post-multiplying both sides by :math:`R(t_{1} \rightarrow 0)`, and then swapping sides, this becomes...

.. math::

   R(0 \rightarrow t_{2}) \times R(t_{1} \rightarrow 0) &= R(t_{1} \rightarrow t_{2}) \times R(0 \rightarrow t_{1}) \times R(t_{1} \rightarrow 0) \\
   R(0 \rightarrow t_{2}) \times R(t_{1} \rightarrow 0) &= R(t_{1} \rightarrow t_{2}) \\
   R(t_{1} \rightarrow t_{2}) &= R(0 \rightarrow t_{2}) \times R(t_{1} \rightarrow 0)

The *plate circuit path* rotation from anchor plate :math:`P_{A}` to moving plate :math:`P_{M}` (via fixed plate :math:`P_{F}`) is given by:

.. math::

   R(P_{A} \rightarrow P_{M}) = R(P_{A} \rightarrow P_{F}) \times R(P_{F} \rightarrow P_{M})

...or by pre-multiplying both sides by :math:`R(P_{F} \rightarrow P_{A})` this becomes...

.. math::

   R(P_{F} \rightarrow P_{A}) \times R(P_{A} \rightarrow P_{M}) &= R(P_{F} \rightarrow P_{A}) \times R(P_{A} \rightarrow P_{F}) \times R(P_{F} \rightarrow P_{M}) \\
   R(P_{F} \rightarrow P_{A}) \times R(P_{A} \rightarrow P_{M}) &= R(P_{F} \rightarrow P_{M}) \\
   R(P_{F} \rightarrow P_{M}) &= R(P_{F} \rightarrow P_{A}) \times R(P_{A} \rightarrow P_{M})

Note that the rotations for relative times and for relative plates have the opposite order of each other !

In other words:

* For times :math:`0 \rightarrow t_{1} \rightarrow t_{2}` you apply the :math:`0 \rightarrow t_{1}` rotation first followed by the :math:`t_{1} \rightarrow t_{2}` rotation:
  
  .. math::

     R(0 \rightarrow t_{2})  = R(t_{1} \rightarrow t_{2}) \times R(0 \rightarrow t_{1})

* For plate circuit :math:`P_{A} \rightarrow P_{F} \rightarrow P_{M}` you apply the :math:`P_{F} \rightarrow P_{M}` rotation first followed by the :math:`P_{A} \rightarrow P_{F}` rotation:
  
  .. math::

     R(P_{A} \rightarrow P_{M}) = R(P_{A} \rightarrow P_{F}) \times R(P_{F} \rightarrow P_{M})

  .. note:: This is not :math:`P_{A} \rightarrow P_{F}` followed by :math:`P_{F} \rightarrow P_{M}` as you might expect (looking at the time example).

This is probably best explained by the difference between thinking in terms of the grand fixed
coordinate system and local coordinate system (see http://glprogramming.com/red/chapter03.html#name2).
Essentially, in the plate circuit :math:`P_{A} \rightarrow P_{F} \rightarrow P_{M}`, the :math:`P_{F} \rightarrow P_{M}` rotation can be thought of as a rotation
within the local coordinate system of :math:`P_{A} \rightarrow P_{F}`. In other words :math:`P_{F} \rightarrow P_{M}` is not a rotation that
occurs relative to the global spin axis but a rotation relative to the local coordinate system
of plate :math:`P_{F}` *after* it has been rotated relative to the anchor plate :math:`P_{A}`.

For the times :math:`0 \rightarrow t_{1} \rightarrow t_{2}` this local/relative coordinate system concept does not apply.

Note that a rotation must be relative to present day (0Ma) before it can be separated into a (plate circuit) chain of moving/fixed plate pairs.
Hence :math:`R(t_{1} \rightarrow t_{2},P_{A} \rightarrow P_{C}) \neq R(t_{1} \rightarrow t_{2},P_{A} \rightarrow P_{B}) \times R(t_{1} \rightarrow t_{2},P_{B} \rightarrow P_{C})`
demonstrates this mistake.

The following shows the correct way to separate :math:`P_{A} \rightarrow P_{C}` into the (plate circuit) chain of moving/fixed plate pairs :math:`P_{A} \rightarrow P_{B}` and :math:`P_{B} \rightarrow P_{C}`...

.. math::

   R(t_{1} \rightarrow t_{2},P_{A} \rightarrow P_{C}) \\
   & = R(0 \rightarrow t_{2},P_{A} \rightarrow P_{C}) \times R(t_{1} \rightarrow 0,P_{A} \rightarrow P_{C}) \\
   & = R(0 \rightarrow t_{2},P_{A} \rightarrow P_{C}) \times R(0 \rightarrow t_{1},P_{A} \rightarrow P_{C})^{-1} \\
   &   \text{// Now that all times are relative to 0Ma we can split } P_{A} \rightarrow P_{C} \text{ into } P_{A} \rightarrow P_{B} \rightarrow P_{C} \text{ ...} \\
   & = R(0 \rightarrow t_{2},P_{A} \rightarrow P_{B}) \times R(0 \rightarrow t_{2},P_{B} \rightarrow P_{C}) \times [R(0 \rightarrow t_{1},P_{A} \rightarrow P_{B}) \times R(0 \rightarrow t_{1},P_{B} \rightarrow P_{C})]^{-1} \\
   & = R(0 \rightarrow t_{2},P_{A} \rightarrow P_{B}) \times R(0 \rightarrow t_{2},P_{B} \rightarrow P_{C}) \times R(0 \rightarrow t_{1},P_{B} \rightarrow P_{C})^{-1} \times R(0 \rightarrow t_{1},P_{A} \rightarrow P_{B})^{-1}

...where :math:`P_{A} \rightarrow P_{B} \rightarrow P_{C}` means :math:`P_{B} \rightarrow P_{C}` is the rotation of :math:`P_{C}` relative to :math:`P_{B}` and :math:`P_{A} \rightarrow P_{B}` is
the rotation of :math:`P_{B}` relative to :math:`P_{A}`. The need for rotation :math:`P_{A} \rightarrow P_{C}` to be relative
to present day (0Ma) before it can be split into :math:`P_{A} \rightarrow P_{B}` and :math:`P_{B} \rightarrow P_{C}` is because
:math:`P_{A} \rightarrow P_{B}` and :math:`P_{B} \rightarrow P_{C}` are defined (in the rotation file) as total reconstruction
poles which are always relative to present day.

.. note:: | The inverse of rotation :math:`R` is denoted :math:`R^{-1}`.
          | Such that :math:`R \times R^{-1} = R^{-1} \times R = I` where :math:`I` is the :meth:`identify rotation<pygplates.FiniteRotation.represents_identity_rotation>`.


.. _pygplates_primer_equivalent_total_rotation:

Equivalent total rotation
^^^^^^^^^^^^^^^^^^^^^^^^^

The equivalent total rotation of moving plate :math:`P_{M}` relative to anchor plate :math:`P_{A}`, and
from present day time :math:`0` to time :math:`t_{to}` is:

.. math::

   R(0 \rightarrow t_{to},P_{A} \rightarrow P_{M})

In pyGPlates, the equivalent total rotation can be obtained :meth:`pygplates.RotationModel.get_rotation` as:
::

  rotation_model = pygplates.RotationModel(...)
  ...
  equivalent_total_rotation = rotation_model.get_rotation(to_time, moving_plate)


.. _pygplates_primer_relative_total_rotation:

Relative total rotation
^^^^^^^^^^^^^^^^^^^^^^^

The relative total rotation of moving plate :math:`P_{M}` relative to fixed plate :math:`P_{F}`, and
from present day time :math:`0` to time :math:`t_{to}` is:

.. math::

   R(0 \rightarrow t_{to},P_{F} \rightarrow P_{M}) \\
   &  = R(0 \rightarrow t_{to},P_{F} \rightarrow P_{A}) \times R(0 \rightarrow t_{to},P_{A} \rightarrow P_{M}) \\
   &  = R(0 \rightarrow t_{to},P_{A} \rightarrow P_{F})^{-1} \times R(0 \rightarrow t_{to},P_{A} \rightarrow P_{M})

...where :math:`P_{A}` is the anchor plate.

In pyGPlates, the relative total rotation can be obtained from :meth:`pygplates.RotationModel.get_rotation` as:
::

  rotation_model = pygplates.RotationModel(...)
  ...
  relative_total_rotation = rotation_model.get_rotation(to_time, moving_plate, fixed_plate_id=fixed_plate)


.. _pygplates_primer_equivalent_stage_rotation:

Equivalent stage rotation
^^^^^^^^^^^^^^^^^^^^^^^^^

The equivalent stage rotation of moving plate :math:`P_{M}` relative to anchor plate :math:`P_{A}`, and
from time :math:`t_{from}` to time :math:`t_{to}` is:

.. math::

   R(t_{from} \rightarrow t_{to},P_{A} \rightarrow P_{M}) \\
   &  = R(0 \rightarrow t_{to},P_{A} \rightarrow P_{M}) \times R(t_{from} \rightarrow 0,P_{A} \rightarrow P_{M}) \\
   &  = R(0 \rightarrow t_{to},P_{A} \rightarrow P_{M}) \times R(0 \rightarrow t_{from},P_{A} \rightarrow P_{M})^{-1}

In pyGPlates, the equivalent stage rotation can be obtained :meth:`pygplates.RotationModel.get_rotation` as:
::

  rotation_model = pygplates.RotationModel(...)
  ...
  equivalent_stage_rotation = rotation_model.get_rotation(to_time, moving_plate, from_time)


.. _pygplates_primer_relative_stage_rotation:

Relative stage rotation
^^^^^^^^^^^^^^^^^^^^^^^

The relative stage rotation of moving plate :math:`P_{M}` relative to fixed plate :math:`P_{F}`, and
from time :math:`t_{from}` to time :math:`t_{to}` is:

.. math::

   R(t_{from} \rightarrow t_{to},P_{F} \rightarrow P_{M}) \\
   &  = R(0 \rightarrow t_{to},P_{F} \rightarrow P_{M}) \times R(t_{from} \rightarrow 0,P_{F} \rightarrow P_{M}) \\
   &  = R(0 \rightarrow t_{to},P_{F} \rightarrow P_{M}) \times R(0 \rightarrow t_{from},P_{F} \rightarrow P_{M})^{-1} \\
   &  = R(0 \rightarrow t_{to},P_{F} \rightarrow P_{A}) \times R(0 \rightarrow t_{to},P_{A} \rightarrow P_{M}) \times [R(0 \rightarrow t_{from},P_{F} \rightarrow P_{A}) \times R(0 \rightarrow t_{from},P_{A} \rightarrow P_{M})]^{-1} \\
   &  = R(0 \rightarrow t_{to},P_{A} \rightarrow P_{F})^{-1} \times R(0 \rightarrow t_{to},P_{A} \rightarrow P_{M}) \times [R(0 \rightarrow t_{from},P_{A} \rightarrow P_{F})^{-1} \times R(0 \rightarrow t_{from},P_{A} \rightarrow P_{M})]^{-1} \\
   &  = R(0 \rightarrow t_{to},P_{A} \rightarrow P_{F})^{-1} \times R(0 \rightarrow t_{to},P_{A} \rightarrow P_{M}) \times R(0 \rightarrow t_{from},P_{A} \rightarrow P_{M})^{-1} \times R(0 \rightarrow t_{from},P_{A} \rightarrow P_{F})

...where :math:`P_{A}` is the anchor plate.

In pyGPlates, the relative stage rotation can be obtained :meth:`pygplates.RotationModel.get_rotation` as:
::

  rotation_model = pygplates.RotationModel(...)
  ...
  relative_stage_rotation = rotation_model.get_rotation(to_time, moving_plate, from_time, fixed_plate)



.. _pygplates_primer_deformation:

Deformation
-----------

This section covers deformation in pyGPlates.

.. contents::
   :local:
   :depth: 2


.. _pygplates_primer_deformation_topological_network:

Topological network
^^^^^^^^^^^^^^^^^^^

To model deformation, a topological network must first be created. This consists of a boundary polygon
(resolved by intersecting boundary line segments, similar to topological closed plate polygons), optional interior rigid blocks,
individual deforming points, and a deforming region (triangulation with vertices from boundary, rigid blocks and deforming points).

.. figure:: images/DeformingNetworkDiagram.png

   On the left are the elements that make up a topological network.
   On the right is the resolving of these elements at a reconstruction time to form a resolved topological network.

More information on topological networks in GPlates/pyGPlates can be found in the following paper:

* Michael Gurnis, Ting Yang, John Cannon, Mark Turner, Simon Williams, Nicolas Flament, R. Dietmar Müller, 2018,
  `Global tectonic reconstructions with continuously deforming and evolving rigid plates <https://doi.org/10.1016/j.cageo.2018.04.007>`_,
  **Computers & Geosciences,** 116, 32-41, doi: 10.1016/j.cageo.2018.04.007

.. _pygplates_primer_deformation_rigid_blocks:

Rigid blocks
^^^^^^^^^^^^

A topological network can *optionally* have interior islands that are rigid (unlike the :ref:`deforming triangulation <pygplates_primer_deformation_deforming_triangulation>`).

.. note:: Any :meth:`interior geometry of a network <pygplates.GpmlTopologicalSection.create_network_interior>` that is a *polygon* is considered a rigid block.

Each rigid block is represented by a :class:`pygplates.ReconstructedFeatureGeometry`, and is obtained from a :class:`pygplates.ResolvedTopologicalNetwork` with:
::

   rigid_blocks = resolved_topological_network.get_rigid_blocks()

For example, you can get the plate ID and boundary polygon of each interior rigid block (if any):
::

  for rigid_block in rigid_blocks:
      rigid_block_plate_id = rigid_block.get_feature().get_reconstruction_plate_id()
      rigid_block_boundary = rigid_block.get_reconstructed_geometry()

.. _pygplates_primer_deformation_deforming_triangulation:

Deforming triangulation
^^^^^^^^^^^^^^^^^^^^^^^

A deforming triangulation represents the *deforming* region of a :class:`resolved topological network <pygplates.ResolvedTopologicalNetwork>`.

It is created by first forming the Delaunay triangulation of vertices obtained from the network's boundary (polygon), and any interior rigid blocks (polygons) and
any interior geometries (points or lines). The Delaunay triangulation is the convex hull around the network boundary, so it includes triangles outside the
network boundary (and also triangles inside any non-deforming interior blocks). To limit the triangulation to only the deforming region, only those triangles
whose centroid is *inside* the deforming region are retained (the rest are excluded from the deforming triangulation). Note that, for this purpose, the deforming
region is defined to be *inside* the network's boundary polygon but *outside* any interior rigid block polygons.

.. note:: The Delaunay triangulation is not a *constrained* triangulation. This means the edges of some Delaunay triangles can cross over network boundary edges or
   interior block edges, rather than be constrained to follow them. However the removal of Delaunay triangles, with centroids *outside* the deforming region, deals
   with this quite effectively for current topological network datasets.

The triangles in a deforming triangulation do not overlap any :ref:`interior rigid blocks <pygplates_primer_deformation_rigid_blocks>` (other than the above-mentioned
note about *constrained* triangulations). In other words, the deforming triangulation represents the *deforming* region of a
:class:`resolved topological network <pygplates.ResolvedTopologicalNetwork>` and the rigid blocks (if any) represent the *rigid* regions.

A deforming triangulation is represented by a :class:`pygplates.DeformingTriangulation`, and is obtained from a :class:`pygplates.ResolvedTopologicalNetwork` with:
::

   deforming_triangulation = resolved_topological_network.get_deforming_triangulation()

It consists of a sequence of vertices and a sequence of triangles. Each vertex is represented by a :class:`pygplates.DeformingTriangulation.Vertex` and contains a position,
a velocity and a strain rate. Each triangle is represented by a :class:`pygplates.DeformingTriangulation.Triangle` and contains three vertex indices and a strain rate.
A triangle's three vertex indices are indices into the sequence of vertices of the triangulation.
::

   triangles = deforming_triangulation.get_triangles()
   vertices = deforming_triangulation.get_vertices()

   for triangle in triangles:
      triangle_vertex_0 = vertices[triangle.get_vertex_index(0)]
      triangle_vertex_1 = vertices[triangle.get_vertex_index(1)]
      triangle_vertex_2 = vertices[triangle.get_vertex_index(2)]
      triangle_strain_rate = triangle.strain_rate

   for vertex in vertices:
      vertex_position = vertex.position
      vertex_velocity = vertex.velocity
      vertex_strain_rate = vertex.strain_rate

.. _pygplates_primer_deformation_strain_rates_in_triangulation:

Strain rates in triangulation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Each :class:`triangle <pygplates.DeformingTriangulation.Triangle>` in a :class:`deforming triangulation <pygplates.DeformingTriangulation>` is assigned a :class:`strain rate <pygplates.StrainRate>`
that is *constant* across the triangle. Furthermore, the strain rate of each triangle can optionally be :ref:`clamped to a maximum strain rate <pygplates_primer_deformation_strain_rate_clamping>`.
Then each :class:`vertex <pygplates.DeformingTriangulation.Vertex>` in the triangulation is assigned a strain rate that is an area-weighted average of the (potentially clamped) strain rates
from triangles incident to the vertex.

Finally, the strain rate that is queried at an *arbitrary* location (within the deforming triangulation) is either assigned the strain rate of the triangle containing that location,
or calculated by interpolating the strain rates of nearby vertices if :ref:`strain rates are smoothed <pygplates_primer_deformation_strain_rate_smoothing>`.

.. note:: Both strain rate :ref:`clamping <pygplates_primer_deformation_strain_rate_clamping>` and :ref:`smoothing <pygplates_primer_deformation_strain_rate_smoothing>` affect strain *rate* queries
   (such as :meth:`pygplates.ReconstructedGeometryTimeSpan.get_strain_rates`). They also affects *strain* queries (such as :meth:`pygplates.ReconstructedGeometryTimeSpan.get_strains`),
   since strain is :meth:`accumulated <pygplates.Strain.accumulate>` from strain rate.

.. _pygplates_primer_deformation_strain_rate_clamping:

Strain rate clamping
""""""""""""""""""""

Strain rates can optionally be clamped to a maximum strain rate to avoid excessive or spurious extension/compression in some triangles of a deforming triangulation.
This can happen in some topological networks depending on how they were built.

It is the :meth:`total strain rate <pygplates.StrainRate.get_total_strain_rate>` that is clamped, since it includes both the normal and shear components of deformation.
When a strain rate is clamped, all components of its tensor (specifically its :class:`spatial gradients of velocity tensor <pygplates.StrainRate.get_velocity_spatial_gradient>`)
are scaled equally to ensure its total strain rate equals the maximum total strain rate.

.. note:: Clamping the total strain rate also limits quantities derived from strain rate such as crustal thinning and tectonic subsidence.

Strain rate clamping is determined by :attr:`pygplates.ResolveTopologyParameters.enable_strain_rate_clamping` when topological networks are resolved at a reconstruction time
(using :class:`pygplates.TopologicalModel`, :class:`pygplates.TopologicalSnapshot` or :func:`pygplates.resolve_topologies`).
And the maximum strain rate is :attr:`pygplates.ResolveTopologyParameters.max_clamped_strain_rate`.

.. _pygplates_primer_deformation_strain_rate_smoothing:

Strain rate smoothing
"""""""""""""""""""""

Strain rates can optionally be smoothed to help reduce the faceted (piecewise constant) strain rate across a deforming triangulation (due to each triangle having a *constant* strain rate across its face).

.. note:: Smoothing the strain rate also affects quantities derived from strain rate such as crustal thinning and tectonic subsidence.

Strain rate smoothing is determined by :attr:`pygplates.ResolveTopologyParameters.strain_rate_smoothing` when topological networks are resolved at a reconstruction time
(using :class:`pygplates.TopologicalModel`, :class:`pygplates.TopologicalSnapshot` or :func:`pygplates.resolve_topologies`).
The strain rate at an arbitrary location within a deforming triangulation is affected by the smoothing value:

* ``pygplates.StrainRateSmoothing.none`` - No smoothing. The strain rate is equal to the (constant) strain rate of the :class:`triangle <pygplates.DeformingTriangulation.Triangle>` containing the query location.
* ``pygplates.StrainRateSmoothing.barycentric`` - Use linear interpolation of the strain rates of the 3 :class:`vertices <pygplates.DeformingTriangulation.Vertex>` of the
  :class:`triangle <pygplates.DeformingTriangulation.Triangle>` containing the query location.
* ``pygplates.StrainRateSmoothing.natural_neighbour`` - Use natural neighbour interpolation of the strain rates of triangulation :class:`vertices <pygplates.DeformingTriangulation.Vertex>` near the query location.

.. _pygplates_primer_deformation_exponential_rift_stretching_profile:

Exponential rift stretching profile
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

A rift is typically modeled using two topological networks, one on each side of the rift axis. Each side of the rift axis typically has a single row of triangles (between the un-stretched side and the rift axis).
As a result, the strain rate at any location within the rift will essentially be *constant*, even when the :ref:`strain rates are smoothed <pygplates_primer_deformation_strain_rate_smoothing>`.
This is because triangulation vertices, along both the un-stretched boundary line and the rift axis, will effectively end up with the strain rate of the triangles (which is constant across each triangle).

To avoid the problem of *constant* stretching across the rift, an *exponential* rift stretching profile can be activated by adding rift left/right plate ID properties to a topological network feature.

Internally the exponential strain rate profile is implemented by automatically adding more points to the interior of a deforming triangulation and distributing the velocities
at these points such that the strain rate varies exponentially (along the stretching direction) from the un-stretched side of the rift towards the rift axis.

.. note:: This works reasonably well for regular rifts (like AFR-SAM), but not as well for oblique rifts (like AUS-ANT).

.. note:: The exponential rift stretching profile affects quantities derived from strain rate such as crustal thinning and tectonic subsidence.

Rift left/right plate IDs
"""""""""""""""""""""""""

An *exponential* rift stretching profile is activated by adding a ``gpml:riftLeftPlate``/``gpml:riftRightPlate`` pair of conjugate plate ID properties to a topological network :class:`pygplates.Feature`.
This can be done, for example, by using the *rift_parameters* argument of :meth:`pygplates.Feature.create_topological_network_feature`.
The presence of these plate IDs triggers the internal generation of an exponential strain rate rift profile when the topological networks are resolved at a reconstruction time
(using :class:`pygplates.TopologicalModel`, :class:`pygplates.TopologicalSnapshot` or :func:`pygplates.resolve_topologies`).

For example, to create a rift between Africa and South America:
::

  SAM_rift_network = pygplates.GpmlTopologicalNetwork([...])
  SAM_rift_feature = pygplates.Feature.create_topological_network_feature(
      SAM_rift_network,
      name='SAM rift',
      valid_time=(145, 115),
      rift_parameters=(201, 701))
  SAM_rift_feature.set_reconstruction_plate_id(201)

  AFR_rift_network = pygplates.GpmlTopologicalNetwork([...])
  AFR_rift_feature = pygplates.Feature.create_topological_network_feature(
      AFR_rift_network,
      name='AFR rift',
      valid_time=(145, 115),
      rift_parameters=(201, 701))
  AFR_rift_feature.set_reconstruction_plate_id(701)

.. note:: If the rift left/right plate ID properties are not present in a topological network feature then it is *not* considered a *rift*.

There are also three other parameters, in addition to the rift left/right plate IDs, that are optional and can either be set individually in each a topological network feature
(eg, using the *rift_parameters* argument of :meth:`pygplates.Feature.create_topological_network_feature`) or as default values for all topological network features
(using :class:`pygplates.ResolveTopologyParameters`).

.. note:: If these parameters are set in both places, then the feature properties have precedence.

When set on a topological network feature they become feature properties named:

* ``gpml:riftExponentialStretchingConstant``
* ``gpml:riftStrainRateResolutionLog10`` (note that this is :math:`\log_{10}` of the rift strain rate resolution)
* ``gpml:riftEdgeLengthThresholdDegrees``

...and for features missing these properties these parameters are instead obtained from :class:`pygplates.ResolveTopologyParameters` attributes:

* :attr:`pygplates.ResolveTopologyParameters.rift_exponential_stretching_constant`
* :attr:`pygplates.ResolveTopologyParameters.rift_strain_rate_resolution`
* :attr:`pygplates.ResolveTopologyParameters.rift_edge_length_threshold_degrees`

...when the topological networks are resolved at a reconstruction time
(using :class:`pygplates.TopologicalModel`, :class:`pygplates.TopologicalSnapshot` or :func:`pygplates.resolve_topologies`).

Rift exponential stretching constant
""""""""""""""""""""""""""""""""""""

The strain rate in the rift stretching direction varies exponentially from the un-stretched side of the rift towards the rift axis.
The spatial variation in strain rate is:

  .. math::

     strain\_rate(x) = strain\_rate \times e^{C x} \frac{C}{e^C - 1}

...where :math:`strain\_rate` is the un-subdivided, original (constant) strain rate, :math:`C` is the *rift exponential stretching constant*
and :math:`x = 0` at the un-stretched side and :math:`x = 1` at the stretched point. Therefore :math:`strain\_rate(0) < strain\_rate < strain\_rate(1)`.
For example, when :math:`C = 1.0` then :math:`strain\_rate(0) = 0.58 \times strain\_rate` and :math:`strain\_rate(1) = 1.58 \times strain\_rate`.

Rift strain rate resolution
"""""""""""""""""""""""""""

The *rift strain rate resolution* controls how accurately the actual strain rate curve (across rift profile) matches the exponential curve (in units of :math:`second^{-1}`).
Rift edges in the network triangulation are sub-divided until the strain rate matches the exponential curve (within this tolerance).

Rift edge length threshold
""""""""""""""""""""""""""

Rift edges in network triangulation shorter than the *rift edge length threshold* (in degrees) will not be further sub-divided.
