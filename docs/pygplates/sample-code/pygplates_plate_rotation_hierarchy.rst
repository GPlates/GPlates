.. _pygplates_plate_rotation_hierarchy:

Hierarchy of plate rotations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example traverses the :ref:`plate rotation hierarchy<pygplates_primer_plate_reconstruction_hierarchy>`
at a reconstruction time, and prints the relative and equivalent total rotations of each plate.

The output is similar to the ``Reconstruction Tree`` tab of the ``Total Reconstruction Poles``
dialog in `GPlates <http://www.gplates.org>`_.

.. seealso:: :ref:`pygplates_plate_circuits_to_anchored_plate`

.. contents::
   :local:
   :depth: 2

Data files
""""""""""

``rotations.rot``
    A rotation file. Rotation files are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_plate_rotation_hierarchy.py

Details
"""""""

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.

.. sample-code:: pygplates_plate_rotation_hierarchy.py
   :fragment: load-rotations

The :ref:`plate rotation hierarchy<pygplates_primer_plate_reconstruction_hierarchy>` at a
reconstruction time is held in a :class:`reconstruction tree<pygplates.ReconstructionTree>`.
:meth:`pygplates.RotationModel.get_reconstruction_tree` returns the tree at the reconstruction
time, here 60Ma.

The hierarchy can differ from one reconstruction time to the next, depending on how the rotations
are arranged in the rotation file.

.. sample-code:: pygplates_plate_rotation_hierarchy.py
   :fragment: reconstruction-tree

An :class:`edge<pygplates.ReconstructionTreeEdge>` of the reconstruction tree is the rotation of a
moving plate relative to a fixed plate. The edges form a tree whose root is the anchor plate
(plate ID zero, unless another is asked for).

The anchor plate edges are the edges at the top of the tree, and
:meth:`pygplates.ReconstructionTree.get_anchor_plate_edges` returns them.

.. sample-code:: pygplates_plate_rotation_hierarchy.py
   :fragment: anchor-plate-edges

The anchor plate edges all have the same fixed plate, the anchor plate, and each has a different
moving plate. Each of those moving plates is the root of a sub-tree of the reconstruction tree.

The script traverses the sub-tree of each anchor plate edge, starting at a depth of zero.

.. sample-code:: pygplates_plate_rotation_hierarchy.py
   :fragment: traverse-sub-trees

The traversal is done by a function. It visits one edge, and then the sub-tree below the moving
plate of that edge.

It is a function because it is recursive (it calls itself), which is the simplest way to traverse a
tree.

Its ``depth`` argument is the depth of the edge below the anchor plate. It is used only to indent
the printed lines.

::

    def traverse_sub_tree(edge, depth):
        ...

The function first gets the :ref:`relative<pygplates_primer_relative_total_rotation>` and
:ref:`equivalent<pygplates_primer_equivalent_total_rotation>` total rotations of the edge, with
:meth:`pygplates.ReconstructionTreeEdge.get_relative_total_rotation` and
:meth:`pygplates.ReconstructionTreeEdge.get_equivalent_total_rotation`:

- The relative total rotation is the rotation of the moving plate of the edge relative to its fixed plate.
- The equivalent total rotation is the rotation of the moving plate of the edge relative to the anchor plate.

A *total* rotation is a rotation from present day (0Ma) to the reconstruction time.

:meth:`pygplates.FiniteRotation.get_lat_lon_euler_pole_and_angle_degrees` returns the pole and
angle of each rotation.

.. sample-code:: pygplates_plate_rotation_hierarchy.py
   :fragment: relative-and-equivalent-rotations

The function then prints the moving and fixed plate IDs of the edge, and its two rotations. The
indent is ``prefix_padding``, which is two spaces for each level of depth.

.. sample-code:: pygplates_plate_rotation_hierarchy.py
   :fragment: print-rotations

Just as the anchor plate has anchor plate edges below it, the moving plate of an edge can have
child edges below it. :meth:`pygplates.ReconstructionTreeEdge.get_child_edges` returns them.

The function calls itself on each child edge, with a depth that is one greater. In this way it
visits every edge, and so every plate, of the sub-tree.

The recursion stops at an edge that has no child edges. No plate moves relative to the moving plate
of that edge.

.. sample-code:: pygplates_plate_rotation_hierarchy.py
   :fragment: recurse-into-child-edges

Output
""""""

::

  Plate ID: 1, Fixed Plate ID: 0:
    Rotation rel. fixed (parent) plate: lat: 90.000000, lon: 0.000000:, angle:0.000000
    Equivalent rotation rel. anchored plate: lat: 90.000000, lon: 0.000000:, angle:0.000000
  
    Plate ID: 701, Fixed Plate ID: 1:
      Rotation rel. fixed (parent) plate: lat: 23.730000, lon: -42.140000:, angle:-12.530000
      Equivalent rotation rel. anchored plate: lat: 23.730000, lon: -42.140000:, angle:-12.530000
  
      Plate ID: 201, Fixed Plate ID: 701:
        Rotation rel. fixed (parent) plate: lat: 62.238025, lon: -32.673047:, angle:23.349295
        Equivalent rotation rel. anchored plate: lat: 77.493750, lon: 57.067142:, angle:15.711412
  
        Plate ID: 202, Fixed Plate ID: 201:
          Rotation rel. fixed (parent) plate: lat: 90.000000, lon: 0.000000:, angle:0.000000
          Equivalent rotation rel. anchored plate: lat: 77.493750, lon: 57.067142:, angle:15.711412
  
          Plate ID: 290, Fixed Plate ID: 202:
            Rotation rel. fixed (parent) plate: lat: 90.000000, lon: 0.000000:, angle:0.000000
            Equivalent rotation rel. anchored plate: lat: 77.493750, lon: 57.067142:, angle:15.711412
  
        Plate ID: 203, Fixed Plate ID: 201:
          Rotation rel. fixed (parent) plate: lat: 90.000000, lon: 0.000000:, angle:0.000000
          Equivalent rotation rel. anchored plate: lat: 77.493750, lon: 57.067142:, angle:15.711412
  
        Plate ID: 225, Fixed Plate ID: 201:
          Rotation rel. fixed (parent) plate: lat: -1.520000, lon: -62.240000:, angle:9.500000
          Equivalent rotation rel. anchored plate: lat: 59.149009, lon: -33.687205:, angle:17.238928
  
          Plate ID: 226, Fixed Plate ID: 225:
            Rotation rel. fixed (parent) plate: lat: 90.000000, lon: 0.000000:, angle:0.000000
            Equivalent rotation rel. anchored plate: lat: 59.149009, lon: -33.687205:, angle:17.238928
  
      ...
  
      Plate ID: 802, Fixed Plate ID: 701:
        Rotation rel. fixed (parent) plate: lat: 10.617614, lon: -47.371326:, angle:10.778033
        Equivalent rotation rel. anchored plate: lat: 62.066424, lon: 9.485588:, angle:-3.331182
    
        Plate ID: 511, Fixed Plate ID: 802:
          Rotation rel. fixed (parent) plate: lat: 11.359510, lon: 16.375417:, angle:-41.965910
          Equivalent rotation rel. anchored plate: lat: 14.473724, lon: 14.865304:, angle:-44.136911
    
          Plate ID: 501, Fixed Plate ID: 511:
            Rotation rel. fixed (parent) plate: lat: -5.200000, lon: 74.300000:, angle:5.930000
            Equivalent rotation rel. anchored plate: lat: 18.734696, lon: 8.570162:, angle:-41.677784
    
            Plate ID: 502, Fixed Plate ID: 501:
              Rotation rel. fixed (parent) plate: lat: 90.000000, lon: 0.000000:, angle:0.000000
              Equivalent rotation rel. anchored plate: lat: 18.734696, lon: 8.570162:, angle:-41.677784
    
            Plate ID: 510, Fixed Plate ID: 501:
              Rotation rel. fixed (parent) plate: lat: 90.000000, lon: 0.000000:, angle:0.000000
              Equivalent rotation rel. anchored plate: lat: 18.734696, lon: 8.570162:, angle:-41.677784
  
      ...

Here ``lat: 90.000000, lon: 0.000000:, angle:0.000000`` is what
:meth:`pygplates.FiniteRotation.get_lat_lon_euler_pole_and_angle_degrees` returns, by default, for
an :meth:`identity rotation<pygplates.FiniteRotation.represents_identity_rotation>`: the north
pole and an angle of zero. An identity rotation has no pole of its own, since it does not rotate.

See also
""""""""

- Primer: :ref:`pygplates_primer_plate_reconstruction_hierarchy`, :ref:`pygplates_primer_equivalent_total_rotation`,
  :ref:`pygplates_primer_relative_total_rotation`
- Reference: :meth:`pygplates.RotationModel.get_reconstruction_tree`, :meth:`pygplates.ReconstructionTree.get_anchor_plate_edges`,
  :meth:`pygplates.ReconstructionTreeEdge.get_child_edges`
- Sample code: :ref:`pygplates_plate_circuits_to_anchored_plate`, :ref:`pygplates_modify_reconstruction_pole`
